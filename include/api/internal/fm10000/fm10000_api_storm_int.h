/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_storm_int.h
 * Creation Date:   October 15, 2013
 * Description:     Prototypes and structure definitions for storm controllers.
 *
 * Copyright (c) 2007 - 2013, Intel Corporation
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

#ifndef __FM_FM10000_API_STORM_INT_H
#define __FM_FM10000_API_STORM_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* The maximum number of storm controllers */
#define FM10000_MAX_NUM_STORM_CTRL          16

/* The maximum number of match conditions that can be combined
 * for a storm controller. */
#define FM10000_MAX_STORM_CONDITION         FM_STORM_COND_MAX

/* Structure that holds a storm controller entry */
typedef struct _fm10000_stormController
{
    /* Trigger Rate Limiter Ressource ID */
    fm_uint32         trigRateLimiterId;

    /* List of all conditions added to this storm controller */
    fm_stormCondition conditions[FM10000_MAX_STORM_CONDITION];

    /* Tracks the number of conditions */
    fm_int            numConditions;

    /* Action of this storm controller */
    fm_stormAction    action;

    /* Tracks usage of the FM_STORM_COND_SECURITY_VIOL_NEW_MAC condition.
     * it can only be used in one storm controller. */
    fm_bool           newMacViolCondUsed;

    /* Tracks usage of the FM_STORM_COND_SECURITY_VIOL_MOVE condition.
     * it can only be used in one storm controller. */
    fm_bool           macMoveViolCondUsed;

} fm10000_stormController;

/* Structure that tracks all storm controllers */
typedef struct fm10000_scInfo
{
    /* Boolean to track usage of each storm controller */
    fm_bool                 used[FM10000_MAX_NUM_STORM_CTRL];

    /* Structure that tracks each storm controller */
    fm10000_stormController stormCtrl[FM10000_MAX_NUM_STORM_CTRL];

} fm10000_scInfo;


/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/

fm_status fm10000InitStormControllers(fm_switch * switchPtr);

fm_status fm10000CreateStormCtrl(fm_int sw, fm_int *stormController);

fm_status fm10000DeleteStormCtrl(fm_int sw, fm_int stormController);

fm_status fm10000SetStormCtrlAttribute(fm_int sw, 
                                       fm_int stormController, 
                                       fm_int attr, 
                                       void *value);

fm_status fm10000GetStormCtrlAttribute(fm_int sw, 
                                       fm_int stormController, 
                                       fm_int attr, 
                                       void *value);

fm_status fm10000GetStormCtrlList(fm_int sw, 
                                  fm_int *numStormControllers, 
                                  fm_int *stormControllers, 
                                  fm_int max);

fm_status fm10000GetStormCtrlFirst(fm_int sw, fm_int *firstStormController);

fm_status fm10000GetStormCtrlNext(fm_int sw, 
                                  fm_int currentStormController, 
                                  fm_int *nextStormController);

fm_status fm10000AddStormCtrlCondition(fm_int sw, 
                                       fm_int stormController, 
                                       fm_stormCondition *condition);

fm_status fm10000DeleteStormCtrlCondition(fm_int sw, 
                                          fm_int stormController, 
                                          fm_stormCondition *condition);

fm_status fm10000GetStormCtrlConditionList(fm_int sw, 
                                           fm_int stormController, 
                                           fm_int *numConditions,
                                           fm_stormCondition *conditionList, 
                                           fm_int max);

fm_status fm10000GetStormCtrlConditionFirst(fm_int sw, 
                                            fm_int stormController, 
                                            fm_stormCondition *firstCondition);

fm_status fm10000GetStormCtrlConditionNext(fm_int sw, 
                                           fm_int stormController, 
                                           fm_stormCondition *currentCondition,
                                           fm_stormCondition *nextCondition);

fm_status fm10000AddStormCtrlAction(fm_int sw, 
                                    fm_int stormController, 
                                    fm_stormAction *action);

fm_status fm10000DeleteStormCtrlAction(fm_int sw, 
                                       fm_int stormController, 
                                       fm_stormAction *action);

fm_status fm10000GetStormCtrlActionList(fm_int          sw,
                                        fm_int          stormController,
                                        fm_int *        numActions,
                                        fm_stormAction *actionList,
                                        fm_int          max);

fm_status fm10000GetStormCtrlActionFirst(fm_int sw, 
                                         fm_int stormController, 
                                         fm_stormAction *action);

fm_status fm10000GetStormCtrlActionNext(fm_int          sw,
                                        fm_int          stormController,
                                        fm_stormAction *currentAction,
                                        fm_stormAction *nextAction);

void fm10000DbgDumpStormCtrl(fm_int sw, fm_int stormController);

#endif /* __FM_FM10000_API_STORM_INT_H */
