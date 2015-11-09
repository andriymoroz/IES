/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_storm.c
 * Creation Date:   2013
 * Description:     Structures and functions for dealing with storm control.
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM10000_STORM_CONTROLLER_UNUSED      0
#define FM10000_STORM_CONTROLLER_RESERVED    1

/* Re-use the trigger lock */
#define TAKE_STORM_LOCK(sw)     TAKE_TRIGGER_LOCK(sw)
#define DROP_STORM_LOCK(sw)     DROP_TRIGGER_LOCK(sw)

#define STORM_CTRL_TRIG_GROUP(x)    (FM10000_TRIGGER_GROUP_STORM_CTRL + (x))

#define DEFAULT_TRIG_RULE       10

#define FM10000_INVALID_STORM_CONTROLLER   (-1)

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* This table is used to see if a condition type is supported for the FM10000
 * chip family */
static const fm_bool conditionSupportedTable[FM_STORM_COND_MAX] = 
{
    TRUE,       /* FM_STORM_COND_BROADCAST */
    TRUE,       /* FM_STORM_COND_IGMP */
    FALSE,      /* FM_STORM_COND_802_1X */
    FALSE,      /* FM_STORM_COND_BPDU */
    FALSE,      /* FM_STORM_COND_LACP */
    TRUE,       /* FM_STORM_COND_FLOOD */
    FALSE,      /* FM_STORM_COND_FLOOD_UCAST */
    FALSE,      /* FM_STORM_COND_FLOOD_MCAST */
    TRUE,       /* FM_STORM_COND_FIDFORWARD */
    FALSE,      /* FM_STORM_COND_FIDFORWARD_UCAST */
    FALSE,      /* FM_STORM_COND_FIDFORWARD_MCAST */
    TRUE,       /* FM_STORM_COND_MULTICAST */
    TRUE,       /* FM_STORM_COND_LOG_ICMP */
    TRUE,       /* FM_STORM_COND_TRAP_ICMP */
    TRUE,       /* FM_STORM_COND_CPU */
    TRUE,       /* FM_STORM_COND_SECURITY_VIOL_NEW_MAC */
    TRUE,       /* FM_STORM_COND_SECURITY_VIOL_MOVE */
    FALSE,      /* FM_STORM_COND_INGRESS_PORT */
    FALSE,      /* FM_STORM_COND_EGRESS_PORT */
    TRUE,       /* FM_STORM_COND_UNICAST */
    FALSE,      /* FM_STORM_COND_NEXTHOP_MISS */
    TRUE,       /* FM_STORM_COND_INGRESS_PORTSET */
    TRUE,       /* FM_STORM_COND_EGRESS_PORTSET */
    TRUE,       /* FM_STORM_COND_RESERVED_MAC */
};


/* This table checks if two conditions can be combined in a stormController.
 * Refer to ''fm_stormCondType'' for the definition of the condition types.
 *
 * NOTE: When updating this table, you must also update the corresponding 
 * information in the descriptions for each member of ''fm_stormCondType''. */
