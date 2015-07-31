/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_lport.c
 * Creation Date:   April 17, 2013 from fm4000_api_lport.c.
 * Description:     Internal API to manage the glort table for the FM10000.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation.
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/**
 * MAKE_DEST_MASK
 *
 * Converts a port mask to an FM10000 destination mask.
 *
 * \param[in]       portMask points to a physical port mask.
 *
 * \return          FM10000 destination mask as a 64-bit integer.
 */
#define MAKE_DEST_MASK(portMask)                        \
    ( (((fm_uint64) (portMask)->maskWord[1]) << 32) |   \
       ((fm_uint64) (portMask)->maskWord[0]) )

/**
 * MAKE_SUB_INDEX
 *
 * Converts an (offset, length) tuple to SubIndex format.
 *
 * \param[in]       offset is the position of the index in the DGLORT.
 * \param[in]       length is the number of bits in the index.
 *
 * \return          Value of the SubIndex field.
 */
#define MAKE_SUB_INDEX(offset, length)  \
        ( (((length) & 0x0f) << 4) | ((offset) & 0x0f) )

/** Define a default static GLORT configuration for PF/VF/VMDq */
#define FIRST_VF_INDEX              0
#define LAST_VF_INDEX              63
#define FIRST_PF_INDEX             64
#define LAST_PF_INDEX              64
#define FIRST_VMDQ_INDEX           65
#define LAST_VMDQ_INDEX           255

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
/** SetPortTxTimestampMode
 * \ingroup intPort
 *
 * \desc            Sets the TxTimestampMode of a logical port belonging to
 *                  a PCIE port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port to create.
 *
 * \param[in]       txTimestampMode is the timestamp types that can be sent to
 *                  the port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_VALUE if txTimestampMode is invalid.
 * \return          FM_ERR_ALREADY_EXISTS if an Ethernet timestamp owner
 *                  already exists.
 *
 *****************************************************************************/
static fm_status SetPortTxTimestampMode(fm_int                 sw,
                                        fm_int                 port,
                                        fm_portTxTimestampMode txTimestampMode)
{
    fm10000_port *      portExt;
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm_portType         portType;
    fm_bool             isPciePort;

    err = fm10000GetLogicalPortAttribute(sw, port, FM_LPORT_TYPE, (void *)&portType);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fm10000IsPciePort(sw, port, &isPciePort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Allow only PCIE physical port or virtual port belonging to PEP. */
    if ( !( (portType == FM_PORT_TYPE_PHYSICAL && isPciePort) ||
            (portType == FM_PORT_TYPE_VIRTUAL) ||
            (portType == FM_PORT_TYPE_CPU && isPciePort) ) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    portExt   = GET_PORT_EXT(sw, port);
    switchExt = GET_SWITCH_EXT(sw);

    switch (txTimestampMode)
    {
        case FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_ANY:
            if (switchExt->ethTimestampsOwnerPort == -1)
            {
                switchExt->ethTimestampsOwnerPort = port;
                portExt->txTimestampMode = txTimestampMode;
            }
            else
            {
                err = FM_ERR_ALREADY_EXISTS;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }
            break;

        case FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_PEP:
        case FM_PORT_TX_TIMESTAMP_MODE_NONE:
            /* If mode of the port previously registered to receive ethernet
             * timestamps is changed. */
            if (portExt->txTimestampMode == FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_ANY)
            {
                switchExt->ethTimestampsOwnerPort = -1;
            }
            portExt->txTimestampMode = txTimestampMode;
            break;

        default:
            err = FM_ERR_INVALID_VALUE;
            break;
    }

ABORT:
    return err;

}    /* end SetPortTxTimestampMode */




/*****************************************************************************/
/** WriteGlortCamEntry
 * \ingroup intPort
 *
 * \desc            Writes an entry to the glort CAM/RAM.
 *
 * \note            Assumes that the caller has
 *                  (1) validated the switch,
 *                  (2) taken the switch and register locks, and
 *                  (3) suspended error checking on the GLORT CAM.
 *
 * \note            If ((camKey & camMask) != camKey) the cam entry will
 *                  be disabled.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       camEntry points to the cam entry to be written.
 *
 * \param[in]       mode specifies whether to update glort CAM, glort RAM,
 *                  or both CAM and RAM.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteGlortCamEntry(fm_int            sw,
                                    fm_glortCamEntry *camEntry,
                                    fm_camUpdateMode  mode)
{
    fm_switch * switchPtr;
    fm_uint64   rv;
    fm_uint32   cv;
    fm_uint32   destCount;
    fm_uint32   subIndexA;
    fm_uint32   subIndexB;
    fm_status   err = FM_OK;
    fm_uint32   key;
    fm_uint32   keyInvert;

    FM_LOG_DEBUG(FM_LOG_CAT_PORT, "sw=%d camIndex=%d\n", sw, camEntry->camIndex);

    switchPtr = GET_SWITCH_PTR(sw);

    /***************************************************
     * Write into the RAM first.
     **************************************************/

    if (mode != FM_UPDATE_CAM_ONLY)
    {
        if ((destCount = camEntry->destCount) == 16)
        {
            /* The destCount field is only four bits wide, so the
             * FM10000 uses 0 to represent 16. */
            destCount = 0;
        }

        rv = 0;

        subIndexA = MAKE_SUB_INDEX(camEntry->rangeAOffset,
                                   camEntry->rangeALength);

        subIndexB = MAKE_SUB_INDEX(camEntry->rangeBOffset,
                                   camEntry->rangeBLength);

        FM_SET_FIELD64(rv, FM10000_GLORT_RAM, Strict, camEntry->strict );
        FM_SET_FIELD64(rv, FM10000_GLORT_RAM, DestIndex, camEntry->destIndex );
        FM_SET_FIELD64(rv, FM10000_GLORT_RAM, RangeSubIndexA, subIndexA );
        FM_SET_FIELD64(rv, FM10000_GLORT_RAM, RangeSubIndexB, subIndexB );
        FM_SET_FIELD64(rv, FM10000_GLORT_RAM, DestCount, destCount );
        FM_SET_BIT64(rv, FM10000_GLORT_RAM, HashRotation, camEntry->hashRotation );

#if (FM10000_USE_GLORT_RAM_CACHE)
        err = fmRegCacheWriteUINT64(sw,
                                    &fm10000CacheGlortRam,
                                    camEntry->camIndex,
                                    rv);
#else
        err = switchPtr->WriteUINT64(sw,
                                     FM10000_GLORT_RAM(camEntry->camIndex, 0),
                                     rv);
#endif
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    }   /* end if (mode != FM_UPDATE_CAM_ONLY) */

    /***************************************************
     * Now write into the CAM.
     **************************************************/

    if (mode != FM_UPDATE_RAM_ONLY)
    {
        if ( ( (camEntry->camKey & camEntry->camMask) != camEntry->camKey ) ||
             ( (camEntry->camKey == 0) && (camEntry->camMask == 0) ) )
        {
            /* Invalidate the entry */
            key       = 0xFFFF;
            keyInvert = 0xFFFF;
        }
        else
        {
            fmGenerateCAMKey2(&camEntry->camKey,
                              &camEntry->camMask,
                              &key,
                              &keyInvert,
                              1);
        }

        cv = 0;
        FM_SET_FIELD(cv, FM10000_GLORT_CAM, Key, key);
        FM_SET_FIELD(cv, FM10000_GLORT_CAM, KeyInvert, keyInvert);

#if (FM10000_USE_GLORT_CAM_CACHE)
        err = fmRegCacheWriteSingle1D(sw,
                                      &fm10000CacheGlortCam,
                                      &cv,
                                      camEntry->camIndex,
                                      TRUE);
#else
        err = switchPtr->WriteUINT32(sw,
                                     FM10000_GLORT_CAM(camEntry->camIndex),
                                     cv);
#endif
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    }   /* end if (mode != FM_UPDATE_RAM_ONLY) */

ABORT:
    return err;

}   /* end WriteGlortCamEntry */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000CreateLogicalPort
 * \ingroup intPort
 *
 * \desc            Allocates and initializes a chip-specific port extension
 *                  structure.
 *                                                                      \lb\lb
 *                  Called via the CreateLogicalPort function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port to create.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if the structure could not be allocated.
 *
 *****************************************************************************/
fm_status fm10000CreateLogicalPort(fm_int sw, fm_int port)
{
#if 0
    fm_int       hogWm;
#endif
    fm_port *     portPtr;
    fm10000_port *portExt;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "sw=%d port=%d\n", sw, port);

    /***************************************************
     * Allocate the port extension structure.
     **************************************************/

    portExt = fmAlloc(sizeof(fm10000_port));
    if (portExt == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_NO_MEM);
    }

    memset(portExt, 0, sizeof(*portExt));

    portPtr = GET_PORT_PTR(sw, port);

    portPtr->extension = portExt;
    portExt->base      = portPtr;

    /***************************************************
     * Initialize function pointers.
     **************************************************/

    portPtr->InitPort         = fm10000InitPort;
    portPtr->SetPortState     = fm10000SetPortState;
    portPtr->GetPortState     = fm10000GetPortState;
    portPtr->SetPortAttribute = fm10000SetPortAttribute;
    portPtr->GetPortAttribute = fm10000GetPortAttribute;
    portPtr->UpdatePortMask   = fm10000UpdatePortMask;
    portPtr->GetNumPortLanes  = fm10000GetNumPortLanes;
    portPtr->IsPortDisabled   = fm10000IsPortDisabled;
    portPtr->SetPortSecurity  = NULL;

