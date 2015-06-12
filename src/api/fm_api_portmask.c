/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_portmask.c
 * Creation Date:   June 23, 2009.
 * Description:     Port mask data type.
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
static fm_int CountBitsInWord(fm_uint32 word);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** CountBitsInWord
 *
 * \desc            Helper function to count the number of one bits in a word.
 *
 * \param[in]       word is the value whose bits are to be counted.
 *
 * \return          The number of bits set in the word.
 *
 *****************************************************************************/
static fm_int CountBitsInWord(fm_uint32 word)
{
    int n = 0;

    while (word != 0)
    {
        n++;
        word &= (word - 1);
    }

    return n;

}   /* end CountBitsInWord */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmPortMaskEnableAll
 * \ingroup intPort
 *
 * \desc            Enables all the ports in the port mask.
 *
 * \param[out]      maskPtr points to the port mask to be modified.
 *
 * \param[in]       numPorts is the number of ports in the mask.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmPortMaskEnableAll(fm_portmask* maskPtr, fm_uint numPorts)
{
    fm_uint i;

    for (i = 0 ; i < FM_PORTMASK_NUM_WORDS ; ++i)
    {
        if (numPorts >= 32)
        {
            maskPtr->maskWord[i] = 0xffffffff;
            numPorts -= 32;
        }
        else if (numPorts != 0)
        {
            maskPtr->maskWord[i] = (1U << numPorts) - 1 ;
            numPorts = 0;
        }
        else
        {
            maskPtr->maskWord[i] = 0;
        }
    }

}   /* end fmPortMaskEnableAll */




/*****************************************************************************/
/** fmPortMaskToBitArray
 * \ingroup intPort
 *
 * \desc            Converts a port mask to a bit array.
 *
 * \param[in]       maskPtr points to the port mask to be converted.
 *
 * \param[in,out]   arrayPtr points to the bit array into which the
 *                  port mask is to be copied.
 *
 * \param[in]       numPorts is the maximum number of ports in the mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPortMaskToBitArray(fm_portmask* maskPtr,
                               fm_bitArray* arrayPtr,
                               fm_int       numPorts)
{
    fm_int      bitNo;
    fm_status   err;

    if (maskPtr == NULL || arrayPtr == NULL || 
        numPorts > FM_PORTMASK_NUM_BITS || numPorts < 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    err = fmClearBitArray(arrayPtr);
    if (err != FM_OK)
    {
        return err;
    }

    if (numPorts > arrayPtr->bitCount)
    {
        numPorts = arrayPtr->bitCount;
    }

    for (bitNo = 0 ; bitNo < numPorts; bitNo++)
    {
        if (FM_PORTMASK_IS_BIT_SET(maskPtr, bitNo))
        {
            err = fmSetBitArrayBit(arrayPtr, bitNo, 1);
            if (err != FM_OK)
            {
                return err;
            }
        }
    }

    return FM_OK;

}   /* end fmPortMaskToBitArray */




/*****************************************************************************/
/** fmBitArrayToPortMask
 * \ingroup intPort
 *
 * \desc            Converts a bit array to a port mask.
 *
 * \param[in]       arrayPtr points to the bit array to be converted.
 *
 * \param[out]      maskPtr points to the port mask into which the bit array
 *                  is to be copied.
 *
 * \param[in]       numPorts is the maximum number of ports in the mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmBitArrayToPortMask(fm_bitArray* arrayPtr,
                               fm_portmask* maskPtr,
                               fm_int       numPorts)
{
    fm_int      bitNo;
    fm_status   err = FM_OK;

    if (maskPtr == NULL || arrayPtr == NULL ||
        numPorts > FM_PORTMASK_NUM_BITS || numPorts < 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_PORTMASK_DISABLE_ALL(maskPtr);

    if (numPorts > arrayPtr->bitCount)
    {
        numPorts = arrayPtr->bitCount;
    }

    for (bitNo = 0 ; ; bitNo++)
    {
        err = fmFindBitInBitArray(arrayPtr, bitNo, TRUE, &bitNo);
        if (err != FM_OK || bitNo < 0 || bitNo >= numPorts)
        {
            break;
        }
        FM_PORTMASK_ENABLE_BIT(maskPtr, bitNo);
    }

    return err;

}   /* end fmBitArrayToPortMask */




/*****************************************************************************/
/** fmBitArrayLogicalToPhysMask
 * \ingroup intPort
 *
 * \desc            Converts a logical port bit array to a physical port mask.
 *
 * \param[in]       switchPtr points to the switch table entry.
 *
 * \param[in]       arrayPtr points to the bit array to be converted.
 *
 * \param[out]      maskPtr points to the port mask into which the bit array
 *                  is to be converted.
 *
 * \param[in]       numPorts is the maximum number of ports in the mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmBitArrayLogicalToPhysMask(fm_switch *  switchPtr,
                                      fm_bitArray* arrayPtr,
                                      fm_portmask* maskPtr,
                                      fm_int       numPorts)
{
    fm_int      cpi;
    fm_int      physPort;
    fm_status   err = FM_OK;

    if (maskPtr == NULL || arrayPtr == NULL ||
        numPorts > FM_PORTMASK_NUM_BITS || numPorts < 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_PORTMASK_DISABLE_ALL(maskPtr);

    if (numPorts > arrayPtr->bitCount)
    {
        numPorts = arrayPtr->bitCount;
    }

    for (cpi = 0 ; ; cpi++)
    {
        err = fmFindBitInBitArray(arrayPtr, cpi, TRUE, &cpi);
        if (err != FM_OK || cpi < 0 || cpi >= numPorts)
        {
            break;
        }
        physPort = switchPtr->cardinalPortInfo.portMap[cpi].physPort;
        FM_PORTMASK_ENABLE_BIT(maskPtr, physPort);
    }

    return err;

}   /* end fmBitArrayLogicalToPhysMask */




