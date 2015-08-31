/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:          fm10000_api_mac_security.c            
 * Creation Date: November 13, 2013
 * Description:   FM10000 MAC security functions.
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

#define FM_LOG_CAT_SECURITY     FM_LOG_CAT_ADDR


/* Rule numbers for the triggers in the MAC Security group. */
enum
{
    RULE_MAC_MOVE_TRAP = 10,

    RULE_PORT_MISS_DROP = 20,
    RULE_PORT_MISS_TRAP = 30,

    RULE_PORT_MOVE_DROP = 40,
    RULE_PORT_MOVE_TRAP = 50,
};

enum
{
    TRIG_TYPE_SECURE,
    TRIG_TYPE_DROP,
    TRIG_TYPE_TRAP,
};


/**************************************************
 * Port set descriptor.
 **************************************************/
typedef struct
{
    /* Short name, used for log messages. */
    fm_text     descName;

    /* Offset of the variable containing the port set identifier
     * in the security information structure. */
    fm_uint     portSetOff;

} portSetDesc;


/**************************************************
 * Trigger descriptor.
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

    /* Offset of the associated port set identifier.*/
    fm_uint     portSetOff;

    /** Offset of the associated rate limiter identifier. */
    fm_uint     rateLimitOff;

} triggerDesc;


#define GET_PORTSET_PTR(basePtr, desc)    \
    ( (fm_int *) ( ( (char *) (basePtr) ) + (desc)->portSetOff ) )

#define GET_LIMITER_PTR(basePtr, desc)    \
    ( (fm_int *) ( ( (char *) (basePtr) ) + (desc)->rateLimitOff ) )


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/**************************************************
 * Port set descriptors.
 **************************************************/

static const portSetDesc dropSetDesc =
{
    .descName   = "dropPortSet",
    .portSetOff = offsetof(fm10000_securityInfo, dropPortSet),
};

static const portSetDesc trapSetDesc =
{
    .descName   = "trapPortSet",
    .portSetOff = offsetof(fm10000_securityInfo, trapPortSet),
};


/**************************************************
 * Trigger descriptors.
 **************************************************/

/** 
 * Traps frames with secure SMACs moving to any port when 
 * FM_MAC_TABLE_SECURITY_ACTION is EVENT or TRAP. 
 *  
 * Not used when FM_MAC_TABLE_SECURITY_ACTION is DROP, since dropping is 
 * the normal behavior of secure MA Table entries. 
 */
static const triggerDesc macMoveTrapDesc =
{
    .descName       = "TrMacMoveTrap",
    .trigName       = "SecureMacTrapTrigger",
    .group          = FM10000_TRIGGER_GROUP_SECURITY,
    .rule           = RULE_MAC_MOVE_TRAP,
    .portSetOff     = offsetof(fm10000_securityInfo, securePortSet),
    .rateLimitOff   = offsetof(fm10000_securityInfo, macMoveLimiter),
};

/** 
 * Traps frames with unknown SMACs received on a secure port whose 
 * FM_PORT_SECURITY_ACTION is TRAP or EVENT. 
 */
static const triggerDesc portMissTrapDesc =
{
    .descName       = "TrPortMissTrap",
    .trigName       = "NewMacTrapTrigger",
    .group          = FM10000_TRIGGER_GROUP_SECURITY,
    .rule           = RULE_PORT_MISS_TRAP,
    .portSetOff     = offsetof(fm10000_securityInfo, trapPortSet),
    .rateLimitOff   = offsetof(fm10000_securityInfo, newMacLimiter),
};

/**
 * Drops frames with unknown SMACs received on a secure port whose 
 * FM_PORT_SECURITY_ACTION is DROP. 
 */
static const triggerDesc portMissDropDesc =
{
    .descName       = "TrPortMissDrop",
    .trigName       = "NewMacDropTrigger",
    .group          = FM10000_TRIGGER_GROUP_SECURITY,
    .rule           = RULE_PORT_MISS_DROP,
    .portSetOff     = offsetof(fm10000_securityInfo, dropPortSet),
    .rateLimitOff   = offsetof(fm10000_securityInfo, noRateLimiter),
};

/**
 * Traps frames with non-secure SMACs moving to a secure port whose 
 * FM_PORT_SECURITY_ACTION is TRAP or EVENT.
 */
static const triggerDesc portMoveTrapDesc =
{
    .descName       = "TrPortMoveTrap",
    .trigName       = "NonSecureMacTrapTrigger",
    .group          = FM10000_TRIGGER_GROUP_SECURITY,
    .rule           = RULE_PORT_MOVE_TRAP,
    .portSetOff     = offsetof(fm10000_securityInfo, trapPortSet),
    .rateLimitOff   = offsetof(fm10000_securityInfo, macMoveLimiter),
};

/**
 * Traps frames with non-secure SMACs moving to a secure port whose 
 * FM_PORT_SECURITY_ACTION is DROP.
 */
static const triggerDesc portMoveDropDesc =
{
    .descName       = "TrPortMoveDrop",
    .trigName       = "NonSecureMacDropTrigger",
    .group          = FM10000_TRIGGER_GROUP_SECURITY,
    .rule           = RULE_PORT_MOVE_DROP,
    .portSetOff     = offsetof(fm10000_securityInfo, dropPortSet),
    .rateLimitOff   = offsetof(fm10000_securityInfo, noRateLimiter),
};

