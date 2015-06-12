/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug.c
 * Creation Date:   April 27, 2006
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#include <fm_sdk_int.h>
#include <common/fm_version.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#ifndef FM_BUILD_IDENTIFIER
#define FM_BUILD_IDENTIFIER            "<UNKNOWN>"
#endif

#define TAKE_DBG_LOCK() \
    fmCaptureLock(&fmRootDebug->fmDbgLock, FM_WAIT_FOREVER);

#define DROP_DBG_LOCK() \
    fmReleaseLock(&fmRootDebug->fmDbgLock);

#if 0
#define MIN_MAC_FLUSH_THRESH           0.00000001
#endif


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

fm_rootDebug *fmRootDebug;


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

fm_int  trackFuncInit = 0;
fm_int  trackPreInitCall = 0;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

#if 0
void fmDbgInitMacFlushStat();
void fmDbgUpdateMacFlushTime(int addrCount);
#endif

static fm_status DbgDiagCountInitialize(void);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/**********************************************************************
 * fmDebugRootInit
 *
 * \desc            Initializes the diagnostic root structure.
 *
 * \return          Nothing.
 *
 **********************************************************************/
static fm_status fmDebugRootInit(void)
{
    fm_status err = FM_OK;

    fmRootDebug = fmAlloc( sizeof(fm_rootDebug) );

    if (fmRootDebug == NULL)
    {
        return FM_ERR_NO_MEM;
    }

    memset( fmRootDebug, 0, sizeof(fm_rootDebug) );

#ifdef FM_DBG_NEED_TRACK_FUNC
    fmRootDebug->trace = fmAlloc( sizeof(fm_debugTrace) );

    if (fmRootDebug->trace == NULL)
    {
        return FM_ERR_NO_MEM;
    }

    memset( fmRootDebug->trace, 0, sizeof(fm_debugTrace) );
    
    trackFuncInit = 1;
    
#endif

    err = fmCreateLockV2("fmDbgLock", 
                         FM_LOCK_SWITCH_NONE,
                         FM_LOCK_SUPER_PRECEDENCE,
                         &fmRootDebug->fmDbgLock);

    if (err != FM_OK)
    {
        return err;
    }

    fmDbgInitTrace();
    fmDbgInitSnapshots();
    fmDbgInitEyeDiagrams();
#if 0
    fmDbgInitMacFlushStat();
#endif

    if ( ( err = DbgDiagCountInitialize() ) != FM_OK )
    {
        return err;
    }

    return err;

}   /* end fmDebugRootInit */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/**********************************************************************
 * fmDbgInitialize
 *
 * Description: performs debug facility initializations
 *
 * Arguments:   none
 *
 * Returns:     nothing
 *
 **********************************************************************/
fm_status fmDbgInitialize(void)
{
    return fmGetRoot("debug", (void **) &fmRootDebug, fmDebugRootInit);

}   /* end fmDbgInitialize */




/*****************************************************************************/
/** fmDbgMapLogicalPortToPhysical
 * \ingroup intDiag
 *
 * \desc            Maps a logical port number to a physical port number
 *                  on this switch. Intended for debugging use only.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logPort is the logical port number to be mapped
 *
 * \param[in]       physPort points to caller-allocated storage where this
 *                  function should place the corresponding physical port
 *                  number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgMapLogicalPortToPhysical(fm_int sw, 
                                        fm_int logPort, 
                                        fm_int *physPort)
{
    return ( fmMapLogicalPortToPhysical(GET_SWITCH_PTR(sw),
                                        (fm_uint) logPort,
                                        physPort) );

}   /* end fmDbgMapLogicalPortToPhysical */




/**********************************************************************
 * fmDbgMapPhysicalPortToLogical
 *
 * Description: Maps a physical port number to a logical port.
 *              Intended for debugging use only.
 *
 * Arguments:   switchNum identifies the switch, physPort specifies the
 *              physical port.
 *
 * Returns:     Logical port number associated with physPort.
 *
 **********************************************************************/
fm_int fmDbgMapPhysicalPortToLogical(fm_int switchNum, fm_int physPort)
{
    fm_int logPort;

    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(switchNum),
                               (fm_uint) physPort,
                               &logPort);

    return logPort;

}   /* end fmDbgMapPhysicalPortToLogical */




/*****************************************************************************/
/** fmDbgMapLogicalPort
 * \ingroup intDiag
 *
 * \desc            Generic logical port mapping function. It maps the given
 *                  logical port according to the mappingType specification.
 *                  The goal of this function is mostly to be used for low
 *                  level mappings, such as logical ports to serdes address,
 *                  which are required by some debug functions.
 *                  In some cases that the mapped value is returned as a
 *                  tuple: one element is encoded in the lower 16 bits and the
 *                  other one in the upper 16 bits, as follows:
 *                   -FM_MAP_LOGICAL_PORT_TO_PHYSICAL_PORT:
 *                      *pMapped[0..15]:  physical port
 *                      *pMapped[16..31]: physical switch
 *                   -FM_MAP_LOGICAL_PORT_TO_SERDES_ADDRESS:
 *                      *pMapped[0..15]:  serdes sbus address
 *                      *pMapped[16..31]: ring (1:EPL ring; 0:PCIe ring)
 *                   -FM_MAP_LOGICAL_PORT_TO_EPL_ABS_LANE:
 *                      *pMapped[0..15]:  epl
 *                      *pMapped[0..15]:  absolute lane   
 *                   -FM_MAP_LOGICAL_PORT_TO_EPL_CHANNEL:
 *                      *pMapped[0..15]:  epl
 *                      *pMapped[0..15]:  channel   
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logPort is the logical port number to be mapped
 * 
 * \param[in]       lane is the relative number of the lane for multi-lane
 *                  ports. It must be specified as 0 for single lane ports.
 *
 * \param[in]       mappingType specifies the required mapping. See
 *                  ''fmLogPortMappingType''.
 * 
 * \param[out]      pMapped points to caller-allocated storage where this
 *                  function should place the mapped value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if pMapped table is NULL.
 *
 *****************************************************************************/
fm_status fmDbgMapLogicalPort(fm_int                 sw,
                              fm_int                 logPort,
                              fm_int                 lane,
                              fm_logPortMappingType  mappingType,
                              fm_int                *pMapped)
{
    fm_status   err;
    fm_switch  *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    PROTECT_SWITCH(sw);
    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgMapLogicalPort,
                        sw,
                        logPort,
                        lane,
                        mappingType,
                        pMapped)

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgMapLogicalPort */




/******************************************************************************
 * Diagnostic counter API
 *****************************************************************************/

fm_status DbgDiagCountInitialize(void)
{
    /* clear all diagnostics */
    memset( fmRootDebug->fmSwitchDiagnostics, 0, sizeof(fmRootDebug->fmSwitchDiagnostics) );
    memset( &fmRootDebug->fmGlobalDiagnostics, 0, sizeof(fmRootDebug->fmGlobalDiagnostics) );
    return FM_OK;

}   /* end DbgDiagCountInitialize */




/*****************************************************************************/
/** fmDbgDumpDriverCounts
 * \ingroup intDiagTrackingStats
 *
 * \desc            Display statistics that track the flow of information
 *                  between the driver, the Adaptation Layer to Protocol Stack
 *                  and the application (protocol stack).  Information that is
 *                  tracked includes Ethernet packet flow (chunk allocations,
 *                  fp_pkt queue activity, and fm_buffer allocations).
 *
 * \param[in]       sw identifies the switch for which to display statistics.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpDriverCounts(fm_int sw)
{
    fm_switchDiagnostics diags;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();


    FM_LOG_PRINT("================== Rx Packets ==============\n");
    FM_LOG_PRINT("Rx pkt allocations         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_ALLOCS]);
    FM_LOG_PRINT("Rx pkt frees               : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_FREES]);
    FM_LOG_PRINT("Rx pkt array full          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_ARRAY_FULL]);
    FM_LOG_PRINT("Rx frame too large         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_FRAME_TOO_LARGE]);
    FM_LOG_PRINT("Rx pkt complete            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_COMPLETE]);
    FM_LOG_PRINT("Rx pkt FIBM                : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_FIBM]);
    FM_LOG_PRINT("Rx user received pkt       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_USER_PKT_SEEN]);
    FM_LOG_PRINT("Rx user pkt drops security : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_SECURITY]);
    FM_LOG_PRINT("Rx user pkt drops SV event : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_SV_EVENT]);
    FM_LOG_PRINT("Rx user pkt drops STP      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_STP]);
    FM_LOG_PRINT("Rx user pkt drops LACP     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_LACP]);
    FM_LOG_PRINT("Rx user pkt drops no port  : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_NO_PORT]);
    FM_LOG_PRINT("Rx user pkt drops no event : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_NO_EVENT]);
    FM_LOG_PRINT("Rx user pkt drops zero len : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_NO_DATA]);
    FM_LOG_PRINT("Rx API pkt drops           : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_API_PKT_DROPS]);
    FM_LOG_PRINT("Rx API pkt forwarded       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_API_PKT_FWD]);
    FM_LOG_PRINT("Rx API pkt drops for error : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_PKT_DROPS_FOR_ERROR]);
    FM_LOG_PRINT("Rx appl received pkt       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_APPL_PKT_SEEN]);
    FM_LOG_PRINT("Rx appl frame forwarded    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_APPL_FRM_FWD]);
    FM_LOG_PRINT("Rx appl frame drops        : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_APPL_FRM_DROPS]);

    FM_LOG_PRINT("================== Tx Packets ==============\n");
    FM_LOG_PRINT("Tx appl pkt forwarded      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_APPL_PKT_FWD]);
    FM_LOG_PRINT("Tx frame too large         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_FRAME_TOO_LARGE]);
    FM_LOG_PRINT("Tx pkt array full          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_PKT_ARRAY_FULL]);
    FM_LOG_PRINT("Tx pkt complete            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_PKT_COMPLETE]);
    FM_LOG_PRINT("Tx pkt drop                : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_PKT_DROP]);

    FM_LOG_PRINT("================ Dispatches ================\n");
    FM_LOG_PRINT("Rx Request Msgs            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_REQ_MSG]);
    FM_LOG_PRINT("Rx LCI timeout             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_RX_LCI_TIMEOUT]);

    FM_LOG_PRINT("Tx LCI Interrupts          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_LCI_INT]);
    FM_LOG_PRINT("Tx Request Msgs            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_REQ_MSG]);
    FM_LOG_PRINT("Tx LCI timeout             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TX_LCI_TIMEOUT]);

    FM_LOG_PRINT("User driver dispatch       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_USER_DISPATCH]);
    FM_LOG_PRINT("User interrupt ioctl error : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_USER_IOCTL_ERROR]);
    FM_LOG_PRINT("User interrupt ioctl okay  : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_USER_IOCTL_RETURN]);

    return FM_OK;

}   /* end fmDbgDumpDriverCounts */




