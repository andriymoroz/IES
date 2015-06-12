/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_acl_mapper.c
 * Creation Date:  June 28, 2013
 * Description:    ACL code related to the mapper management for FM10000.
 *
 * Copyright (c) 2013, Intel Corporation
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
#include <common/fm_version.h>

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
/** SetL4PortMapperEntry
 * \ingroup intAcl
 *
 * \desc            Set a L4 port mapper range. The L4 port must be sorted by
 *                  port number for each protocol.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       src is true to indicate FM_FM10000_MAP_L4PORT_SRC, or
 *                  false to indicate FM_FM10000_MAP_L4PORT_DST.
 *
 * \param[in]       l4PortMapValue points to the mapper range to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetL4PortMapperEntry(fm_int                sw,
                                      fm_bool               src,
                                      fm_l4PortMapperValue *l4PortMapValue)
{
    fm_byte                   slot;
    fm_int                    tmpSlot;
    fm_status                 err = FM_OK;
    fm_int                    insertSlotPos = -1;
    fm_bool                   protocolHit = FALSE;
    fm_fm10000MapL4Port       mapL4Port;
    fm_fm10000MapL4PortCfg    mapL4PortCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "src = %d, "
                 "l4PortMapValue = %p\n",
                 sw,
                 src,
                 (void*) l4PortMapValue);

    if (src)
    {
        mapL4Port = FM_FM10000_MAP_L4PORT_SRC;
    }
    else
    {
        mapL4Port = FM_FM10000_MAP_L4PORT_DST;
    }

    for (slot = 0 ; slot < FM10000_FFU_MAP_L4_SRC_ENTRIES; slot++)
    {
        err = fm10000GetMapL4Port(sw, mapL4Port, slot, &mapL4PortCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        /********************************************************************
         * Found the same protocol. Now try to insert the new range at the
         * right position. Overlapping range is not permitted.
         ********************************************************************/
        if (mapL4PortCfg.mapProt == l4PortMapValue->mappedProtocol)
        {
            /* Found the right position */
            if ( (mapL4PortCfg.lowerBound > l4PortMapValue->l4PortStart) &&
                 (insertSlotPos < 0) )
            {
                /* Range overlap, invalid range entered */
                if ( (mapL4PortCfg.lowerBound < l4PortMapValue->l4PortEnd) ||
                     (slot & 1) )
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
                }

                insertSlotPos = slot;
            }

            protocolHit = TRUE;
        }
        /********************************************************************
         * This entry should be inserted at the end of these entry with the
         * same protocol since all the range should be sort by port for each
         * protocol.
         ********************************************************************/
        else if ( protocolHit && (insertSlotPos < 0) )
        {
            insertSlotPos = slot;
        }

        if ( (mapL4PortCfg.valid == 0) && ((slot & 1) == 0) )
        {
            break;
        }
    }

    /* New protocol range, insert it at the end */
    if (insertSlotPos < 0)
    {
        insertSlotPos = slot;
    }

    if (slot > FM10000_FFU_MAP_L4_SRC_ENTRIES - 2)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
    }

    if (slot > 0)
    {
        /********************************************************************
         * Move all the entry after the inserted position by 2 using those
         * steps: Copy the entry at the index+2 and invalidate the old one.
         ********************************************************************/
        for (tmpSlot = slot - 1; tmpSlot >= insertSlotPos; tmpSlot-=2)
        {
            err = fm10000GetMapL4Port(sw, mapL4Port, tmpSlot, &mapL4PortCfg, TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            err = fm10000SetMapL4Port(sw,
                                      mapL4Port,
                                      tmpSlot + 2,
                                      &mapL4PortCfg,
                                      FM_FM10000_MAP_L4PORT_ALL,
                                      TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            err = fm10000GetMapL4Port(sw, mapL4Port, tmpSlot - 1, &mapL4PortCfg, TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            err = fm10000SetMapL4Port(sw,
                                      mapL4Port,
                                      tmpSlot + 1,
                                      &mapL4PortCfg,
                                      FM_FM10000_MAP_L4PORT_ALL,
                                      TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            mapL4PortCfg.mapL4Port = 0;
            mapL4PortCfg.lowerBound = 0;
            mapL4PortCfg.mapProt = 0;
            mapL4PortCfg.valid = FALSE;
            err = fm10000SetMapL4Port(sw,
                                      mapL4Port,
                                      tmpSlot - 1,
                                      &mapL4PortCfg,
                                      FM_FM10000_MAP_L4PORT_ALL,
                                      TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            mapL4PortCfg.mapL4Port = 0;
            mapL4PortCfg.lowerBound = 0;
            mapL4PortCfg.mapProt = 0;
            mapL4PortCfg.valid = FALSE;
            err = fm10000SetMapL4Port(sw,
                                      mapL4Port,
                                      tmpSlot,
                                      &mapL4PortCfg,
                                      FM_FM10000_MAP_L4PORT_ALL,
                                      TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            if (tmpSlot == 0)
            {
                break;
            }
        }
    }

    /************************************************************************
     * Now insert the range using the selected position. To make the range
     * effective, the start port is first entered with the proper mapped value
     * followed by the end port + 1 with the valid bit undefined.
     ************************************************************************/
    mapL4PortCfg.mapL4Port = l4PortMapValue->mappedL4PortValue;
    if (l4PortMapValue->l4PortEnd == 0xffff)
    {
        mapL4PortCfg.lowerBound = l4PortMapValue->l4PortEnd;
        mapL4PortCfg.valid = TRUE;
    }
    else
    {
        mapL4PortCfg.lowerBound = l4PortMapValue->l4PortEnd + 1;
        mapL4PortCfg.valid = FALSE;
    }
    mapL4PortCfg.mapProt = l4PortMapValue->mappedProtocol;
    err = fm10000SetMapL4Port(sw,
                              mapL4Port,
                              insertSlotPos + 1,
                              &mapL4PortCfg,
                              FM_FM10000_MAP_L4PORT_ALL,
                              TRUE);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    mapL4PortCfg.mapL4Port = l4PortMapValue->mappedL4PortValue;
    mapL4PortCfg.lowerBound = l4PortMapValue->l4PortStart;
    mapL4PortCfg.mapProt = l4PortMapValue->mappedProtocol;
    mapL4PortCfg.valid = TRUE;
    err = fm10000SetMapL4Port(sw,
                              mapL4Port,
                              insertSlotPos,
                              &mapL4PortCfg,
                              FM_FM10000_MAP_L4PORT_ALL,
                              TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end SetL4PortMapperEntry */


/*****************************************************************************/
/** ClearL4PortMapperEntry
 * \ingroup intAcl
 *
 * \desc            Delete a L4 port mapper range. The L4 port must be sorted
 *                  by port number for each protocol.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       src is true to indicate FM_FM10000_MAP_L4PORT_SRC, or
 *                  false to indicate FM_FM10000_MAP_L4PORT_DST.
 *
 * \param[in]       l4PortMapValue points to the mapper range to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ClearL4PortMapperEntry(fm_int                sw,
                                        fm_bool               src,
                                        fm_l4PortMapperValue *l4PortMapValue)
{
    fm_byte                   slot;
    fm_byte                   tmpSlot;
    fm_status                 err = FM_OK;
    fm_bool                   startOfRange = FALSE;
    fm_int                    rangeToRemovePos = -1;
    fm_uint16                 l4PortEndToClear;
    fm_fm10000MapL4Port       mapL4Port;
    fm_fm10000MapL4PortCfg    mapL4PortCfg;
    fm_bool                   endValid;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "src = %d, "
                 "l4PortMapValue = %p\n",
                 sw,
                 src,
                 (void*) l4PortMapValue);

    if (src)
    {
        mapL4Port = FM_FM10000_MAP_L4PORT_SRC;
    }
    else
    {
        mapL4Port = FM_FM10000_MAP_L4PORT_DST;
    }

    /* The range to clear must be converted in its internal value */
    if (l4PortMapValue->l4PortEnd == 0xffff)
    {
        l4PortEndToClear = 0xffff;
        endValid = TRUE;
    }
    else
    {
        l4PortEndToClear = l4PortMapValue->l4PortEnd + 1;
        endValid = FALSE;
    }

    for (slot = 0 ; slot < FM10000_FFU_MAP_L4_SRC_ENTRIES; slot++)
    {
        err = fm10000GetMapL4Port(sw, mapL4Port, slot, &mapL4PortCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        if ( !startOfRange &&
             (mapL4PortCfg.valid == 1) &&
             (mapL4PortCfg.lowerBound == l4PortMapValue->l4PortStart) &&
             (mapL4PortCfg.mapProt == l4PortMapValue->mappedProtocol) &&
             (mapL4PortCfg.mapL4Port == l4PortMapValue->mappedL4PortValue) )
        {
            startOfRange = TRUE;
        }
        else if (startOfRange)
        {
            /* Found it */
            if ( (mapL4PortCfg.valid == endValid) &&
                 (mapL4PortCfg.lowerBound == l4PortEndToClear) &&
                 (mapL4PortCfg.mapProt == l4PortMapValue->mappedProtocol) &&
                 (mapL4PortCfg.mapL4Port == l4PortMapValue->mappedL4PortValue) )
            {
                rangeToRemovePos = slot - 1;
            }
            startOfRange = FALSE;
        }
    }

    if (rangeToRemovePos == -1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NOT_FOUND);
    }

    /* Reset the two entry refered by this range */
    mapL4PortCfg.mapL4Port = 0;
    mapL4PortCfg.lowerBound = 0;
    mapL4PortCfg.mapProt = 0;
    mapL4PortCfg.valid = FALSE;
    err = fm10000SetMapL4Port(sw,
                              mapL4Port,
                              rangeToRemovePos,
                              &mapL4PortCfg,
                              FM_FM10000_MAP_L4PORT_ALL,
                              TRUE);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    err = fm10000SetMapL4Port(sw,
                              mapL4Port,
                              rangeToRemovePos + 1,
                              &mapL4PortCfg,
                              FM_FM10000_MAP_L4PORT_ALL,
                              TRUE);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }


    /* Pack all the entry to be continuous */
    for (tmpSlot = rangeToRemovePos + 2; tmpSlot < slot; tmpSlot+=2)
    {
        err = fm10000GetMapL4Port(sw, mapL4Port, tmpSlot+1, &mapL4PortCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        err = fm10000SetMapL4Port(sw,
                                  mapL4Port,
                                  tmpSlot - 1,
                                  &mapL4PortCfg,
                                  FM_FM10000_MAP_L4PORT_ALL,
                                  TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        err = fm10000GetMapL4Port(sw, mapL4Port, tmpSlot, &mapL4PortCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        err = fm10000SetMapL4Port(sw,
                                  mapL4Port,
                                  tmpSlot - 2,
                                  &mapL4PortCfg,
                                  FM_FM10000_MAP_L4PORT_ALL,
                                  TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        mapL4PortCfg.mapL4Port = 0;
        mapL4PortCfg.lowerBound = 0;
        mapL4PortCfg.mapProt = 0;
        mapL4PortCfg.valid = FALSE;
        err = fm10000SetMapL4Port(sw,
                                  mapL4Port,
                                  tmpSlot,
                                  &mapL4PortCfg,
                                  FM_FM10000_MAP_L4PORT_ALL,
                                  TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        mapL4PortCfg.mapL4Port = 0;
        mapL4PortCfg.lowerBound = 0;
        mapL4PortCfg.mapProt = 0;
        mapL4PortCfg.valid = FALSE;
        err = fm10000SetMapL4Port(sw,
                                  mapL4Port,
                                  tmpSlot + 1,
                                  &mapL4PortCfg,
                                  FM_FM10000_MAP_L4PORT_ALL,
                                  TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end ClearL4PortMapperEntry */


/*****************************************************************************/
/** SetMACMapperEntry
 * \ingroup intAcl
 *
 * \desc            Set a MAC Mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       macMapValue points to the mapper entry to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetMACMapperEntry(fm_int             sw,
                                   fm_macMapperValue *macMapValue)
{
    fm_fm10000MapMacCfg mapMacCfg;
    fm_int slot;
    fm_status err = FM_OK;
    fm_bool found = FALSE;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "macMapValue = %p\n",
                 sw,
                 (void*) macMapValue);


    /* Try to find a free slot in the proper MAC mapper. Router MACs are
       set on the first x slots while ACL Mapper are filled from the end. */
    for (slot = FM10000_FFU_MAP_MAC_ENTRIES - 1 ;
         slot >= switchPtr->maxVirtualRouters;
         slot--)
    {
        err = fm10000GetMapMac(sw, slot, &mapMacCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        /************************************************************
         * Free slot have their MAC and their mapped MAC reset
         * to 0.
         ************************************************************/
        if ( ((mapMacCfg.macAddr & FM_LITERAL_64(0xffffffffffff)) == 0) &&
             (mapMacCfg.mapMac == 0) &&
             (mapMacCfg.router == 0) )
        {
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
    }

    /* Mapped MAC value 0 can't be used. */
    if (macMapValue->mappedMac == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    mapMacCfg.macAddr = macMapValue->mac;
    mapMacCfg.mapMac = macMapValue->mappedMac;
    mapMacCfg.ignoreLength = macMapValue->ignoreLength;
    mapMacCfg.validSMAC = macMapValue->validSrcMac;
    mapMacCfg.validDMAC = macMapValue->validDstMac;
    mapMacCfg.router = 0;

    err = fm10000SetMapMac(sw,
                           slot,
                           &mapMacCfg,
                           FM_FM10000_MAP_MAC_ALL,
                           TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end SetMACMapperEntry */


/*****************************************************************************/
/** ClearMACMapperEntry
 * \ingroup intAcl
 *
 * \desc            Clear a MAC Mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       macMapValue points to the mapper entry to remove.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ClearMACMapperEntry(fm_int             sw,
                                     fm_macMapperValue *macMapValue)
{
    fm_fm10000MapMacCfg mapMacCfg;
    fm_int slot;
    fm_status err = FM_OK;
    fm_bool found = FALSE;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "macMapValue = %p\n",
                 sw,
                 (void*) macMapValue);


    /* Try to find the proper MAC mapper to remove */
    for (slot = FM10000_FFU_MAP_MAC_ENTRIES - 1 ;
         slot >= switchPtr->maxVirtualRouters;
         slot--)
    {
        err = fm10000GetMapMac(sw, slot, &mapMacCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        if ( (mapMacCfg.macAddr == macMapValue->mac) &&
             (mapMacCfg.ignoreLength == macMapValue->ignoreLength) &&
             (mapMacCfg.mapMac == macMapValue->mappedMac) &&
             (mapMacCfg.validSMAC == macMapValue->validSrcMac) &&
             (mapMacCfg.validDMAC == macMapValue->validDstMac) &&
             (mapMacCfg.router == 0) )
        {
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NOT_FOUND);
    }

    mapMacCfg.macAddr = 0;
    mapMacCfg.ignoreLength = 0;
    mapMacCfg.mapMac = 0;
    mapMacCfg.validSMAC = 0;
    mapMacCfg.validDMAC = 0;
    mapMacCfg.router = 0;

    err = fm10000SetMapMac(sw,
                           slot,
                           &mapMacCfg,
                           FM_FM10000_MAP_MAC_ALL,
                           TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end ClearMACMapperEntry */


/*****************************************************************************/
/** SetIpMapperEntry
 * \ingroup intAcl
 *
 * \desc            Set an IP Mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ipMapValue points to the mapper entry to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetIpMapperEntry(fm_int                sw,
                                  fm_ipAddrMapperValue *ipMapValue)
{
    fm_fm10000MapIpCfg mapIpCfg;
    fm_int slot;
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "ipMapValue = %p\n",
                 sw,
                 (void*) ipMapValue);


    /* Try to find a free slot in the proper IP mapper */
    for (slot = 0 ; slot < FM10000_FFU_MAP_IP_CFG_ENTRIES; slot++)
    {
        err = fm10000GetMapIp(sw, slot, &mapIpCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        /************************************************************
         * Free slot have their IP and their mapped IP reset
         * to 0.
         ************************************************************/
        if ( (mapIpCfg.ipAddr.addr[0] == 0) &&
             (mapIpCfg.mapIp == 0) )
        {
            break;
        }
    }
    if (slot == FM10000_FFU_MAP_IP_CFG_ENTRIES)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
    }

    /* Mapped IP value 0 can't be used. */
    if (ipMapValue->mappedIp == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    mapIpCfg.ipAddr = ipMapValue->ipAddr;
    mapIpCfg.ignoreLength = ipMapValue->ignoreLength;
    mapIpCfg.mapIp = ipMapValue->mappedIp;
    mapIpCfg.validSIP = ipMapValue->validSrcIp;
    mapIpCfg.validDIP = ipMapValue->validDstIp;

    err = fm10000SetMapIp(sw,
                          slot,
                          &mapIpCfg,
                          FM_FM10000_MAP_IP_ALL,
                          TRUE,
                          TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end SetIpMapperEntry */


/*****************************************************************************/
/** ClearIpMapperEntry
 * \ingroup intAcl
 *
 * \desc            Clear an IP Mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ipMapValue points to the mapper entry to remove.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ClearIpMapperEntry(fm_int                sw,
                                    fm_ipAddrMapperValue *ipMapValue)
{
    fm_fm10000MapIpCfg mapIpCfg;
    fm_int slot;
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "ipMapValue = %p\n",
                 sw,
                 (void*) ipMapValue);


    /* Try to find the proper IP mapper to remove */
    for (slot = 0 ; slot < FM10000_FFU_MAP_IP_CFG_ENTRIES; slot++)
    {
        err = fm10000GetMapIp(sw, slot, &mapIpCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        if ( (fmCompareIPAddresses(&mapIpCfg.ipAddr,
                                   &ipMapValue->ipAddr) == 0) &&
             (mapIpCfg.mapIp == ipMapValue->mappedIp) &&
             (mapIpCfg.ignoreLength == ipMapValue->ignoreLength) &&
             (mapIpCfg.validSIP == ipMapValue->validSrcIp) &&
             (mapIpCfg.validDIP == ipMapValue->validDstIp) )
        {
            break;
        }
    }
    if (slot == FM10000_FFU_MAP_IP_CFG_ENTRIES)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NOT_FOUND);
    }

    mapIpCfg.ipAddr.addr[0] = 0;
    mapIpCfg.ipAddr.isIPv6 = FALSE;
    mapIpCfg.mapIp = 0;
    mapIpCfg.ignoreLength = 0;
    mapIpCfg.validSIP = 0;
    mapIpCfg.validDIP = 0;

    err = fm10000SetMapIp(sw,
                          slot,
                          &mapIpCfg,
                          FM_FM10000_MAP_IP_ALL,
                          TRUE,
                          TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end ClearIpMapperEntry */


/*****************************************************************************/
/** SetIpLengthMapperEntry
 * \ingroup intAcl
 *
 * \desc            Set a IP length mapper range. The IP length must be sorted
 *                  by length value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ipLengthMap points to the mapper range to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetIpLengthMapperEntry(fm_int                  sw,
                                        fm_ipLengthMapperValue *ipLengthMap)
{
    fm_byte                   slot;
    fm_byte                   tmpSlot;
    fm_status                 err = FM_OK;
    fm_int                    insertSlotPos = -1;
    fm_uint16                 lastLength = 0;
    fm_byte                   lastMapLength = 0;
    fm_fm10000MapLenCfg       mapLenCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "ipLengthMap = %p\n",
                 sw,
                 (void*) ipLengthMap);

    for (slot = 2 ; slot < FM10000_FFU_MAP_LENGTH_ENTRIES; slot++)
    {
        err = fm10000GetMapLength(sw, slot, &mapLenCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        /********************************************************************
         * Try to insert the new range at the right position. Overlapping
         * range is not permitted.
         ********************************************************************/

        /* Found the right position */
        if ( (mapLenCfg.length > ipLengthMap->ipLengthStart) &&
             (insertSlotPos < 0) )
        {
            /* Range overlap, invalid range entered */
            if ( (mapLenCfg.length < ipLengthMap->ipLengthEnd) ||
                 (slot & 1) )
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
            }

            insertSlotPos = slot;
        }

        /********************************************************************
         * Empty slot could be either configured with length == mapLength == 0
         * or as the same as the previous entry.
         ********************************************************************/
        if ( (((mapLenCfg.length == 0) && (mapLenCfg.mapLength == 0)) ||
              ((mapLenCfg.length == lastLength) && mapLenCfg.mapLength == lastMapLength)) &&
             ((slot & 1) == 0) )
        {
            break;
        }
        else
        {
            lastLength = mapLenCfg.length;
            lastMapLength = mapLenCfg.mapLength;
        }
    }

    if (insertSlotPos < 0)
    {
        insertSlotPos = slot;
        /* Normal insertion at the end only takes 2 entry */
        if ( insertSlotPos > FM10000_FFU_MAP_LENGTH_ENTRIES - 2 )
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
        }
    }
    else if (slot > FM10000_FFU_MAP_LENGTH_ENTRIES - 2)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
    }

    /********************************************************************
     * Move all the entry after the inserted position by 2 using those
     * steps: Copy the entry at the index+2 and invalidate the old one.
     ********************************************************************/
    if (slot)
    {
        for (tmpSlot = slot - 1; tmpSlot >= insertSlotPos; tmpSlot--)
        {
            err = fm10000GetMapLength(sw, tmpSlot, &mapLenCfg, TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            err = fm10000SetMapLength(sw,
                                      tmpSlot + 2,
                                      &mapLenCfg,
                                      FM_FM10000_MAP_LEN_ALL,
                                      TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            if (tmpSlot == 0)
            {
                break;
            }
        }
    }

    if (ipLengthMap->ipLengthEnd == 0xffff)
    {
        mapLenCfg.length = 0xffff;
        mapLenCfg.mapLength = ipLengthMap->mappedIpLength;
    }
    else
    {
        mapLenCfg.length = ipLengthMap->ipLengthEnd + 1;
        mapLenCfg.mapLength = 0;
    }

    err = fm10000SetMapLength(sw,
                              insertSlotPos + 1,
                              &mapLenCfg,
                              FM_FM10000_MAP_LEN_ALL,
                              TRUE);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    /***********************************************************************
     * If the range entered is the last one in the mapper, repeat the latest
     * entry to the end of the mapper. This should be done to prevent
     * length == mapLength == 0 to hit at every length since they would be
     * located at the end of the mapper.
     ***********************************************************************/
    if (insertSlotPos == slot)
    {
        for (tmpSlot = insertSlotPos + 2;
             tmpSlot < FM10000_FFU_MAP_LENGTH_ENTRIES;
             tmpSlot++)
        {
            err = fm10000SetMapLength(sw,
                                      tmpSlot,
                                      &mapLenCfg,
                                      FM_FM10000_MAP_LEN_ALL,
                                      TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
        }
    }

    mapLenCfg.length = ipLengthMap->ipLengthStart;
    mapLenCfg.mapLength = ipLengthMap->mappedIpLength;
    err = fm10000SetMapLength(sw,
                              insertSlotPos,
                              &mapLenCfg,
                              FM_FM10000_MAP_LEN_ALL,
                              TRUE);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end SetIpLengthMapperEntry */


/*****************************************************************************/
/** ClearIpLengthMapperEntry
 * \ingroup intAcl
 *
 * \desc            Clear a IP length mapper range. The IP length must be
 *                  sorted by length value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ipLengthMap points to the mapper range to remove.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ClearIpLengthMapperEntry(fm_int                  sw,
                                          fm_ipLengthMapperValue *ipLengthMap)
{
    fm_byte                   slot;
    fm_byte                   tmpSlot;
    fm_status                 err = FM_OK;
    fm_bool                   startOfRange = FALSE;
    fm_int                    rangeToRemovePos = -1;
    fm_uint16                 ipLengthEndToClear;
    fm_uint16                 ipLengthMapEndToClear;
    fm_uint16                 lastLength = 0;
    fm_byte                   lastMapLength = 0;
    fm_fm10000MapLenCfg       mapLenCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "ipLengthMap = %p\n",
                 sw,
                 (void*) ipLengthMap);

    /* The range to clear must be converted in its internal value */
    if (ipLengthMap->ipLengthEnd == 0xffff)
    {
        ipLengthEndToClear = 0xffff;
        ipLengthMapEndToClear = ipLengthMap->mappedIpLength;
    }
    else
    {
        ipLengthEndToClear = ipLengthMap->ipLengthEnd + 1;
        ipLengthMapEndToClear = 0;
    }

    for (slot = 2 ; slot < FM10000_FFU_MAP_LENGTH_ENTRIES; slot++)
    {
        err = fm10000GetMapLength(sw, slot, &mapLenCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        if ( !startOfRange &&
             (mapLenCfg.length == ipLengthMap->ipLengthStart) &&
             (mapLenCfg.mapLength == ipLengthMap->mappedIpLength) )
        {
            startOfRange = TRUE;
        }
        else if (startOfRange)
        {
            /* Found it */
            if ( (mapLenCfg.length == ipLengthEndToClear) &&
                 (mapLenCfg.mapLength == ipLengthMapEndToClear) )
            {
                rangeToRemovePos = slot - 1;
            }
            startOfRange = FALSE;
        }

        /********************************************************************
         * Empty slot could be either configured with length == mapLength == 0
         * or as the same as the previous entry.
         ********************************************************************/
        if ( (((mapLenCfg.length == 0) && (mapLenCfg.mapLength == 0)) ||
              ((mapLenCfg.length == lastLength) && mapLenCfg.mapLength == lastMapLength)) &&
             ((slot & 1) == 0) )
        {
            break;
        }
        else
        {
            lastLength = mapLenCfg.length;
            lastMapLength = mapLenCfg.mapLength;
        }
    }

    if (rangeToRemovePos == -1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NOT_FOUND);
    }

    /* Pack all the entry to be continuous */
    for (tmpSlot = rangeToRemovePos + 2; tmpSlot < slot; tmpSlot++)
    {
        err = fm10000GetMapLength(sw, tmpSlot, &mapLenCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        err = fm10000SetMapLength(sw,
                                  tmpSlot - 2,
                                  &mapLenCfg,
                                  FM_FM10000_MAP_LEN_ALL,
                                  TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }
    }

    /* Process the case where range remains in the mapper */
    if (slot > 2)
    {
        err = fm10000GetMapLength(sw, slot - 3, &mapLenCfg, TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }
        tmpSlot = slot - 2;
    }
    /* Removing the latest range, clear the mapper completely */
    else
    {
        mapLenCfg.length = 0;
        mapLenCfg.mapLength = 0;
        tmpSlot = 0;
    }

    for ( ; tmpSlot < FM10000_FFU_MAP_LENGTH_ENTRIES; tmpSlot++)
    {
        err = fm10000SetMapLength(sw,
                                  tmpSlot,
                                  &mapLenCfg,
                                  FM_FM10000_MAP_LEN_ALL,
                                  TRUE);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end ClearIpLengthMapperEntry */


/*****************************************************************************/
/** ClaimMapperOwnership
 * \ingroup intAcl
 *
 * \desc            Claims ownership of the specified Mapper resource.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapResource is the mapper resource to claim.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_FFU_RES_OWNED if a mapper is already owned.
 *
 *****************************************************************************/
static fm_status ClaimMapperOwnership(fm_int                sw,
                                      fm_fm10000MapResource mapResource)
{
    fm_fm10000MapOwnerType owner;
    fm_status              err;

    err = fm10000GetMapOwnership(sw, &owner, mapResource);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_NONE)
    {
        err = fm10000SetMapOwnership(sw, FM_FM10000_MAP_OWNER_ACL, mapResource);
    }
    else if (owner != FM_FM10000_MAP_OWNER_ACL)
    {
        err = FM_ERR_FFU_RES_OWNED;
    }

    return err;

}   /* end ClaimMapperOwnership */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000AddMapperEntry
 * \ingroup intAcl
 *
 * \desc            Set a specific mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the mapper type to configure.
 *
 * \param[in]       value points to the mapper value to set.
 *
 * \param[in]       mode define if this refered entry should be cache or not.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddMapperEntry(fm_int             sw,
                                fm_mapper          mapper,
                                void *             value,
                                fm_mapperEntryMode mode)
{
    fm_sourceMapperValue *    srcMapValue;
    fm_protocolMapperValue *  protMapValue;
    fm_l4PortMapperValue *    l4PortMapValue;
    fm_macMapperValue *       macMapValue;
    fm_ethTypeValue *         ethTypeMapValue;
    fm_ipLengthMapperValue *  ipLengthMapValue;
    fm_ipAddrMapperValue *    ipAddrMapValue;
    fm_vlanMapperValue *      vlanMapValue;
    fm_switch *               switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_int                    physicalPort;
    fm_byte                   slot;
    fm_status                 err = FM_OK;
    fm_fm10000MapSrcPortCfg   mapSrcPortCfg;
    fm_fm10000MapProtCfg      mapProtCfg;
    fm_fm10000MapETypeCfg     mapETypeCfg;
    fm_fm10000MapVIDCfg       mapVIDCfg;
    fm_uint64                 key;
    fm_int                    size;
    void *                    mapperEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, mapper = %d, value = %p, mode = %d\n",
                 sw,
                 mapper,
                 value,
                 mode);

    /****************************************************************
     * Nothing to do now if this added entry should be write to the
     * hardware under the next call of fmCompileACL.
     ****************************************************************/
    if (mode == FM_MAPPER_ENTRY_MODE_CACHE)
    {
        if (switchExt->compiledAcls != NULL)
        {
            switchExt->compiledAcls->valid = FALSE;
        }
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
    }

    switch (mapper)
    {
        case FM_MAPPER_SOURCE:
            if (switchExt->appliedAcls != NULL)
            {
                /* FM_FM10000_MAP_SRC needs to be free. */
                if (((switchExt->appliedAcls->usedPortSet &
                     (1 << FM10000_ACL_PORTSET_OWNER_POS)) == 0) &&
                    (switchExt->appliedAcls->usedPortSet & 0xf) != 0)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
                }
            }
            srcMapValue = (fm_sourceMapperValue *) value;
            err = fmMapLogicalPortToPhysical(switchPtr,
                                             srcMapValue->sourcePort,
                                             &physicalPort);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            mapSrcPortCfg.mapSrc = srcMapValue->mappedSourcePortValue;
            err = fm10000SetMapSourcePort(sw,
                                          physicalPort,
                                          &mapSrcPortCfg,
                                          FM_FM10000_MAP_SRC_ID,
                                          TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            if (switchExt->appliedAcls != NULL)
            {
                switchExt->appliedAcls->usedPortSet |= (1 << FM10000_ACL_PORTSET_OWNER_POS);
                switchExt->appliedAcls->usedPortSet |= 0xf;
            }
            break;

        case FM_MAPPER_PROTOCOL:
            protMapValue = (fm_protocolMapperValue *) value;

            /* Try to find a free slot in the protocol mapper */
            for (slot = 0 ; slot < FM10000_FFU_MAP_PROT_ENTRIES; slot++)
            {
                err = fm10000GetMapProt(sw,
                                        slot,
                                        &mapProtCfg,
                                        TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }

                /************************************************************
                 * Free slot have their protocol and their mapped value reset
                 * to 0.
                 ************************************************************/
                if ( (mapProtCfg.protocol == 0) &&
                     (mapProtCfg.mapProt == 0) )
                {
                    break;
                }
            }
            if (slot == FM10000_FFU_MAP_PROT_ENTRIES)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
            }

            /* Mapped protocol 0 could not be used.*/
            if (protMapValue->mappedProtocolValue == 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
            }

            mapProtCfg.protocol = protMapValue->protocol;
            mapProtCfg.mapProt = protMapValue->mappedProtocolValue;

            err = fm10000SetMapProt(sw,
                                    slot,
                                    &mapProtCfg,
                                    FM_FM10000_MAP_PROT_ALL,
                                    TRUE);
            break;

        case FM_MAPPER_L4_SRC:
            l4PortMapValue = (fm_l4PortMapperValue *) value;
            err = SetL4PortMapperEntry(sw, TRUE, l4PortMapValue);
            break;

        case FM_MAPPER_L4_DST:
            l4PortMapValue = (fm_l4PortMapperValue *) value;
            err = SetL4PortMapperEntry(sw, FALSE, l4PortMapValue);
            break;

        case FM_MAPPER_MAC:
            macMapValue = (fm_macMapperValue *) value;
            err = SetMACMapperEntry(sw, macMapValue);
            break;

        case FM_MAPPER_ETH_TYPE:
            ethTypeMapValue = (fm_ethTypeValue *) value;

            /* Try to find a free slot in the ethertype mapper */
            for (slot = 0 ; slot < FM10000_FFU_MAP_TYPE_ENTRIES; slot++)
            {
                err = fm10000GetMapEtherType(sw,
                                             slot,
                                             &mapETypeCfg,
                                             TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }

                /************************************************************
                 * Free slot have their etherType and their mapped value reset
                 * to 0.
                 ************************************************************/
                if ( (mapETypeCfg.ethType == 0) &&
                     (mapETypeCfg.mapType == 0) )
                {
                    break;
                }
            }
            if (slot == FM10000_FFU_MAP_TYPE_ENTRIES)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_FFU_RES_FOUND);
            }

            /* Mapped ethertype 0 should not be used. */
            if (ethTypeMapValue->mappedEthType == 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
            }

            mapETypeCfg.ethType = ethTypeMapValue->ethType;
            mapETypeCfg.mapType = ethTypeMapValue->mappedEthType;

            err = fm10000SetMapEtherType(sw,
                                         slot,
                                         &mapETypeCfg,
                                         FM_FM10000_MAP_ETYPE_ALL,
                                         TRUE);
            break;

        case FM_MAPPER_IP_LENGTH:
            ipLengthMapValue = (fm_ipLengthMapperValue *) value;
            err = SetIpLengthMapperEntry(sw, ipLengthMapValue);
            break;

        case FM_MAPPER_IP_ADDR:
            ipAddrMapValue = (fm_ipAddrMapperValue *) value;
            err = SetIpMapperEntry(sw, ipAddrMapValue);
            break;

        case FM_MAPPER_VLAN:
            vlanMapValue = (fm_vlanMapperValue *) value;

            mapVIDCfg.mapVid = vlanMapValue->mappedVlanId;
            err = fm10000SetMapVID(sw,
                                   vlanMapValue->vlanId,
                                   &mapVIDCfg,
                                   FM_FM10000_MAP_VID_MAPPEDVID,
                                   TRUE);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    if ((err == FM_OK) &&
        (switchExt->appliedAcls != NULL))
    {
        err = fmGetMapperKeyAndSize(sw, mapper, value, &key, &size);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        err = fmTreeFind(&switchExt->appliedAcls->mappers,
                         key,
                         &mapperEntry);
        if (err == FM_ERR_NOT_FOUND)
        {
            mapperEntry = fmAlloc(size);
            if (mapperEntry == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_MEM);
            }

            err = fmTreeInsert(&switchExt->appliedAcls->mappers,
                               key,
                               mapperEntry);
            if (err != FM_OK)
            {
                fmFree(mapperEntry);
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
        }
        else if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        FM_MEMCPY_S(mapperEntry, size, value, size);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000AddMapperEntry */


/*****************************************************************************/
/** fm10000DeleteMapperEntry
 * \ingroup intAcl
 *
 * \desc            Delete a specific mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the mapper type to clear.
 *
 * \param[in]       value points to the mapper value to clear.
 *
 * \param[in]       mode define if this refered entry should be cache or not.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteMapperEntry(fm_int             sw,
                                   fm_mapper          mapper,
                                   void *             value,
                                   fm_mapperEntryMode mode)
{
    fm_sourceMapperValue *    srcMapValue;
    fm_protocolMapperValue *  protMapValue;
    fm_l4PortMapperValue *    l4PortMapValue;
    fm_macMapperValue *       macMapValue;
    fm_ethTypeValue *         ethTypeMapValue;
    fm_ipLengthMapperValue *  ipLengthMapValue;
    fm_ipAddrMapperValue *    ipAddrMapValue;
    fm_vlanMapperValue *      vlanMapValue;
    fm_switch *               switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_int                    physicalPort;
    fm_byte                   slot;
    fm_status                 err = FM_OK;
    fm_fm10000MapSrcPortCfg   mapSrcPortCfg;
    fm_fm10000MapProtCfg      mapProtCfg;
    fm_fm10000MapETypeCfg     mapETypeCfg;
    fm_fm10000MapVIDCfg       mapVIDCfg;
    fm_treeIterator           it;
    fm_uint64                 nextKey;
    void *                    nextValue;
    fm_bool                   found;
    fm_uint64                 key;
    fm_int                    size;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, mapper = %d, value = %p, mode = %d\n",
                 sw,
                 mapper,
                 value,
                 mode);

    /****************************************************************
     * Nothing to do now if this deleted entry should be remove from the
     * hardware under the next call of fmCompileACL.
     ****************************************************************/
    if (mode == FM_MAPPER_ENTRY_MODE_CACHE)
    {
        if (switchExt->compiledAcls != NULL)
        {
            switchExt->compiledAcls->valid = FALSE;
        }
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
    }

    switch (mapper)
    {
        case FM_MAPPER_SOURCE:
            srcMapValue = (fm_sourceMapperValue *) value;

            err = fmMapLogicalPortToPhysical(switchPtr,
                                             srcMapValue->sourcePort,
                                             &physicalPort);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            mapSrcPortCfg.mapSrc = 0;
            err = fm10000SetMapSourcePort(sw,
                                          physicalPort,
                                          &mapSrcPortCfg,
                                          FM_FM10000_MAP_SRC_ID,
                                          TRUE);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }

            if (switchExt->appliedAcls != NULL)
            {
                /* Can the mapper return the ownership of the SRC mapper to the
                 * ACL subsystem? */
                found = FALSE;
                for (fmTreeIterInit(&it, &switchPtr->aclInfo.mappers) ;
                     ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
                {
                    if (nextKey >> FM_MAPPER_TYPE_KEY_POS == FM_MAPPER_SOURCE)
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (err == FM_ERR_NO_MORE)
                {
                    /* this means the iteration has ended normally */
                    err = FM_OK;
                }
                if (found == FALSE)
                {
                    switchExt->appliedAcls->usedPortSet &= ~(1 << FM10000_ACL_PORTSET_OWNER_POS);
                    switchExt->appliedAcls->usedPortSet &= ~0xf;
                }
            }
            break;

        case FM_MAPPER_PROTOCOL:
            protMapValue = (fm_protocolMapperValue *) value;

            /* Try to find the right slot for this mapper entry */
            for (slot = 0 ; slot < FM10000_FFU_MAP_PROT_ENTRIES; slot++)
            {
                err = fm10000GetMapProt(sw,
                                        slot,
                                        &mapProtCfg,
                                        TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }

                if ( (mapProtCfg.protocol == protMapValue->protocol) &&
                     (mapProtCfg.mapProt == protMapValue->mappedProtocolValue) )
                {
                    break;
                }
            }
            if (slot == FM10000_FFU_MAP_PROT_ENTRIES)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NOT_FOUND);
            }

            mapProtCfg.protocol = 0;
            mapProtCfg.mapProt = 0;

            err = fm10000SetMapProt(sw,
                                    slot,
                                    &mapProtCfg,
                                    FM_FM10000_MAP_PROT_ALL,
                                    TRUE);
            break;

        case FM_MAPPER_L4_SRC:
            l4PortMapValue = (fm_l4PortMapperValue *) value;
            err = ClearL4PortMapperEntry(sw, TRUE, l4PortMapValue);
            break;

        case FM_MAPPER_L4_DST:
            l4PortMapValue = (fm_l4PortMapperValue *) value;
            err = ClearL4PortMapperEntry(sw, FALSE, l4PortMapValue);
            break;

        case FM_MAPPER_MAC:
            macMapValue = (fm_macMapperValue *) value;
            err = ClearMACMapperEntry(sw, macMapValue);
            break;

        case FM_MAPPER_ETH_TYPE:
            ethTypeMapValue = (fm_ethTypeValue *) value;

            /* Try to find a the ethertype mapper entry to remove */
            for (slot = 0 ; slot < FM10000_FFU_MAP_TYPE_ENTRIES; slot++)
            {
                err = fm10000GetMapEtherType(sw,
                                             slot,
                                             &mapETypeCfg,
                                             TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }

                if ( (mapETypeCfg.ethType == ethTypeMapValue->ethType) &&
                     (mapETypeCfg.mapType == ethTypeMapValue->mappedEthType) )
                {
                    break;
                }
            }
            if (slot == FM10000_FFU_MAP_TYPE_ENTRIES)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NOT_FOUND);
            }

            mapETypeCfg.ethType = 0;
            mapETypeCfg.mapType = 0;

            err = fm10000SetMapEtherType(sw,
                                         slot,
                                         &mapETypeCfg,
                                         FM_FM10000_MAP_ETYPE_ALL,
                                         TRUE);
            break;

        case FM_MAPPER_IP_LENGTH:
            ipLengthMapValue = (fm_ipLengthMapperValue *) value;
            err = ClearIpLengthMapperEntry(sw, ipLengthMapValue);
            break;

        case FM_MAPPER_IP_ADDR:
            ipAddrMapValue = (fm_ipAddrMapperValue *) value;
            err = ClearIpMapperEntry(sw, ipAddrMapValue);
            break;

        case FM_MAPPER_VLAN:
            vlanMapValue = (fm_vlanMapperValue *) value;

            mapVIDCfg.mapVid = 0;
            err = fm10000SetMapVID(sw,
                                   vlanMapValue->vlanId,
                                   &mapVIDCfg,
                                   FM_FM10000_MAP_VID_MAPPEDVID,
                                   TRUE);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    if ((err == FM_OK) &&
        (switchExt->appliedAcls != NULL))
    {
        err = fmGetMapperKeyAndSize(sw, mapper, value, &key, &size);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        err = fmTreeRemove(&switchExt->appliedAcls->mappers,
                           key,
                           fmFree);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000DeleteMapperEntry */


/*****************************************************************************/
/** fm10000ClearMapper
 * \ingroup intAcl
 *
 * \desc            Clear all the entry of a mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the mapper type to clear.
 *
 * \param[in]       mode define if this clearing action should be cache or not.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ClearMapper(fm_int             sw,
                             fm_mapper          mapper,
                             fm_mapperEntryMode mode)
{
    fm_uint16  slot;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_fm10000MapSrcPortCfg   mapSrcPortCfg;
    fm_fm10000MapProtCfg      mapProtCfg;
    fm_fm10000MapETypeCfg     mapETypeCfg;
    fm_fm10000MapL4PortCfg    mapL4PortCfg;
    fm_fm10000MapMacCfg       mapMacCfg;
    fm_fm10000MapLenCfg       mapLenCfg;
    fm_fm10000MapIpCfg        mapIpCfg;
    fm_fm10000MapVIDCfg       mapVIDCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, mapper = %d, mode = %d\n",
                 sw,
                 mapper,
                 mode);

    /****************************************************************
     * Nothing to do now if this added entry should be write to the
     * hardware under the next call of fmCompileACL.
     ****************************************************************/
    if (mode == FM_MAPPER_ENTRY_MODE_CACHE)
    {
        if (switchExt->compiledAcls != NULL)
        {
            switchExt->compiledAcls->valid = FALSE;
        }
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
    }

    switch (mapper)
    {
        case FM_MAPPER_SOURCE:
            mapSrcPortCfg.mapSrc = 0;
            for (slot = 0 ; slot < FM10000_FFU_MAP_SRC_ENTRIES; slot++)
            {
                err = fm10000SetMapSourcePort(sw,
                                              slot,
                                              &mapSrcPortCfg,
                                              FM_FM10000_MAP_SRC_ID,
                                              TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
                if (switchExt->appliedAcls != NULL)
                {
                    switchExt->appliedAcls->usedPortSet &= ~(1 << FM10000_ACL_PORTSET_OWNER_POS);
                    switchExt->appliedAcls->usedPortSet &= ~0xf;
                }
            }
            break;

        case FM_MAPPER_PROTOCOL:
            mapProtCfg.protocol = 0;
            mapProtCfg.mapProt = 0;
            for (slot = 0 ; slot < FM10000_FFU_MAP_PROT_ENTRIES; slot++)
            {
                err = fm10000SetMapProt(sw,
                                        slot,
                                        &mapProtCfg,
                                        FM_FM10000_MAP_PROT_ALL,
                                        TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_L4_SRC:
            mapL4PortCfg.mapL4Port = 0;
            mapL4PortCfg.lowerBound = 0;
            mapL4PortCfg.mapProt = 0;
            mapL4PortCfg.valid = FALSE;
            for (slot = 0 ; slot < FM10000_FFU_MAP_L4_SRC_ENTRIES; slot++)
            {
                err = fm10000SetMapL4Port(sw,
                                          FM_FM10000_MAP_L4PORT_SRC,
                                          slot,
                                          &mapL4PortCfg,
                                          FM_FM10000_MAP_L4PORT_ALL,
                                          TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_L4_DST:
            mapL4PortCfg.mapL4Port = 0;
            mapL4PortCfg.lowerBound = 0;
            mapL4PortCfg.mapProt = 0;
            mapL4PortCfg.valid = FALSE;
            for (slot = 0 ; slot < FM10000_FFU_MAP_L4_DST_ENTRIES; slot++)
            {
                err = fm10000SetMapL4Port(sw,
                                          FM_FM10000_MAP_L4PORT_DST,
                                          slot,
                                          &mapL4PortCfg,
                                          FM_FM10000_MAP_L4PORT_ALL,
                                          TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_MAC:
            mapMacCfg.macAddr = 0;
            mapMacCfg.mapMac = 0;
            mapMacCfg.ignoreLength = 0;
            mapMacCfg.validSMAC = 0;
            mapMacCfg.validDMAC = 0;
            mapMacCfg.router = 0;
            for (slot = switchPtr->maxVirtualRouters ; 
                 slot < FM10000_FFU_MAP_MAC_ENTRIES ;
                 slot++)
            {
                err = fm10000SetMapMac(sw,
                                       slot,
                                       &mapMacCfg,
                                       FM_FM10000_MAP_MAC_ALL,
                                       TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_ETH_TYPE:
            mapETypeCfg.ethType = 0;
            mapETypeCfg.mapType = 0;
            for (slot = 0 ; slot < FM10000_FFU_MAP_TYPE_ENTRIES; slot++)
            {
                err = fm10000SetMapEtherType(sw,
                                             slot,
                                             &mapETypeCfg,
                                             FM_FM10000_MAP_ETYPE_ALL,
                                             TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_IP_LENGTH:
            mapLenCfg.length = 0;
            mapLenCfg.mapLength = 0;
            for (slot = 0 ; slot < FM10000_FFU_MAP_LENGTH_ENTRIES; slot++)
            {
                err = fm10000SetMapLength(sw,
                                          slot,
                                          &mapLenCfg,
                                          FM_FM10000_MAP_LEN_ALL,
                                          TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_IP_ADDR:
            mapIpCfg.ipAddr.addr[0] = 0;
            mapIpCfg.ipAddr.isIPv6 = FALSE;
            mapIpCfg.mapIp = 0;
            mapIpCfg.ignoreLength = 0;
            mapIpCfg.validSIP = 0;
            mapIpCfg.validDIP = 0;
            for (slot = 0 ; slot < FM10000_FFU_MAP_IP_CFG_ENTRIES; slot++)
            {
                err = fm10000SetMapIp(sw,
                                      slot,
                                      &mapIpCfg,
                                      FM_FM10000_MAP_IP_ALL,
                                      TRUE,
                                      TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        case FM_MAPPER_VLAN:
            mapVIDCfg.mapVid = 0;
            for (slot = 0 ; slot < FM10000_FFU_MAP_VLAN_ENTRIES; slot++)
            {
                err = fm10000SetMapVID(sw,
                                      slot,
                                      &mapVIDCfg,
                                      FM_FM10000_MAP_VID_MAPPEDVID,
                                      TRUE);
                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
            }
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    if ((err == FM_OK) &&
        (switchExt->appliedAcls != NULL))
    {
        fmTreeDestroy(&switchExt->appliedAcls->mappers, fmFree);
        fmTreeInit(&switchExt->appliedAcls->mappers);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ClearMapper */


/*****************************************************************************/
/** fm10000GetMapperSize
 * \ingroup intAcl
 *
 * \desc            Get the size of any mapper type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the mapper type selected.
 *
 * \param[in]       mapperSize points to caller-allocated storage where this
 *                  function should place the mapper size.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetMapperSize(fm_int     sw,
                               fm_mapper  mapper,
                               fm_uint32 *mapperSize)
{
    fm_status  err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, mapper = %d, mapperSize = %p\n",
                 sw,
                 mapper,
                 (void *)mapperSize);

    switch (mapper)
    {
        case FM_MAPPER_SOURCE:
            *mapperSize = FM10000_FFU_MAP_SRC_ENTRIES;
            break;

        case FM_MAPPER_PROTOCOL:
            *mapperSize = FM10000_FFU_MAP_PROT_ENTRIES;
            break;

        /* Each range take 2 entries */
        case FM_MAPPER_L4_SRC:
            *mapperSize = FM10000_FFU_MAP_L4_SRC_ENTRIES / 2;
            break;

        /* Each range take 2 entries */
        case FM_MAPPER_L4_DST:
            *mapperSize = FM10000_FFU_MAP_L4_DST_ENTRIES / 2;
            break;

        case FM_MAPPER_MAC:
            *mapperSize = FM10000_FFU_MAP_MAC_ENTRIES;
            break;

        case FM_MAPPER_ETH_TYPE:
            *mapperSize = FM10000_FFU_MAP_TYPE_ENTRIES;
            break;

        /* Each range take 2 entries */
        case FM_MAPPER_IP_LENGTH:
            *mapperSize = (FM10000_FFU_MAP_LENGTH_ENTRIES - 1) / 2;
            break;

        case FM_MAPPER_IP_ADDR:
            *mapperSize = FM10000_FFU_MAP_IP_CFG_ENTRIES;
            break;

        case FM_MAPPER_VLAN:
            *mapperSize = FM10000_FFU_MAP_VLAN_ENTRIES;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000GetMapperSize */


/*****************************************************************************/
/** fm10000GetMapperL4PortKey
 * \ingroup intAcl
 *
 * \desc            Fill the key value of a specific L4 port mapper structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the mapper type selected.
 *
 * \param[in]       portMapValue points to the filled structure to convert in
 *                  a key value.
 *
 * \param[out]      key points to a caller-allocated 64 bit unsigned integer
 *                  type used to return the computed key value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetMapperL4PortKey(fm_int     sw,
                                    fm_mapper  mapper,
                                    fm_l4PortMapperValue *portMapValue,
                                    fm_uint64 *key)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, mapper = %d, portMapValue = %p\n",
                 sw,
                 mapper,
                 (void *)portMapValue);

    *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
           FM_LITERAL_U64(portMapValue->mappedProtocol & 0xff) <<
                                        FM_MAPPER_L4_PROT_MAP_KEY_POS |
           FM_LITERAL_U64(portMapValue->l4PortStart & 0xffff) <<
                                        FM_MAPPER_L4_PORT_START_KEY_POS |
           FM_LITERAL_U64(portMapValue->l4PortEnd & 0xffff) <<
                                        FM_MAPPER_L4_PORT_END_KEY_POS;

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

}   /* end fm10000GetMapperL4PortKey */


/*****************************************************************************/
/** fm10000VerifyMappers
 * \ingroup intAcl
 *
 * \desc            Verify that mapper entry entered using fmAddMapperEntry
 *                  are valid and inform the caller if the source ID2 mapper
 *                  is required.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[out]      srcMap points to a caller-allocated boolean type that is
 *                  being used to inform the caller about the utilization of
 *                  the source mapper.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_FFU_RES_OWNED if a mapper is already owned.
 *
 *****************************************************************************/
fm_status fm10000VerifyMappers(fm_int               sw,
                               fm_aclErrorReporter *errReport,
                               fm_bool *            srcMap)
{
    fm_switch*              switchPtr;
    fm_treeIterator         it;
    fm_uint64               nextKey;
    void *                  nextValue;
    fm_status               err;
    fm_sourceMapperValue *  srcMapValue;
    fm_protocolMapperValue *protMapValue;
    fm_l4PortMapperValue *  l4PortMapValue;
    fm_macMapperValue *     macMapValue;
    fm_ethTypeValue *       ethTypeMapValue;
    fm_ipLengthMapperValue *ipLengthMapValue;
    fm_ipAddrMapperValue *  ipAddrMapValue;
    fm_vlanMapperValue *    vlanMapValue;
    fm_int                  numProtocolMapEntry = 0;
    fm_int                  prevSrcProt = 0;
    fm_int                  prevSrcEnd = 0;
    fm_int                  numL4SrcRange = 0;
    fm_int                  prevDstProt = 0;
    fm_int                  prevDstEnd = 0;
    fm_int                  numL4DstRange = 0;
    fm_int                  numMacMapEntry = 0;
    fm_int                  numEthTypeMapEntry = 0;
    fm_int                  numIpLengthRange = 0;
    fm_int                  numIpAddrMapEntry = 0;
    fm_int                  prevIpLengthEnd = 0;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    *srcMap = FALSE;

    for (fmTreeIterInit(&it, &switchPtr->aclInfo.mappers) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        switch(nextKey >> FM_MAPPER_TYPE_KEY_POS)
        {
            case FM_MAPPER_SOURCE:
                srcMapValue = (fm_sourceMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_SRC_PORT);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if(!fmIsCardinalPort(sw, srcMapValue->sourcePort))
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_SOURCE entry with out of "
                                           "range sourcePort (%d)\n",
                                           srcMapValue->sourcePort);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                *srcMap = TRUE;

                break;

            case FM_MAPPER_PROTOCOL:
                protMapValue = (fm_protocolMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_PROT);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if( (protMapValue->mappedProtocolValue == 0) ||
                    (protMapValue->mappedProtocolValue >
                     FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_PROT, MAP_PROT)) )
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_PROTOCOL entry with "
                                           "invalid mappedProtocolValue (%d)\n",
                                           protMapValue->mappedProtocolValue);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                numProtocolMapEntry++;

                break;

            case FM_MAPPER_L4_SRC:
                l4PortMapValue = (fm_l4PortMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_L4PORT_SRC);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if (prevSrcProt != l4PortMapValue->mappedProtocol)
                {
                    prevSrcProt = l4PortMapValue->mappedProtocol;
                    prevSrcEnd = 0;
                }

                if (prevSrcEnd > l4PortMapValue->l4PortStart)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_L4_SRC entry don't "
                                           "support port overlap for any "
                                           "defined map protocol (%d)\n",
                                           l4PortMapValue->mappedProtocol);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                if (l4PortMapValue->l4PortStart > l4PortMapValue->l4PortEnd)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_L4_SRC invalid entry "
                                           "detected, l4PortEnd needs to be "
                                           "greater or equal to l4PortStart\n");
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                prevSrcEnd = l4PortMapValue->l4PortEnd;
                numL4SrcRange++;
                break;

            case FM_MAPPER_L4_DST:
                l4PortMapValue = (fm_l4PortMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_L4PORT_DST);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if (prevDstProt != l4PortMapValue->mappedProtocol)
                {
                    prevDstProt = l4PortMapValue->mappedProtocol;
                    prevDstEnd = 0;
                }

                if (prevDstEnd > l4PortMapValue->l4PortStart)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_L4_DST entry don't "
                                           "support port overlap for any "
                                           "defined map protocol (%d)\n",
                                           l4PortMapValue->mappedProtocol);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                if (l4PortMapValue->l4PortStart > l4PortMapValue->l4PortEnd)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_L4_DST invalid entry "
                                           "detected, l4PortEnd needs to be "
                                           "greater or equal to l4PortStart\n");
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                prevDstEnd = l4PortMapValue->l4PortEnd;
                numL4DstRange++;
                break;

            case FM_MAPPER_MAC:
                macMapValue = (fm_macMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_MAC);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if ( (macMapValue->mappedMac == 0) ||
                     (macMapValue->mappedMac >
                      FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_MAC, MAP_MAC)) )
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_MAC entry with "
                                           "invalid mappedMac (%d)\n",
                                           macMapValue->mappedMac);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                numMacMapEntry++;
                break;

            case FM_MAPPER_ETH_TYPE:
                ethTypeMapValue = (fm_ethTypeValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_ETHTYPE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if ( (ethTypeMapValue->mappedEthType == 0) ||
                     (ethTypeMapValue->mappedEthType >
                      FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_TYPE, MAP_TYPE)) )
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_ETH_TYPE entry with "
                                           "invalid mappedEthType (%d)\n",
                                           ethTypeMapValue->mappedEthType);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                numEthTypeMapEntry++;
                break;

            case FM_MAPPER_IP_LENGTH:
                ipLengthMapValue = (fm_ipLengthMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_LENGTH_COMPARE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if ( (ipLengthMapValue->mappedIpLength == 0) ||
                     (ipLengthMapValue->mappedIpLength >
                      FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_LENGTH, MAP_LENGTH)) )
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_IP_LENGTH entry with "
                                           "invalid mappedIpLength (%d)\n",
                                           ipLengthMapValue->mappedIpLength);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                if (prevIpLengthEnd > ipLengthMapValue->ipLengthStart)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_IP_LENGTH entry don't "
                                           "support length overlap for any "
                                           "defined range\n");
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                if (ipLengthMapValue->ipLengthStart > ipLengthMapValue->ipLengthEnd)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_IP_LENGTH invalid entry "
                                           "detected, ipLengthEnd needs to be "
                                           "greater or equal to ipLengthStart\n");
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                prevIpLengthEnd = ipLengthMapValue->ipLengthEnd;
                numIpLengthRange++;
                break;

            case FM_MAPPER_IP_ADDR:
                ipAddrMapValue = (fm_ipAddrMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_IP);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if ( (ipAddrMapValue->mappedIp == 0) ||
                     (ipAddrMapValue->mappedIp >
                      FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_IP_CFG, MAP_IP)) )
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_IP_ADDR entry with "
                                           "invalid mappedIp (%d)\n",
                                           ipAddrMapValue->mappedIp);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                if (ipAddrMapValue->ignoreLength > 128)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_IP_ADDR entry with "
                                           "invalid ignoreLength (%d)\n",
                                           ipAddrMapValue->ignoreLength);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                numIpAddrMapEntry++;
                break;

            case FM_MAPPER_VLAN:
                vlanMapValue = (fm_vlanMapperValue *) nextValue;
                err = ClaimMapperOwnership(sw, FM_FM10000_MAP_RESOURCE_VID);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if(vlanMapValue->vlanId >= FM10000_FFU_MAP_VLAN_ENTRIES)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_VLAN entry with out of "
                                           "range vlanId (%d)\n",
                                           vlanMapValue->vlanId);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else if(vlanMapValue->mappedVlanId >= FM10000_FFU_MAP_VLAN_ENTRIES)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "FM_MAPPER_VLAN entry with out of "
                                           "range mappedVlanId (%d)\n",
                                           vlanMapValue->mappedVlanId);
                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                break;

            default:
                fm10000FormatAclStatus(errReport, TRUE,
                                       "Undefined mapper type configured "
                                       "(%lld)\n",
                                       nextKey >> FM_MAPPER_TYPE_KEY_POS);
                err = FM_ERR_ACL_COMPILE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                break;
        }
    }
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    if ( (numL4SrcRange > (FM10000_FFU_MAP_L4_SRC_ENTRIES / 2)) ||
         (numL4DstRange > (FM10000_FFU_MAP_L4_DST_ENTRIES / 2)) )
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "Too many L4 ports ranges defined in the "
                               "mapper\n");
        err = FM_ERR_ACL_COMPILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (numProtocolMapEntry > FM10000_FFU_MAP_PROT_ENTRIES)
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "Too many protocols defined in the mapper\n");
        err = FM_ERR_ACL_COMPILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ( numMacMapEntry > (FM10000_FFU_MAP_MAC_ENTRIES - switchPtr->maxVirtualRouters) )
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "Too many MAC defined in the mapper\n");
        err = FM_ERR_ACL_COMPILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (numEthTypeMapEntry > FM10000_FFU_MAP_TYPE_ENTRIES)
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "Too many Ethernet type defined in the mapper\n");
        err = FM_ERR_ACL_COMPILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (numIpLengthRange > ((FM10000_FFU_MAP_LENGTH_ENTRIES - 1) / 2))
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "Too many IP length range defined in the mapper\n");
        err = FM_ERR_ACL_COMPILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (numIpAddrMapEntry > FM10000_FFU_MAP_IP_CFG_ENTRIES)
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "Too many IP address defined in the mapper\n");
        err = FM_ERR_ACL_COMPILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000VerifyMappers */


/*****************************************************************************/
/** fm10000ApplyMappers
 * \ingroup intAcl
 *
 * \desc            Writes the new mapper configuration to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ApplyMappers(fm_int sw)
{
    fm_status               err;
    fm_fm10000MapOwnerType  owner;
    fm_switch*              switchPtr;
    fm10000_switch *        switchExt;
    fm_treeIterator         it;
    fm_uint64               nextKey;
    void *                  nextValue;
    fm_mapper               mapper;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    /*********************************************************************
     * Clear all the mapper owned by the ACL subsystem that can be
     * configured by the function fmAddMapperEntry(). On every ACL apply
     * each mapper are cleared and reconfigure according to the current
     * mapper state defined by the mapper tree.
     *********************************************************************/
    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_SRC_PORT);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        if (switchExt->appliedAcls != NULL)
        {
            /* FM_FM10000_MAP_SRC needs to be free. */
            if (((switchExt->appliedAcls->usedPortSet &
                 (1 << FM10000_ACL_PORTSET_OWNER_POS)) == 1) &&
                (switchExt->appliedAcls->usedPortSet & 0xf) != 0)
            {
                err = fm10000ClearMapper(sw,
                                         FM_MAPPER_SOURCE,
                                         FM_MAPPER_ENTRY_MODE_APPLY);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
        else
        {
            err = fm10000ClearMapper(sw,
                                     FM_MAPPER_SOURCE,
                                     FM_MAPPER_ENTRY_MODE_APPLY);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_PROT);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_PROTOCOL,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_L4PORT_SRC);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_L4_SRC,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_L4PORT_DST);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_L4_DST,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_MAC);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_MAC,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_ETHTYPE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_ETH_TYPE,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_LENGTH_COMPARE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_IP_LENGTH,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_IP);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_IP_ADDR,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000GetMapOwnership(sw, &owner, FM_FM10000_MAP_RESOURCE_VID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (owner == FM_FM10000_MAP_OWNER_ACL)
    {
        err = fm10000ClearMapper(sw,
                                 FM_MAPPER_VLAN,
                                 FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Now fill the hardware with all the mapper entry. */
    for (fmTreeIterInit(&it, &switchPtr->aclInfo.mappers) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        mapper = nextKey >> FM_MAPPER_TYPE_KEY_POS;
        err = fm10000AddMapperEntry(sw,
                                    mapper,
                                    nextValue,
                                    FM_MAPPER_ENTRY_MODE_APPLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ApplyMappers */