static const fm_bool conditionCompatibleTable[FM_STORM_COND_MAX][FM_STORM_COND_MAX] =
{
    /*                  BCAST   IGMP    802.1X  BPDU    LACP    FLOOD   FLOOD_U FLOOD_M FIDFWD  FIDFWD_U  FIDFWD_M  MCAST   LOGICMP TRAPICMP  CPU      SECMAC  SECMV   INPORT  EGPORT  UCAST   NH_MISS IN_PS   EG_PS   RES_MAC  */
    /*                  ------  ----    ------  -----   ----    -----   ------  -----   ------- --------  --------  ------  ------  ------    ------   ------  -----   ------  ------  ------  ------  ------  ------  ------   */
    /* BCAST */     {   FALSE,  TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   TRUE,   TRUE      },
                                                                                                                                                                                                                                 
    /* IGMP */      {   TRUE,   FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   FALSE,  TRUE      },
                                                                                                                                                                                                                                 
    /* 802.1X */    {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* BPDU */      {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* LACP */      {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* FLOOD */     {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   TRUE,   TRUE      },
                                                                                                                                                                                                                                 
    /* FLOOD_U */   {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* FLOOD_M */   {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* FIDFWD */    {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   TRUE,   TRUE      },
                                                                                                                                                                                                                                 
    /* FIDFWD_U */  {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* FIDFWD_M */  {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* MCAST */     {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    FALSE,  TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   TRUE,   TRUE      },
                                                                                                                                                                                                                                 
    /* LOGICMP */   {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   FALSE,  TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   TRUE,   TRUE      },
                                                                                                                                                                                                                                 
    /* TRAPICMP */  {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   FALSE,    TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   FALSE,  TRUE      },
                                                                                                                                                                                                                                 
    /* CPU */       {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   FALSE,  TRUE      },
                                                                                                                                                                                                                                 
    /* SECMAC */    {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  TRUE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* SECMV */     {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   TRUE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* INPORT */    {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* EGPORT */    {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                       
    /* UCAST */     {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   TRUE,   TRUE      },
                                                                                                                                                                                                       
    /* NH_MISS */   {   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,    FALSE,    FALSE,  FALSE,  FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* IN_PS */     {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   TRUE      },
                                                                                                                                                                                                                                 
    /* EG_PS */     {   TRUE,   FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   FALSE,    FALSE,   FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   FALSE,  FALSE     },
                                                                                                                                                                                                                                 
    /* RES_MAC */   {   TRUE,   TRUE,   FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  FALSE,  TRUE,   FALSE,    FALSE,    TRUE,   TRUE,   TRUE,     TRUE,    FALSE,  FALSE,  FALSE,  FALSE,  TRUE,   FALSE,  TRUE,   FALSE,  FALSE     },

};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** SetDefaultCondition
 * \ingroup intstorm
 *
 * \desc            restore the default initial condition of a storm controller.
 *
 * \param[out]      trigCond points to the user allocated storage
 *                  to store the default trigger condition for the
 *                  storm controller.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
static fm_status SetDefaultCondition(fm_triggerCondition *trigCond)
{
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "trigCond=%p\n",
                 (void *) trigCond);

    FM_CLEAR(*trigCond);

    trigCond->cfg.matchSA =             FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchDA =             FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchHitSA =          FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchHitDA =          FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchHitSADA =        FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchVlan =           FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchFFU =            FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchSwitchPri =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchEtherType =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    trigCond->cfg.matchDestGlort =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;

    trigCond->cfg.matchFrameClassMask = 0;

    trigCond->cfg.matchRoutedMask =     (FM_TRIGGER_SWITCHED_FRAMES |
                                        FM_TRIGGER_ROUTED_FRAMES);

    trigCond->cfg.matchFtypeMask =      FM_TRIGGER_FTYPE_NORMAL;

    trigCond->cfg.matchRandomNumber =   FALSE;
    trigCond->cfg.matchTx =             FM_TRIGGER_TX_MASK_CONTAINS;

    trigCond->cfg.rxPortset =           FM_PORT_SET_ALL_BUT_CPU;
    trigCond->cfg.txPortset =           FM_PORT_SET_ALL;
    trigCond->cfg.HAMask =              0;

    FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_OK);

}   /* end SetDefaultCondition*/




/*****************************************************************************/
/** CheckConditionValid
 * \ingroup intstorm
 *
 * \desc            check the feasibility of the requested storm controller
 *                  condition. Assume the validity of the arguments has been
 *                  verified.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       scPtr is a pointer to the storm controller's internal
 *                  structure.
 *
 * \param[in]       condType is the condition whose validity is to be checked.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_COND if condType is not compatible with
 *                  the current condition configuration.
 *
 *****************************************************************************/
static fm_status CheckConditionValid(fm_int                   sw, 
                                     fm10000_stormController *scPtr,
                                     fm_stormCondType         condType)
{
    fm10000_scInfo *   scInfo;
    fm10000_switch *   switchExt;
    fm_stormCondition *curCond;
    fm_int             i;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM, 
                 "sw=%d scPtr=%p condType=%s\n", 
                 sw, (void *) scPtr, fmStormCondTypeToText(condType));

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    if (conditionSupportedTable[condType] == FALSE)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                         "The input condition %s is not supported\n",
                         fmStormCondTypeToText(condType));
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_COND);
    }

    for (i = 0; i < scPtr->numConditions; i++)
    {
        curCond = &scPtr->conditions[i];

        if (!conditionCompatibleTable[condType][curCond->type])
        {
            FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                         "The input condition %s conflicts with %s\n",
                         fmStormCondTypeToText(condType), 
                         fmStormCondTypeToText(curCond->type));
            FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_COND);
        }
    }

    /* Validate that FM_STORM_COND_SECURITY_VIOL_NEW_MAC and
     * FM_STORM_COND_SECURITY_VIOL_MOVE are not in other storm controllers
     * if they are the requested condition. */
    for (i = 0; i < FM10000_MAX_NUM_STORM_CTRL; i++)
    {
        if (scInfo->used[i] == FM10000_STORM_CONTROLLER_RESERVED)
        {
            if ( (condType == FM_STORM_COND_SECURITY_VIOL_NEW_MAC) &&
                 (scInfo->stormCtrl[i].newMacViolCondUsed))
            {
                FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                             "The input condition %s can only be used in "
                             "one storm controller\n",
                             fmStormCondTypeToText(condType));
                FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_COND);
            }
            else if ( (condType == FM_STORM_COND_SECURITY_VIOL_MOVE) &&
                      (scInfo->stormCtrl[i].macMoveViolCondUsed))
            {
                FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                             "The input condition %s can only be used in "
                             "one storm controller\n",
                             fmStormCondTypeToText(condType));
                FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_COND);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_OK);

}   /* end CheckConditionValid */




/*****************************************************************************/
/** ApplyStormCtrlConditions
 * \ingroup intstorm
 *
 * \desc            Apply the current configured conditions to the storm
 *                  controller.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       stormController is the storm controller number.
 *
 * \param[in]       scPtr is a pointer to the storm controller's internal
 *                  structure.
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status ApplyStormCtrlConditions(fm_int                   sw,
                                          fm_int                   stormController,
                                          fm10000_stormController *scPtr)
{
    fm_status           err;
    fm_triggerCondition trigCond;
    fm_int              i;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM, 
                 "sw = %d, stormController = %d, scPtr = %p\n", 
                 sw, 
                 stormController, 
                 (void *)scPtr);

    /* If a mac security condition is used, the mac security API is responsible
     * of storm controlling, don't configure the trigger. */
    if ( (scPtr->newMacViolCondUsed) || (scPtr->macMoveViolCondUsed) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_OK);
    }

    err = SetDefaultCondition(&trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    
    for (i = 0 ; i < scPtr->numConditions ; i++)
    {
        switch (scPtr->conditions[i].type)
        {
            case FM_STORM_COND_UNICAST:
                trigCond.cfg.matchFrameClassMask |= FM_TRIGGER_FRAME_CLASS_UCAST;
                break;

            case FM_STORM_COND_MULTICAST:
                trigCond.cfg.matchFrameClassMask |= FM_TRIGGER_FRAME_CLASS_MCAST;
                break;

            case FM_STORM_COND_BROADCAST:
                trigCond.cfg.matchFrameClassMask |= FM_TRIGGER_FRAME_CLASS_BCAST;
                break;

            case FM_STORM_COND_FLOOD:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_FORWARD_FLOOD;
                break;

            case FM_STORM_COND_FIDFORWARD:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_FORWARD_FID;
                break;

            case FM_STORM_COND_IGMP:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_TRAP_IGMP;

                /* The trapped frame has a NULL DMASK in the trigger block. */
                trigCond.cfg.matchTx   = FM_TRIGGER_TX_MASK_EQUALS;
                trigCond.cfg.txPortset = FM_PORT_SET_NONE;
                break;

            case FM_STORM_COND_LOG_ICMP:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_LOG_MCST_ICMP_TTL;
                break;

            case FM_STORM_COND_TRAP_ICMP:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_TRAP_ICMP_TTL;
                                                    
                /* The trapped frame has a NULL DMASK in the trigger block. */
                trigCond.cfg.matchTx   = FM_TRIGGER_TX_MASK_EQUALS;
                trigCond.cfg.txPortset = FM_PORT_SET_NONE;
                break;

            case FM_STORM_COND_CPU:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_TRAP_CPU;

                /* The trapped frame has a NULL DMASK in the trigger block. */
                trigCond.cfg.matchTx   = FM_TRIGGER_TX_MASK_EQUALS;
                trigCond.cfg.txPortset = FM_PORT_SET_NONE;
                break;

            case FM_STORM_COND_RESERVED_MAC:
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_TRAP_RESERVED_MAC;
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP;
                trigCond.cfg.HAMask   |= FM_TRIGGER_HA_LOG_RESERVED_MAC;

                /* The trapped frame has a NULL DMASK in the trigger block. */
                trigCond.cfg.matchTx   = FM_TRIGGER_TX_MASK_EQUALS;
                trigCond.cfg.txPortset = FM_PORT_SET_NONE;
                break;

            case FM_STORM_COND_INGRESS_PORTSET:
                trigCond.cfg.rxPortset = scPtr->conditions[i].param;
                break;

            case FM_STORM_COND_EGRESS_PORTSET:
                trigCond.cfg.txPortset = scPtr->conditions[i].param;
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
                break;

        }   /* end switch (scPtr->conditions[i].type) */

    }   /* end for (i = 0 ; i < scPtr->numConditions ; i++) */

    /* Storm Controllers that don't have any conditions should not hit */
    if (scPtr->numConditions >= 1)
    {
        /**********************************************
         * Assign some values if none are defined
         **********************************************/
        if (trigCond.cfg.matchFrameClassMask == 0)
        {
            trigCond.cfg.matchFrameClassMask = FM_TRIGGER_FRAME_CLASS_UCAST |
                                               FM_TRIGGER_FRAME_CLASS_MCAST |
                                               FM_TRIGGER_FRAME_CLASS_BCAST;
        }

        if (trigCond.cfg.HAMask == 0)
        {
            /* All non-drop handler actions */
            trigCond.cfg.HAMask |= (FM_TRIGGER_HA_TRAP_CPU                |
                                    FM_TRIGGER_HA_TRAP_FFU                |
                                    FM_TRIGGER_HA_TRAP_ICMP_TTL           |
                                    FM_TRIGGER_HA_TRAP_IP_OPTION          |
                                    FM_TRIGGER_HA_TRAP_MTU                |
                                    FM_TRIGGER_HA_TRAP_IGMP               |
                                    FM_TRIGGER_HA_TRAP_TTL                |
                                    FM_TRIGGER_HA_TRAP_RESERVED_MAC       |
                                    FM_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP |
                                    FM_TRIGGER_HA_FORWARD_SPECIAL         |
                                    FM_TRIGGER_HA_FORWARD_DGLORT          |
                                    FM_TRIGGER_HA_FORWARD_FLOOD           |
                                    FM_TRIGGER_HA_SWITCH_RESERVED_MAC     |
                                    FM_TRIGGER_HA_FORWARD_FID             |
                                    FM_TRIGGER_HA_LOG_FFU_I               |
                                    FM_TRIGGER_HA_LOG_RESERVED_MAC        |
                                    FM_TRIGGER_HA_LOG_ARP_REDIRECT        |
                                    FM_TRIGGER_HA_LOG_MCST_ICMP_TTL       |
                                    FM_TRIGGER_HA_LOG_IP_MCST_TTL         |
                                    FM_TRIGGER_HA_PARSE_TIMEOUT           |
                                    FM_TRIGGER_HA_MIRROR_FFU);
        }
    }

    err = fm10000SetTriggerCondition(sw, 
                                     STORM_CTRL_TRIG_GROUP(stormController), 
                                     DEFAULT_TRIG_RULE,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end ApplyStormCtrlConditions */




/*****************************************************************************/
/** ApplyStormCtrlAction
 * \ingroup intstorm
 *
 * \desc            Apply the configured action to a storm controller.
 *                  Action dictate what happens when a trigger condition match
 *                  is detected by the storm controller and the controller's
 *                  token bucket limit is reached.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       stormController is the storm controller number.
 *
 * \param[in]       scPtr is a pointer to the storm controller's internal
 *                  structure.
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status ApplyStormCtrlActions(fm_int                   sw,
                                       fm_int                   stormController,
                                       fm10000_stormController *scPtr)
{
    fm_status         err;
    fm_triggerAction  trigAction;
    fm_rateLimiterCfg rateLimiterCfg;
    fm_uint32         rateLimiterId;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM, 
                 "sw = %d, stormController = %d, scPtr = %p\n", 
                 sw, 
                 stormController, 
                 (void *)scPtr);

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    if (scPtr->action.type == FM_STORM_ACTION_FILTER_PORTSET)
    {
        trigAction.cfg.rateLimitAction = FM_TRIGGER_RATELIMIT_ACTION_RATELIMIT;
        trigAction.param.rateLimitNum = scPtr->trigRateLimiterId;

        err = fm10000GetTriggerRateLimiter(sw, 
                                           scPtr->trigRateLimiterId,
                                           &rateLimiterCfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

        rateLimiterCfg.dropPortset = scPtr->action.param;

        err = fm10000SetTriggerRateLimiter(sw, 
                                           scPtr->trigRateLimiterId,
                                           &rateLimiterCfg,
                                           TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

        rateLimiterId = scPtr->trigRateLimiterId;
    }
    else
    {
        rateLimiterId = FM10000_INVALID_RATE_LIMITER_ID;
    }

    if (scPtr->newMacViolCondUsed)
    {
        err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                    FM_STORM_COND_SECURITY_VIOL_NEW_MAC, 
                                                    rateLimiterId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    if (scPtr->macMoveViolCondUsed)
    {
        err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                    FM_STORM_COND_SECURITY_VIOL_MOVE, 
                                                    rateLimiterId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }
    
    err = fm10000SetTriggerAction(sw, 
                                  STORM_CTRL_TRIG_GROUP(stormController), 
                                  DEFAULT_TRIG_RULE,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end ApplyStormCtrlActions */




/*****************************************************************************/
/** InitStormCtrl
 * \ingroup intstorm
 *
 * \desc            Initialize the storm controller
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the next action
 *                  should be retrieved.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 *
 *****************************************************************************/
static fm_status InitStormCtrl(fm_int sw, fm_int stormController)
{
    fm_rateLimiterCfg        rateLimitCfg;
    fm10000_stormController *scPtr;
    fm10000_switch *         switchExt;
    fm_status                err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM, "sw = %d, stormController = %d\n",
                 sw, stormController);

    switchExt = GET_SWITCH_EXT(sw);
    scPtr = &switchExt->scInfo.stormCtrl[stormController];

    scPtr->numConditions = 0;

    /**********************************
     * Initialize trigger rate limiter
     *********************************/

    /* Capacity = 1024 Bytes
     * Rate     = 150  Mbps */
    rateLimitCfg.capacity    = 1;
    rateLimitCfg.rate        = 150000;
    rateLimitCfg.dropPortset = FM_PORT_SET_NONE;

    /**********************************
     * Apply to triggers
     *********************************/

    err = fm10000SetTriggerRateLimiter(sw, 
                                       scPtr->trigRateLimiterId, 
                                       &rateLimitCfg, 
                                       TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    err = ApplyStormCtrlActions(sw, stormController, scPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    err = ApplyStormCtrlConditions(sw, stormController, scPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end InitStormCtrl */




/*****************************************************************************/
/** DumpStormControllerCompTable
 * \ingroup intstorm
 *
 * \desc            Dump storm controller compatibility table. 
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status DumpStormControllerCompTable(void)
{
    fm_status err = FM_OK;
    fm_int    i;
    fm_int    j;
    fm_char   sep[80];
    fm_char   buf[10];

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Supported Conditions and their ID:\n");
    FM_LOG_PRINT("==========================================\n");

    for (i = 0; i < FM_STORM_COND_MAX; i++)
    {
        if (conditionSupportedTable[i] == TRUE)
        {
            FM_LOG_PRINT(" %02d - %s\n", i, fmStormCondTypeToText(i));
        }
    }

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Condition Compatibility Table\n");
    FM_LOG_PRINT("==========================================\n\n");


    FM_LOG_PRINT("    ");
    FM_STRCPY_S(sep, sizeof(sep), " ---");

    /* column */
    for (j = 0; j < FM_STORM_COND_MAX; j++)
    {
        if (conditionSupportedTable[j] == FALSE)
        {
            continue;
        }

        FM_LOG_PRINT("| %02d ", j);
        FM_STRCAT_S(sep, sizeof(sep), "+----");
    }
    FM_LOG_PRINT("|\n");
    FM_STRCAT_S(sep, sizeof(sep), "+\n");
    FM_LOG_PRINT("%s", sep);

    /* row */
    for (i = 0; i < FM_STORM_COND_MAX; i++)
    {
        if (conditionSupportedTable[i] == FALSE)
        {
            continue;
        }

        FM_LOG_PRINT(" %02d ", i);

        /* column */
        for (j = 0; j < FM_STORM_COND_MAX; j++)
        {
            if (conditionSupportedTable[j] == FALSE)
            {
                continue;
            }

            if (conditionCompatibleTable[i][j] == TRUE)
            {
                FM_LOG_PRINT("| x  ");
            }
            else
            {
                FM_LOG_PRINT("|    ");
            }
        }
        
        FM_LOG_PRINT("|\n");
        FM_LOG_PRINT("%s", sep);
    }

    FM_LOG_PRINT("\n\n");

    return err;
}




/*****************************************************************************/
/** DumpStormController
 * \ingroup intstorm
 *
 * \desc            Dump a storm controller. This function assumes the storm
 *                  lock has been taken by the caller. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl''). 
 *                  controllers.
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status DumpStormController(fm_int sw, fm_int stormController)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    fm_int                   i;
    fm_int                   capacity;
    fm_int                   rate;
    fm_uint64                counter;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d\n",
                 sw, stormController);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    FM_LOG_PRINT("####################################\n");
    FM_LOG_PRINT("# Storm Controller %d\n", stormController);
    FM_LOG_PRINT("####################################\n");
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Conditions:\n");

    for (i = 0; i < scPtr->numConditions; i++)
    {
        FM_LOG_PRINT("  Type=%25s Param=%d\n", 
                     fmStormCondTypeToText(scPtr->conditions[i].type),
                     scPtr->conditions[i].param);
    }

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Action:\n");
    FM_LOG_PRINT("  Type=%25s Param=%d\n", 
                     fmStormActionTypeToText(scPtr->action.type),
                     scPtr->action.param);

    err = fm10000GetStormCtrlAttribute(sw, 
                                       stormController, 
                                       FM_STORM_CAPACITY,
                                       (void *) &capacity);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    err = fm10000GetStormCtrlAttribute(sw, 
                                       stormController, 
                                       FM_STORM_RATE,
                                       (void *) &rate);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    err = fm10000GetStormCtrlAttribute(sw, 
                                       stormController, 
                                       FM_STORM_COUNT,
                                       (void *) &counter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);


    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Attributes:\n");
    FM_LOG_PRINT("  Capacity: %d bytes\n", capacity);
    FM_LOG_PRINT("  Rate:     %d kbps\n", rate);
    FM_LOG_PRINT("  Count:    %" FM_FORMAT_64 "d\n", counter);
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\n");

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end DumpStormController */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000InitStormControllers
 * \ingroup intstorm
 *
 * \desc            Initialize storm controller allocator.
 *
 * \param[in]       switchPtr points to the switch control structure.
 *
 * \return          FM_OK if successfull.
 *
 *****************************************************************************/
fm_status fm10000InitStormControllers(fm_switch * switchPtr)
{
    fm10000_switch * switchExt;
    
    switchExt = (fm10000_switch *) switchPtr->extension;

    /* This will put all scInfo.used bits in the
     * FM10000_STORM_CONTROLLER_UNUSED state. */
    FM_CLEAR(switchExt->scInfo);

    /* Make sure that new storm controller conditions are added at the end
     * of the typedef enum. The "conditionSupportedTable" and
     * "conditionCompatibleTable" tables expect the order of enum elements 
     * to remain the same. */
    if (FM_STORM_COND_RESERVED_MAC != 23)
    {
        FM_LOG_FATAL(FM_LOG_CAT_STORM, 
                     "Ordering in fm_stormCondType can cause undefined behavior\n");
        return FM_FAIL;
    }

    return FM_OK;
    
}   /* end fm10000InitStormControllers */




/*****************************************************************************/
/** fm10000CreateStormCtrl
 * \ingroup intstorm
 *
 * \desc            Create a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      stormController points to caller-allocated storage where
 *                  this function should place the storm controller number
 *                  (handle) of the newly created storm controller.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_FREE_STORM_CTRL if the maximum number of storm
 *                  controllers (FM10000_MAX_NUM_STORM_CTRL) have already
 *                  been created.
 * \return          FM_ERR_INVALID_ARGUMENT if stormController is NULL.
 *
 *****************************************************************************/
fm_status fm10000CreateStormCtrl(fm_int sw, fm_int *stormController)
{
    fm_status                err = FM_OK;
    fm_status                err2 = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    fm_int                   index;
    fm_char                  trigName[32];

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %p\n",
                 sw, (void *) stormController);

    if (stormController == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    for (index = 0 ; index < FM10000_MAX_NUM_STORM_CTRL ; index++)
    {
        if (scInfo->used[index] == FM10000_STORM_CONTROLLER_UNUSED)
        {
            /* Mark it as reserved. */
            scInfo->used[index] = FM10000_STORM_CONTROLLER_RESERVED;
            break;
        }
    }

    if (index >= FM10000_MAX_NUM_STORM_CTRL)
    {
        DROP_STORM_LOCK(sw);
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_NO_FREE_STORM_CTRL);
    }

    scPtr = &scInfo->stormCtrl[index];

    FM_CLEAR(*scPtr);
    scPtr->trigRateLimiterId = FM10000_INVALID_RATE_LIMITER_ID;

    FM_SPRINTF_S(trigName, sizeof(trigName), "stormController[%d]", index);

    err = fm10000AllocateTriggerResource(sw, 
                                         FM_TRIGGER_RES_RATE_LIMITER,
                                         &scPtr->trigRateLimiterId,
                                         TRUE);
    if (err)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                     "Cannot create storm controller, %s\n",
                     fmErrorMsg(err));
        err = FM_ERR_NO_FREE_STORM_CTRL;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    /* Future optimization: the trigger creation could be done
     * when a condition is added. This way one or two triggers
     * could be spared when FM_STORM_COND_SECURITY_VIOL_NEW_MAC 
     * and FM_STORM_COND_SECURITY_VIOL_MOVE are not used. */ 
    err = fm10000CreateTrigger(sw, 
                               STORM_CTRL_TRIG_GROUP(index), 
                               DEFAULT_TRIG_RULE,
                               TRUE, 
                               trigName);
    if (err)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                     "Cannot create storm controller, %s\n",
                     fmErrorMsg(err));
        err = FM_ERR_NO_FREE_STORM_CTRL;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    err = InitStormCtrl(sw, index);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    *stormController = index;

ABORT:
    if (err)
    {
        /* Mark it as free. */
        scInfo->used[index] = FM10000_STORM_CONTROLLER_UNUSED;
        
        if ( (scPtr != NULL) &&
             (scPtr->trigRateLimiterId != FM10000_INVALID_RATE_LIMITER_ID) )
        {
            err2 = fm10000FreeTriggerResource(sw, 
                                              FM_TRIGGER_RES_RATE_LIMITER,
                                              scPtr->trigRateLimiterId, 
                                              TRUE);

            if (err2)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_STORM,
                             "Failed to free trigger rate limiter, %s\n",
                             fmErrorMsg(err2));
            }
        }
    }

    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000CreateStormCtrl */




/*****************************************************************************/
/** fm10000DeleteStormCtrl
 * \ingroup intstorm
 *
 * \desc            Delete a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not the
 *                  handle of an existing storm controller.
 *
 *****************************************************************************/
fm_status fm10000DeleteStormCtrl(fm_int sw, fm_int stormController)
{
    fm_status                err;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d\n",
                 sw, stormController);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if (scPtr->newMacViolCondUsed)
    {
        err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                    FM_STORM_COND_SECURITY_VIOL_NEW_MAC, 
                                                    FM10000_INVALID_RATE_LIMITER_ID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    if (scPtr->macMoveViolCondUsed)
    {
        err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                    FM_STORM_COND_SECURITY_VIOL_MOVE, 
                                                    FM10000_INVALID_RATE_LIMITER_ID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    err = fm10000DeleteTrigger(sw, 
                               STORM_CTRL_TRIG_GROUP(stormController), 
                               DEFAULT_TRIG_RULE,
                               TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    err = fm10000FreeTriggerResource(sw, 
                                     FM_TRIGGER_RES_RATE_LIMITER, 
                                     scPtr->trigRateLimiterId,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

    scInfo->used[stormController] = FM10000_STORM_CONTROLLER_UNUSED;

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000DeleteStormCtrl */




/*****************************************************************************/
/** fm10000SetStormCtrlAttribute
 * \ingroup intstorm
 *
 * \desc            Set a storm controller attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') whose attribute is to be set.
 *
 * \param[in]       attr is the storm controller attribute to set.
 *                  See ''Storm Controller Attributes'' for details.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormControllers is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ATTRIB if attr is unrecognized.
 * \return          FM_ERR_INVALID_ARGUMENT if the value is NULL.
 *
 *****************************************************************************/
fm_status fm10000SetStormCtrlAttribute(fm_int sw,
                                      fm_int stormController,
                                      fm_int attr,
                                      void * value)
{
    fm_status                err = FM_OK;
    fm_rateLimiterCfg        rateLimiterCfg;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, attr=%d, value=%p \n",
                 sw, stormController, attr, value);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if (attr != FM_STORM_COUNT)
    {
        err = fm10000GetTriggerRateLimiter(sw, 
                                           scPtr->trigRateLimiterId, 
                                           &rateLimiterCfg); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    switch (attr)
    {
        case FM_STORM_CAPACITY:
            /* The *value is in bytes, capacity is in 1024 bytes unit */
            rateLimiterCfg.capacity = ( ( *( (fm_uint32 *) (value) ) / 1024 ) + 0.5);

            err = fm10000SetTriggerRateLimiter(sw, 
                                               scPtr->trigRateLimiterId, 
                                               &rateLimiterCfg,
                                               TRUE); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            break;

        case FM_STORM_RATE:
            rateLimiterCfg.rate = *( (fm_uint32 *) (value));

            err = fm10000SetTriggerRateLimiter(sw, 
                                               scPtr->trigRateLimiterId, 
                                               &rateLimiterCfg,
                                               TRUE); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            break;

        case FM_STORM_COUNT:
            err = fm10000SetTriggerAttribute(sw, 
                                             STORM_CTRL_TRIG_GROUP(stormController), 
                                             DEFAULT_TRIG_RULE,
                                             FM_TRIGGER_ATTR_COUNTER,
                                             value,
                                             TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;

    }   /* end switch (attr) */

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000SetStormCtrlAttribute */




/*****************************************************************************/
/** fm10000GetStormCtrlAttribute
 * \ingroup intstorm
 *
 * \desc            Get a storm controller attribute
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') whose attribute is to be
 *                  retrieved.
 *
 * \param[in]       attr is the storm controller attribute to get.
 *                  See ''Storm Controller Attributes'' for details.
 *
 * \param[out]      value points to caller-allocated storage where
 *                  this function will place the value of the attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormControllers is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ATTRIB if attr is unrecognized.
 * \return          FM_ERR_INVALID_ARGUMENT if the value is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlAttribute(fm_int sw,
                                      fm_int stormController,
                                      fm_int attr,
                                      void * value)
{
    fm_status                err = FM_OK;
    fm_rateLimiterCfg        rateLimiterCfg;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, attr=%d, value=%p \n",
                 sw, stormController, attr, value);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if (attr != FM_STORM_COUNT)
    {
        err = fm10000GetTriggerRateLimiter(sw, 
                                           scPtr->trigRateLimiterId, 
                                           &rateLimiterCfg); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    switch (attr)
    {
        case FM_STORM_CAPACITY:
            /* the value is in bytes, capacity is in 1024 bytes unit */
            *( (fm_uint32 *) value ) = rateLimiterCfg.capacity * 1024;
            break;

        case FM_STORM_RATE:
            *( (fm_uint32 *) value ) = rateLimiterCfg.rate;
            break;

        case FM_STORM_COUNT:
            err = fm10000GetTriggerAttribute(sw, 
                                             STORM_CTRL_TRIG_GROUP(stormController), 
                                             DEFAULT_TRIG_RULE,
                                             FM_TRIGGER_ATTR_COUNTER,
                                             value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;

    }   /* end switch (attr) */

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlAttribute */




/*****************************************************************************/
/** fm10000GetStormCtrlList
 * \ingroup intstorm
 *
 * \desc            Return a list of valid storm controller numbers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numStormControllers points to caller allocated storage
 *                  where this function should place the number of valid storm
 *                  controllers returned in stormControllers.
 *
 * \param[out]      stormControllers is an array that this function will fill
 *                  with the list of valid storm controller numbers.
 *
 * \param[in]       max is the size of stormControllers, being the maximum
 *                  number of storm controller numbers that stormControllers
 *                  can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of valid storm controller numbers.
 * \return          FM_ERR_INVALID_ARGUMENT if numStormControllers is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlList(fm_int  sw,
                                  fm_int *numStormControllers,
                                  fm_int *stormControllers,
                                  fm_int  max)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm10000_scInfo *scInfo;
    fm_int          i;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, "
                 "numStormControllers = %p, "
                 "stormControllers=%p, "
                 "max=%d \n",
                 sw, 
                 (void *) numStormControllers, 
                 (void *) stormControllers, 
                 max);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    *numStormControllers = 0;

    for (i = 0; i < FM10000_MAX_NUM_STORM_CTRL; i++)
    {
        if (scInfo->used[i] == FM10000_STORM_CONTROLLER_RESERVED)
        {
            if (*numStormControllers >= max)
            {
                err = FM_ERR_BUFFER_FULL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            }

            stormControllers[*numStormControllers] = i;
            *numStormControllers = *numStormControllers + 1;
        }
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlList */




/*****************************************************************************/
/** fm10000GetStormCtrlFirst
 * \ingroup intstorm
 *
 * \desc            Retrieve the first storm controller number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstStormController points to caller-allocated storage
 *                  where this function should place the number of the first
 *                  storm controller number.  Will be set to -1 if no storm
 *                  controllers found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_STORM_CONTROLLERS if no storm controller found.
 * \return          FM_ERR_INVALID_ARGUMENT if firstStormController is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlFirst(fm_int sw, fm_int *firstStormController)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm10000_scInfo *scInfo;
    fm_int          i;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, firstStormController = %p\n",
                 sw, 
                 (void *) firstStormController);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    *firstStormController = FM10000_INVALID_STORM_CONTROLLER;

    TAKE_STORM_LOCK(sw);

    for (i = 0; i < FM10000_MAX_NUM_STORM_CTRL; i++)
    {
        if (scInfo->used[i] == FM10000_STORM_CONTROLLER_RESERVED)
        {
            *firstStormController = i;
            break;
        }
    }

    if (*firstStormController == FM10000_INVALID_STORM_CONTROLLER)
    {
        err = FM_ERR_NO_STORM_CONTROLLERS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlFirst */




/*****************************************************************************/
/** fm10000GetStormCtrlNext
 * \ingroup intstorm
 *
 * \desc            Retrieve the next storm controller number, following
 *                  a prior call to this function or to
 *                  ''fmGetStormCtrlFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentStormController is the last storm controller number
 *                  found by a previous call to this function or to
 *                  ''fmGetStormCtrlFirst''.
 *
 * \param[out]      nextStormController points to caller-allocated storage
 *                  where this function should place the number of the next
 *                  storm controller. Will be set to -1 if no more storm
 *                  controllers found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if currentStormController is out
 *                  of range or is not the handle of an existing storm
 *                  controller.
 * \return          FM_ERR_NO_STORM_CONTROLLERS if no more storm controller
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if nextStormController is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlNext(fm_int  sw,
                                  fm_int  currentStormController,
                                  fm_int *nextStormController)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm10000_scInfo *scInfo;
    fm_int          i;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, "
                 "currentStormController = %d, "
                 "nextStormController = %p\n",
                 sw, 
                 currentStormController,
                 (void *) nextStormController);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    if ( (currentStormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (currentStormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    *nextStormController = FM10000_INVALID_STORM_CONTROLLER;

    TAKE_STORM_LOCK(sw);

    /* Validate that currentStormController is valid */
    if (scInfo->used[currentStormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    for (i = (currentStormController + 1); i < FM10000_MAX_NUM_STORM_CTRL; i++)
    {
        if (scInfo->used[i] == FM10000_STORM_CONTROLLER_RESERVED)
        {
            *nextStormController = i;
            break;
        }
    }

    if (*nextStormController == FM10000_INVALID_STORM_CONTROLLER)
    {
        err = FM_ERR_NO_STORM_CONTROLLERS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlNext */




/*****************************************************************************/
/** fm10000AddStormCtrlCondition
 * \ingroup intstorm
 *
 * \desc            Add a condition to a storm controller. This function may
 *                  be called multiple times to add multiple conditions to a
 *                  single storm controller. All conditions are ORed together.
 *                  As an example, setting both FM_STORM_COND_IGMP and
 *                  FM_STORM_COND_BROADCAST (see ''fm_stormCondType'') causes
 *                  either packet type to trigger the storm controller. This
 *                  function will automatically check existing conditions to
 *                  prevent duplicate entries.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') to which the condition should
 *                  be added.
 *
 * \param[in]       condition points to an ''fm_stormCondition'' structure
 *                  describing the condition to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_STORM_COND_EXCEEDED if adding the condition exceed the
 *                  stormController's maximum condition numbers.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_STORM_COND if condition is not compatible with
 *                  the current condition configuration.
 * \return          FM_ERR_INVALID_ARGUMENT if condition is NULL.
 *
 *****************************************************************************/
fm_status fm10000AddStormCtrlCondition(fm_int             sw,
                                       fm_int             stormController,
                                       fm_stormCondition *condition)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    fm_int                   i;
    fm_bool                  overwrite = FALSE;
    fm_uint32                rateLimiterId;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, condition(%p) = (%s, %d)\n",
                 sw, stormController, (void *) condition, 
                 fmStormCondTypeToText(condition->type), condition->param );

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    if (condition->type >= FM_STORM_COND_MAX)
    {
        err = FM_ERR_INVALID_STORM_COND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    /* Check if condition already exists */
    for (i = 0; i < scPtr->numConditions; i++)
    {
        if (scPtr->conditions[i].type == condition->type)
        {
            /* Some conditions just need to be overwritten */
            if ( (condition->type == FM_STORM_COND_INGRESS_PORTSET) ||
                 (condition->type == FM_STORM_COND_EGRESS_PORTSET) )
            {
                overwrite = TRUE;
                break;
            }
            else
            {
                /* Nothing needs to be done, exit */
                goto ABORT;
            }
        }
    }

    /* Check if the condition is compatible with the existing configuration. */
    if (!overwrite)
    {
        err = CheckConditionValid(sw, scPtr, condition->type);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

        scPtr->numConditions++;
    }

    if (scPtr->action.type == FM_STORM_ACTION_FILTER_PORTSET)
    {
        rateLimiterId = scPtr->trigRateLimiterId;
    }
    else
    {
        rateLimiterId = FM10000_INVALID_RATE_LIMITER_ID;
    }
    
    if (condition->type == FM_STORM_COND_SECURITY_VIOL_NEW_MAC)
    {
        scPtr->newMacViolCondUsed = TRUE;
        err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                    condition->type, 
                                                    rateLimiterId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }
    else if (condition->type == FM_STORM_COND_SECURITY_VIOL_MOVE)
    {
        scPtr->macMoveViolCondUsed = TRUE;
        err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                    condition->type, 
                                                    rateLimiterId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr->conditions[i] = *condition;

    err = ApplyStormCtrlConditions(sw, stormController, scPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000AddStormCtrlCondition */




/*****************************************************************************/
/** fm10000DeleteStormCtrlCondition
 * \ingroup intstorm
 *
 * \desc            Delete a condition from a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the condition
 *                  should be deleted.
 *
 * \param[in]       condition points to an ''fm_stormCondition'' structure
 *                  describing the condition to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_STORM_COND if condition does not exist
 *                  in the storm controller.
 * \return          FM_ERR_INVALID_ARGUMENT if condition is NULL.
 *
 *****************************************************************************/
fm_status fm10000DeleteStormCtrlCondition(fm_int             sw,
                                          fm_int             stormController,
                                          fm_stormCondition *condition)
{
    fm_status                err;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    fm_int                   i;
    fm_bool                  found = FALSE;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, condition(%p) = (%s, %d)\n",
                 sw, stormController, (void *) condition, 
                 fmStormCondTypeToText(condition->type), condition->param );

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    /* Check if condition exists and delete it */
    for (i = 0; i < scPtr->numConditions; i++)
    {
        if (scPtr->conditions[i].type == condition->type)
        {
            found = TRUE;

            /* If this is not the last condition, move the last
             * condition to the deleted condition slot to keep
             * conditions packed */
            if (i != (scPtr->numConditions - 1))
            {
                scPtr->conditions[i] = scPtr->conditions[scPtr->numConditions - 1];
            }

            scPtr->numConditions--;

            if (condition->type == FM_STORM_COND_SECURITY_VIOL_NEW_MAC)
            {
                scPtr->newMacViolCondUsed = FALSE;
                err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                            condition->type, 
                                                            FM10000_INVALID_RATE_LIMITER_ID);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            }
            else if (condition->type == FM_STORM_COND_SECURITY_VIOL_MOVE)
            {
                scPtr->macMoveViolCondUsed = FALSE;
                err = fm10000NotifyMacSecurityRateLimiterId(sw, 
                                                            condition->type, 
                                                            FM10000_INVALID_RATE_LIMITER_ID);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            }

            err = ApplyStormCtrlConditions(sw, stormController, scPtr);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

            break;
        }
    }

    if (!found)
    {
        err = FM_ERR_INVALID_STORM_COND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000DeleteStormCtrlCondition */




/*****************************************************************************/
/** fm10000GetStormCtrlConditionList
 * \ingroup intstorm
 *
 * \desc            Function to return the list of conditions that will
 *                  trigger a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the condition
 *                  list should be retrieved.
 *
 * \param[out]      numConditions points to caller-allocated storage where
 *                  this function should place the number of conditions
 *                  returned in conditionList.
 *
 * \param[out]      conditionList is an array that this function will fill with
 *                  the list of conditions associated with the storm controller.
 *
 * \param[in]       max is the size of conditionList, being the maximum number
 *                  of conditions that conditionList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of conditions.
 * \return          FM_ERR_INVALID_ARGUMENT if numConditions is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlConditionList(fm_int             sw,
                                           fm_int             stormController,
                                           fm_int *           numConditions,
                                           fm_stormCondition *conditionList,
                                           fm_int             max)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr;
    fm_int                   i;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, "
                 "stormController = %d, "
                 "numConditions = %p, "
                 "conditionList=%p, "
                 "max=%d \n",
                 sw, 
                 stormController,
                 (void *) numConditions, 
                 (void *) conditionList, 
                 max);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if (scPtr->numConditions >= max)
    {
        err = FM_ERR_BUFFER_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    *numConditions = scPtr->numConditions;

    for (i = 0; i < scPtr->numConditions; i++)
    {
        conditionList[i] = scPtr->conditions[i];
    }
    
ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlConditionList */




/*****************************************************************************/
/** fm10000GetStormCtrlConditionFirst
 * \ingroup intstorm
 *
 * \desc            Retrieve the first condition associated with a storm
 *                  controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which to retrieve the
 *                  first condition.
 *
 * \param[out]      firstCondition points to caller-allocated storage where
 *                  this function should place the first condition for this
 *                  storm controller.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_CONDITION if no storm controller conditions
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if firstConditions is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlConditionFirst(fm_int             sw,
                                            fm_int             stormController,
                                            fm_stormCondition *firstCondition)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, "
                 "stormController = %d, "
                 "firstCondition = %p\n",
                 sw, 
                 stormController,
                 (void *) firstCondition);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if (scPtr->numConditions == 0)
    {
        err = FM_ERR_NO_STORM_CONDITION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    *firstCondition = scPtr->conditions[0];
    
ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlConditionFirst */




/*****************************************************************************/
/** fm10000GetStormCtrlConditionNext
 * \ingroup intstorm
 *
 * \desc            Retrieve the next condition associated with a storm
 *                  controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which to retrieve the
 *                  next condition.
 *
 * \param[in]       currentCondition points to the last condition found by a
 *                  previous call to this function or to
 *                  ''fmGetStormCtrlConditionFirst''.
 *
 * \param[out]      nextCondition points to caller-allocated storage where this
 *                  function should place the next condition for this
 *                  storm controller.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_CONDITION if no more storm controller
 *                  conditions found.
 * \return          FM_ERR_INVALID_ARGUMENT if the currentCondition or
 *                  nextCondition is NULL.
 * 
 *****************************************************************************/
fm_status fm10000GetStormCtrlConditionNext(fm_int             sw,
                                           fm_int             stormController,
                                           fm_stormCondition *currentCondition,
                                           fm_stormCondition *nextCondition)
{

    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr;
    fm_int                   i;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, "
                 "stormController = %d, "
                 "currentCondition = %p, "
                 "nextCondition = %p \n",
                 sw, 
                 stormController,
                 (void *) currentCondition,
                 (void *) nextCondition);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    for (i = 0; i < scPtr->numConditions; i++)
    {
        if ( (scPtr->conditions[i].type == currentCondition->type) &&
             (scPtr->conditions[i].param == currentCondition->param) )
        {
            break;
        }
    }

    if (i == scPtr->numConditions)
    {
        /* there was no match with currentCondition */
        err = FM_ERR_NO_STORM_CONDITION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }
    else if (i == (scPtr->numConditions - 1) )
    {
        /* this is the last condition, there are none after */
        err = FM_ERR_NO_STORM_CONDITION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }
    else
    {
        *nextCondition = scPtr->conditions[i+1];
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlConditionNext */




/*****************************************************************************/
/** fm10000AddStormCtrlAction
 * \ingroup intstorm
 *
 * \desc            Add an action to a storm controller. The action dictates
 *                  what happens when a storm condition is detected by the storm
 *                  controller (the controller's token bucket limit is reached).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') to which the action should
 *                  be added.
 *
 * \param[in]       action points to an ''fm_stormAction'' structure
 *                  describing the action.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ACTION if action is not a valid stormControl
 *                  action.
 * \return          FM_ERR_INVALID_ARGUMENT if action is NULL.
 *
 *****************************************************************************/
fm_status fm10000AddStormCtrlAction(fm_int          sw,
                                    fm_int          stormController,
                                    fm_stormAction *action)
{
    fm_status                err;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, action(%p) = (%s, %d)\n",
                 sw, stormController, (void *) action, 
                 fmStormCondTypeToText(action->type), action->param );

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    if ( action->type != FM_STORM_ACTION_FILTER_PORTSET )
    {
        err = FM_ERR_INVALID_ACTION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    scPtr->action = *action;

    err = ApplyStormCtrlActions(sw, stormController, scPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000AddStormCtrlAction */




/*****************************************************************************/
/** fm10000DeleteStormCtrlAction
 * \ingroup intstorm
 *
 * \desc            Delete an action from a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the action should be
 *                  deleted.
 *
 * \param[in]       action points to an ''fm_stormAction'' structure
 *                  describing the action to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ACTION if action is not found in the storm
 *                  controller.
 * \return          FM_ERR_INVALID_ARGUMENT if action is NULL.
 *
 *****************************************************************************/
fm_status fm10000DeleteStormCtrlAction(fm_int          sw,
                                       fm_int          stormController,
                                       fm_stormAction *action)
{
    fm_status                err;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, action(%p) = (%s, %d)\n",
                 sw, stormController, (void *) action, 
                 fmStormCondTypeToText(action->type), action->param );

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if ( (action->type != FM_STORM_ACTION_FILTER_PORTSET) || 
         (scPtr->action.type != FM_STORM_ACTION_FILTER_PORTSET) )
    {
        err = FM_ERR_INVALID_ACTION;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr->action.type = FM_STORM_ACTION_DO_NOTHING;

    err = ApplyStormCtrlActions(sw, stormController, scPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000DeleteStormCtrlAction */




/*****************************************************************************/
/** fm10000GetStormCtrlActionList
 * \ingroup intstorm
 *
 * \desc            Return a list of actions associated with a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the action should be
 *                  retrieved.
 *
 * \param[out]      numActions points to caller-allocated storage where
 *                  this function should place the number of actions
 *                  returned in actionList.
 *
 * \param[out]      actionList is an array that this function will fill with
 *                  the list of actions associated with the storm controller.
 *
 * \param[in]       max is the size of actionList, being the maximum number
 *                  of actions that actionList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of actions.
 * \return          FM_ERR_INVALID_ARGUMENT if numActions or actionList is
 *                  NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlActionList(fm_int          sw,
                                        fm_int          stormController,
                                        fm_int *        numActions,
                                        fm_stormAction *actionList,
                                        fm_int          max)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, numActions = %p, "
                 "actionList = %p, max = %d\n",
                 sw,
                 stormController,
                 (void *) numActions,
                 (void *) actionList,
                 max);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    if (scPtr->action.type == FM_STORM_ACTION_DO_NOTHING)
    {
        *numActions = 0;
    }
    else if (max <= 0)
    {
        err = FM_ERR_BUFFER_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }
    else
    {
        *numActions = 1;
        actionList[0] = scPtr->action;
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlActionList */




/*****************************************************************************/
/** fm10000GetStormCtrlActionFirst
 * \ingroup intstorm
 *
 * \desc            Retrieve the action in a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the action should
 *                  be retrieved.
 *
 * \param[out]      action points to caller-allocated storage where this
 *                  function should place the action from this storm
 *                  controller. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ARGUMENT if the firstAction is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlActionFirst(fm_int          sw,
                                         fm_int          stormController,
                                         fm_stormAction *action)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, action(%p)\n",
                 sw, stormController, (void *) action);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    *action = scPtr->action;

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlActionFirst */




/*****************************************************************************/
/** fm10000GetStormCtrlActionNext
 * \ingroup intstorm
 *
 * \desc            Retrieve the next action in a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the next action
 *                  should be retrieved.
 *
 * \param[in]       currentAction points to the last action found by a previous
 *                  call to this function or to ''fmGetStormCtrlActionFirst''.
 *
 * \param[out]      nextAction points to caller-allocated storage where this
 *                  function should place the next action in the storm
 *                  controller. Will be set to -1 if no more actions found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_ACTION if no more storm controller actions
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if currentAction or nextAction is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetStormCtrlActionNext(fm_int          sw,
                                        fm_int          stormController,
                                        fm_stormAction *currentAction,
                                        fm_stormAction *nextAction)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm10000_stormController *scPtr = NULL;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d, currentAction = %p, "
                 "nextAction = %p\n",
                 sw,
                 stormController,
                 (void *) currentAction,
                 (void *) nextAction);

    FM_NOT_USED(nextAction);

    if ( (stormController >= FM10000_MAX_NUM_STORM_CTRL) ||
         (stormController < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STORM, FM_ERR_INVALID_STORM_CTRL);
    }

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    if (scInfo->used[stormController] == FM10000_STORM_CONTROLLER_UNUSED)
    {
        err = FM_ERR_INVALID_STORM_CTRL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

    scPtr = &scInfo->stormCtrl[stormController];

    /* Only one action is supported, which can be retrieved with
     * fm10000GetStormCtrlActionFirst() */
    err = FM_ERR_NO_STORM_ACTION;
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_STORM, err);

}   /* end fm10000GetStormCtrlActionNext */




/*****************************************************************************/
/** fm10000DbgDumpStormCtrl
 * \ingroup intstorm
 *
 * \desc            Dump a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl''). Use -1 to dump all storm
 *                  controllers.
 *
 * \return          NONE.
 * 
 *****************************************************************************/
void fm10000DbgDumpStormCtrl(fm_int sw, fm_int stormController)
{
    fm_status                err;
    fm10000_switch *         switchExt;
    fm10000_scInfo *         scInfo;
    fm_int                   i;

    FM_LOG_ENTRY(FM_LOG_CAT_STORM,
                 "sw = %d, stormController = %d\n",
                 sw, stormController);

    switchExt = GET_SWITCH_EXT(sw);
    scInfo    = &switchExt->scInfo;

    TAKE_STORM_LOCK(sw);

    /* Iterate on all storm controllers */
    if ( stormController < 0 )
    {
        err = DumpStormControllerCompTable();
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);

        FM_LOG_PRINT("Dumping all storm controllers:\n\n");

        for (i = 0; i < FM10000_MAX_NUM_STORM_CTRL; i++)
        {
            if (scInfo->used[i] == FM10000_STORM_CONTROLLER_RESERVED)
            {
                err = DumpStormController(sw, i);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
            }
        }
    }
    else
    {
        err = DumpStormController(sw, stormController);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STORM, err);
    }

ABORT:
    DROP_STORM_LOCK(sw);

    FM_LOG_EXIT_VOID(FM_LOG_CAT_STORM);

}
