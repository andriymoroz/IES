/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_storm_int.h
 * Creation Date:   June 17, 2008
 * Description:     Contains functions dealing with storm controllers and
 *                  switch-aggregates.
 *
 * Copyright (c) 2008 - 2014, Intel Corporation
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

#ifndef __FM_FMSWAG_API_STORM_INT_H
#define __FM_FMSWAG_API_STORM_INT_H


fm_status fmSWAGInitStormCtrl(fm_int sw);
fm_status fmSWAGCreateStormCtrl(fm_int sw, fm_int *stormController);
fm_status fmSWAGDeleteStormCtrl(fm_int sw, fm_int stormController);
fm_status fmSWAGSetStormCtrlAttribute(fm_int sw,
                                      fm_int stormController,
                                      fm_int attr,
                                      void * value);
fm_status fmSWAGGetStormCtrlAttribute(fm_int sw,
                                      fm_int stormController,
                                      fm_int attr,
                                      void * value);
fm_status fmSWAGGetStormCtrlList(fm_int  sw,
                                 fm_int *numStormControllers,
                                 fm_int *stormControllers,
                                 fm_int  max);
fm_status fmSWAGGetStormCtrlFirst(fm_int sw, fm_int *firstStormController);
fm_status fmSWAGGetStormCtrlNext(fm_int  sw,
                                 fm_int  currentStormController,
                                 fm_int *nextStormController);
fm_status fmSWAGAddStormCtrlCondition(fm_int             sw,
                                      fm_int             stormController,
                                      fm_stormCondition *condition);
fm_status fmSWAGDeleteStormCtrlCondition(fm_int             sw,
                                         fm_int             stormController,
                                         fm_stormCondition *condition);
fm_status fmSWAGGetStormCtrlConditionList(fm_int             sw,
                                          fm_int             stormController,
                                          fm_int *           numConditions,
                                          fm_stormCondition *conditionList,
                                          fm_int             max);
fm_status fmSWAGGetStormCtrlConditionFirst(fm_int             sw,
                                           fm_int             stormController,
                                           fm_stormCondition *firstCondition);
fm_status fmSWAGGetStormCtrlConditionNext(fm_int             sw,
                                          fm_int             stormController,
                                          fm_stormCondition *currentCondition,
                                          fm_stormCondition *nextCondition);
fm_status fmSWAGAddStormCtrlAction(fm_int          sw,
                                   fm_int          stormController,
                                   fm_stormAction *action);
fm_status fmSWAGDeleteStormCtrlAction(fm_int          sw,
                                      fm_int          stormController,
                                      fm_stormAction *action);
fm_status fmSWAGGetStormCtrlActionList(fm_int          sw,
                                       fm_int          stormController,
                                       fm_int *        numActions,
                                       fm_stormAction *actionList,
                                       fm_int          max);
fm_status fmSWAGGetStormCtrlActionFirst(fm_int          sw,
                                        fm_int          stormController,
                                        fm_stormAction *firstAction);
fm_status fmSWAGGetStormCtrlActionNext(fm_int          sw,
                                       fm_int          stormController,
                                       fm_stormAction *currentAction,
                                       fm_stormAction *nextAction);


#endif /* __FM_FMSWAG_API_STORM_INT_H */