static const triggerDesc * triggerDescTable[] =
{
    &macMoveTrapDesc,
    &portMissDropDesc,
    &portMissTrapDesc,
    &portMoveDropDesc,
    &portMoveTrapDesc
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** ConfigTrigger
 * \ingroup intSwitch
 *
 * \desc            Configures a single MAC security trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trigDesc points to the descriptor for the trigger
 *                  to be configured.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigTrigger(fm_int sw, const triggerDesc * trigDesc)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_triggerCondition     trigCond;
    fm_triggerAction        trigAction;
    fm_int                  portSet;
    fm_uint32               rateLimiter;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY,
                 "sw=%d desc=%s\n",
                 sw,
                 trigDesc->descName);

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    /* Get Rx port set identifier. */
    portSet = *GET_PORTSET_PTR(secInfo, trigDesc);

    /* Get rate limiter identifier. */
    rateLimiter = *GET_LIMITER_PTR(secInfo, trigDesc);

    /**************************************************
     * Get trigger configuration.
     **************************************************/

    err = fm10000GetTrigger(sw,
                            trigDesc->group,
                            trigDesc->rule,
                            &trigCond,
                            &trigAction);

    if (err == FM_OK)
    {
        /**************************************************
         * The trigger exists. Update the trigger condition
         * with the appropriate port set.
         **************************************************/

        trigCond.cfg.rxPortset = portSet;

        FM_LOG_DEBUG(FM_LOG_CAT_SECURITY,
                     "Updating %s (%d,%d)\n",
                     trigDesc->trigName,
                     trigDesc->group,
                     trigDesc->rule);
    }
    else if (err == FM_ERR_INVALID_TRIG)
    {
        /**************************************************
         * The trigger does not exist. If we're disabling 
         * the trigger and it does not exist, treat the 
         * request as a no-op. 
         **************************************************/

        if (portSet == FM_PORT_SET_NONE)
        {
            err = FM_OK;
            goto ABORT;
        }

        /**************************************************
         * Create the trigger.
         **************************************************/

        FM_LOG_DEBUG(FM_LOG_CAT_SECURITY,
                     "Creating %s (%d,%d)\n",
                     trigDesc->trigName,
                     trigDesc->group,
                     trigDesc->rule);

        err = fm10000CreateTrigger(sw,
                                   trigDesc->group,
                                   trigDesc->rule,
                                   TRUE,
                                   trigDesc->trigName);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /**************************************************
         * Configure trigger condition.
         **************************************************/

        err = fmInitTriggerCondition(sw, &trigCond);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);

        /* Apply trigger to selected ports. */
        trigCond.cfg.rxPortset = portSet;

        switch (trigDesc->rule)
        {
            case RULE_MAC_MOVE_TRAP:
                /* Match station move (SMAC hit). */
                trigCond.cfg.matchHitSA = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;

                /* Match frames dropped to MAC security violation. */
                trigCond.cfg.HAMask = FM_TRIGGER_HA_DROP_PORT_SV;
                break;

            case RULE_PORT_MISS_DROP:
            case RULE_PORT_MISS_TRAP:
                /* New source address (SMAC miss). */
                trigCond.cfg.matchHitSA = FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL;
                break;

            case RULE_PORT_MOVE_DROP:
            case RULE_PORT_MOVE_TRAP:
                /* Match station move (SMAC hit). */
                trigCond.cfg.matchHitSA = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;

                /* Match addresses that have trigger identifier T3. */
                trigCond.cfg.matchSA = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
                trigCond.param.saId  = secInfo->triggerId3;
                break;

            default:
                break;

        }   /* end switch (trigDesc->rule) */

        /**************************************************
         * Configure trigger action.
         **************************************************/

        err = fmInitTriggerAction(sw, &trigAction);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);

        switch (trigDesc->rule)
        {
            case RULE_MAC_MOVE_TRAP:
                /* Trap frame to CPU. */
                trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_TRAP;
                break;

            case RULE_PORT_MISS_DROP:
            case RULE_PORT_MOVE_DROP:
                /* Drop the frame. */
                trigAction.cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_DROP;
                trigAction.param.dropPortset = FM_PORT_SET_ALL;

                /* Suppress learning event. */
                trigAction.cfg.learningAction = FM_TRIGGER_LEARN_ACTION_CANCEL;
                break;

            case RULE_PORT_MISS_TRAP:
            case RULE_PORT_MOVE_TRAP:
                /* Trap frame to CPU. */
                trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_TRAP;

                /* Suppress learning event. */
                trigAction.cfg.learningAction = FM_TRIGGER_LEARN_ACTION_CANCEL;
                break;

        }   /* end switch (trigDesc->rule) */
    }
    else
    {
        /**************************************************
         * Unexpected error getting trigger.
         **************************************************/

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);
    }

    /**************************************************
     * Configure rate limiter.
     **************************************************/

#if 0
    FM_LOG_DEBUG(FM_LOG_CAT_SECURITY,
                 "rateLimiter=%u portSetCount=%d\n",
                 rateLimiter,
                 fmGetPortSetCountInt(sw, portSet));
#endif

    if (rateLimiter != FM10000_INVALID_RATE_LIMITER_ID &&
        !fmIsPortSetEmpty(sw, portSet))
    {
        trigAction.cfg.rateLimitAction = FM_TRIGGER_RATELIMIT_ACTION_RATELIMIT;
        trigAction.param.rateLimitNum  = rateLimiter;
    }
    else
    {
        trigAction.cfg.rateLimitAction = FM_TRIGGER_RATELIMIT_ACTION_ASIS;
    }

    /**************************************************
     * Apply configuration.
     **************************************************/

    err = fm10000SetTriggerAction(sw,
                                  trigDesc->group,
                                  trigDesc->rule,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);

    err = fm10000SetTriggerCondition(sw,
                                     trigDesc->group,
                                     trigDesc->rule,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, err);

}   /* end ConfigTrigger */




/*****************************************************************************/
/** FreePortSet
 * \ingroup intSecurity
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

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                     "Error deleting portSet (%d): %s\n",
                     *portSet,
                     fmErrorMsg(err));
    }

    *portSet = FM_PORT_SET_NONE;

    return err;

}   /* end FreePortSet */




