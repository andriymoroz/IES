/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_sched.c
 * Creation Date:   March 20th, 2014
 * Description:     Functions for manipulating the scheduler configuration.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

/* For simulation, initialize fewer pointers as it takes a very long time 
 * to simulate all this register access */
#ifdef FV_CODE
#define FM10000_FREELIST_MAX_RXQ          400
#define FM10000_FREELIST_MAX_TXQ          400  
#define FM10000_FREELIST_MAX_TXQ_QID      384
#define FM10000_FREELIST_MAX_ARRAY        400  
#else
#define FM10000_FREELIST_MAX_RXQ          1024
#define FM10000_FREELIST_MAX_TXQ          24576
#define FM10000_FREELIST_MAX_TXQ_QID      384
#define FM10000_FREELIST_MAX_ARRAY        24576
#endif

/* 64B(Pkt) + 8B(preamble) + 12B(IFG) */
#define BITS_PER_64B_PKT                  ((64 + 8 + 12) * 8)

#define SLOT_SPEED                       (2.5)
#define SLOT_SPEED_MBPS                  (SLOT_SPEED * 1000)
#define SLOTS_PER_100G                   (100 / SLOT_SPEED)
#define SLOTS_PER_60G                    (60  / SLOT_SPEED)
#define SLOTS_PER_40G                    (40  / SLOT_SPEED)
#define SLOTS_PER_25G                    (25  / SLOT_SPEED)
#define SLOTS_PER_10G                    (10  / SLOT_SPEED)
#define SLOTS_PER_2500M                  (2.5 / SLOT_SPEED)

/* The minimum spacing requirement in order to respect the 4 clock cycles
 * spacing. */
#define MIN_PORT_SPACING                 4

#define LIST0_BASE                       0
#define LIST1_BASE                       (FM10000_SCHED_RX_SCHEDULE_ENTRIES/2)

/* STAT Types */
enum
{
    STAT_TYPE_SPEED = 0,
    STAT_TYPE_QPC,
    STAT_TYPE_PORT,
};

#define DIFFICULTY_25G_IN_10G25G_QPC         10
#define DIFFICULTY_10G_IN_10G25G_QPC         9
#define DIFFICULTY_25G_IN_MULTI_25G_QPC      8
#define DIFFICULTY_10G_IN_MULTI_10G_QPC      7
#define DIFFICULTY_25G_IN_MULTI_SPEED_QPC    6
#define DIFFICULTY_10G_IN_MULTI_SPEED_QPC    5
#define DIFFICULTY_2500M_IN_MULTI_SPEED_QPC  4
#define DIFFICULTY_2500M_IN_MULTI_2500M_QPC  3
#define DIFFICULTY_OTHER                     0
#define DIFFICULTY_INVALID                   (-1)

#define ACTIVE_SCHEDULE                      0
#define TMP_SCHEDULE                         1

#define NUM_PORTS_PER_QPC                    4
#define NUM_QPC                              (FM10000_NUM_FABRIC_PORTS / NUM_PORTS_PER_QPC)

/* This bit is set for a speed bin if it has been used already */
#define SPEED_BIN_USED                       0x40000000

#define FREE_ENTRY                           (-1)

/* Automatically derive APP (assigned physical port) from
 * AFP (assigned fabric port) */
#define AUTO_APP                             (-1)


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
/** GetNbPorts
 * \ingroup intSwitch
 *
 * \desc            Retrieves the number of ports in a bitArray
 * 
 * \param[in]       b is a pointer to a bitArray structure
 * 
 * \return          The number of ports.
 *
 *****************************************************************************/
static fm_int GetNbPorts(fm_bitArray *b)
{
    fm_status err;

    fm_int  cnt;
    err = fmGetBitArrayNonZeroBitCount(b, &cnt);
    if (err != FM_OK)
    {
        cnt = 0;
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Could not retrieve number of bits...\n");
    }

    return cnt;

}   /* end GetNbPorts */



/*****************************************************************************/
/** InitializeFreeLists
 * \ingroup intSwitch
 *
 * \desc            Initializes scheduler's segment free lists.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitializeFreeLists(fm_int sw)
{
    fm_status err = FM_OK;
    fm_switch*  switchPtr;
    fm_uint64   rv64;
    fm_int      i;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing RXQ_MCAST List\n");
    for (i = 0; i < 8; i++)
    {
        rv64 = 0;
        FM_SET_FIELD64(rv64, FM10000_SCHED_RXQ_STORAGE_POINTERS, HeadPage, i);
        FM_SET_FIELD64(rv64, FM10000_SCHED_RXQ_STORAGE_POINTERS, TailPage, i);
        FM_SET_FIELD64(rv64, FM10000_SCHED_RXQ_STORAGE_POINTERS, HeadIdx,  0);
        FM_SET_FIELD64(rv64, FM10000_SCHED_RXQ_STORAGE_POINTERS, TailIdx,  0);
        FM_SET_FIELD64(rv64, FM10000_SCHED_RXQ_STORAGE_POINTERS, NextPage, 0);

        err = switchPtr->WriteUINT64(sw, FM10000_SCHED_RXQ_STORAGE_POINTERS(i, 0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    for (i = 0; i < (FM10000_FREELIST_MAX_RXQ - 8); i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_RXQ_FREELIST_INIT(), 8+i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing TXQ List\n");
    for (i = 0; i < 384; i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_TXQ_HEAD_PERQ(i), i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_TXQ_TAIL0_PERQ(i), i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_TXQ_TAIL1_PERQ(i), i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    for (i = 0; i < (FM10000_FREELIST_MAX_TXQ - 384); i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_TXQ_FREELIST_INIT(), 384 + i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing Free Segment List\n");
    for (i = 0; i < 48; i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_SSCHED_RX_PERPORT(i), i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    for (i = 0; i < (FM10000_FREELIST_MAX_ARRAY - 48); i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_FREELIST_INIT(), 48 + i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end InitializeFreeLists */




/*****************************************************************************/
/** ComputeScheduleLength
 * \ingroup intSwitch
 *
 * \desc            Computes the schedule length
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ComputeScheduleLength(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_uint64           freq;
    fm_uint64           segRate;
    fm_float            fhMhz;
    fm_int              overspeed;

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    /*********************************************************************
     * Determine the number of entries required in the schedule (including 
     * idle). 
     *
     * One slot per 2.5G, rounded to 10G.
     *
     * Segment Rate is the max segment rate for 64B packets
     * 
     * segRate = bps / ((64B + preamble + ifg) * 8bits)
     * segRate = bps / 672
     *
     * n = FREQ / floor( (segRate @ 2.5G)           <-- not rounded
     * n = FREQ / floor( (segRate @ 2.5G) * 4) / 4  <-- rounded to 10G
     *
     * Or simply:
     * n = FREQ / (segRate @ 10G) * 4               <-- rounded to 10G
     ********************************************************************/

    err = fm10000ComputeFHClockFreq(sw, &fhMhz);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    overspeed = GET_FM10000_PROPERTY()->schedOverspeed;

    freq                = fhMhz * 1e6;
    segRate             = (10e9 + overspeed) / BITS_PER_64B_PKT;
    sInfo->tmp.schedLen = freq / segRate * 4;

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                 "Freq = %" FM_FORMAT_64 "d Hz, SchedLen = %d\n", 
                 freq, 
                 sInfo->tmp.schedLen);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end ComputeScheduleLength */




/*****************************************************************************/
/** FilterBwDuplicates
 * \ingroup intSwitch
 *
 * \desc            Filters out duplicate BW requests from each QPC.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FilterBwDuplicates(fm_int sw)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_schedulerPort   *spPtr;
    fm_uint32           rv;
    fm_int              i;
    fm_int              j;
    fm_int              qpc;
    fm_int              channel;
    fm_int              qpcBw[FM10000_NUM_QPC][4];
    fm_int              qpcQuadMaster[FM10000_NUM_QPC];
    fm_int              chan[4];
    fm_int              pep;
    fm_int              pcieHost;
    
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    /*********************************************
     * Silently remove duplicate BW requests for 
     * the following scenarios:
     * 
     * Below chan[0] is the master port's channel
     *  
     * if chan[0] > 40000
     *   100000  -> 100000
     *   25000   -> 0
     *   25000   -> 0
     *   25000   -> 0
     *
     * if chan[0] >= 40000 && chan[!=0] > 10000
     *   40000   -> 100000
     *   25000   -> 0
     *   10000   -> 0
     *   2500    -> 0
     *
     * if chan[0] == 40000
     *   40000   -> 40000
     *   10000   -> 0
     *   10000   -> 0
     *   2500    -> 0
     *
     * if chan[0,1,2,3] == 25000
     *   25000   -> 100000
     *   25000   -> 0
     *   25000   -> 0
     *   25000   -> 0
     *
     * if chan[0,1,2,3] == 10000
     *   10000   -> 40000
     *   10000   -> 0
     *   10000   -> 0
     *   10000   -> 0
     * 
     * For PEPs, if bifurcation is disabled
     * (1x8 PEP) remove any BW allocated to
     * the bifurcated PEP. 
     *********************************************/

    FM_CLEAR(qpcBw);

    for (i = 0; i < FM10000_NUM_QPC; i++)
    {
        qpcQuadMaster[i] = -1;
    }
    
    for (i = 0; i < sInfo->tmp.nbPorts; i++)
    {
        spPtr = &sInfo->tmp.portList[i];
        qpc     = spPtr->fabricPort / 4;
        channel = spPtr->fabricPort % 4;

        qpcBw[qpc][channel] = spPtr->speed;

        /* Find the master port such that we can effectively remove BW
         * duplicates on non-master ports of the QPC. Only speeds of 40G
         * and 100G are considered multi-lane. */
        if (spPtr->speed >= 40000)
        {
            if (qpcQuadMaster[qpc] == -1)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                             "physPort=%d speed>=40000 is being marked as "
                             "the master port for QPC=%d\n", 
                             spPtr->physPort,
                             qpc);
                qpcQuadMaster[qpc] = channel;
            }
            else
            {
                err = FM_ERR_SCHED_INIT;
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                             "Two ports within QPC=%d are marked as "
                             "multi-lane (speed>=40000)\n", 
                             qpc);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
        }

        /* PCIE ports can safely be but lowered to 40G because the host
         * interface only achieves 50G for 256B+ frames; 
         * it doesn't need to be fully provisioned for minsize frames. 
         * See bugzilla #25673 comment #12 */
        if ( (spPtr->speed > 40000) && 
             ( (spPtr->fabricPort >= FM10000_FIRST_PCIE_FABRIC_PORT) &&
               (spPtr->fabricPort <= FM10000_LAST_PCIE_FABRIC_PORT) ) )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                         "Silently changing PCIE port (physPort=%d) speed "
                         "from from %d to %d due to PCIE host interface "
                         "performance limitations.\n",
                         spPtr->physPort,
                         spPtr->speed,
                         40000);
            spPtr->speed = 40000;
        }
    }

    for (i = 0; i < sInfo->tmp.nbPorts; i++)
    {
        spPtr   = &sInfo->tmp.portList[i];
        qpc     = spPtr->fabricPort / 4;
        channel = spPtr->fabricPort % 4;

        /* Fill in chan[]:
         *   0: Master Channel
         *   1..3 Non-Master Channels */
        for (j = 0; j < 4; j++) 
        {
            chan[j] = j;
        }

        /* Swap Master Channel with 0 */
        if (qpcQuadMaster[qpc] != -1)
        {
            chan[qpcQuadMaster[qpc]] = chan[0];
            chan[0] = qpcQuadMaster[qpc];
        }
        
        if (channel == chan[0])
        {
            if ( (qpcBw[qpc][chan[0]] >= 40000) &&
                 ( (qpcBw[qpc][chan[1]] > 10000) ||
                   (qpcBw[qpc][chan[2]] > 10000) ||
                   (qpcBw[qpc][chan[3]] > 10000) ) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                             "Silently changing physPort %d speed "
                             "from %d to %d because of BW overlap\n",
                             spPtr->physPort,
                             spPtr->speed,
                             100000);

                spPtr->speed = 100000;
            }
            else if ( (qpcBw[qpc][chan[0]] == 25000) &&
                      (qpcBw[qpc][chan[1]] == 25000) &&
                      (qpcBw[qpc][chan[2]] == 25000) &&
                      (qpcBw[qpc][chan[3]] == 25000) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                                 "Silently changing physPort %d speed "
                                 "from %d to %d because of BW overlap\n",
                                 spPtr->physPort,
                                 spPtr->speed,
                                 100000);

                spPtr->speed = 100000;
            }
            else if ( (qpcBw[qpc][chan[0]] == 10000) &&
                      (qpcBw[qpc][chan[1]] == 10000) &&
                      (qpcBw[qpc][chan[2]] == 10000) &&
                      (qpcBw[qpc][chan[3]] == 10000) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                                 "Silently changing physPort %d speed "
                                 "from %d to %d because of BW overlap\n",
                                 spPtr->physPort,
                                 spPtr->speed,
                                 40000);

                spPtr->speed = 40000;
            }
        }
        else /* (channel != 0) */
        {
            if ((qpcBw[qpc][chan[0]] >= 40000))
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                                 "Silently changing physPort %d speed "
                                 "from %d to %d because of BW overlap\n",
                                 spPtr->physPort,
                                 spPtr->speed,
                                 0);

                spPtr->speed = 0;
            }
            else if ( (qpcBw[qpc][chan[0]] == 25000) &&
                      (qpcBw[qpc][chan[1]] == 25000) &&
                      (qpcBw[qpc][chan[2]] == 25000) &&
                      (qpcBw[qpc][chan[3]] == 25000) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                                 "Silently changing physPort %d speed "
                                 "from %d to %d because of BW overlap\n",
                                 spPtr->physPort,
                                 spPtr->speed,
                                 0);

                spPtr->speed = 0;
            }
            else if ( (qpcBw[qpc][chan[0]] == 10000) &&
                      (qpcBw[qpc][chan[1]] == 10000) &&
                      (qpcBw[qpc][chan[2]] == 10000) &&
                      (qpcBw[qpc][chan[3]] == 10000) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                                 "Silently changing physPort %d speed "
                                 "from %d to %d because of BW overlap\n",
                                 spPtr->physPort,
                                 spPtr->speed,
                                 0);

                spPtr->speed = 0;
            }

            /* PCIE Port */
            if ( (spPtr->fabricPort >= FM10000_FIRST_PCIE_FABRIC_PORT) &&
                 (spPtr->fabricPort <= FM10000_LAST_PCIE_FABRIC_PORT) )
            {
                pep = (spPtr->fabricPort - FM10000_FIRST_PCIE_FABRIC_PORT) / 2;
                pcieHost = pep / 2;

                /* Get Bifurcation Mode */
                err = switchPtr->ReadUINT32(sw, FM10000_DEVICE_CFG(), &rv);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

                /* If bifurcation is disabled and PEP is odd, remove BW */
                if ( (!(rv & (1 << pcieHost))) &&
                     (pep % 2 == 1) )
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                                 "Silently removing bandwidth for physPort %d"
                                 "(pep=%d) speed because the current "
                                 "bifurcation mode prevents this PEP from "
                                 "being used\n",
                                 spPtr->physPort,
                                 pep);

                    spPtr->speed = 0;
                }
            }
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end FilterBwDuplicates */




/*****************************************************************************/
/** DbgDumpQPCUsage
 * \ingroup intSwitch
 *
 * \desc            Dumps the scheduler token usage per QPC
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       qpc is the quad port channel to dump.
 * 
 * \param[in]       showAll should be set to true to also dump the QPC's
 *                  token assignements. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status DbgDumpQPCUsage(fm_int sw, fm_int qpc, fm_bool showAll)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_schedInfo  *    sInfo;
    fm_treeIterator         it;
    fm_uint64               treeKey;
    fm10000_schedEntryInfo *treeValue;
    fm_int                  laneCnt[4];
    fm_int                  afpCnt[4];
    fm_int                  freeCnt;
    fm_int                  i;
    fm_int                  logPort;
    fm_bool                 isQuad;
    fm_int                  app;
    fm_int                  logSwitch;
    
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Dumping QPC[%d] Slot Usage (2.5G each):\n", qpc);
    FM_LOG_PRINT("--------------------------------------\n");

    FM_CLEAR(laneCnt);
    FM_CLEAR(afpCnt);
    freeCnt = 0;
    isQuad  = 1;
    app     = AUTO_APP;

    for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        laneCnt[treeValue->lane]++;

        if ( (treeValue->afp >= 0) &&
             (treeValue->afp <= FM10000_NUM_FABRIC_PORTS) )
        {
            afpCnt[treeValue->afp % 4]++; 
        }
        else
        {
            freeCnt++;
        }

        isQuad &= treeValue->quad;
        app     = treeValue->app;
    }
    
    if (showAll)
    {
        for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
        {
            FM_LOG_PRINT("slot %03lld = afp %02d, lane = %d, quad = %d\n", 
                         treeKey, 
                         treeValue->afp,
                         treeValue->lane,
                         treeValue->quad);
        }
    }

    FM_LOG_PRINT("laneCnt[0] = %d\n", laneCnt[0]);
    FM_LOG_PRINT("laneCnt[1] = %d\n", laneCnt[1]);
    FM_LOG_PRINT("laneCnt[2] = %d\n", laneCnt[2]);
    FM_LOG_PRINT("laneCnt[3] = %d\n", laneCnt[3]);

    for (i = 0; i < 4; i++)
    {
        if ( (i == 0) &&
             (isQuad) && 
             (app != AUTO_APP) )
        {
            err = fmPlatformMapPhysicalPortToLogical(sw,
                                                     app, 
                                                     &logSwitch, 
                                                     &logPort);
            if (err != FM_OK)
            {
                /* The fabric port maps to no logical port */
                logPort = -1;
                err = FM_OK;
            }
        }
        else
        {
            err = fm10000MapFabricPortToLogicalPort(sw, i + qpc*4, &logPort);
            if (err != FM_OK)
            {
                /* The fabric port maps to no logical port */
                logPort = -1;
                err = FM_OK;
            }
        }

        FM_LOG_PRINT("afpCnt[fabricPort=%d logPort=%d] = %d\n", i + qpc*4, logPort, afpCnt[i]);
    }

    FM_LOG_PRINT("freeCnt = %d\n", freeCnt);

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }
    
    return err;

}   /* end DbgDumpQPCUsage */