/*****************************************************************************/
/** fmDbgDumpMATableCounts
 * \ingroup intDebug
 *
 * \desc            Displays debug counters for the MA Table.
 *
 * \note            The caller must ensure that the diagnostic counter
 *                  system has been inititialized and locked.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpMATableCounts(fm_int sw)
{
    fm_switchDiagnostics diags;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();

    FM_LOG_PRINT("============= MA Learning Events ===========\n");
    FM_LOG_PRINT("Learned (LEARNED event)    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_LEARN_LEARNED]);
    FM_LOG_PRINT("Learned (API request)      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_API_LEARNED]);
    FM_LOG_PRINT("Total addresses learned    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_LEARN_LEARNED] +
                 diags.counters[FM_CTR_MAC_API_LEARNED]);
    FM_LOG_PRINT("Learns reported            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_REPORT_LEARN]);
    FM_LOG_PRINT("Learn reports discarded    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_LEARN_DISCARDED]);

    FM_LOG_PRINT("============= MA Aging Events ==============\n");
    FM_LOG_PRINT("Aged (LEARNED event)       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_LEARN_AGED]);
    FM_LOG_PRINT("Aged (API request)         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_API_AGED]);
    FM_LOG_PRINT("Aged (PURGE request)       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_PURGE_AGED]);
    FM_LOG_PRINT("Total addresses aged       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_LEARN_AGED] +
                 diags.counters[FM_CTR_MAC_API_AGED] +
                 diags.counters[FM_CTR_MAC_PURGE_AGED]);
    FM_LOG_PRINT("Ages reported              : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_REPORT_AGE]);

    FM_LOG_PRINT("============= Other MA Events ==============\n");
    FM_LOG_PRINT("Port change (LEARNED event): %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_LEARN_PORT_CHANGED]);
    FM_LOG_PRINT("Cache entries deleted      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_CACHE_DELETED]);

    FM_LOG_PRINT("============= MA Table Errors ==============\n");
    FM_LOG_PRINT("Duplicate cache entries    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_CACHE_DUP]);
    FM_LOG_PRINT("Security violations        : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_SECURITY]);
    FM_LOG_PRINT("MA Table read errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_READ_ERR]);
    FM_LOG_PRINT("MA Table write errors      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WRITE_ERR]);
    FM_LOG_PRINT("MA Table port errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_PORT_ERR]);
    FM_LOG_PRINT("MA Table VLAN errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_VLAN_ERR]);
    FM_LOG_PRINT("Event allocation errors    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_EVENT_ALLOC_ERR]);
    FM_LOG_PRINT("Event send errors          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_EVENT_SEND_ERR]);

    FM_LOG_PRINT("============= TCN FIFO Interrupts ==========\n");
    FM_LOG_PRINT("TCN Interrupts             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_INTERRUPT]);
    FM_LOG_PRINT("TCN PendingEvents          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_PENDING_EVENTS]);
    FM_LOG_PRINT("TCN Learned overflows      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_LEARNED_OVERFLOW]);

    FM_LOG_PRINT("============= TCN FIFO Events ==============\n");
    FM_LOG_PRINT("TCN Learned events         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_LEARNED_EVENT]);
    FM_LOG_PRINT("TCN SecViolMoved events    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_SEC_VIOL_MOVED_EVENT]);

    FM_LOG_PRINT("============= TCN FIFO Errors ==============\n");
    FM_LOG_PRINT("TCN PTR read errors        : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_PTR_READ_ERR]);
    FM_LOG_PRINT("TCN PTR write errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_PTR_WRITE_ERR]);
    FM_LOG_PRINT("TCN FIFO read errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_FIFO_READ_ERR]);
    FM_LOG_PRINT("TCN FIFO parity errors     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_FIFO_PARITY_ERR]);
    FM_LOG_PRINT("TCN FIFO conversion errors : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_TCN_FIFO_CONV_ERR]);

    FM_LOG_PRINT("============= Table Flush Requests =========\n");
    FM_LOG_PRINT("All Address Delete         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_DELETE_ALL]);
    FM_LOG_PRINT("All Dynamic Address Delete : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_DELETE_ALL_DYNAMIC]);
    FM_LOG_PRINT("All Dynamic Address Flush  : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_FLUSH_ALL_DYNAMIC]);
    FM_LOG_PRINT("Port Address Flush         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_FLUSH_PORT]);
    FM_LOG_PRINT("Vlan Address Flush         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_FLUSH_VLAN]);
    FM_LOG_PRINT("Vlan Port Address Flush    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_FLUSH_PORT_VLAN]);

    FM_LOG_PRINT("============= Table Purge Events ===========\n");
    FM_LOG_PRINT("Purges Requested           : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PURGE_REQ]);
    FM_LOG_PRINT("Purges Executed            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PURGE_EX]);
    FM_LOG_PRINT("Purges Completed           : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PURGE_COMPLETE]);

    FM_LOG_PRINT("============= MAC Maint Requests ===========\n");
    FM_LOG_PRINT("Poll Count                 : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_POLL_COUNT]);
    FM_LOG_PRINT("Total Tasks                : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_TOTAL_TASKS]);
    FM_LOG_PRINT("Update Overflows           : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_UPD_OVFLW]);
    FM_LOG_PRINT("Periodic Scans             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_PERIODIC_SCAN]);
    FM_LOG_PRINT("Purge Requests             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_HANDLE_PURGE]);
    FM_LOG_PRINT("Remote Address Refreshes   : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_REFRESH_REMOTE]);
    FM_LOG_PRINT("Port ACL Updates           : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_PORT_ACL_UPDATE]);
    FM_LOG_PRINT("Dynamic Address Flushes    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_FLUSH_DYN_ADDR]);
    FM_LOG_PRINT("Port Address Flushes       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_PORT_ADDR_FLUSH]);
    FM_LOG_PRINT("Vlan Address Flushes       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_VLAN_ADDR_FLUSH]);
    FM_LOG_PRINT("Vlan Port Address Flushes  : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_VLAN_PORT_ADDR_FLUSH]);
    FM_LOG_PRINT("MAC FIFO Services          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_SERVICE_FIFO]);
    FM_LOG_PRINT("MAC FIFO Events            : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_MAC_WORK_FIFO_EVENTS]);

    return FM_OK;

}   /* end fmDbgDumpMATableCounts */




/*****************************************************************************/
/** fmDbgDumpLinkChangeCounts
 * \ingroup debugInt
 *
 * \desc            Displays debug counters for link status change.
 *
 * \note            The caller must ensure that the diagnostic counter
 *                  system has been inititialized and locked.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpLinkChangeCounts(fm_int sw)
{
    fm_switchDiagnostics diags;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();

    FM_LOG_PRINT("============= Link Status ===========\n");
    FM_LOG_PRINT("Link Change                : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_LINK_CHANGE_EVENT]);
    FM_LOG_PRINT("Link Change No Events      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_LINK_CHANGE_OUT_OF_EVENTS]);


    return FM_OK;

}   /* end fmDbgDumpLinkChangeCounts */




/*****************************************************************************/
/** fmDbgDumpMacSecurityCounts
 * \ingroup debugInt
 * 
 * \chips           FM10000
 *
 * \desc            Displays MAC security debug counters.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpMacSecurityCounts(fm_int sw)
{
    fm_switchDiagnostics diags;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();

    FM_LOG_PRINT("============ MAC Security ===========\n");

    FM_LOG_PRINT("Unknown SMAC Events        : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_SECURITY_UNKNOWN_SMAC_EVENTS]);

    FM_LOG_PRINT("Unknown SMAC Traps         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_SECURITY_UNKNOWN_SMAC_TRAPS]);

    FM_LOG_PRINT("Non-Secure SMAC Events     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_SECURITY_NON_SECURE_SMAC_EVENTS]);

    FM_LOG_PRINT("Non-Secure SMAC Traps      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_SECURITY_NON_SECURE_SMAC_TRAPS]);

    FM_LOG_PRINT("Secure SMAC Events         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_SECURITY_SECURE_SMAC_EVENTS]);

    FM_LOG_PRINT("Secure SMAC Traps          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_SECURITY_SECURE_SMAC_TRAPS]);

    return FM_OK;

}   /* end fmDbgDumpMacSecurityCounts */




/*****************************************************************************/
/** fmDbgDumpMiscEventCounts
 * \ingroup debugInt
 *
 * \desc            Displays miscellaneous event counters.
 *
 * \note            The caller must ensure that the diagnostic counter
 *                  system has been inititialized and locked.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpMiscEventCounts(fm_int sw)
{
    fm_switchDiagnostics diags;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();

    FM_LOG_PRINT("============= Timestamp Events ===========\n");
    FM_LOG_PRINT("Egress Timestamps          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_EGRESS_TIMESTAMP_EVENT]);
    FM_LOG_PRINT("Egress Timestamps Lost     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_EGRESS_TIMESTAMP_LOST]);


    return FM_OK;

}   /* end fmDbgDumpMiscEventCounts */




/*****************************************************************************/
/** fmDbgDumpParityErrorCounts
 * \ingroup intDebug
 *
 * \chips           FM10000
 *
 * \desc            Displays debug counters for the parity API.
 *
 * \note            The caller must ensure that the diagnostic counter
 *                  system has been inititialized and locked.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpParityErrorCounts(fm_int sw)
{
    fm_switchDiagnostics diags;
    fm_switchInfo info;
    fm_status err;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    err = fmGetSwitchInfoInternal(sw, &info);
    if (err != FM_OK)
    {
        return err;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();

    FM_LOG_PRINT("============= Parity Error Area counters ===========\n");

    FM_LOG_PRINT("ARRAY memory errors        : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_ARRAY]);
    FM_LOG_PRINT("CROSSBAR memory errors     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_CROSSBAR]);
    FM_LOG_PRINT("EPL memory errors          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_EPL]);
    FM_LOG_PRINT("FH_HEAD memory errors      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_FH_HEAD]);
    FM_LOG_PRINT("FH_TAIL memory errors      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_FH_TAIL]);
    FM_LOG_PRINT("FREELIST memory errors     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_FREELIST]);
    FM_LOG_PRINT("MGMT memory errors         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_MGMT]);
    FM_LOG_PRINT("MODIFY memory errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_MODIFY]);
    FM_LOG_PRINT("PARSER memory errors       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_PARSER]);
    FM_LOG_PRINT("POLICER memory errors      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_POLICER]);
    FM_LOG_PRINT("REFCOUNT memory errors     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_REFCOUNT]);
    FM_LOG_PRINT("SCHEDULER memory errors    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_SCHEDULER]);
    FM_LOG_PRINT("TCAM memory errors         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_TCAM]);
    FM_LOG_PRINT("TE memory errors           : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_AREA_TUNNEL_ENGINE]);

    FM_LOG_PRINT("============= Parity Error Severity counters ===========\n");

    FM_LOG_PRINT("Transient parity errors    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_SEVERITY_TRANSIENT]);
    FM_LOG_PRINT("Repairable parity errors   : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_SEVERITY_REPAIRABLE]);
    FM_LOG_PRINT("Cumulative parity errors   : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_SEVERITY_CUMULATIVE]);
    FM_LOG_PRINT("Fatal parity errors        : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_SEVERITY_FATAL]);

    FM_LOG_PRINT("============= Miscellaneous counters ===========\n");

    FM_LOG_PRINT("Parity events lost         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_EVENT_LOST]);

    return FM_OK;

}   /* end fmDbgDumpParityErrorCounts */




/*****************************************************************************/
/** fmDbgDumpParityRepairCounts
 * \ingroup intDebug
 *
 * \chips           FM10000
 *
 * \desc            Displays debug counters for the parity repair task.
 *
 * \note            The caller must ensure that the diagnostic counter
 *                  system has been inititialized.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpParityRepairCounts(fm_int sw)
{
    fm_switchDiagnostics diags;
    fm_switchInfo info;
    fm_status err;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    err = fmGetSwitchInfoInternal(sw, &info);
    if (err != FM_OK)
    {
        return err;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmSwitchDiagnostics[sw];

    DROP_DBG_LOCK();

    FM_LOG_PRINT("============= Parity Repair Error counters ===========\n");

    FM_LOG_PRINT("Repair task dispatches     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_REPAIR_DISPATCH]);
    FM_LOG_PRINT("Successful repairs         : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_STATUS_FIXED]);
    FM_LOG_PRINT("Unsuccessful repairs       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_STATUS_FIX_FAILED]);
    FM_LOG_PRINT("Invalid repair requests    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_CTR_PARITY_REPAIR_INVALID]);

    return FM_OK;

}   /* end fmDbgDumpParityRepairCounts */




/*****************************************************************************/
/** fmDbgDumpParitySweeperCounts
 * \ingroup intDebug
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Displays debug counters for the parity sweeper.
 *
 * \note            The caller must ensure that the diagnostic counter
 *                  system has been inititialized and locked.
 *
 * \param[in]       sw specifies the switch on which to operate.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDumpParitySweeperCounts(fm_int sw)
{

    return fmDbgDumpParityErrorCounts(sw);

}   /* end fmDbgDumpParitySweeperCounts */




/*****************************************************************************/
/** fmDbgDiagCountDump 
 * \ingroup diagTrackingStats 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display statistics that track the flow of information
 *                  between the driver, the Adaptation Layer to Protocol Stack
 *                  and the application (protocol stack).  Information that is
 *                  tracked includes Ethernet packet flow (chunk allocations,
 *                  fp_pkt queue activity, fm_buffer allocations) and MA Table
 *                  age and learn events.
 *
 * \param[in]       sw identifies the switch for which to display statistics.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDiagCountDump(fm_int sw)
{
    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (fmRootDebug == NULL)
    {
        UNPROTECT_SWITCH(sw);

        return FM_ERR_UNSUPPORTED;
    }

    fmDbgDumpDriverCounts(sw);
    fmDbgDumpMATableCounts(sw);
    fmDbgDumpLinkChangeCounts(sw);
    fmDbgDumpMiscEventCounts(sw);

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDiagCountDump */




/*****************************************************************************/
/** fmDbgDiagCountClear
 * \ingroup diagTrackingStats 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Reset a statistic in the set of statistics that track
 *                  the flow of information between the driver, the Adaptation
 *                  Layer to Protocol Stack and the application (protocol
 *                  stack).
 *
 * \param[in]       sw identifies the switch to clear the counter for.
 *
 * \param[in]       counter identifies the counter to be reset.
 *
 * \return          FM_OK if the counter is reset.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDiagCountClear(fm_int sw, fm_trackingCounterIndex counter)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    fmRootDebug->fmSwitchDiagnostics[sw].counters[counter] = 0;

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgDiagCountClear */




/*****************************************************************************/
/** fmDbgDiagCountClearAll
 * \ingroup diagTrackingStats 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Resets all statistics in the set of statistics that track
 *                  the flow of information between the driver, the Adaptation
 *                  Layer to Protocol Stack and the application (protocol
 *                  stack).
 *
 * \param[in]       sw identifies the switch to reset the counters for.
 *
 * \return          FM_OK if the counters are reset.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDiagCountClearAll(fm_int sw)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    memset( fmRootDebug->fmSwitchDiagnostics[sw].counters, 0,
           sizeof(fmRootDebug->fmSwitchDiagnostics[sw].counters) );

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgDiagCountClearAll */




/*****************************************************************************/
/** fmDbgDiagCountGet
 * \ingroup diagTrackingStats 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Returns the current value of the specified diagnostic
 *                  counter.
 *
 * \param[in]       sw identifies the switch to retrieve the counter value of.
 *
 * \param[in]       counter is the index of the tracking counter to get the value
 *                  of. See the fm_trackingCounterIndex type.
 *
 * \param[out]      outValue is a pointer to the fm_uint64 that will contain the
 *                  value of the diagnostic counter if the function exits
 *                  with an FM_OK status.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if value is a NULL pointer.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDiagCountGet(fm_int                  sw,
                            fm_trackingCounterIndex counter,
                            fm_uint64 *             outValue)
{
    fm_status err = FM_OK;

    if (outValue == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (counter >= FM_SWITCH_CTR_MAX)
    {
        *outValue = (fm_uint64) -1;
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    *outValue = fmRootDebug->fmSwitchDiagnostics[sw].counters[counter];

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgDiagCountGet */




/*****************************************************************************/
/** fmDbgDiagCountSet
 * \ingroup intDiagTrackingStats
 *
 * \desc            Sets the current value of the specified diagnostic
 *                  counter.
 *
 * \param[in]       sw identifies the switch to set the counter value of.
 *
 * \param[in]       counter is the index of the counter to set the value of.
 *                  See the fm_trackingCounterIndex type.
 *
 * \param[in]       value is the value to set the diagnostic counter to.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDiagCountSet(fm_int                  sw,
                            fm_trackingCounterIndex counter,
                            fm_uint64               value)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    fmRootDebug->fmSwitchDiagnostics[sw].counters[counter] = value;

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgDiagCountSet */




/*****************************************************************************/
/** fmDbgDiagCountIncr
 * \ingroup intDiagTrackingStats
 *
 * \desc            Increments the value of the specified diagnostic
 *                  counter by a specified amount.
 *
 * \param[in]       sw identifies the switch to increment the counter of.
 *
 * \param[in]       counter is the switch diagnostic counter to increment the
 *                  value of.
 *                  See the fm_trackingCounterIndex type.
 *
 * \param[in]       amount is the amount to increment the diagnostic counter
 *                  by.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgDiagCountIncr(fm_int                  sw,
                             fm_trackingCounterIndex counter,
                             fm_uint64               amount)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    fmRootDebug->fmSwitchDiagnostics[sw].counters[counter] += amount;

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgDiagCountIncr */




/*****************************************************************************/
/** fmDbgGlobalDiagCountDump
 * \ingroup diagTrackingStats
 *
 * \desc            Displays the value of the global diagnostic counters.
 *                  These are statistics that track the flow of information
 *                  between the driver, the Adaptation Layer to Protocol Stack
 *                  and the application (protocol stack).  Information that is
 *                  tracked includes Ethernet packet flow (chunk allocations,
 *                  fp_pkt queue activity, fm_buffer allocations) and MA Table
 *                  age and learn events.
 *
 * \return          FM_OK if counters could be printed.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgGlobalDiagCountDump(void)
{
    fm_status                   err = FM_OK;
    fm_globalDiagnostics diags;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    diags = fmRootDebug->fmGlobalDiagnostics;

    DROP_DBG_LOCK();

    FM_LOG_PRINT("================== Buffer Management ==================\n");
    FM_LOG_PRINT("Total Allocations          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_BUFFER_ALLOCS]);
    FM_LOG_PRINT("Total Frees                : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_BUFFER_FREES]);
    FM_LOG_PRINT("Outstanding Allocations    : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_BUFFER_ALLOCS]
                 - diags.counters[FM_GLOBAL_CTR_BUFFER_FREES]);
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("RX allocations             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_RX_BUFFER_ALLOCS]);
    FM_LOG_PRINT("RX allocations freed       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_RX_BUFFER_FREES]);
    FM_LOG_PRINT("RX out of buffers          : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_RX_OUT_OF_BUFFERS]);
    FM_LOG_PRINT("RX fails from TX rsrv      : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_NO_BUFFERS_FOR_RX]);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("TX allocations             : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_TX_BUFFER_ALLOCS]);
    FM_LOG_PRINT("TX allocations freed       : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_TX_BUFFER_FREES]);
    FM_LOG_PRINT("Est. TX out of buffers     : %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_OUT_OF_BUFFERS] -
                 diags.counters[FM_GLOBAL_CTR_RX_OUT_OF_BUFFERS]);

    FM_LOG_PRINT("================== Event Management ===================\n");
    FM_LOG_PRINT("Out of event storage events: %15" FM_FORMAT_64 "u\n",
                 diags.counters[FM_GLOBAL_CTR_NO_EVENTS_AVAILABLE]);

    return err;

}   /* end fmDbgGlobalDiagCountDump */




/*****************************************************************************/
/** fmDbgGlobalDiagCountClear
 * \ingroup diagTrackingStats
 *
 * \desc            Reset a global diagnostic counter.
 *                  This is part of the set of statistics that track
 *                  the flow of information between the driver, the Adaptation
 *                  Layer to Protocol Stack and the application (protocol
 *                  stack).
 *
 * \param[in]       counter identifies the counter to be reset.
 *
 * \return          FM_OK if the counter is reset.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgGlobalDiagCountClear(fm_globalDiagCounter counter)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    fmRootDebug->fmGlobalDiagnostics.counters[counter] = 0;

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgGlobalDiagCountClear */




/*****************************************************************************/
/** fmDbgGlobalDiagCountClearAll
 * \ingroup diagTrackingStats
 *
 * \desc            Resets all statistics in the set of statistics that track
 *                  the flow of information between the driver, the Adaptation
 *                  Layer to Protocol Stack and the application (protocol
 *                  stack).
 *
 * \return          FM_OK if the counters are reset.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgGlobalDiagCountClearAll(void)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    memset( fmRootDebug->fmGlobalDiagnostics.counters, 0,
           sizeof(fmRootDebug->fmGlobalDiagnostics.counters) );

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgGlobalDiagCountClearAll */




/*****************************************************************************/
/** fmDbgGlobalDiagCountGet
 * \ingroup diagTrackingStats
 *
 * \desc            Returns the current value of the specified global
 *                  diagnostic counter.
 *
 * \param[in]       counter identifies the diagnostic counter to get the
 *                  value of.
 *                  See the fm_globalDiagCounter type.
 *
 * \param[out]      outValue is a pointer to the fm_uint64 that will contain the
 *                  value of the diagnostic counter if the function exits
 *                  with an FM_OK status.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if value is a NULL pointer.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgGlobalDiagCountGet(fm_globalDiagCounter counter,
                                  fm_uint64 *          outValue)
{
    fm_status err = FM_OK;

    if (outValue == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    *outValue = fmRootDebug->fmGlobalDiagnostics.counters[counter];

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgGlobalDiagCountGet */




/*****************************************************************************/
/** fmDbgGlobalDiagCountSet
 * \ingroup intDiagTrackingStats
 *
 * \desc            Sets the current value of the specified global diagnostic
 *                  counter.
 *
 * \param[in]       counter identifies the diagnostic counter to set the value
 *                  of.
 *                  See the fm_globalDiagCounter type.
 *
 * \param[in]       value is the value to set the diagnostic counter to.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgGlobalDiagCountSet(fm_globalDiagCounter counter,
                                  fm_uint64            value)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    fmRootDebug->fmGlobalDiagnostics.counters[counter] = value;

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgGlobalDiagCountSet */




/*****************************************************************************/
/** fmDbgGlobalDiagCountIncr
 * \ingroup intDiagTrackingStats
 *
 * \desc            Increments the value of the specified globaldiagnostic
 *                  counter by a specified amount.
 *
 * \param[in]       counter is the global diagnostic counter to increment the
 *                  value of.
 *                  See the fm_globalDiagCounter type.
 *
 * \param[in]       amount is the amount to increment the diagnostic counter
 *                  by.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if the diagnostic counter system has
 *                  not been initialized.
 *
 *****************************************************************************/
fm_status fmDbgGlobalDiagCountIncr(fm_globalDiagCounter counter,
                                   fm_uint64            amount)
{
    fm_status err = FM_OK;

    if (fmRootDebug == NULL)
    {
        return FM_ERR_UNSUPPORTED;
    }

    TAKE_DBG_LOCK();

    fmRootDebug->fmGlobalDiagnostics.counters[counter] += amount;

    DROP_DBG_LOCK();

    return err;

}   /* end fmDbgGlobalDiagCountIncr */




/******************************************************************************
 * End diagnostic counter API
 *****************************************************************************/

/*****************************************************************************/
/* fmDbgBfrDump
 * \ingroup diagMemory
 *
 * \desc            Display the chunk free list. (Obsolete)
 *
 * \param[in]       sw identifies the switch for which to display the chunk
 *                  list.
 *
 * \return          FM_ERR_UNSUPPORTED because this function is no longer
 *                  supported.
 *
 *****************************************************************************/
fm_status fmDbgBfrDump(fm_int sw)
{
    FM_NOT_USED(sw);
    return FM_ERR_UNSUPPORTED;

}   /* end fmDbgBfrDump */




/*****************************************************************************/
/** fmDbgDumpPortMap
 * \ingroup diagPorts 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the logical-to-physical port number mapping for
 *                  all ports on the specified switch.
 *                                                                      \lb\lb
 *                  See also ''fmDbgDumpPortMapV2''.
 *
 * \param[in]       sw is the switch whose port map is to be displayed.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpPortMap(fm_int sw)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_int      port;
    fm_int      physPort;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_PRINT("Logical    Physical\n");

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);
        FM_LOG_PRINT("%2d         %d\n", port, physPort);
    }

}   /* end fmDbgDumpPortMap */




/*****************************************************************************/
/** fmDbgDumpPortMapV2
 * \ingroup diagPorts 
 *
 * \chips           FM6000
 *
 * \desc            Display the logical-to-physical port number mapping for
 *                  one port or all ports on the specified switch. The display
 *                  includes detailed information about the port's hardware
 *                  configuration.
 *
 * \param[in]       sw is the switch whose port map is to be displayed.
 *
 * \param[in]       port identifies a particular port to be displayed and can
 *                  identify that port in several ways, as determined by the
 *                  portType argument (see). Specify as FM_ALL_PORTS to 
 *                  display all ports.
 *
 * \param[in]       portType indicates how the port argument should be
 *                  interpreted. See ''Port Map Dump Types'' for possible
 *                  values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDbgDumpPortMapV2(fm_int sw, fm_int port, fm_int portType)
{
    fm_switch * switchPtr;
    fm_status   err = FM_OK;

    VALIDATE_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->DbgDumpPortMap)
    {
        VALIDATE_AND_PROTECT_SWITCH(sw);

        FM_API_CALL_FAMILY(err,
                           switchPtr->DbgDumpPortMap,
                           sw, port, portType);

        UNPROTECT_SWITCH(sw);
    }
    else
    {
        fmDbgDumpPortMap(sw);
    }

    return err;

}   /* end fmDbgDumpPortMap */




/*****************************************************************************/
/** fmDbgDumpLinkDebounceInfo
 * \ingroup diagPorts 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Display the debounce status of each link state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpLinkDebounceInfo(fm_int sw)
{
    fm_int       cpi;
    fm_int       port;
    fm_port *    portEntry;
    fm_timestamp currTime;
    fm_switch *  switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    fmGetTime(&currTime);

    FM_LOG_PRINT("Current Timestamp = %" FM_FORMAT_64 "u.%06" FM_FORMAT_64 "u\n",
                 currTime.sec, currTime.usec);
    FM_LOG_PRINT("Port    Expiration        Value\n");

    for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);
        portEntry = switchPtr->portTable[port];

        if (portEntry->linkStateChangePending)
        {
            FM_LOG_PRINT("%4d    %9" FM_FORMAT_64 "u.%06" FM_FORMAT_64 "u %08X\n",
                         port,
                         portEntry->linkStateChangeExpiration.sec,
                         portEntry->linkStateChangeExpiration.usec,
                         (fm_uint) portEntry->pendingLinkStateValue);
        }
    }

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpLinkDebounceInfo */




/*****************************************************************************/
/** fmDbgDumpPortMasks
 * \ingroup diagPorts 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Display the allowed egress ports for each ingress port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpPortMasks(fm_int sw)
{
    fm_status err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY(err,
                       GET_SWITCH_PTR(sw)->DbgDumpPortMasks,
                       sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpPortMasks */




/*****************************************************************************/
/** fmDbgDumpLag
 * \ingroup diagPorts 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the link aggregation status of a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpLag(fm_int sw)
{
    fm_status  err = FM_OK;
    fm_switch *swstate;
    fm_int     port;
    fm_int     groupNum;
    fm_int     cpi;
    fm_uint    member;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    swstate = GET_SWITCH_PTR(sw);

    FM_LOG_PRINT("\n\n******  LagInfoTable  ******\n"
                 "lagMode = %d (%s)\n",
                 swstate->lagInfoTable.lagMode, 
                 swstate->lagInfoTable.lagMode == FM_MODE_STATIC ? 
                     "static" : "dynamic");

    FM_LOG_PRINT("Ports In a Lag:");

    for (cpi = 0 ; cpi < swstate->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);
        if ( fmPortIsInALAG(sw, port) )
        {
            FM_LOG_PRINT(" %d", port);
        }
    }

    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("Lag Index For Port:\n");

    for (cpi = 0 ; cpi < swstate->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);
        if (fmGetPortLagIndex(sw, port) >= 0)
        {
            FM_LOG_PRINT( "Port %d, LagIndex %d, Status %s\n",
                          port,
                          fmGetPortLagIndex(sw, port),
                          fmIsPortLinkUp(sw, port) ? "UP":"DOWN" );
        }
    }

    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("LagAllocated:\n");

    for (groupNum = 0 ; groupNum < FM_MAX_NUM_LAGS ; groupNum++)
    {
        if (swstate->lagInfoTable.lag[groupNum])
        {
            FM_LOG_PRINT("  %d\n", groupNum);
            FM_LOG_PRINT("    Index %d\n", swstate->lagInfoTable.lag[groupNum]->index);
            FM_LOG_PRINT("    Logical Port %d\n", swstate->lagInfoTable.lag[groupNum]->logicalPort);
            FM_LOG_PRINT("    Nb Members %d\n", swstate->lagInfoTable.lag[groupNum]->nbMembers);
            FM_LOG_PRINT("    Members ");
            for (member = 0 ; member < swstate->lagInfoTable.lag[groupNum]->nbMembers ; member++)
            {
                FM_LOG_PRINT("%d ", swstate->lagInfoTable.lag[groupNum]->member[member].port);
            }
            FM_LOG_PRINT("\n");
            FM_LOG_PRINT("    Internal? %d\n", swstate->lagInfoTable.lag[groupNum]->isInternalPort);
            FM_LOG_PRINT("    Filtering? %d\n", swstate->lagInfoTable.lag[groupNum]->filteringEnabled);
            FM_LOG_PRINT("    Hash Rotation %d\n", swstate->lagInfoTable.lag[groupNum]->hashRotation);
        }
    }

    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("LagResources:\n");

    for (groupNum = 0 ; groupNum < FM_ALLOC_LAGS_MAX ; groupNum++)
    {
        FM_LOG_PRINT("  %d\n", groupNum);
        FM_LOG_PRINT("    BaseLagIndex %d\n", swstate->lagInfoTable.allocLags[groupNum].baseLagIndex);
        FM_LOG_PRINT("    numLags %d\n", swstate->lagInfoTable.allocLags[groupNum].numLags);
        FM_LOG_PRINT("    baseHandle %d\n", swstate->lagInfoTable.allocLags[groupNum].baseHandle);
        FM_LOG_PRINT("    step %d\n", swstate->lagInfoTable.allocLags[groupNum].step);
    }

    FM_LOG_PRINT("\n");

    if (swstate->DbgDumpLag)
    {
        FM_API_CALL_FAMILY(err, swstate->DbgDumpLag, sw);
    }

    FM_LOG_PRINT("\n");

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpLag */




/*****************************************************************************/
/** fmDbgDumpLinkUpMask
 * \ingroup diagPorts 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the link up/down status for all ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
fm_status fmDbgDumpLinkUpMask(fm_int sw)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_int      port;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        if (switchPtr->portTable[port]->linkUp)
        {
            FM_LOG_PRINT("Port %02d: link UP\n", port);
        }
        else
        {
            FM_LOG_PRINT("Port %02d: link DOWN\n", port);
        }
    }

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpLinkUpMask */




/*****************************************************************************/
/** fmDbgDumpBstTable
 * \ingroup diagMisc 
 *
 * \chips           FM6000
 *
 * \desc            Display the contents of the BST software table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpBstTable(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpBstTable, sw);
    
    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgDumpBstTable */




/*****************************************************************************
 * fmDbgDumpThreads
 *
 * Description: Dumps thread information to the console
 *
 * Arguments:   none
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgDumpThreads(void)
{
    extern void i(int taskNameOrId);


#if 0
    i(0);
#endif

}   /* end fmDbgDumpThreads */




/*****************************************************************************
 * fmDbgDumpString
 *
 * Description: Dumps a string to the console
 *
 * Arguments:   stringAddr is the address of the string in memory.
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgDumpString(fm_char *stringAddr)
{
    FM_NOT_USED(stringAddr);

    FM_LOG_PRINT("string at address %p is: %s\n", stringAddr, stringAddr);

}   /* end fmDbgDumpString */




/*****************************************************************************/
/** fmDbgDumpTriggers
 * \ingroup diagMisc 
 *
 * \chips           FM2000, FM3000, FM4000, FM10000
 *
 * \desc            Dumps switch trigger information.
 *
 * \note            Switch triggers are an intrinsic feature of the switch
 *                  device and should not be confused with triggers in the
 *                  ''Event Trace Buffer''.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK unconditionally.
 *
 *****************************************************************************/