/*****************************************************************************/
/** FreeTrigger
 * \ingroup intSecurity
 *
 * \desc            Frees a single MAC security trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trigDesc points to the descriptor for the trigger
 *                  to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeTrigger(fm_int sw, const triggerDesc * trigDesc)
{
    fm_status   err;

    err = fm10000DeleteTrigger(sw,
                               trigDesc->group,
                               trigDesc->rule,
                               TRUE);

    if (err == FM_ERR_INVALID_TRIG)
    {
        return FM_OK;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                     "Error deleting %s (%d,%d): %s\n",
                     trigDesc->trigName,
                     trigDesc->group,
                     trigDesc->rule,
                     fmErrorMsg(err));
    }

    return err;

}   /* end FreeTrigger */




/*****************************************************************************/
/** FreeTriggerId
 * \ingroup intSecurity
 *
 * \desc            Frees the specified MAC address trigger identifier.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   triggerId points to a location containing the trigger
 *                  identifier to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeTriggerId(fm_int sw, fm_uint32 * triggerId)
{
    fm_status   err;

    if (triggerId == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (*triggerId == FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX)
    {
        return FM_OK;
    }

    err = fm10000FreeTriggerResource(sw,
                                     FM_TRIGGER_RES_MAC_ADDR,
                                     *triggerId,
                                     TRUE);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                     "Error freeing trigId (%u): %s\n",
                     *triggerId,
                     fmErrorMsg(err));
    }

    *triggerId = FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX;

    return err;

}   /* end FreeTriggerId */




/*****************************************************************************/
/** GetTriggerCounter
 * \ingroup intSecurity
 *
 * \desc            Returns a MAC trigger counter.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trigDesc points to the descriptor for the trigger.
 * 
 * \param[in]       counter points to the location to receive the count.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetTriggerCounter(fm_int              sw,
                                   const triggerDesc * trigDesc,
                                   fm_uint64 *         counter)
{
    fm_status   err;

    err = fm10000GetTriggerAttribute(sw,
                                     trigDesc->group,
                                     trigDesc->rule,
                                     FM_TRIGGER_ATTR_COUNTER,
                                     counter);

    if (err == FM_ERR_INVALID_TRIG)
    {
        *counter = 0;
        return FM_OK;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                     "Error getting %s: %s\n",
                     trigDesc->trigName,
                     fmErrorMsg(err));
    }

    return err;

}   /* end GetTriggerCounter */




/*****************************************************************************/
/** ReportStationMove
 * \ingroup intSecurity
 *
 * \desc            Creates UPDATE event entries for a station move.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       oldEntry points to an internal structure describing
 *                  the original MA Table entry.
 * 
 * \param[in]       newEntry points to the internal structure describing
 *                  the updated MA Table entry.
 * 
 * \param[in]       index is the MA Table index.
 *
 * \param[in,out]   numUpdates points to where the total number of updates
 *                  in the current event is stored. Will be incremented
 *                  or reset by this function.
 *
 * \param[in,out]   outEvent points to the pointer to the event buffer.
 *                  Will be reset to NULL if event is sent to the target task.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void ReportStationMove(fm_int                    sw,
                              fm_internalMacAddrEntry * oldEntry,
                              fm_internalMacAddrEntry * newEntry,
                              fm_uint32                 index,
                              fm_uint32 *               numUpdates,
                              fm_event **               outEvent)
{

    /* Add an AGED update for the old station address. */
    fmGenerateUpdateForEvent(sw,
                             &fmRootApi->eventThread,
                             FM_EVENT_ENTRY_AGED,
                             FM_MAC_REASON_LEARN_CHANGED,
                             index,
                             oldEntry,
                             numUpdates,
                             outEvent);

    fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_AGED, 1);

    /* Add a LEARNED update for new station address. */
    fmGenerateUpdateForEvent(sw,
                             &fmRootApi->eventThread,
                             FM_EVENT_ENTRY_LEARNED,
                             FM_MAC_REASON_LEARN_EVENT,
                             index,
                             newEntry,
                             numUpdates,
                             outEvent);

    fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_LEARNED, 1);
    fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_PORT_CHANGED, 1);

}   /* end ReportStationMove */




/*****************************************************************************/
/** ResetTriggerCounter
 * \ingroup intSecurity
 *
 * \desc            Resets a MAC trigger counter.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trigDesc points to the descriptor for the trigger.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ResetTriggerCounter(fm_int sw, const triggerDesc * trigDesc)
{
    fm_uint64   counter = 0;
    fm_status   err;

    err = fm10000SetTriggerAttribute(sw,
                                     trigDesc->group,
                                     trigDesc->rule,
                                     FM_TRIGGER_ATTR_COUNTER,
                                     &counter,
                                     TRUE);

    if (err == FM_ERR_INVALID_TRIG)
    {
        return FM_OK;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                     "Error resetting %s: %s\n",
                     trigDesc->trigName,
                     fmErrorMsg(err));
    }

    return err;

}   /* end ResetTriggerCounter */




/*****************************************************************************/
/** SetTriggerPort
 * \ingroup intAddr
 *
 * \desc            Adds or removes a port in a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       desc points to the descriptor for the port set to be
 *                  updated.
 *
 * \param[in]       port is the port number to be updated in the port set.
 * 
 * \param[in]       state is TRUE if the port is to be added to the set,
 *                  or FALSE if it should be removed from the set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetTriggerPort(fm_int              sw,
                                const portSetDesc * desc,
                                fm_int              port,
                                fm_bool             state)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_int *                portSetPtr;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d desc=%s port=%d state=%s\n",
                 sw,
                 desc->descName,
                 port,
                 FM_BOOLSTRING(state));

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    portSetPtr = GET_PORTSET_PTR(secInfo, desc);

    /**************************************************
     * Create port set if it does not already exist.
     **************************************************/

    if (*portSetPtr == FM_PORT_SET_NONE)
    {
        /* If we're disabling a port for a trigger portset that
         * does not yet exist, treat the request as a no-op. */
        if (!state)
        {
            err = FM_OK;
            goto ABORT;
        }

        err = fmCreatePortSetInt(sw, portSetPtr, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Assigned portset %d to %s\n",
                     *portSetPtr,
                     desc->descName);

    }   /* end if (*portSet == FM_PORT_SET_NONE) */

    /**************************************************
     * Update the port in the port set.
     **************************************************/

    err = fmSetPortSetPortInt(sw, *portSetPtr, port, state);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SetTriggerPort */