/*****************************************************************************/
/** DbgDumpSchedulerConfig
 * \ingroup intSwitch
 *
 * \desc            Dumps the scheduler token lists
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       active should be set to ACTIVE_SCHEDULE to show the
 *                  current active schedule or TMP_SCHEDULE for a schedule
 *                  that is pending.
 * 
 * \param[in]       dumpQPC should be set to TRUE if the QPC information should
 *                  be dumped. 
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DbgDumpSchedulerConfig(fm_int sw, fm_int active, fm_bool dumpQPC)
{
    fm_status              err = FM_OK;
    fm_switch *            switchPtr;
    fm10000_switch *       switchExt;
    fm10000_schedInfo  *   sInfo;
    fm_int                 i;
    fm_uint64              treeKey;
    fm10000_schedStat  *   statPtr;
    fm_treeIterator        it;
    fm10000_schedInfoInt * sInfoInt;
    fm_int                 activeEntries;
    fm_float               fhMhz;
    fm_uint64              freq;
    fm_int                 cpi;
    fm_int                 physPort;
    fm_int                 logPort;
    fm10000_schedSpeedInfo speed;
    fm_int                 reservedTotal;
    fm_int                 reserved;
    fm_text                quadStr;
    fm_int                 preReservedTotal;
    fm_int                 preReserved;
    
    const fm_text scheduleFormat     = "%-5d  %-4d  %-4d  %-4s  %-4s  %6d\n";
    const fm_text scheduleFormatIdle = "%-5d  %-4s  %-4s  %-4s  %-4s  %6s\n";
    const fm_text speedStatFormat = "%6d  %-4d  %-5d  %-5d  %-3d(%-3d)  %-3d(%-3d)  %-6.2f  %-6d\n";
    const fm_text qpcStatFormat  = "%6d  %-4d  %-5d  %-5d  %-3d(%-3d)  %-3d(%-3d)  %-6.2f  %-6d\n";
    const fm_text portStatFormat = "%-4d  %6d  %-4d  %-5d  %-5d  %-3d(%-3d)  %-3d(%-3d)  %-6.2f  %-6d\n";
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    if (active == ACTIVE_SCHEDULE)
    {
        sInfoInt = &sInfo->active;
    }
    else
    {
        sInfoInt = &sInfo->tmp;
    }

    err = fm10000ComputeFHClockFreq(sw, &fhMhz);
    
    if (err != FM_OK)
    {
        fhMhz = 0;
    }

    freq = fhMhz * 1e6;
    
    FM_LOG_PRINT("Frame Handler Frequency = %" FM_FORMAT_64 "d Hz\n", freq);
    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("Dumping Scheduler Software State: \n");
    FM_LOG_PRINT("Entry  Log   Phys  Quad  Idle  Speed \n");
    FM_LOG_PRINT("-----  ----  ----  ----  ----  ------\n");

    activeEntries = 0;

    for (i = 0; i < sInfoInt->schedLen; i++)
    {
        if (sInfoInt->schedList[i].idle == TRUE)
        {
            FM_LOG_PRINT(scheduleFormatIdle, 
                         i, 
                         "--",
                         "--",
                         "--",
                         "yes",
                         "--");
        }
        else
        {
            FM_LOG_PRINT(scheduleFormat, 
                         i, 
                         sInfoInt->schedList[i].port,
                         sInfoInt->schedList[i].fabricPort,
                         sInfoInt->schedList[i].quad ? "yes" : "no",
                         "no",
                         sInfoInt->speedList[i]);
        }

        if (sInfoInt->schedList[i].idle == 0)
        {
            activeEntries++;
        }
    }

    FM_LOG_PRINT("%d/%d entries used (%.1fG/%.1fG) \n", 
                 activeEntries, 
                 sInfoInt->schedLen,
                 (activeEntries * (fm_float)(SLOT_SPEED)),
                 (sInfoInt->schedLen * (fm_float)(SLOT_SPEED)));

    /* Stats are only valid for active schedule */
    if (active == ACTIVE_SCHEDULE)
    {
    
        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("Dumping Scheduler Stats (Per Speed): \n");

        FM_LOG_PRINT("Speed   Cnt   First  Last   Min(loc)  Max(loc)  Avg     Jitter\n");
        FM_LOG_PRINT("------  ----  -----  -----  --------  --------  ------  ------\n");
        
        for (fmTreeIterInit(&it, &sInfo->speedStatsTree);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &statPtr)) == FM_OK ;)
        {
            FM_LOG_PRINT(speedStatFormat, 
                         statPtr->speed, 
                         statPtr->cnt,
                         statPtr->first,
                         statPtr->last,
                         statPtr->minDiff,
                         statPtr->minLoc,
                         statPtr->maxDiff,
                         statPtr->maxLoc,
                         ((fm_float) (sInfo->active.schedLen)) / statPtr->cnt,
                         statPtr->maxDiff - statPtr->minDiff);
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }

        FM_LOG_PRINT("*These stats only reflect the initial state\n");

        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("Dumping Scheduler Stats (Per Quad Port Channel): \n");
        FM_LOG_PRINT("Quad    Cnt   First  Last   Min(loc)  Max(loc)  Avg     Jitter\n");
        FM_LOG_PRINT("------  ----  -----  -----  --------  --------  ------  ------\n");
        
        for (fmTreeIterInit(&it, &sInfo->qpcStatsTree);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &statPtr)) == FM_OK ;)
        {
            FM_LOG_PRINT(qpcStatFormat, 
                         (fm_int)(treeKey & 0xFFFFFFFF), 
                         statPtr->cnt,
                         statPtr->first,
                         statPtr->last,
                         statPtr->minDiff,
                         statPtr->minLoc,
                         statPtr->maxDiff,
                         statPtr->maxLoc,
                         ((fm_float) (sInfo->active.schedLen)) / statPtr->cnt,
                         statPtr->maxDiff - statPtr->minDiff);
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }

        FM_LOG_PRINT("* These stats only reflect the initial state\n");

        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("Dumping Scheduler Stats (Per Port): \n");
        FM_LOG_PRINT("Port  Speed   Cnt   First  Last   Min(loc)  Max(loc)  Avg     Jitter\n");
        FM_LOG_PRINT("----  ------  ----  -----  -----  --------  --------  ------  ------\n");
        
        for (fmTreeIterInit(&it, &sInfo->portStatsTree);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &statPtr)) == FM_OK ;)
        {
            FM_LOG_PRINT(portStatFormat, 
                         (fm_int)(treeKey & 0xFFFFFFFF), 
                         statPtr->speed,
                         statPtr->cnt,
                         statPtr->first,
                         statPtr->last,
                         statPtr->minDiff,
                         statPtr->minLoc,
                         statPtr->maxDiff,
                         statPtr->maxLoc,
                         ((fm_float) (sInfo->active.schedLen)) / statPtr->cnt,
                         statPtr->maxDiff - statPtr->minDiff);
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }

        FM_LOG_PRINT("* These stats only reflect the initial state\n");
    }

    if (dumpQPC)
    {
        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("Dumping Scheduler Slot Usage (Per QPC): \n");
        for (i = 0; i < NUM_QPC; i++)
        {
            DbgDumpQPCUsage(sw, i, TRUE); 
        }
    }

    if ( fmTreeIsInitialized(&sInfo->qpcState[0]) )
    {
        reservedTotal = 0;
        preReservedTotal = 0;
        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("Dumping scheduler BW per logical port:\n");
        FM_LOG_PRINT("Port  Assigned   Avail(sl/ml)   Reserved  PreReserved Quad\n");
        FM_LOG_PRINT("----  ---------  -------------  --------- ----------- ----\n");
        for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
        {
            err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);

            if (err != FM_OK)
            {
                FM_LOG_PRINT("ERROR: Can't convert cpi=%d (%s), skipping\n", 
                             cpi, 
                             fmErrorMsg(err));
                continue;
            }

            err = fm10000GetSchedPortSpeed(sw, 
                                           physPort, 
                                           &speed);
            if (err != FM_OK)
            {
                FM_LOG_PRINT("ERROR: Can't get sched port speed of "
                             "logPort=%d (%s), skipping\n", 
                             logPort,
                             fmErrorMsg(err));
                continue;
            }

            quadStr = speed.isQuad == 1 ? "yes" : "-";

            if (sInfo->attr.mode == FM10000_SCHED_MODE_STATIC)
            {
                reserved    = -1;
                preReserved = -1;
            }
            else if (sInfo->attr.mode == FM10000_SCHED_MODE_DYNAMIC)
            {
                reserved    = sInfo->reservedSpeed[physPort];
                preReserved = sInfo->preReservedSpeed[physPort];
            }
            else
            {
                reserved    = -1;
                preReserved = -1;
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                             "Unexpected scheduler mode %d\n", 
                             sInfo->attr.mode); 
            }

            FM_LOG_PRINT("%02d    %-6d     %6d/%-6d  %-6d    %-6d     %s\n", 
                         logPort, 
                         speed.assignedSpeed, 
                         speed.singleLaneSpeed,
                         speed.multiLaneSpeed,
                         reserved,
                         preReserved,
                         quadStr);

            reservedTotal    += sInfo->reservedSpeed[physPort];
            preReservedTotal += sInfo->preReservedSpeed[physPort];
        }

        if (sInfo->attr.mode == FM10000_SCHED_MODE_DYNAMIC)
        {
            FM_LOG_PRINT("                                -------- --------\n");
            FM_LOG_PRINT("                        Total:  %06d   %06d\n", reservedTotal, preReservedTotal);
        }
        
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end DbgDumpSchedulerConfig */




/*****************************************************************************/
/** SplitBandwidth
 * \ingroup intSwitch
 *
 * \desc            Splits the bandwidth
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       speedAorB is the speed that should be split into speedA and
 *                  speedB.
 * 
 * \param[in]       bwA is the number of slots reserved for speedA
 * 
 * \param[in]       speedA is the first speed.
 * 
 * \param[in]       bwB is the number of slots reserved for speedB
 *
 * \param[in]       speedB is the second speed.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SplitBandwidth(fm_int             sw, 
                                fm10000_schedSpeed speedAorB,
                                fm_int             bwA,
                                fm10000_schedSpeed speedA,
                                fm_int             bwB,
                                fm10000_schedSpeed speedB)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              maxCredit;
    fm_int              creditB;
    fm_int              i;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw=%d, speedAorB=%d, bwA=%d, speedA=%d, bwB=%d, speedB=%d\n",
                 sw, speedAorB, bwA, speedA, bwB, speedB);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    maxCredit = bwA + bwB;
    creditB   = 0;
    
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        if ( sInfo->tmp.speedList[i] == speedAorB )
        {
            creditB += bwB;

            if ( creditB >= maxCredit )
            {
                sInfo->tmp.speedList[i] = speedB;
                creditB -= maxCredit;
            }
            else
            {
                sInfo->tmp.speedList[i] = speedA;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SplitBandwidth */




/*****************************************************************************/
/** ValidateQuad4Constraint
 * \ingroup intSwitch
 *
 * \desc            Validates that the given slot has no conflict with
 *                  previous/next three slots in regards to the QUAD 4 cycle
 *                  spacing requirement.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port to fit in the specified slot.
 * 
 * \param[in]       slot is the slot where the port is (or would be) inserted. 
 * 
 * \return          TRUE if the port CANNOT be safely put in the specified slot.
 * \return          FALSE otherwise.
 *
 *****************************************************************************/
static fm_bool ValidateQuad4Constraint(fm_int sw, 
                                       fm_int port, 
                                       fm_int slot)
{
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_bool             notValid;
    fm_int              first;
    fm_int              last;
    fm_int              current;
    fm_int              qpcPort;
    fm_int              portTmp;
    fm_int              qpcTmp;
    fm_bool             isIdle;

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    if (sInfo->tmp.schedList[slot].idle)
    {
        /* idle tokens don't conflict with other tokens */
        return FALSE;
    }

    /* Determine the first/last slots, note that last is not inclusive */
    first = slot - 3;
    last  = slot + 4;

    if (first < 0)
    {
        first += sInfo->tmp.schedLen;
    }

    if (last >= sInfo->tmp.schedLen)
    {
        last -= sInfo->tmp.schedLen;
    }

    qpcPort = sInfo->physicalToFabricMap[port] / 4;
    current = first;
    notValid = FALSE;

    while ( (current != last) &&
            (notValid == FALSE) )
    {
        portTmp = sInfo->tmp.schedList[current].port;
        isIdle  = sInfo->tmp.schedList[current].idle;

        /* Skip the check if port has not been set yet, or slot is the
         * specified one. Or the slot is idle */
        if ( (portTmp == -1) ||
             (current == slot) ||
             (isIdle) )
        {
            /* do nothing */
        }
        else
        {
            qpcTmp = sInfo->physicalToFabricMap[portTmp] / 4; 

            if (qpcPort == qpcTmp)
            {
                notValid = TRUE;
            }
        }

        current++;

        if (current >= sInfo->tmp.schedLen)
        {
            current -= sInfo->tmp.schedLen;
        }
    }

    return notValid;

}   /* end ValidateQuad4Constraint */




/*****************************************************************************/
/** CompareSpeedBins
 * \ingroup intSwitch
 *
 * \desc            Compare function used for sorting speed bins by the
 *                  distance seperating them to the next closest bin. 
 * 
 * \param[in]       a is a pointer to a speed bin location and its distance to
 *                  the next closest bin. 
 *
 * \param[in]       b is a pointer to a speed bin location and its distance to
 *                  the next closest bin. 
 * 
 * \return          <0 if 'a' should be sorted before 'b'
 * \return          0 if 'a' is quivalent to 'b'
 * \return          >0 if 'a' should be sorted after 'b'
 *
 *****************************************************************************/
static fm_int CompareSpeedBins(const void *a, const void *b)
{
    /* The distance is encoded into the upper 16 bits of value pointed
     * by a or b, if distance is equal the bin location (lower 16 bits) 
     * determines precedence. */

    return ((*(fm_int *)a) - (*(fm_int *)b));

}   /* end CompareSpeedBins */




/*****************************************************************************/
/** CompareDifficulty
 * \ingroup intSwitch
 *
 * \desc            Compare function used for sorting difficulty.
 * 
 * \param[in]       a is a pointer to a difficulty.
 *
 * \param[in]       b is a pointer to a difficulty.
 * 
 * \return          <0 if 'a' should be sorted after 'b'
 * \return          0 if 'a' is quivalent to 'b'
 * \return          >0 if 'a' should be sorted before 'b'
 *
 *****************************************************************************/
static fm_int CompareDifficulty(const void *a, const void *b)
{
    fm10000_schedPortDifficulty *pdA;
    fm10000_schedPortDifficulty *pdB;
    pdA = (fm10000_schedPortDifficulty*)a;
    pdB = (fm10000_schedPortDifficulty*)b;

    return pdB->diff - pdA->diff;
    
}   /* end CompareDifficulty */




/*****************************************************************************/
/** AssignPort
 * \ingroup intSwitch
 *
 * \desc            Assign a port by finding which slots it can be inserted
 *                  into.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port to assign.
 * 
 * \param[in]       ba is a fm_bitArray pointer to bitArray containing
 *                  all other ports of the same speed.
 * 
 * \param[in]       speed is the speed at which the port will be assigned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_VIOLATION if a valid solution could not be
 *                  generated while assigning ports. 
 *
 *****************************************************************************/
