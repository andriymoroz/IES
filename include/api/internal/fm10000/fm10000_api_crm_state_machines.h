/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_crm_state_machines.h
 * Creation Date:   May 28, 2015
 * Description:     Header file for the CRM state machine
 * 
 *                  ------------------------------------------------------
 *                  THIS FILE IS AUTO-GENERATED BY THE BUILD SYSTEM, DO
 *                  NOT MODIFY THIS FILE.  MODIFY THE FOLLOWING INSTEAD
 *                  ------------------------------------------------------
 * 
 *                  1) fm10000_api_crm_state_machines.xml
 *                  2) templates/fm10000_api_crm_state_machines.h
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

#ifndef __FM_FM10000_API_CRM_STATE_MACHINES_H
#define __FM_FM10000_API_CRM_STATE_MACHINES_H


/* declaration of the CRM state machine types */
#define FM10000_BASIC_CRM_STATE_MACHINE 500


/* declaration of CRM-level states */
typedef enum
{
    FM10000_CRM_STATE_IDLE = 0,
    FM10000_CRM_STATE_ACTIVE,
    FM10000_CRM_STATE_UPDATING,
    FM10000_CRM_STATE_REPAIRING,
    FM10000_CRM_STATE_DUPLEX,
    FM10000_CRM_STATE_RESUMING,
    FM10000_CRM_STATE_MAX

} fm10000_crmSmStates;

extern fm_text fm10000CrmStatesMap[FM10000_CRM_STATE_MAX];


/* declaration of CRM-level events */
typedef enum
{
    FM10000_CRM_EVENT_SUSPEND_REQ = 0,
    FM10000_CRM_EVENT_RESUME_REQ,
    FM10000_CRM_EVENT_FAULT_IND,
    FM10000_CRM_EVENT_REPAIR_IND,
    FM10000_CRM_EVENT_TIMEOUT_IND,
    FM10000_CRM_EVENT_MAX

} fm10000_crmSmEvents;

extern fm_text fm10000CrmEventsMap[FM10000_CRM_EVENT_MAX];

/* declaration of external counterparts of action callbacks */
fm_status fm10000CrmMaskInterrupts( fm_smEventInfo *eventInfo, void *userInfo );
fm_status fm10000CrmUpdateChecksum( fm_smEventInfo *eventInfo, void *userInfo );
fm_status fm10000CrmStartTimer( fm_smEventInfo *eventInfo, void *userInfo );
fm_status fm10000CrmCancelTimer( fm_smEventInfo *eventInfo, void *userInfo );
fm_status fm10000CrmUnmaskInterrupts( fm_smEventInfo *eventInfo, void *userInfo );



/* declaration of external counterparts of condition callbacks */
fm_status fm10000CrmDummy( fm_smEventInfo *eventInfo, void *userInfo, fm_int *nextState );



/* declaration of the log callback functions, if any */
fm_status fm10000LogCrmTransition( fm_smTransitionRecord *record );

/* declaration of registration functions for serdes-level state machine types */
fm_status fm10000RegisterBasicCrmStateMachine( void );

#endif /* __FM_FM10000_API_CRM_STATE_MACHINES_H */
