/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stats.c
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with counters
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


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** LookupVlanCounterID
 * \ingroup intStats
 *
 * \desc            find the VLAN counter table associated with a specific
 *                  vlan
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ci points to the counter table array
 *
 * \param[in]       vlan is the desired vlan id
 *
 * \param[out]      vcid contains the counter table array index, if found.
 *
 * \return          TRUE if successful.
 * \return          FALSE if unsuccessful.
 *
 *****************************************************************************/
static fm_bool LookupVlanCounterID(fm_int          sw,
                                   fm_counterInfo *ci,
                                   fm_int          vlan,
                                   fm_int *        vcid)
{
    fm_int     i;
    fm_switch *switchPtr;
    fm_bool    found = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d ci=%p, vlan=%d, vcid=%p\n",
                 sw,
                 (void *) ci,
                 vlan,
                 (void *) vcid);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_TAKE_STATE_LOCK(sw);

    for (i = 0 ; i <= switchPtr->maxVlanCounter ; i++)
    {
        /* If this counter is the unused counter then no single
         * vlan can be mapped to it.
         */
        if (i == FM_UNUSED_VLAN_COUNTER_ID)
        {
            continue;
        }

        if (ci->vlanAssignedToCounter[i] == vlan)
        {
            *vcid = i;
            found = TRUE;
            break;
        }
    }

    FM_DROP_STATE_LOCK(sw);

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VLAN,
                       found,
                       "found = %d, id = %d\n",
                       found,
                       *vcid);

}   /* end LookupVlanCounterID */




/*****************************************************************************/
/** AllocateVlanCounterID
 * \ingroup intStats
 *
 * \desc            allocate a vlan counter entry for a vlan
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ci points to the counter table array
 *
 * \param[in]       vlan is the desired vlan id
 *
 * \param[out]      vcid contains the counter table array index, if an
 *                  available counter is found.
 *
 * \return          TRUE if successful.
 * \return          FALSE if unsuccessful.
 *
 *****************************************************************************/
static fm_bool AllocateVlanCounterID(fm_int          sw,
                                     fm_counterInfo *ci,
                                     fm_int          vlan,
                                     fm_int *        vcid)
{
    fm_int     i;
    fm_switch *switchPtr;
    fm_bool    allocated = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d ci=%p, vlan=%d, vcid=%p\n",
                 sw,
                 (void *) ci,
                 vlan,
                 (void *) vcid);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_TAKE_STATE_LOCK(sw);

    for (i = 0 ; i <= switchPtr->maxVlanCounter ; i++)
    {
        /* A single VLAN cannot be mapped to the FM_UNUSED_VLAN_COUNTER_ID
         * counter set.
         */
        if (i == FM_UNUSED_VLAN_COUNTER_ID)
        {
            continue;
        }

        if (ci->vlanAssignedToCounter[i] == FM_UNALLOCATED_VLAN_COUNTER)
        {
            ci->vlanAssignedToCounter[i] = vlan;
            *vcid                        = i;
            allocated = TRUE;
            break;
        }
    }

    FM_DROP_STATE_LOCK(sw);

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VLAN,
                       allocated,
                       "allocated = %d, id = %d\n",
                       allocated,
                       *vcid);

}   /* end AllocateVlanCounterID */




/*****************************************************************************/
/** fmInitPortCounters
 * \ingroup intStats
 *
 * \desc            initializes counters for a port
 *
 * \param[in]       pc points to the port counters table
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void fmInitPortCounters(fm_portCounters *pc)
{
    /* All counters are zero initially.
     * The version field is also zero as the actual version value depends
     * on the current port state and the switch family.
     */
    memset( pc, 0, sizeof(fm_portCounters) );

}   /* end fmInitPortCounters */




/*****************************************************************************/
/** fmInitVlanCounters
 * \ingroup intStats
 *
 * \desc            initializes counters for a vlan
 *
 * \param[in]       vc points to the vlan counters table
 *
 * \return          None.
 *
 *****************************************************************************/
static void fmInitVlanCounters(fm_vlanCounters *vc)
{
    /* All counters are initially zero.
     */
    memset( vc, 0, sizeof(fm_vlanCounters) );

}   /* end fmInitVlanCounters */




/*****************************************************************************/
/** fmInitSwitchCounters
 * \ingroup intStats
 *
 * \desc            initializes counters for a switch
 *
 * \param[in]       sc points to the switch counters table
 *
 * \return          None.
 *
 *****************************************************************************/