static fm_status AssignPort(fm_int       sw, 
                            fm_int       port, 
                            fm_bitArray *ba, 
                            fm_int       speed)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              j;
    fm_int              portCnt;
    fm_int              binCnt;
    fm_int              startBin;
    fm_int              speedBin[FM10000_MAX_SCHEDULE_LENGTH];
    fm_bool             notValid;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, port = %d, speed = %d\n", 
                 sw,
                 port,
                 speed);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    err = fmGetBitArrayNonZeroBitCount(ba, &portCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* First, extract speed bins, start at j=1, because j=0 is
     * the implicit idle and cannot be used */
    binCnt = 0;
    for (j = 1; j < sInfo->tmp.schedLen; j++)
    {
        if ( sInfo->tmp.speedList[j] == speed )
        {
            speedBin[binCnt++] = j;
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                 "portCnt=%d, binCnt=%d\n",
                 portCnt,
                 binCnt);

    /* Find and validate in which bins the port can be fitted */
    startBin = 0;
    notValid = TRUE;

    while ( (startBin < portCnt) && (notValid == TRUE) )
    {
        notValid = FALSE;
        for (j = startBin; j < binCnt; j += portCnt)
        {
            notValid |= ValidateQuad4Constraint(sw, port, speedBin[j]);

            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                         "port = %d, speedBin[%d] = %d, notValid = %d\n", 
                         port, 
                         j, 
                         speedBin[j], 
                         notValid);

            if (notValid)
            {
                /* Try with the next set of bins */
                startBin++;
                break;
            }
        }
    }
    
    /* No solution could be found */
    if (notValid == TRUE)
    {
        err = FM_ERR_SCHED_VIOLATION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "Inserting port %d in the schedule\n",
                     port);

        for (j = startBin; j < binCnt; j += portCnt)
        {
            sInfo->tmp.schedList[speedBin[j]].port = port;

            sInfo->tmp.speedList[speedBin[j]] |= SPEED_BIN_USED;
        }

        /* Remove port from bit array */
        err = fmSetBitArrayBit(ba, port, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end AssignPort */




/*****************************************************************************/
/** AssignPort2500
 * \ingroup intSwitch
 *
 * \desc            Assign a port 2.5G port by finding which slots it can be
 *                  inserted into (this function is an optimized version
 *                  of AssignPort).
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port to assign.
 * 
 * \param[in]       ba is a fm_bitArray pointer to bitArray containing
 *                  all other ports of the same speed.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_VIOLATION if a valid solution could not be
 *                  generated while assigning ports. 
 *
 *****************************************************************************/
static fm_status AssignPort2500(fm_int       sw, 
                                fm_int       port, 
                                fm_bitArray *ba)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              j;
    fm_int              bin;
    fm_int              binCnt;
    fm_int              speedBin[FM10000_MAX_SCHEDULE_LENGTH];
    fm_int              speedBin2500;
    fm_bool             notValid;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, port = %d\n", 
                 sw,
                 port);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /* First, extract idle speed bins, start at j=1, because j=0 is
     * the implicit idle and cannot be used */
    binCnt = 0;
    for (j = 1; j < sInfo->tmp.schedLen; j++)
    {
        if ( sInfo->tmp.speedList[j] == FM10000_SCHED_SPEED_IDLE )
        {
            speedBin[binCnt++] = j;
        }
    }

    speedBin2500 = -1;

    /* Find a 2.5G bin, which shall get swapped later */
    for (j = 1; j < sInfo->tmp.schedLen; j++)
    {
        if ( sInfo->tmp.speedList[j] == FM10000_SCHED_SPEED_2500M )
        {
            speedBin2500 = j;
            break;
        }
    }

    if (speedBin2500 == -1)
    {
        /* Unexpected error */
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "binCnt=%d\n", binCnt);

    notValid = TRUE;

    /* Find and validate in which bins the port can be fitted */
    for (bin = 0; bin < binCnt; bin++)
    {
        notValid = ValidateQuad4Constraint(sw, port, speedBin[bin]);

        if (notValid == FALSE)
        {
            /* Swap Bins Speed*/
            sInfo->tmp.speedList[speedBin[bin]] = FM10000_SCHED_SPEED_2500M;
            sInfo->tmp.speedList[speedBin2500]  = FM10000_SCHED_SPEED_IDLE;
            break;
        }
    }
    
    /* No solution could be found */
    if (notValid == TRUE)
    {
        err = FM_ERR_SCHED_VIOLATION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "Inserting port %d in the schedule at slot %d\n",
                     port,
                     speedBin[bin]);

        sInfo->tmp.schedList[speedBin[bin]].port = port;
        sInfo->tmp.speedList[speedBin[bin]] |= SPEED_BIN_USED;

        /* Remove port from bit array */
        err = fmSetBitArrayBit(ba, port, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end AssignPort2500 */




/*****************************************************************************/
/** AssignPorts2500in2500Qpc
 * \ingroup intSwitch
 *
 * \desc            Assign all ports at 2.5G that have the difficulty level
 *                  DIFFICULTY_2500M_IN_MULTI_2500M_QPC.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_VIOLATION if a valid solution could not be
 *                  generated while assigning ports. 
 *
 *****************************************************************************/
static fm_status AssignPorts2500in2500Qpc(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_int              j;
    fm_int              port;
    fm_int              speed;
    fm_int              portCnt;
    fm_int              binCnt;
    fm_int              startBin;
    fm_int              speedBin[FM10000_MAX_SCHEDULE_LENGTH];
    fm_bool             notValid;
    fm_int              next;
    fm_int              dist;
    fm_bitArray *       ba;

     FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    speed = FM10000_SCHED_SPEED_2500M;
    ba    = &sInfo->tmp.p2500M;

    err = fmGetBitArrayNonZeroBitCount(ba, &portCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* First, extract speed bins */
    binCnt = 0;
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        if ( sInfo->tmp.speedList[i] == speed )
        {
            speedBin[binCnt++] = i;
        }
    }

    /* If 1 slot needed per port, sort the speedBins per difficulty */
    for (j = 0; j < binCnt; j++)
    {
        next = j + 1;
        
        if (next >= binCnt)
        {
            next -= binCnt;
        }

        dist = (speedBin[next] & 0xFFFF) - (speedBin[j] & 0xFFFF);

        if (dist < 0)
        {
            dist += sInfo->tmp.schedLen;
        }

        /* Store the distance in the upper 16bits of speedBin,
         * those 16bits will be cleared after sort. */
        speedBin[j] |= ((dist & 0x7FFF) << 16);

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "speedBin[%02d] = slot %03d, dist = %d\n", 
                     j, 
                     speedBin[j] & 0xFFFF, 
                     dist);
    }

    qsort(speedBin, binCnt, sizeof(speedBin[0]), CompareSpeedBins);

    for (j = 0; j < binCnt; j++)
    {
        /* Remove distance encoded in upper 16 bits */
        speedBin[j] &= 0xFFFF;
    }
        
    startBin = 0;

    /* We have 'portCnt' ports to add */
    for (i = 0; i < portCnt; i++)
    {
        err = fmFindBitInBitArray(ba, 0, 1, &port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        notValid = TRUE;
        while ( (notValid) && 
                (err == FM_OK) &&
                (port != -1) )
        {
            notValid = FALSE;
            for (j = startBin; j < binCnt; j += portCnt)
            {
                notValid |= ValidateQuad4Constraint(sw, port, speedBin[j]);

                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                             "port = %d, speedBin[%d] = %d, "
                             "notValid = %d\n", 
                             port, 
                             j, 
                             speedBin[j], 
                             notValid);

                if (notValid)
                {
                    /* Try with the next port */
                    err = fmFindBitInBitArray(ba, port+1, 1, &port);
                    break;
                }
            }
        }

        /* No solution could be found */
        if ( (err != FM_OK) ||
             (port == -1) )
        {
            break;
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                         "Inserting port %d in the schedule\n",
                         port);

            /* We have a valid port */
            for (j = startBin; j < binCnt; j += portCnt)
            {
                sInfo->tmp.schedList[speedBin[j]].port = port;
            }

            /* Remove port from bit array */
            err = fmSetBitArrayBit(ba, port, 0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            startBin++;
        }
    }

    err = fmGetBitArrayNonZeroBitCount(ba, &portCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (portCnt != 0)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "There are %d x 2.5G that could not be placed, "
                     "attempting to use an optimized algorithm\n", portCnt);

        for (i = 0; i < portCnt; i++)
        {
            err = fmFindBitInBitArray(ba, 0, 1, &port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            err = AssignPort2500(sw, port, ba);
            
            /* No solution could be found */
            if (err == FM_ERR_SCHED_VIOLATION)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH, "No solution could be found\n");
            }

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end AssignPorts2500in2500Qpc */




/*****************************************************************************/
/** AssignPortsByDifficulty
 * \ingroup intSwitch
 *
 * \desc            Assign ports by inserting the harder ports to fit first. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_VIOLATION if a valid solution could not be
 *                  generated while assigning ports. 
 *
 *****************************************************************************/
static fm_status AssignPortsByDifficulty(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              port;
    fm_int              diff;
    fm_int              speed;
    fm_int              i;
    fm_bitArray *       ba;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /*************************************************************** 
     * Insert each port (hardest first)
     ***************************************************************/ 
    for (i = 0 ; i < FM10000_NUM_PORTS; i++)
    {
        port  = sInfo->tmp.diffTable[i].port;
        diff  = sInfo->tmp.diffTable[i].diff;
        speed = sInfo->tmp.physPortSpeed[port];

        /* No more ports to add */
        if (diff == DIFFICULTY_INVALID)
        {
            break;
        }
        else if (diff == DIFFICULTY_2500M_IN_MULTI_2500M_QPC)
        {
            /* Treat this difficulty seperatly because ports only require
             * one slot per port. */
            continue;
        }

        switch (speed)
        {
            case FM10000_SCHED_SPEED_2500M:
                ba          = &sInfo->tmp.p2500M;
                break;

            case FM10000_SCHED_SPEED_10G:
                ba          = &sInfo->tmp.p10G;
                break;

            case FM10000_SCHED_SPEED_25G:
                ba          = &sInfo->tmp.p25G;
                break;

            case FM10000_SCHED_SPEED_40G:
                ba          = &sInfo->tmp.p40G;
                break;

            case FM10000_SCHED_SPEED_60G:
                ba          = &sInfo->tmp.p60G;
                break;

            case FM10000_SCHED_SPEED_100G:
                ba          = &sInfo->tmp.p100G;
                break;

            default:
                /* We should never get here */
                err = FM_FAIL;
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH, 
                             "Unexpected failure (speed = %d)\n", 
                             speed);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;
        }

        err = AssignPort(sw, port, ba, speed);

        if ( (err != FM_OK) && (speed == FM10000_SCHED_SPEED_2500M))
        {
            /* Attempt to use an optmized version for 2.5G ports */
            err = AssignPort2500(sw, port, ba);
        }

        if (err == FM_ERR_SCHED_VIOLATION)
        {
            FM_LOG_FATAL(FM_LOG_CAT_SWITCH, "No solution could be found\n");
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /*************************************************************** 
     * Take care of the DIFFICULTY_2500M_IN_MULTI_2500M_QPC
     * difficulty by identifying the distance between each 2500M
     * slots. Each slot reserved for 2500M are then sorted by 
     * difficulty to fill. 
     ***************************************************************/ 

    err = AssignPorts2500in2500Qpc(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    /* Clear the used flag */
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        sInfo->tmp.speedList[i] &= ~SPEED_BIN_USED;
    }

    if (err != FM_OK)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Dumping speedList:\n");
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Entry   Speed\n");
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "------  --------\n");
        
        for (i = 0; i < sInfo->tmp.schedLen; i++)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                         "%3d     %d\n", 
                         i, 
                         sInfo->tmp.speedList[i]);
            sInfo->tmp.speedList[i] &= ~SPEED_BIN_USED;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end AssignPortsByDifficulty */




/*****************************************************************************/
/** RotateSchedule
 * \ingroup intSwitch
 *
 * \desc            Rotates the schedule so that an idle token is in slot 0
 *                  which will be implicit slot.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RotateSchedule(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              src;
    fm_int              dst;
    fm_int              start;
    fm10000_schedSpeed  tmpSpeed;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);
    
    start = -1;

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;
    
    /* Find the first idle token */ 
    for (src = 0; src < sInfo->tmp.schedLen; src++)
    {
        if (sInfo->tmp.speedList[src] == FM10000_SCHED_SPEED_IDLE)
        {
            start = src;
            break;
        }
    }

    /* Sanity check, should never happen */
    if (start == -1)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    src = start;
    dst = 0;

    /* Rotate the ring so that the idle token is at slot 0 */
    while (src != dst)
    {
        /* Swap Src/Dst */
        tmpSpeed                  = sInfo->tmp.speedList[dst];
        sInfo->tmp.speedList[dst] = sInfo->tmp.speedList[src];
        sInfo->tmp.speedList[src] = tmpSpeed; 

        src++;
        dst++;

        if (src == sInfo->tmp.schedLen)
        {
            src = start;
        }
        else if (dst == start)
        {
            start = src;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end RotateSchedule */




/*****************************************************************************/
/** GeneratePortMappingTables
 * \ingroup intSwitch
 *
 * \desc            Generates the physical port to fabric port mapping table
 *                  and its reverse equivalent. 
 * 
 * \param[in]       sw is the switch to operate on. 
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GeneratePortMappingTables(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_int              j;
    fm_int              fabricPort;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_SWITCH, 
                           sInfo->tmp.nbPorts <= FM10000_SCHED_MAX_NUM_PORTS, 
                           err = FM_FAIL,
                           "Number of ports exceeded (%d > %d)\n",
                           sInfo->tmp.nbPorts,
                           FM10000_SCHED_MAX_NUM_PORTS);

    for (i = 0; i < FM10000_NUM_PORTS; i++)
    {
        sInfo->physicalToFabricMap[i] = -1;

        for (j = 0; j < sInfo->tmp.nbPorts; j++)
        {
            if (sInfo->tmp.portList[j].physPort == i)
            {
                sInfo->physicalToFabricMap[i] = 
                    sInfo->tmp.portList[j].fabricPort;
                break;
            }
        }
    }

    for (i = 0; i < FM10000_NUM_FABRIC_PORTS; i++)
    {
        sInfo->fabricToPhysicalMap[i] = -1;
    }

    for (i = 0; i < FM10000_NUM_PORTS; i++)
    {
        fabricPort = sInfo->physicalToFabricMap[i];

        if (fabricPort != -1)
        {
            sInfo->fabricToPhysicalMap[fabricPort] = i;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end GeneratePortMappingTables */




/*****************************************************************************/
/** PopulateSpeedList
 * \ingroup intSwitch
 *
 * \desc            This function populates the speed list table
 *                  based on the number of slots needed for each speed. 
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       slots100G is for 100G slots.
 * 
 * \param[in]       slots60G is for 60G slots.
 * 
 * \param[in]       slots40G is for 40G slots.
 * 
 * \param[in]       slots25G is for 25G slots.
 * 
 * \param[in]       slots10G is for 10G slots.
 * 
 * \param[in]       slots2500M is for 2500M slots.
 * 
 * \param[in]       slotsIdle is for idle slots.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PopulateSpeedList(fm_int sw, 
                                   fm_int slots100G,
                                   fm_int slots60G,
                                   fm_int slots40G,
                                   fm_int slots25G,
                                   fm_int slots10G,
                                   fm_int slots2500M,
                                   fm_int slotsIdle)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              slotsHS;
    fm_int              slots10GRsvd;
    fm10000_schedSpeed  speedHS;
    fm_int              i;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;
    slotsHS   = 0;
    speedHS   = FM10000_SCHED_SPEED_40G;

    /* Initialize all slots as unused */
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        sInfo->tmp.speedList[i] = FM10000_SCHED_SPEED_ANY;
    }

    /* Determine the number of High Speed (HS) slots
     * 100G  or 60G or 40G. */

    if (slots100G)
    {
        slotsHS = slots100G;
        speedHS = FM10000_SCHED_SPEED_100G;
    }
    else if (slots60G)
    {
        slotsHS = slots60G;
        speedHS = FM10000_SCHED_SPEED_60G;
    }
    else if (slots40G)
    {
        slotsHS = slots40G;
        speedHS = FM10000_SCHED_SPEED_40G;
    }

    slots10GRsvd = sInfo->tmp.schedLen - slotsHS;

    /* Split one speed bin into two. Jitter increases as 
     * the starting category gets sparser */
    err = SplitBandwidth(sw,
                         FM10000_SCHED_SPEED_ANY,
                         slots10GRsvd,
                         FM10000_SCHED_SPEED_10G_RSVD,
                         slotsHS,
                         speedHS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* 10G reserved BW is a perfect multiple of 10G requirement,
     * so 10G ports should fill in with no extra jitter 
     *
     * Extra jitter on highest speed (100G or 40G) is 
     * small, <10G of bandwidth */

    if (slots100G > 0)
    { 
         err = SplitBandwidth(sw,
                              FM10000_SCHED_SPEED_10G_RSVD,
                              slots10G,
                              FM10000_SCHED_SPEED_10G,
                              slots10GRsvd - slots10G,
                              FM10000_SCHED_SPEED_NOT_10G_NOT_100G);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* 40G and 25G can have higher jitter. */
        err = SplitBandwidth(sw,
                             FM10000_SCHED_SPEED_NOT_10G_NOT_100G,
                             slots60G,
                             FM10000_SCHED_SPEED_60G,
                             slotsIdle + slots40G + slots25G + slots2500M,
                             FM10000_SCHED_SPEED_NOT_10G_NOT_60G_NOT_100G);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = SplitBandwidth(sw,
                             FM10000_SCHED_SPEED_NOT_10G_NOT_60G_NOT_100G,
                             slots40G,
                             FM10000_SCHED_SPEED_40G,
                             slotsIdle + slots25G + slots2500M,
                             FM10000_SCHED_SPEED_IDLE_25G_2500M);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }
    else if (slots60G > 0)
    {
        err = SplitBandwidth(sw,
                             FM10000_SCHED_SPEED_10G_RSVD,
                             slots10G,
                             FM10000_SCHED_SPEED_10G,
                             slots10GRsvd - slots10G,
                             FM10000_SCHED_SPEED_NOT_10G_NOT_60G_NOT_100G);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = SplitBandwidth(sw,
                             FM10000_SCHED_SPEED_NOT_10G_NOT_60G_NOT_100G,
                             slots40G,
                             FM10000_SCHED_SPEED_40G,
                             slotsIdle + slots25G + slots2500M,
                             FM10000_SCHED_SPEED_IDLE_25G_2500M);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }
    else
    {
        err = SplitBandwidth(sw,
                             FM10000_SCHED_SPEED_10G_RSVD,
                             slots10G,
                             FM10000_SCHED_SPEED_10G,
                             slots10GRsvd - slots10G,
                             FM10000_SCHED_SPEED_IDLE_25G_2500M);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    err = SplitBandwidth(sw,
                         FM10000_SCHED_SPEED_IDLE_25G_2500M,
                         slots25G,
                         FM10000_SCHED_SPEED_25G,
                         slotsIdle + slots2500M,
                         FM10000_SCHED_SPEED_IDLE_2500M);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Idle and 2.5G don't repeat in schedule, hence can't jitter at all */
    err = SplitBandwidth(sw,
                         FM10000_SCHED_SPEED_IDLE_2500M,
                         slotsIdle,
                         FM10000_SCHED_SPEED_IDLE,
                         slots2500M,
                         FM10000_SCHED_SPEED_2500M);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end PopulateSpeedList */




/*****************************************************************************/
/** StripeDifficultyTable
 * \ingroup intSwitch
 *
 * \desc            For ports that have the same difficulty 
 *                  level, make sure that they are stripped 
 *                  by QPC.
 * 
 * \param[in]       sw is the switch to operate on. 
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status StripeDifficultyTable(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_int              j;
    fm_int              startDiffIndex;
    fm_int              currentDiffIndex;
    
    fm_int              currDiff;
    fm_int              lastDiff;
    fm_int              port;
    fm_int              qpc;

    fm_int              qpcUsage[NUM_QPC][NUM_PORTS_PER_QPC];
    fm_int              qpcCnt[NUM_QPC];

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    startDiffIndex   = 0;
    currentDiffIndex = 0;
    currDiff         = 0;
    
    while ( (currentDiffIndex < FM10000_NUM_PORTS) && 
            (currDiff != -1) )
    {
        startDiffIndex = currentDiffIndex;

        currDiff = sInfo->tmp.diffTable[currentDiffIndex].diff;
        lastDiff = currDiff;

        /* Clear the structures */
        FM_CLEAR(qpcCnt);

        for (i = 0; i < NUM_QPC; i++)
        {
            for (j = 0; j < NUM_PORTS_PER_QPC; j++)
            {
                qpcUsage[i][j] = -1;
            }
        }
        
        /* Insert each port in qpcUsage */
        while ( (currDiff == lastDiff) && (currDiff != -1) )
        {
            lastDiff = currDiff;

            port  = sInfo->tmp.diffTable[currentDiffIndex++].port;
            qpc   = sInfo->physicalToFabricMap[port] / 4;
            
            qpcUsage[qpc][qpcCnt[qpc]] = port;
            qpcCnt[qpc]++;

            if (currentDiffIndex >= FM10000_NUM_PORTS)
            {
                currDiff = -1;
            }
            else
            {
                currDiff = sInfo->tmp.diffTable[currentDiffIndex].diff; 
            }
        }

        currentDiffIndex = startDiffIndex;

        /* Update the diff table, sorting the entries by the most used QPC port */
        for (j = (NUM_PORTS_PER_QPC-1); j >= 0; j--)
        {
            for (i = 0; i < NUM_QPC; i++)
            {
                if (qpcUsage[i][j] != -1)
                {
                    sInfo->tmp.diffTable[currentDiffIndex++].port = qpcUsage[i][j];
                    qpcUsage[i][j] = -1;
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end StripeDifficultyTable */




/*****************************************************************************/
/** SortPortsByDifficulty
 * \ingroup intSwitch
 *
 * \desc            Sorts the ports by the difficulty of placing them in
 *                  the schedule. Subsequently the ports can be placed
 *                  hardest first. 
 * 
 * \param[in]       sw is the switch to operate on. 
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SortPortsByDifficulty(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_int              j;
    fm_int              port;
    fm_int              n2500M;
    fm_int              n10G;
    fm_int              n25G;
    fm_int              qpc;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);
    
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /* Clear the difficulty level */
    for (port = 0; port < FM10000_NUM_PORTS; port++)
    {
        sInfo->tmp.diffTable[port].port = port;
        sInfo->tmp.diffTable[port].diff = DIFFICULTY_INVALID;
    }

    /*******************************************
     * First sort the ports by difficulty level
     ******************************************/
    n2500M = 0;
    n10G = 0;
    n25G = 0;
    for (i = 0; i < FM10000_NUM_FABRIC_PORTS; i++)
    {
        /* Compute info for the QPC */
        if ((i % 4) == 0)
        {
            n2500M = 0;
            n10G = 0;
            n25G = 0;

            for (j = 0; j < 4; j++)
            {
                switch (sInfo->tmp.fabricPortSpeed[i+j])
                {
                    case FM10000_SCHED_SPEED_2500M:
                        n2500M++;
                        break;

                    case FM10000_SCHED_SPEED_10G:
                        n10G++;
                        break;

                    case FM10000_SCHED_SPEED_25G:
                        n25G++;
                        break;

                    default:
                        /* do nothing */
                        break;
                }
            }
        }

        port = sInfo->fabricToPhysicalMap[i];

        if (port == -1)
        {
            continue;
        }

        if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_25G) &&
             (n10G > 0) )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_25G_IN_10G25G_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_10G) &&
                  (n25G > 0))
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_10G_IN_10G25G_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_25G) &&
                  (n25G > 1) )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_25G_IN_MULTI_25G_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_10G) &&
                  (n10G > 1) )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_10G_IN_MULTI_10G_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_25G) &&
                  (n2500M > 0) )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_25G_IN_MULTI_SPEED_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_10G) &&
                  (n2500M > 0) )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_10G_IN_MULTI_SPEED_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_2500M) &&
                  ( (n25G > 0) || (n10G > 0) ) )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_2500M_IN_MULTI_SPEED_QPC;
        }
        else if ( (sInfo->tmp.fabricPortSpeed[i] == FM10000_SCHED_SPEED_2500M) &&
                  (n25G == 0) && (n10G == 0) && n2500M > 0 )
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_2500M_IN_MULTI_2500M_QPC;
        }
        else if ( sInfo->tmp.fabricPortSpeed[i] <= FM10000_SCHED_SPEED_IDLE)
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_INVALID;
        } 
        else 
        {
            sInfo->tmp.diffTable[port].diff = DIFFICULTY_OTHER;
        }
    }

    qsort(sInfo->tmp.diffTable, 
          FM10000_NUM_PORTS, 
          sizeof(fm10000_schedPortDifficulty), 
          CompareDifficulty);

    /*******************************************
     * For ports that have the same difficulty 
     * level, make sure that they are stripped 
     * by QPC.
     ******************************************/

    err = StripeDifficultyTable(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Diff  Port  QPC\n");
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "----  ----  ----\n");
    
    for (i = 0; i < FM10000_NUM_PORTS; i++)
    {
        if (sInfo->tmp.diffTable[i].diff != -1)
        {
            port  = sInfo->tmp.diffTable[i].port;
            qpc   = sInfo->physicalToFabricMap[port] / 4;

            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                         "%3d   %3d   %3d\n", 
                         sInfo->tmp.diffTable[i].diff, 
                         sInfo->tmp.diffTable[i].port,
                         qpc); 
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SortPortsByDifficulty */




/*****************************************************************************/
/** InitStatStruct
 * \ingroup intSwitch
 *
 * \desc            Initialize a stat structure.
 * 
 * \param[in]       statPtr is a pointer to the stat struct to initialize. 
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitStatStruct(fm10000_schedStat *statPtr)
{
    fm_status err = FM_OK;

    if (statPtr == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    else
    {
        statPtr->speed   = FM10000_SCHED_SPEED_ANY;
        statPtr->first   = -1;
        statPtr->last    = -1;
        statPtr->minDiff = 9999;
        statPtr->maxDiff = 0;    
        statPtr->minLoc  = -1;
        statPtr->maxLoc  = -1;
        statPtr->cnt     = 1;
    }

ABORT:
    return err;

}   /* end InitStatStruct */




/*****************************************************************************/
/** FreeStatEntry
 * \ingroup intLbg
 *
 * \desc            This method frees the stat entry
 *                  member list.
 *
 * \param[in]       ptr points to the object being freed.
 *
 * \return          None
 *
 *****************************************************************************/
static void FreeStatEntry(void *ptr)
{
	if (ptr != NULL)
	{
		fmFree(ptr);
	}

}   /* end FreeStatEntry */




/*****************************************************************************/
/** FreeSchedEntryInfo
 * \ingroup intLbg
 *
 * \desc            This method frees the scheduler entry info
 *
 * \param[in]       ptr points to the object being freed.
 *
 * \return          None
 *
 *****************************************************************************/
static void FreeSchedEntryInfo(void *ptr)
{
	if (ptr != NULL)
	{
		fmFree(ptr);
	}

}   /* end FreeSchedEntryInfo */




/*****************************************************************************/
/** ComputeStats
 * \ingroup intSwitch
 *
 * \desc            Compute stats for a given type (speed, qpc or port)
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       statType specifies the type of stats to be computed.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ComputeStats(fm_int sw, fm_int statType)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_tree            *treePtr;
    fm_uint64           treeKey;
    fm10000_schedStat  *statPtr;
    fm_treeIterator     it;
    fm_int              i;
    fm_int              diff;

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    switch (statType)
    {
        case STAT_TYPE_SPEED:
            treePtr = &sInfo->speedStatsTree; 
            break;

        case STAT_TYPE_QPC:
            treePtr = &sInfo->qpcStatsTree; 
            break;

        case STAT_TYPE_PORT:
            treePtr = &sInfo->portStatsTree; 
            break;

        default:
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
    /* First pass */
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        /* Skip Idles */
        if (sInfo->tmp.schedList[i].idle == TRUE)
        {
            continue;
        }

        switch (statType)
        {
            case STAT_TYPE_SPEED:
                treeKey = sInfo->tmp.speedList[i];
                break;

            case STAT_TYPE_QPC:
                treeKey = sInfo->tmp.schedList[i].fabricPort / 4;
                break;

            case STAT_TYPE_PORT:
                treeKey = sInfo->tmp.schedList[i].port;
                break;

            default:
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }

        err = fmTreeFind(treePtr,
                         treeKey, 
                         (void **) &statPtr);

        if (err == FM_ERR_NOT_FOUND)
        {
            statPtr = fmAlloc(sizeof(fm10000_schedStat));

            err = InitStatStruct(statPtr);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            statPtr->speed = sInfo->tmp.speedList[i];
            statPtr->first = i;
            statPtr->last  = i;
            
            err = fmTreeInsert(treePtr, 
                               treeKey, 
                               statPtr);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            diff          = i - statPtr->last;
            
            if (diff < statPtr->minDiff)
            {
                statPtr->minDiff = diff;
                statPtr->minLoc  = statPtr->last;
            }
            
            if (diff > statPtr->maxDiff)
            {
                statPtr->maxDiff = diff;
                statPtr->maxLoc  = statPtr->last;
            }

            statPtr->last = i;
            statPtr->cnt++;
        }
    }

    /* Second Pass (Wrap arround). To wrap arround we iterate in each
     * tree element inserted and compare first and last items */
    for (fmTreeIterInit(&it, treePtr);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &statPtr)) == FM_OK ;)
    {
        diff = sInfo->tmp.schedLen - statPtr->last + statPtr->first;

        if (diff < statPtr->minDiff)
        {
            statPtr->minDiff = diff;
            statPtr->minLoc  = statPtr->last;
        }
        
        if (diff > statPtr->maxDiff)
        {
            statPtr->maxDiff = diff;
            statPtr->maxLoc  = statPtr->last;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

ABORT:
    return err;

}   /* end ComputeStats */




/*****************************************************************************/
/** CalcStats
 * \ingroup intSwitch
 *
 * \desc            Calculate per speed/qpc/port stats
 *                  for validation/debug purposes.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CalcStats(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);
    
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    if (fmTreeIsInitialized(&sInfo->speedStatsTree))
    {
        fmTreeDestroy(&sInfo->speedStatsTree, FreeStatEntry); 
        fmTreeDestroy(&sInfo->qpcStatsTree, FreeStatEntry);
        fmTreeDestroy(&sInfo->portStatsTree, FreeStatEntry);
    }

    fmTreeInit(&sInfo->speedStatsTree);
    fmTreeInit(&sInfo->qpcStatsTree);
    fmTreeInit(&sInfo->portStatsTree);

    err = ComputeStats(sw, STAT_TYPE_SPEED);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = ComputeStats(sw, STAT_TYPE_QPC);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = ComputeStats(sw, STAT_TYPE_PORT);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end CalcStats */




/*****************************************************************************/
/** ValidateSchedule
 * \ingroup intSwitch
 *
 * \desc            Validate the schedule generated by GenerateSchedule()
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_VIOLATION if the schedule has an error. 
 *
 *****************************************************************************/
static fm_status ValidateSchedule(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_uint64           treeKey;
    fm10000_schedStat  *statPtr;
    fm_treeIterator     it;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);
    
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /*********************************************** 
     * Validate there are no invalid speed
     **********************************************/
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        if (sInfo->tmp.speedList[i] < 0)
        {
            err = FM_ERR_SCHED_VIOLATION;
            FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                         "Invalid speed bins remaining\n");
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

    /*********************************************** 
     * Validate Minimum Quad Port Spacing of 4 cycles
     **********************************************/
    for (fmTreeIterInit(&it, &sInfo->qpcStatsTree);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &statPtr)) == FM_OK ;)
    {
        if (statPtr->minDiff < 4)
        {
            err = FM_ERR_SCHED_VIOLATION;
            FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                         "Minimum Quad Port Spacing of 4 Cycles Violation\n");
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    /*********************************************** 
     * Validate ports of speed 10G respect 
     * the 1 cycle maximum spacing variation.  
     **********************************************/
    for (fmTreeIterInit(&it, &sInfo->portStatsTree);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &statPtr)) == FM_OK ;)
    {
        if (statPtr->speed == FM10000_SCHED_SPEED_10G)
        {
            if ((statPtr->maxDiff - statPtr->minDiff) > 1)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                         "Maximum Spacing Variation for 10G port exceeds 1\n");
                err = FM_ERR_SCHED_VIOLATION;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
            
            break;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end ValidateSchedule */




/*****************************************************************************/
/** GenerateSchedule
 * \ingroup intSwitch
 *
 * \desc            Generates Rx and Tx scheduler rings
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_OVERSUBSCRIBED if the frequency of
 *                  the chip is not high enough, resulting in oversuscription. 
 * \return          FM_ERR_SCHED_VIOLATION if the schedule could not be
 *                  generated because a violation was detected. 
 *
 *****************************************************************************/
static fm_status GenerateSchedule(fm_int sw)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_schedulerPort   *spPtr;
    fm_int              i;
    fm_int              j;
    fm_int              slots100G;
    fm_int              slots60G;
    fm_int              slots40G;
    fm_int              slots25G;
    fm_int              slots10G;
    fm_int              slots2500M;
    fm_int              slotsIdle;
    fm_schedulerToken  *sToken;
    fm_uint64           logCat;
    fm_uint64           logLvl;
    fm_uint32           rv;
    fm_uint32           pcieHost;
    fm_uint32           pep;
    fm_bool             quad;

    fm_timestamp       tStart = {0,0};
    fm_timestamp       tGen   = {0,0};
    fm_timestamp       tDiff  = {0,0};
    fmGetTime(&tStart);
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /* Initialize Internal Structures */
    err = fmCreateBitArray(&sInfo->tmp.p2500M, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p10G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p25G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p40G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p60G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p100G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = ComputeScheduleLength(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = FilterBwDuplicates(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /*********************************************
     * Sort all ports into speed bins 
     *********************************************/

    for (i = 0; i < sInfo->tmp.nbPorts; i++)
    {
        spPtr = &sInfo->tmp.portList[i];

        if (spPtr->speed == 0)
        {
            /* Inactive port, continue */
            continue;
        }

        switch (spPtr->speed)
        {
            /* 1G ports are stored in the same bin as 2.5G to limit the number
             * of speed bins */
            case 1000:
            case 2500:
                sInfo->tmp.physPortSpeed[spPtr->physPort] = FM10000_SCHED_SPEED_2500M;
                sInfo->tmp.fabricPortSpeed[spPtr->fabricPort] = FM10000_SCHED_SPEED_2500M;

                err = fmSetBitArrayBit(&sInfo->tmp.p2500M, spPtr->physPort, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case 10000:
                sInfo->tmp.physPortSpeed[spPtr->physPort] = FM10000_SCHED_SPEED_10G;
                sInfo->tmp.fabricPortSpeed[spPtr->fabricPort] = FM10000_SCHED_SPEED_10G;

                err = fmSetBitArrayBit(&sInfo->tmp.p10G, spPtr->physPort, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case 25000:
                sInfo->tmp.physPortSpeed[spPtr->physPort] = FM10000_SCHED_SPEED_25G;
                sInfo->tmp.fabricPortSpeed[spPtr->fabricPort] = FM10000_SCHED_SPEED_25G;

                err = fmSetBitArrayBit(&sInfo->tmp.p25G, spPtr->physPort, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            /* 50G ports (will usually be PCIE) can safely be but in
             * the 40G bin, because the host interface only achieves 
             * 50G for 256B+ frames; it doesn't need to be fully provisioned 
             * for minsize frames. See bugzilla #25673 comment #12 */
            case 40000:
            case 50000:
                sInfo->tmp.physPortSpeed[spPtr->physPort] = FM10000_SCHED_SPEED_40G;
                sInfo->tmp.fabricPortSpeed[spPtr->fabricPort] = FM10000_SCHED_SPEED_40G;

                err = fmSetBitArrayBit(&sInfo->tmp.p40G, spPtr->physPort, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case 60000:
                sInfo->tmp.physPortSpeed[spPtr->physPort] = FM10000_SCHED_SPEED_60G;
                sInfo->tmp.fabricPortSpeed[spPtr->fabricPort] = FM10000_SCHED_SPEED_60G;

                err = fmSetBitArrayBit(&sInfo->tmp.p60G, spPtr->physPort, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case 100000:
                sInfo->tmp.physPortSpeed[spPtr->physPort] = FM10000_SCHED_SPEED_100G;
                sInfo->tmp.fabricPortSpeed[spPtr->fabricPort] = FM10000_SCHED_SPEED_100G;

                err = fmSetBitArrayBit(&sInfo->tmp.p100G, spPtr->physPort, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            default:
                err = FM_ERR_SCHED_VIOLATION;
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Invalid Speed for entry: " 
                             "physPort=%d, fabricPort=%d speed=%d\n",
                             spPtr->physPort,
                             spPtr->fabricPort,
                             spPtr->speed);
                goto ABORT;
                break;
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 100G Ports", GetNbPorts(&sInfo->tmp.p100G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 60G Ports", GetNbPorts(&sInfo->tmp.p60G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 40G Ports", GetNbPorts(&sInfo->tmp.p40G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 25G Ports", GetNbPorts(&sInfo->tmp.p25G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 10G Ports", GetNbPorts(&sInfo->tmp.p10G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 2.5G Ports", GetNbPorts(&sInfo->tmp.p2500M) );

    /*********************************************
     * Validate that the frequency is sufficiently
     * high to support 100G, 60G and 40G ports
     *********************************************/
    if ( GetNbPorts(&sInfo->tmp.p100G) &&
         (sInfo->tmp.schedLen < (MIN_PORT_SPACING * SLOTS_PER_100G) ) )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "freq not high enough to support 100G w/4-cycle spacing\n");
        goto ABORT;
    }

    if ( GetNbPorts(&sInfo->tmp.p60G) &&
         (sInfo->tmp.schedLen < (MIN_PORT_SPACING * SLOTS_PER_60G) ) )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "freq not high enough to support 60G w/4-cycle spacing\n");
        goto ABORT;
    }

    if ( GetNbPorts(&sInfo->tmp.p40G) &&
         (sInfo->tmp.schedLen < (MIN_PORT_SPACING * SLOTS_PER_40G) ) )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "freq not high enough to support 40G w/4-cycle spacing\n");
        goto ABORT;
    }

    /*********************************************
     * Compute number of slots required per 
     * speed bin. 
     ********************************************/
    slots100G   = GetNbPorts(&sInfo->tmp.p100G)  * SLOTS_PER_100G;
    slots60G    = GetNbPorts(&sInfo->tmp.p60G)   * SLOTS_PER_60G;
    slots40G    = GetNbPorts(&sInfo->tmp.p40G)   * SLOTS_PER_40G;
    slots25G    = GetNbPorts(&sInfo->tmp.p25G)   * SLOTS_PER_25G;
    slots10G    = GetNbPorts(&sInfo->tmp.p10G)   * SLOTS_PER_10G;
    slots2500M  = GetNbPorts(&sInfo->tmp.p2500M) * SLOTS_PER_2500M;
    slotsIdle   = sInfo->tmp.schedLen - slots100G - slots60G - slots40G - slots25G - slots10G - slots2500M;
    
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 100G Slots", slots100G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 60G Slots", slots60G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 40G Slots", slots40G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 25G Slots", slots25G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 10G Slots", slots10G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 2.5G Slots", slots2500M);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number Idle Slots", slotsIdle);

    if (slotsIdle <= 0)
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Oversubscribed schedule, minimum of 1 idle slot needed\n");
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Requested %d slots (%.1fG), but only %d are available (%.1fG)\n",
                     slots100G + slots60G + slots40G + slots25G + slots10G + slots2500M + 1,
                     ((slots100G + slots60G + slots40G + slots25G + slots10G + slots2500M + 1) * (fm_float)(SLOT_SPEED)),
                     sInfo->tmp.schedLen,
                     (sInfo->tmp.schedLen * (fm_float)(SLOT_SPEED)));
        goto ABORT;
    }

    /*********************************************
     * Split the bandwidth by populating the slots 
     * with port speeds
     *********************************************/
    err = PopulateSpeedList(sw, slots100G, slots60G, slots40G, slots25G, slots10G, slots2500M, slotsIdle );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);


    /*********************************************
     * Need to find the first idle token, this 
     * will be our implicit idle and it does not
     * need to be added to the schedule 
     *********************************************/
    err = RotateSchedule(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /*********************************************
     * Sort the ports by the difficulty level 
     * of placing them in the schedule.
     *********************************************/
    err = SortPortsByDifficulty(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /*********************************************
     * We have the speed bins, fill them with 
     * ports
     *********************************************/
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        /* Mark all ports as invalid */
        sInfo->tmp.schedList[i].port = -1;
    }

    err = AssignPortsByDifficulty(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Assign Idle Tokens */
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        if ( sInfo->tmp.speedList[i] == FM10000_SCHED_SPEED_IDLE )
        {
            sToken = &sInfo->tmp.schedList[i];
            sToken->port        = 0;
            sToken->fabricPort  = 0;
            sToken->quad        = 0;
            sToken->idle        = 1;
        }
    }

    /* Fill in the fabricPort, Quad, and Idle fields per portList entries */
    for (i = 0; i < sInfo->tmp.nbPorts; i++)
    {
        /* Get Quad bit from port type */
        if ( (sInfo->tmp.portList[i].fabricPort >= FM10000_FIRST_PCIE_FABRIC_PORT) &&
             (sInfo->tmp.portList[i].fabricPort <= FM10000_LAST_PCIE_FABRIC_PORT) )
        {
            /* Get Bifurcation Mode */
            err = switchPtr->ReadUINT32(sw, FM10000_DEVICE_CFG(), &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            pcieHost = (sInfo->tmp.portList[i].fabricPort - 
                        FM10000_FIRST_PCIE_FABRIC_PORT) / 4;
            pep      = (sInfo->tmp.portList[i].fabricPort - 
                        FM10000_FIRST_PCIE_FABRIC_PORT) / 
                        FM10000_PORTS_PER_PCIE_FABRIC_PORT;

            /* If in x4, there is one channel, if in x8, there are 4 channels */
            if ( (pep % 2 == 1 ) ||
                 (rv & (1 << pcieHost)) )
            {
                quad = 0;
            }
            else
            {
                quad = 1;
            }
        }
        else if ( (sInfo->tmp.portList[i].fabricPort >= FM10000_FIRST_TE_FABRIC_PORT) &&
                  (sInfo->tmp.portList[i].fabricPort <= FM10000_LAST_TE_FABRIC_PORT) )
        {
            quad = 1;
        } 
        else if (sInfo->tmp.portList[i].speed > 25000)
        {
            quad = 1;
        }
        else
        {
            /* Default for EPL, Loopback, PCIE[8] and FIBM ports */
            quad = 0;
        }

        for (j = 0; j < sInfo->tmp.schedLen; j++)
        {
            if ( (sInfo->tmp.portList[i].physPort == sInfo->tmp.schedList[j].port) &&
                 (sInfo->tmp.schedList[j].idle == 0) )
            {
                sInfo->tmp.schedList[j].fabricPort = sInfo->tmp.portList[i].fabricPort;
                sInfo->tmp.schedList[j].quad       = quad;
                sInfo->tmp.schedList[j].idle       = 0;
            }
        }

        sInfo->tmp.isQuad[sInfo->tmp.portList[i].physPort] = quad;
    }

    fmGetLoggingAttribute(FM_LOG_ATTR_CATEGORY_MASK, 
                          0, 
                          (void *) &logCat);
    fmGetLoggingAttribute(FM_LOG_ATTR_LEVEL_MASK, 
                          0, 
                          (void *) &logLvl);
    
    if ( (logCat & FM_LOG_CAT_SWITCH) &&
         (logLvl & FM_LOG_LEVEL_DEBUG) )
    {
        DbgDumpSchedulerConfig(sw, TMP_SCHEDULE, FALSE);
    }

    err = CalcStats(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = ValidateSchedule(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    
    /* Delete Internal BitArrays */
    fmDeleteBitArray(&sInfo->tmp.p2500M);
    fmDeleteBitArray(&sInfo->tmp.p10G);
    fmDeleteBitArray(&sInfo->tmp.p25G);
    fmDeleteBitArray(&sInfo->tmp.p40G);
    fmDeleteBitArray(&sInfo->tmp.p60G);
    fmDeleteBitArray(&sInfo->tmp.p100G);

    fmGetTime(&tGen);

    fmSubTimestamps(&tGen, &tStart, &tDiff);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                 "Generation Length = %lld us\n", 
                 tDiff.usec + tDiff.sec * 1000000);
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end GenerateSchedule */




/*****************************************************************************/
/** GenerateQpcState
 * \ingroup intSwitch
 *
 * \desc            Generates Quad Port Channel State to be able to
 *                  later support state changes in port speed. 
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       schedList is a pointer to the list of scheduler tokens.
 * 
 * \param[in]       schedLen is the length of schedlist.
 * 
 * \param[in]       init should be set to TRUE if the QPC State is generated
 *                  for the first time.
 * 
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_NO_MEM if there was a memory allocation problem
 *
 *****************************************************************************/
static fm_status GenerateQpcState(fm_int             sw, 
                                  fm_schedulerToken *schedList, 
                                  fm_int             schedLen,
                                  fm_bool            init)
{
    fm_status               err = FM_OK;
    fm10000_switch *        switchExt;
    fm10000_schedInfo  *    sInfo;
    fm_int                  i;
    fm_int                  qpc;
    fm_int                  qpcLastLane[FM10000_NUM_QPC];
    fm10000_schedEntryInfo *entry;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /* Start from a clean state */
    for (i = 0; i < FM10000_NUM_QPC; i++)
    {
        if (fmTreeIsInitialized(&sInfo->qpcState[i]))
        {
            fmTreeDestroy(&sInfo->qpcState[i], fmFree); 
        }

        fmTreeInit(&sInfo->qpcState[i]);

        qpcLastLane[i] = 0;
    }
    
    for (i = 0; i < schedLen; i++)
    {
        /* Skip Idle Tokens */
        if (schedList[i].idle == 1)
        {
            continue;
        }

        entry = fmAlloc(sizeof(fm10000_schedEntryInfo));
        if ( entry != NULL )
        {
            qpc = schedList[i].fabricPort / 4;

            /* If the token is used for a quad port, we must alternate the lane
             * such that the tokens can be used when a non-quad mode is used */
            if (schedList[i].quad == 1)
            {
                entry->lane = qpcLastLane[qpc]++;
                qpcLastLane[qpc] %= 4;
            }
            else
            {
                entry->lane = schedList[i].fabricPort % 4;
            }

            entry->quad = schedList[i].quad;
            entry->app  = AUTO_APP;

            /* On init only, mark the EPL ports as FREE_ENTRY as they will be
             * initialized based on the default ethernet mode.
             * Additionnaly, set the slot as idle to prevent any conflict. */
            if ( (schedList[i].fabricPort >= FM10000_FIRST_EPL_FABRIC_PORT) && 
                 (schedList[i].fabricPort <= FM10000_LAST_EPL_FABRIC_PORT) &&
                 (init == TRUE) )
            {
                entry->afp = FREE_ENTRY;
                schedList[i].idle = TRUE;
            }
            else
            {
                entry->afp = schedList[i].fabricPort;
            }
            
            err = fmTreeInsert(&sInfo->qpcState[qpc], i, (void *) entry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
        else
        {
            err = FM_ERR_NO_MEM;
            break;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end GenerateQpcState */




/*****************************************************************************/
/** ReserveSchedBw
 * \ingroup intSwitch
 *
 * \desc            PreReserves or Reserves BW for a given port. Use a 
 *                  speed of 0 to free the BW for a given port. At any given 
 *                  time, the sum of pre-reserved or reserved BW must be equal 
 *                  or below the maximum BW the system supports. If the BW 
 *                  request would cause the reserved BW to go over the maximum 
 *                  BW, this function will return an error. 
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port.
 * 
 * \param[in]       speed is the speed to reserve of physical port. A value
 *                  of FM10000_SCHED_SPEED_DEFAULT may be used to default
 *                  the speed to what was assigned at initialization (from LT
 *                  config file). 
 * 
 * \param[in]       mode is the quad channel mode of physical port.
 * 
 * \param[in]       isPreReserve is determines whether preReserved or reserved
 *                  BW to be updated.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if port or speed are out of range.
 * \return          FM_ERR_SCHED_OVERSUBSCRIBED if there is not enough BW
 *                  available and can't reserve the requested BW. 
 *
 *****************************************************************************/
fm_status ReserveSchedBw(fm_int               sw,
                         fm_int               physPort,
                         fm_int               speed,
                         fm_schedulerPortMode mode,
                         fm_bool              isPreReserve)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              fabricPort;
    fm_int              lastSpeed;
    fm_int              lastQuad;
    fm_int              i;
    fm_int              slots;
    fm_int              opPorts[NUM_PORTS_PER_QPC];
    fm_int              basePort;
    fm_int              totalBW;
    fm_int              nbMultiLane;
    fm10000_schedSpeed *speedTable;
    fm_bool            *quadTable;   
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, physPort = %d, speed = %d, mode = %d, "
                 "isPreReserve = %d\n",
                 sw, physPort, speed, mode, isPreReserve);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    if (isPreReserve)
    {
        speedTable = sInfo->preReservedSpeed;
        quadTable  = sInfo->preReservedQuad;
    }
    else
    {
        speedTable = sInfo->reservedSpeed;
        quadTable  = sInfo->reservedQuad;
    }

    lastSpeed = -1;
    
    if (speed == FM10000_SCHED_SPEED_DEFAULT)
    {
        /* Grab the speed set during init (which is the LT config file defined
         * speed */
        for (i = 0; i < sInfo->active.nbPorts; i++)
        {
            if (physPort == sInfo->active.portList[i].physPort)
            {
                speed = sInfo->active.portList[i].speed;
            }
        }
    }

    /* Validate / Round up the speed to the closest upper speed */
    if (speed < 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else if (speed == 0)
    {
        /* Do nothing */
    }
    else if (speed <= 2500)
    {
        speed = 2500;
    }
    else if (speed <= 10000)
    {
        speed = 10000;
    }
    else if (speed <= 25000)
    {
        speed = 25000;
    }
    else if (speed <= 40000)
    {
        speed = 40000;
    }
    else if (speed == 50000)
    {
        /* Special case for PCIE, round down to 40G */
        speed = 40000;
    }
    else if (speed == 60000)
    {
        speed = 60000;
    }
    else if (speed <= 100000)
    {
        speed = 100000;
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Convert the physPort into fabricPort */
    err = fm10000MapPhysicalPortToFabricPort(sw, physPort, &fabricPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    basePort = fabricPort - (fabricPort % 4);
    for (i = 0; i < NUM_PORTS_PER_QPC; i++)
    {
        /* Populate other physical ports list (if they exist) */
        err = fm10000MapFabricPortToPhysicalPort(sw, basePort + i, &opPorts[i]);

        if (err == FM_ERR_INVALID_PORT)
        {
            err = FM_OK;
            opPorts[i] = -1;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Cache old values (in case of error) and update */
    lastSpeed = speedTable[physPort];
    lastQuad  = quadTable[physPort];
    speedTable[physPort] = speed;
    quadTable[physPort]  = (mode == FM_SCHED_PORT_MODE_QUAD);

    /******************************************* 
     * 1. Need to make sure the QPC's total 
     *    BW is not exceeded.
     *******************************************/ 
    totalBW = 0;
    nbMultiLane = 0;
    for (i = 0; i < NUM_PORTS_PER_QPC; i++)
    {
        if (opPorts[i] == -1)
        {
            /* No port */
            continue;
        }

        totalBW += speedTable[opPorts[i]];

        if ( (speedTable[opPorts[i]] > FM10000_SCHED_SPEED_25G) ||
             (quadTable[opPorts[i]] ) )
        {
            nbMultiLane++;
        }
    }

    if ( totalBW > FM10000_SCHED_SPEED_100G )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "Cannot have QPC with BW > 100G (QPC Total BW = %d)\n", 
                     totalBW);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

     if (nbMultiLane > 1)
     {
         err = FM_ERR_SCHED_OVERSUBSCRIBED;
         FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                      "Cannot have more than one multi-lane port on the same "
                      "QPC (nbMultiLane = %d)\n", 
                      nbMultiLane);
         FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
     }

     /* It is assumed that the only multilane port that can exist is the one
      * being reserved, hence the comparison (totalBW != speed) */
     if ( (nbMultiLane == 1) && (totalBW != speed) )
     {
         err = FM_ERR_SCHED_OVERSUBSCRIBED;
         FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                      "Cannot have a multi-lane port simultaniously with single"
                      "lane ports on the same QPC\n");
         FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
     }

    /******************************************* 
     * 2. Validate there is enough BW in the 
     *    system with the change. 
     *******************************************/ 
    
    /* Take into account the idle Slot */
    slots = 1;

    for (i = 0; i < FM10000_SCHED_NUM_PORTS; i++)
    {
        switch (speedTable[i])
        {
            case FM10000_SCHED_SPEED_IDLE:
                break;

            case FM10000_SCHED_SPEED_2500M:
                slots += SLOTS_PER_2500M;
                break;

            case FM10000_SCHED_SPEED_10G:
                slots += SLOTS_PER_10G;
                break;

            case FM10000_SCHED_SPEED_25G:
                slots += SLOTS_PER_25G;
                break;

            case FM10000_SCHED_SPEED_40G:
                slots += SLOTS_PER_40G;
                break;

            case FM10000_SCHED_SPEED_60G:
                slots += SLOTS_PER_60G;
                break;

            case FM10000_SCHED_SPEED_100G:
                slots += SLOTS_PER_100G;
                break;

            default:
                /* We should never get here */
                err = FM_FAIL;
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH, "Unexpected failure\n");
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;
        }
    }

    if (slots > sInfo->active.schedLen)
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "slots=%d > schedLen=%d\n", 
                     slots, 
                     sInfo->active.schedLen);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    if ( (err != FM_OK) && (lastSpeed != -1) )
    {
        speedTable[physPort] = lastSpeed;
        quadTable[physPort]  = lastQuad;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end ReserveSchedBw */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000InitScheduler
 * \ingroup intSwitch
 *
 * \desc            Initializes the scheduler.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitScheduler(fm_int sw)
{
    fm_status          err = FM_OK;
    fm10000_switch *   switchExt;
    fm10000_schedInfo *sInfo;
    fm_schedulerConfig sc;
    fm_int             i;
    fm_text            schedModeStr;
    fm_int             fabricPort;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    TAKE_SCHEDULER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    err = InitializeFreeLists(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    schedModeStr = GET_FM10000_PROPERTY()->schedMode;

    sInfo->attr.updateLnkChange = GET_FM10000_PROPERTY()->updateSchedOnLinkChange;

    if (strcmp(schedModeStr, "static") == 0)
    {
        sInfo->attr.mode = FM10000_SCHED_MODE_STATIC;
    }
    else if (strcmp(schedModeStr, "dynamic") == 0)
    {
        sInfo->attr.mode = FM10000_SCHED_MODE_DYNAMIC;
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "%s is not a valid scheduler mode\n", 
                     schedModeStr);
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                 "Scheduler Mode = %s (%d), updateLnkChange = %d\n", 
                 schedModeStr, 
                 sInfo->attr.mode,
                 sInfo->attr.updateLnkChange);
    
    err = fmPlatformGetSchedulerConfig(sw, &sc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    switch ( sc.mode )
    {
        case FM_SCHED_INIT_MODE_NONE:
            /* If the scheduler config mode is none, let the platform code 
             * initialize the scheduler */
            err = FM_OK;
            goto ABORT;

        case FM_SCHED_INIT_MODE_AUTOMATIC:

            FM_CLEAR(sInfo->tmp);

            /* Validate sc->nbPorts */    
            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_SWITCH, 
                           sc.nbPorts <= FM10000_SCHED_MAX_NUM_PORTS, 
                           err = FM_FAIL,
                           "Number of ports exceeded (%d > %d)\n",
                           sc.nbPorts,
                           FM10000_SCHED_MAX_NUM_PORTS);

            /* Copy the portlist locally, so that the API can update it
             * as needed */
            for (i = 0; i < sc.nbPorts; i++)
            {
                sInfo->tmp.portList[i] = sc.portList[i];
            }

            /* In dynamic mode, ignore any speed assigned to ethernet ports
             * as those will be generated on the fly. */
            if (sInfo->attr.mode == FM10000_SCHED_MODE_DYNAMIC)
            {
                for (i = 0; i < sc.nbPorts; i++)
                {
                    fabricPort = sInfo->tmp.portList[i].fabricPort;

                    if ( (fabricPort >= FM10000_FIRST_EPL_FABRIC_PORT) && 
                         (fabricPort <= FM10000_LAST_EPL_FABRIC_PORT) )
                    {
                        sInfo->tmp.portList[i].speed = 0;
                    }
                }
            }

            sInfo->tmp.nbPorts = sc.nbPorts;

            /*********************************************
             * Generate the physical to fabric port 
             * mapping and its reverse
             *********************************************/
            err = GeneratePortMappingTables(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            err = GenerateSchedule(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            err = GenerateQpcState(sw, 
                                   sInfo->tmp.schedList, 
                                   sInfo->tmp.schedLen, 
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            for (i = 0; i < FM10000_SCHED_NUM_PORTS; i++)
            {
                fabricPort = sInfo->physicalToFabricMap[i];

                if ( (fabricPort >= FM10000_FIRST_EPL_FABRIC_PORT) && 
                     (fabricPort <= FM10000_LAST_EPL_FABRIC_PORT) )
                {
                    /* skip, let port API to handle the reservation */
                    continue;
                }

                sInfo->preReservedSpeed[i] = sInfo->tmp.physPortSpeed[i];
                sInfo->preReservedQuad[i]  = sInfo->tmp.isQuad[i];

                sInfo->reservedSpeed[i]    = sInfo->preReservedSpeed[i];
                sInfo->reservedQuad[i]     = sInfo->preReservedQuad[i];
            }

            err = fm10000SetSchedRing(sw, 
                                      FM10000_SCHED_RING_ALL, 
                                      sInfo->tmp.schedList,
                                      sInfo->tmp.schedLen);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* We have succeeded, store the scheduler state into the active
             * structure */
            FM_MEMCPY_S(&sInfo->active, 
                        sizeof(sInfo->active), 
                        &sInfo->tmp, 
                        sizeof(sInfo->tmp) );
            break;

        case FM_SCHED_INIT_MODE_MANUAL:
            /* Not supported yet */
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitScheduler */




/*****************************************************************************/
/** fm10000FreeSchedulerResources
 * \ingroup intSwitch
 *
 * \desc            Free's resources allocated during init/generation
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeSchedulerResources(fm_int sw)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    if (fmTreeIsInitialized(&sInfo->speedStatsTree))
    {
        fmTreeDestroy(&sInfo->speedStatsTree, FreeStatEntry); 
        fmTreeDestroy(&sInfo->qpcStatsTree, FreeStatEntry);
        fmTreeDestroy(&sInfo->portStatsTree, FreeStatEntry);
    }

    for (i = 0; i < FM10000_NUM_QPC; i++)
    {
        if (fmTreeIsInitialized(&sInfo->qpcState[i]))
        {
            fmTreeDestroy(&sInfo->qpcState[i], FreeSchedEntryInfo);
        }
    }

    return err;

}   /* end fm10000FreeSchedulerResources */




/*****************************************************************************/
/** fm10000MapPhysicalPortToFabricPort
 * \ingroup intSwitch
 *
 * \desc            Maps a physical port to a fabric port.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port to convert.
 * 
 * \param[out]      fabricPort is a pointer to the caller allocated storage
 *                  where this function should store the associated fabric port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the physical port is not tied to a
 *                  fabric port.
 *
 *****************************************************************************/
fm_status fm10000MapPhysicalPortToFabricPort(fm_int  sw, 
                                             fm_int  physPort, 
                                             fm_int *fabricPort)
{
    fm_status          err = FM_OK;
    fm10000_switch *   switchExt;
    fm10000_schedInfo *sInfo;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, physPort = %d\n", sw, physPort);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    /* Sanity check */
    if (physPort < 0 || physPort >= FM10000_NUM_PORTS)
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    *fabricPort = sInfo->physicalToFabricMap[physPort]; 

    if ( *fabricPort == -1 )
    {
        err = FM_ERR_INVALID_PORT;
        
        /* silently ABORT (port is not in the map) */
        goto ABORT;
    }

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapPhysicalPortToFabricPort */




/*****************************************************************************/
/** fm10000MapPhysicalPortToEplLane
 * \ingroup intSwitch
 *
 * \desc            Maps a physical port to an EPL/Lane tupple.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port to convert.
 * 
 * \param[out]      epl is a pointer to the caller allocated storage
 *                  where this function should store the associated EPL.
 * 
 * \param[out]      lane is a pointer to the caller allocated storage
 *                  where this function should store the associated lane.
 *                  
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the physical port is not tied to an
 *                  EPL lane tupple.
 *
 *****************************************************************************/
fm_status fm10000MapPhysicalPortToEplLane(fm_int  sw, 
                                          fm_int  physPort, 
                                          fm_int *epl,
                                          fm_int *lane)
{
    fm_status          err = FM_OK;
    fm10000_switch *   switchExt;
    fm10000_schedInfo *sInfo;
    fm_int             fabricPort;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, physPort = %d\n", sw, physPort);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    /* Sanity check */
    if (physPort < 0 || physPort >= FM10000_NUM_PORTS)
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    fabricPort = sInfo->physicalToFabricMap[physPort]; 

    if ( (fabricPort < 0) ||
         (fabricPort > FM10000_LAST_EPL_FABRIC_PORT) )
    {
        err = FM_ERR_INVALID_PORT;

        /* silently ABORT (port is not in the map) */
        goto ABORT;
    }

    *epl = fabricPort / 4;
    *lane = fabricPort % 4;

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapPhysicalPortToEplLane */




/*****************************************************************************/
/** fm10000MapFabricPortToPhysicalPort
 * \ingroup intSwitch
 *
 * \desc            Maps a fabric port to a physical port.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       fabricPort is the fabric port to convert.
 * 
 * \param[out]      physPort is a pointer to the caller allocated storage
 *                  where this function should store the associated physical port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if fabric port is not a valid value.
 * \return          FM_ERR_INVALID_PORT if the fabric port is not tied to a
 *                  physical port.
 *
 *****************************************************************************/
fm_status fm10000MapFabricPortToPhysicalPort(fm_int  sw, 
                                             fm_int  fabricPort, 
                                             fm_int *physPort)
{
    fm_status          err = FM_OK;
    fm10000_switch *   switchExt;
    fm10000_schedInfo *sInfo;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, fabricPort = %d\n", sw, fabricPort);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    /* Sanity check */
    if ( (fabricPort < 0) || 
         (fabricPort >= FM10000_NUM_FABRIC_PORTS) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    *physPort = sInfo->fabricToPhysicalMap[fabricPort];

    if (*physPort == -1)
    {
        err = FM_ERR_INVALID_PORT;
        
        /* silently ABORT (port is not in the map) */
        goto ABORT;
    }

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapFabricPortToPhysicalPort */




/*****************************************************************************/
/** fm10000MapEplLaneToPhysicalPort
 * \ingroup intSwitch
 *
 * \desc            Maps an EPL/Lane tupple to a physical port.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       epl is the epl to convert.
 * 
 * \param[in]       lane is the lane to convert.
 * 
 * \param[out]      physPort is a pointer to the caller allocated storage
 *                  where this function should store the associated physical port.
 *                  
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the EPL/Lane tupple is not
 *                  valid.
 * \return          FM_ERR_INVALID_PORT if the EPL/Lane tupple is not 
 *                  tied to a physical port.
 *
 *****************************************************************************/
fm_status fm10000MapEplLaneToPhysicalPort(fm_int  sw, 
                                          fm_int  epl,
                                          fm_int  lane,
                                          fm_int *physPort)
{
    fm_status          err = FM_OK;
    fm10000_switch *   switchExt;
    fm10000_schedInfo *sInfo;
    fm_int             fabricPort;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, epl = %d, lane = %d\n", sw, epl, lane);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    /* Sanity check */
    if ( (epl < 0) || 
         (epl > FM10000_MAX_EPL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if ( (lane < 0) || 
         (lane >= FM10000_PORTS_PER_EPL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    fabricPort = (epl * FM10000_PORTS_PER_EPL) + lane;

    *physPort = sInfo->fabricToPhysicalMap[fabricPort];

    if (*physPort == -1)
    {
        err = FM_ERR_INVALID_PORT;
        
        /* silently ABORT (port is not in the map) */
        goto ABORT;
    }

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapEplLaneToPhysicalPort */




/*****************************************************************************/
/** fm10000MapLogicalPortToFabricPort
 * \ingroup intSwitch
 *
 * \desc            Maps a logical port to a fabric port.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       logPort is the logical port to convert.
 * 
 * \param[out]      fabricPort is a pointer to the caller allocated storage
 *                  where this function should store the associated fabric port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if logPort has an invalid value.
 * \return          FM_ERR_INVALID_PORT if the logical port is not tied to a
 *                  fabric port.
 *
 *****************************************************************************/
fm_status fm10000MapLogicalPortToFabricPort(fm_int  sw, 
                                            fm_int  logPort, 
                                            fm_int *fabricPort)
{
    fm_status          err = FM_OK;
    fm_int             physSwitch;
    fm_int             physPort;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, logPort = %d\n", sw, logPort);

    err = fmPlatformMapLogicalPortToPhysical(sw, 
                                             logPort, 
                                             &physSwitch, 
                                             &physPort);
    
    if (err == FM_OK)
    {
        err = fm10000MapPhysicalPortToFabricPort(sw, physPort, fabricPort);
    }
    
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapLogicalPortToFabricPort */




/*****************************************************************************/
/** fm10000MapLogicalPortToEplLane
 * \ingroup intSwitch
 *
 * \desc            Maps a logical port to an EPL/Lane tupple.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       logPort is the logical port to convert.
 * 
 * \param[out]      epl is a pointer to the caller allocated storage
 *                  where this function should store the associated EPL.
 * 
 * \param[out]      lane is a pointer to the caller allocated storage
 *                  where this function should store the associated lane.
 *                  
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if logPort has an invalid value.
 * \return          FM_ERR_INVALID_PORT if the logical port is not tied to an
 *                  EPL lane tupple.
 *
 *****************************************************************************/
fm_status fm10000MapLogicalPortToEplLane(fm_int  sw, 
                                         fm_int  logPort, 
                                         fm_int *epl,
                                         fm_int *lane)
{
    fm_status          err = FM_OK;
    fm_int             physSwitch;
    fm_int             physPort;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, logPort = %d\n", sw, logPort);

    err = fmPlatformMapLogicalPortToPhysical(sw, 
                                             logPort, 
                                             &physSwitch, 
                                             &physPort);
    if (err == FM_OK)
    {
        err = fm10000MapPhysicalPortToEplLane(sw, physPort, epl, lane);
    }
    
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapLogicalPortToEplLane */




/*****************************************************************************/
/** fm10000MapFabricPortToLogicalPort
 * \ingroup intSwitch
 *
 * \desc            Maps a fabric port to a logical port.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       fabricPort is the fabric port to convert.
 * 
 * \param[out]      logPort is a pointer to the caller allocated storage
 *                  where this function should store the associated logical port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if fabric port is not a valid value.
 * \return          FM_ERR_INVALID_PORT if the fabric port is not tied to a
 *                  logical port.
 *
 *****************************************************************************/
fm_status fm10000MapFabricPortToLogicalPort(fm_int  sw, 
                                            fm_int  fabricPort, 
                                            fm_int *logPort)
{
    fm_status          err = FM_OK;
    fm_int             physPort;
    fm_int             logSwitch;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, fabricPort = %d\n", sw, fabricPort);
    
    err = fm10000MapFabricPortToPhysicalPort(sw, fabricPort, &physPort);
    
    if (err == FM_OK)
    {
        err = fmPlatformMapPhysicalPortToLogical(sw, 
                                                 physPort, 
                                                 &logSwitch, 
                                                 logPort);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapFabricPortToLogicalPort */




/*****************************************************************************/
/** fm10000MapEplLaneToLogicalPort
 * \ingroup intSwitch
 *
 * \desc            Maps an EPL/Lane tupple to a logical port.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       epl is the epl to convert.
 * 
 * \param[in]       lane is the lane to convert.
 * 
 * \param[out]      logPort is a pointer to the caller allocated storage
 *                  where this function should store the associated logical port.
 *                  
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the EPL/Lane tupple is not
 *                  valid.
 * \return          FM_ERR_INVALID_PORT if the EPL/Lane tupple is not 
 *                  tied to a logical port.
 *
 *****************************************************************************/
fm_status fm10000MapEplLaneToLogicalPort(fm_int  sw, 
                                         fm_int  epl,
                                         fm_int  lane,
                                         fm_int *logPort)
{
    fm_status          err = FM_OK;
    fm_int             physPort;
    fm_int             logSwitch;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw = %d, epl = %d, lane = %d\n", sw, epl, lane);

    err = fm10000MapEplLaneToPhysicalPort(sw, epl, lane, &physPort);
    
    if (err == FM_OK)
    {
        err = fmPlatformMapPhysicalPortToLogical(sw,
                                             physPort, 
                                             &logSwitch, 
                                             logPort);
    }
    
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000MapLogicalPortToEplLane */




/*****************************************************************************/
/** fm10000UpdateSchedPort
 * \ingroup lowlevSched10k
 *
 * \desc            Updates a scheduler port in the scheduler list. This
 *                  function should be used to update the speed or quad mode
 *                  of the port. The update is only applied after a call
 *                  to fm10000ApplySchedPortUpdates(). A speed of 0 should
 *                  be used if the port is disabled. This allows more bandwidth
 *                  to other ports. 
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port.
 * 
 * \param[in]       speed is the new speed of physical port.
 * 
 * \param[in]       mode is the quad channel mode of physical port.
 * 
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_NOT_FOUND if the physical port is not found.
 * \return          FM_ERR_INVALID_ARGUMENT if the speed, or quad parameters
 *                  are out of range.
 * \return          FM_ERR_SCHED_VIOLATION if there is not enough BW to
 *                  update this port's speed. 
 *
 *****************************************************************************/
fm_status fm10000UpdateSchedPort(fm_int               sw, 
                                 fm_int               physPort,
                                 fm_int               speed,
                                 fm_schedulerPortMode mode)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_schedInfo  *    sInfo;
    fm_int                  i;
    fm_int                  nbRequiredSlots;
    fm_int                  nbFreeSlots;
    fm_int                  nbOwnedSlots;
    fm_int                  qpc;
    fm_int                  port;
    fm_int                  mappedSw;
    fm_int                  fabricPort;
    fm_int                  nonQuadfabricPort;
    fm_int                  lane;
    fm_treeIterator         it;
    fm_uint64               treeKey;
    fm10000_schedEntryInfo *treeValue;
    fm_int                  physPortTmp;
    fm_uint64               logCat;
    fm_uint64               logLvl;
    fm_bool                 portAttrLockTaken;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw=%d, physPort=%d, speed=%d, mode=%d\n", 
                 sw, physPort, speed, mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    portAttrLockTaken = FALSE;

    /* Take port attribute lock since fm10000DrainPhysPort might be 
     * updating portExt and release lock at the end of this function
     * to be on a safer side. */
    FM_FLAG_TAKE_PORT_ATTR_LOCK(sw);

    TAKE_SCHEDULER_LOCK(sw);

    /* Work on a copy of the active scheduler info structure */
    FM_MEMCPY_S(&sInfo->tmp, 
                sizeof(sInfo->tmp),
                &sInfo->active, 
                sizeof(sInfo->active) );

    if ( (speed < 0) || 
         (speed > FM10000_SCHED_SPEED_100G) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Validate quad mode */
    if ((mode == FM_SCHED_PORT_MODE_NONE) && (speed != 0))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (mode > FM_SCHED_PORT_MODE_QUAD)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Validate / Round up the speed to the closest upper speed */
    if (speed < 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else if (speed == 0)
    {
        nbRequiredSlots = 0;
    }
    else if (speed <= 2500)
    {
        speed = 2500;
        nbRequiredSlots = SLOTS_PER_2500M;
    }
    else if (speed <= 10000)
    {
        speed = 10000;
        nbRequiredSlots = SLOTS_PER_10G;
    }
    else if (speed <= 25000)
    {
        speed = 25000;
        nbRequiredSlots = SLOTS_PER_25G;
    }
    else if (speed <= 40000)
    {
        speed = 40000;
        nbRequiredSlots = SLOTS_PER_40G;
    }
    else if (speed == 50000)
    {
        /* Special case for PCIE, See bugzilla #25673 comment #12 */
        speed = 40000;
        nbRequiredSlots = SLOTS_PER_40G;
    }
    else if (speed <= 60000)
    {
        speed = 60000;
        nbRequiredSlots = SLOTS_PER_60G;
    }
    else if (speed <= 100000)
    {
        speed = 100000;
        nbRequiredSlots = SLOTS_PER_100G;
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Convert the physPort into fabricPort */
    err = fm10000MapPhysicalPortToFabricPort(sw, physPort, &fabricPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    nonQuadfabricPort = fabricPort;
    qpc               = fabricPort / 4;
    lane              = fabricPort % 4;
    physPortTmp       = AUTO_APP;

    if (mode == FM_SCHED_PORT_MODE_QUAD)
    {
        /* Root fabric port must be used in quad mode */
        fabricPort  = qpc * 4;
        physPortTmp = physPort;
    }

    nbFreeSlots = 0;
    nbOwnedSlots = 0;

    /*********************************************** 
     * Verify if we have a sufficient number of 
     * slots available.
     ***********************************************/ 
    if (speed != 0)
    {
        for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
        {
            if ( (mode == FM_SCHED_PORT_MODE_SINGLE) &&
                 (treeValue->lane == lane) )
            {
                if ( (treeValue->afp == fabricPort) ||
                     (treeValue->app == physPort) )
                {
                    nbOwnedSlots++;
                }
                else if (treeValue->afp == FREE_ENTRY)
                {
                    nbFreeSlots++;
                }
            }
            else if ( mode == FM_SCHED_PORT_MODE_QUAD )
            {
                if ( (treeValue->afp == nonQuadfabricPort) ||
                     (treeValue->app == physPort) )
                {
                    nbOwnedSlots++;
                }
                else if (treeValue->afp == FREE_ENTRY)
                {
                    nbFreeSlots++;
                }
            }
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }

        /* Do we have enough available slots? */
        if ( (nbFreeSlots + nbOwnedSlots) < nbRequiredSlots)
        {
            /* This will typically happen if a port does not have the right
             * port speed capability. The port speed capability is used at
             * init to determine the total BW to allocate. */

            if (GET_PROPERTY()->ignoreBwViolation >= 2)
            {
                err = FM_OK;
                goto ABORT;
            }
            
            err = fmPlatformMapPhysicalPortToLogical(sw, physPort, &mappedSw, &port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                         "For port %d, %d slots (%.1fG) are needed, only got %d slots (%.1fG).\n",
                         port,
                         nbRequiredSlots,
                         (nbRequiredSlots * (fm_float)(SLOT_SPEED)),
                         nbFreeSlots + nbOwnedSlots,
                         ((nbFreeSlots + nbOwnedSlots) * (fm_float)(SLOT_SPEED)));

            if (GET_PROPERTY()->ignoreBwViolation >= 1)
            {
                FM_LOG_WARNING(FM_LOG_CAT_SWITCH, 
                               "Ignoring above violation. Sending/Receiving traffic on port %d "
                               "will cause undefined behavior.\n",
                               port);
                err = FM_OK;
                goto ABORT;
            }
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                         "The port needs to be provisioned with "
                         "more BW to support this speed.\n");
#ifdef FM_AAK_API_PLATFORM_HW_PORT_SPEED
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                         "In LT Config File, see \"" FM_AAK_API_PLATFORM_HW_PORT_SPEED"\" \n", 
                         sw, 
                         physPort);
#endif
            err = FM_ERR_SCHED_VIOLATION;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

    /*********************************************** 
     * We have enough slots available. Let's update 
     * our structures. 
     ***********************************************/
    if (speed == 0)
    {
        /* Drain non-Ethernet ports. Ethernet ports 
         * are drained as part of Port State Machine. */
        if ( !( fabricPort >= FM10000_FIRST_EPL_FABRIC_PORT &&
                fabricPort <= FM10000_LAST_EPL_FABRIC_PORT ) ) 
        {
            err = fm10000DrainPhysPort(sw, physPort, 1, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
        /* Just update the QPC Tree Entries, marking them as free */
        for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
        {
            if ( (treeValue->afp == fabricPort) ||
                 (treeValue->app == physPort) )
            {
                treeValue->afp = FREE_ENTRY;
                treeValue->app = AUTO_APP;
                treeValue->quad = 0;
            }
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
    }
    else if (nbOwnedSlots > nbRequiredSlots)
    {
        /* Only free extra entries if going from a Quad Mode to Single Mode.
         *
         * We need to do this such that other lanes can get some bandwidth.
         */
        for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
        {
            if ( (treeValue->quad == 1) &&
                 (mode == FM_SCHED_PORT_MODE_SINGLE) )
            {
                treeValue->quad = 0;

                if (treeValue->lane != lane)
                {
                    treeValue->afp = FREE_ENTRY;
                    treeValue->app = AUTO_APP;
                }
                else
                {
                    treeValue->afp = fabricPort;
                    treeValue->app = AUTO_APP; 
                }
            }
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
    }
    else if (nbOwnedSlots == nbRequiredSlots)
    {
        /* Update the Quad Mode. We could be going from 40G to 4x10G or vice
         * versa */
        for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
        {
            /* if going from quad mode to single mode, free up other lanes */
            if ( (treeValue->quad == 1) &&
                 (mode == FM_SCHED_PORT_MODE_SINGLE) )
            {
                if (treeValue->lane != lane)
                {
                    treeValue->afp = FREE_ENTRY;
                    treeValue->app = AUTO_APP;
                }
                else
                {
                    treeValue->afp = fabricPort;
                    treeValue->app = AUTO_APP; 
                }
            }

            treeValue->quad = (mode == FM_SCHED_PORT_MODE_QUAD);
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
    }
    else if (nbOwnedSlots < nbRequiredSlots)
    {
        /* Let's take ownership of matching lanes. Also take ownership of
         * non-matching lanes if we are going into quad mode and the slots 
         * are free */
        for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
             (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
        {
            if (treeValue->lane == lane)
            {
                /* matching lanes */
                treeValue->quad     = (mode == FM_SCHED_PORT_MODE_QUAD);
                treeValue->afp      = fabricPort;
                treeValue->app      = physPortTmp;
            } 
            else
            {
                /* Non-matching lanes */
                if ( (mode == FM_SCHED_PORT_MODE_QUAD) &&
                     (treeValue->afp == FREE_ENTRY) )
                {
                    treeValue->quad     = (mode == FM_SCHED_PORT_MODE_QUAD);
                    treeValue->afp      = fabricPort;
                    treeValue->app      = physPortTmp;
                }
            }
        }

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
    }

    sInfo->tmp.isQuad[physPort] = (mode == FM_SCHED_PORT_MODE_QUAD);

    /*********************************************** 
     * Update the schedule
     ***********************************************/

    for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        if ( (treeValue->afp != FREE_ENTRY) )
        {
            sInfo->tmp.schedList[treeKey].fabricPort = treeValue->afp; 

            /* We must configure the proper port, not just fabric port */
            if (treeValue->app == AUTO_APP)
            {
                fm10000MapFabricPortToPhysicalPort(sw, 
                                                   treeValue->afp, 
                                                   &physPortTmp);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
            else
            {
                physPortTmp = treeValue->app;
            }

            sInfo->tmp.schedList[treeKey].port = physPortTmp; 
            sInfo->tmp.schedList[treeKey].idle = FALSE; 
        }
        else
        {
            sInfo->tmp.schedList[treeKey].idle = TRUE; 
        }

        sInfo->tmp.schedList[treeKey].quad = treeValue->quad; 
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    fmGetLoggingAttribute(FM_LOG_ATTR_CATEGORY_MASK, 
                          0, 
                          (void *) &logCat);
    fmGetLoggingAttribute(FM_LOG_ATTR_LEVEL_MASK, 
                          0, 
                          (void *) &logLvl);

    if ( (logCat & FM_LOG_CAT_SWITCH) &&
         (logLvl & FM_LOG_LEVEL_DEBUG) )
    {
        err = DbgDumpQPCUsage(sw, qpc, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Update HW */
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        err = ValidateQuad4Constraint(sw, sInfo->tmp.schedList[i].port, i);

        if (err != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                         "Validation failed for port %d (at slot %d)\n",
                         sInfo->tmp.schedList[i].port,
                         i);

            DbgDumpSchedulerConfig(sw, 0, TRUE);
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    err = fm10000SetSchedRing(sw, 
                              FM10000_SCHED_RING_ALL, 
                              sInfo->tmp.schedList,
                              sInfo->tmp.schedLen);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (sInfo->attr.mode == FM10000_SCHED_MODE_DYNAMIC)
    {
        sInfo->reservedSpeed[physPort]    = speed;
        sInfo->preReservedSpeed[physPort] = speed;
    }

    /* If previously no bandwidth was allocated, but now 
     * bandwidth is allocated then enable this port in the
     * portMask of all ports. */
    if (speed > 0 && nbOwnedSlots == 0)
    {
        /* Disable drain on non-Ethernet ports and Enable portMask.
         * fm10000DrainPhysPort enables port mask of all ports
         * to include this port. */
        if ( !( fabricPort >= FM10000_FIRST_EPL_FABRIC_PORT &&
                fabricPort <= FM10000_LAST_EPL_FABRIC_PORT ) ) 
        {
            err = fm10000DrainPhysPort(sw, physPort, 1, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

    /* We have succeeded, store the scheduler state into the active
     * structure */
    FM_MEMCPY_S(&sInfo->active, 
                sizeof(sInfo->active), 
                &sInfo->tmp, 
                sizeof(sInfo->tmp) );

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    if (portAttrLockTaken)
    {
        FM_DROP_PORT_ATTR_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000UpdateSchedPort */




/*****************************************************************************/
/** fm10000SwapSchedQpcBw
 * \ingroup lowlevSched10k
 *
 * \desc            Swap all the bandwidth from a quad port channel (QPC)
 *                  to another QPC.
 *
 *                  All traffic to/from the old/new QPC will be stopped during
 *                  the transition.
 *
 *                  The list of QPC is available in RRC
 *                  EAS under the 'Quad Port Channel Mapping' section.
 *
 *                  As an example, this function can be used to swap the
 *                  BW of 4x10G of EPL 0 with EPL 1, allowing EPL 1 to run
 *                  as 4x10G or 1x40G. It is also possible to swap the BW
 *                  from an EPL QPC to a PCIE QPC and vice versa.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       oldQpc is the old QPC.
 *
 * \param[in]       newQpc is the new QPC.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if the QPC is out of range.
 * \return          FM_ERR_PORT_IN_USE if either old or new QPC port is
 *                  already in use.
 *
 *****************************************************************************/
fm_status fm10000SwapSchedQpcBw(fm_int sw,
                                fm_int oldQpc,
                                fm_int newQpc)
{
    fm_status               err = FM_OK;
    fm_status               err2 = FM_OK;
    fm10000_switch *        switchExt;
    fm10000_schedInfo  *    sInfo;
    fm_tree *               oldTree;
    fm_tree *               newTree;
    fm_treeIterator         it;
    fm_uint64               treeKey;
    fm10000_schedEntryInfo *treeValue;
    fm_int                  nbSlots;
    fm_int                  nbFreeSlots;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw=%d, oldQpc=%d, newQpc=%d\n", 
                 sw, oldQpc, newQpc);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    if ( (oldQpc < 0) || (oldQpc >= NUM_QPC) ||
         (newQpc < 0) || (newQpc >= NUM_QPC) ||
         (oldQpc == newQpc) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    oldTree = &sInfo->qpcState[oldQpc];
    newTree = &sInfo->qpcState[newQpc];

    /*********************************************
     * 1. Make sure the new QPC is not in use
     *********************************************/

    if (fmTreeSize(newTree) != 0)
    {
        err = FM_ERR_PORT_IN_USE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
    /*********************************************
     * 2. Make sure the old QPC is not in use
     *********************************************/

    /* Scan the oldTree and make sure all entries are free */
    nbSlots     = 0;
    nbFreeSlots = 0;
    for (fmTreeIterInit(&it, oldTree);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        nbSlots++;
        if (treeValue->afp == FREE_ENTRY)
        {
            nbFreeSlots++;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Not all slots are free in the old QPC */
    if (nbSlots != nbFreeSlots)
    {
        err = FM_ERR_PORT_IN_USE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /*********************************************
     * 3. Move the old QPC slots to the new QPC
     *********************************************/

    /* Move entries */
    for (fmTreeIterInit(&it, oldTree);
         (err2 = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        err = fmTreeInsert(newTree, treeKey, (void *)treeValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Clean up old tree (by iterating in newTree for the same entries)*/
    for (fmTreeIterInit(&it, newTree);
         (err2 = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        err = fmTreeRemove(oldTree, treeKey, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
ABORT:
    DROP_SCHEDULER_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SwapSchedQpcBw */




/*****************************************************************************/
/** fm10000GetSchedRing
 * \ingroup intSwitch
 *
 * \desc            Get the active scheduler ring.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       ring is a ring to retrieve, ''FM10000_SCHED_RING_RX'' or
 *                  ''FM10000_SCHED_RING_TX''.
 * 
 * \param[in]       stMax is the maximum number of entries that this function
 *                  should store in stList. Should be of length 512 to cover
 *                  all cases. 
 * 
 * \param[out]      stList is a pointer to the caller allocated array storage
 *                  where this function should store the ring's tokens.
 * 
 * \param[out]      stCount is a pointer to the caller allocated storage
 *                  where the number of tokens should be stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the ring is not valid.
 *
 *****************************************************************************/
fm_status fm10000GetSchedRing(fm_int             sw, 
                              fm_uint32          ring,
                              fm_int             stMax,
                              fm_schedulerToken *stList,
                              fm_int            *stCount)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  rv;
    fm_int     base;
    fm_int     i;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, ring = 0x%1x, stMax = %d\n", 
                 sw, ring, stMax);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_SCHEDULER_LOCK(sw);

    if (stList == NULL || stCount == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_SCHEDULE_CTRL(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (ring == FM10000_SCHED_RING_RX)
    {
        base     = FM_GET_BIT(rv, FM10000_SCHED_SCHEDULE_CTRL, RxPage) ? 
                        LIST1_BASE : LIST0_BASE;
        *stCount = FM_GET_FIELD(rv, FM10000_SCHED_SCHEDULE_CTRL, RxMaxIndex) + 1;
    }
    else if (ring == FM10000_SCHED_RING_TX)
    {
        base     = FM_GET_BIT(rv, FM10000_SCHED_SCHEDULE_CTRL, TxPage) ? 
                        LIST1_BASE : LIST0_BASE;
        *stCount = FM_GET_FIELD(rv, FM10000_SCHED_SCHEDULE_CTRL, TxMaxIndex) + 1;
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (stMax < *stCount)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (ring == FM10000_SCHED_RING_RX)
    {
        for (i = 0; i < *stCount; i++)
        {
            err = switchPtr->ReadUINT32(sw, FM10000_SCHED_RX_SCHEDULE(base+i), &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            stList[i].fabricPort = FM_GET_FIELD(rv, 
                                                FM10000_SCHED_RX_SCHEDULE, 
                                                PhysPort);

            stList[i].port = FM_GET_FIELD(rv, 
                                          FM10000_SCHED_RX_SCHEDULE, 
                                          Port);

            stList[i].quad = FM_GET_BIT(rv, 
                                        FM10000_SCHED_RX_SCHEDULE, 
                                        Quad);

            stList[i].idle = FM_GET_BIT(rv, 
                                        FM10000_SCHED_RX_SCHEDULE, 
                                        Idle);
        }
    }
    else if (ring == FM10000_SCHED_RING_TX)
    {
        for (i = 0; i < *stCount; i++)
        {
            err = switchPtr->ReadUINT32(sw, FM10000_SCHED_TX_SCHEDULE(base+i), &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            stList[i].fabricPort = FM_GET_FIELD(rv, 
                                                FM10000_SCHED_TX_SCHEDULE, 
                                                PhysPort);

            stList[i].port = FM_GET_FIELD(rv, 
                                          FM10000_SCHED_TX_SCHEDULE, 
                                          Port);

            stList[i].quad = FM_GET_BIT(rv, 
                                        FM10000_SCHED_TX_SCHEDULE, 
                                        Quad);

            stList[i].idle = FM_GET_BIT(rv, 
                                        FM10000_SCHED_TX_SCHEDULE, 
                                        Idle);
        }
    }
    
ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GetSchedRing */




/*****************************************************************************/
/** fm10000SetSchedRing
 * \ingroup intSwitch
 *
 * \desc            Replace the active scheduler ring by a new one.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       mask is a bit mask containing the rings to update.
 *                  FM10000_SCHED_RING_RX                               \lb
 *                  FM10000_SCHED_RING_TX                               \lb
 *                  FM10000_SCHED_RING_ALL                              \lb 
 * 
 * \param[in]       stList is a pointer to the array containing the list of
 *                  tokens of the new scheduler ring. Index 0 must be the
 *                  implicit idle cycle. 
 *                  
 * \param[in]       stCount is the number of tokens to retrieve from stList.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the ring is not valid.
 *
 *****************************************************************************/
fm_status fm10000SetSchedRing(fm_int             sw, 
                              fm_uint32          mask,
                              fm_schedulerToken *stList,
                              fm_int             stCount)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_int              index;
    fm_uint32           rv;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, mask = 0x%1x, stCount = %d\n", 
                 sw, mask, stCount);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    if (mask & FM10000_SCHED_RING_RX)
    {
        index = LIST0_BASE;

        if (sInfo->activeListRx == 0)
        {
            index = LIST1_BASE;
        }

        /* start from index 1, as the index 0 is the implicit idle */
        for (i = 1; i < stCount; i++)
        {
            rv = 0;
            FM_SET_FIELD(rv, FM10000_SCHED_RX_SCHEDULE, PhysPort, stList[i].fabricPort);
            FM_SET_FIELD(rv, FM10000_SCHED_RX_SCHEDULE, Port, stList[i].port);
            FM_SET_BIT(rv, FM10000_SCHED_RX_SCHEDULE, Quad, stList[i].quad);
            FM_SET_BIT(rv, FM10000_SCHED_RX_SCHEDULE, Color, FALSE);
            FM_SET_BIT(rv, FM10000_SCHED_RX_SCHEDULE, Idle, stList[i].idle);

            err = switchPtr->WriteUINT32(sw, FM10000_SCHED_RX_SCHEDULE(index++), rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }

        /* Activate scheduler */
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "Swapping RX active scheduler ring from %d to %d\n", 
                     sInfo->activeListRx,
                     !sInfo->activeListRx);

        sInfo->activeListRx = !sInfo->activeListRx;
    }

    if (mask & FM10000_SCHED_RING_TX)
    {
        index = LIST0_BASE;

        if (sInfo->activeListTx == 0)
        {
            index = LIST1_BASE;
        }

         /* start from index 1, as the index 0 is the implicit idle */
        for (i = 1; i < stCount; i++)
        {
            rv = 0;
            FM_SET_FIELD(rv, FM10000_SCHED_TX_SCHEDULE, PhysPort, stList[i].fabricPort);
            FM_SET_FIELD(rv, FM10000_SCHED_TX_SCHEDULE, Port, stList[i].port);
            FM_SET_BIT(rv, FM10000_SCHED_TX_SCHEDULE, Quad, stList[i].quad);
            FM_SET_BIT(rv, FM10000_SCHED_TX_SCHEDULE, Color, FALSE);
            FM_SET_BIT(rv, FM10000_SCHED_TX_SCHEDULE, Idle, stList[i].idle);

            err = switchPtr->WriteUINT32(sw, FM10000_SCHED_TX_SCHEDULE(index++), rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }

        /* Activate scheduler */
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "Swapping TX active scheduler ring from %d to %d\n", 
                     sInfo->activeListTx,
                     !sInfo->activeListTx);

        sInfo->activeListTx = !sInfo->activeListTx;
    }

    /* Atomic update of the rx/tx rings */
    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_SCHEDULE_CTRL(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (mask & FM10000_SCHED_RING_RX)
    {
        FM_SET_BIT(rv,   FM10000_SCHED_SCHEDULE_CTRL, RxEnable, TRUE);
        FM_SET_BIT(rv,   FM10000_SCHED_SCHEDULE_CTRL, RxPage, sInfo->activeListRx);
        FM_SET_FIELD(rv, FM10000_SCHED_SCHEDULE_CTRL, RxMaxIndex, (stCount-2)); 
    }

    if (mask & FM10000_SCHED_RING_TX)
    {
        FM_SET_BIT(rv,   FM10000_SCHED_SCHEDULE_CTRL, TxEnable, TRUE);
        FM_SET_BIT(rv,   FM10000_SCHED_SCHEDULE_CTRL, TxPage, sInfo->activeListTx);
        FM_SET_FIELD(rv, FM10000_SCHED_SCHEDULE_CTRL, TxMaxIndex, (stCount-2) );
    }
    
    err = switchPtr->WriteUINT32(sw, FM10000_SCHED_SCHEDULE_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    DROP_REG_LOCK(sw);

ABORT:
    DROP_SCHEDULER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SetSchedRing */




/*****************************************************************************/
/** fm10000DbgDumpSchedulerConfig
 * \ingroup intSwitch
 *
 * \desc            Dumps the scheduler token lists (both active innactive).
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpSchedulerConfig(fm_int sw)
{
    fm_status           err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    TAKE_SCHEDULER_LOCK(sw);

    err = DbgDumpSchedulerConfig(sw, ACTIVE_SCHEDULE, TRUE);

    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000DbgDumpSchedulerConfig */




/*****************************************************************************/
/** fm10000GenerateSchedule
 * \ingroup intSwitch
 *
 * \desc            Unit testing function to generate rings. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       portList is a pointer to the caller allocated storage where
 *                  the portList is stored.
 * 
 * \param[in]       nbPorts is number of entries stored in portList.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GenerateSchedule(fm_int sw, 
                                  fm_schedulerPort *portList, 
                                  fm_int nbPorts)
{
    fm_status          err = FM_OK;
    fm10000_switch *   switchExt;
    fm10000_schedInfo *sInfo;
    fm_int             i;

    fm_timestamp       tStart = {0,0};
    fm_timestamp       tGen   = {0,0};
    fm_timestamp       tDiff  = {0,0};
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n, nbPorts = %d", sw, nbPorts);

    fmGetTime(&tStart);

    TAKE_SCHEDULER_LOCK(sw);
    
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_SWITCH, 
                           nbPorts <= FM10000_SCHED_MAX_NUM_PORTS, 
                           err = FM_FAIL,
                           "Number of ports exceeded (%d > %d)\n",
                           nbPorts,
                           FM10000_SCHED_MAX_NUM_PORTS);

    FM_CLEAR(sInfo->tmp);

    /* Copy the portlist locally, so that the API can update it
     * as needed */
    for (i = 0; i < nbPorts; i++)
    {
        sInfo->tmp.portList[i] = portList[i];
    }

    sInfo->tmp.nbPorts = nbPorts;

    err = GenerateSchedule(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    fmGetTime(&tGen);

    fmSubTimestamps(&tGen, &tStart, &tDiff);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                 "Generation Length = %lld us\n", 
                 tDiff.usec + tDiff.sec * 1000000);
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GenerateSchedule */




/*****************************************************************************/
/** fm10000GetSchedPortSpeed
 * \ingroup intSwitch
 *
 * \desc            Return scheduler port speed of a physical 
 *                  port as per the active scheduler configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port.
 * 
 * \param[out]      speed a pointer to the caller allocated storage where
 *                  information regarding this port's scheduler BW should be
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the physical port is not found.
 *
 *****************************************************************************/
fm_status fm10000GetSchedPortSpeed(fm_int  sw, 
                                   fm_int  physPort,
                                   fm10000_schedSpeedInfo *speed)
{
    fm_status               err = FM_OK;
    fm10000_switch *        switchExt;
    fm10000_schedInfo  *    sInfo;
    fm_int                  fabricPort;
    fm_int                  qpc;
    fm_int                  lane;
    fm_int                  nbSlots;
    fm_int                  nbSlotsFreeForLane;
    fm_int                  nbSlotsUsedForLane;
    fm_int                  nbSlotsFree;
    fm_treeIterator         it;
    fm_uint64               treeKey;
    fm10000_schedEntryInfo *treeValue;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, 
                         "sw=%d, physPort=%d\n", 
                         sw, physPort);

    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    TAKE_SCHEDULER_LOCK(sw);

    /* Find the port belongs to which QPC */
    err = fm10000MapPhysicalPortToFabricPort(sw, physPort, &fabricPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    qpc = fabricPort / 4;

    FM_CLEAR(*speed);

    nbSlots = 0;
    
    /* Iterate in the QPCs slots and find those that match */
    for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        if (treeValue->afp == fabricPort)
        {
            nbSlots++;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    speed->assignedSpeed = nbSlots * SLOT_SPEED_MBPS;

    nbSlots = 0;
    nbSlotsFreeForLane = 0;
    nbSlotsUsedForLane = 0;
    nbSlotsFree = 0;

    lane = fabricPort % 4;

    /* Iterate in the QPCs slots and find those that match */
    for (fmTreeIterInit(&it, &sInfo->qpcState[qpc]);
         (err = fmTreeIterNext(&it, &treeKey, (void **) &treeValue)) == FM_OK ;)
    {
        nbSlots++;

        if (treeValue->lane == lane)
        {
            if (treeValue->afp == FREE_ENTRY)
            {
                nbSlotsFreeForLane++;
            }
            else
            {
                nbSlotsUsedForLane++;
            }
        }

        if (treeValue->afp == FREE_ENTRY)
        {
            nbSlotsFree++;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    speed->singleLaneSpeed = nbSlotsFreeForLane * SLOT_SPEED_MBPS;

    /* EPL Ports can be single or multilane. */
    if ( (fabricPort >= FM10000_FIRST_EPL_FABRIC_PORT) &&
         (fabricPort <= FM10000_LAST_EPL_FABRIC_PORT) &&
         ( (nbSlotsFree == nbSlots) ||
           (nbSlotsFree == (nbSlots - nbSlotsUsedForLane) ) ) )
    {
        /* MultiLane speed is available */
        speed->multiLaneSpeed =  nbSlots * SLOT_SPEED_MBPS; 
    }
    else
    {
        speed->multiLaneSpeed = 0;
    }

    speed->reservedSpeed    = sInfo->reservedSpeed[physPort];
    speed->preReservedSpeed = sInfo->preReservedSpeed[physPort]; 

    if (sInfo->attr.mode == FM10000_SCHED_MODE_STATIC)
    {
        speed->isQuad = sInfo->active.isQuad[physPort];
    }
    else if (sInfo->attr.mode == FM10000_SCHED_MODE_DYNAMIC)
    {
        speed->isQuad = sInfo->reservedQuad[physPort];
    }
    
ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GetSchedPortSpeed */




/*****************************************************************************/
/** fm10000GetSchedPortSpeedForPep
 * \ingroup intSwitch
 *
 * \desc            Return scheduler port speed of a pep 
 *                  port as per the active scheduler configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       pepId is the pep port ID.
 * 
 * \param[out]      speed of the physical port is returned in this variable.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the logical/physical port is not found.
 *
 *****************************************************************************/
fm_status fm10000GetSchedPortSpeedForPep(fm_int  sw,
                                         fm_int  pepId,
                                         fm_int *speed)
{
    fm_status              err;
    fm_int                 logicalPort;
    fm_int                 physSw;
    fm_int                 physPort;
    fm10000_schedSpeedInfo speedInfo;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, 
                         "sw=%d, pepId=%d\n", 
                         sw, pepId);

    err = FM_OK;

    err = fm10000MapPepToLogicalPort(sw,
                                     pepId,
                                     &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmPlatformMapLogicalPortToPhysical(sw, 
                                             logicalPort,
                                             &physSw, 
                                             &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000GetSchedPortSpeed(sw,
                                   physPort,
                                   &speedInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    *speed = speedInfo.assignedSpeed;

ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GetSchedPortSpeedForPep */




/*****************************************************************************/
/** fm10000PreReserveSchedBw
 * \ingroup intSwitch
 *
 * \desc            PreReserves BW for a given port. Use a speed of 0 to free 
 *                  the BW for a given port. At any given time, the sum of
 *                  the pre-reserved BW must be equal or below the maximum BW
 *                  the system supports. If the BW request would cause the
 *                  reserved BW to go over the maximum BW, this function will
 *                  return an error. 
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port.
 * 
 * \param[in]       speed is the speed to reserve of physical port. A value
 *                  of FM10000_SCHED_SPEED_DEFAULT may be used to default
 *                  the speed to what was assigned at initialization (from LT
 *                  config file). 
 * 
 * \param[in]       mode is the quad channel mode of physical port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if port or speed are out of range.
 * \return          FM_ERR_SCHED_OVERSUBSCRIBED if there is not enough BW
 *                  available and can't reserve the requested BW. 
 *
 *****************************************************************************/
fm_status fm10000PreReserveSchedBw(fm_int               sw,
                                   fm_int               physPort,
                                   fm_int               speed,
                                   fm_schedulerPortMode mode)
{
    fm_status           err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, physPort = %d, speed = %d, mode = %d\n", 
                 sw, physPort, speed, mode);

    TAKE_SCHEDULER_LOCK(sw);

    err = ReserveSchedBw(sw, physPort, speed, mode, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000PreReserveSchedBw */




/*****************************************************************************/
/** fm10000ReserveSchedBw
 * \ingroup intSwitch
 *
 * \desc            Reserves BW for a given port. Use a speed of 0 to free 
 *                  the BW for a given port. At any given time, the sum of
 *                  the reserved BW must be equal or below the maximum BW
 *                  the system supports. If the BW request would cause the
 *                  reserved BW to go over the maximum BW, this function will
 *                  return an error. 
 *                  This should be used by Non-AN73 ports to update both
 *                  pre-reserved and reserved BW.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port.
 * 
 * \param[in]       speed is the speed to reserve of physical port. A value
 *                  of FM10000_SCHED_SPEED_DEFAULT may be used to default
 *                  the speed to what was assigned at initialization (from LT
 *                  config file). 
 * 
 * \param[in]       mode is the quad channel mode of physical port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if port or speed are out of range.
 * \return          FM_ERR_SCHED_OVERSUBSCRIBED if there is not enough BW
 *                  available and can't reserve the requested BW. 
 *
 *****************************************************************************/
fm_status fm10000ReserveSchedBw(fm_int               sw,
                                fm_int               physPort,
                                fm_int               speed,
                                fm_schedulerPortMode mode)
{

    fm_status           err ;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw = %d, physPort = %d, speed = %d, mode = %d\n",
                 sw, physPort, speed, mode);

    TAKE_SCHEDULER_LOCK(sw);

    /* Update pre-reserved speed and validate the change */
    err = ReserveSchedBw(sw, physPort, speed, mode, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Update reserved speed and validate the change */
    err = ReserveSchedBw(sw, physPort, speed, mode, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}    /* end fm10000ReserveSchedBw */



/*****************************************************************************/
/** fm10000ReserveSchedBwForAnPort
 * \ingroup intSwitch
 *
 * \desc            Reserves BW for a given port. Use a speed of 0 to free 
 *                  the BW for a given port. At any given time, the sum of
 *                  the reserved BW must be equal or below the maximum BW
 *                  the system supports. If the BW request would cause the
 *                  reserved BW to go over the maximum BW, this function will
 *                  return an error. 
 *                  This should be used by AN73 ports to update only reserved 
 *                  BW which will be used to generate Scheduler ring.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       physPort is the physical port.
 * 
 * \param[in]       speed is the speed to reserve of physical port. A value
 *                  of FM10000_SCHED_SPEED_DEFAULT may be used to default
 *                  the speed to what was assigned at initialization (from LT
 *                  config file). 
 * 
 * \param[in]       mode is the quad channel mode of physical port.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if port or speed are out of range.
 * \return          FM_ERR_SCHED_OVERSUBSCRIBED if there is not enough BW
 *                  available and can't reserve the requested BW. 
 *
 *****************************************************************************/
fm_status fm10000ReserveSchedBwForAnPort(fm_int               sw,
                                         fm_int               physPort,
                                         fm_int               speed,
                                         fm_schedulerPortMode mode)
{

    fm_status           err ;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw = %d, physPort = %d, speed = %d, mode = %d\n",
                 sw, physPort, speed, mode);

    TAKE_SCHEDULER_LOCK(sw);

    err = ReserveSchedBw(sw, physPort, speed, mode, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000ReserveSchedBwForAnPort */




/*****************************************************************************/
/** fm10000RegenerateSchedule
 * \ingroup intSwitch
 *
 * \desc            Regenerates Rx and Tx scheduler rings
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_SCHED_OVERSUBSCRIBED if the frequency of
 *                  the chip is not high enough, resulting in oversuscription. 
 * \return          FM_ERR_SCHED_VIOLATION if the schedule could not be
 *                  generated because a violation was detected. 
 *
 *****************************************************************************/
fm_status fm10000RegenerateSchedule(fm_int sw)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;
    fm_int              i;
    fm_int              j;
    fm_int              slots100G;
    fm_int              slots60G;
    fm_int              slots40G;
    fm_int              slots25G;
    fm_int              slots10G;
    fm_int              slots2500M;
    fm_int              slotsIdle;
    fm_schedulerToken  *sToken;
    fm_uint64           logCat;
    fm_uint64           logLvl;
    fm_int              physPort;
    fm_int              fabricPort;

    fm_timestamp       tStart = {0,0};
    fm_timestamp       tGen   = {0,0};
    fm_timestamp       tDiff  = {0,0};
    fmGetTime(&tStart);
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw = %d\n", sw);

    TAKE_SCHEDULER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    /* Work on a copy of the active scheduler info structure */
    FM_MEMCPY_S(&sInfo->tmp, 
                sizeof(sInfo->tmp),
                &sInfo->active, 
                sizeof(sInfo->active) );

    /* Initialize Internal Structures */
    err = fmCreateBitArray(&sInfo->tmp.p2500M, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p10G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p25G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p40G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p60G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    err = fmCreateBitArray(&sInfo->tmp.p100G, FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /*********************************************
     * Sort all ports into speed bins 
     *********************************************/

    for (i = 0; i < FM10000_SCHED_NUM_PORTS; i++) 
    {
        if (sInfo->physicalToFabricMap[i] == -1)
        {
            continue;
        }

        fabricPort = sInfo->physicalToFabricMap[i];

        sInfo->tmp.physPortSpeed[i] = sInfo->reservedSpeed[i];
        sInfo->tmp.fabricPortSpeed[fabricPort] = sInfo->tmp.physPortSpeed[i];

        switch (sInfo->tmp.physPortSpeed[i])
        {
            case FM10000_SCHED_SPEED_IDLE:
                break;

            case FM10000_SCHED_SPEED_2500M:
                err = fmSetBitArrayBit(&sInfo->tmp.p2500M, i, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case FM10000_SCHED_SPEED_10G:
                err = fmSetBitArrayBit(&sInfo->tmp.p10G, i, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case FM10000_SCHED_SPEED_25G:
                err = fmSetBitArrayBit(&sInfo->tmp.p25G, i, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case FM10000_SCHED_SPEED_40G:
                err = fmSetBitArrayBit(&sInfo->tmp.p40G, i, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case FM10000_SCHED_SPEED_60G:
                err = fmSetBitArrayBit(&sInfo->tmp.p60G, i, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            case FM10000_SCHED_SPEED_100G:
                err = fmSetBitArrayBit(&sInfo->tmp.p100G, i, 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
                break;

            default:
                err = FM_ERR_SCHED_VIOLATION;
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Invalid Speed for entry: physPort=%d speed=%d\n",
                             i,
                             sInfo->tmp.physPortSpeed[i]);
                goto ABORT;
                break;
        }
    }
        
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 100G Ports", GetNbPorts(&sInfo->tmp.p100G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 60G Ports", GetNbPorts(&sInfo->tmp.p60G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 40G Ports", GetNbPorts(&sInfo->tmp.p40G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 25G Ports", GetNbPorts(&sInfo->tmp.p25G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 10G Ports", GetNbPorts(&sInfo->tmp.p10G) );
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 2.5G Ports", GetNbPorts(&sInfo->tmp.p2500M) );

    /*********************************************
     * Validate that the frequency is sufficiently
     * high to support 100G and 40G ports
     *********************************************/
    if ( GetNbPorts(&sInfo->tmp.p100G) &&
         (sInfo->tmp.schedLen < (MIN_PORT_SPACING * SLOTS_PER_100G) ) )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "freq not high enough to support 100G w/4-cycle spacing\n");
        goto ABORT;
    }

    if ( GetNbPorts(&sInfo->tmp.p60G) &&
         (sInfo->tmp.schedLen < (MIN_PORT_SPACING * SLOTS_PER_60G) ) )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "freq not high enough to support 60G w/4-cycle spacing\n");
        goto ABORT;
    }

    if ( GetNbPorts(&sInfo->tmp.p40G) &&
         (sInfo->tmp.schedLen < (MIN_PORT_SPACING * SLOTS_PER_40G) ) )
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "freq not high enough to support 40G w/4-cycle spacing\n");
        goto ABORT;
    }

    /*********************************************
     * Compute number of slots required per 
     * speed bin. 
     ********************************************/
    slots100G   = GetNbPorts(&sInfo->tmp.p100G)  * SLOTS_PER_100G;
    slots60G    = GetNbPorts(&sInfo->tmp.p60G)   * SLOTS_PER_60G;
    slots40G    = GetNbPorts(&sInfo->tmp.p40G)   * SLOTS_PER_40G;
    slots25G    = GetNbPorts(&sInfo->tmp.p25G)   * SLOTS_PER_25G;
    slots10G    = GetNbPorts(&sInfo->tmp.p10G)   * SLOTS_PER_10G;
    slots2500M  = GetNbPorts(&sInfo->tmp.p2500M) * SLOTS_PER_2500M;
    slotsIdle   = sInfo->tmp.schedLen - slots100G - slots60G - slots40G - slots25G - slots10G - slots2500M;
    
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 100G Slots", slots100G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 60G Slots", slots60G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 40G Slots", slots40G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 25G Slots", slots25G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 10G Slots", slots10G);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number 2.5G Slots", slots2500M);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "%-20s = %d\n", "Number Idle Slots", slotsIdle);

    if (slotsIdle <= 0)
    {
        err = FM_ERR_SCHED_OVERSUBSCRIBED;
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Oversubscribed schedule, minimum of 1 idle slot needed\n");
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Requested %d slots (%.1fG), but only %d are available (%.1fG)\n",
                     slots100G + slots60G + slots40G + slots25G + slots10G + slots2500M + 1,
                     ((slots100G + slots60G + slots40G + slots25G + slots10G + slots2500M + 1) * (fm_float)(SLOT_SPEED)),
                     sInfo->tmp.schedLen,
                     (sInfo->tmp.schedLen * (fm_float)(SLOT_SPEED)));
        goto ABORT;
    }

    /*********************************************
     * Split the bandwidth by populating the slots 
     * with port speeds
     *********************************************/
    err = PopulateSpeedList(sw, slots100G, slots60G, slots40G, slots25G, slots10G, slots2500M, slotsIdle );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);


    /*********************************************
     * Need to find the first idle token, this 
     * will be our implicit idle and it does not
     * need to be added to the schedule 
     *********************************************/
    err = RotateSchedule(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);


    /*********************************************
     * Sort the ports by the difficulty level 
     * of placing them in the schedule.
     *********************************************/
    err = SortPortsByDifficulty(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    
    /*********************************************
     * We have the speed bins, fill them with 
     * ports
     *********************************************/
    FM_CLEAR(sInfo->tmp.schedList);
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        /* Mark all ports as invalid */
        sInfo->tmp.schedList[i].port = -1;
    }

    err = AssignPortsByDifficulty(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Assign Idle Tokens */
    for (i = 0; i < sInfo->tmp.schedLen; i++)
    {
        if ( sInfo->tmp.speedList[i] == FM10000_SCHED_SPEED_IDLE )
        {
            sToken = &sInfo->tmp.schedList[i];
            sToken->port        = 0;
            sToken->fabricPort  = 0;
            sToken->quad        = 0;
            sToken->idle        = 1;
        }
    }

    /* Fill in the fabricPort, Quad, and Idle fields per portList entries */
    for (i = 0; i < sInfo->tmp.nbPorts; i++)
    {
        physPort = sInfo->tmp.portList[i].physPort;

        for (j = 0; j < sInfo->tmp.schedLen; j++)
        {
            if ( (physPort == sInfo->tmp.schedList[j].port) &&
                 (sInfo->tmp.schedList[j].idle == 0) )
            {
                sInfo->tmp.schedList[j].fabricPort = sInfo->tmp.portList[i].fabricPort;
                sInfo->tmp.schedList[j].quad       = sInfo->reservedQuad[physPort];
                sInfo->tmp.schedList[j].idle       = 0;
            }
        }
    }

    fmGetLoggingAttribute(FM_LOG_ATTR_CATEGORY_MASK, 
                          0, 
                          (void *) &logCat);
    fmGetLoggingAttribute(FM_LOG_ATTR_LEVEL_MASK, 
                          0, 
                          (void *) &logLvl);
    
    if ( (logCat & FM_LOG_CAT_SWITCH) &&
         (logLvl & FM_LOG_LEVEL_DEBUG) )
    {
        DbgDumpSchedulerConfig(sw, TMP_SCHEDULE, FALSE);
    }

    err = CalcStats(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = ValidateSchedule(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = GenerateQpcState(sw, sInfo->tmp.schedList, sInfo->tmp.schedLen, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetSchedRing(sw, 
                              FM10000_SCHED_RING_ALL, 
                              sInfo->tmp.schedList,
                              sInfo->tmp.schedLen);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
   
ABORT:
    
    /* Delete Internal BitArrays */
    fmDeleteBitArray(&sInfo->tmp.p2500M);
    fmDeleteBitArray(&sInfo->tmp.p10G);
    fmDeleteBitArray(&sInfo->tmp.p25G);
    fmDeleteBitArray(&sInfo->tmp.p40G);
    fmDeleteBitArray(&sInfo->tmp.p60G);
    fmDeleteBitArray(&sInfo->tmp.p100G);

    if (err == FM_OK)
    {
        /* We have succeeded, store the scheduler state into the active
         * structure */
        FM_MEMCPY_S(&sInfo->active, 
                    sizeof(sInfo->active), 
                    &sInfo->tmp, 
                    sizeof(sInfo->tmp) );
    }

    DROP_SCHEDULER_LOCK(sw);

    fmGetTime(&tGen);

    fmSubTimestamps(&tGen, &tStart, &tDiff);
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                 "Generation Length = %lld us\n", 
                 tDiff.usec + tDiff.sec * 1000000);
    
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000RegenerateSchedule */




/*****************************************************************************/
/** fm10000GetSchedMode
 * \ingroup intSwitch
 *
 * \desc            Retrieve the scheduler mode. 
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      attr is a pointer to the caller allocated storage where
 *                  the attributes should be stored. 
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetSchedAttributes(fm_int sw, fm10000_schedAttr *attr)
{
    fm_status err = FM_OK;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm10000_schedInfo  *sInfo;

    TAKE_SCHEDULER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    sInfo     = &switchExt->schedInfo;

    attr->mode            = sInfo->attr.mode;
    attr->updateLnkChange = sInfo->attr.updateLnkChange;

    DROP_SCHEDULER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GetSchedMode */