#if 0
    portPtr->NotifyLinkEvent   = fm10000NotifyLinkEvent,
#endif
    portPtr->SetPortQOS        = fm10000SetPortQOS;
    portPtr->GetPortQOS        = fm10000GetPortQOS;
#if 0
    portPtr->NotifyXcvrState   = NULL;
#endif

    /* Counter functions. */
    portPtr->GetPortCounters   = fm10000GetPortCounters;
    portPtr->ResetPortCounters = fm10000ResetPortCounters;

    /* VLAN support functions. */
    portPtr->SetVlanMembership = fm10000SetVlanMembership;
    portPtr->SetVlanTag        = fm10000SetVlanTag;
    portPtr->GetVlanMembership = fm10000GetVlanMembership;
    portPtr->GetVlanTag        = fm10000GetVlanTag;
    portPtr->isPciePort        = fm10000IsPciePort;
    portPtr->isSpecialPort     = fm10000IsSpecialPort;

#if 0
    for (hogWm = 0 ; hogWm < 4 ; hogWm++)
    {
        portExt->txHogWm[hogWm] = -1;
        portExt->autoTxHogWm[hogWm] = -1;
    }
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_OK);

}   /* end fm10000CreateLogicalPort */




/*****************************************************************************/
/** fm10000DbgDumpGlortTable
 * \ingroup intDebug
 *
 * \desc            Dumps the entire glort table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpGlortTable(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;
    fm_uint32   rv32;
    fm_uint64   rv64;
    fm_uint32   glort;
    fm_uint64   destMask;

    fm_int      cpi;
    fm_int      logPort;
    fm_int      physPort;
    fm_uint32   i;
    fm_uint32   j;

    fm_uint32   key;
    fm_uint32   keyInvert;
    fm_uint32   camKey;
    fm_uint32   camMask;
    fm_uint32   strict;
    fm_uint32   destCount;
    fm_uint32   subIndex;
    fm_uint32   aLength;
    fm_uint32   aStart;
    fm_uint32   bLength;
    fm_uint32   bStart;
    fm_uint32   rotation;
    fm_uint32   baseIndex;
    fm_uint32   numStrict;
    fm_uint32   numHashed;
    fm_uint32   num;
    fm_uint32   ipMcastIndex;

    static const char *strictDesc[] =
    {
        "inherited",
        "reserved",
        "not strict",
        "strict"
    };

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT64(sw, FM10000_MA_TABLE_CFG_2(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    glort = FM_GET_FIELD64(rv64,
                           FM10000_MA_TABLE_CFG_2,
                           FloodUnicastGlort);
    FM_LOG_PRINT("\tFlood Unicast Glort  : 0x%04x\n", glort);

    glort = FM_GET_FIELD64(rv64,
                           FM10000_MA_TABLE_CFG_2,
                           FloodMulticastGlort);
    FM_LOG_PRINT("\tFlood Multicast Glort: 0x%04x\n", glort);

    glort = FM_GET_FIELD64(rv64,
                           FM10000_MA_TABLE_CFG_2,
                           BroadcastGlort);
    FM_LOG_PRINT("\tBroadcast Glort      : 0x%04x\n", glort);

    for (i = 0 ; i < FM10000_GLORT_CAM_ENTRIES ; i++)
    {
#if (FM10000_USE_GLORT_CAM_CACHE)
        err = fmRegCacheReadSingle1D(sw, &fm10000CacheGlortCam, &rv32, i, TRUE);
#else
        err = switchPtr->ReadUINT32(sw, FM10000_GLORT_CAM(i), &rv32);
#endif
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

        if (rv32 == 0xffffffff)
        {
            continue;
        }

#if (FM10000_USE_GLORT_RAM_CACHE)
        err = fmRegCacheReadUINT64(sw, &fm10000CacheGlortRam, i, &rv64);
#else
        err = switchPtr->ReadUINT64(sw, FM10000_GLORT_RAM(i, 0), &rv64);
#endif
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

        FM_LOG_PRINT("GLORT_CAM[0x%04x] = 0x%08x, "
                     "GLORT_RAM[0x%04x] = 0x%016llx\n",
                     i,
                     rv32,
                     i,
                     rv64);

        key         = FM_GET_FIELD(rv32, FM10000_GLORT_CAM, Key);
        keyInvert   = FM_GET_FIELD(rv32, FM10000_GLORT_CAM, KeyInvert);

        fmGenerateCAMValueMask2(&key, &keyInvert, &camKey, &camMask, 1);

        strict      = FM_GET_FIELD64(rv64,
                                     FM10000_GLORT_RAM,
                                     Strict);

        baseIndex   = FM_GET_FIELD64(rv64,
                                     FM10000_GLORT_RAM,
                                     DestIndex );

        destCount   = FM_GET_FIELD64(rv64,
                                     FM10000_GLORT_RAM,
                                     DestCount);

        rotation    = FM_GET_BIT64(rv64,
                                   FM10000_GLORT_RAM,
                                   HashRotation);

        subIndex    = FM_GET_FIELD64(rv64,
                                     FM10000_GLORT_RAM,
                                     RangeSubIndexA);
        aLength     = subIndex >> 4;
        aStart      = subIndex & 0xFL;

        subIndex    = FM_GET_FIELD64(rv64,
                                     FM10000_GLORT_RAM,
                                     RangeSubIndexB);
        bLength     = subIndex >> 4;
        bStart      = subIndex & 0xFL;

        FM_LOG_PRINT("\tKey           : 0x%04x   (key : 0x%04x)\n",
                     key,
                     camKey);

        FM_LOG_PRINT("\tKeyInvert     : 0x%04x   (mask: 0x%04x)\n",
                     keyInvert,
                     camMask);

        FM_LOG_PRINT("\tStrict        : %d (%s)\n", strict, strictDesc[strict]);

        FM_LOG_PRINT("\tBaseIndex     : 0x%08x\n", baseIndex);

        FM_LOG_PRINT("\tDestCount     : %d\n", destCount);

        FM_LOG_PRINT("\tHashRotation  : %d\n", rotation);

        FM_LOG_PRINT("\tRangeA        : start=%d length=%d\n",
                     aStart,
                     aLength);

        FM_LOG_PRINT("\tRangeB        : start=%d length=%d\n",
                     bStart,
                     bLength);

        numStrict = 1 << (aLength + bLength);

        numHashed = destCount << aLength;
        if (numHashed == 0)
        {
            numHashed = 16;
        }

        if (strict == FM_GLORT_ENTRY_TYPE_STRICT)
        {
            num = numStrict;
        }
        else if (strict == FM_GLORT_ENTRY_TYPE_HASHED)
        {
            num = numHashed;
        }
        else
        {
            /* If strict is ISL, then the entries may be either strict or
             * hashed depending on the FTYPE, so print the larger of the
             * two numbers of dest entries. */
            num = (numStrict > numHashed) ? numStrict : numHashed;
        }

        for (j = baseIndex ; j < (baseIndex + num) ; j++)
        {
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_GLORT_DEST_TABLE(j, 0),
                                        &rv64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            if (rv64 == 0)
            {
                continue;
            }

            destMask = FM_GET_FIELD64(rv64,
                                      FM10000_GLORT_DEST_TABLE,
                                      DestMask);

            FM_LOG_PRINT("\tGLORT_DEST_TABLE[0x%04x] = 0x%016llx\n", j, rv64);

            FM_LOG_PRINT("\t\tDest mask : 0x%012llx => ", destMask);
            if ( destMask != 0 )
            {
                FM_LOG_PRINT("Port(s): ");

                for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
                {
                    fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
                    if (destMask & (1LL << physPort))
                    {
                        if (logPort == switchPtr->cpuPort)
                        {
                            FM_LOG_PRINT("CPU ");
                        }
                        else
                        {
                            FM_LOG_PRINT("%d ", logPort);
                        }
                    }
                }
                FM_LOG_PRINT("\n");
            }
            else
            {
                FM_LOG_PRINT("(no ports)\n");
            }

            ipMcastIndex = FM_GET_FIELD64(rv64,
                                          FM10000_GLORT_DEST_TABLE,
                                          IP_MulticastIndex);
            if ( ipMcastIndex != 0 )
            {
                FM_LOG_PRINT( "\t\tIP Multicast Index : %d\n",
                             ipMcastIndex );
            }
        }
    }

ABORT:
    return err;

}   /* end fm10000DbgDumpGlortTable */




/*****************************************************************************/
/** fm10000DbgDumpGlortDestTable
* \ingroup intDebug
*
* \desc             Dumps the glort dest table.
*
* \param[in]        sw is the switch on which to operate.
*
* \param[in]        raw is TRUE to dump the values in the hardware registers,
*                   or FALSE to dump the software configuration.
*
* \return           FM_OK if successful.
*
 ****************************************************************************/
fm_status fm10000DbgDumpGlortDestTable(fm_int sw, fm_bool raw)
{
    fm_switch *         switchPtr;
    fm_logicalPortInfo *lportInfo;
    fm_glortDestEntry * destEntry;
    fm_int              i;
    fm_uint64           regEntry;
    fm_status           err = FM_OK;

    FM_NOT_USED(raw);

    switchPtr = GET_SWITCH_PTR(sw);
    lportInfo = &switchPtr->logicalPortInfo;

    for (i = 0 ; i < lportInfo->numDestEntries ; ++i)
    {
        if (!raw)
        {
            destEntry = &lportInfo->destEntries[i];

            if (destEntry->usedBy)
            {
                FM_LOG_PRINT("%4d : ", i);
                FM_LOG_PRINT("destIndex=0x%04x, ", destEntry->destIndex);
                FM_LOG_PRINT("destMask=0x%04x%08x, ",
                             destEntry->destMask.maskWord[1],
                             destEntry->destMask.maskWord[0]);
                FM_LOG_PRINT("multicastIndex=0x%04x ", destEntry->multicastIndex);

                if (destEntry->owner)
                {
                    FM_LOG_PRINT("(used by 0x%x CAM entry %02x (%04x/%04x))\n",
                                 destEntry->usedBy,
                                 destEntry->owner->camIndex,
                                 destEntry->owner->camKey & 0xffff,
                                 destEntry->owner->camMask & 0xffff);
                }
                else
                {
                    FM_LOG_PRINT("(unused)\n");
                }
            }
        }
        else
        {
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_GLORT_DEST_TABLE(i, 0),
                                        &regEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            if (regEntry)
            {
                FM_LOG_PRINT("%4d : ", i);
                FM_LOG_PRINT("reg = 0x%016llx ", regEntry);
            }
        }
    }

ABORT:
    return err;

}   /* end fm10000DbgDumpGlortDestTable */




/*****************************************************************************/
/** fm10000FreeDestEntry
 * \ingroup intPort
 *
 * \desc            Frees an allocated destination table entry.
 *                  Called via the FreeDestEntry function pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in,out]   destEntry is the destination table entry to free.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeDestEntry(fm_int             sw,
                               fm_glortDestEntry *destEntry)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d destEntry=%p\n",
                  sw,
                 (void *) destEntry);

    FM_PORTMASK_DISABLE_ALL(&destEntry->destMask);
    destEntry->multicastIndex = 0;

    err = fm10000WriteDestEntry(sw, destEntry);

    memset( destEntry, 0, sizeof(*destEntry) );

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000FreeDestEntry */




/*****************************************************************************/
/** fm10000GetGlortForSpecialPort
 * \ingroup intPort
 *
 * \desc            Gets the glort number for a given special logical port.
 *                  Called via the GetGlortForSpecialPort function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[out]      glort points to caller allocated storage where the value
 *                  of the glort will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if unknown special port.
 *
 *****************************************************************************/
fm_status fm10000GetGlortForSpecialPort(fm_int sw, fm_int port, fm_int *glort)
{

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d port=%d glort=%p\n",
                 sw,
                 port,
                 (void *)glort);

    FM_NOT_USED(sw);

    switch (port)
    {
        case FM_PORT_FLOOD:
            *glort = FM10000_GLORT_FLOOD;
            break;

        case FM_PORT_BCAST:
            *glort = FM10000_GLORT_BCAST;
            break;

        case FM_PORT_MCAST:
            *glort = FM10000_GLORT_MCAST;
            break;

        case FM_PORT_DROP:
            *glort = FM10000_GLORT_DROP;
            break;

        case FM_PORT_NOP_FLOW:
            *glort = FM10000_GLORT_NOP_FLOW;
            break;

        case FM_PORT_RPF_FAILURE:
            *glort = FM10000_GLORT_RPF_FAILURE;
            break;

        default:
            /* Only support the above port types */
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_OK);

}   /* end fm10000GetGlortForSpecialPort */




/*****************************************************************************/
/** fm10000GetLogicalPortAttribute
 * \ingroup intPort
 *
 * \desc            Gets a logical port attribute.
 *                  Called via the GetLogicalPortAttribute function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[in]       attr is the logical port attribute to set.
 *
 * \param[in]       value points to caller allocated storage where the value
 *                  of the attribute will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 * \return          FM_ERR_INVALID_ATTRIB if read-only attribute.
 *
 *****************************************************************************/