/*****************************************************************************/
/** UpdateMacEntry
 * \ingroup intAddr
 *
 * \desc            Updates an MA Table entry as the result of a station move.
 *                  Clears squelch state if it is set.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       index is the MA Table index at which the entry is to be
 *                  written.
 *
 * \param[in,out]   entry points to the MA Table entry structure to be
 *                  updated and written to the hardware.
 * 
 * \param[in]       newPort is the logical port number to which the source
 *                  MAC address is moving.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateMacEntry(fm_int                    sw,
                                fm_uint32                 index,
                                fm_internalMacAddrEntry * entry,
                                fm_int                    newPort)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_status               status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d index=%d entry=%p\n",
                         sw,
                         index,
                         (void *) entry);

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    /* Update cache entry to new configuration. */
    entry->port   = newPort;
    entry->secure = FM_IS_ADDR_TYPE_SECURE(entry->addrType);

    switch (entry->addrType)
    {
        case FM_ADDRESS_STATIC:
        case FM_ADDRESS_DYNAMIC:
            entry->trigger = secInfo->triggerId3;
            break;

        default:
            entry->trigger = 0;
            break;

    }   /* end switch (entry->addrType) */

    /* Write updated MA Table entry. */
    status = fm10000WriteEntryAtIndex(sw, index, entry);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, status);

}   /* end UpdateMacEntry */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000AssignMacTrigger
 * \ingroup intSecurity
 *
 * \desc            Assigns the trigger to be used by this MAC entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to an ''fm_macAddressEntry'' structure that
 *                  describes the MA Table entry to be added.
 * 
 * \param[out]      trigger points to the location to receive the trigger
 *                  identifier to associate with this MA Table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AssignMacTrigger(fm_int               sw,
                                  fm_macAddressEntry * entry,
                                  fm_uint32 *          trigger)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_status               err;

    switchExt   = GET_SWITCH_EXT(sw);
    secInfo     = &switchExt->securityInfo;
    err         = FM_OK;

    switch (entry->type)
    {
        case FM_ADDRESS_STATIC:
        case FM_ADDRESS_DYNAMIC:
            *trigger = secInfo->triggerId3;
            break;

        default:
            *trigger = 0;
            break;

    }   /* end switch (entry->type) */

    return err;

}   /* end fm10000AssignMacTrigger */




/*****************************************************************************/
/** fm10000CheckSecurityViolation
 * \ingroup intSecurity
 *
 * \desc            Performs a security check on a trapped packet.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       trapCode is the trap code that indicates the reason
 *                  the packet was trapped.
 * 
 * \param[out]      svAction points to the location to receive the
 *                  action to be taken on the received packet. See
 *                  ''fm10000_secViolAction'' for possible values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CheckSecurityViolation(fm_int   sw,
                                        fm_int   port,
                                        fm_int   trapCode,
                                        fm_int * svAction)
{
    fm10000_switch *    switchExt;
    fm_status           err;
    fm_int              group;
    fm_int              rule;
    fm_uint32           securityAction;
    fm10000_svAction    violAction;
    fm_trackingCounterIndex diagCounter;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY,
                 "sw=%d port=%d trapCode=%d\n",
                 sw,
                 port,
                 trapCode);

    switchExt = GET_SWITCH_EXT(sw);

    violAction  = FM10000_SV_NO_ACTION;
    diagCounter = FM_CTR_INVALID;

    err = fm10000GetTriggerFromTrapCode(sw, trapCode, &group, &rule);
    if (err != FM_OK || group != FM10000_TRIGGER_GROUP_SECURITY)
    {
        err = FM_OK;
        goto ABORT;
    }

    /**************************************************
     * Determine trap action.
     **************************************************/

    switch (rule)
    {
        case RULE_MAC_MOVE_TRAP:
            switch (switchExt->macSecurityAction)
            {
                case FM_MAC_SECURITY_ACTION_EVENT:
                    violAction  = FM10000_SV_SECURE_SMAC_EVENT;
                    diagCounter = FM_CTR_SECURITY_SECURE_SMAC_EVENTS;
                    break;

                case FM_MAC_SECURITY_ACTION_TRAP:
                    violAction  = FM10000_SV_SECURE_SMAC_TRAP;
                    diagCounter = FM_CTR_SECURITY_SECURE_SMAC_TRAPS;
                    break;

                case FM_MAC_SECURITY_ACTION_DROP:
                    violAction  = FM10000_SV_SECURE_SMAC_DROP;
                    /* Fall through to default. */

                default:
                    FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                                 "Unexpected security trap: "
                                 "rule=%d macAction=%d\n",
                                 rule,
                                 switchExt->macSecurityAction);
                    err = FM_ERR_INVALID_VALUE;
                    break;
            }
            break;

        case RULE_PORT_MISS_DROP:
        case RULE_PORT_MOVE_DROP:
            FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                         "Unexpected security trap: rule=%d\n", rule);
            err = FM_ERR_INVALID_VALUE;
            break;

        case RULE_PORT_MISS_TRAP:
            err = fm10000GetPortAttribute(sw,
                                          port,
                                          FM_PORT_ACTIVE_MAC,
                                          FM_PORT_LANE_NA,
                                          FM_PORT_SECURITY_ACTION,
                                          &securityAction);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SECURITY, err);

            switch (securityAction)
            {
                case FM_PORT_SECURITY_ACTION_EVENT:
                    violAction  = FM10000_SV_UNKNOWN_SMAC_EVENT;
                    diagCounter = FM_CTR_SECURITY_UNKNOWN_SMAC_EVENTS;
                    break;

                case FM_PORT_SECURITY_ACTION_TRAP:
                    violAction  = FM10000_SV_UNKNOWN_SMAC_TRAP;
                    diagCounter = FM_CTR_SECURITY_UNKNOWN_SMAC_TRAPS;
                    break;

                case FM_PORT_SECURITY_ACTION_DROP:
                    violAction  = FM10000_SV_UNKNOWN_SMAC_DROP;
                    /* Fall through to default. */

                default:
                    FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                                 "Unexpected security trap: "
                                 "rule=%d portAction=%d\n",
                                 rule,
                                 securityAction);
                    err = FM_ERR_INVALID_VALUE;
                    break;
            }
            break;

        case RULE_PORT_MOVE_TRAP:
            err = fm10000GetPortAttribute(sw,
                                          port,
                                          FM_PORT_ACTIVE_MAC,
                                          FM_PORT_LANE_NA,
                                          FM_PORT_SECURITY_ACTION,
                                          &securityAction);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SECURITY, err);

            switch (securityAction)
            {
                case FM_PORT_SECURITY_ACTION_EVENT:
                    violAction  = FM10000_SV_NON_SECURE_SMAC_EVENT;
                    diagCounter = FM_CTR_SECURITY_NON_SECURE_SMAC_EVENTS;
                    break;

                case FM_PORT_SECURITY_ACTION_TRAP:
                    violAction  = FM10000_SV_NON_SECURE_SMAC_TRAP;
                    diagCounter = FM_CTR_SECURITY_NON_SECURE_SMAC_TRAPS;
                    break;

                case FM_PORT_SECURITY_ACTION_DROP:
                    violAction  = FM10000_SV_NON_SECURE_SMAC_DROP;
                    /* Fall through to default. */

                default:
                    FM_LOG_ERROR(FM_LOG_CAT_SECURITY,
                                 "Unexpected security trap: "
                                 "rule=%d portAction=%d\n",
                                 rule,
                                 securityAction);
                    err = FM_ERR_INVALID_VALUE;
                    break;
            }
            break;

        default:
            /* Not one of our security triggers. */
            err = FM_ERR_NOT_FOUND;

    }   /* end switch (rule) */

