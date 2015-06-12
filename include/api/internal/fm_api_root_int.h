/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_api_root_int.h
 * Creation Date:  June 21, 2007
 * Description:    Structure containing API's global variables
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

#ifndef __FM_FM_API_ROOT_INT_H
#define __FM_FM_API_ROOT_INT_H

#ifndef FM_GLOBAL_DECL 
#define FM_GLOBAL_DECL 
#endif

typedef enum
{
    TIB_INITIAL = 0,
    TIB_BCAST_ON,
    TIB_BCAST_OFF,
    TIB_DISABLE,

    /* Add new states above this line */
    TIB_MAX_STATE

} fm_TIBState;

typedef struct _fm_rootApi
{
    /**************************************************
     * fm_api_init.c
     **************************************************/
     
    /* global storage for mac address that is to be traced for debugging */
    /* Due to the way this feature was implemented, it is not possible to trace
     *  MAC Address 00:00:00:00:00:00 */
    fm_macaddr          testTraceMacAddress;

    /* this thread receives all non MA table related events */
    fm_thread           eventThread;

    /* Interrupt-Processing Thread */
    fm_thread           interruptTask;

    /* semaphore for interrupt signaling */
    fm_semaphore        intrAvail;

    /* Interrupt-Processing Thread for remote switches */
    fm_thread           fibmSlaveInterruptTask;

    /* semaphore for interrupt signaling for remote switches */
    fm_semaphore        fibmSlaveIntrAvail;

    /* semaphore to trigger packet transmission on NIC */
    fm_semaphore        nicPacketTxSemaphore;

    /* flags to indicate remote switch */
    fm_bool             isSwitchFibmSlave[FM_MAX_NUM_SWITCHES];

    /* MAC Table Maintenance Thread */
    fm_thread           maintenanceTask;

    /* Fast Maintenance Thread */
    fm_thread           fastMaintenanceTask;

    /* Link-State Debounce Thread */
    fm_thread           debounceTask;

    /* timer task */
    fm_thread           timerTask;

    /* absolute time when the API was initialized */
    fm_timestamp        apiInitTime;

    /* Thread for handling packet reception */
    fm_thread           packetReceiveTask;

    /* Routing Maintenance Thread */
    fm_thread           routingMaintenanceTask;

    /* semaphore to trigger packet reception */
    fm_semaphore        packetReceiveSemaphore;

    /* semaphore to wait on buffer shortage */
    fm_semaphore        waitForBufferSemaphore;

    /**************************************************
     * fm_api_parity.c
     **************************************************/
    /* Parity sweeper thread */
    fm_thread           paritySweeperTask;

    fm_thread           parityRepairTask;
    fm_semaphore        parityRepairSemaphore;

    /**************************************************
     * fm_api_glob.c
     **************************************************/
    /* list of switch state pointers */
    fm_switch *         fmSwitchStateTable[FM_MAX_NUM_SWITCHES];

    /* list of switch pointer read/write locks */
    fm_rwLock *         fmSwitchLockTable[FM_MAX_NUM_SWITCHES];

    /* list of pointers for registers cache */
    void *              fmSwRegCache[FM_MAX_NUM_SWITCHES];

    /**************************************************
     * fm_api_event_mgmt.c
     **************************************************/
    /* the free event queue */
    fm_eventQueue       fmEventFreeQueue;

    /* the semaphore used for throttling low priority events */
    fm_semaphore        fmLowPriorityEventSem;

    /**************************************************
     * fm_api_event_mac_maint.c
     **************************************************/
    /* semaphore to wake up table maintenance thread when there is work to do */
    fm_semaphore        macTableMaintSemaphore;

    /* Diagnostics */
    fm_int              macTableMaintMaxTasks[FM_MAX_NUM_SWITCHES];

#if defined(FM_SUPPORT_FM2000)
    /**************************************************
     * fm2000_api_port.c
     **************************************************/
    fm_int              executeFlag;

    /**************************************************
     * fm2000_api_pkt_rx.c
     **************************************************/
    fm_timestamp        lastTime[FM_MAX_NUM_SWITCHES];
    fm_uint64           lastBcastCount[FM_MAX_NUM_SWITCHES];
    fm_TIBState         state[FM_MAX_NUM_SWITCHES];

    /**************************************************
     * fm2000_api_attr.c
     **************************************************/
    fm_macAddressEntry *tempMacTable;
#endif

    /**************************************************
     * fm_api_event_handler.c
     **************************************************/
    /* list of fm_localDelivery, one for each process */
    fm_dlist            localDeliveryThreads;

    /* number of threads in the above list */
    fm_uint             localDeliveryCount;

    /* lock for the above list and count */
    fm_lock             localDeliveryLock;

    /* semaphore to start the global event handler thread */
    fm_semaphore        startGlobalEventHandler;

    /* Pointer to MAC hashing table that must be shared between processes */
    fm_int *            l2lHashTable;

} fm_rootApi;

extern FM_GLOBAL_DECL fm_rootApi *fmRootApi;

#define fmApiTimerTask &fmRootApi->timerTask

#endif /* __FM_FM_API_ROOT_INT_H */
