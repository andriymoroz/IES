/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_flooding.c
 * Creation Date:   January 28, 2014
 * Description:     Layer 2 frame flooding feature.
 *
 * Copyright (c) 2015, Intel Corporation
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

#define ENABLE_AUX_TRIGGERS         0
#define ENABLE_STATIC_TRIGGERS      1

enum
{
    RULE_UCAST_FLOOD_TRAP,
    RULE_UCAST_FLOOD_LOG,
    RULE_UCAST_FLOOD_DROP,

    RULE_MCAST_FLOOD_TRAP,
    RULE_MCAST_FLOOD_LOG,
    RULE_MCAST_FLOOD_DROP,

    RULE_BCAST_FLOOD_TRAP,
    RULE_BCAST_FLOOD_LOG,
    RULE_BCAST_FLOOD_DROP,

#if (ENABLE_AUX_TRIGGERS)
    RULE_MCAST_DROP_TRAP,
    RULE_MCAST_MASK_DROP_TRAP,
#endif
};

enum
{
    TRIG_TYPE_TRAP,
    TRIG_TYPE_LOG,
    TRIG_TYPE_DROP,
};


/**************************************************
 * Internal trigger descriptor.
 **************************************************/
typedef struct
{
    /* Short name, used for logging. */
    fm_text     descName;

    /* Name to use when creating the trigger. */
    fm_text     trigName;

    /* Trigger group number. */
    fm_int      group;

    /* Trigger rule number. */
    fm_int      rule;

    /* The classes of frames (unicast, multicast, broadcast) the trigger
     * may match. */
    fm_uint32   classMask;

    /* The glort to match. */
    fm_uint16   glort;

    /* The type of trigger (TRAP, LOG) being configured. */
    fm_int      trigType;

    /* Offset of the associated port set.*/
    fm_uint     portSetOff;

} triggerDesc;

#define GET_PORTSET_PTR(floodInfo, desc)    \
    ( (fm_int *) ( ( (char *) (floodInfo) ) + (desc)->portSetOff ) )


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/**************************************************
 * Broadcast trigger descriptors.
 **************************************************/

static const triggerDesc bcastDropDesc =
{
    .descName   = "BCAST_DROP",
    .trigName   = "bcastFloodingDropTrigger",
    .group      = FM10000_TRIGGER_GROUP_BCAST_FLOOD,
    .rule       = RULE_BCAST_FLOOD_DROP,
    .classMask  = FM_TRIGGER_FRAME_CLASS_BCAST,
    .glort      = FM10000_GLORT_BCAST,
    .trigType   = TRIG_TYPE_DROP,
    .portSetOff = offsetof(fm10000_floodInfo, bcastDropSet),
};

static const triggerDesc bcastLogDesc =
{
    .descName   = "BCAST_LOG",
    .trigName   = "bcastFloodingLogTrigger",
    .group      = FM10000_TRIGGER_GROUP_BCAST_FLOOD,
    .rule       = RULE_BCAST_FLOOD_LOG,
    .classMask  = FM_TRIGGER_FRAME_CLASS_BCAST,
    .glort      = FM10000_GLORT_BCAST,
    .trigType   = TRIG_TYPE_LOG,
    .portSetOff = offsetof(fm10000_floodInfo, bcastLogSet),
};

static const triggerDesc bcastTrapDesc =
{
    .descName   = "BCAST_TRAP",
    .trigName   = "bcastFloodingTrapTrigger",
    .group      = FM10000_TRIGGER_GROUP_BCAST_FLOOD,
    .rule       = RULE_BCAST_FLOOD_TRAP,
    .classMask  = FM_TRIGGER_FRAME_CLASS_BCAST,
    .glort      = FM10000_GLORT_BCAST,
    .trigType   = TRIG_TYPE_TRAP,
    .portSetOff = offsetof(fm10000_floodInfo, bcastTrapSet),
};


/**************************************************
 * Multicast trigger descriptors.
 **************************************************/

static const triggerDesc mcastDropDesc =
{
    .descName   = "MCAST_DROP",
    .trigName   = "mcastFloodingDropTrigger",
    .group      = FM10000_TRIGGER_GROUP_MCAST_FLOOD,
    .rule       = RULE_MCAST_FLOOD_DROP,
    .classMask  = FM_TRIGGER_FRAME_CLASS_MCAST,
    .glort      = FM10000_GLORT_MCAST,
    .trigType   = TRIG_TYPE_DROP,
    .portSetOff = offsetof(fm10000_floodInfo, mcastDropSet),
};

static const triggerDesc mcastLogDesc =
{
    .descName   = "MCAST_LOG",
    .trigName   = "mcastFloodingLogTrigger",
    .group      = FM10000_TRIGGER_GROUP_MCAST_FLOOD,
    .rule       = RULE_MCAST_FLOOD_LOG,
    .classMask  = FM_TRIGGER_FRAME_CLASS_MCAST,
    .glort      = FM10000_GLORT_MCAST,
    .trigType   = TRIG_TYPE_LOG,
    .portSetOff = offsetof(fm10000_floodInfo, mcastLogSet),
};

static const triggerDesc mcastTrapDesc =
{
    .descName   = "MCAST_TRAP",
    .trigName   = "mcastFloodingTrapTrigger",
    .group      = FM10000_TRIGGER_GROUP_MCAST_FLOOD,
    .rule       = RULE_MCAST_FLOOD_TRAP,
    .classMask  = FM_TRIGGER_FRAME_CLASS_MCAST,
    .glort      = FM10000_GLORT_MCAST,
    .trigType   = TRIG_TYPE_TRAP,
    .portSetOff = offsetof(fm10000_floodInfo, mcastTrapSet),
};

/**************************************************
 * Unicast trigger descriptors.
 **************************************************/

static const triggerDesc ucastDropDesc =
{
    .descName   = "UCAST_DROP",
    .trigName   = "ucastFloodingDropTrigger",
    .group      = FM10000_TRIGGER_GROUP_UCAST_FLOOD,
    .rule       = RULE_UCAST_FLOOD_DROP,
    .classMask  = FM_TRIGGER_FRAME_CLASS_UCAST,
    .glort      = FM10000_GLORT_FLOOD,
    .trigType   = TRIG_TYPE_DROP,
    .portSetOff = offsetof(fm10000_floodInfo, ucastDropSet),
};

static const triggerDesc ucastLogDesc =
{
    .descName   = "UCAST_LOG",
    .trigName   = "ucastFloodingLogTrigger",
    .group      = FM10000_TRIGGER_GROUP_UCAST_FLOOD,
    .rule       = RULE_UCAST_FLOOD_LOG,
    .classMask  = FM_TRIGGER_FRAME_CLASS_UCAST,
    .glort      = FM10000_GLORT_FLOOD,
    .trigType   = TRIG_TYPE_LOG,
    .portSetOff = offsetof(fm10000_floodInfo, ucastLogSet),
};