fm_status fmDbgDumpTriggers(fm_int sw)
{
    fm_status err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY(err,
                       GET_SWITCH_PTR(sw)->DbgDumpTriggers,
                       sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpTriggers */




/*****************************************************************************/
/** fmDbgDumpTriggerUsage
 * \ingroup diagMisc 
 *
 * \chips           FM2000, FM3000, FM4000
 *
 * \desc            Dumps switch trigger usage.
 *
 * \note            Switch triggers are an intrinsic feature of the switch
 *                  device and should not be confused with triggers in the
 *                  ''Event Trace Buffer''.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK unconditionally.
 *
 *****************************************************************************/
fm_status fmDbgDumpTriggerUsage(fm_int sw)
{
    fm_status err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY(err,
                       GET_SWITCH_PTR(sw)->DbgDumpTriggerUsage,
                       sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpTriggerUsage */




#ifdef FM_DBG_NEED_TRACK_FUNC
/*****************************************************************************/
/** fmDbgCompareFuncExecTraceEntries
 * \ingroup intDiag
 *
 * \desc            Compares function execution trace entries for searching
 *                  and sorting purposes.
 *
 * \param[in]       obj1 points to the first function exec trace entry
 *
 * \param[in]       obj2 points to the second function exec trace entry
 *
 * \return          -1 if the first entry comes before the 2nd entry.
 * \return          0 if the entries are identical.
 * \return          1 if the first entry comes after the second entry.
 *
 *****************************************************************************/
int fmDbgCompareFuncExecTraceEntries(const void *obj1, const void *obj2)
{
    fmDbgExecTraceEntry *entry1;
    fmDbgExecTraceEntry *entry2;

    entry1 = (fmDbgExecTraceEntry *) obj1;
    entry2 = (fmDbgExecTraceEntry *) obj2;

    if (entry1->address < entry2->address)
    {
        return -1;
    }
    else if (entry1->address > entry2->address)
    {
        return 1;
    }

    if (entry1->lineNum < entry2->lineNum)
    {
        return -1;
    }
    else if (entry1->lineNum > entry2->lineNum)
    {
        return 1;
    }

    return 0;

}   /* end fmDbgCompareFuncExecTraceEntries */




/*****************************************************************************/
/** fmDbgFuncExec
 * \ingroup intDiag
 *
 * \desc            tracks function execution.
 *
 * \param[in]       funcName contains the name of the function.
 *
 * \param[in]       lineNum is the line number within the file containing
 *                  the function.
 *
 * \return          none
 *
 *****************************************************************************/
void fmDbgFuncExec(const char *funcName, int lineNum)
{
    /* prevent this from running before the memory is allocated */
    if (trackFuncInit == 1)
    {    
        fmDbgExecTraceEntry  key;
        fmDbgExecTraceEntry *curEntry = NULL;
    
        /* Try to find the entry in the table */
        if (fmRootDebug->trace->fmDbgExecTraceEntryCount > 0)
        {
            key.address = funcName;
            key.lineNum = lineNum;
            curEntry    = bsearch(&key,
                                  fmRootDebug->trace->fmDbgExecTraceTable,
                                  fmRootDebug->trace->fmDbgExecTraceEntryCount,
                                  sizeof(fmDbgExecTraceEntry),
                                  fmDbgCompareFuncExecTraceEntries);
    
            if (curEntry != NULL)
            {
                curEntry->execCount++;
                return;
            }
        }
    
        /* Create a new entry */
        if (fmRootDebug->trace->fmDbgExecTraceEntryCount <
            FM_DBG_MAX_EXEC_TRACE_ENTRIES)
        {
            curEntry            = &fmRootDebug->trace->fmDbgExecTraceTable[fmRootDebug->trace->fmDbgExecTraceEntryCount++];
            curEntry->address   = funcName;
            curEntry->lineNum   = lineNum;
            curEntry->execCount = FM_LITERAL_64(1);
            qsort(fmRootDebug->trace->fmDbgExecTraceTable,
                  fmRootDebug->trace->fmDbgExecTraceEntryCount,
                  sizeof(fmDbgExecTraceEntry),
                  fmDbgCompareFuncExecTraceEntries);
        }
        else
        {
            fmRootDebug->trace->fmDbgExecTraceOverflows++;
        }
    }
    else
    {
        trackPreInitCall ++;
    }
}   /* end fmDbgFuncExec */

#endif




/*****************************************************************************/
/** fmDbgDumpFuncExecTable
 * \ingroup intDiag
 *
 * \desc            dumps function execution tracking table
 *
 * \return          none
 *
 *****************************************************************************/
void fmDbgDumpFuncExecTable()
{
#ifdef FM_API_FUNCTION_TRACKING
    fmDbgExecTraceEntry *curEntry;
    fm_int               cur;
    char                 addr[100];

    FM_PRINTF_S("fmDbgDumpFuncExecTable : Entry Count = %d, "
                "Overflow counter = %" FM_FORMAT_64 "u PreInit calls %d \n",
                fmRootDebug->trace->fmDbgExecTraceEntryCount,
                fmRootDebug->trace->fmDbgExecTraceOverflows,
                trackPreInitCall);

    for (cur = 0, curEntry = fmRootDebug->trace->fmDbgExecTraceTable ;
         cur < fmRootDebug->trace->fmDbgExecTraceEntryCount ;
         cur++, curEntry++)
    {
        FM_SNPRINTF_S(addr,
                      sizeof(addr),
                      "%s",
                      curEntry->address);
        FM_PRINTF_S("%-40.40s    %" FM_FORMAT_64 "u\n",
                    addr,
                    curEntry->execCount);
    }

    FM_PRINTF_S("fmDbgDumpFuncExecTable : End of Table \n");

#endif
}   /* end fmDbgDumpFuncExecTable */




/*****************************************************************************/
/** fmDbgDumpVersion
 * \ingroup diagMisc
 *
 * \desc            Displays the build ID of the software.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpVersion(void)
{
    FM_LOG_PRINT("Build ID: %s\n", FM_BUILD_IDENTIFIER);

}   /* end fmDbgDumpVersion */




char *fmDbgGetVersion()
{
    return FM_BUILD_IDENTIFIER;

}   /* end fmDbgGetVersion */




void fmDbgPacketSizeDistAdd(int ps)
{
    if (ps < FM_DBG_MAX_PACKET_SIZE)
    {
        fmRootDebug->dbgPacketSizeDist[ps]++;
    }

}   /* end fmDbgPacketSizeDistAdd */




void fmDbgDumpPacketSizeDist(void)
{
    int i, total;

    for (total = 0, i = 0 ; i < FM_DBG_MAX_PACKET_SIZE ; i++)
    {
        total += fmRootDebug->dbgPacketSizeDist[i];
    }

    FM_LOG_PRINT("CPU Received Packet Size Distribution:\n");
    FM_LOG_PRINT("-----------------------------------------\n");

    for (i = 0 ; i < FM_DBG_MAX_PACKET_SIZE ; i++)
    {
        if (fmRootDebug->dbgPacketSizeDist[i] > 0)
        {
            FM_LOG_PRINT( "%8d bytes : %d (%3.3f%%)\n",
                         i,
                         fmRootDebug->dbgPacketSizeDist[i],
                         (fmRootDebug->dbgPacketSizeDist[i] * 100.0 / total) );
        }
    }

}   /* end fmDbgDumpPacketSizeDist */




/*****************************************************************************/
/** fmDbgDumpDeviceMemoryStats
 * \ingroup diagMisc 
 *
 * \chips           FM2000
 *
 * \desc            Display device memory usage.
 *
 * \param[in]       sw is the switch whose memory is to be displayed.
 *
 * \return          FM_OK returned unconditionally.
 *
 *****************************************************************************/
fm_status fmDbgDumpDeviceMemoryStats(int sw)
{
    fm_status err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY(err,
                       GET_SWITCH_PTR(sw)->DbgDumpDeviceMemoryStats,
                       sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpDeviceMemoryStats */




/*****************************************************************************/
/** fmPrettyPrintPacket
 * \ingroup diagMisc
 *
 * \desc            Display the contents of a message packet stored in a
 *                  buffer chain.
 *
 * \param[in]       pkt points to the packet buffer's first fm_buffer
 *                  structure in a chain of one or more buffers.
 *
 * \return          None
 *
 *****************************************************************************/
void fmPrettyPrintPacket(fm_buffer *pkt)
{
    int i;
    int c;
    int offset = 0;
    fm_uint32 hData;

    while (pkt)
    {
        for (i = 0, c = 0 ; c < pkt->len ; i++, c += 4, offset++)
        {
            /* fm_buffer.data is stored in network byte order,
             * convert it to host order */
            hData = ntohl(pkt->data[i]);
            FM_LOG_PRINT("%02x %02x %02x %02x ",
                         (hData >> 24) & 0xff,
                         (hData >> 16) & 0xff,
                         (hData >> 8 ) & 0xff,
                         (hData >> 0 ) & 0xff);

            if ( (offset > 0) && ( (offset % 5) == 4 ) )
            {
                FM_LOG_PRINT("\n");
            }
        }

        pkt = pkt->next;
    }

    FM_LOG_PRINT("\n\n");

}   /* end fmPrettyPrintPacket */




#if 0
#define MAX_ADDR_FLUSH_STAT_COUNT  50
fm_lock         fmDbgMacFlushLock;
static fm_float lastMacFlushTimes[MAX_ADDR_FLUSH_STAT_COUNT];
static int      lastMacFlushAddrCount[MAX_ADDR_FLUSH_STAT_COUNT];
static int      numMacFlushCount = 0;
static fm_float avgMacFlushTime  = 0.0;
static fm_float minMacFlushTime  = 1.0e10;
static fm_float maxMacFlushTime  = 0.0;
static fm_float flushTimeStart   = 0.0;
static fm_float flushTimeEnd     = 0.0;

void fmDbgResetMacFlushStat()
{
    int i;

    fmCaptureLock(&fmDbgMacFlushLock, FM_WAIT_FOREVER);

    for (i = 0 ; i < MAX_ADDR_FLUSH_STAT_COUNT ; i++)
    {
        lastMacFlushTimes[i] = 0.0;
    }

    numMacFlushCount = 0;
    avgMacFlushTime  = 0.0;
    minMacFlushTime  = 1.0e10;
    maxMacFlushTime  = 0.0;
    flushTimeStart   = 0.0;
    flushTimeEnd     = 0.0;
    fmReleaseLock(&fmDbgMacFlushLock);

}   /* end fmDbgResetMacFlushStat */




void fmDbgInitMacFlushStat()
{
    fmCreateLock("port flush lock", &fmDbgMacFlushLock);
    fmDbgResetMacFlushStat();

}   /* end fmDbgInitMacFlushStat */




void fmDbgMarkMacFlushBegin()
{
    struct timespec ts;

    fmCaptureLock(&fmDbgMacFlushLock, FM_WAIT_FOREVER);
    clock_gettime(CLOCK_REALTIME, &ts);
    flushTimeStart = (ts.tv_sec) + (1.0e-9 * ts.tv_nsec);

}   /* end fmDbgMarkMacFlushBegin */




void fmDbgMarkMacFlushEnd(int addrCount)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    flushTimeEnd = (ts.tv_sec) + (1.0e-9 * ts.tv_nsec);

    fmDbgUpdateMacFlushTime(addrCount);

}   /* end fmDbgMarkMacFlushEnd */




void fmDbgUpdateMacFlushTime(int addrCount)
{
    fm_float newFlushTime = flushTimeEnd - flushTimeStart;
    fm_float total        = 0.0;
    int      i, n;

    if ( (addrCount < 1024) || (newFlushTime < MIN_MAC_FLUSH_THRESH) )
    {
        goto ABORT;
    }

    if (numMacFlushCount < MAX_ADDR_FLUSH_STAT_COUNT)
    {
        n = numMacFlushCount;
    }
    else
    {
        n = MAX_ADDR_FLUSH_STAT_COUNT;
    }

    /* shift flush times over, new slot opened in 0 */
    for (i = MAX_ADDR_FLUSH_STAT_COUNT - 1 ; i > 0 ; i--)
    {
        lastMacFlushTimes[i]     = lastMacFlushTimes[i - 1];
        lastMacFlushAddrCount[i] = lastMacFlushAddrCount[i - 1];
    }

    lastMacFlushTimes[0]     = newFlushTime;
    lastMacFlushAddrCount[0] = addrCount;

    for (i = 0 ; i < MAX_ADDR_FLUSH_STAT_COUNT ; i++)
    {
        total += lastMacFlushTimes[i];
    }

    avgMacFlushTime = total / (n + 1);

    if (newFlushTime > maxMacFlushTime)
    {
        maxMacFlushTime = newFlushTime;
    }

    if (newFlushTime < minMacFlushTime)
    {
        minMacFlushTime = newFlushTime;
    }

    numMacFlushCount++;
ABORT:
    flushTimeStart = 0.0;
    flushTimeEnd   = 0.0;
    fmReleaseLock(&fmDbgMacFlushLock);

}   /* end fmDbgUpdateMacFlushTime */




void fmDbgDumpMacFlushTime()
{
    int i, n;

    if (numMacFlushCount < MAX_ADDR_FLUSH_STAT_COUNT)
    {
        n = numMacFlushCount;
    }
    else
    {
        n = MAX_ADDR_FLUSH_STAT_COUNT;
    }

    FM_LOG_PRINT("MAC Table Flush Time Statistics\n");
    FM_LOG_PRINT("------------------------------------------------\n");
    FM_LOG_PRINT("NUM     : %d\n", numMacFlushCount);
    FM_LOG_PRINT("MIN     : %3.6f s\n", minMacFlushTime);
    FM_LOG_PRINT("MAX     : %3.6f s\n", maxMacFlushTime);
    FM_LOG_PRINT("AVG     : %3.6f s\n", avgMacFlushTime);
    FM_LOG_PRINT("\nLast 10 Flush Times:\n");

    for (i = 0 ; i < n ; i++)
    {
        FM_LOG_PRINT("    #%d: %3.6f s (%d)\n", i, lastMacFlushTimes[i],
                     lastMacFlushAddrCount[i]);
    }

}   /* end fmDbgDumpMacFlushTime */

#endif




/*****************************************************************************/
/** fmDbgDumpPort
 * \ingroup diagPorts 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the API's data on a specified port. Also
 *                  dumps the port's configuration registers on the switch
 *                  device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpPort(fm_int sw, fm_int port)
{
#define TF(x)  x ? "TRUE" : "FALSE"

    fm_portAttr *portAttr;
    fm_switch*   switchPtr;
    fm_port *    portState;
    fm_status    err;
    int          i;

    switchPtr = GET_SWITCH_PTR(sw);
    portState = GET_PORT_PTR(sw, port);
    portAttr  = GET_PORT_ATTR(sw, port);

    FM_LOG_PRINT("State for port %d:\n", port);
    FM_LOG_PRINT("mode                   : %d\n", portState->mode);
    FM_LOG_PRINT("submode                : %d\n", portState->submode);
    FM_LOG_PRINT("speed                  : %d\n", portAttr->speed);
    FM_LOG_PRINT("link                   : %d\n", portState->linkUp);

    /* Compute the index of the highest-numbered word in the port mask
     * required to represent all the physical ports in the switch; then
     * print the required number of words in reverse order (N..0). */
    FM_LOG_PRINT("lastSourceMask         :");
    for (i = (switchPtr->maxPhysicalPort - 1) / 32 ; i >= 0; --i)
    {
        FM_LOG_PRINT(" %08x", portAttr->portMask.maskWord[i]);
    }
    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("dropTagged             : %s\n", TF(portAttr->dropTagged));
    FM_LOG_PRINT("defaultVID             : %d\n", portAttr->defVlan);
    FM_LOG_PRINT("linkStateChangePending : %s\n",
                 TF(portState->linkStateChangePending) );
    FM_LOG_PRINT("pendingLinkStateValue  : %08x\n",
                 portState->pendingLinkStateValue);
    FM_LOG_PRINT("security               : %d\n", portAttr->security);
    FM_LOG_PRINT("learning               : %s\n", TF(portAttr->learning));
    FM_LOG_PRINT("securityTrap           : %d\n", portAttr->securityTrap);
    FM_LOG_PRINT("portSecurityEnabled    : %d\n", 
                 portState->portSecurityEnabled);

    FM_API_CALL_FAMILY(err, portState->DbgDumpPort, sw, port);

    if (err != FM_OK && err != FM_ERR_UNSUPPORTED)
    {
        FM_LOG_ERROR(FM_LOG_CAT_DEBUG, 
                     "DbgDumpPort failed with '%s'\n",
                     fmErrorMsg(err) );
    }

    return;

}   /* end fmDbgDumpPort */




/*****************************************************************************/
/** fmDbgConvertMacAddressToString
 * \ingroup diagMisc
 *
 * \desc            Convert a mac address into a string.
 *
 * \param[in]       macAddr is the mac address.
 *
 * \param[out]      textOut points to caller-allocated storage, at least 18
 *                  characters in length, where the string will be stored. 
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgConvertMacAddressToString(fm_macaddr macAddr, fm_char *textOut)
{
    fm_uint32     word0;
    fm_uint32     word1;
    const fm_uint textOutLength = 18; /* 12 digits + 5 colons + 1 NUL = 18 */

    word0 = (fm_uint32) macAddr;
    word1 = (fm_uint32) (macAddr >> 32);

    FM_SNPRINTF_S( (char *) textOut,
                   textOutLength,
                   "%02X:%02X:%02X:%02X:%02X:%02X",
                   (word1 >> 8) & 0xFF,
                   word1 & 0xFF,
                   (word0 >> 24) & 0xFF,
                   (word0 >> 16) & 0xFF,
                   (word0 >> 8) & 0xFF,
                   word0 & 0xFF );

}   /* end fmDbgConvertMacAddressToString */



/*****************************************************************************/
/** fmDbgConvertStringToMacAddress
 * \ingroup intDiag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Function to convert a string to a MAC address
 *
 * \note            The input string is parsed assuming a colon as separator or
 *                  no separators at all
 *
 * \param[in]       addrStr the input string
 * 
 * \param[out]      addrValue user-allocated storage where the converted MAC
 *                  MAC address will be returned
 * 
 * \return          TRUE if the address was successfully converted.
 * \return          FALSE if the address string could not be parsed.
 * 
 *****************************************************************************/
fm_bool fmDbgConvertStringToMacAddress(const char *addrStr, fm_macaddr *addrValue) 
{
    int         i;
    fm_uint32   words[6];
    int         scanCount;
    fm_macaddr  address;

    scanCount = FM_SSCANF_S(addrStr,
                            "%2x:%2x:%2x:%2x:%2x:%2x",
                            &words[0],
                            &words[1],
                            &words[2],
                            &words[3],
                            &words[4],
                            &words[5]);

    if (scanCount != 6)
    {
        /**************************************************
        * Try again without colons.
        **************************************************/
        scanCount = FM_SSCANF_S(addrStr,
                                "%2x%2x%2x%2x%2x%2x",
                                &words[0],
                                &words[1],
                                &words[2],
                                &words[3],
                                &words[4],
                                &words[5]);

        if (scanCount != 6)
        {
            return FALSE;
        }
    }

    address = FM_LITERAL_64(0);

    for (i = 0 ; i < 6 ; i++)
    {
        address <<= 8;
        address  |= (fm_macaddr)words[i];
    }

    *addrValue = address;

    return TRUE;

}   /* end fmDbgConvertStringToMacAddress */




/*****************************************************************************/
/** fmDbgDumpMulticastTables
 * \ingroup diagMisc 
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Display multicast tables.
 *
 * \param[in]       sw contains the switch number.
 *
 * \return          FM_OK unconditionally.
 *
 *****************************************************************************/
fm_status fmDbgDumpMulticastTables(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMulticastTables, sw);
    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpGlortTable, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpMulticastTables */




/*****************************************************************************/
/** fmDbgDumpGlortTable
 * \ingroup diagMisc 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Displays the glort table of a switch.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpGlortTable(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpGlortTable, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpGlortTable */




/*****************************************************************************/
/** fmDbgDumpGlortDestTable
 * \ingroup diagMisc 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Displays the glort destination table of a switch.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       raw - set to TRUE (1) to display the raw value of the
 *                  register and its cache.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpGlortDestTable(fm_int sw, fm_bool raw)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpGlortDestTable, sw, raw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpGlortDestTable */




/*****************************************************************************/
/** fmDbgGenerateSwitchInsertionEvent
 * \ingroup diagMisc
 *
 * \desc            Generate a switch inserted event.
 *
 * \param[in]       model contains the switch model number (see
 *                  ''fm_switchModel'').
 *
 * \param[in]       slot contains the slot number.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmDbgGenerateSwitchInsertionEvent(fm_int model, fm_int slot)
{
    fm_event *              insertEvent;
    fm_eventSwitchInserted *insert;

    /* This is priority high because we don't want to be throttled */
    insertEvent = fmAllocateEvent(slot,
                                  FM_EVID_SYSTEM,
                                  FM_EVENT_SWITCH_INSERTED,
                                  FM_EVENT_PRIORITY_HIGH);

    if (!insertEvent)
    {
        FM_LOG_PRINT("Unable to allocate event for switch insertion\n");
    }
    else
    {
        insert = &insertEvent->info.fpSwitchInsertedEvent;

        insert->model = model;
        insert->slot  = slot;

        fmSendThreadEvent(&fmRootApi->eventThread, insertEvent);
    }

}   /* end fmDbgGenerateSwitchInsertionEvent */




/*****************************************************************************/
/** fmDbgGenerateSwitchRemovalEvent
 * \ingroup diagMisc
 *
 * \desc            Generate a switch removal event.
 *
 * \param[in]       slot contains the slot number.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmDbgGenerateSwitchRemovalEvent(fm_int slot)
{
    fm_event *             removeEvent;
    fm_eventSwitchRemoved *remove;

    /* This is priority high because we don't want to be throttled */
    removeEvent = fmAllocateEvent(slot,
                                  FM_EVID_SYSTEM,
                                  FM_EVENT_SWITCH_REMOVED,
                                  FM_EVENT_PRIORITY_HIGH);

    if (!removeEvent)
    {
        FM_LOG_PRINT("Unable to allocate event for switch removal\n");
    }
    else
    {
        remove = &removeEvent->info.fpSwitchRemovedEvent;

        remove->slot = slot;

        fmSendThreadEvent(&fmRootApi->eventThread, removeEvent);
    }

}   /* end fmDbgGenerateSwitchRemovalEvent */




/*****************************************************************************/
/** fmDbgDumpMemoryUsage
 * \ingroup diagMisc 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dumps the memory usage counters for all ports.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpMemoryUsage(fm_int sw)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMemoryUsage, sw);

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpMemoryUsage */




/*****************************************************************************/
/** fmDbgDumpMemoryUsageV2
 * \ingroup diagMisc 
 *
 * \chips           FM6000
 *
 * \desc            Dumps the memory usage counters for the specified port and
 *                  memory partition.  If the port or memory partition is
 *                  out of range, filtering will not be performed for
 *                  the respective argument.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rxPort is the receive port to filter on. If rxPort is 
 *                  outside of the valid port range, all rxPorts will be used.
 *
 * \param[in]       txPort is the transmit port to filter on. If txPort is 
 *                  outside of the valid port range, all txPorts will be used.
 *
 * \param[in]       rxmp is the receive memory partition to filter on.
 *                  If rxmp is outside of the valid range, all receive memory
 *                  partitions will be used.
 *
 * \param[in]       txmp is the transmit memory partition to filter on.
 *                  If txmp is outside of the valid range, all transmit memory
 *                  partitions will be used.
 *
 * \param[in]       bsg is the bandwidth sharing group to filter on.  
 *                  If bsg is outside of the valid range, all bandwidth 
 *                  sharing groups will be used.
 *
 * \param[in]       useSegments indicates whether to use segments (TRUE) or 
 *                  bytes (FALSE) in display.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDbgDumpMemoryUsageV2(fm_int  sw, 
                                 fm_int  rxPort, 
                                 fm_int  txPort,
                                 fm_int  rxmp, 
                                 fm_int  txmp, 
                                 fm_int  bsg,
                                 fm_bool useSegments)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMemoryUsageV2, sw, rxPort, 
                            txPort, rxmp, txmp, bsg, useSegments);
    
    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpMemoryUsageV2 */




/*****************************************************************************/
/** fmDbgDumpMemoryUsageV3
 * \ingroup diagMisc 
 *
 * \chips           FM10000
 *
 * \desc            Dumps the memory usage counters for the specified port and
 *                  memory partition and traffic class.  If the port, memory 
 *                  partition or traffic class is out of range, filtering will 
 *                  not be performed for the respective argument.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rxPort is the receive port to filter on. If rxPort is 
 *                  outside of the valid port range, all rxPorts will be used.
 *
 * \param[in]       txPort is the transmit port to filter on. If txPort is 
 *                  outside of the valid port range, all txPorts will be used.
 *
 * \param[in]       smp is the shared memory partition to filter on.
 *                  If smp is outside of the valid range, all memory partitions 
 *                  will be used.
 *
 * \param[in]       txTc is the traffic class to filter by. If out of 
 *                  range, all TCs will be selected.
 *
 * \param[in]       bsg is the bandwidth sharing group to filter on.  
 *                  If bsg is outside of the valid range, all bandwidth 
 *                  sharing groups will be used.
 *
 * \param[in]       useSegments indicates whether to use segments (TRUE) or 
 *                  bytes (FALSE) in display.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDbgDumpMemoryUsageV3(fm_int  sw, 
                                 fm_int  rxPort, 
                                 fm_int  txPort,
                                 fm_int  smp, 
                                 fm_int  txTc, 
                                 fm_int  bsg,
                                 fm_bool useSegments)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMemoryUsageV3, sw, rxPort, 
                            txPort, smp, txTc, bsg, useSegments);
    
    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpMemoryUsageV3 */




/*****************************************************************************/
/** fmDbgDumpPolicers
 * \ingroup diagMisc 
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Display the contents of the Policer banks.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpPolicers(fm_int sw)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpPolicers, sw);

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpPolicers */




/*****************************************************************************/
/** fmDbgDumpWatermarks
 * \ingroup diagMisc 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dumps the watermark values for all ports.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpWatermarks(fm_int sw)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpWatermarks, sw);

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpWatermarks */




/*****************************************************************************/
/** fmDbgDumpWatermarksV2
 * \ingroup diagMisc 
 *
 * \chips           FM6000
 *
 * \desc            Dumps the watermark values for the specified port, ISL
 *                  priority, TXMP and RXMP.  If the port, ISL priority, TXMP 
 *                  or RXMP is out of range, all possible values for that
 *                  argument will be used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rxPort is the RX port to filter on. If out of range, 
 *                  all RX ports will be selected.
 *
 * \param[in]       txPort is the TX port to filter on. If out of range, 
 *                  all TX ports will be selected.
 *
 * \param[in]       rxmp is the RXMP to filter on. If out of range, all RXMPs
 *                  will be selected.
 *
 * \param[in]       txmp is the TXMP to filter on. If out of range, all TXMPs
 *                  will be selected.
 *
 * \param[in]       islPri is the ISL priority to filter on. If out of range,
 *                  all ISL Priorities will be selected.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDbgDumpWatermarksV2(fm_int sw, fm_int rxPort, fm_int txPort, 
                                fm_int rxmp, fm_int txmp, fm_int islPri)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpWatermarksV2, 
                            sw, 
                            rxPort, 
                            txPort,
                            rxmp, 
                            txmp, 
                            islPri);

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpWatermarksV2 */




/*****************************************************************************/
/** fmDbgDumpWatermarksV3
 * \ingroup diagMisc 
 *
 * \chips           FM10000
 *
 * \desc            Dumps the watermark values for the specified port, ISL
 *                  priority, SMP, TC. If the port, ISL priority, SMP or TC
 *                  is out of range, all possible values for that argument
 *                  will be used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rxPort is the RX port to filter on. If out of range, 
 *                  all RX ports will be selected.
 *
 * \param[in]       txPort is the TX port to filter on. If out of range, 
 *                  all TX ports will be selected.
 *
 * \param[in]       smp is the shared memory partition to filter on. If out of
 *                  range, all SMPs will be selected.
 *
 * \param[in]       txTc is the traffic class to filter on. 
 *                  If out of range, all TCs will be selected.
 *
 * \param[in]       islPri is the ISL priority to filter on. If out of range,
 *                  all ISL Priorities will be selected.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDbgDumpWatermarksV3(fm_int sw, fm_int rxPort, fm_int txPort, 
                                fm_int smp, fm_int txTc, fm_int islPri)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpWatermarksV3, 
                            sw, 
                            rxPort, 
                            txPort,
                            smp, 
                            txTc, 
                            islPri);

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpWatermarksV3 */




/*****************************************************************************/
/** fmDbgDumpQOS
 * \ingroup diagPorts 
 *
 * \chips           FM6000
 *
 * \desc            Dumps the QoS hardware debug states.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number on which to report
 *                  QoS status.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpQOS(fm_int sw, fm_int port)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpQOS, sw, port);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpQOS */




/*****************************************************************************/
/** fmDbgDumpPortMax
 * \ingroup diagPorts 
 *
 * \chips           FM6000
 *
 * \desc            Dumps the port maximums based on current configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number on which to report
 *                  maximums.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDbgDumpPortMax(fm_int sw, fm_int port)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpPortMax, sw, port);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpPortMax */




/*****************************************************************************/
/** fmDbgDumpSwpriMap
 * \ingroup diagMisc
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Dumps a Switch Priority (ISLPRI) based mapping table
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch attribute to display:                \lb
 *                  ''FM_QOS_SWPRI_RXMP_MAP''                               \lb
                      (does not apply to FM10000 devices)
 *                  ''FM_QOS_TC_SMP_MAP''
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if attr is not supported by this
 *                  function.
 * \return          FM_ERR_UNSUPPORTED if feature not supported on switch.
 *
 *****************************************************************************/
fm_status fmDbgDumpSwpriMap(fm_int sw, fm_int attr)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpSwpriMap, sw, attr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpSwpriMap */




/*****************************************************************************/
/** fmDbgDumpPortIdxMap
 * \ingroup diagMisc
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Dumps a port indexed mapping table
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number to dump the map for.
 *
 * \param[in]       attr is the switch attribute to dump:                   \lb
 *                  ''FM_QOS_TC_PC_MAP''                                    \lb
 *                  ''FM_QOS_PC_RXMP_MAP''
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if attr is not supported by this
 *                  function.
 * \return          FM_ERR_INVALID_PORT if log is not a valid port number.
 * \return          FM_ERR_NO_PORTS_IN_LAG if log refers to an empty LAG.
 * \return          FM_ERR_UNSUPPORTED if feature not supported on switch.
 *
 *****************************************************************************/
fm_status fmDbgDumpPortIdxMap(fm_int sw, fm_int port, fm_int attr)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpPortIdxMap, sw, port, attr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpPortIdxMap */




/*****************************************************************************/
/** fmDbgDumpVid
 * \ingroup diagMisc 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dumps the hardware VLAN tables.
 *
 * \param[in]       sw is the switch number to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpVid(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpVid, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpVid */




/*****************************************************************************/
/** fmDbgDumpStatChanges
 * \ingroup diagMisc 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dumps the statistics that have changed since the last
 *                  call to this function.  The first time it is called,
 *                  it will dump all stats that are non-zero.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       resetCopy is TRUE if the stored counter copies should be
 *                  set back to zero after the comparisons have been made.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
void fmDbgDumpStatChanges(fm_int sw, fm_bool resetCopy)
{
    static fm_bool            firstRun = TRUE;
    static fm_portCounters *  savedPortStats[FM_MAX_NUM_SWITCHES];
    static fm_switchCounters *savedSwitchStats[FM_MAX_NUM_SWITCHES];
    static fm_vlanCounters *  savedVlanStats[FM_MAX_NUM_SWITCHES];
    static fm_int             numPorts[FM_MAX_NUM_SWITCHES];
    static fm_int             numVlans[FM_MAX_NUM_SWITCHES];
    fm_portCounters *         statsPtr;
    fm_portCounters           portStats;
    fm_status                 err;
    static fm_switchInfo      info;
    fm_int                    nbytes;
    fm_int                    cpi;
    fm_int                    port;
    fm_int                    vlan;
    fm_switchCounters *       switchStatsPtr;
    fm_switchCounters         swStats;
    fm_vlanCounters *         vlanStatsPtr;
    fm_vlanCounters           vlanStats;

#define TRACK_SWITCH_STAT(statVar)                                             \
    if (switchStatsPtr->statVar != swStats.statVar)                            \
    {                                                                          \
        FM_LOG_PRINT( "switch %d stats change: "                               \
                     # statVar " delta = %lld\n",                              \
                     sw,                                                       \
                     (fm_int64) (swStats.statVar - switchStatsPtr->statVar) ); \
                                                                               \
        switchStatsPtr->statVar = swStats.statVar;                             \
    }

#define TRACK_PORT_STAT(statVar)                                           \
    if (statsPtr->statVar != portStats.statVar)                            \
    {                                                                      \
        FM_LOG_PRINT( "switch %d, port %d stats change: "                  \
                     # statVar " delta = %lld\n",                          \
                     sw,                                                   \
                     port,                                                 \
                     (fm_int64) (portStats.statVar - statsPtr->statVar) ); \
                                                                           \
        statsPtr->statVar = portStats.statVar;                             \
    }

#define TRACK_VLAN_STAT(statVar)                                               \
    if (vlanStatsPtr->statVar != vlanStats.statVar)                            \
    {                                                                          \
        FM_LOG_PRINT( "switch %d, vlan %d stats change: "                      \
                     # statVar " delta = %lld\n",                              \
                     sw,                                                       \
                     vlan,                                                     \
                     (fm_int64) (vlanStats.statVar - vlanStatsPtr->statVar) ); \
                                                                               \
        vlanStatsPtr->statVar = vlanStats.statVar;                             \
    }

    if (firstRun)
    {
        memset( savedPortStats, 0, sizeof(savedPortStats) );
        memset( savedSwitchStats, 0, sizeof(savedSwitchStats) );
        memset( savedVlanStats, 0, sizeof(savedVlanStats) );
        memset( numPorts, 0, sizeof(numPorts) );
        memset( numVlans, 0, sizeof(numVlans) );
        firstRun = FALSE;
    }

    if (savedPortStats[sw] == NULL)
    {
        err = fmGetSwitchInfo(sw, &info);

        if (err != FM_OK)
        {
            return;
        }

        numPorts[sw] = info.numCardPorts;

        nbytes = sizeof(fm_portCounters) * (numPorts[sw] + 1);

        savedPortStats[sw] = fmAlloc(nbytes);

        if (savedPortStats[sw] == NULL)
        {
            return;
        }

        memset(savedPortStats[sw], 0, nbytes);

        savedSwitchStats[sw] = fmAlloc( sizeof(fm_switchCounters) );

        if (savedSwitchStats[sw] == NULL)
        {
            return;
        }

        memset( savedSwitchStats[sw], 0, sizeof(fm_switchCounters) );

        numVlans[sw] = info.maxVLAN;

        nbytes = sizeof(fm_vlanCounters) * (numVlans[sw] + 1);

        savedVlanStats[sw] = fmAlloc(nbytes);

        if (savedVlanStats[sw] == NULL)
        {
            return;
        }

        memset(savedVlanStats[sw], 0, nbytes);
    }

    switchStatsPtr = savedSwitchStats[sw];

    err = fmGetSwitchCounters(sw, &swStats);

    if (err == FM_OK)
    {
        TRACK_SWITCH_STAT(cntGlobalLowDropPkts);
        TRACK_SWITCH_STAT(cntGlobalHighDropPkts);
        TRACK_SWITCH_STAT(cntGlobalPrivilegeDropPkts);
        TRACK_SWITCH_STAT(cntStatsDropCountTx);
        TRACK_SWITCH_STAT(cntStatsDropCountRx);
    }
    else if (err != FM_ERR_UNSUPPORTED)
    {
        FM_LOG_PRINT("Unexpected error from fmGetSwitchCounters "
                     "(sw=%d): %s\n",
                     sw,
                     fmErrorMsg(err));
        return;
    }

    for (cpi = 0 ; cpi < numPorts[sw] ; cpi++)
    {
        err = fmMapCardinalPort(sw, cpi, &port, NULL);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_DEBUG,
                         "Unexpected error from fmMapCardinalPort "
                         "(sw=%d cpi=%d): %s\n",
                         sw,
                         cpi,
                         fmErrorMsg(err));
            continue;
        }

        err = fmGetPortCounters(sw, port, &portStats);
        if (err != FM_OK)
        {
            FM_LOG_PRINT("Unexpected error from fmGetPortCounters "
                         "(sw=%d port=%d): %s\n",
                         sw,
                         port,
                         fmErrorMsg(err));
            continue;
        }

        statsPtr = savedPortStats[sw] + cpi;

        /*********************************************/
        /* Group 1 Counters - Rx Type (frames)       */
        /*********************************************/

        TRACK_PORT_STAT(cntRxUcstPkts);
        TRACK_PORT_STAT(cntRxUcstPktsNonIP);
        TRACK_PORT_STAT(cntRxUcstPktsIPv4);
        TRACK_PORT_STAT(cntRxUcstPktsIPv6);
        TRACK_PORT_STAT(cntRxBcstPkts);
        TRACK_PORT_STAT(cntRxBcstPktsNonIP);
        TRACK_PORT_STAT(cntRxBcstPktsIPv4);
        TRACK_PORT_STAT(cntRxBcstPktsIPv6);
        TRACK_PORT_STAT(cntRxMcstPkts);
        TRACK_PORT_STAT(cntRxMcstPktsNonIP);
        TRACK_PORT_STAT(cntRxMcstPktsIPv4);
        TRACK_PORT_STAT(cntRxMcstPktsIPv6);
        TRACK_PORT_STAT(cntRxPausePkts);
        TRACK_PORT_STAT(cntRxCBPausePkts);
        TRACK_PORT_STAT(cntRxFCSErrors);
        TRACK_PORT_STAT(cntRxSymbolErrors);
        TRACK_PORT_STAT(cntRxFrameSizeErrors);
        TRACK_PORT_STAT(cntRxFramingErrorPkts);

        /*********************************************/
        /* Group 2 Counters - Rx Size (frames)       */
        /*********************************************/

        TRACK_PORT_STAT(cntRxMinTo63Pkts);
        TRACK_PORT_STAT(cntRx64Pkts);
        TRACK_PORT_STAT(cntRx65to127Pkts);
        TRACK_PORT_STAT(cntRx128to255Pkts);
        TRACK_PORT_STAT(cntRx256to511Pkts);
        TRACK_PORT_STAT(cntRx512to1023Pkts);
        TRACK_PORT_STAT(cntRx1024to1522Pkts);
        TRACK_PORT_STAT(cntRx1523to2047Pkts);
        TRACK_PORT_STAT(cntRx2048to4095Pkts);
        TRACK_PORT_STAT(cntRx4096to8191Pkts);
        TRACK_PORT_STAT(cntRx8192to10239Pkts);
        TRACK_PORT_STAT(cntRx10240toMaxPkts);

        /*********************************************/
        /* Group 2 Counters - Rx Size (octets)       */
        /*********************************************/

        TRACK_PORT_STAT(cntRxMinTo63octets);
        TRACK_PORT_STAT(cntRx64octets);
        TRACK_PORT_STAT(cntRx65to127octets);
        TRACK_PORT_STAT(cntRx128to255octets);
        TRACK_PORT_STAT(cntRx256to511octets);
        TRACK_PORT_STAT(cntRx512to1023octets);
        TRACK_PORT_STAT(cntRx1024to1522octets);
        TRACK_PORT_STAT(cntRx1523to2047octets);
        TRACK_PORT_STAT(cntRx2048to4095octets);
        TRACK_PORT_STAT(cntRx4096to8191octets);
        TRACK_PORT_STAT(cntRx8192to10239octets);
        TRACK_PORT_STAT(cntRx10240toMaxOctets);

        /*********************************************/
        /* Group 3 Counters - Rx Type (octets)       */
        /*********************************************/

        TRACK_PORT_STAT(cntRxOctetsNonIp);
        TRACK_PORT_STAT(cntRxOctetsIPv4);
        TRACK_PORT_STAT(cntRxOctetsIPv6);
        TRACK_PORT_STAT(cntRxUcstOctetsNonIP);
        TRACK_PORT_STAT(cntRxUcstOctetsIPv4);
        TRACK_PORT_STAT(cntRxUcstOctetsIPv6);
        TRACK_PORT_STAT(cntRxBcstOctetsNonIP);
        TRACK_PORT_STAT(cntRxBcstOctetsIPv4);
        TRACK_PORT_STAT(cntRxBcstOctetsIPv6);
        TRACK_PORT_STAT(cntRxMcstOctetsNonIP);
        TRACK_PORT_STAT(cntRxMcstOctetsIPv4);
        TRACK_PORT_STAT(cntRxMcstOctetsIPv6);
        TRACK_PORT_STAT(cntRxPauseOctets);
        TRACK_PORT_STAT(cntRxCBPauseOctets);
        TRACK_PORT_STAT(cntRxFCSErrorsOctets);
        TRACK_PORT_STAT(cntRxFramingErrorOctets);
        TRACK_PORT_STAT(cntRxGoodOctets);
        TRACK_PORT_STAT(cntRxBadOctets);

        /*********************************************/
        /* Group 4 Counters - Rx Priority (Frames)   */
        /*********************************************/

        TRACK_PORT_STAT(cntRxPriorityPkts[0]);
        TRACK_PORT_STAT(cntRxPriorityPkts[1]);
        TRACK_PORT_STAT(cntRxPriorityPkts[2]);
        TRACK_PORT_STAT(cntRxPriorityPkts[3]);
        TRACK_PORT_STAT(cntRxPriorityPkts[4]);
        TRACK_PORT_STAT(cntRxPriorityPkts[5]);
        TRACK_PORT_STAT(cntRxPriorityPkts[6]);
        TRACK_PORT_STAT(cntRxPriorityPkts[7]);
        TRACK_PORT_STAT(cntRxPriorityPkts[8]);
        TRACK_PORT_STAT(cntRxPriorityPkts[9]);
        TRACK_PORT_STAT(cntRxPriorityPkts[10]);
        TRACK_PORT_STAT(cntRxPriorityPkts[11]);
        TRACK_PORT_STAT(cntRxPriorityPkts[12]);
        TRACK_PORT_STAT(cntRxPriorityPkts[13]);
        TRACK_PORT_STAT(cntRxPriorityPkts[14]);
        TRACK_PORT_STAT(cntRxPriorityPkts[15]);
        TRACK_PORT_STAT(cntRxInvalidPriorityPkts);

        /*********************************************/
        /* Group 5 Counters - Rx Priority (Octets)   */
        /*********************************************/

        TRACK_PORT_STAT(cntRxPriorityOctets[0]);
        TRACK_PORT_STAT(cntRxPriorityOctets[1]);
        TRACK_PORT_STAT(cntRxPriorityOctets[2]);
        TRACK_PORT_STAT(cntRxPriorityOctets[3]);
        TRACK_PORT_STAT(cntRxPriorityOctets[4]);
        TRACK_PORT_STAT(cntRxPriorityOctets[5]);
        TRACK_PORT_STAT(cntRxPriorityOctets[6]);
        TRACK_PORT_STAT(cntRxPriorityOctets[7]);
        TRACK_PORT_STAT(cntRxPriorityOctets[8]);
        TRACK_PORT_STAT(cntRxPriorityOctets[9]);
        TRACK_PORT_STAT(cntRxPriorityOctets[10]);
        TRACK_PORT_STAT(cntRxPriorityOctets[11]);
        TRACK_PORT_STAT(cntRxPriorityOctets[12]);
        TRACK_PORT_STAT(cntRxPriorityOctets[13]);
        TRACK_PORT_STAT(cntRxPriorityOctets[14]);
        TRACK_PORT_STAT(cntRxPriorityOctets[15]);
        TRACK_PORT_STAT(cntRxInvalidPriorityOctets);

        /*********************************************/
        /* Group 6 Counters - Rx Forwarding (frames) */
        /*********************************************/

        TRACK_PORT_STAT(cntFIDForwardedPkts);
        TRACK_PORT_STAT(cntFloodForwardedPkts);
        TRACK_PORT_STAT(cntGlortSwitchedPkts);
        TRACK_PORT_STAT(cntGlortRoutedPkts);
        TRACK_PORT_STAT(cntSpeciallyHandledPkts);
        TRACK_PORT_STAT(cntParseErrDropPkts);
        TRACK_PORT_STAT(cntParityErrorPkts);
        TRACK_PORT_STAT(cntTrappedPkts);
        TRACK_PORT_STAT(cntPauseDropPkts);
        TRACK_PORT_STAT(cntSTPDropPkts);
        TRACK_PORT_STAT(cntSTPIngressDropsPkts);
        TRACK_PORT_STAT(cntSTPEgressDropsPkts);
        TRACK_PORT_STAT(cntReservedTrapPkts);
        TRACK_PORT_STAT(cntSecurityViolationPkts);
        TRACK_PORT_STAT(cntVLANTagDropPkts);
        TRACK_PORT_STAT(cntVLANIngressBVPkts);
        TRACK_PORT_STAT(cntVLANEgressBVPkts);
        TRACK_PORT_STAT(cntLoopbackDropsPkts);
        TRACK_PORT_STAT(cntGlortMissDropPkts);
        TRACK_PORT_STAT(cntFFUDropPkts);
        TRACK_PORT_STAT(cntInvalidDropPkts);
        TRACK_PORT_STAT(cntPolicerDropPkts);
        TRACK_PORT_STAT(cntTTLDropPkts);
        TRACK_PORT_STAT(cntGlobalWMDropPkts);
        TRACK_PORT_STAT(cntRXMPDropPkts);
        TRACK_PORT_STAT(cntRxHogDropPkts);
        TRACK_PORT_STAT(cntTxHogDropPkts);
        TRACK_PORT_STAT(cntOtherPkts);
        TRACK_PORT_STAT(cntFloodControlDropPkts);
        TRACK_PORT_STAT(cntCmPrivDropPkts);
        TRACK_PORT_STAT(cntSmp0DropPkts);
        TRACK_PORT_STAT(cntSmp1DropPkts);
        TRACK_PORT_STAT(cntRxHog0DropPkts);
        TRACK_PORT_STAT(cntRxHog1DropPkts);
        TRACK_PORT_STAT(cntTxHog0DropPkts);
        TRACK_PORT_STAT(cntTxHog1DropPkts);
        TRACK_PORT_STAT(cntRateLimit0DropPkts);
        TRACK_PORT_STAT(cntRateLimit1DropPkts);
        TRACK_PORT_STAT(cntBadSmpDropPkts);
        TRACK_PORT_STAT(cntTriggerDropRedirPkts);
        TRACK_PORT_STAT(cntTriggerDropPkts);
        TRACK_PORT_STAT(cntTriggerRedirPkts);
        TRACK_PORT_STAT(cntGlortForwardedPkts);
        TRACK_PORT_STAT(cntTriggerMirroredPkts);
        TRACK_PORT_STAT(cntBroadcastDropPkts);
        TRACK_PORT_STAT(cntDLFDropPkts);
        TRACK_PORT_STAT(cntRxCMDropPkts);

        /*********************************************/
        /* Group 6 Counters - Rx Forwarding (octets) */
        /*********************************************/

        TRACK_PORT_STAT(cntFIDForwardedOctets);
        TRACK_PORT_STAT(cntFloodForwardedOctets);
        TRACK_PORT_STAT(cntSpeciallyHandledOctets);
        TRACK_PORT_STAT(cntParseErrDropOctets);
        TRACK_PORT_STAT(cntParityErrorOctets);
        TRACK_PORT_STAT(cntTrappedOctets);
        TRACK_PORT_STAT(cntPauseDropOctets);
        TRACK_PORT_STAT(cntSTPDropOctets);
        TRACK_PORT_STAT(cntSecurityViolationOctets);
        TRACK_PORT_STAT(cntVLANTagDropOctets);
        TRACK_PORT_STAT(cntVLANIngressBVOctets);
        TRACK_PORT_STAT(cntVLANEgressBVOctets);
        TRACK_PORT_STAT(cntLoopbackDropOctets);
        TRACK_PORT_STAT(cntGlortMissDropOctets);
        TRACK_PORT_STAT(cntFFUDropOctets);
        TRACK_PORT_STAT(cntPolicerDropOctets);
        TRACK_PORT_STAT(cntTTLDropOctets);
        TRACK_PORT_STAT(cntOtherOctets);
        TRACK_PORT_STAT(cntFloodControlDropOctets);
        TRACK_PORT_STAT(cntCmPrivDropOctets);
        TRACK_PORT_STAT(cntSmp0DropOctets);
        TRACK_PORT_STAT(cntSmp1DropOctets);
        TRACK_PORT_STAT(cntRxHog0DropOctets);
        TRACK_PORT_STAT(cntRxHog1DropOctets);
        TRACK_PORT_STAT(cntTxHog0DropOctets);
        TRACK_PORT_STAT(cntTxHog1DropOctets);
        TRACK_PORT_STAT(cntTriggerDropOctets);
        TRACK_PORT_STAT(cntTriggerRedirOctets);
        TRACK_PORT_STAT(cntGlortForwardedOctets);

        /*********************************************/
        /* Group 7 Counters - Tx Types (Frames)      */
        /*********************************************/
        
        TRACK_PORT_STAT(cntTxUcstPkts);
        TRACK_PORT_STAT(cntTxBcstPkts);
        TRACK_PORT_STAT(cntTxMcstPkts);
        TRACK_PORT_STAT(cntTxUcstPktsNonIP);
        TRACK_PORT_STAT(cntTxBcstPktsNonIP);
        TRACK_PORT_STAT(cntTxMcstPktsNonIP);
        TRACK_PORT_STAT(cntTxUcstPktsIP);
        TRACK_PORT_STAT(cntTxBcstPktsIP);
        TRACK_PORT_STAT(cntTxMcstPktsIP);
        TRACK_PORT_STAT(cntTxPausePkts);
        TRACK_PORT_STAT(cntTxCBPausePkts);
        TRACK_PORT_STAT(cntTxFCSErrDropPkts);
        TRACK_PORT_STAT(cntTxFramingErrorPkts);
        TRACK_PORT_STAT(cntTxErrorSentPkts);
        TRACK_PORT_STAT(cntTxErrorDropPkts);
        TRACK_PORT_STAT(cntTxTimeOutPkts);
        TRACK_PORT_STAT(cntTxOutOfMemErrPkts);
        TRACK_PORT_STAT(cntTxUnrepairEccPkts);
        TRACK_PORT_STAT(cntTxLoopbackPkts);
        TRACK_PORT_STAT(cntTxTTLDropPkts);

        /*********************************************/
        /* Group 8 Counters - Tx Size (frames)       */
        /*********************************************/

        TRACK_PORT_STAT(cntTxMinTo63Pkts);
        TRACK_PORT_STAT(cntTx64Pkts);
        TRACK_PORT_STAT(cntTx65to127Pkts);
        TRACK_PORT_STAT(cntTx128to255Pkts);
        TRACK_PORT_STAT(cntTx256to511Pkts);
        TRACK_PORT_STAT(cntTx512to1023Pkts);
        TRACK_PORT_STAT(cntTx1024to1522Pkts);
        TRACK_PORT_STAT(cntTx1523to2047Pkts);
        TRACK_PORT_STAT(cntTx2048to4095Pkts);
        TRACK_PORT_STAT(cntTx4096to8191Pkts);
        TRACK_PORT_STAT(cntTx8192to10239Pkts);
        TRACK_PORT_STAT(cntTx10240toMaxPkts);

        /*********************************************/
        /* Group 8 Counters - Tx Size (octets)       */
        /*********************************************/

        TRACK_PORT_STAT(cntTxMinTo63octets);
        TRACK_PORT_STAT(cntTx64octets);
        TRACK_PORT_STAT(cntTx65to127octets);
        TRACK_PORT_STAT(cntTx128to255octets);
        TRACK_PORT_STAT(cntTx256to511octets);
        TRACK_PORT_STAT(cntTx512to1023octets);
        TRACK_PORT_STAT(cntTx1024to1522octets);
        TRACK_PORT_STAT(cntTx1523to2047octets);
        TRACK_PORT_STAT(cntTx2048to4095octets);
        TRACK_PORT_STAT(cntTx4096to8191octets);
        TRACK_PORT_STAT(cntTx8192to10239octets);
        TRACK_PORT_STAT(cntTx10240toMaxOctets);

        /*********************************************/
        /* Group 9 Counters - Tx Types (Octets)      */
        /*********************************************/

        TRACK_PORT_STAT(cntTxUcstOctetsNonIP);
        TRACK_PORT_STAT(cntTxBcstOctetsNonIP);
        TRACK_PORT_STAT(cntTxMcstOctetsNonIP);
        TRACK_PORT_STAT(cntTxUcstOctetsIP);
        TRACK_PORT_STAT(cntTxBcstOctetsIP);
        TRACK_PORT_STAT(cntTxMcstOctetsIP);
        TRACK_PORT_STAT(cntTxUcstOctets);
        TRACK_PORT_STAT(cntTxMcstOctets);
        TRACK_PORT_STAT(cntTxBcstOctets);
        TRACK_PORT_STAT(cntTxFCSErrDropOctets);
        TRACK_PORT_STAT(cntTxOctets);
        TRACK_PORT_STAT(cntTxErrorOctets);
        TRACK_PORT_STAT(cntTxFramingErrorOctets);
        TRACK_PORT_STAT(cntTxPauseOctets);
        TRACK_PORT_STAT(cntTxCBPauseOctets);
        TRACK_PORT_STAT(cntTxFCSErroredOctets);
        TRACK_PORT_STAT(cntTxErrorSentOctets);
        TRACK_PORT_STAT(cntTxTimeOutOctets);
        TRACK_PORT_STAT(cntTxOutOfMemErrOctets);
        TRACK_PORT_STAT(cntTxUnrepairEccOctets);
        TRACK_PORT_STAT(cntTxLoopbackOctets);
        TRACK_PORT_STAT(cntTxTTLDropOctets);

        /*********************************************/
        /* Group 16 Counters - Tx Priority (Octets)  */
        /*********************************************/

        TRACK_PORT_STAT(cntTxPriorityOctets[0]);
        TRACK_PORT_STAT(cntTxPriorityOctets[1]);
        TRACK_PORT_STAT(cntTxPriorityOctets[2]);
        TRACK_PORT_STAT(cntTxPriorityOctets[3]);
        TRACK_PORT_STAT(cntTxPriorityOctets[4]);
        TRACK_PORT_STAT(cntTxPriorityOctets[5]);
        TRACK_PORT_STAT(cntTxPriorityOctets[6]);
        TRACK_PORT_STAT(cntTxPriorityOctets[7]);
        TRACK_PORT_STAT(cntTxPriorityOctets[8]);
        TRACK_PORT_STAT(cntTxPriorityOctets[9]);
        TRACK_PORT_STAT(cntTxPriorityOctets[10]);
        TRACK_PORT_STAT(cntTxPriorityOctets[11]);
        TRACK_PORT_STAT(cntTxPriorityOctets[12]);
        TRACK_PORT_STAT(cntTxPriorityOctets[13]);
        TRACK_PORT_STAT(cntTxPriorityOctets[14]);
        TRACK_PORT_STAT(cntTxPriorityOctets[15]);

        /*********************************************/
        /* EPL Counters                              */
        /*********************************************/

        TRACK_PORT_STAT(cntUnderrunPkts);
        TRACK_PORT_STAT(cntOverrunPkts);
        TRACK_PORT_STAT(cntRxFragmentPkts);
        TRACK_PORT_STAT(cntRxUndersizedPkts);
        TRACK_PORT_STAT(cntRxJabberPkts);
        TRACK_PORT_STAT(cntCorruptedPkts);
        TRACK_PORT_STAT(cntCodeErrors);
        TRACK_PORT_STAT(cntRxOversizedPkts);
        TRACK_PORT_STAT(cntTxFCSErroredPkts);

        /*********************************************/
        /* Other Counters                            */
        /*********************************************/

        TRACK_PORT_STAT(cntStatsDropCountTx);
        TRACK_PORT_STAT(cntStatsDropCountRx);
        TRACK_PORT_STAT(cntTxMirrorPkts);
        TRACK_PORT_STAT(cntTxMirrorOctets);
        TRACK_PORT_STAT(cntTxCMDropPkts);
    }

    for (vlan = 0 ; vlan < numVlans[sw] ; vlan++)
    {
        err = fmGetVLANCounters(sw, vlan, &vlanStats);

        switch (err)
        {
            case FM_ERR_NO_VLANCOUNTER:
            case FM_ERR_INVALID_VLAN:
                continue;
            case FM_OK:
                break;
            default:
                FM_LOG_PRINT("Unexpected error %d (%s) from fmGetVLANCounters"
                             "(sw=%d, vlan=%d)\n",
                             err,
                             fmErrorMsg(err),
                             sw,
                             vlan);
                return;
        }

        if (err != FM_OK)
        {
            return;
        }

        vlanStatsPtr = savedVlanStats[sw] + vlan;

        TRACK_VLAN_STAT(cntUcstOctets);
        TRACK_VLAN_STAT(cntXcstOctets);
        TRACK_VLAN_STAT(cntUcstPkts);
        TRACK_VLAN_STAT(cntXcstPkts);
        TRACK_VLAN_STAT(cntRxUcstPkts);
        TRACK_VLAN_STAT(cntRxMcstPkts);
        TRACK_VLAN_STAT(cntRxBcstPkts);
        TRACK_VLAN_STAT(cntRxDropPkts);
        TRACK_VLAN_STAT(cntRxUcstOctets);
        TRACK_VLAN_STAT(cntRxMcstOctets);
        TRACK_VLAN_STAT(cntRxBcstOctets);
        TRACK_VLAN_STAT(cntRxDropOctets);
    }

    if (resetCopy)
    {
        memset( savedSwitchStats[sw], 0, sizeof(fm_switchCounters) );
        nbytes = sizeof(fm_portCounters) * (numPorts[sw] + 1);
        memset(savedPortStats[sw], 0, nbytes);
        nbytes = sizeof(fm_vlanCounters) * (numVlans[sw] + 1);
        memset(savedVlanStats[sw], 0, nbytes);
    }

}   /* end fmDbgDumpStatChanges */




