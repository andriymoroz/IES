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

#define TAKE_SFLOW_LOCK(sw)     TAKE_TRIGGER_LOCK(sw)
#define DROP_SFLOW_LOCK(sw)     DROP_TRIGGER_LOCK(sw)


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
/** ConfigTriggerCond
 * \ingroup intSflow
 *
 * \desc            Configures an SFlow trigger.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       sflowId is the SFlow identifier.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigTriggerCond(fm_int sw, fm_int sflowId)
{
    fm10000_sflowEntry *sflowEntry;
    fm_triggerCondition trigCond;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW, "sw=%d sflowId=%d\n", sw, sflowId);

    sflowEntry = GetSflowEntry(sw, sflowId);

    if (sflowEntry == NULL)
    {
        /* Should never happen. */
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /**************************************************
     * Initialize trigger condition.
     **************************************************/

    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Configure Tx/Rx port masks.
     **************************************************/

    if (sflowEntry->sflowType == FM_SFLOW_TYPE_INGRESS)
    {
        /* Match selected ingress ports. */
        /* Match all egress ports (by default). */
        trigCond.cfg.rxPortset = sflowEntry->portSet;
    }
    else
    {
        /* Match all ingress ports. */
        /* Match selected egress ports. */
        trigCond.cfg.rxPortset = FM_PORT_SET_ALL;
        trigCond.cfg.txPortset = sflowEntry->portSet;
        trigCond.cfg.matchTx   = FM_TRIGGER_TX_MASK_CONTAINS;
    }

    /**************************************************
     * Configure vlan trigger.
     **************************************************/

    if (sflowEntry->vlanID == FM_SFLOW_VLAN_ANY)
    {
        /* Match any vlan. */
        trigCond.cfg.matchVlan = FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
        trigCond.param.vidId = 0;
    }
    else if (sflowEntry->vlanResId != FM10000_SFLOW_RES_ID_NONE)
    {
        /* Match selected vlan. */
        trigCond.cfg.matchVlan = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
        trigCond.param.vidId = sflowEntry->vlanResId;
    }

    /**************************************************
     * Configure sample rate.
     **************************************************/

    if (sflowEntry->sampleRate)
    {
        trigCond.cfg.matchRandomNumber = TRUE;
        trigCond.param.randGenerator = FM_TRIGGER_RAND_GEN_A;
        trigCond.param.randMatchThreshold = 
            ((FM10000_SFLOW_MAX_SAMPLE_RATE + 1) / sflowEntry->sampleRate) - 1;
    }

    /**************************************************
     * Set trigger condition.
     **************************************************/

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_SFLOW,
                                     sflowEntry->rule,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end ConfigTriggerCond */




/*****************************************************************************/
/** FreePortSet
 * \ingroup intSflow
 *
 * \desc            Frees the specified port set.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   portSet points to a location containing the identifier
 *                  of the port set to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreePortSet(fm_int sw, fm_int * portSet)
{
    fm_status   err;

    if (portSet == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (*portSet == FM_PORT_SET_NONE)
    {
        return FM_OK;
    }

    err = fmDeletePortSetInt(sw, *portSet);

    if (err == FM_ERR_INVALID_PORT_SET)
    {
        err = FM_OK;
    }
    else if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                     "Error deleting portSet (%d): %s\n",
                     *portSet,
                     fmErrorMsg(err));
    }

    *portSet = FM_PORT_SET_NONE;

    return err;

}   /* end FreePortSet */




/*****************************************************************************/
/** FreeTrigger
 * \ingroup intSflow
 *
 * \desc            Frees the specified trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the trigger group number.
 * 
 * \param[in]       rule is the trigger rule number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeTrigger(fm_int sw, fm_int group, fm_int rule)
{
    fm_status   err;

    err = fm10000DeleteTrigger(sw, group, rule, TRUE);

    if (err == FM_ERR_INVALID_TRIG)
    {
        err = FM_OK;
    }
    else if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                     "Error deleting trigger (%d, %d): %s\n",
                     group,
                     rule,
                     fmErrorMsg(err));
    }

    return err;

}   /* end FreeTrigger */