static const triggerDesc ucastTrapDesc =
{
    .descName   = "UCAST_TRAP",
    .trigName   = "ucastFloodingTrapTrigger",
    .group      = FM10000_TRIGGER_GROUP_UCAST_FLOOD,
    .rule       = RULE_UCAST_FLOOD_TRAP,
    .classMask  = FM_TRIGGER_FRAME_CLASS_UCAST,
    .glort      = FM10000_GLORT_FLOOD,
    .trigType   = TRIG_TYPE_TRAP,
    .portSetOff = offsetof(fm10000_floodInfo, ucastTrapSet),
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static const char * fmBcastFloodingToText(fm_int floodType);
static const char * fmMcastFloodingToText(fm_int floodType);
static const char * fmUcastFloodingToText(fm_int floodType);

static fm_status FreeFloodingTrigger(fm_int sw, const triggerDesc * desc);
static fm_status FreePortSet(fm_int sw, fm_int * portSet);
static fm_status FreeTrigger(fm_int sw, fm_int group, fm_int rule);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/



#if (ENABLE_AUX_TRIGGERS)

/*****************************************************************************/
/** ConfigDroppingTrigger1
 * \ingroup intSwitch
 *
 * \desc            Configures the first multicast dropping trigger.
 *                  Its purpose is to prevent dropped flooded multicast frames
 *                  from being trapped.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigDroppingTrigger1(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    /* SetupTriggerForMaskingMcastDropTrapping */

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /**************************************************
     * Initialize trigger condition.
     **************************************************/

    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /*trapTrigCondition.triggerConditionParam.matchDroppedFrames = TRUE;*/
    /*trapTrigCondition.triggerConditionTx = 0;*/

    trigCond.cfg.matchFrameClassMask = FM_TRIGGER_FRAME_CLASS_MCAST;
    trigCond.cfg.matchRoutedMask = FM_TRIGGER_SWITCHED_FRAMES;

    trigCond.cfg.matchDestGlort  = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
    trigCond.param.destGlort     = FM10000_GLORT_MCAST;
    trigCond.param.destGlortMask = 0xFFFF;

    trigCond.cfg.rxPortset = floodInfo->dropMaskSet;

    trigCond.cfg.HAMask =
        FM_TRIGGER_HA_DROP_VLAN_IV |
        FM_TRIGGER_HA_DROP_STP_INL |
        FM_TRIGGER_HA_DROP_STP_IL  |
        FM_TRIGGER_HA_DROP_FFU;

    /**************************************************
     * Initialize trigger action.
     **************************************************/

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    trigAction.cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_ASIS;

    /**************************************************
     * Apply trigger configuration.
     **************************************************/

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_MCAST_MASK_DROP_TRAP,
                                  RULE_MCAST_MASK_DROP_TRAP,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_MCAST_MASK_DROP_TRAP,
                                     RULE_MCAST_MASK_DROP_TRAP,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    return err;

}   /* end ConfigDroppingTrigger1 */




/*****************************************************************************/
/** ConfigDroppingTrigger2
 * \ingroup intSwitch
 *
 * \desc            Configures the second multicast dropping trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigDroppingTrigger2(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    /* SetupTriggerForMcastDropTrapping */

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /**************************************************
     * Initialize trigger condition.
     **************************************************/

    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /*trapTrigCondition.triggerConditionParam.matchDroppedFrames = TRUE;*/
    /*trapTrigCondition.triggerConditionTx = 0;*/

    trigCond.cfg.matchFrameClassMask = FM_TRIGGER_FRAME_CLASS_MCAST;
    trigCond.cfg.matchRoutedMask = FM_TRIGGER_SWITCHED_FRAMES;

    trigCond.cfg.matchDestGlort  = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
    trigCond.param.destGlort     = FM10000_GLORT_MCAST;
    trigCond.param.destGlortMask = 0xFFFF;

    trigCond.cfg.rxPortset = floodInfo->dropMaskSet;

    trigCond.cfg.HAMask =
        FM_TRIGGER_HA_DROP_VLAN_EV |
        FM_TRIGGER_HA_DROP_STP_E;

    /**************************************************
     * Initialize trigger action.
     **************************************************/

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_TRAP;

    /**************************************************
     * Apply trigger configuration.
     **************************************************/

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_MCAST_DROP_TRAP,
                                  RULE_MCAST_DROP_TRAP,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_MCAST_DROP_TRAP,
                                     RULE_MCAST_DROP_TRAP,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    return err;

}   /* end ConfigDroppingTrigger2 */

#endif  /* end ENABLE_AUX_TRIGGERS */



