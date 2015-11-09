/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_common_int.h
 * Creation Date:   2005
 * Description:     Contains structure definitions and constants related to the
 *                  initialization of the system and keeping system state
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

#ifndef __FM_FM_API_COMMON_INT_H
#define __FM_FM_API_COMMON_INT_H


#define FM_LOG_PORT_USE_FTYPE_NORMAL     0xFFFE

/* Defines the VLAN types (VTYPE) for the F64 ISL tag. */
#define FM_VTYPE_NONE                    0
#define FM_VTYPE_8100                    1
#define FM_VTYPE_USER_A                  2
#define FM_VTYPE_USER_B                  3

/* Defines the management types (MTYPE) for the F64 ISL tag. */
#define FM_MTYPE_REQUEST                 0
#define FM_MTYPE_RESPONSE_INTERRUPT      1
#define FM_MTYPE_RESERVED_1              2
#define FM_MTYPE_RESERVED_2              3

/* Defines the frame type (FTYPE) for the F64 ISL tag. */
#define FM_FTYPE_NORMAL                  0
#define FM_FTYPE_ROUTED                  1
#define FM_FTYPE_SPECIAL_DELIVERY        2
#define FM_FTYPE_MANAGEMENT              3


typedef struct _fm_switch    fm_switch;
typedef struct _fm_port      fm_port;
typedef struct _fm_lane      fm_lane;
typedef struct _fm_lag       fm_lag;

/* pointer to stack event handler */
extern fm_eventHandler fmEventHandler;


#define FID_OUT_OF_BOUNDS(f)   ( (f) >= FM_MAX_VLAN )

/* A VLAN of 0 is invalid, but we use it in shared VLAN
 * learning mode. */
#define VLAN_OUT_OF_BOUNDS(v)  ( (v) >= FM_MAX_VLAN || (v) <= 0 )

#define SWITCH_LOCK_EXISTS(sw)                                \
    ( ( ( (sw) >= 0 ) && ( (sw) < fmRootPlatform->cfg.numSwitches ) &&    \
       (fmRootApi->fmSwitchLockTable[sw] != NULL) ) ? TRUE : FALSE )

#define PROTECT_SWITCH(sw) \
    fmCaptureReadLock(fmRootApi->fmSwitchLockTable[sw], FM_WAIT_FOREVER)

#define UNPROTECT_SWITCH(sw) \
    fmReleaseReadLock(fmRootApi->fmSwitchLockTable[sw])

#define LOCK_SWITCH(sw) \
    fmCaptureWriteLock(fmRootApi->fmSwitchLockTable[sw], FM_WAIT_FOREVER)

#define UNLOCK_SWITCH(sw) \
    fmReleaseWriteLock(fmRootApi->fmSwitchLockTable[sw])

#define VALIDATE_SWITCH_LOCK(sw)      \
    if ( !SWITCH_LOCK_EXISTS(sw) )    \
    {                                 \
        return FM_ERR_INVALID_SWITCH; \
    }

#define VALIDATE_SWITCH_INDEX(sw)                                 \
    if ( (sw) < 0 || (sw) >= fmRootPlatform->cfg.numSwitches )    \
    {                                                             \
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,                           \
                     "VALIDATE_SWITCH_INDEX: %d not in [0,%d]\n", \
                     (sw),                                        \
                     fmRootPlatform->cfg.numSwitches);            \
        return FM_ERR_INVALID_SWITCH;                             \
    }


#define FM_TAKE_L2_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[sw]->L2Lock, FM_WAIT_FOREVER)

#define FM_DROP_L2_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[sw]->L2Lock)
    

#define ACL_OUT_OF_BOUNDS(acl) \
    ( ( (acl) < 0 ) ||         \
     ( (acl) >= FM_MAX_ACLS ) )

#define ACL_RULE_OUT_OF_BOUNDS(rule) \
    ( ( (rule) < 0 ) || ( (rule) >= FM_MAX_ACL_RULES ) )

#define VALIDATE_ACL_ID(sw, acl)   \
    if ( ACL_OUT_OF_BOUNDS(acl) )  \
    {                              \
        if (swProtected)           \
        {                          \
            UNPROTECT_SWITCH(sw);  \
        }                          \
        return FM_ERR_INVALID_ACL; \
    }

#define VALIDATE_ACL_RULE_ID(sw, rule)  \
    if ( ACL_RULE_OUT_OF_BOUNDS(rule) ) \
    {                                   \
        if (swProtected)                \
        {                               \
            UNPROTECT_SWITCH(sw);       \
        }                               \
        return FM_ERR_INVALID_ACL_RULE; \
    }


/* Call the chip family-specific implementation for an API function, but
 * only if the implementation exists. */
#define FM_API_CALL_FAMILY(err, func, ...) \
    if (func)                              \
    {                                      \
        (err) = func(__VA_ARGS__);         \
    }                                      \
    else                                   \
    {                                      \
        (err) = FM_ERR_UNSUPPORTED;        \
    }

#define FM_API_CALL_FAMILY_VOID(func, ...) \
    if (func)                              \
    {                                      \
        func(__VA_ARGS__);                 \
    }                                      \


fm_macaddr fmGetPacketDestAddr(fm_int sw, fm_buffer *pkt);
fm_macaddr fmGetPacketSrcAddr(fm_int sw, fm_buffer *pkt);


#endif /* __FM_FM_API_COMMON_INT_H */