ABORT:
    if (diagCounter != FM_CTR_INVALID)
    {
        fmDbgDiagCountIncr(sw, diagCounter, 1);
    }

    *svAction = violAction;

    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, err);

}   /* end fm10000CheckSecurityViolation */




/*****************************************************************************/
/** fm10000FreeMacSecurity
 * \ingroup intSecurity
 *
 * \desc            Frees the MAC Security trigger resources.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeMacSecurity(fm_int sw)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    const triggerDesc *     trigDesc;
    fm_status               err;
    fm_status               retVal;
    fm_uint                 trigNo;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    retVal    = FM_OK;

    /**************************************************
     * Free triggers.
     **************************************************/

    for (trigNo = 0 ; trigNo < FM_NENTRIES(triggerDescTable) ; ++trigNo)
    {
        trigDesc = triggerDescTable[trigNo];
        if (trigDesc)
        {
            err = FreeTrigger(sw, trigDesc);
            FM_ERR_COMBINE(retVal, err);
        }
    }

    /***************************************************
     * Free port sets.
     **************************************************/

    err = FreePortSet(sw, &secInfo->dropPortSet);
    FM_ERR_COMBINE(retVal, err);

    err = FreePortSet(sw, &secInfo->trapPortSet);
    FM_ERR_COMBINE(retVal, err);

    /**************************************************
     * Free trigger identifier.
     **************************************************/

    err = FreeTriggerId(sw, &secInfo->triggerId3);
    FM_ERR_COMBINE(retVal, err);

    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, retVal);

}   /* end fm10000FreeMacSecurity */