fm_status fm10000GetLogicalPortAttribute(fm_int sw,
                                         fm_int port,
                                         fm_int attr,
                                         void * value)
{
    fm_port *      portPtr;
    fm10000_port * portExt;
    fm_status      err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d port=%d attr=%d value=%p\n",
                 sw,
                 port,
                 attr,
                 value);

    portPtr = GET_PORT_PTR(sw, port);

    if (!portPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_PORT);
    }

    switch (attr)
    {
        case FM_LPORT_DEST_MASK:
            *( (fm_portmask *) value ) = portPtr->destEntry->destMask;
            break;

        case FM_LPORT_MULTICAST_INDEX:
            *( (fm_uint32 *) value ) = portPtr->destEntry->multicastIndex;
            break;

        case FM_LPORT_TYPE:
            *( (fm_portType *) value ) = portPtr->portType;
            break;

        case FM_LPORT_TX_TIMESTAMP_MODE:
            portExt = GET_PORT_EXT( sw, port );
            *( (fm_portTxTimestampMode *) value ) = portExt->txTimestampMode;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000GetLogicalPortAttribute */




/*****************************************************************************/
/** fm10000GetMaxGlortsPerLag
 * \ingroup intPort
 *
 * \desc            Gets the maximum number of glorts per lag.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      maxGlorts points to caller allocated storage where the
 *                  maximum number of glorts will be written.
 *
 * \return          FM_OK always.
 *
 *****************************************************************************/
fm_status fm10000GetMaxGlortsPerLag(fm_int sw, fm_int *maxGlorts)
{

    FM_NOT_USED(sw);

    *maxGlorts = FM10000_GLORTS_PER_LAG;
    return FM_OK;

}   /* end fm10000GetMaxGlortsPerLag */




/*****************************************************************************/
/** fm10000ResetPortSettings
 * \ingroup intPort
 *
 * \desc            Resets port setting to defaults. This will be called on
 *                  switch init and when a port is removed from a LAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the logical port on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000ResetPortSettings(fm_int sw, fm_int logicalPort)
{
    fm_status    err;
    fm_int       phys_port;
    fm_switch *  switchPtr;
    fm_uint32    rv = 0;
    fm_portAttr *portAttr;
    fm_bool      regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "sw=%d port=%d\n", sw, logicalPort);

    switchPtr = GET_SWITCH_PTR(sw);
    portAttr  = GET_PORT_ATTR(sw, logicalPort);

    err = fmMapLogicalPortToPhysical(switchPtr, logicalPort, &phys_port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_FLAG_TAKE_REG_LOCK(sw);
    err = switchPtr->ReadUINT32(sw, FM10000_PORT_CFG_ISL(phys_port), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_SET_FIELD( rv,
                  FM10000_PORT_CFG_ISL,
                  srcGlort,
                  FM_GET_GLORT_NUMBER(switchPtr, logicalPort) );

    err = switchPtr->WriteUINT32(sw, FM10000_PORT_CFG_ISL(phys_port), rv);

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000ResetPortSettings */





/*****************************************************************************/
/** fm10000SetGlortDestMask
 * \ingroup intPort
 *
 * \desc            Sets the destination mask for a given glort destination
 *                  table entry.
 *                                                                      \lb\lb
 *                  Called via the SetGlortDestMask function pointer.
 *
 * \note            Assumes that the switch protection lock has already been
 *                  acquired and the switch pointer is valid.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in,out]   destEntry is the glort destination entry to be updated.
 *
 * \param[in]       destMask points to the destination mask value.
 *                  May be NULL, in which case the mask will be cleared.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetGlortDestMask(fm_int             sw,
                                  fm_glortDestEntry *destEntry,
                                  fm_portmask *      destMask)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d destIndex=%x "
                 "destMask=0x%08x %08x\n",
                 sw,
                 destEntry->destIndex,
                 (destMask) ? destEntry->destMask.maskWord[1] : 0,
                 (destMask) ? destEntry->destMask.maskWord[0] : 0);

    if (destMask)
    {
        /* Save the new destination mask. */
        destEntry->destMask = *destMask;
    }
    else
    {
        /* Clear the saved destination mask. */
        FM_PORTMASK_DISABLE_ALL(&destEntry->destMask);
    }

    err = fm10000WriteDestEntry(sw, destEntry);
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000SetGlortDestMask */




/*****************************************************************************/
/** fm10000SetLogicalPortAttribute
 * \ingroup intPort
 *
 * \desc            Sets a logical port attribute, preserving an internal
 *                  record of some attributes.
 *                                                                      \lb\lb
 *                  Called via the SetLogicalPortAttribute function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[in]       attr is the logical port attribute to set.
 *
 * \param[in]       value points to caller allocated storage where the value
 *                  of the attribute will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 * \return          FM_ERR_INVALID_ATTRIB if read-only attribute.
 *
 *****************************************************************************/