/*****************************************************************************/
/** fmDbgDumpMirror
 * \ingroup diagMirror 
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Dumps the Mirror configuration.
 *
 * \param[in]       sw is the switch number to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpMirror(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpMirror, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpMirror */




/*****************************************************************************/
/** fmDbgDumpSAFTable
 * \ingroup intDiag
 *
 * \chips           FM6000
 *
 * \desc            This function prints out the current store-and-forward
 *                  table by logical port or physical port.
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpSAFTable(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;

    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);

    if (err == FM_OK)
    {
        switchPtr = GET_SWITCH_PTR(sw);

        FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpSAFTable, sw);

        UNPROTECT_SWITCH(sw);
    }

}   /* end fmDbgDumpSAFTable */




/*****************************************************************************/
/** fmDbgSetPacketDefaultSourcePort
 * \ingroup intDiag
 *
 * \chips           FM6000
 *
 * \desc            Sets the default source port used when sending a frame
 *                  to the CPU port for transmission on the switch. If the
 *                  incoming fm_packetInfo structure specifies a zero value
 *                  for the sourcePort field, this default value will be
 *                  used instead. Intended for debugging use when using
 *                  TestPoint without needing to plumb the source port through
 *                  the TP packet send commands.
 *
 * \param[in]       sw is the switch to be updated.
 *
 * \param[in]       sourcePort identifies the source logical port. This
 *                  port's glort will be determined and put into the ISL
 *                  tag. If this value is less than zero, the packet will
 *                  be processed as if the zeroSourceGlort field in the
 *                  fm_packetInfo structure were TRUE.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDbgSetPacketDefaultSourcePort(fm_int sw, fm_int sourcePort)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchPtr->defaultSourcePort = sourcePort;

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgSetPacketDefaultSourcePort */




/*****************************************************************************/
/** fmDbgDumpMirrorProfile
 * \ingroup diagMirror 
 *
 * \chips           FM10000
 *
 * \desc            Dumps the Mirror PROFILE configuration.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpMirrorProfile(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpMirrorProfile, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpMirrorProfile */




/*****************************************************************************/
/** fmDbgDumpScheduler
 * \ingroup intDiag
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Dumps the frame scheduler configuration
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpScheduler(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpScheduler, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpScheduler */

