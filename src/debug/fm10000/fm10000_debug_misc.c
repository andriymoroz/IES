/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug_misc.c
 * Creation Date:   July 17, 2011 
 * Description:     Provide miscellaneous debugging functions
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define MAX_STR_LEN 20

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
/** fm10000DbgDumpLogicalPortMapping
 * \ingroup intDiagPorts
 *
 * \desc            Display detailed port mapping information.
 *
 * \param[in]       sw is the switch whose port map is to be displayed.
 *
 * \param[in]       logPort is the port number to be displayed.
 *
 * \param[in]       filterPhysPort specifies only ports matching to
 *                  physical port to be displayed.
 *
 * \param[in]       filterEpl specifies only ports matching to
 *                  EPL port to be displayed.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000DbgDumpLogicalPortMapping(fm_int sw, fm_int logPort,
                                           fm_int filterPhysPort, fm_int filterEpl)
{
    fm10000_port   *portExt;
    fm_int         physPort;
    fm_int         fabricPort;
    fm_int         tmp;
    fm_int         eplPep;  /* EPL or PEP */
    fm_int         channel;
    fm_int         laneNum; /* As argument to port attribute function */
    fm_int         laneCount; /* Number of lanes for the port */
    fm_int         serdes;
    fm_int         ethMode;
    fm_uint32      rxPolarity;
    fm_uint32      txPolarity;
    fm_uint        sbusAddr;
    fm_serdesRing  ring;
    fm_status      status = FM_OK;
    fm_char        logPortStr[MAX_STR_LEN];
    fm_char        eplPepStr[MAX_STR_LEN];
    fm_char        serdesStr[MAX_STR_LEN];
    fm_char        fabPortStr[MAX_STR_LEN];
    fm_char        physPortStr[MAX_STR_LEN];
    fm_char        channelStr[MAX_STR_LEN];
    fm_char        sbusStr[MAX_STR_LEN];
    fm_char        modeStr[MAX_STR_LEN];
    fm_char        polarityStr[MAX_STR_LEN];


    if (!fmIsCardinalPort(sw, logPort))
    {
        /* Ignore it */
        return FM_OK;
    }

    FM_SNPRINTF_S(logPortStr, MAX_STR_LEN, "%s", "--");
    FM_SNPRINTF_S(eplPepStr, MAX_STR_LEN, "%s", "--");
    FM_SNPRINTF_S(serdesStr, MAX_STR_LEN, "%s", "--");
    FM_SNPRINTF_S(fabPortStr, MAX_STR_LEN, "%s", "--"); 
    FM_SNPRINTF_S(physPortStr, MAX_STR_LEN, "%s", "--");
    FM_SNPRINTF_S(channelStr, MAX_STR_LEN, "%s", "--");
    FM_SNPRINTF_S(sbusStr, MAX_STR_LEN, "%s", "--");
    FM_SNPRINTF_S(modeStr, MAX_STR_LEN, "%s", "----------");
    FM_SNPRINTF_S(polarityStr, MAX_STR_LEN, "%s", "----");

    status = fmPlatformMapLogicalPortToPhysical(sw, logPort, &sw, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_DEBUG, status);

    if (filterPhysPort > 0 && filterPhysPort != physPort)
    {
        return FM_OK;
    }
    if (status == FM_OK)
    {
        FM_SNPRINTF_S(physPortStr, MAX_STR_LEN, "%d", physPort);
    }
    FM_SNPRINTF_S(logPortStr, MAX_STR_LEN, "%d", logPort);

    status = fm10000MapPhysicalPortToEplChannel(sw, physPort, &eplPep, &channel);
    /* Could fail here */

    if (filterEpl > 0 && status && filterEpl != eplPep)
    {
        return FM_OK;
    }

    if (status)
    {
        status = fm10000MapLogicalPortToPep(sw, logPort, &eplPep);
    }
    if (status == FM_OK)
    {
        FM_SNPRINTF_S(eplPepStr, MAX_STR_LEN, "%d", eplPep);
    }
    else
    {
        eplPep = -1;
    }

    status = fm10000MapPhysicalPortToFabricPort(sw, physPort, &fabricPort);
    if (status == FM_OK)
    {
        FM_SNPRINTF_S(fabPortStr, MAX_STR_LEN, "%d", fabricPort);
    }

    status = fm10000GetNumLanes(sw, logPort, &laneCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_DEBUG, status);

    status = fm10000GetPortAttribute(sw,
                                    logPort,
                                    FM_PORT_ACTIVE_MAC,
                                    FM_PORT_LANE_NA,
                                    FM_PORT_ETHERNET_INTERFACE_MODE,
                                    &ethMode);
    if (status != FM_OK)
    {
        ethMode = FM_ETH_MODE_DISABLED;
    }
    status = fm10000GetPortModeName(sw, logPort, modeStr, MAX_STR_LEN);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_DEBUG, status);

    for (laneNum = 0; laneNum < laneCount; laneNum++)
    {
        if (eplPep >= 0)
        {
            status = fm10000MapPortLaneToSerdes(sw, logPort, laneNum, &serdes);
            if (status == FM_OK)
            {
                FM_SNPRINTF_S(serdesStr, MAX_STR_LEN, "%d", serdes);
                status = fm10000MapSerdesToSbus(sw, serdes, &sbusAddr, &ring);
                if (status == FM_OK)
                {
                    FM_SNPRINTF_S(sbusStr, MAX_STR_LEN, "%02x", sbusAddr);
                }
                status = fm10000MapSerdesToEplLane(sw, serdes, &tmp, &channel);
                if (status == FM_OK)
                {
                    FM_SNPRINTF_S(channelStr, MAX_STR_LEN, "%d", channel);
                }
            }

            status = fm10000GetPortAttribute(sw,
                                            logPort,
                                            FM_PORT_ACTIVE_MAC,
                                            laneNum,
                                            FM_PORT_RX_LANE_POLARITY,
                                            &rxPolarity);

            status = fm10000GetPortAttribute(sw,
                                            logPort,
                                            FM_PORT_ACTIVE_MAC,
                                            laneNum,
                                            FM_PORT_TX_LANE_POLARITY,
                                            &txPolarity);

            if (rxPolarity && txPolarity)
            {
                FM_SNPRINTF_S(polarityStr, MAX_STR_LEN, "%s", "RXTX");
            }
            else if (rxPolarity)
            {
                FM_SNPRINTF_S(polarityStr, MAX_STR_LEN, "%s", "RX");
            }
            else if (txPolarity)
            {
                FM_SNPRINTF_S(polarityStr, MAX_STR_LEN, "%s", "TX");
            }
            else
            {
                FM_SNPRINTF_S(polarityStr, MAX_STR_LEN, "%s", "NONE");;
            }
        }

        portExt = GET_PORT_EXT(sw, logPort);

        if (laneCount > 1)
        {
            FM_SNPRINTF_S(logPortStr, MAX_STR_LEN, "%d.%d", logPort, laneNum);
            if (laneNum > 0)
            {
                /* Not applicable for lane > 0 */
                FM_SNPRINTF_S(eplPepStr, MAX_STR_LEN, "%s", "--");
                FM_SNPRINTF_S(fabPortStr, MAX_STR_LEN, "%s", "--"); 
                FM_SNPRINTF_S(physPortStr, MAX_STR_LEN, "%s", "--");
                FM_SNPRINTF_S(modeStr, MAX_STR_LEN, "%s", "----------");
            }
        }

        FM_LOG_PRINT("%-4s %8s %8s %8s %7s %5s %5s %15s %8s\n",
                     logPortStr,
                     physPortStr,
                     fabPortStr,
                     eplPepStr,
                     channelStr,
                     serdesStr,
                     sbusStr,
                     modeStr,
                     polarityStr);
    }

    return FM_OK;