fm_status fm10000SetLogicalPortAttribute(fm_int sw,
                                         fm_int port,
                                         fm_int attr,
                                         void * value)
{
    fm_port *              portPtr;
    fm_status              err = FM_OK;
    fm_portTxTimestampMode txTimestampMode;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d port=%d attr=%d value=%p\n",
                 sw,
                 port,
                 attr,
                 value);

    portPtr = GET_PORT_PTR(sw, port);

    switch (attr)
    {
        case FM_LPORT_DEST_MASK:
            portPtr->destEntry->destMask = *( (fm_portmask *) value );
            err = fm10000WriteDestEntry(sw, portPtr->destEntry);
            break;

        case FM_LPORT_MULTICAST_INDEX:
            portPtr->destEntry->multicastIndex = *( (fm_uint32 *) value);
            err = fm10000WriteDestEntry(sw, portPtr->destEntry);
            break;

        case FM_LPORT_TYPE:
            portPtr->portType = *( (fm_portType *) value );
            break;

        case FM_LPORT_TX_TIMESTAMP_MODE:
            txTimestampMode = *( (fm_portTxTimestampMode *) value );
            err = SetPortTxTimestampMode(sw, port, txTimestampMode);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000SetLogicalPortAttribute */




/*****************************************************************************/
/** fm10000WriteDestEntry
 * \ingroup intPort
 *
 * \desc            Writes a glort destination table entry to the hardware,
 *                  setting the destination mask and IP multicast index.
 *                                                                      \lb\lb
 *                  Called internally within the FM10000 API.
 *
 * \note            Assumes that the switch protection lock has already been
 *                  acquired and the switch pointer is valid.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       destEntry points to the glort destination table entry
 *                  to be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteDestEntry(fm_int             sw,
                                fm_glortDestEntry *destEntry)
{
    fm_switch * switchPtr;
    fm_portmask physMask;
    fm_status   err;
    fm_uint64   rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d destIndex=%x "
                 "destMask=0x%08x %08x\n",
                 sw,
                 destEntry->destIndex,
                 destEntry->destMask.maskWord[1],
                 destEntry->destMask.maskWord[0]);

    switchPtr = GET_SWITCH_PTR(sw);

    if (!FM_PORTMASK_IS_ZERO(&destEntry->destMask))
    {
        /* Translate the logical port mask to a physical port mask,
         * and set the corresponding bits in the register value. */
        err = fmPortMaskLogicalToPhysical(switchPtr,
                                          &destEntry->destMask,
                                          &physMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }
    else
    {
        FM_PORTMASK_DISABLE_ALL(&physMask);
    }

    rv = 0;

    FM_SET_FIELD64(rv,
                   FM10000_GLORT_DEST_TABLE,
                   DestMask,
                   MAKE_DEST_MASK(&physMask));

    FM_SET_FIELD64(rv,
                   FM10000_GLORT_DEST_TABLE,
                   IP_MulticastIndex,
                   destEntry->multicastIndex);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_GLORT_DEST_TABLE(destEntry->destIndex, 0),
                                 rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000WriteDestEntry */




/*****************************************************************************/
/** fm10000WriteGlortCamEntry
 * \ingroup intPort
 *
 * \desc            Writes an entry to the glort CAM/RAM.
 *                  Called via the WriteGlortCamEntry function pointer.
 *
 * \note            Assumes that the switch protection lock has already been
 *                  acquired and the switch pointer is valid.
 *
 * \note            If ((camKey & camMask) != camKey) the cam entry will
 *                  be disabled.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       camEntry points to the cam entry to be written.
 *
 * \param[in]       mode specifies whether to update glort CAM, glort RAM,
 *                  or both CAM and RAM.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteGlortCamEntry(fm_int            sw,
                                    fm_glortCamEntry *camEntry,
                                    fm_camUpdateMode  mode)
{
    fm_status   err;
    fm_bool     regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d camEntry=%p mode=%d\n",
                 sw,
                 (void *) camEntry,
                 mode);


#if (FM10000_USE_GLORT_CAM_MONITOR)
    if (mode != FM_UPDATE_RAM_ONLY)
    {
        err = NotifyCRMEvent(sw, FM10000_GLORT_CAM_CRM_ID, FM10000_CRM_EVENT_SUSPEND_REQ);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }
#endif

    /* Make the Glort CAM & RAM writes atomic */
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = WriteGlortCamEntry(sw, camEntry, mode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

#if (FM10000_USE_GLORT_CAM_MONITOR)
    if (mode != FM_UPDATE_RAM_ONLY)
    {
        DROP_REG_LOCK(sw);
        regLockTaken = FALSE;

        err = NotifyCRMEvent(sw, FM10000_GLORT_CAM_CRM_ID, FM10000_CRM_EVENT_RESUME_REQ);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }
#endif

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000WriteGlortCamEntry */




/*****************************************************************************/
/** fm10000InitGlortCam
 * \ingroup intPort
 *
 * \desc            Initializes the GLORT_CAM register table to be in
 *                  always-mismatch mode at startup.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitGlortCam(fm_int sw)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_int     i;
    fm_glortCamEntry camEntry;
    fm_bool    regLockTaken = FALSE;

    switchPtr = GET_SWITCH_PTR(sw);



#if (FM10000_USE_GLORT_CAM_MONITOR)

    err = NotifyCRMEvent(sw, FM10000_GLORT_CAM_CRM_ID, FM10000_CRM_EVENT_SUSPEND_REQ);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

#endif

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    camEntry.camKey = 0xFFFF;
    camEntry.camMask = 0x0000;

    /* Invalidate all entries */
    for (i = 0 ; i < FM10000_GLORT_CAM_ENTRIES ; i++)
    {
        camEntry.camIndex = i;

        err = WriteGlortCamEntry(sw, &camEntry, FM_UPDATE_CAM_ONLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

#if (FM10000_USE_GLORT_CAM_MONITOR)

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    err = NotifyCRMEvent(sw, FM10000_GLORT_CAM_CRM_ID, FM10000_CRM_EVENT_RESUME_REQ);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

#endif

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end fm10000InitGlortCam */




/*****************************************************************************/
/** fm10000FreeLaneResources
 * \ingroup intPort
 *
 * \desc            Free lane-level resources for the FM10000 chip.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch is invalid.
 *
 *****************************************************************************/
fm_status fm10000FreeLaneResources( fm_int sw )
{
    fm10000_lane    *laneExt;
    fm10000_laneDfe *laneDfePtr;
    fm_switch       *switchPtr;
    fm_int           lane;

    /* validate the switch ID */
    if ( sw < 0 || sw >= FM_MAX_NUM_SWITCHES )
    {
        return FM_ERR_INVALID_SWITCH;
    }

    switchPtr = GET_SWITCH_PTR( sw );

    if ( switchPtr->laneTable )
    {
        for ( lane = 0 ; lane < switchPtr->laneTableSize ; lane++ )
        {
            if ( switchPtr->laneTable[lane] )
            {
                /* Delete state machines and timers associated to these lanes */
                laneExt = switchPtr->laneTable[lane]->extension;
                laneDfePtr = &laneExt->dfeExt;
                laneDfePtr->pLaneExt = laneExt;
                fmDeleteStateMachine( laneExt->smHandle );
                fmDeleteTimer( laneExt->timerHandle );
                fmDeleteStateMachine( laneDfePtr->smHandle );
                fmDeleteTimer( laneDfePtr->timerHandle );

                /* Free lane table. */
                if ( switchPtr->laneTable[lane]->extension )
                {
                    fmFree( switchPtr->laneTable[lane]->extension );
                }
                fmFree( switchPtr->laneTable[lane] );
            }
        }

        fmFree( switchPtr->laneTable );
        switchPtr->laneTable = NULL;
    }

    return FM_OK;

}   /* end fm10000FreeLaneResources */




/*****************************************************************************/
/** fm10000FreeLogicalPort
 * \ingroup intPort
 *
 * \desc            Deallocates a logical port or a block of logical ports.
 *                  Note that the number of logical ports freed is determined
 *                  by the port type.
 *                                                                      \lb\lb
 *                  Called via the FreeLogicalPort function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the logical port to free.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the logical port is invalid.
 * \return          FM_ERR_PORT_IS_IN_LAG if the logical port is currently
 *                  a LAG member.
 *
 *****************************************************************************/
fm_status fm10000FreeLogicalPort( fm_int sw, fm_int logicalPort )
{
    fm_port      *portPtr;
    fm10000_port *portExt;

    portPtr = GET_PORT_PTR( sw, logicalPort );
    if ( (portPtr != NULL)
         && ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
              || (portPtr->portType == FM_PORT_TYPE_CPU) ) )
    {
        portExt = GET_PORT_EXT( sw, logicalPort );
        fmDeleteTimer( portExt->timerHandle );
        fmDeleteTimer( portExt->pcieIntrTimerHandle );
        fmDeleteStateMachine( portExt->smHandle );

        if (portExt->anSmHandle)
        {
            fmDeleteStateMachine( portExt->anSmHandle );
        }
    }

    return fmCommonFreeLogicalPort( sw, logicalPort );

}   /* end fm10000FreeLogicalPort */




/*****************************************************************************/
/** fm10000MapVirtualGlortToLogicalPort
 * \ingroup intPort
 *
 * \desc            maps a virtual glort to the associated logical  port
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       glort is the virtual glort number on which to operate
 *
 * \param[out]      port is the pointer to a caller-allocated area where this
 *                  function will return the logical port number
 *
 * \return          FM_OK if request succeeded.
 *
 * \return          FM_ERR_NOT_FOUND if this PEP glort number was not found
 *                  in the PEP glort range.
 *
 *****************************************************************************/
fm_status fm10000MapVirtualGlortToLogicalPort(fm_int    sw,
                                              fm_uint32 glort,
                                              fm_int *  port)
{
    fm_status status;
    fm_int    i;
    fm_bool   portFound;
    fm_uint32 firstGlort;
    fm_uint32 lastGlort;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d, glort=0x%x, port = %p\n",
                 sw,
                 glort,
                 (void *) port);

    portFound  = FALSE;
    status     = FM_OK;
    firstGlort = 0;
    lastGlort  = 0;
    info       = GET_MAILBOX_INFO(sw);

    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        firstGlort = info->glortBase +
                     (i * info->glortsPerPep);
        lastGlort = info->glortBase +
                     ( (i + 1) * info->glortsPerPep);

        if ( (glort >= firstGlort) && (glort < lastGlort) )
        {
            status = fm10000MapPepToLogicalPort(sw,
                                                i,
                                                port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
            portFound = TRUE;
            break;
        }
    }

    if (!portFound)
    {
        status = FM_ERR_NOT_FOUND;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PORT, status);

} /* end fm10000MapVirtualGlortToLogicalPort */




/*****************************************************************************/
/** fm10000MapVirtualGlortToPhysicalPort
 * \ingroup intPort
 *
 * \desc            maps a virtual glort to the associated physical port
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       glort is the virtual port glort number on which to operate
 *
 * \param[out]      port is the pointer to a caller-allocated area where this
 *                  function will return the physical port number
 *
 * \return          FM_OK if request succeeded.
 *
 * \return          FM_ERR_NOT_FOUND if this PEP glort number was not found
 *                  in the PEP glort range.
 *
 *****************************************************************************/
fm_status fm10000MapVirtualGlortToPhysicalPort(fm_int    sw,
                                               fm_uint32 glort,
                                               fm_int *  port)
{
    fm_status status;
    fm_int    i;
    fm_int    portNumber;
    fm_int    physSw;
    fm_bool   portFound;
    fm_uint32 firstGlort;
    fm_uint32 lastGlort;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d, glort=0x%x, port = %p\n",
                 sw,
                 glort,
                 (void *) port);

    portFound  = FALSE;
    status     = FM_OK;
    physSw     = 0;
    portNumber = 0;
    firstGlort = 0;
    lastGlort  = 0;
    info       = GET_MAILBOX_INFO(sw);

    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        firstGlort = info->glortBase +
                     (i * info->glortsPerPep);
        lastGlort = info->glortBase +
                     ( (i + 1) * info->glortsPerPep);

        if ( (glort >= firstGlort) && (glort < lastGlort) )
        {
            status = fm10000MapPepToLogicalPort(sw,
                                                i,
                                                &portNumber);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

            status = fmPlatformMapLogicalPortToPhysical(sw,
                                                        portNumber,
                                                        &physSw,
                                                        port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

            portFound = TRUE;
            break;
        }
    }

    if (!portFound)
    {
        status = FM_ERR_NOT_FOUND;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PORT, status);

} /* end fm10000MapVirtualGlortToPhysicalPort */




/*****************************************************************************/
/** fm10000MapGlortToPepNumber
 * \ingroup intPort
 *
 * \desc            maps a virtual glort to the pep number
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       glort is the virtual port glort number on which to operate
 *
 * \param[out]      pepNb is the pointer to a caller-allocated area where this
 *                  function will return the pep number
 *
 * \return          FM_OK if request succeeded.
 *
 * \return          FM_ERR_NOT_FOUND if this VSI glort number was not found
 *                  in the PEP glort range.
 *
 *****************************************************************************/
fm_status fm10000MapGlortToPepNumber(fm_int    sw,
                                     fm_uint32 glort,
                                     fm_int *  pepNb)
{
    fm_status status;
    fm_int    i;
    fm_bool   pepFound;
    fm_uint32 firstGlort;
    fm_uint32 lastGlort;
    fm_mailboxInfo *info;

    status     = FM_OK;
    pepFound   = FALSE;
    firstGlort = 0;
    lastGlort  = 0;
    info       = GET_MAILBOX_INFO(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d, glort=0x%x, pepNb = %p\n",
                 sw,
                 glort,
                 (void *) pepNb);

    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        firstGlort = info->glortBase +
                     (i * info->glortsPerPep);
        lastGlort = info->glortBase +
                     ( (i + 1) * info->glortsPerPep);

        if ( (glort >= firstGlort) && (glort < lastGlort) )
        {
            *pepNb = i;
             pepFound = TRUE;
        }
    }

    if (!pepFound)
    {
        status = FM_ERR_NOT_FOUND;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, status);

}   /* end fm10000MapGlortToPepNumber */




