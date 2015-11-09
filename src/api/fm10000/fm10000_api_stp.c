/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_stp.c
 * Creation Date:   July 2, 2013 from fm6000_api_stp.c
 * Description:     Implement FM10000 specific STP functions.
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
 *****************************************************************************/

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM_STP_DBG_DUMP_VLANS_PER_ROW   15
#define FM_STP_DBG_DUMP_PORTS_PER_ROW   30




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
/** ReadIngressMstTable
 * \ingroup intPort
 *
 * \desc            Reads an entry from the INGRESS_MST_TABLE register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index1 is the outer index of the register table entry.
 * 
 * \param[in]       index0 is the inner index of the register table entry.
 * 
 * \param[out]      value points to the location to receive the contents
 *                  of the register table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadIngressMstTable(fm_int      sw,
                                     fm_int      index1,
                                     fm_int      index0,
                                     fm_uint64 * value)
{

#if (FM10000_USE_MST_TABLE_CACHE)
    return fmRegCacheReadUINT64V2(sw,
                                  &fm10000CacheIngressMstTable,
                                  index1,
                                  index0,
                                  value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->ReadUINT64(sw,
                                 FM10000_INGRESS_MST_TABLE(index1, index0, 0),
                                 value);
#endif

}   /* end ReadIngressMstTable */




/*****************************************************************************/
/** WriteIngressMstTable
 * \ingroup intPort
 *
 * \desc            Writes an entry to the INGRESS_MST_TABLE register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index1 is the outer index of the register table entry.
 * 
 * \param[in]       index0 is the inner index of the register table entry.
 * 
 * \param[in]       value is the value to be written to the register entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteIngressMstTable(fm_int    sw,
                                      fm_int    index1,
                                      fm_int    index0,
                                      fm_uint64 value)
{

#if (FM10000_USE_MST_TABLE_CACHE)
    return fmRegCacheWriteUINT64V2(sw,
                                   &fm10000CacheIngressMstTable,
                                   index1,
                                   index0,
                                   value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->WriteUINT64(sw,
                                  FM10000_INGRESS_MST_TABLE(index1, index0, 0),
                                  value);
#endif

}   /* end WriteIngressMstTable */




/*****************************************************************************/
/** ReadEgressMstTable
 * \ingroup intPort
 *
 * \desc            Reads an entry from the EGRESS_MST_TABLE register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[out]      value points to the location to receive the contents
 *                  of the register table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadEgressMstTable(fm_int sw, fm_int index, fm_uint64 * value)
{

#if (FM10000_USE_MST_TABLE_CACHE)
    return fmRegCacheReadUINT64(sw, &fm10000CacheEgressMstTable, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->ReadUINT64(sw, FM10000_EGRESS_MST_TABLE(index, 0), value);
#endif

}   /* end ReadEgressMstTable */




/*****************************************************************************/
/** WriteEgressMstTable
 * \ingroup intPort
 *
 * \desc            Writes an entry to the EGRESS_MST_TABLE register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[in]       value is the value to be written to the register entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteEgressMstTable(fm_int sw, fm_int index, fm_uint64 value)
{

#if (FM10000_USE_MST_TABLE_CACHE)
    return fmRegCacheWriteUINT64(sw, &fm10000CacheEgressMstTable, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->WriteUINT64(sw, FM10000_EGRESS_MST_TABLE(index, 0), value);
#endif

}   /* end WriteEgressMstTable */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000CreateSpanningTree
 * \ingroup intStp
 *
 * \desc            Creates a spanning tree instance.
 *                  Always returns an error if the mode is incorrect.
 *                  Called through the CreateSpanningTree function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       stpInstance is the instance number being created. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_STP_MODE if the mode is not MULTIPLE.
 *
 *****************************************************************************/
fm_status fm10000CreateSpanningTree(fm_int sw, fm_int stpInstance)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, 
                 "sw=%d, stpInstance=%d\n",
                 sw, 
                 stpInstance);

    FM_NOT_USED(stpInstance);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->stpMode != FM_SPANNING_TREE_MULTIPLE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STP, FM_ERR_INVALID_STP_MODE);
    }

    FM_LOG_EXIT(FM_LOG_CAT_STP, FM_OK);

}   /* end fm10000CreateSpanningTree */




/*****************************************************************************/
/** fm10000AddSpanningTreeVlan
 * \ingroup intStp
 *
 * \desc            Adds a vlan to a spanning tree.
 *                  Called through the AddSpanningTreeVlan function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       stpInstance is the spanning tree instance.
 *
 * \param[in]       vlan is the vlan ID.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddSpanningTreeVlan(fm_int sw, fm_int stpInstance, fm_int vlan)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "sw=%d, stpInstance=%d, vlan=%d\n",
                 sw, 
                 stpInstance, 
                 vlan);

    status = fm10000WriteVlanEntryV2(sw, vlan, stpInstance);

    FM_LOG_EXIT(FM_LOG_CAT_STP, status);

}   /* end fm10000AddSpanningTreeVlan */




/*****************************************************************************/
/** fm10000DeleteSpanningTreeVlan
 * \ingroup intStp
 *
 * \desc            Implements the FM10000 specific portion of deleting a
 *                  VLAN from a spanning tree. The function simply updates
 *                  the VLAN entry in the hardware by assuming that the vlan
 *                  should be put into the default spanning-tree instance.
 *                  Called through the DeleteSpanningTreeVlan function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       stpInstance is the instance number the VLAN is being
 *                  deleted from.
 *
 * \param[in]       vlan is the VLAN being deleted. 
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteSpanningTreeVlan(fm_int sw,
                                        fm_int stpInstance, 
                                        fm_int vlan)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "sw=%d, stpInstance=%d, vlan=%d\n",
                 sw, 
                 stpInstance, 
                 vlan);

    FM_NOT_USED(stpInstance);

    status = fm10000WriteVlanEntryV2(sw, vlan, FM_DEFAULT_STP_INSTANCE);

    FM_LOG_EXIT(FM_LOG_CAT_STP, status);

}   /* end fm10000DeleteSpanningTreeVlan */




/*****************************************************************************/
/** fm10000RefreshSpanningTree
 * \ingroup intStp
 *
 * \desc            Optimized refresh routing to apply a STP state change
 *                  to all member VLANs.  Assumes the STP instance info
 *                  has already been updated.
 *                  Called through the RefreshSpanningTree function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      instance points to the info structure for the 
 *                  STP instance.
 *
 * \param[in]       vlan is the vlan to update or -1 to update all VLANs in
 *                  that instance.
 *
 * \param[in]       port is the port to update or -1 to update all ports.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000RefreshSpanningTree(fm_int             sw,
                                     fm_stpInstanceInfo *instance, 
                                     fm_int              vlan,
                                     fm_int              port)
{
    fm_status   err;
    fm_switch * switchPtr;
    fm_int      currentPort;
    fm_int      physPort;
    fm_int      cpi;
    fm_int      mcastSearchPortList[FM10000_NUM_PORTS];
    fm_int      numPortsForMcastSearch;
    fm_int      index;
    fm_int      state;
    fm_uint64   portIngressState;
    fm_uint64   portEgressState;
    fm_uint64   ingressStates[FM10000_INGRESS_MST_TABLE_ENTRIES_1];
    fm_uint64   egressStates;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "sw=%d, instance=%p, vlan=%d, port=%d\n",
                 sw, 
                 (void *) instance, 
                 vlan, 
                 port);

    switchPtr = GET_SWITCH_PTR(sw);

    /***************************************************
     * On FM10000, we only need to write to the hardware
     * when we change STP state, not when VLANs are
     * added.  When one port changes state we will
     * write new states for all ports.
     **************************************************/
    if (vlan != -1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STP, FM_OK);
    }

    /***************************************************
     * Construct list of ports to be used for updating
     * the MTable listener state.
     **************************************************/

    if (port == -1)
    {
        err = fmGetCardinalPortList(sw,
                                    &numPortsForMcastSearch,
                                    mcastSearchPortList,
                                    FM10000_NUM_PORTS);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }
    else
    {
        /* A single port list only */
        numPortsForMcastSearch = 1;
        mcastSearchPortList[0] = port;
    }

    /***************************************************
     * Update the MTable state using the list of ports
     * computed in mcastSearchPortList
     **************************************************/

    for ( index = 0 ; index < numPortsForMcastSearch ; index++ )
    {
        currentPort = mcastSearchPortList[index];
        cpi = GET_PORT_INDEX(sw, currentPort);
        state = instance->states[cpi];

        /***************************************************
         * The IP multicast table is not STP aware, so now
         * let us inform the MTable code to skip any
         * instances of this (port, instance) pair before we
         * update the port's STP state.  Note that we 
         * rely on the MTable code to maintain a list of
         * (port, instance) pairs by keeping track of 
         * the vlan to instance mapping.
         **************************************************/
        err = fm10000MTableUpdateListenerState(sw,
                                              instance->instance, 
                                              currentPort, 
                                              state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    }

    /***************************************************
     * Now proceed to update the instance itself.  
     **************************************************/

    memset( ingressStates, 0, sizeof(ingressStates) );
    egressStates = 0;

    /***************************************************
     * Build the ingress and egress state words.  To avoid
     * reading, we always recompute the entire table
     * since it takes the same amount of work and 
     * all the information is already here.
     **************************************************/
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        fmMapCardinalPortInternal(switchPtr, cpi, &currentPort, &physPort);

        state = instance->states[cpi];

        /***************************************************
         * In FM4xxx, the CPU port was always forwarding,
         * so we preserve this behaviour.
         **************************************************/
        if (currentPort == switchPtr->cpuPort)
        {
            state = FM_STP_STATE_FORWARDING;
        }

        switch (state)
        {
            case FM_STP_STATE_DISABLED:
            case FM_STP_STATE_BLOCKING:
                portIngressState = FM10000_STPSTATE_DISABLED;
                portEgressState  = 0;
                break;

            case FM_STP_STATE_LISTENING:
                portIngressState = FM10000_STPSTATE_LISTENING;
                portEgressState  = 0;
                break;

            case FM_STP_STATE_LEARNING:
                portIngressState = FM10000_STPSTATE_LEARNING;
                portEgressState  = 0;
                break;

            case FM_STP_STATE_FORWARDING:
                portIngressState = FM10000_STPSTATE_FORWARDING;
                portEgressState  = 1;
                break;

            default:
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
        }

        if (physPort <= 23)
        {
            ingressStates[0] |= portIngressState << (physPort * 2);
        }
        else
        {
            ingressStates[1] |= portIngressState << ( (physPort - 24) * 2 );
        }

        egressStates |= portEgressState << physPort;

    }

    err = WriteIngressMstTable(sw, 0, instance->instance, ingressStates[0]);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = WriteIngressMstTable(sw, 1, instance->instance, ingressStates[1]);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = WriteEgressMstTable(sw, instance->instance, egressStates);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fm10000RefreshSpanningTree */




