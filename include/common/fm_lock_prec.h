/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_lock_prec.h
 * Creation Date:   November 3, 2009
 * Description:     Specifies precedence for all locks used by the API.
 *
 * Copyright (c) 2009 - 2015, Intel Corporation
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

#ifndef __FM_FM_LOCK_PREC_H
#define __FM_FM_LOCK_PREC_H

/**************************************************
 * The following enumeration dictates the order
 * in which a thread may take these locks. A lock
 * listed earlier in the list may not be taken
 * after a lock later in the list.
 **************************************************/

enum
{
    FM_LOCK_PREC_SWITCH,                    /* fmRootApi->fmSwitchLockTable[sw] */
    FM_LOCK_PREC_MAILBOX,                   /* swstate->mailboxLock */
    FM_LOCK_PREC_NAT,                       /* swstate->natLock */
    FM_LOCK_PREC_FLOW,                      /* swstate->flowLock */
    FM_LOCK_PREC_LBGS,                      /* switchPtr->lbgInfo.lbgLock */
    FM_LOCK_PREC_ROUTING,                   /* swstate->routingLock */
    FM_LOCK_PREC_LAGS,                      /* swstate->lagLock */
    FM_LOCK_PREC_L2,                        /* swstate->L2Lock */
    FM_LOCK_PREC_PURGE,                     /* swstate->maPurgeLock */
    FM_LOCK_PREC_MATABLE_MAINT,             /* swstate->macTableMaintWorkListLock */
    FM_LOCK_PREC_ACLS,                      /* swstate->aclLock */
    FM_LOCK_PREC_TUNNEL,                    /* swstate->tunnelLock */
    FM_LOCK_PREC_PARITY,                    /* switchExt->parityLock */
    FM_LOCK_PREC_CRM,                       /* swstate->crmLock */
    FM_LOCK_PREC_MTABLE,                    /* swstate->mtableLock */
    FM_LOCK_PREC_STATE_LOCK,                /* swstate->stateLock */
    FM_LOCK_PREC_MIRROR,                    /* swstate->mirrorLock */
    FM_LOCK_PREC_TRIGGERS,                  /* swstate->triggerLock */
    FM_LOCK_PREC_FFU,                       /* switchExt->ffuAtomicAccessLock */
    FM_LOCK_PREC_PORT_SET,                  /* switchExt->portSetLock */
    FM_LOCK_PREC_SCHEDULER,                 /* switchExt->schedulerLock */
    FM_LOCK_PREC_PLATFORM,                  /* ps->accessLocks[FM_MEM_TYPE_CSR] */
    FM_LOCK_PREC_TREE_TREE,                 /* fmRootApi->treeTreeLock */

};


/**************************************************
 * Special lock precedence value that is used to
 * transcend the precedence check. Locks with this
 * precedence may be taken in any order with respect
 * to any other lock.
 **************************************************/

#define FM_LOCK_SUPER_PRECEDENCE            -1


#endif /* __FM_FM_LOCK_PREC_H */