/*****************************************************************************/
/** fm10000GetSecurityStats
 * \ingroup intSecurity
 *
 * \desc            Retrieves MAC security statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      stats points to an ''fm_securityStats'' structure to be
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetSecurityStats(fm_int sw, fm_securityStats * stats)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, "sw=%d\n", sw);

    FM_CLEAR(*stats);

    /**************************************************
     * Unknown SMAC counters.
     **************************************************/

    err = GetTriggerCounter(sw,
                            &portMissDropDesc,
                            &stats->cntUnknownSmacDropPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountGet(sw,
                            FM_CTR_SECURITY_UNKNOWN_SMAC_EVENTS,
                            &stats->cntUnknownSmacEventPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountGet(sw,
                            FM_CTR_SECURITY_UNKNOWN_SMAC_TRAPS,
                            &stats->cntUnknownSmacTrapPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /**************************************************
     * Non-Secure SMAC counters.
     **************************************************/

    err = GetTriggerCounter(sw,
                            &portMoveDropDesc,
                            &stats->cntNonSecureSmacDropPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountGet(sw,
                            FM_CTR_SECURITY_NON_SECURE_SMAC_EVENTS,
                            &stats->cntNonSecureSmacEventPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountGet(sw,
                            FM_CTR_SECURITY_NON_SECURE_SMAC_TRAPS,
                            &stats->cntNonSecureSmacTrapPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /**************************************************
     * Secure SMAC counters.
     **************************************************/

    err = fmDbgDiagCountGet(sw,
                            FM_CTR_SECURITY_SECURE_SMAC_EVENTS,
                            &stats->cntSecureSmacEventPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountGet(sw,
                            FM_CTR_SECURITY_SECURE_SMAC_TRAPS,
                            &stats->cntSecureSmacTrapPkts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000GetSecurityStats */




/*****************************************************************************/
/** fm10000InitMacSecurity
 * \ingroup intSecurity
 *
 * \desc            Initializes the Security API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitMacSecurity(fm_int sw)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    secInfo->triggerId3 = FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX;

    secInfo->newMacLimiter = FM10000_INVALID_RATE_LIMITER_ID;
    secInfo->macMoveLimiter = FM10000_INVALID_RATE_LIMITER_ID;
    secInfo->noRateLimiter = FM10000_INVALID_RATE_LIMITER_ID;

    secInfo->dropPortSet = FM_PORT_SET_NONE;
    secInfo->trapPortSet = FM_PORT_SET_NONE;
    secInfo->securePortSet = FM_PORT_SET_NONE;

    switchExt->macSecurityAction = FM_MAC_SECURITY_ACTION_DROP;

    err = fm10000AllocateTriggerResource(sw,
                                         FM_TRIGGER_RES_MAC_ADDR,
                                         &secInfo->triggerId3,
                                         TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);
    
ABORT:        
    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, err);

}   /* end fm10000InitMacSecurity */




/*****************************************************************************/
/** fm10000MoveAddressSecure
 * \ingroup intSecurity
 *
 * \desc            Updates the port number of an MA Table entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       macAddress is the source MAC address.
 * 
 * \param[in]       vlanID is the VLAN identifier.
 * 
 * \param[in]       newPort is the new logical port number.
 * 
 * \param[in]       index is the MA Table index to be updated.
 *
 * \param[in,out]   numUpdates points to a variable containing the number of
 *                  updates stored in the event buffer. Will be updated if
 *                  an event is added to the buffer.
 *
 * \param[in,out]   outEvent points to a variable containing a pointer to
 *                  the buffer to which the learning and aging events
 *                  should be added. May be NULL, in which case an event
 *                  buffer will be allocated if one is needed. Will be
 *                  updated to point to the new event buffer.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ADDR_NOT_FOUND if the MA Table entry does not
 *                  match the specified (SMAC, VID). This is not necessarily
 *                  an error.
 *
 *****************************************************************************/
fm_status fm10000MoveAddressSecure(fm_int       sw,
                                   fm_macaddr   macAddress,
                                   fm_uint16    vlanID,
                                   fm_int       newPort,
                                   fm_uint32    index,
                                   fm_uint32 *  numUpdates,
                                   fm_event **  outEvent)
{
    fm_internalMacAddrEntry *   cachePtr;
    fm10000_securityInfo *      secInfo;

    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_status       status;
    fm_uint16       learningFID;
    fm_bool         l2Locked = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY,
                 "sw=%d macAddress=%012llx vlanID=%u newPort=%d index=%u "
                 "numUpdates=%u outEvent=%p\n",
                 sw,
                 macAddress,
                 vlanID,
                 newPort,
                 index,
                 *numUpdates,
                 (void *) *outEvent);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;
    secInfo   = &switchExt->securityInfo;

    /***************************************************
     * Convert VLAN ID to learning FID.
     **************************************************/

    status = fm10000GetLearningFID(sw, vlanID, &learningFID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, status);

    /***************************************************
     * Serialize access to MA Table entry.
     **************************************************/

    FM_TAKE_L2_LOCK(sw);
    l2Locked = TRUE;

    cachePtr = &switchPtr->maTable[index];
    
    /**************************************************
     * Make sure the MA Table entry at the specified 
     * index matches the (SMAC, FID). If it doesn't, 
     * the TCN FIFO entry is stale, and should be
     * ignored. 
     **************************************************/

    if ( (cachePtr->state == FM_MAC_ENTRY_STATE_INVALID) ||
         (cachePtr->macAddress != macAddress) ||
         (cachePtr->vlanID != learningFID) )
    {
        status = FM_ERR_ADDR_NOT_FOUND;
        goto ABORT;
    }

    /***************************************************
     * Ignore request if the MA Table entry is static.
     * (Bug 27856)
     **************************************************/

    if (FM_IS_ADDR_TYPE_STATIC(cachePtr->addrType))
    {
        status = FM_OK;
        goto ABORT;
    }
    
    /**************************************************
     * Verify that the port is a member of the vlan. 
     * (Bug 28151) 
     **************************************************/

    status = fm10000CheckVlanMembership(sw, vlanID, newPort);
    if (status != FM_OK)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_MAC_VLAN_ERR, 1);
        goto ABORT;
    }

    /***************************************************
     * Perform a normal station move.
     **************************************************/

    if (newPort != cachePtr->port)
    {
        fm_internalMacAddrEntry oldEntry;

        /* Save old entry for UPDATE event. */
        oldEntry = *cachePtr;

        /* Perform a normal station move. */
        status = UpdateMacEntry(sw, index, cachePtr, newPort);

        /* Drop lock before generating event. */
        FM_DROP_L2_LOCK(sw);
        l2Locked = FALSE;

        /* Generate the UPDATE events for the station move. */
        ReportStationMove(sw,
                          &oldEntry,
                          cachePtr,
                          index,
                          numUpdates,
                          outEvent);

    }   /* end if (newPort != cachePtr->port) */

ABORT:
    if (l2Locked)
    {
        FM_DROP_L2_LOCK(sw);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, status);

}   /* end fm10000MoveAddressSecure */




/*****************************************************************************/
/** fm10000NotifyMacSecurityRateLimiterId
 * \ingroup intSecurity
 *
 * \desc            Notifies the MAC security code of which storm controller
 *                  rate limiter should be used for each security violation 
 *                  condition. This is only useful when the action of a 
 *                  security violation is TRAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       cond is the the type of security violation to storm
 *                  control. FM_STORM_COND_SECURITY_VIOL_NEW_MAC or 
 *                  FM_STORM_COND_SECURITY_VIOL_MOVE.
 *
 * \param[in]       rateLimiterId is the index of the HW rate limiter to use.
 *                  FM10000_INVALID_RATE_LIMITER_ID indicates that the
 *                  storm controller has been disabled. Note that the rate
 *                  limiter is configured by the storm controller code. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NotifyMacSecurityRateLimiterId(fm_int    sw,
                                                fm_int    cond, 
                                                fm_uint32 rateLimiterId)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY,
                 "sw=%d cond=%d rateLimiterId=%d\n",
                 sw,
                 cond,
                 rateLimiterId);

    switchExt   = GET_SWITCH_EXT(sw);
    secInfo     = &switchExt->securityInfo;

    switch (cond)
    {
        case FM_STORM_COND_SECURITY_VIOL_NEW_MAC:
            secInfo->newMacLimiter = rateLimiterId;
            break;

        case FM_STORM_COND_SECURITY_VIOL_MOVE:
            secInfo->macMoveLimiter = rateLimiterId;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;

    }   /* end switch (cond) */

    err = fm10000UpdateMacSecurity(sw);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, err);

}   /* end fm10000NotifyMacSecurityRateLimiterId */




/*****************************************************************************/
/** fm10000ReportSecurityEvent
 * \ingroup intSecurity
 *
 * \desc            Sends a MAC address security violation to the event 
 *                  handler.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       macAddress is the source MAC address.
 * 
 * \param[in]       vlanID is the VLAN identifier.
 * 
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ReportSecurityEvent(fm_int     sw,
                                     fm_macaddr macAddress,
                                     fm_uint16  vlanID,
                                     fm_int     port)
{
    fm_event*   evPtr;
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY,
                 "sw=%d macAddress=%012llx vlanID=%u port=%d\n",
                 sw,
                 macAddress,
                 vlanID,
                 port);

    /* NOTE: This call will block if the number of free event 
     * buffers drops below the acceptable threshold. */
    evPtr = fmAllocateEvent(sw,
                            FM_EVID_HIGH_MAC_SECURITY,
                            FM_EVENT_SECURITY,
                            FM_EVENT_PRIORITY_LOW);

    if (evPtr)
    {
        evPtr->info.fpSecEvent.address = macAddress;
        evPtr->info.fpSecEvent.vlan    = vlanID;
        evPtr->info.fpSecEvent.port    = port;

        status = fmSendThreadEvent(&fmRootApi->eventThread, evPtr);

        if (status != FM_OK)
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_EVENT_SEND_ERR, 1);

            /* Free the event since it could not be sent to the
             * event thread.  */
            fmReleaseEvent(evPtr);
        }
    }
    else
    {
        fmDbgDiagCountIncr(sw, FM_CTR_MAC_EVENT_ALLOC_ERR, 1);
        status = FM_ERR_NO_EVENTS_AVAILABLE;
    }

    fmDbgDiagCountIncr(sw, FM_CTR_MAC_SECURITY, 1);

    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, status);

}   /* end fm10000ReportSecurityEvent */