static void fmInitSwitchCounters(fm_switchCounters *sc)
{
    /* All counters are initially zero.
     */
    memset( sc, 0, sizeof(fm_switchCounters) );

}   /* end fmInitSwitchCounters */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAllocateCounterDataStructures
 * \ingroup intStats
 *
 * \desc            initializes fm_counterInfo structure storage arrays.
 *                  Called from fmApiRootInit.
 *
 * \param[in]       swState points to the switch state structure to initialize
 *                  a counter info structure for.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if not enough memory for counter storage.
 *
 *****************************************************************************/
fm_status fmAllocateCounterDataStructures(fm_switch *swState)
{
    fm_status       err = FM_OK;
    fm_int          nbytes = 0;
    fm_uint32       initMode;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "switch=%p\n", (void *) swState);

    if (swState == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    if (swState->GetCountersInitMode != NULL)
    {
        err = swState->GetCountersInitMode(swState->switchNumber, &initMode);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else
    {
        /* Should not happen, the GetCountersInitMode function pointer should
         * be initialized */
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (initMode & FM_STAT_PORT_INIT_GENERIC)
    {
        swState->counterInfo.portCount = swState->maxPhysicalPort;

        /* lastReadPort */

        nbytes = sizeof(fm_portCounters) * (swState->maxPhysicalPort + 1);
        swState->counterInfo.lastReadPort = (fm_portCounters *) fmAlloc(nbytes);

        if (swState->counterInfo.lastReadPort == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }

        memset(swState->counterInfo.lastReadPort, 0, nbytes);

        /* subtractPort */

        swState->counterInfo.subtractPort = (fm_portCounters *) fmAlloc(nbytes);

        if (swState->counterInfo.subtractPort == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }

        memset(swState->counterInfo.subtractPort, 0, nbytes);
    }

    if (initMode & FM_STAT_VLAN_INIT_GENERIC)
    {
        swState->counterInfo.vlanCount = swState->maxVlanCounter;

        /* lastReadVlan */

        nbytes = sizeof(fm_vlanCounters) * (swState->maxVlanCounter + 1);
        swState->counterInfo.lastReadVlan = (fm_vlanCounters *) fmAlloc(nbytes);

        if (swState->counterInfo.lastReadVlan == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }

        memset(swState->counterInfo.lastReadVlan, 0, nbytes);

        /* subtractVlan */

        swState->counterInfo.subtractVlan = (fm_vlanCounters *) fmAlloc(nbytes);

        if (swState->counterInfo.subtractVlan == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }

        memset(swState->counterInfo.subtractVlan, 0, nbytes);
    }

    if (initMode & FM_STAT_VLAN_ASSIGNMENT_INIT_GENERIC)
    {
        swState->counterInfo.vlanCount = swState->maxVlanCounter;

        /* vlanAssignedCounter */

        nbytes = sizeof(fm_int) * (swState->maxVlanCounter + 1);
        swState->counterInfo.vlanAssignedToCounter = (fm_int *) fmAlloc(nbytes);

        if (swState->counterInfo.vlanAssignedToCounter == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }

        memset(swState->counterInfo.vlanAssignedToCounter, 0, nbytes);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmAllocateCounterDataStructures */




/*****************************************************************************/
/** fmInitCounters
 * \ingroup intStats
 *
 * \desc            Initializes statistic counters
 *
 * \param[in]       sw is the switch to operate on
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitCounters(fm_int sw)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_counterInfo *ci;
    fm_uint32       initMode;
    int             i;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->GetCountersInitMode != NULL)
    {
        err = switchPtr->GetCountersInitMode(sw, &initMode);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else
    {
        /* Should not happen, the GetCountersInitMode function pointer should
         * be initialized */
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    ci = &switchPtr->counterInfo;

    if (initMode & FM_STAT_PORT_INIT_GENERIC)
    {
        for (i = 0 ; i <= ci->portCount ; i++)
        {
            fmInitPortCounters(&ci->lastReadPort[i]);
            fmInitPortCounters(&ci->subtractPort[i]);
        }
    }

    fmInitSwitchCounters(&ci->lastReadSwitch);
    fmInitSwitchCounters(&ci->subtractSwitch);
    
    if (initMode & FM_STAT_VLAN_INIT_GENERIC)
    {
        for (i = 0 ; i <= ci->vlanCount ; i++)
        {
            fmInitVlanCounters(&ci->lastReadVlan[i]);
            fmInitVlanCounters(&ci->subtractVlan[i]);
        }
    }

    if (initMode & FM_STAT_VLAN_ASSIGNMENT_INIT_GENERIC)
    {
        for (i = 0 ; i <= ci->vlanCount ; i++)
        {
            ci->vlanAssignedToCounter[i] = FM_UNALLOCATED_VLAN_COUNTER;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmInitCounters */




/*****************************************************************************/
/** fmFreeCounterDataStructures
 * \ingroup intStats
 *
 * \desc            Frees memory allocated for the counter storage arrays.
 *
 * \param[in]       swState points to the switch state structure to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeCounterDataStructures(fm_switch *swState)
{
    fm_status       err = FM_OK;
    fm_uint32       initMode;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "switch=%p\n", (void *) swState);

    if (swState == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    if (swState->GetCountersInitMode != NULL)
    {
        err = swState->GetCountersInitMode(swState->switchNumber, &initMode);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else
    {
        /* Should not happen, the GetCountersInitMode function pointer should
         * be initialized */
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (initMode & FM_STAT_PORT_INIT_GENERIC)
    {
        if (swState->counterInfo.lastReadPort)
        {
            fmFree(swState->counterInfo.lastReadPort);
            swState->counterInfo.lastReadPort = NULL;
        }
        if (swState->counterInfo.subtractPort)
        {
            fmFree(swState->counterInfo.subtractPort);
            swState->counterInfo.subtractPort = NULL;
        }
    }

    if (initMode & FM_STAT_VLAN_INIT_GENERIC)
    {
        if (swState->counterInfo.lastReadVlan)
        {
            fmFree(swState->counterInfo.lastReadVlan);
            swState->counterInfo.lastReadVlan = NULL;
        }
        if (swState->counterInfo.subtractVlan)
        {
            fmFree(swState->counterInfo.subtractVlan);
            swState->counterInfo.subtractVlan = NULL;
        }
    }

    if (initMode & FM_STAT_VLAN_ASSIGNMENT_INIT_GENERIC)
    {
        if (swState->counterInfo.vlanAssignedToCounter)
        {
            fmFree(swState->counterInfo.vlanAssignedToCounter);
            swState->counterInfo.vlanAssignedToCounter = NULL;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmFreeCounterDataStructures */




/*****************************************************************************/
/** fmGetPortCounters
 * \ingroup stats
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve port statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port for which to retrieve statistics. May
 *                  be the CPU interface port.
 *
 * \param[out]      counters points to an ''fm_portCounters'' structure to be
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmGetPortCounters(fm_int sw, fm_int port, fm_portCounters *counters)
{
    fm_status  err;
    fm_port *  portPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT, 
                     "sw=%d port=%d counters=%p\n", 
                     sw, 
                     port, 
                     (void *)counters);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    if (counters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    portPtr = GET_PORT_PTR(sw, port);

    FM_API_CALL_FAMILY(err, portPtr->GetPortCounters, sw, port, counters);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortCounters */




/*****************************************************************************/
/** fmResetPortCounters
 * \ingroup stats
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Reset port statistics.
 *
 * \note            The current values of port statistics are not affected 
 *                  when the port state is changed with ''fmSetPortState''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port for which to reset statistics. May
 *                  be the CPU interface port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmResetPortCounters(fm_int sw, fm_int port)
{
    fm_status err;
    fm_port * portPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT, "sw=%d port=%d\n", sw, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    portPtr = GET_PORT_PTR(sw, port);

    FM_API_CALL_FAMILY(err, portPtr->ResetPortCounters, sw, port);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmResetPortCounters */




/*****************************************************************************/
/** fmGetVLANCounters
 * \ingroup stats
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve VLAN statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlan is the VLAN for which to retrieve statistics. 
 *                  Statistics are available only for VLANs that have had
 *                  a set of counters allocated to them with a prior call to
 *                  ''fmAllocateVLANCounters''.
 *
 * \param[out]      counters points to an ''fm_vlanCounters'' structure to be
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlan is invalid.
 * \return          FM_ERR_NO_VLANCOUNTER if no statistics are available for
 *                  vlan.
 *
 *****************************************************************************/
fm_status fmGetVLANCounters(fm_int sw, fm_int vlan, fm_vlanCounters *counters)
{
    fm_int          vcid;
    fm_status       err;
    fm_counterInfo *ci;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, 
                     "sw=%d vlan=%d counters=%p\n", 
                     sw, 
                     vlan, 
                     (void *)counters);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    GET_SWITCH_COUNTER_INFO(sw, ci);

    if (counters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    VALIDATE_VLAN_ID(sw, vlan);

    if ( !LookupVlanCounterID(sw, ci, vlan, &vcid) )
    {
        err = FM_ERR_NO_VLANCOUNTER;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    FM_API_CALL_FAMILY(err, switchPtr->GetVLANCounters, sw, vcid, counters);

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVLANCounters */




/*****************************************************************************/
/** fmResetVLANCounters
 * \ingroup stats
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Reset VLAN statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlan is the VLAN for which to reset statistics.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlan is invalid.
 * \return          FM_ERR_NO_VLANCOUNTER if no statistics are available for
 *                  vlan.
 *
 *****************************************************************************/
fm_status fmResetVLANCounters(fm_int sw, fm_int vlan)
{
    fm_int          vcid;
    fm_counterInfo *ci;
    fm_status       err;
    /* Bug 10428: Get counters before reset. */
    fm_switch *     switchPtr;
    fm_vlanCounters counters;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, "sw=%d vlan=%d\n", sw, vlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    GET_SWITCH_COUNTER_INFO(sw, ci);

    VALIDATE_VLAN_ID(sw, vlan);

    if ( !LookupVlanCounterID(sw, ci, vlan, &vcid) )
    {
        err = FM_ERR_NO_VLANCOUNTER;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    if (switchPtr->ResetVLANCounters)
    {
        err = switchPtr->ResetVLANCounters(sw, vcid);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }
    else
    {
        /* Bug 10428: Get counters before reset. */
        FM_API_CALL_FAMILY(err, switchPtr->GetVLANCounters, sw, vcid, &counters);

        FM_TAKE_STATE_LOCK(sw);

        ci->subtractVlan[vcid] = ci->lastReadVlan[vcid];

        FM_DROP_STATE_LOCK(sw);
    }

ABORT:

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmResetVLANCounters */




/*****************************************************************************/
/** fmAllocateVLANCounters
 * \ingroup stats
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Allocate a set of VLAN counters to a particular VLAN.
 *
 * \note            The switch has a limited number of sets of VLAN counters
 *                  for collecting statistics.  These counter sets must be
 *                  allocated to a particular VLAN since there are not enough
 *                  sets to service all VLANs.
 *                                                                      \lb\lb
 *                  FM2000 devices provide 31 sets of counters.             \lb
 *                  FM3000 and FM4000 devices provide 63 sets of counters.  \lb
 *                  FM6000 devices provide 255 sets of counters.            \lb
 *                  FM10000 devices provide 63 sets of counters.            \lb
 *                                                                      \lb\lb
 *                  VLAN counter sets operate only during the time they are
 *                  allocated to a VLAN. After ''fmFreeVLANCounters'' is called,
 *                  the associated VLAN will no longer have its traffic
 *                  counted.
 *                                                                      \lb\lb
 *                  A call to this function should generally be followed by
 *                  a call to ''fmResetVLANCounters'' to eliminiate residual
 *                  counter values prior to counting on the newly assigned
 *                  VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlan is the VLAN to which the counters are to be allocated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlan is invalid.
 * \return          FM_ERR_NO_VLANCOUNTER if no statistics are available for
 *                  vlan.
 *
 *****************************************************************************/
fm_status fmAllocateVLANCounters(fm_int sw, fm_int vlan)
{
    fm_int          vcid;
    fm_counterInfo *ci;
    fm_status       err;
    fm_switch *     switchPtr;
    fm_bool         allocated = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, "sw=%d vlan=%d\n", sw, vlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlan);

    switchPtr = GET_SWITCH_PTR(sw);

    /* The non-swag physical switch case */
    GET_SWITCH_COUNTER_INFO(sw, ci);

    if ( LookupVlanCounterID(sw, ci, vlan, &vcid) )
    {
        err = FM_OK; /* Already allocated */
        goto ABORT;
    }

    if ( !AllocateVlanCounterID(sw, ci, vlan, &vcid) )
    {
        err = FM_ERR_NO_VLANCOUNTER;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    allocated = TRUE;

    if (switchPtr->AllocateVLANCounters != NULL)
    {
        /* We arrive here in the SWAG case, which recursively
           call this function on the component switches. */
        err = switchPtr->AllocateVLANCounters(sw, vlan);
    }
    else
    {
        err = fmSetVlanCounterID(sw, vlan, vcid);
    }

ABORT:
    if ( (err != FM_OK) && (allocated == TRUE) )
    {
        /* There was an error make sure the vlan counter ID is released */
        FM_TAKE_STATE_LOCK(sw);
        ci->vlanAssignedToCounter[vcid] = FM_UNALLOCATED_VLAN_COUNTER;
        FM_DROP_STATE_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmAllocateVLANCounters */




/*****************************************************************************/
/** fmFreeVLANCounters
 * \ingroup stats
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deallocate a set of VLAN counters.
 *
 * \note            The switch has a limited number of sets of VLAN counters
 *                  for collecting statistics.  When all counter sets have been
 *                  used, a set must be deallocated from one VLAN before is
 *                  can be realloacted to another VLAN since there are not
 *                  enough sets to service all VLANs at once.
 *                                                                      \lb\lb
 *                  After this function has been called, traffic on the
 *                  specified VLAN will no longer be counted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlan is the VLAN from which to deallocate counters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlan is invalid.
 * \return          FM_ERR_NO_VLANCOUNTER if no counters are allocated to
 *                  vlan.
 *
 *****************************************************************************/
fm_status fmFreeVLANCounters(fm_int sw, fm_int vlan)
{
    fm_int          vcid;
    fm_counterInfo *ci;
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, "sw=%d vlan=%d\n", sw, vlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_SWITCH_COUNTER_INFO(sw, ci);

    switchPtr = GET_SWITCH_PTR(sw);

    VALIDATE_VLAN_ID(sw, vlan);

    if ( !LookupVlanCounterID(sw, ci, vlan, &vcid) )
    {
        err = FM_ERR_NO_VLANCOUNTER;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    if (switchPtr->FreeVLANCounters != NULL)
    {
        /* We arrive here in the SWAG case, which recursively
           call this function on the component switches. */
        err = switchPtr->FreeVLANCounters(sw, vlan);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

        FM_TAKE_STATE_LOCK(sw);
        ci->vlanAssignedToCounter[vcid] = FM_UNALLOCATED_VLAN_COUNTER;
        FM_DROP_STATE_LOCK(sw);
    }
    else
    {
        err = fmSetVlanCounterID(sw, vlan, FM_UNUSED_VLAN_COUNTER_ID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

        FM_TAKE_STATE_LOCK(sw);
        ci->vlanAssignedToCounter[vcid] = FM_UNALLOCATED_VLAN_COUNTER;
        FM_DROP_STATE_LOCK(sw);
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmFreeVLANCounters */




/*****************************************************************************/
/** fmGetSwitchCounters
 * \ingroup stats
 *
 * \chips           FM2000
 *
 * \desc            Retrieve global switch statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      counters is a pointer to an ''fm_switchCounters'' structure
 *                  to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if the switch is FM4000 family.
 *
 *****************************************************************************/
fm_status fmGetSwitchCounters(fm_int sw, fm_switchCounters *counters)
{
    fm_status        err;
    fm_switch *      switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, 
                     "sw=%d counters=%p\n", 
                     sw, 
                     (void *)counters);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (counters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err, switchPtr->GetSwitchCounters, sw, counters);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmGetSwitchCounters */




/*****************************************************************************/
/** fmResetSwitchCounters
 * \ingroup stats
 *
 * \chips           FM2000
 *
 * \desc            Reset global switch statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmResetSwitchCounters(fm_int sw)
{
    fm_status       err;
    fm_counterInfo *ci;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    /* Bug 10428: Get counters before reset. */
    fm_switch *       switchPtr;
    fm_switchCounters counters;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    GET_SWITCH_COUNTER_INFO(sw, ci);

    /* Bug 10428: Get counters before reset. */
    FM_API_CALL_FAMILY(err, switchPtr->GetSwitchCounters, sw, &counters);

    FM_TAKE_STATE_LOCK(sw);

    ci->subtractSwitch = ci->lastReadSwitch;

    FM_DROP_STATE_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmResetSwitchCounters */

