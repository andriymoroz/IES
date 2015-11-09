/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_sflow.c
 * Creation Date:   February 14, 2014
 * Description:     FM10000 sFlow API implementation.
 *
 * Copyright (c) 2008 - 2015, Intel Corporation
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

#define TAKE_SFLOW_LOCK(sw)     TAKE_MIRROR_LOCK(sw)
#define DROP_SFLOW_LOCK(sw)     DROP_MIRROR_LOCK(sw)


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
/** GetSflowEntry
 * \ingroup intSflow
 *
 * \desc            Returns a pointer to an SFlow instance.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       sflowId is the SFlow identifier.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm10000_sflowEntry * GetSflowEntry(fm_int sw, fm_int sflowId)
{
    fm10000_switch *    switchExt;
    fm10000_sflowEntry *sflowEntry;

    if (sflowId < 0 || sflowId >= FM10000_MAX_SFLOWS)
    {
        return NULL;
    }

    switchExt  = GET_SWITCH_EXT(sw);
    sflowEntry = &switchExt->sflowEntry[sflowId];

    return sflowEntry;

}   /* end GetSflowEntry */




/*****************************************************************************/
/** FreeSFlow
 * \ingroup intSflow
 *
 * \desc            Frees the specified trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       sflowId is the SFlow to free.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeSFlow(fm_int sw, fm_int sflowId)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err = FM_OK;

    sflowEntry = GetSflowEntry(sw, sflowId);
    if (sflowEntry && sflowEntry->isValid)
    {
        err = fmDeleteMirrorInt(sw, sflowEntry->mirrorId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);
        
        sflowEntry->isValid = FALSE;
    }

ABORT:
    return err;

}   /* end FreeSFlow */




/*****************************************************************************/
/** SetSampleRate
 * \ingroup intSflow
 *
 * \desc            Sets the sample rate for the SFlow.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       sflowId is the SFlow to free.
 * 
 * \param[in]       sampleRate is the sample rate to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetSampleRate(fm_int sw, fm_int sflowId, fm_uint sampleRate)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d sflowId=%d sampleRate=%u\n",
                 sw,
                 sflowId,
                 sampleRate);

    if (sampleRate < 1 || sampleRate > FM10000_SFLOW_MAX_SAMPLE_RATE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    sflowEntry = GetSflowEntry(sw, sflowId);

    if (sflowEntry == NULL)
    {
        /* Should never happen. */
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    sflowEntry->sampleRate = sampleRate;

    err = fmSetMirrorAttributeInt(sw, 
                                  sflowEntry->mirrorId, 
                                  FM_MIRROR_SAMPLE_RATE, 
                                  (void *)&sampleRate);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end SetSampleRate */




/*****************************************************************************/
/** SetVlan
 * \ingroup intSflow
 *
 * \desc            Sets the VLAN association for the SFlow.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       sflowId is the SFlow to free.
 * 
 * \param[in]       vlanID is the VLAN identifier to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetVlan(fm_int sw, fm_int sflowId, fm_uint16 vlanID)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;
    fm_mirrorVlanType   mirrorVlanType;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d sflowId=%d vlanID=%u\n",
                 sw,
                 sflowId,
                 vlanID);

    err = FM_OK;
    sflowEntry = GetSflowEntry(sw, sflowId);

    if (sflowEntry == NULL)
    {
        /* Should never happen. */
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    if ( (vlanID >= FM_MAX_VLAN) ||
         (vlanID != FM_SFLOW_VLAN_ANY && !(GET_VLAN_PTR(sw, vlanID))->valid) )
    {
        err = FM_ERR_INVALID_VLAN;
        goto ABORT;
    }

    switch (sflowEntry->sflowType)
    {
        case FM_SFLOW_TYPE_INGRESS:
            mirrorVlanType = FM_MIRROR_VLAN_INGRESS;
            break;

        case FM_SFLOW_TYPE_EGRESS:
            mirrorVlanType = FM_MIRROR_VLAN_EGRESS;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;
    }

    /* If there is existing VLAN, then delete the VLAN in the 
     * mirror group and then add new VLAN */
    if (sflowEntry->vlanID  != FM_SFLOW_VLAN_ANY)
    {
        err = fmDeleteMirrorVlanInternal(sw,
                                         sflowEntry->mirrorId,
                                         FM_VLAN_SELECT_VLAN1,
                                         sflowEntry->vlanID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);
    }

    if (vlanID != FM_SFLOW_VLAN_ANY)
    {
        err = fmAddMirrorVlanInternal(sw, 
                                      sflowEntry->mirrorId, 
                                      FM_VLAN_SELECT_VLAN1,
                                      vlanID,
                                      mirrorVlanType);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);
    }
    sflowEntry->vlanID = vlanID;



ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end SetVlan */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000InitSFlows
 * \ingroup intSflow
 *
 * \desc            Initializes the SFlow subsystem.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitSFlows(fm_int sw)
{
    fm10000_sflowEntry *sflowEntry;
    fm_int              sflowId;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW, "sw=%d\n", sw);

    for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId)
    {
        sflowEntry = GetSflowEntry(sw, sflowId);

        FM_CLEAR(*sflowEntry);

        sflowEntry->mirrorId   = GET_SWITCH_PTR(sw)->mirrorTableSize
                                  - sflowId - 1;
        sflowEntry->isValid    = FALSE;
        sflowEntry->trapCodeId = sflowId;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, FM_OK);

}   /* end fm10000InitSFlows */




/*****************************************************************************/
/** fm10000FreeSFlows
 * \ingroup intSflow
 *
 * \desc            Frees the SFlow API resources.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeSFlows(fm_int sw)
{
    fm_status   retVal;
    fm_status   err;
    fm_int      sflowId;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW, "sw=%d\n", sw);

    retVal = FM_OK;

    for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId)
    {
        err = FreeSFlow(sw, sflowId);
        FM_ERR_COMBINE(retVal, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, retVal);

}   /* end fm10000FreeSFlows */




/*****************************************************************************/
/** fm10000CreateSFlow
 * \ingroup intSflow
 *
 * \desc            Create a sflow instance on an FM10000 chip. 
 *                  The switch has been validated by the top-level fuction.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is the ID of the sFlow instance to create.
 *
 * \param[in]       sFlowType is the type of the sFlow instance to create.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is out of range
 *                  or the instance already exists.
 * \return          FM_ERR_NO_FREE_TRIG_RES if no free trigger resources are
 *                  available.
 * \return          FM_ERR_TRIGGER_UNAVAILABLE if no free trigger is available.
 *
 *****************************************************************************/
fm_status fm10000CreateSFlow(fm_int sw, fm_int sFlowId, fm_sFlowType sFlowType)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;
    fm_bool             mirrorAlloc;
    fm_mirrorType       mirrorType;
    fm_int              mirrorTrapCodeId;
    fm_switch          *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW, "sw=%d, sFlowId=%d\n", sw, sFlowId);

    mirrorAlloc = FALSE;
    switchPtr   = GET_SWITCH_PTR(sw);

    TAKE_SFLOW_LOCK(sw);

    /**************************************************
     * Get SFlow entry.
     **************************************************/

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (!sflowEntry || sflowEntry->isValid)
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
        goto ABORT;
    }

    switch (sFlowType)
    {
        case FM_SFLOW_TYPE_INGRESS:
            mirrorType = FM_MIRROR_TYPE_INGRESS;
            break;

        case FM_SFLOW_TYPE_EGRESS:
            mirrorType = FM_MIRROR_TYPE_EGRESS;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;
    }

    sflowEntry->sampleRate  = 1;    /* is this correct? */
    sflowEntry->sflowType   = sFlowType;
    sflowEntry->vlanID      = FM_SFLOW_VLAN_ANY;

    /**************************************************
     * Create mirror.
     **************************************************/
    err = fmCreateMirrorInt(sw, 
                            sflowEntry->mirrorId, 
                            switchPtr->cpuPort,
                            mirrorType,
                            FM_MIRROR_USAGE_TYPE_SFLOW);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);
    
    mirrorAlloc = TRUE;

    mirrorTrapCodeId = FM10000_SFLOW_TRAPCODE_ID_START + 
                       sflowEntry->trapCodeId;

    err = fmSetMirrorAttributeInt(sw,
                                  sflowEntry->mirrorId,
                                  FM_MIRROR_TRAPCODE_ID,
                                  (void *)&mirrorTrapCodeId );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

    err = SetSampleRate(sw, sFlowId, sflowEntry->sampleRate);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);


    /**************************************************
     * Mark the SFlow as active.
     **************************************************/

    sflowEntry->isValid = TRUE;

ABORT:

    if (err != FM_OK)
    {
        if (mirrorAlloc)
        {
            fmDeleteMirrorInt(sw, sflowEntry->mirrorId);
        }
    }
    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000CreateSFlow */




/*****************************************************************************/
/** fm10000DeleteSFlow
 * \ingroup intSflow
 *
 * \desc            Delete a sFlow instance.
 * 
 * \note            The switch has been validated by the top-level fuction.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is the ID of the sFlow instance to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteSFlow(fm_int sw, fm_int sFlowId)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW, "sw=%d, sFlowId=%d\n", sw, sFlowId);

    /* ACL Lock needs to be taken prior to the mirror lock for lock inversion
     * prevention. The ACL lock is needed on fmDeleteMirrorInt() because some
     * validation is done at the ACL level to make sure this group is currently
     * unused. */
    FM_TAKE_ACL_LOCK(sw);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (sflowEntry && sflowEntry->isValid)
    {
        err = FreeSFlow(sw, sFlowId);
    }
    else
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
    }

    DROP_SFLOW_LOCK(sw);
    FM_DROP_ACL_LOCK(sw);
 
    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000DeleteSFlow */




/*****************************************************************************/
/** fm10000AddSFlowPort
 * \ingroup intSflow
 *
 * \desc            Add a port to a sFlow instance.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is sFlow instance to operate on.
 *
 * \param[in]       port is port to be added to the sFlow instance.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 *
 *****************************************************************************/
fm_status fm10000AddSFlowPort(fm_int sw, fm_int sFlowId, fm_int port)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;
    fm_mirrorType       mirrorType;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, port=%d\n",
                 sw, 
                 sFlowId,
                 port);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (!sflowEntry || !sflowEntry->isValid)
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
        goto ABORT;
    }

    switch (sflowEntry->sflowType)
    {
        case FM_SFLOW_TYPE_INGRESS:
            mirrorType = FM_MIRROR_TYPE_INGRESS;
            break;

        case FM_SFLOW_TYPE_EGRESS:
            mirrorType = FM_MIRROR_TYPE_EGRESS;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;
    }

    err = fmAddMirrorPortInternal(sw, sflowEntry->mirrorId, port, mirrorType);

ABORT:
    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000AddSFlowPort */




/*****************************************************************************/
/** fm10000DeleteSFlowPort
 * \ingroup intSflow
 *
 * \desc            Delete a port from a sFlow instance.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is sFlow instance to operate on.
 *
 * \param[in]       port is port to be deleted from the sFlow instance.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteSFlowPort(fm_int sw, fm_int sFlowId, fm_int port)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, port=%d\n",
                 sw, 
                 sFlowId, 
                 port);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (!sflowEntry || !sflowEntry->isValid)
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
        goto ABORT;
    }

    err = fmDeleteMirrorPortInt(sw, sflowEntry->mirrorId, port);

ABORT:
    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000DeleteSFlowPort */




/*****************************************************************************/
/** fm10000GetSFlowPortFirst
 * \ingroup intSflow
 *
 * \desc            Retrieve the first port of the sFlow.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is sFlow instance to operate on.
 *
 * \param[out]      firstPort points to a caller-supplied memory location 
 *                  to receive the first logical port of the sFlow.
 *                  It will be -1 if no port is found.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetSFlowPortFirst(fm_int  sw, fm_int sFlowId, fm_int *firstPort)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, firstPort=%p\n",
                 sw,
                 sFlowId,
                 (void *) firstPort);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (sflowEntry && sflowEntry->isValid)
    {
        err = fmGetMirrorPortFirstInt(sw, sflowEntry->mirrorId, firstPort);
        if (err == FM_ERR_NO_PORTS_IN_MIRROR_GROUP)
        {
            err = FM_ERR_NO_SFLOW_PORT;
        }
    }
    else
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
    }

    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000GetSFlowPortFirst */




/*****************************************************************************/
/** fm10000GetSFlowPortNext
 * \ingroup intSflow
 *
 * \desc            Retrieve the next port of the sFlow.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is the sFlow instance to operate on.
 *
 * \param[in]       startPort is logicalPort to start from. 
 *
 * \param[out]      nextPort points to a caller-supplied memory location
 *                  to receive the next logical port of the sFlow.
 *                  It will be -1 if no port is found.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetSFlowPortNext(fm_int   sw, 
                                  fm_int   sFlowId, 
                                  fm_int   startPort, 
                                  fm_int * nextPort)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, startPort=%d, nextPort=%p\n",
                 sw, 
                 sFlowId,
                 startPort,
                 (void *) nextPort);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (sflowEntry && sflowEntry->isValid)
    {
        err = fmGetMirrorPortNextInt(sw, 
                                     sflowEntry->mirrorId, 
                                     startPort,
                                     nextPort);
        if (err == FM_ERR_NO_PORTS_IN_MIRROR_GROUP)
        {
            err = FM_ERR_NO_SFLOW_PORT;
        }
    }
    else
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
    }

    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000GetSFlowPortNext */




/*****************************************************************************/
/** fm10000GetSFlowPortList
 * \ingroup intSflow
 *
 * \desc            Retrieve a list of ports of the sFlow.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is the sFlow instance to operate on.
 *
 * \param[out]      numPorts points to a caller-supplied location to
 *                  receive the number of ports retrieved.
 *
 * \param[out]      portList points to a caller-supplied array to receive
 *                  the list of ports.
 *
 * \param[in]       maxPorts is the size of portList.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_BUFFER_FULL is portList is not enough to hold
 *                  all the ports of the sFlow.
 *
 *****************************************************************************/
fm_status fm10000GetSFlowPortList(fm_int   sw, 
                                  fm_int   sFlowId, 
                                  fm_int * numPorts, 
                                  fm_int * portList, 
                                  fm_int   maxPorts)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, numPorts=%p, portList=%p, max=%d\n",
                 sw,
                 sFlowId,
                 (void *) numPorts,
                 (void *) portList,
                 maxPorts);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (sflowEntry && sflowEntry->isValid)
    {
        if (sflowEntry->sflowType == FM_SFLOW_TYPE_INGRESS)
        {
            err = fmGetMirrorPortListsInt(sw,
                                          sflowEntry->mirrorId,
                                          numPorts,
                                          portList,
                                          maxPorts,
                                          NULL,
                                          NULL,
                                          0);
        }
        else if (sflowEntry->sflowType == FM_SFLOW_TYPE_EGRESS)
        {
            err = fmGetMirrorPortListsInt(sw,
                                          sflowEntry->mirrorId,
                                          NULL,
                                          NULL,
                                          0,
                                          numPorts,
                                          portList,
                                          maxPorts);
        }
        else
        {
            err = FM_ERR_INVALID_SFLOW_INSTANCE;
        }
    }
    else
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
    }

    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000GetSFlowPortList */




/*****************************************************************************/
/** fm10000SetSFlowAttribute
 * \ingroup intSflow
 *
 * \desc            Set the sFlow attributes (See sFlow attribute definition). 
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is the sFlow instance to set the attribute.
 *
 * \param[in]       attr is sFlow attribute to set.
 *
 * \param[out]      value points to the value of the sFlow attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SFLOW_ATTR if attr is not recognized.
 *
 *****************************************************************************/
fm_status fm10000SetSFlowAttribute(fm_int sw, 
                                   fm_int sFlowId, 
                                   fm_int attr, 
                                   void * value)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, attr=%d, value=%p\n",
                 sw, 
                 sFlowId, 
                 attr, 
                 (void *) value);

    TAKE_SFLOW_LOCK(sw);

    /**************************************************
     * Get SFlow entry.
     **************************************************/

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (!sflowEntry || !sflowEntry->isValid)
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
        goto ABORT;
    }

    /**************************************************
     * Process SFlow attribute.
     **************************************************/

    switch (attr)
    {
        case FM_SFLOW_VLAN:
            err = SetVlan(sw, sFlowId, *( (fm_uint16 *) value));
            break;

        case FM_SFLOW_SAMPLE_RATE:
            err = SetSampleRate(sw, sFlowId, *( (fm_uint *) value));
            break;

        default:
            err = FM_ERR_INVALID_SFLOW_ATTR;
            break;

    }   /* end switch (attr) */

ABORT:
    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000SetSFlowAttribute */




/*****************************************************************************/
/** fm10000GetSFlowAttribute
 * \ingroup intSflow
 *
 * \desc            Get sFlow attribute.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is sFlow Instance to operate on.
 *
 * \param[in]       attr is sFlow attribute to get.
 *
 * \param[out]      value points to the caller-supplied location to receive
 *                  the attribute value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetSFlowAttribute(fm_int sw, 
                                   fm_int sFlowId, 
                                   fm_int attr, 
                                   void * value)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;
    fm_int              mirrorCntAttr;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, attr=%d, value=%p\n",
                 sw, 
                 sFlowId, 
                 attr, 
                 (void *) value);

    TAKE_SFLOW_LOCK(sw);

    /**************************************************
     * Get SFlow entry.
     **************************************************/

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (!sflowEntry || !sflowEntry->isValid)
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
        goto ABORT;
    }

    /**************************************************
     * Process SFlow attribute.
     **************************************************/

    err = FM_OK;

    switch (attr)
    {
        case FM_SFLOW_COUNT:
            mirrorCntAttr = (sflowEntry->sflowType == FM_SFLOW_TYPE_INGRESS) ?
                            FM_MIRROR_INGRESS_COUNTER :
                            FM_MIRROR_EGRESS_COUNTER;    
            err = fmGetMirrorAttributeInt(sw,
                                          sflowEntry->mirrorId,
                                          mirrorCntAttr,
                                          value);            
            break;

        case FM_SFLOW_TRAP_CODE:
            *( (fm_int *) value) = sflowEntry->trapCodeId;
            break;

        case FM_SFLOW_VLAN:
            *( (fm_uint16 *) value) = sflowEntry->vlanID;
            break;

        case FM_SFLOW_SAMPLE_RATE:
            *( (fm_uint *) value) = sflowEntry->sampleRate;
            break;

        default:
            err = FM_ERR_INVALID_SFLOW_ATTR;
            break;

    }   /* end switch (attr) */

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

ABORT:
    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000GetSFlowAttribute */




/*****************************************************************************/
/** fm10000CheckSFlowLogging
 * \ingroup intSflow
 *
 * \desc            Check if the packet is received due to a sFlow instance.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pktEvent points to the FM_EVENT_PACKET_RECV event to check.
 *
 * \param[out]      isPktSFlowLogged points to the caller-supplied storage on
 *                  whether the packet is logged due to a sFlow instance.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
fm_status fm10000CheckSFlowLogging(fm_int            sw, 
                                   fm_eventPktRecv * pktEvent, 
                                   fm_bool         * isPktSFlowLogged)
{
    fm10000_sflowEntry *sflowEntry;
    fm_int              sflowId;
    fm_int              sflowTrapCodeId;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, pktEvent=%p, isPktSFlowLogged=%p\n",
                 sw,
                 (void *) pktEvent,
                 (void *) isPktSFlowLogged);

    *isPktSFlowLogged = FALSE;
    sflowTrapCodeId   = pktEvent->trapAction - 
                        FM10000_MIRROR_CPU_CODE_BASE -
                        FM10000_SFLOW_TRAPCODE_ID_START;

    TAKE_SFLOW_LOCK(sw);

    for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId)
    {
        sflowEntry = GetSflowEntry(sw, sflowId);
        if (sflowEntry && sflowEntry->isValid && 
            sflowEntry->trapCodeId == sflowTrapCodeId)
        {
            *isPktSFlowLogged = TRUE;
            break;
        }
    }

    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, FM_OK);

}   /* end fm10000CheckSFlowLogging */




/*****************************************************************************/
/** fm10000GetSFlowType
 * \ingroup intSflow
 *
 * \desc            Retrieve the type of the sFlow instance.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       sFlowId is the sFlow instance to operate on.
 *
 * \param[out]      sFlowType is the type of the sFlow instance.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetSFlowType(fm_int sw, fm_int sFlowId, fm_sFlowType *sFlowType)
{
    fm10000_sflowEntry *sflowEntry;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, sFlowId=%d, sFlowType=%p\n",
                 sw,
                 sFlowId,
                 (void *) sFlowType);

    TAKE_SFLOW_LOCK(sw);

    sflowEntry = GetSflowEntry(sw, sFlowId);

    if (sflowEntry && sflowEntry->isValid)
    {
        *sFlowType = sflowEntry->sflowType;
        err = FM_OK;
    }
    else
    {
        err = FM_ERR_INVALID_SFLOW_INSTANCE;
    }

    DROP_SFLOW_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fm10000GetSFlowType */




/*****************************************************************************/
/** fmDbgPrintPortList
 * \ingroup intSflow
 *
 * \desc            Prints a port set as a list of comma-separated ranges.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       numPorts is the number of ports in the array.
 *
 * \param[in]       portList is the array which contains the list of ports.
 *
 * \return          FM_OK if successful.
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmDbgPrintPortList(fm_int sw, fm_int numPorts, fm_int *portList)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_int      port;
    fm_int      lastPort;
    fm_bool     inSet;
    fm_int      i;

    enum { EMPTY, FIRST, RANGE, GAP } state;

    switchPtr = GET_SWITCH_PTR(sw);
    lastPort  = -1;
    state     = EMPTY;

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; ++cpi)
    {
        port  = GET_LOGICAL_PORT(sw, cpi);
        inSet = FALSE;
        for (i = 0; i < numPorts; i++)
        {
            if (portList[i] == port)
            {
                inSet = TRUE;
            }
        }

        switch (state)
        {
            case EMPTY:
                /* The output set is empty. */
                if (inSet)
                {
                    FM_LOG_PRINT("%d", port);
                    state = FIRST;
                }
                break;

            case FIRST:
                /* We've processed the first port in a sequence. */
                if (inSet)
                {
                    lastPort = port;
                    state = RANGE;
                }
                else
                {
                    state = GAP;
                }
                break;

            case RANGE:
                /* We're processing a range in the port list. */
                if (inSet)
                {
                    lastPort = port;
                }
                else
                {
                    FM_LOG_PRINT("..%d", lastPort);
                    state = GAP;
                }
                break;

            case GAP:
                /* We're processing a gap in the port list. */
                if (inSet)
                {
                    FM_LOG_PRINT(",%d", port);
                    state = FIRST;
                }
                break;

        }   /* end switch (state) */

    }   /* end for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; ++cpi) */

    if (state == RANGE)
    {
        FM_LOG_PRINT("..%d", lastPort);
    }

    return FM_OK;

}   /* end fmDbgPrintPortList */




/*****************************************************************************/
/** fm10000DbgDumpSFlows
 * \ingroup intSflow
 *
 * \desc            Dumps information about the SFLOW subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpSFlows(fm_int sw)
{
    fm10000_sflowEntry *sflowEntry;
    fm_int              sflowId;
    fm_text             typeText;
    fm_int              portList[48];
    fm_int              numPorts;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Id  MirId sflowType  vlanID  ports\n");
    FM_LOG_PRINT("--  ----  ---------  ------  --------------------\n");

    for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId)
    {
        sflowEntry = GetSflowEntry(sw, sflowId);
        if (!sflowEntry || !sflowEntry->isValid)
        {
            continue;
        }

        switch (sflowEntry->sflowType)
        {
            case FM_SFLOW_TYPE_INGRESS:
                typeText = "ingress";
                break;

            case FM_SFLOW_TYPE_EGRESS:
                typeText = "egress";
                break;

            default:
                typeText = "unknown";
                break;
        }

        FM_LOG_PRINT("%2d  %4d  %-9s  ",
                     sflowId,
                     sflowEntry->mirrorId,
                     typeText);

        if (sflowEntry->vlanID == FM_SFLOW_VLAN_ANY)
        {
            FM_LOG_PRINT("%5s   ", "ANY");
        }
        else
        {
            FM_LOG_PRINT("%5u   ",
                         sflowEntry->vlanID);
        }

        fm10000GetSFlowPortList(sw, sflowId, &numPorts, portList, 48);
        fmDbgPrintPortList(sw, numPorts, portList);
        FM_LOG_PRINT("\n");

    }   /* end for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId) */

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fm10000DbgDumpSFlows */