ABORT:
    return status;

}   /* end fm10000DbgDumpLogicalPortMapping */





/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000DbgDumpPortMap
 * \ingroup intDiagPorts
 *
 * \desc            Display detailed port mapping if there exists an API
 *                  call to specific hardware. Otherwise default to
 *                  simple fmDbgDumpPortMap function.
 *
 * \param[in]       sw is the switch whose port map is to be displayed.
 *
 * \param[in]       port is the port number to be displayed. If the port
 *                  value is out of its range, then the entire range will
 *                  be displayed.
 *
 * \param[in]       portType indicates which type port parameter refers to:
 *                  0 - logical port.
 *                  1 - physical port.
 *                  2 - epl port.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_PORT if port is invalid
 *
 *****************************************************************************/
fm_status fm10000DbgDumpPortMap(fm_int sw, fm_int port, fm_int portType)
{
    fm_int     logPort;
    fm_int     filterPhysPort = -1;
    fm_int     filterEpl      = -1;
    fm_status  err            = FM_OK;
    fm_switch *switchPtr      = GET_SWITCH_PTR(sw);
    fm_int     start          = port;
    fm_int     end            = port;

    FM_LOG_PRINT("LogPort PhysPort FabricPort EPL/PEP Lane SERDES SBUS        Type      polarity\n");

    switch (portType)
    {
        case FM_PORT_DUMP_TYPE_PHYSICAL:
            filterPhysPort = port;
            start          = -1;
            break;

        case FM_PORT_DUMP_TYPE_EPL:
            filterEpl = port;
            start     = -1;
            break;

        default:
            break;
    }   /* end switch (portType) */

    if (start < 0 || start > switchPtr->maxPhysicalPort)
    {
        start = 0;
        end   = switchPtr->maxPhysicalPort;
    }

    for (logPort = start ; logPort <= end ; logPort++)
    {
        err = fm10000DbgDumpLogicalPortMapping(sw, logPort, filterPhysPort, filterEpl);
    }

    return err;

}   /* end fm10000DbgDumpPortMap */