/*****************************************************************************/
/** fm10000ResetSecurityStats
 * \ingroup intSecurity
 *
 * \desc            Resets MAC security statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fm10000ResetSecurityStats(fm_int sw)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, "sw=%d\n", sw);

    /**************************************************
     * Unknown SMAC counters.
     **************************************************/

    err = ResetTriggerCounter(sw, &portMissDropDesc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountClear(sw, FM_CTR_SECURITY_UNKNOWN_SMAC_EVENTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountClear(sw, FM_CTR_SECURITY_UNKNOWN_SMAC_TRAPS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /**************************************************
     * Non-Secure SMAC counters.
     **************************************************/

    err = ResetTriggerCounter(sw, &portMoveDropDesc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountClear(sw, FM_CTR_SECURITY_NON_SECURE_SMAC_EVENTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountClear(sw, FM_CTR_SECURITY_NON_SECURE_SMAC_TRAPS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /**************************************************
     * Secure SMAC counters.
     **************************************************/

    err = fmDbgDiagCountClear(sw, FM_CTR_SECURITY_SECURE_SMAC_EVENTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    err = fmDbgDiagCountClear(sw, FM_CTR_SECURITY_SECURE_SMAC_TRAPS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000ResetSecurityStats */




/*****************************************************************************/
/* fm10000SetPortSecurityAction
 * \ingroup intSecurity
 *
 * \desc            Processes a change in the security state of a port.
 * 
 * \note            For the time being, assume that the caller has taken
 *                  the requisite lock(s) to protect the soft state of the
 *                  switch.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \parm[in]        port is the port on which to operate.
 * 
 * \param[in]       action is the new security action value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetPortSecurityAction(fm_int    sw,
                                       fm_int    port,
                                       fm_uint32 action)
{
    fm_switch *switchPtr;
    fm_status  retVal;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY,
                 "sw=%d, port=%d, action=%u\n",
                 sw,
                 port,
                 action);

    switchPtr = GET_SWITCH_PTR(sw);
    retVal    = FM_OK;

    /* exclude CPU port */
    if (port == switchPtr->cpuPort)
    {
        action = FM_PORT_SECURITY_ACTION_NONE;
    }

    switch (action)
    {
        case FM_PORT_SECURITY_ACTION_NONE:
            err = SetTriggerPort(sw, &dropSetDesc, port, FALSE);
            FM_ERR_COMBINE(retVal, err);

            err = SetTriggerPort(sw, &trapSetDesc, port, FALSE);
            FM_ERR_COMBINE(retVal, err);
            break;

        case FM_PORT_SECURITY_ACTION_DROP:
            err = SetTriggerPort(sw, &dropSetDesc, port, TRUE);
            FM_ERR_COMBINE(retVal, err);

            err = SetTriggerPort(sw, &trapSetDesc, port, FALSE);
            FM_ERR_COMBINE(retVal, err);
            break;

        case FM_PORT_SECURITY_ACTION_EVENT:
        case FM_PORT_SECURITY_ACTION_TRAP:
            err = SetTriggerPort(sw, &dropSetDesc, port, FALSE);
            FM_ERR_COMBINE(retVal, err);

            err = SetTriggerPort(sw, &trapSetDesc, port, TRUE);
            FM_ERR_COMBINE(retVal, err);
            break;

        default:
            retVal = FM_ERR_INVALID_ARGUMENT;
            break;

    }   /* end switch (action) */

    if (retVal == FM_OK)
    {
        retVal = fm10000UpdateMacSecurity(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ADDR, retVal);

}   /* end fm10000SetPortSecurityAction */




/*****************************************************************************/
/* fm10000UpdateMacSecurity
 * \ingroup intSecurity
 *
 * \desc            Processes a change in mac security state
 * 
 * \note            For the time being, assume that the caller has taken
 *                  the requisite lock(s) to protect the soft state of the
 *                  switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateMacSecurity(fm_int sw)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    const triggerDesc *     trigDesc;
    fm_uint                 trigNo;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_SECURITY, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    err = FM_OK;

    switch (switchExt->macSecurityAction)
    {
        case FM_MAC_SECURITY_ACTION_DROP:
            secInfo->securePortSet = FM_PORT_SET_NONE;
            break;

        case FM_MAC_SECURITY_ACTION_EVENT:
        case FM_MAC_SECURITY_ACTION_TRAP:
            secInfo->securePortSet = FM_PORT_SET_ALL_EXTERNAL;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            goto ABORT;

    }   /* end  switch (switchExt->macSecurityAction) */

    for (trigNo = 0 ; trigNo < FM_NENTRIES(triggerDescTable) ; ++trigNo)
    {
        trigDesc = triggerDescTable[trigNo];
        if (trigDesc == NULL)
        {
            break;
        }
        err = ConfigTrigger(sw, trigDesc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SECURITY, err);
    }
    
ABORT:        
    FM_LOG_EXIT(FM_LOG_CAT_SECURITY, err);

}   /* end fm10000UpdateMacSecurity */




/*****************************************************************************/
/** fm10000DbgDumpMacSecurity
 * \ingroup intSwitch
 *
 * \desc            Dumps information about the MAC security subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpMacSecurity(fm_int sw)
{
    fm10000_switch *        switchExt;
    fm10000_securityInfo *  secInfo;
    fm_securityStats        stats;
    fm_status               err;
    fm_text                 cntFmt = "%-20s : %lld\n";

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchExt = GET_SWITCH_EXT(sw);
    secInfo   = &switchExt->securityInfo;

    err = fm10000GetSecurityStats(sw, &stats);
    if (err != FM_OK)
    {
        FM_LOG_PRINT("ERROR: %s\n", fmErrorMsg(err));
        goto ABORT;
    }

    FM_LOG_PRINT("\n");

    FM_LOG_PRINT(cntFmt, "UnknownSmacDrops",    stats.cntUnknownSmacDropPkts);
    FM_LOG_PRINT(cntFmt, "UnknownSmacEvents",   stats.cntUnknownSmacEventPkts);
    FM_LOG_PRINT(cntFmt, "UnknownSmacTraps",    stats.cntUnknownSmacTrapPkts);

    FM_LOG_PRINT(cntFmt, "NonSecureSmacDrops",  stats.cntNonSecureSmacDropPkts);
    FM_LOG_PRINT(cntFmt, "NonSecureSmacEvents", stats.cntNonSecureSmacEventPkts);
    FM_LOG_PRINT(cntFmt, "NonSecureSmacTraps",  stats.cntNonSecureSmacTrapPkts);

    FM_LOG_PRINT(cntFmt, "SecureSmacEvents",    stats.cntSecureSmacEventPkts);
    FM_LOG_PRINT(cntFmt, "SecureSmacTraps",     stats.cntSecureSmacTrapPkts);

ABORT:
    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fm10000DbgDumpMacSecurity */




/*****************************************************************************/
/** fmSvActionToText
 * \ingroup intEvent
 *
 * \desc            Returns the textual representation of a security
 *                  violation trap action.
 *
 * \param[in]       svAction is the security violation trap action.
 *
 * \return          Pointer to a string representing the trap action.
 *
 *****************************************************************************/
const char * fmSvActionToText(fm_int svAction)
{

    switch (svAction)
    {
        case FM10000_SV_NO_ACTION:
            return "NO_ACTION";

        case FM10000_SV_UNKNOWN_SMAC_DROP:
            return "UNKNOWN_SMAC_DROP";

        case FM10000_SV_UNKNOWN_SMAC_EVENT:
            return "UNKNOWN_SMAC_EVENT";

        case FM10000_SV_UNKNOWN_SMAC_TRAP:
            return "UNKNOWN_SMAC_TRAP";

        case FM10000_SV_NON_SECURE_SMAC_DROP:
            return "NON_SECURE_SMAC_DROP";

        case FM10000_SV_NON_SECURE_SMAC_EVENT:
            return "NON_SECURE_SMAC_EVENT";

        case FM10000_SV_NON_SECURE_SMAC_TRAP:
            return "NON_SECURE_SMAC_TRAP";

        case FM10000_SV_SECURE_SMAC_DROP:
            return "SECURE_SMAC_DROP";

        case FM10000_SV_SECURE_SMAC_EVENT:
            return "SECURE_SMAC_EVENT";

        case FM10000_SV_SECURE_SMAC_TRAP:
            return "SECURE_SMAC_TRAP";

        default:
            return "UNKNOWN";

    }   /* switch (svAction) */

}   /* end fmSvActionToText */