/*****************************************************************************/
/** fmPortMaskLogicalToPhysical
 * \ingroup intSwitch
 *
 * \desc            Maps logical port mask to physical port mask.
 *
 * \param[in]       switchPtr points to the switch table entry.
 *
 * \param[in]       logMask points to the logical port mask.
 *
 * \param[out]      physMask points to caller-supplied storage to receive
 *                  the physical port mask.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
fm_status fmPortMaskLogicalToPhysical(fm_switch *   switchPtr,
                                      fm_portmask * logMask,
                                      fm_portmask * physMask)
{
    fm_int      cpi;
    fm_int      physPort;
    fm_portmask newMask;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, "
                         "logMask = 0x%06x %08x %08x, "
                         "physMask = %p\n",
                         switchPtr->switchNumber,
                         logMask->maskWord[2],
                         logMask->maskWord[1],
                         logMask->maskWord[0],
                         (void *) physMask);

    FM_PORTMASK_DISABLE_ALL(&newMask);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        if (FM_PORTMASK_IS_BIT_SET(logMask, cpi))
        {
            physPort = switchPtr->cardinalPortInfo.portMap[cpi].physPort;
            FM_PORTMASK_ENABLE_BIT(&newMask, physPort);
        }
    }

    *physMask = newMask;

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmPortMaskLogicalToPhysical */




/*****************************************************************************/
/** fmPortMaskPhysicalToLogical
 * \ingroup intSwitch
 *
 * \desc            Maps physical port mask to logical port mask.
 *
 * \param[in]       switchPtr points to the switch table entry.
 *
 * \param[in]       physMask points to the physical port mask.
 *
 * \param[out]      logMask points to caller-supplied storage to receive
 *                  the logical port mask.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
fm_status fmPortMaskPhysicalToLogical(fm_switch *   switchPtr,
                                      fm_portmask * physMask,
                                      fm_portmask * logMask)
{
    fm_int      cpi;
    fm_int      physPort;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, "
                         "physMask = 0x%06x %08x %08x, "
                         "logMask = %p\n",
                         switchPtr->switchNumber,
                         physMask->maskWord[2],
                         physMask->maskWord[1],
                         physMask->maskWord[0],
                         (void *) logMask);

    FM_PORTMASK_DISABLE_ALL(logMask);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        physPort = switchPtr->cardinalPortInfo.portMap[cpi].physPort;

        if (FM_PORTMASK_IS_BIT_SET(physMask, physPort))
        {
            FM_PORTMASK_ENABLE_BIT(logMask, cpi);
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmPortMaskPhysicalToLogical */




/*****************************************************************************/
/** fmPortMaskLogicalToLinkUpMask
 * \ingroup intSwitch
 *
 * \desc            Maps logical port mask to link-up mask.
 *
 * \param[in]       switchPtr points to the switch table entry.
 *
 * \param[in]       logMask points to the logical port mask.
 *
 * \param[out]      upMask points to a caller-supplied location to receive
 *                  the link-up mask. Only those ports that were set in the
 *                  logMask and are also in a link up state will be set in
 *                  this mask.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
fm_status fmPortMaskLogicalToLinkUpMask(fm_switch *   switchPtr,
                                        fm_portmask * logMask,
                                        fm_portmask * upMask)
{
    fm_int      cpi;
    fm_int      port;
    fm_portmask um;
    fm_bool     isMgmtPort;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, "
                         "logMask = 0x%06x %08x %08x, "
                         "upMask = %p\n",
                         switchPtr->switchNumber,
                         logMask->maskWord[2],
                         logMask->maskWord[1],
                         logMask->maskWord[0],
                         (void *) upMask);

    FM_PORTMASK_DISABLE_ALL(&um);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = switchPtr->cardinalPortInfo.portMap[cpi].logPort;

        isMgmtPort = fmIsMgmtPort(switchPtr->switchNumber, port);
        
        /* MGMT ports are always available to send to */
        if ( switchPtr->portTable[port]->linkUp || isMgmtPort)
        {
            FM_PORTMASK_ENABLE_BIT(&um, cpi);
        }
    }

    FM_AND_PORTMASKS(upMask, logMask, &um);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmPortMaskLogicalToLinkUpMask */




/*****************************************************************************/
/** fmGetPortMaskCount
 * \ingroup intSwitch
 *
 * \desc            Counts the number of ports in a port mask.
 *
 * \param[in]       portMask points to the port mask.
 *
 * \param[out]      portCount points to a caller-supplied location to receive
 *                  the number of ports in the mask.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_ARGUMENT if either argument is NULL
 *
 *****************************************************************************/
fm_status fmGetPortMaskCount(fm_portmask * portMask, fm_int * portCount)
{
    fm_int i;

    if ( portMask == NULL || portCount == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *portCount = 0;

    for (i = 0 ; i < FM_PORTMASK_NUM_WORDS ; i++)
    {
        *portCount += CountBitsInWord(portMask->maskWord[i]);
    }

    return FM_OK;

}   /* end fmGetPortMaskCount */