/*****************************************************************************/
/** fm10000GetPcieLogicalPort
 * \ingroup intlport
 *
 * \desc            Retrieve the logical port of a PCIe queue. This function
 *                  is used to map a PEP/VF/PF/VMDq to a Virtual Logical port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pep is the PEP Id.
 *
 * \param[in]       type is the type of pcie port to retrieve.
 *
 * \param[in]       index is the queue offset to retrieve.
 *
 * \param[out]      logicalPort points to a caller-provided integer
 *                  variable where this function will return the virtual
 *                  logical port assigned for this specific entry.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_UNSUPPORTED if the switch family does not support
 *                  this functionality
 * \return          FM_ERR_INVALID_ARGUMENT if the pointer argument is invalid
 *                  or if the index is out of range.
 * \return          FM_ERR_INVALID_PORT if the pep is out of range or if the
 *                  PEP/VF/PF/VMDq does not map to a logical port.
 *
 *****************************************************************************/
fm_status fm10000GetPcieLogicalPort(fm_int sw,
                                    fm_int pep,
                                    fm_pciePortType type,
                                    fm_int index,
                                    fm_int *logicalPort)
{
    fm_status status = FM_OK;
    fm_uint32 firstGlort;
    fm_uint32 lastGlort;
    fm_uint32 virtualGlort = 0;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d, pep=%d, type=%d, index=%d, logicalPort=%p\n",
                 sw,
                 pep,
                 type,
                 index,
                 (void *) logicalPort);

    if ((pep < 0) || (pep > FM10000_MAX_PEP))
    {
        status = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

    if (logicalPort == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

    if (index < 0)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

    info = GET_MAILBOX_INFO(sw);

    firstGlort = info->glortBase +
                 (pep * info->glortsPerPep);
    lastGlort = info->glortBase +
                 ( (pep + 1) * info->glortsPerPep);

    switch (type)
    {
       case FM_PCIE_PORT_PF:
           if (index > (LAST_PF_INDEX - FIRST_PF_INDEX))
           {
               status = FM_ERR_INVALID_ARGUMENT;
               FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
           }
           virtualGlort = firstGlort + FIRST_PF_INDEX + index;
           break;

       case FM_PCIE_PORT_VF:
           if (index > (LAST_VF_INDEX - FIRST_VF_INDEX))
           {
               status = FM_ERR_INVALID_ARGUMENT;
               FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
           }
           virtualGlort = firstGlort + FIRST_VF_INDEX + index;
           break;

       case FM_PCIE_PORT_VMDQ:
           if (index > (LAST_VMDQ_INDEX - FIRST_VMDQ_INDEX))
           {
               status = FM_ERR_INVALID_ARGUMENT;
               FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
           }
           virtualGlort = firstGlort + FIRST_VMDQ_INDEX + index;
           break;

       default:
           status = FM_ERR_INVALID_ARGUMENT;
           FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
           break;
    }

    if (virtualGlort > lastGlort)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

    status = fmGetGlortLogicalPort(sw, virtualGlort, logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PORT, status);

}   /* end fm10000GetPcieLogicalPort */




/*****************************************************************************/
/** fm10000GetLogicalPortPcie
 * \ingroup intlport
 *
 * \desc            Retrieve the PCIe queue of a logical port. This function
 *                  is used to map a Virtual Logical port to a PEP/VF/PF/VMDq
 *                  entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the virtual port to map
 *
 * \param[out]      pep points to a caller-provided integer
 *                  variable where this function will return the PEP id
 *                  assigned for this specific entry.
 *
 * \param[out]      type points to a caller-provided fm_pciePortType
 *                  variable where this function will return the pcie port
 *                  type for this specific entry.
 *
 * \param[out]      index points to a caller-provided integer
 *                  variable where this function will return the index for
 *                  this specific entry.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_UNSUPPORTED if the switch family does not support
 *                  this functionality
 * \return          FM_ERR_INVALID_ARGUMENT if the pointer argument is invalid.
 * \return          FM_ERR_INVALID_PORT if the logical port is out of range or
 *                  if the logical port does not map into a PEP/VF/PF/VMDq
 *                  entry.
 *
 *****************************************************************************/
fm_status fm10000GetLogicalPortPcie(fm_int sw,
                                    fm_int logicalPort,
                                    fm_int *pep,
                                    fm_pciePortType *type,
                                    fm_int *index)
{
    fm_status status = FM_OK;
    fm_uint32 firstGlort;
    fm_uint32 lastGlort;
    fm_uint32 virtualGlort = 0;
    fm_int offset;
    fm_int i;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d, logicalPort=%d, pep=%p, type=%p, index=%p\n",
                 sw,
                 logicalPort,
                 (void *) pep,
                 (void *) type,
                 (void *) index);

    if ( (pep == NULL) || (type == NULL) || (index == NULL))
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

    status = fmGetLogicalPortGlort(sw, logicalPort, &virtualGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

    info = GET_MAILBOX_INFO(sw);

    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        firstGlort = info->glortBase +
                     (i * info->glortsPerPep);
        lastGlort = info->glortBase +
                     ( (i + 1) * info->glortsPerPep);

        if ( (virtualGlort >= firstGlort) && (virtualGlort <= lastGlort) )
        {
            break;
        }
    }

    if (i == FM10000_NUM_PEPS)
    {
        status = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

    offset = virtualGlort - firstGlort;
    *pep = i;

    if ( (offset >= FIRST_PF_INDEX) && (offset <= LAST_PF_INDEX) )
    {
        *type = FM_PCIE_PORT_PF;
        *index = offset - FIRST_PF_INDEX;
    }
    else if ( (offset >= FIRST_VF_INDEX) && (offset <= LAST_VF_INDEX) )
    {
        *type = FM_PCIE_PORT_VF;
        *index = offset - FIRST_VF_INDEX;
    }
    else if ( (offset >= FIRST_VMDQ_INDEX) && (offset <= LAST_VMDQ_INDEX) )
    {
        *type = FM_PCIE_PORT_VMDQ;
        *index = offset - FIRST_VMDQ_INDEX;
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PORT, status);

}   /* end fm10000GetLogicalPortPcie */