/*****************************************************************************/
/** ConfigFloodingTrigger
 * \ingroup intSwitch
 *
 * \desc            Configures a single flooding control trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       desc points to the descriptor for the flooding trigger
 *                  to be configured.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigFloodingTrigger(fm_int sw, const triggerDesc * desc)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /**************************************************
     * Configure trigger condition.
     **************************************************/

    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    trigCond.cfg.matchFrameClassMask = desc->classMask;

    trigCond.cfg.matchRoutedMask = FM_TRIGGER_SWITCHED_FRAMES;

    trigCond.cfg.HAMask = FM_TRIGGER_HA_FORWARD_FLOOD;
    if (desc->group == FM10000_TRIGGER_GROUP_BCAST_FLOOD)
    {
        trigCond.cfg.HAMask |= FM_TRIGGER_HA_FORWARD_FID;
    }

    trigCond.cfg.matchDestGlort  = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
    trigCond.param.destGlort     = desc->glort;
    trigCond.param.destGlortMask = 0xffff;

    trigCond.cfg.rxPortset = *GET_PORTSET_PTR(floodInfo, desc);

    if ( (desc->classMask == FM_TRIGGER_FRAME_CLASS_MCAST) &&
         (floodInfo->trapAlwaysId >= 0) )
    {
        trigCond.cfg.matchFFU    = FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL;
        trigCond.param.ffuId     = floodInfo->trapAlwaysId;
        trigCond.param.ffuIdMask = floodInfo->trapAlwaysId;
    }

    /**************************************************
     * Configure trigger action.
     **************************************************/

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    switch (desc->trigType)
    {
        case TRIG_TYPE_TRAP:
            trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_TRAP;
            break;

        case TRIG_TYPE_LOG:
            trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_LOG;
            break;

        case TRIG_TYPE_DROP:
            trigAction.cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_DROP;
            trigAction.param.dropPortset = FM_PORT_SET_ALL;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (floodInfo->trapPri >= 0)
    {
        trigAction.cfg.switchPriAction = FM_TRIGGER_SWPRI_ACTION_REASSIGN;
        trigAction.param.newSwitchPri  = floodInfo->trapPri;
    }

    /**************************************************
     * Apply trigger configuration.
     **************************************************/

    err = fm10000SetTriggerAction(sw,
                                  desc->group,
                                  desc->rule,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetTriggerCondition(sw,
                                     desc->group,
                                     desc->rule,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    return err;

}   /* end ConfigFloodingTrigger */




#if (ENABLE_AUX_TRIGGERS)

/*****************************************************************************/
/** CreateAuxiliaryTriggers
 * \ingroup intSwitch
 *
 * \desc            Creates the auxiliary triggers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateAuxiliaryTriggers(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /**************************************************
     * Create auxiliary triggers.
     **************************************************/

    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_MCAST_MASK_DROP_TRAP,
                               RULE_MCAST_MASK_DROP_TRAP,
                               TRUE,
                               "mcastMaskDroppingTrapTrigger");
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_MCAST_DROP_TRAP,
                               RULE_MCAST_DROP_TRAP,
                               TRUE,
                               "mcastDroppingTrapTrigger");
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Create associated port sets.
     **************************************************/

    if (floodInfo->dropMaskSet == FM_PORT_SET_NONE)
    {
        err = fmCreatePortSetInt(sw, &floodInfo->dropMaskSet, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Assigned portset %d to dropMaskSet\n",
                     floodInfo->dropMaskSet);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end CreateAuxiliaryTriggers */

#endif  /* end ENABLE_AUX_TRIGGERS */




/*****************************************************************************/
/** CreateFloodingTrigger
 * \ingroup intSwitch
 *
 * \desc            Creates a single flooding control trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       desc points to the descriptor for the flooding trigger
 *                  to be created.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateFloodingTrigger(fm_int sw, const triggerDesc * desc)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_int *            portSet;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d descName=%s\n",
                 sw,
                 desc->descName);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;
    portSet   = GET_PORTSET_PTR(floodInfo, desc);

    /**************************************************
     * Create the trigger.
     **************************************************/

    err = fm10000CreateTrigger(sw,
                               desc->group,
                               desc->rule,
                               TRUE,
                               desc->trigName);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Create the associated port set.
     **************************************************/

    if (*portSet == FM_PORT_SET_NONE)
    {
        err = fmCreatePortSetInt(sw, portSet, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Assigned portset %d to %s\n",
                     *portSet,
                     desc->trigName);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end CreateFloodingTrigger */




/*****************************************************************************/
/** DbgDumpLogicalPort
 * \ingroup intSwitch
 *
 * \desc            Dumps information about the flood control subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DbgDumpLogicalPort(fm_int sw, fm_int port)
{
    const char *portName;
    fm_switch * switchPtr;
    fm_portmask destMask;
    fm_status   err;
    fm_uint64   rv64;
    fm_uint16   glort;

    switchPtr = GET_SWITCH_PTR(sw);

    portName = fmSpecialPortToText(port);

    err = switchPtr->ReadUINT64(sw, FM10000_MA_TABLE_CFG_2(0), &rv64);
    if (err != FM_OK)
    {
        goto ERROR;
    }
    
    switch (port)
    {
        case FM_PORT_BCAST:
            glort = FM_GET_FIELD64(rv64,
                                   FM10000_MA_TABLE_CFG_2,
                                   BroadcastGlort);
            break;

        case FM_PORT_MCAST:
            glort = FM_GET_FIELD64(rv64,
                                   FM10000_MA_TABLE_CFG_2,
                                   FloodMulticastGlort);
            break;

        case FM_PORT_FLOOD:
            glort = FM_GET_FIELD64(rv64,
                                   FM10000_MA_TABLE_CFG_2,
                                   FloodUnicastGlort);
            break;

        default:
            err = FM_ERR_INVALID_PORT;
            goto ERROR;
    }

    err = fm10000GetLogicalPortAttribute(sw,
                                         port,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);
    if (err != FM_OK)
    {
        goto ERROR;
    }

    FM_LOG_PRINT("%-8s  %5d  %04x   %04x:%04x:%04x\n",
                 portName,
                 port,
                 glort,
                 destMask.maskWord[1],
                 destMask.maskWord[0] >> 16,
                 destMask.maskWord[0] & 0xffff);

    return FM_OK;

ERROR:
    FM_LOG_PRINT("%s (%d): %s\n", portName, port, fmErrorMsg(err));
    return err;

}   /* end DbgDumpLogicalPort */




/*****************************************************************************/
/** DbgDumpPortSet
 * \ingroup intSwitch
 *
 * \desc            Dumps information about the flood control subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       portSet is the port set number.
 * 
 * \param[in]       portSetName is the port set name.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DbgDumpPortSet(fm_int  sw,
                                fm_int  portSet,
                                fm_text portSetName)
{
    fm_portmask portMask;
    fm_status   err;

    if (portSet == FM_PORT_SET_NONE)
    {
        FM_LOG_PRINT("%-16s  %5s\n", portSetName, "NONE");
        return FM_OK;
    }

    err = fmPortSetToPortMask(sw, portSet, &portMask);
    if (err != FM_OK)
    {
        FM_LOG_PRINT("%-16s  %5d    %s\n", portSetName, portSet, fmErrorMsg(err));
        return err;
    }

    FM_LOG_PRINT("%-16s  %5d    %04x:%04x:%04x\n",
                 portSetName,
                 portSet,
                 portMask.maskWord[1],
                 portMask.maskWord[0] >> 16,
                 portMask.maskWord[0] & 0xffff);

    return FM_OK;

}   /* end DbgDumpPortSet */




/*****************************************************************************/
/** fmBcastFloodingToText
 * \ingroup intSwitch
 *
 * \desc            Returns the textual representation of a broadcast
 *                  flooding type.
 *
 * \param[in]       floodType is the broadcast flooding type.
 *
 * \return          Textual representation of the flooding type.
 *
 *****************************************************************************/
static const char * fmBcastFloodingToText(fm_int floodType)
{

    switch (floodType)
    {
        case FM_BCAST_FWD:
            return "BCAST_FWD";

        case FM_BCAST_DISCARD:
            return "BCAST_DISCARD";

        case FM_BCAST_FWD_EXCPU:
            return "BCAST_FWD_EXCPU";

        case FM_BCAST_FLOODING_PER_PORT:
            return "BCAST_FLOODING_PER_PORT";

        default:
            return "UNKNOWN";
    }

}   /* end fmBcastFloodingToText */




/*****************************************************************************/
/** fmMcastFloodingToText
 * \ingroup intSwitch
 *
 * \desc            Returns the textual representation of a multicast
 *                  flooding type.
 *
 * \param[in]       floodType is the multicast flooding type.
 *
 * \return          Textual representation of the flooding type.
 *
 *****************************************************************************/
static const char * fmMcastFloodingToText(fm_int floodType)
{

    switch (floodType)
    {
        case FM_MCAST_FWD:
            return "MCAST_FWD";

        case FM_MCAST_DISCARD:
            return "MCAST_DISCARD";

        case FM_MCAST_FWD_EXCPU:
            return "MCAST_FWD_EXCPU";

        case FM_MCAST_FLOODING_PER_PORT:
            return "MCAST_FLOODING_PER_PORT";

        default:
            return "UNKNOWN";
    }

}   /* end fmMcastFloodingToText */




/*****************************************************************************/
/** fmUcastFloodingToText
 * \ingroup intSwitch
 *
 * \desc            Returns the textual representation of a unicast
 *                  flooding type.
 *
 * \param[in]       floodType is the unicast flooding type.
 *
 * \return          Textual representation of the flooding type.
 *
 *****************************************************************************/
static const char * fmUcastFloodingToText(fm_int floodType)
{

    switch (floodType)
    {
        case FM_UCAST_FWD:
            return "UCAST_FWD";

        case FM_UCAST_DISCARD:
            return "UCAST_DISCARD";

        case FM_UCAST_FWD_EXCPU:
            return "UCAST_FWD_EXCPU";

        case FM_UCAST_FLOODING_PER_PORT:
            return "UCAST_FLOODING_PER_PORT";

        default:
            return "UNKNOWN";
    }

}   /* end fmUcastFloodingToText */




/*****************************************************************************/
/** FreeFloodingTrigger
 * \ingroup intSwitch
 *
 * \desc            Frees the specified flooding trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       desc points to the descriptor for the flooding trigger
 *                  to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeFloodingTrigger(fm_int sw, const triggerDesc * desc)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_status           retVal;
    fm_status           err;

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    retVal = FM_OK;

    /**************************************************
     * Delete flooding trigger.
     **************************************************/

    err = FreeTrigger(sw, desc->group, desc->rule);
    FM_ERR_COMBINE(retVal, err);

    /**************************************************
     * Free associated port set.
     **************************************************/

    err = FreePortSet(sw, GET_PORTSET_PTR(floodInfo, desc));
    FM_ERR_COMBINE(retVal, err);

    return retVal;

}   /* end FreeFloodingTrigger */




/*****************************************************************************/
/** FreePortSet
 * \ingroup intSwitch
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
 * \ingroup intSwitch
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




#if (ENABLE_STATIC_TRIGGERS)

/*****************************************************************************/
/** InitConfigTriggers
 * \ingroup intSwitch
 *
 * \desc            Configures the triggers at initialization time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitConfigTriggers(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    err = FM_OK;

    if (floodInfo->initUcastFlooding)
    {
        /**************************************************
         * Configure the unicast flooding triggers.
         **************************************************/

        err = ConfigFloodingTrigger(sw, &ucastTrapDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigFloodingTrigger(sw, &ucastLogDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigFloodingTrigger(sw, &ucastDropDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }   /* end if (floodInfo->initUcastFlooding) */

    if (floodInfo->initMcastFlooding)
    {
        /**************************************************
         * Configure the multicast flooding triggers.
         **************************************************/

        err = ConfigFloodingTrigger(sw, &mcastTrapDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigFloodingTrigger(sw, &mcastLogDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigFloodingTrigger(sw, &mcastDropDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if (ENABLE_AUX_TRIGGERS)
        /**************************************************
         * Configure the dropping triggers.
         **************************************************/

        err = ConfigDroppingTrigger1(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigDroppingTrigger2(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#endif

    }   /* end if (floodInfo->initMcastFlooding) */

    if (floodInfo->initBcastFlooding)
    {
        /**************************************************
         * Configure the broadcast flooding triggers.
         **************************************************/

        err = ConfigFloodingTrigger(sw, &bcastTrapDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigFloodingTrigger(sw, &bcastLogDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = ConfigFloodingTrigger(sw, &bcastDropDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }   /* end if (floodInfo->initBcastFlooding) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end InitConfigTriggers */




/*****************************************************************************/
/** InitCreateTriggers
 * \ingroup intSwitch
 *
 * \desc            Creates the triggers at initialization time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitCreateTriggers(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    err = FM_OK;

    /**************************************************
     * Create unicast flooding triggers.
     **************************************************/

    if (floodInfo->initUcastFlooding)
    {
        err = CreateFloodingTrigger(sw, &ucastTrapDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = CreateFloodingTrigger(sw, &ucastLogDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = CreateFloodingTrigger(sw, &ucastDropDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }   /* end if (floodInfo->initUcastFlooding) */

    /**************************************************
     * Create multicast flooding triggers.
     **************************************************/

    if (floodInfo->initMcastFlooding)
    {
        err = CreateFloodingTrigger(sw, &mcastTrapDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = CreateFloodingTrigger(sw, &mcastLogDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = CreateFloodingTrigger(sw, &mcastDropDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if (ENABLE_AUX_TRIGGERS)
        err = CreateAuxiliaryTriggers(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#endif

    }   /* end if (floodInfo->initMcastFlooding) */

    /**************************************************
     * Create broadcast flooding triggers.
     **************************************************/

    if (floodInfo->initBcastFlooding)
    {
        err = CreateFloodingTrigger(sw, &bcastTrapDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = CreateFloodingTrigger(sw, &bcastLogDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = CreateFloodingTrigger(sw, &bcastDropDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }   /* end if (floodInfo->initBcastFlooding) */


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end InitCreateTriggers */

#endif  /* end ENABLE_STATIC_TRIGGERS */




/*****************************************************************************/
/** InitFloodStructures
 * \ingroup intSwitch
 *
 * \desc            Initializes the flooding control structures.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitFloodStructures(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /**************************************************
     * Initialize port set identifiers.
     **************************************************/

    floodInfo->ucastDropSet = FM_PORT_SET_NONE;
    floodInfo->ucastLogSet  = FM_PORT_SET_NONE;
    floodInfo->ucastTrapSet = FM_PORT_SET_NONE;

    floodInfo->mcastDropSet = FM_PORT_SET_NONE;
    floodInfo->mcastLogSet  = FM_PORT_SET_NONE;
    floodInfo->mcastTrapSet = FM_PORT_SET_NONE;

    floodInfo->bcastDropSet = FM_PORT_SET_NONE;
    floodInfo->bcastLogSet  = FM_PORT_SET_NONE;
    floodInfo->bcastTrapSet = FM_PORT_SET_NONE;

    floodInfo->dropMaskSet  = FM_PORT_SET_NONE;

    floodInfo->trapAlwaysId = -1;

    /**************************************************
     * Cache API properties.
     **************************************************/

    floodInfo->initUcastFlooding = fmGetBoolApiProperty(
        FM_AAK_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS,
        FM_AAD_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS);

    floodInfo->initMcastFlooding = fmGetBoolApiProperty(
        FM_AAK_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS,
        FM_AAD_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS);

    floodInfo->initBcastFlooding = fmGetBoolApiProperty(
        FM_AAK_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS,
        FM_AAD_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS);

    /* Should this be a switch attribute, rather than a property? */
    floodInfo->trapPri = fmGetIntApiProperty(
        FM_AAK_API_FM10000_FLOODING_TRAP_PRIORITY, 
        FM_AAD_API_FM10000_FLOODING_TRAP_PRIORITY);

    return FM_OK;

}   /* end InitFloodStructures */



/*****************************************************************************/
/** SetFloodingTriggerPort
 * \ingroup intSwitch
 *
 * \desc            Configures (adds or removes) a port in the specified
 *                  flooding trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       desc points to the descriptor for the flooding trigger
 *                  to be updated.
 *
 * \param[in]       port is the port to configure.
 * 
 * \param[in]       state is TRUE if the port should be added to the trigger,
 *                  or FALSE if it should be removed from the trigger.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetFloodingTriggerPort(fm_int               sw,
                                        const triggerDesc *  desc,
                                        fm_int               port,
                                        fm_bool              state)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_int *            portSet;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d trigger=%s port=%d state=%s\n",
                 sw,
                 desc->descName,
                 port,
                 FM_BOOLSTRING(state));

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    portSet   = GET_PORTSET_PTR(floodInfo, desc);

    /**************************************************
     * Create a dedicated port set if one does not 
     * already exist.
     **************************************************/

    if (*portSet == FM_PORT_SET_NONE)
    {
        /* If we're disabling a port for a trigger portset that
         * does not yet exist, treat the request as a no-op. */
        if (!state)
        {
            err = FM_OK;
            goto ABORT;
        }

        err = fmCreatePortSetInt(sw, portSet, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Assigned portset %d to %s\n",
                     *portSet,
                     desc->trigName);

    }   /* end if (*portSet == FM_PORT_SET_NONE) */

    /**************************************************
     * Add/remove the port in the port set.
     **************************************************/

    err = fmSetPortSetPortInt(sw, *portSet, port, state);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Get the trigger configuration.
     **************************************************/

    err = fm10000GetTrigger(sw,
                            desc->group,
                            desc->rule,
                            &trigCond,
                            &trigAction);

    if (err == FM_OK)
    {
        /**************************************************
         * The trigger exists. Update the trigger condition
         * with the updated port set.
         **************************************************/

        trigCond.cfg.rxPortset = *portSet;

        err = fm10000SetTriggerCondition(sw,
                                         desc->group,
                                         desc->rule,
                                         &trigCond,
                                         TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else if (err == FM_ERR_INVALID_TRIG)
    {
        /**************************************************
         * The trigger does not exist. Create it.
         **************************************************/

        err = CreateFloodingTrigger(sw, desc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /**************************************************
         * Configure the newly created trigger.
         **************************************************/

        err = ConfigFloodingTrigger(sw, desc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SetFloodingTriggerPort */




/*****************************************************************************/
/** SetFloodingTriggerPriority
 * \ingroup intSwitch
 *
 * \desc            Configures switch priority in the
 *                  specified flooding trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       desc points to the descriptor for the flooding trigger
 *                  to be updated.
 *
 * \param[in]       priority is the switch priority to configure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetFloodingTriggerPriority(fm_int               sw,
                                            const triggerDesc *  desc,
                                            fm_uint32            priority)
{
    fm10000_switch *    switchExt;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d trigger=%s priority=%d\n",
                 sw,
                 desc->descName,
                 priority);

    switchExt = GET_SWITCH_EXT(sw);

    /**************************************************
     * Get the trigger configuration.
     **************************************************/

    err = fm10000GetTrigger(sw,
                            desc->group,
                            desc->rule,
                            &trigCond,
                            &trigAction);

    if (err == FM_OK)
    {
        /**************************************************
         * The trigger exists. Update the trigger action
         **************************************************/

        trigAction.cfg.switchPriAction = FM_TRIGGER_SWPRI_ACTION_REASSIGN;

        if( (priority != FM_QOS_SWPRI_DEFAULT) && 
            (priority < FM10000_MAX_SWITCH_PRIORITIES) )
        {
            /**************************************************
             * New priority is valid set this value.
             **************************************************/

            trigAction.param.newSwitchPri = priority;

        }
        else
        {
            /**************************************************
             * New priority is not valid go back to the initial
             * configuration. First check cached api property.
             **************************************************/
             if((switchExt->floodInfo.trapPri >= 0) && 
                (switchExt->floodInfo.trapPri < FM10000_MAX_SWITCH_PRIORITIES))
             {

                 trigAction.param.newSwitchPri = switchExt->floodInfo.trapPri;

             }
             else
             {
                 /**************************************************
                  * Set trigger's init values
                  **************************************************/
                 trigAction.param.newSwitchPri = 0;
                 trigAction.cfg.switchPriAction = FM_TRIGGER_SWPRI_ACTION_ASIS;

             }
        }

        err = fm10000SetTriggerAction(sw,
                                      desc->group,
                                      desc->rule,
                                      &trigAction,
                                      TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SetFloodingTriggerPriority */

#if 0

/*****************************************************************************/
/** SetBcastFloodDestPort
 * \ingroup intSwitch
 *
 * \desc            Configures (adds or removes) a port in the broadcast
 *                  flooding GLORT.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port number to be added to or removed from
 *                  the glort.
 * 
 * \param[in]       state is TRUE if the port should be added to the glort,
 *                  or FALSE if it should be removed from the glort.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetBcastFloodDestPort(fm_int sw, fm_int port, fm_bool state)
{
    fm_portmask destMask;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d port=%d state=%s\n",
                 sw,
                 port,
                 FM_BOOLSTRING(state));

    /* Get destination mask for multicast flooding glort. */
    err = fm10000GetLogicalPortAttribute(sw,
                                         FM_PORT_BCAST,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Set state of port in destination mask. */
    err = fmSetPortInPortMask(sw, &destMask, port, state);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Write updated destination mask. */
    err = fm10000SetLogicalPortAttribute(sw,
                                         FM_PORT_BCAST,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SetBcastFloodDestPort */




/*****************************************************************************/
/** SetUcastFloodDestPort
 * \ingroup intSwitch
 *
 * \desc            Configures (adds or removes) a port in the unicast
 *                  flooding GLORT.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port number to be added to or removed from
 *                  the glort.
 * 
 * \param[in]       state is TRUE if the port should be added to the glort,
 *                  or FALSE if it should be removed from the glort.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetUcastFloodDestPort(fm_int sw, fm_int port, fm_bool state)
{
    fm_portmask destMask;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d port=%d state=%s\n",
                 sw,
                 port,
                 FM_BOOLSTRING(state));

    /* Get destination mask for unicast flooding glort. */
    err = fm10000GetLogicalPortAttribute(sw,
                                         FM_PORT_FLOOD,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Set state of port in destination mask. */
    err = fmSetPortInPortMask(sw, &destMask, port, state);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Write updated destination mask. */
    err = fm10000SetLogicalPortAttribute(sw,
                                         FM_PORT_FLOOD,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SetUcastFloodDestPort */

#endif




/*****************************************************************************/
/** UpdateFloodingTrigger
 * \ingroup intSwitch
 *
 * \desc            Updates the configuration of a single flooding control
 *                  trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       desc points to the descriptor for the flooding trigger
 *                  to be updated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateFloodingTrigger(fm_int sw, const triggerDesc * desc)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d descName=%s\n",
                 sw,
                 desc->descName);

    /**************************************************
     * Get the trigger configuration.
     **************************************************/

    err = fm10000GetTrigger(sw,
                            desc->group,
                            desc->rule,
                            &trigCond,
                            &trigAction);

    if (err == FM_OK)
    {
        /**************************************************
         * The trigger exists. Update its configuration.
         **************************************************/

        err = ConfigFloodingTrigger(sw, desc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    else if (err == FM_ERR_INVALID_TRIG)
    {
        err = FM_OK;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end UpdateFloodingTrigger */




/*****************************************************************************/
/** fm10000SetFloodDestPort
 * \ingroup intSwitch
 *
 * \desc            Configures (adds or removes) a port in the chosen
 *                  flooding GLORT
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port number to be added to or removed from
 *                  the glort.
 * 
 * \param[in]       state is TRUE if the port should be added to the glort,
 *                  or FALSE if it should be removed from the glort.
 *
 * \param[in]       floodPort is the logical port number on whose mask to operate
 *                  (e.g. ''FM_PORT_BCAST'', ''FM_PORT_MCAST'' or
 *                  ''FM_PORT_FLOOD'').
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetFloodDestPort(fm_int  sw,
                                  fm_int  port,
                                  fm_bool state,
                                  fm_int  floodPort)
{
    fm_portmask destMask;
    fm_status   err;
    fm_bool     portAlreadySet;
    fm_bool     mcastHNIFlooding;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d port=%d state=%s\n",
                 sw,
                 port,
                 FM_BOOLSTRING(state));

    mcastHNIFlooding = fmGetBoolApiProperty(FM_AAK_API_MULTICAST_HNI_FLOODING,
                                            FM_AAD_API_MULTICAST_HNI_FLOODING);

    /* Get destination mask for multicast flooding glort. */
    err = fm10000GetLogicalPortAttribute(sw,
                                         floodPort,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Check if this port is already set to a given state */
    portAlreadySet = ( (FM_PORTMASK_GET_BIT(&destMask, port)) == state );

    /* Set state of port in destination mask. */
    err = fmSetPortInPortMask(sw, &destMask, port, state);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Write updated destination mask. */
    err = fm10000SetLogicalPortAttribute(sw,
                                         floodPort,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);

    /* Update multicast HNI flooding groups */
    if ( (mcastHNIFlooding) && 
         (floodPort == FM_PORT_MCAST) &&
         (!portAlreadySet) )
    {
        err = fm10000UpdateMcastHNIFloodingGroups(sw,
                                                  port,
                                                  state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SetFloodDestPort */




/*****************************************************************************/
/** fm10000DbgDumpFlooding
 * \ingroup intSwitch
 *
 * \desc            Dumps information about the flood control subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpFlooding(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("bcastFlooding : %s\n",
                 fmBcastFloodingToText(switchExt->bcastFlooding));
    FM_LOG_PRINT("mcastFlooding : %s\n",
                 fmMcastFloodingToText(switchExt->mcastFlooding));
    FM_LOG_PRINT("ucastFlooding : %s\n",
                 fmUcastFloodingToText(switchExt->ucastFlooding));

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("initBcastFlooding   : %s\n",
                 FM_BOOLSTRING(floodInfo->initBcastFlooding));
    FM_LOG_PRINT("initMcastFlooding   : %s\n",
                 FM_BOOLSTRING(floodInfo->initMcastFlooding));
    FM_LOG_PRINT("initUcastFlooding   : %s\n",
                 FM_BOOLSTRING(floodInfo->initUcastFlooding));
    FM_LOG_PRINT("trapPri             : %d\n",
                 floodInfo->trapPri);
    FM_LOG_PRINT("trapAlwaysId        : %d\n",
                 floodInfo->trapAlwaysId);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Name      Port   Glort  DestMask\n");
    FM_LOG_PRINT("--------  -----  -----  --------------\n");

    DbgDumpLogicalPort(sw, FM_PORT_BCAST);
    DbgDumpLogicalPort(sw, FM_PORT_MCAST);
    DbgDumpLogicalPort(sw, FM_PORT_FLOOD);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Name              PortSet  DestMask\n");
    FM_LOG_PRINT("----------------  -------  --------------\n");

    DbgDumpPortSet(sw, floodInfo->bcastDropSet, "bcastDropSet");
    DbgDumpPortSet(sw, floodInfo->bcastLogSet,  "bcastLogSet");
    DbgDumpPortSet(sw, floodInfo->bcastTrapSet, "bcastTrapSet");

    DbgDumpPortSet(sw, floodInfo->mcastDropSet, "mcastDropSet");
    DbgDumpPortSet(sw, floodInfo->mcastLogSet,  "mcastLogSet");
    DbgDumpPortSet(sw, floodInfo->mcastTrapSet, "mcastTrapSet");

    DbgDumpPortSet(sw, floodInfo->ucastDropSet, "ucastDropSet");
    DbgDumpPortSet(sw, floodInfo->ucastLogSet,  "ucastLogSet");
    DbgDumpPortSet(sw, floodInfo->ucastTrapSet, "ucastTrapSet");

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fm10000DbgDumpFlooding */




/*****************************************************************************/
/** fm10000InitFlooding
 * \ingroup intSwitch
 *
 * \desc            Initializes the flooding control subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitFlooding(fm_int sw)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    err = InitFloodStructures(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if (ENABLE_STATIC_TRIGGERS)
    err = InitCreateTriggers(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = InitConfigTriggers(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#endif

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitFlooding */




/*****************************************************************************/
/** fm10000FreeFlooding
 * \ingroup intSwitch
 *
 * \desc            Frees the flooding control resources.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeFlooding(fm_int sw)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_status           retVal;
    fm_status           err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    retVal = FM_OK;

    /**************************************************
     * Free unicast triggers.
     **************************************************/

    err = FreeFloodingTrigger(sw, &ucastDropDesc);
    FM_ERR_COMBINE(retVal, err);

    err = FreeFloodingTrigger(sw, &ucastLogDesc);
    FM_ERR_COMBINE(retVal, err);

    err = FreeFloodingTrigger(sw, &ucastTrapDesc);
    FM_ERR_COMBINE(retVal, err);

    /**************************************************
     * Free multicast triggers.
     **************************************************/

    err = FreeFloodingTrigger(sw, &mcastDropDesc);
    FM_ERR_COMBINE(retVal, err);

    err = FreeFloodingTrigger(sw, &mcastLogDesc);
    FM_ERR_COMBINE(retVal, err);

    err = FreeFloodingTrigger(sw, &mcastTrapDesc);
    FM_ERR_COMBINE(retVal, err);

    /**************************************************
     * Free broadcast triggers.
     **************************************************/

    err = FreeFloodingTrigger(sw, &bcastDropDesc);
    FM_ERR_COMBINE(retVal, err);

    err = FreeFloodingTrigger(sw, &bcastLogDesc);
    FM_ERR_COMBINE(retVal, err);

    err = FreeFloodingTrigger(sw, &bcastTrapDesc);
    FM_ERR_COMBINE(retVal, err);

    /**************************************************
     * Free auxiliary triggers.
     **************************************************/

#if (ENABLE_AUX_TRIGGERS)
    err = FreeTrigger(sw,
                      FM10000_TRIGGER_GROUP_MCAST_MASK_DROP_TRAP,
                      RULE_MCAST_MASK_DROP_TRAP);
    FM_ERR_COMBINE(retVal, err);

    err = FreeTrigger(sw,
                      FM10000_TRIGGER_GROUP_MCAST_DROP_TRAP,
                      RULE_MCAST_DROP_TRAP);
    FM_ERR_COMBINE(retVal, err);

    err = FreePortSet(sw, &floodInfo->dropMaskSet);
    FM_ERR_COMBINE(retVal, err);
#endif

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, retVal);

}   /* end fm10000FreeFlooding */




/*****************************************************************************/
/** fm10000NotifyFloodingTrapAlwaysId
 * \ingroup intSwitch
 *
 * \desc            Notifies the flood control code that an FFU trigger
 *                  identifier has been assigned for the TRAP_ALWAYS
 *                  condition.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trapAlwaysId is the FFU trigger identifier assigned to
 *                  the TRAP_ALWAYS condition.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NotifyFloodingTrapAlwaysId(fm_int sw, fm_int trapAlwaysId)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d trapAlwaysId=%d\n", sw, trapAlwaysId);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    floodInfo->trapAlwaysId = trapAlwaysId;

    /* Update multicast triggers. */
    err = UpdateFloodingTrigger(sw, &mcastDropDesc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = UpdateFloodingTrigger(sw, &mcastTrapDesc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = UpdateFloodingTrigger(sw, &mcastLogDesc);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fm10000NotifyFloodingTrapAlwaysId */




/*****************************************************************************/
/** fm10000SetPortBcastFlooding
 * \ingroup intSwitch
 *
 * \desc            Applies the specified broadcast flooding attribute
 *                  to a port.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port number to be configured.
 * 
 * \param[in]       value is the broadcast flooding attribute to be applied.
 *                  See ''fm_bcastFlooding'' for possible values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetPortBcastFlooding(fm_int sw, fm_int port, fm_int value)
{
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_uint32           trapClassSwPriMap;
    fm_status           err;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    switch (value)
    {
        case FM_PORT_BCAST_FWD_EXCPU:
            /* Disable broadcast DROP. */
            err = SetFloodingTriggerPort(sw, &bcastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable broadcast TRAP. */
            err = SetFloodingTriggerPort(sw, &bcastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable broadcast LOG. */
            err = SetFloodingTriggerPort(sw, &bcastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_BCAST_FWD:
            /* Disable broadcast DROP. */
            err = SetFloodingTriggerPort(sw, &bcastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable broadcast TRAP. */
            err = SetFloodingTriggerPort(sw, &bcastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Enable broadcast LOG. */
            err = SetFloodingTriggerPort(sw, &bcastLogDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_BCAST_DISCARD:
            /* Enable broadcast DROP. */
            err = SetFloodingTriggerPort(sw, &bcastDropDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable broadcast TRAP. */
            err = SetFloodingTriggerPort(sw, &bcastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable broadcast LOG. */
            err = SetFloodingTriggerPort(sw, &bcastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_BCAST_TRAP:
            /* Disable broadcast DROP. */
            err = SetFloodingTriggerPort(sw, &bcastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Enable broadcast TRAP. */
            err = SetFloodingTriggerPort(sw, &bcastTrapDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable broadcast LOG. */
            err = SetFloodingTriggerPort(sw, &bcastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Update qos sw priority mapping */
            err = fm10000GetSwitchQOS(sw,
                                      FM_QOS_TRAP_CLASS_SWPRI_MAP,
                                      FM_QOS_TRAP_CLASS_BCAST_FLOODING,
                                      &trapClassSwPriMap);

            if ((err == FM_OK) && (trapClassSwPriMap != FM_QOS_SWPRI_DEFAULT))
            {
                err = SetFloodingTriggerPriority(sw,
                                                 &bcastTrapDesc,
                                                 trapClassSwPriMap);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
            else
            {
                /* Ignore error when the trap class was not found */
                err = FM_OK;
            }

            break;

        default:
            err = FM_ERR_INVALID_VALUE;
            break;

    }   /* end switch (value) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SetPortBcastFlooding */




/*****************************************************************************/
/** fm10000SetPortMcastFlooding
 * \ingroup intSwitch
 *
 * \desc            Applies the specified multicast flooding attribute
 *                  to a port.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port number to be configured.
 * 
 * \param[in]       value is the multicast flooding attribute to be applied.
 *                  See ''fm_mcastFlooding'' for possible values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetPortMcastFlooding(fm_int sw, fm_int port, fm_int value)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_uint32           trapClassSwPriMap;
    fm_status           err;

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    switch (value)
    {
        case FM_PORT_MCAST_FWD_EXCPU:
            /* Disable multicast DROP. */
            err = SetFloodingTriggerPort(sw, &mcastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable multicast TRAP. */
            err = SetFloodingTriggerPort(sw, &mcastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable multicast LOG. */
            err = SetFloodingTriggerPort(sw, &mcastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_MCAST_DISCARD:
            /* Enable multicast DROP. */
            err = SetFloodingTriggerPort(sw, &mcastDropDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable multicast TRAP. */
            err = SetFloodingTriggerPort(sw, &mcastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable multicast LOG. */
            err = SetFloodingTriggerPort(sw, &mcastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_MCAST_TRAP:
            /* Disable multicast DROP. */
            err = SetFloodingTriggerPort(sw, &mcastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Enable multicast TRAP. */
            err = SetFloodingTriggerPort(sw, &mcastTrapDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable multicast LOG. */
            err = SetFloodingTriggerPort(sw, &mcastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Update qos sw priority mapping */
            err = fm10000GetSwitchQOS(sw,
                                      FM_QOS_TRAP_CLASS_SWPRI_MAP,
                                      FM_QOS_TRAP_CLASS_MCAST_FLOODING,
                                      &trapClassSwPriMap);

            if ((err == FM_OK) && (trapClassSwPriMap != FM_QOS_SWPRI_DEFAULT))
            {
                err = SetFloodingTriggerPriority(sw,
                                                 &mcastTrapDesc,
                                                 trapClassSwPriMap);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
            else
            {
                /* Ignore error when the trap class was not found */
                err = FM_OK;
            }

            break;

        case FM_PORT_MCAST_FWD:
            /* Disable multicast DROP. */
            err = SetFloodingTriggerPort(sw, &mcastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable multicast TRAP. */
            err = SetFloodingTriggerPort(sw, &mcastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Enable multicast LOG. */
            err = SetFloodingTriggerPort(sw, &mcastLogDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        default:
            err = FM_ERR_INVALID_VALUE;
            break;

    }   /* end switch (value) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SetPortMcastFlooding */




/*****************************************************************************/
/** fm10000SetPortUcastFlooding
 * \ingroup intSwitch
 *
 * \desc            Applies the specified unicast flooding attribute
 *                  to a port.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port number to be configured.
 * 
 * \param[in]       value is the unicast flooding attribute to be applied.
 *                  See ''fm_ucastFlooding'' for possible values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetPortUcastFlooding(fm_int sw, fm_int port, fm_int value)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_uint32           trapClassSwPriMap;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d port=%d value=%d\n",
                 sw,
                 port,
                 value);

    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    switch (value)
    {
        case FM_PORT_UCAST_FWD_EXCPU:
            /* Disable unicast DROP. */
            err = SetFloodingTriggerPort(sw, &ucastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable unicast TRAP. */
            err = SetFloodingTriggerPort(sw, &ucastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable unicast LOG. */
            err = SetFloodingTriggerPort(sw, &ucastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_UCAST_DISCARD:
            /* Enable unicast DROP. */
            err = SetFloodingTriggerPort(sw, &ucastDropDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable unicast TRAP. */
            err = SetFloodingTriggerPort(sw, &ucastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable unicast LOG. */
            err = SetFloodingTriggerPort(sw, &ucastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        case FM_PORT_UCAST_TRAP:
            /* Disable unicast DROP. */
            err = SetFloodingTriggerPort(sw, &ucastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Enable unicast TRAP. */
            err = SetFloodingTriggerPort(sw, &ucastTrapDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable unicast LOG. */
            err = SetFloodingTriggerPort(sw, &ucastLogDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Update qos sw priority mapping */
            err = fm10000GetSwitchQOS(sw,
                                      FM_QOS_TRAP_CLASS_SWPRI_MAP,
                                      FM_QOS_TRAP_CLASS_UCAST_FLOODING,
                                      &trapClassSwPriMap);
            if ((err == FM_OK) && (trapClassSwPriMap != FM_QOS_SWPRI_DEFAULT))
            {
                err = SetFloodingTriggerPriority(sw,
                                                 &ucastTrapDesc,
                                                 trapClassSwPriMap);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
            else
            {
                /* Ignore error when the trap class was not found */
                err = FM_OK;
            }

            break;

        case FM_PORT_UCAST_FWD:
            /* Disable unicast DROP. */
            err = SetFloodingTriggerPort(sw, &ucastDropDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Disable unicast TRAP. */
            err = SetFloodingTriggerPort(sw, &ucastTrapDesc, port, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Enable unicast LOG. */
            err = SetFloodingTriggerPort(sw, &ucastLogDesc, port, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            break;

        default:
            err = FM_ERR_INVALID_VALUE;
            break;

    }   /* end switch (value) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SetPortUcastFlooding */



/*****************************************************************************/
/** fm10000SetTrapPriorityUcastFlooding
 * \ingroup intSwitch
 *
 * \desc            Configures the specified switch priority for unicast 
 *                  flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       priority is the switch priority to be configured.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetTrapPriorityUcastFlooding(fm_int sw, fm_uint32 priority)
{
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d priority=%d\n",
                 sw,
                 priority);

    err = SetFloodingTriggerPriority(sw, &ucastTrapDesc, priority);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

} /* end fm10000SetTrapPriorityUcastFlooding */




/*****************************************************************************/
/** fm10000SetTrapPriorityMcastFlooding
 * \ingroup intSwitch
 *
 * \desc            Configures the specified switch priority for multicast 
 *                  flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       priority is the switch priority to be configured.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetTrapPriorityMcastFlooding(fm_int sw, fm_uint32 priority)
{
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d priority=%d\n",
                 sw,
                 priority);

    err = SetFloodingTriggerPriority(sw, &mcastTrapDesc, priority);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

} /* end fm10000SetTrapPriorityMcastFlooding */




/*****************************************************************************/
/** fm10000SetTrapPriorityBcastFlooding
 * \ingroup intSwitch
 *
 * \desc            Configures the specified switch priority for broadcast 
 *                  flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       priority is the switch priority to be configured.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetTrapPriorityBcastFlooding(fm_int sw, fm_uint32 priority)
{
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d priority=%d\n",
                 sw,
                 priority);

    err = SetFloodingTriggerPriority(sw, &bcastTrapDesc, priority);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

} /* end fm10000SetTrapPriorityBcastFlooding */




/*****************************************************************************/
/** fm10000GetStateBcastTrapFlooding
 * \ingroup intSwitch
 *
 * \desc            Get state of trapping broadcast flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      enabled points to a caller allocated memory where broadcast 
 *                  flooding trapping state will be set (TRUE if the broadcast 
 *                  flooding trapping is enabled).
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetStateBcastTrapFlooding(fm_int sw, fm_bool * enabled)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_int *            portSet;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d\n",
                 sw);

    /* Check input argument */
    if(enabled == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    err = FM_OK;

    /* Get flood info */
    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /* Based on the porset for bcast trap trigger set requested state */
    portSet  = GET_PORTSET_PTR(floodInfo, &bcastTrapDesc);
    *enabled = FALSE;

    if (*portSet != FM_PORT_SET_NONE)
    {
        /* portSet configured - trap flooding enabled */
        *enabled = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

} /* end fm10000GetStateBcastTrapFlooding */




/*****************************************************************************/
/** fm10000GetStateMcastTrapFlooding
 * \ingroup intSwitch
 *
 * \desc            Get state of trapping multicast flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      enabled points to a caller allocated memory where multicast 
 *                  flooding trapping state will be set (TRUE if the multicast 
 *                  flooding trapping is enabled).
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetStateMcastTrapFlooding(fm_int sw, fm_bool * enabled)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_int *            portSet;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d\n",
                 sw);

    /* Check input argument */
    if(enabled == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    err       = FM_OK;

    /* Get flood info */
    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /* Based on the porset for mcast trap trigger set requested state */
    portSet   = GET_PORTSET_PTR(floodInfo, &mcastTrapDesc);
    *enabled  = FALSE;

    if (*portSet != FM_PORT_SET_NONE)
    {
        /* portSet configured - trap flooding enabled */
        *enabled  = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
} /* end fm10000GetStateMcastTrapFlooding */



/*****************************************************************************/
/** fm10000GetStateUcastTrapFlooding
 * \ingroup intSwitch
 *
 * \desc            Get the state of trapping mcast flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      enabled points to a caller allocated memory where unicast 
 *                  flooding trapping state will be set (TRUE if the unicast 
 *                  flooding trapping is enabled).
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetStateUcastTrapFlooding(fm_int sw, fm_bool * enabled)
{
    fm10000_switch *    switchExt;
    fm10000_floodInfo * floodInfo;
    fm_int *            portSet;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d\n",
                 sw);

    /* Check input argument */
    if(enabled == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    err       = FM_OK;

    /* Get flood info */
    switchExt = GET_SWITCH_EXT(sw);
    floodInfo = &switchExt->floodInfo;

    /* Based on the porset for ucast trap trigger set requested state */
    portSet   = GET_PORTSET_PTR(floodInfo, &ucastTrapDesc);
    *enabled  = FALSE;

    if (*portSet != FM_PORT_SET_NONE)
    {
        /* portSet configured - trap flooding enabled */
        *enabled  = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
} /* end fm10000GetStateUcastTrapFlooding */