/*****************************************************************************/
/** FreeVlanResource
 * \ingroup intSflow
 *
 * \desc            Frees the VLAN resource identifier.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       sflowId is the SFlow on which to operate
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeVlanResource(fm_int sw, fm_int sflowId)
{
    fm10000_sflowEntry *sflowEntry;
    fm10000_vlanEntry * ventryExt;
    fm_status           retVal;
    fm_status           err;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SFLOW, "sw=%d sflowId=%d\n", sw, sflowId);

    sflowEntry = GetSflowEntry(sw, sflowId);

    retVal = FM_OK;

    if (sflowEntry &&
        sflowEntry->isValid &&
        sflowEntry->vlanID != FM_SFLOW_VLAN_ANY &&
        sflowEntry->vlanResId != FM10000_SFLOW_RES_ID_NONE)
    {
        ventryExt = GET_VLAN_EXT(sw, sflowEntry->vlanID);
        if (ventryExt->trigger != 0)
        {
            if (ventryExt->trigger == sflowEntry->vlanResId)
            {
                /* Remove trigger from VLAN entry. */
                err = fm10000SetVlanTrigger(sw, sflowEntry->vlanID, 0);
                FM_ERR_COMBINE(retVal, err);
            }
            else
            {
                /* This should never happen. If it does, it probably means
                 * that two entities (SFlow and Mirroring) are assigning
                 * triggers to the same VLAN. */
                FM_LOG_ERROR(FM_LOG_CAT_SFLOW,
                             "VLAN triggerId (%d) != SFLOW triggerId (%d)\n",
                             ventryExt->trigger,
                             sflowEntry->vlanResId);
            }
        }

        /* Free vlan trigger resource. */
        err = fm10000FreeTriggerResource(sw,
                                         FM_TRIGGER_RES_VLAN,
                                         sflowEntry->vlanResId,
                                         TRUE);
        FM_ERR_COMBINE(retVal, err);

        sflowEntry->vlanResId = FM10000_SFLOW_RES_ID_NONE;
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SFLOW, retVal);

}   /* end FreeVlanResource */




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
    fm_status           retVal;
    fm_status           err;

    sflowEntry = GetSflowEntry(sw, sflowId);

    retVal = FM_OK;

    if (sflowEntry && sflowEntry->isValid)
    {
        err = FreeTrigger(sw, FM10000_TRIGGER_GROUP_SFLOW, sflowEntry->rule);
        sflowEntry->rule = FM10000_SFLOW_RULE_NONE;
        FM_ERR_COMBINE(retVal, err);

        err = FreeVlanResource(sw, sflowId);
        FM_ERR_COMBINE(retVal, err);

        err = FreePortSet(sw, &sflowEntry->portSet);
        FM_ERR_COMBINE(retVal, err);

        sflowEntry->isValid = FALSE;
    }

    return retVal;

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

    err = ConfigTriggerCond(sw, sflowId);

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
    fm10000_vlanEntry * ventryExt;
    fm_uint32           vlanResId;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d sflowId=%d vlanID=%u\n",
                 sw,
                 sflowId,
                 vlanID);

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

    /* Free VLAN resource if one is in use. */
    err = FreeVlanResource(sw, sflowId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

    /* Save VLAN identifier in SFlow entry. */
    sflowEntry->vlanID = vlanID;

    /* Allocate new resource if needed. */
    if (vlanID != FM_SFLOW_VLAN_ANY)
    {
        err = fm10000AllocateTriggerResource(sw,
                                             FM_TRIGGER_RES_VLAN,
                                             &vlanResId,
                                             TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

        sflowEntry->vlanResId = vlanResId;

        ventryExt = GET_VLAN_EXT(sw, vlanID);
        if (ventryExt->trigger != 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_SFLOW,
                         "VLAN %u has existing trigger assignment (%d)\n",
                         vlanID,
                         ventryExt->trigger);
            err = FM_FAIL;
            goto ABORT;
        }

        err = fm10000SetVlanTrigger(sw, vlanID, vlanResId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);
    }

    err = ConfigTriggerCond(sw, sflowId);

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

        sflowEntry->rule     = FM10000_SFLOW_RULE_NONE;
        sflowEntry->portSet  = FM_PORT_SET_NONE;
        sflowEntry->isValid  = FALSE;
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
    fm_triggerAction    trigAction;
    fm_char             trigName[32];
    fm_status           err;
    fm_bool             triggerAlloc;
    fm_bool             portSetAlloc;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW, "sw=%d, sFlowId=%d\n", sw, sFlowId);

    triggerAlloc = FALSE;
    portSetAlloc = FALSE;

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
        case FM_SFLOW_TYPE_EGRESS:
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;
    }

    sflowEntry->rule        = FM10000_SFLOW_RULE_NONE;
    sflowEntry->portSet     = FM_PORT_SET_NONE;
    sflowEntry->sampleRate  = 1;    /* is this correct? */
    sflowEntry->sflowType   = sFlowType;
    sflowEntry->vlanID      = FM_SFLOW_VLAN_ANY;
    sflowEntry->vlanResId   = FM10000_SFLOW_RES_ID_NONE;

    /**************************************************
     * Create the trigger.
     **************************************************/

    FM_SPRINTF_S(trigName, sizeof(trigName), "sFlowTrigger[%d]", sFlowId);

    sflowEntry->rule = sFlowId * 10;

    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_SFLOW,
                               sflowEntry->rule,
                               TRUE,
                               trigName);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    triggerAlloc = TRUE;

    /**************************************************
     * Create the port set.
     **************************************************/

    err = fmCreatePortSetInt(sw, &sflowEntry->portSet, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    portSetAlloc = TRUE;

    /**************************************************
     * Configure trigger action.
     **************************************************/

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_LOG;

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_SFLOW,
                                  sflowEntry->rule,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Configure trigger condition.
     **************************************************/

    err = ConfigTriggerCond(sw, sFlowId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Mark the SFlow as active.
     **************************************************/

    sflowEntry->isValid = TRUE;

ABORT:
    if (err != FM_OK && sflowEntry)
    {
        if (triggerAlloc)
        {
            FreeTrigger(sw, FM10000_TRIGGER_GROUP_SFLOW, sflowEntry->rule);
            sflowEntry->rule = FM10000_SFLOW_RULE_NONE;
        }

        if (portSetAlloc)
        {
            FreePortSet(sw, &sflowEntry->portSet);
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
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
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

    err = fm10000GetTrigger(sw,
                            FM10000_TRIGGER_GROUP_SFLOW,
                            sflowEntry->rule,
                            &trigCond,
                            &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

    err = fmAddPortSetPortInt(sw, sflowEntry->portSet, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_SFLOW,
                                     sflowEntry->rule,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

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
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
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

    err = fm10000GetTrigger(sw,
                            FM10000_TRIGGER_GROUP_SFLOW,
                            sflowEntry->rule,
                            &trigCond,
                            &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

    err = fmDeletePortSetPortInt(sw, sflowEntry->portSet, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_SFLOW,
                                     sflowEntry->rule,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SFLOW, err);

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
        err = fmGetPortSetPortFirstInt(sw, sflowEntry->portSet, firstPort);
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
        err = fmGetPortSetPortNextInt(sw,
                                      sflowEntry->portSet,
                                      startPort,
                                      nextPort);
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
        err = fmPortSetToPortList(sw,
                                  sflowEntry->portSet,
                                  numPorts,
                                  portList,
                                  maxPorts);
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
            err = fm10000GetTriggerAttribute(sw,
                                             FM10000_TRIGGER_GROUP_SFLOW,
                                             sflowEntry->rule,
                                             FM_TRIGGER_ATTR_COUNTER,
                                             value);
            break;

        case FM_SFLOW_TRAP_CODE:
            *( (fm_int *) value) = sflowEntry->rule;
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
    fm_int              group;
    fm_int32            rule;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SFLOW,
                 "sw=%d, pktEvent=%p, isPktSFlowLogged=%p\n",
                 sw,
                 (void *) pktEvent,
                 (void *) isPktSFlowLogged);

    *isPktSFlowLogged = FALSE;

    err = fm10000GetTriggerFromTrapCode(sw, pktEvent->trapAction, &group, &rule);

    if (err == FM_OK && group == FM10000_TRIGGER_GROUP_SFLOW)
    {
        TAKE_SFLOW_LOCK(sw);

        for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId)
        {
            sflowEntry = GetSflowEntry(sw, sflowId);
            if (sflowEntry && sflowEntry->isValid && sflowEntry->rule == rule)
            {
                *isPktSFlowLogged = TRUE;
                break;
            }
        }

        DROP_SFLOW_LOCK(sw);
    }

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
/** fmDbgPrintPortSet
 * \ingroup intSflow
 *
 * \desc            Prints a port set as a list of comma-separated ranges.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       portSet is the port set to dump.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmDbgPrintPortSet(fm_int sw, fm_int portSet)
{
    fm_switch * switchPtr;
    fm_portmask portMask;
    fm_status   err;
    fm_int      cpi;
    fm_int      port;
    fm_int      lastPort;
    fm_bool     inSet;

    enum { EMPTY, FIRST, RANGE, GAP } state;

    switchPtr = GET_SWITCH_PTR(sw);
    lastPort  = -1;
    state     = EMPTY;

    err = fmPortSetToPortMask(sw, portSet, &portMask);
    if (err != FM_OK)
    {
        FM_LOG_PRINT("(ERROR)");
        return err;
    }

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; ++cpi)
    {
        port  = GET_LOGICAL_PORT(sw, cpi);
        inSet = FM_PORTMASK_IS_BIT_SET(&portMask, cpi);

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

}   /* end fmDbgPrintPortSet */




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

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Id  rule  sflowType  vlanID  resId  portSet  ports\n");
    FM_LOG_PRINT("--  ----  ---------  ------  -----  -------  --------------------\n");

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
                     sflowEntry->rule,
                     typeText);

        if (sflowEntry->vlanID == FM_SFLOW_VLAN_ANY)
        {
            FM_LOG_PRINT("%5s   %4s   ", "ANY", "--");
        }
        else
        {
            FM_LOG_PRINT("%5u   %4d   ",
                         sflowEntry->vlanID,
                         sflowEntry->vlanResId);
        }

        if (sflowEntry->portSet == FM_PORT_SET_NONE)
        {
            FM_LOG_PRINT("%5s  ", "NONE");
        }
        else
        {
            FM_LOG_PRINT("%5d    ", sflowEntry->portSet);
            fmDbgPrintPortSet(sw, sflowEntry->portSet);
        }

        FM_LOG_PRINT("\n");

    }   /* end for (sflowId = 0 ; sflowId < FM10000_MAX_SFLOWS ; ++sflowId) */

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fm10000DbgDumpSFlows */