/*****************************************************************************/
/** fm10000ResetVlanSpanningTreeState
 * \ingroup intVlan
 *
 * \desc            Resets the spanning tree state for each port in the VLAN
 *                  to the non-member default value. Called through the
 *                  ResetVlanSpanningTreeState function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ResetVlanSpanningTreeState(fm_int sw, fm_uint16 vlanID)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d vlanID=%u\n", sw, vlanID);

    FM_NOT_USED(vlanID);

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = &switchPtr->stpInstanceInfo;

    /* Retrieve the default stp instance. */
    err = fmTreeFind(stpInfo, FM_DEFAULT_STP_INSTANCE, (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    
    err = fm10000RefreshSpanningTree(sw,
                                    instance, 
                                    -1, 
                                    -1);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fm10000ResetVlanSpanningTreeState */




/*****************************************************************************/
/** fm10000DbgDumpSpanningTree
 * \ingroup intStp
 *
 * \desc            Dumps the software and hardware spanning tree tables.
 *                  Called through the DbgDumpSpanningTree function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       whichInstance is the instance number to dump, or -1 for all.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpSpanningTree(fm_int sw, fm_int whichInstance)
{
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_status           status;
    fm_treeIterator     iter;
    fm_stpInstanceInfo *instance;
    fm_uint64           key;
    fm_int              vlan;
    fm_int              stid;
    fm_int              firstStid;
    fm_int              lastStid;
    fm_int              outer;
    fm_int              limit;
    fm_int              cpi;
    fm_int              port;
    fm_int              i;
    fm_int              ivFid;
    fm_int              evFid;
    fm_uint32           ingressVid[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm_uint32           egressVid[FM10000_EGRESS_VID_TABLE_WIDTH];
    fm_uint64           ingressMst0;
    fm_uint64           ingressMst1;
    fm_uint64           egressMst;
    fm_int              stpState;

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    if ( (switchPtr->stpMode != FM_SPANNING_TREE_MULTIPLE)
         && (whichInstance != 0) )
    {
        FM_LOG_PRINT("Switch #%d is not in multiple spanning-tree mode.", sw);
        return;
    }

    if (whichInstance == -1)
    {
        firstStid = 0;
        lastStid  = FM10000_INGRESS_MST_TABLE_ENTRIES_0 - 1;
    }
    else
    {
        firstStid = whichInstance;
        lastStid  = whichInstance;
    }

    FM_TAKE_L2_LOCK(sw);

    FM_LOG_PRINT("Spanning Tree State (Software)\n");
    FM_LOG_PRINT("------------------------------\n\n");

    fmTreeIterInit(&iter, stpInfo);

    status = fmTreeIterNext(&iter, &key, (void **) &instance);

    while (status == FM_OK)
    {
        /* Skip this instance if it doesn't match */
        if ( (whichInstance != -1 )
             && (instance->instance != whichInstance) )
        {

            status = fmTreeIterNext(&iter,
                                    &key,
                                    (void **) &instance);

            continue;
        }

        FM_LOG_PRINT("Instance #%d:\n", instance->instance);
        FM_LOG_PRINT("  VLAN Members:\n    ");

        for (i = 0, vlan = 0; vlan < FM10000_INGRESS_VID_TABLE_ENTRIES; vlan++)
        {
            fm_bool present;

            fmGetBitArrayBit(&instance->vlans, vlan, &present);

            if (present)
            {
                printf("%04d ", vlan);
                i++;

                if ( (i % FM_STP_DBG_DUMP_VLANS_PER_ROW) ==
                     (FM_STP_DBG_DUMP_VLANS_PER_ROW - 1) )
                {
                    FM_LOG_PRINT("\n    ");
                }
            }

        }

        FM_LOG_PRINT("\n  Port States:\n");

        for ( outer = 0 ;
              outer < switchPtr->numCardinalPorts ;
              outer += FM_STP_DBG_DUMP_PORTS_PER_ROW )
        {
            limit = outer + FM_STP_DBG_DUMP_PORTS_PER_ROW;
            if (limit > switchPtr->numCardinalPorts)
            {
                limit = switchPtr->numCardinalPorts;
            }

            FM_LOG_PRINT("    ");
            for ( cpi = outer ; cpi < limit ; cpi++ )
            {
                port = GET_LOGICAL_PORT(sw, cpi);
                FM_LOG_PRINT("%02d ", port);
            }
            FM_LOG_PRINT("\n    ");

            for ( cpi = outer ; cpi < limit ; cpi++ )
            {
                switch (instance->states[cpi])
                {
                    case FM_STP_STATE_FORWARDING:
                        FM_LOG_PRINT("Fw ");
                        break;

                    case FM_STP_STATE_DISABLED:
                        FM_LOG_PRINT("D  ");
                        break;

                    case FM_STP_STATE_BLOCKING:
                        FM_LOG_PRINT("Bl ");
                        break;

                    case FM_STP_STATE_LISTENING:
                        FM_LOG_PRINT("Li ");
                        break;

                    case FM_STP_STATE_LEARNING:
                        FM_LOG_PRINT("Le ");
                        break;

                    default:
                        FM_LOG_PRINT("Unknown\n");
                        break;

                }   /* end switch (instance->states[cpi]) */
            }

            FM_LOG_PRINT("\n");
        }

        FM_LOG_PRINT("\n");

        status = fmTreeIterNext(&iter, &key, (void **) &instance);
    }

    FM_LOG_PRINT("\nSpanning Tree State (Hardware)\n");
    FM_LOG_PRINT("------------------------------\n\n");
    FM_LOG_PRINT("VLAN -> FID Mappings:\n");

    for (vlan = 0; vlan < FM10000_INGRESS_VID_TABLE_ENTRIES; vlan++)
    {
        status = switchPtr->ReadUINT32Mult(sw,
                                           FM10000_INGRESS_VID_TABLE(vlan, 0),
                                           FM10000_INGRESS_VID_TABLE_WIDTH,
                                           ingressVid);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        ivFid = FM_ARRAY_GET_FIELD(ingressVid,
                                   FM10000_INGRESS_VID_TABLE,
                                   FID);

        status = switchPtr->ReadUINT32Mult(sw,
                                           FM10000_EGRESS_VID_TABLE(vlan, 0),
                                           FM10000_EGRESS_VID_TABLE_WIDTH,
                                           egressVid);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        evFid = FM_ARRAY_GET_FIELD(egressVid,
                                   FM10000_EGRESS_VID_TABLE,
                                   FID);

        if ( (whichInstance != -1)
             && (ivFid != whichInstance)
             && (evFid != whichInstance) )
        {
            continue;
        }

        FM_LOG_PRINT("  %04d : IV=0x%03x EV=0x%03x\n", vlan, ivFid, evFid);
    }

    FM_LOG_PRINT("\nIngress Spanning-Tree States:\n");
    
    for ( stid = firstStid ; stid <= lastStid ; stid++ )
    {
        status = ReadIngressMstTable(sw, 0, stid, &ingressMst0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        status = ReadIngressMstTable(sw, 1, stid, &ingressMst1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        FM_LOG_PRINT("  %03d : ", stid);

        for (i = 23 ; i >= 0 ; i--)
        {
            stpState = ( ingressMst1 >> (i * 2) ) & 0x3;
            FM_LOG_PRINT("%d", stpState);
        }

        for (i = 23 ; i >= 0 ; i--)
        {
            stpState = ( ingressMst0 >> (i * 2) ) & 0x3;
            FM_LOG_PRINT("%d", stpState);
        }
    }

    FM_LOG_PRINT("\nEgress Spanning-Tree States:\n");

    for ( stid = firstStid ; stid <= lastStid ; stid++ )
    {
        status = ReadEgressMstTable(sw, stid, &egressMst);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        FM_LOG_PRINT("  %03d : ", stid);

        for (i = 47 ; i >= 0 ; i--)
        {
            stpState = ( egressMst >> (i * 2) ) & 0x3;
            FM_LOG_PRINT("%d", stpState);
        }
    }

ABORT:

    FM_DROP_L2_LOCK(sw);

}   /* end fm10000DbgDumpSpanningTree */




/*****************************************************************************/
/** fm10000EnableMultipleSpanningTreeMode
 * \ingroup intStp
 *
 * \desc            Configures the transition to multiple spanning tree mode
 *                  which consists of resetting the STP state on the ports.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000EnableMultipleSpanningTreeMode(fm_int sw)
{
    fm_status           status = FM_OK;
    fm_switch *         switchPtr;
    fm_stpInstanceInfo *instance;
    fm_tree *           stpInfo;
    fm_int              cpi;
    fm_int              port;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    FM_TAKE_L2_LOCK(sw);

    /***************************************************
     * Reconfigure the default instance.
     **************************************************/

    status = fmTreeFind(stpInfo, 0, (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

    /* Now set the default state */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        if ( fmIsInternalPort(sw, port) || (port == switchPtr->cpuPort) )
        {
            instance->states[cpi] = FM_STP_STATE_FORWARDING;
        }
        else
        {
            instance->states[cpi] = GET_PROPERTY()->defStateVlanNonMember;
        }
    }

    /* Refresh the hardware for all ports */
    status = fmRefreshStpStateInternal(switchPtr, instance, -1, -1);

ABORT:

    FM_DROP_L2_LOCK(sw);

    if (status == FM_OK)
    {
        /* Delete all the MA table entries */
        status = fmFlushAllDynamicAddresses(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_STP, status);

}   /* end fm10000EnableMultipleSpanningTreeMode */




/*****************************************************************************/
/** fm10000EnableSharedSpanningTreeMode
 * \ingroup intStp
 *
 * \desc            Configures the transition to shared spanning tree mode,
 *                  which removes all stp instances greater than 0.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000EnableSharedSpanningTreeMode(fm_int sw)
{
    fm_status           status = FM_OK;
    fm_switch *         switchPtr;
    fm_treeIterator     iter;
    fm_stpInstanceInfo *instance;
    fm_tree *           stpInfo;
    fm_uint64           key;
    fm_int              cpi;
    fm_int              port;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    FM_TAKE_L2_LOCK(sw);

    fmTreeIterInit(&iter, stpInfo);

    status = fmTreeIterNext(&iter, &key, (void **) &instance);

    while (status == FM_OK)
    {
        if (instance->instance > 0)
        {
            status = fmDeleteSpanningTreeInternal(sw, instance);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

            /* Re-initialize the iterator because we modified the tree */
            fmTreeIterInitFromSuccessor(&iter, stpInfo, 0);
        }
        status = fmTreeIterNext(&iter, &key, (void **) &instance);
    }

    if (status != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);
    }
    
    /***************************************************
     * Now proceed to reconfigure the default instance.
     **************************************************/

    status = fmTreeFind(stpInfo, 0, (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

    /* Now set the default state */
    for (cpi = 0; cpi < switchPtr->numCardinalPorts; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        if ( fmIsInternalPort(sw, port) || (port == switchPtr->cpuPort) )
        {
            instance->states[cpi]  = FM_STP_STATE_FORWARDING;
        }
        else
        {
            instance->states[cpi] = GET_PROPERTY()->defStateVlanNonMember;
        }
    }

    /* Refresh the hardware for all ports */
    status = fmRefreshStpStateInternal(switchPtr, instance, -1, -1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

ABORT:

    FM_DROP_L2_LOCK(sw);

    if (status == FM_OK)
    {
        /* Delete all the MA table entries */
        status = fmFlushAllDynamicAddresses(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_STP, status);

}   /* end fm10000EnableSharedSpanningTreeMode */

