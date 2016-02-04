/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_nat.c
 * Creation Date:   March 13, 2014
 * Description:     NAT API.
 *
 * Copyright (c) 2014 - 2016, Intel Corporation
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

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmFreeTable
 * \ingroup intNat
 *
 * \desc            Free a fm_natTable structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeTable(void *value)
{
    fm_natTable *natTable = (fm_natTable*)value;
    if (natTable != NULL)
    {
        fmTreeDestroy(&natTable->tunnels, fmFree);
        fmTreeDestroy(&natTable->rules, fmFree);
        fmTreeDestroy(&natTable->prefilters, fmFree);
        fmFree(value);
    }

}   /* end fmFreeTable */


/*****************************************************************************/
/** FreeNatInfoStruct
 * \ingroup intNat
 *
 * \desc            Free a fm_natInfo structure.
 *
 * \param[in,out]   natInfo points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void FreeNatInfoStruct(fm_natInfo *natInfo)
{
    if (natInfo)
    {
        fmTreeDestroy(&natInfo->tables, fmFreeTable);

        fmFree(natInfo);
    }

}   /* end FreeNatInfoStruct */




/*****************************************************************************/
/** InitializeNatInfoStruct
 * \ingroup intNat
 *
 * \desc            Initialize a fm_natInfo structure.
 * 
 * \param[in,out]   natInfo points to the structure to initialize.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void InitializeNatInfoStruct(fm_natInfo *natInfo)
{
    fmTreeInit(&natInfo->tables);


}   /* end InitializeNatInfoStruct */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmNatInit
 * \ingroup intNat
 *
 * \desc            Initialize the NAT members of the switch structure.
 *
 * \note            This function is called from fmxxxxNatInit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmNatInit(fm_int sw)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm_status        err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    /* NAT API only supported on FM10000 or aggregate of such switch for now */
    if ( (switchPtr->switchFamily != FM_SWITCH_FAMILY_SWAG) &&
         (switchPtr->switchFamily != FM_SWITCH_FAMILY_FM10000) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_NAT, FM_OK);
    }

    /* Top level NAT structure initialization */
    if (switchPtr->natInfo != NULL)
    {
        FreeNatInfoStruct(switchPtr->natInfo);
        switchPtr->natInfo = NULL;
    }

    switchPtr->natInfo = fmAlloc( sizeof(fm_natInfo) );

    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    FM_CLEAR(*switchPtr->natInfo);

    InitializeNatInfoStruct(switchPtr->natInfo);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fmNatInit */




/*****************************************************************************/
/** fmNatFree
 * \ingroup intNat
 *
 * \desc            Free the memory used by the NAT members of the
 *                  switch structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmNatFree(fm_int sw)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    if (switchPtr->natInfo != NULL)
    {
        FreeNatInfoStruct(switchPtr->natInfo);
        switchPtr->natInfo = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_NAT, FM_OK);

}   /* end fmNatFree */




/*****************************************************************************/
/** fmCreateNatTable
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Create a NAT Table. The NAT Table will be created based on
 *                  configuration as specified with natParam.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       natParam points to the table parameters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ALREADY_EXISTS if the table already exists.
 * \return          FM_ERR_UNSUPPORTED if matching condition is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_NO_FREE_GROUP if no free slot available.
 * \return          FM_ERR_TUNNEL_LOOKUP_FULL if lookup table is full.
 * \return          FM_ERR_TUNNEL_GLORT_FULL if te glort table is full.
 * \return          FM_ERR_ACLS_TOO_BIG if the compiled NAT table will not fit
 *                  in the portion of the FFU allocated to ACLs.
 *
 *****************************************************************************/
fm_status fmCreateNatTable(fm_int sw, fm_int table, fm_natParam *natParam)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (natParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    if (err == FM_OK)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else if (err != FM_ERR_NOT_FOUND)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateNatTable,
                       sw,
                       table,
                       natParam);

    if (err == FM_OK)
    {
        natTable = fmAlloc(sizeof(fm_natTable));

        if (natTable == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        FM_CLEAR(*natTable);

        fmTreeInit(&natTable->tunnels);
        fmTreeInit(&natTable->rules);
        fmTreeInit(&natTable->prefilters);

        natTable->natParam = *natParam;

        err = fmTreeInsert(&switchPtr->natInfo->tables, table, natTable);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmCreateNatTable */




/*****************************************************************************/
/** fmDeleteNatTable
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Remove a NAT Table
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 *
 *****************************************************************************/
fm_status fmDeleteNatTable(fm_int sw, fm_int table)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteNatTable,
                       sw,
                       table);
    if (err == FM_OK)
    {
        err = fmTreeRemoveCertain(&switchPtr->natInfo->tables, table, fmFreeTable);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmDeleteNatTable */




/*****************************************************************************/
/** fmGetNatTable
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the NAT Table parameter as specified on creation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[out]      natParam points to caller-supplied storage where this
 *                  function should place the table parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatTable(fm_int sw, fm_int table, fm_natParam *natParam)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (natParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *natParam = natTable->natParam;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTable */




/*****************************************************************************/
/** fmGetNatTableFirst
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the first NAT Table handler.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstTable points to caller-supplied storage where this
 *                  function should place the first table handler.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if no NAT tables exist.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatTableFirst(fm_int sw, fm_int *firstTable)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_treeIterator it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (firstTable == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    fmTreeIterInit(&it, &switchPtr->natInfo->tables);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *firstTable = nextKey;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTableFirst */




/*****************************************************************************/
/** fmGetNatTableNext
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Find the next NAT Table handler.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentTable is the last Table found by a previous
 *                  call to this function or to ''fmGetNatTableFirst''.
 *
 * \param[out]      nextTable points to caller-supplied storage where this
 *                  function should place the next table handler.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if no more NAT tables are available.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatTableNext(fm_int sw, fm_int currentTable, fm_int *nextTable)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_treeIterator it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, currentTable = %d\n",
                     sw, currentTable);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (nextTable == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeIterInitFromSuccessor(&it,
                                      &switchPtr->natInfo->tables,
                                      currentTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *nextTable = nextKey;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTableNext */




/*****************************************************************************/
/** fmSetNatTunnelDefault
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Configure default tunnel mode and behaviour. This may
 *                  only be configured if NAT rules specify action 
 *                  FM_NAT_ACTION_TUNNEL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelDefault points to the tunnel configuration to set.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 *
 *****************************************************************************/
fm_status fmSetNatTunnelDefault(fm_int sw, fm_natTunnelDefault *tunnelDefault)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    err          = FM_OK;
    natLockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (tunnelDefault == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetNatTunnelDefault,
                       sw,
                       tunnelDefault);

    if (err == FM_OK)
    {
        switchPtr->natInfo->tunnelDefault = *tunnelDefault;
    }

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmSetNatTunnelDefault */




/*****************************************************************************/
/** fmGetNatTunnelDefault
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Retrieve default tunnel configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      tunnelDefault points to caller-supplied storage where this
 *                  function should place the default tunnel configuration.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 *
 *****************************************************************************/
fm_status fmGetNatTunnelDefault(fm_int sw, fm_natTunnelDefault *tunnelDefault)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    err          = FM_OK;
    natLockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (tunnelDefault == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    *tunnelDefault = switchPtr->natInfo->tunnelDefault;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTunnelDefault */




/*****************************************************************************/
/** fmCreateNatTunnel
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Create a NAT Tunnel entry. This created entry can be
 *                  referenced using NAT rule action FM_NAT_ACTION_TUNNEL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       tunnel is the tunnel ID.
 * 
 * \param[in]       param points to the tunnel parameters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_ALREADY_EXISTS if the tunnel already exists.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 * \return          FM_ERR_TUNNEL_TYPE if configured tunnel type is invalid.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 *
 *****************************************************************************/
fm_status fmCreateNatTunnel(fm_int        sw,
                            fm_int        table,
                            fm_int        tunnel,
                            fm_natTunnel *param)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken;
    fm_natTable *   natTable;
    fm_natTunnel *  natTunnel;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d, tunnel = %d\n",
                     sw, table, tunnel);

    err          = FM_OK;
    natLockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (param == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->tunnels, tunnel, (void**) &natTunnel);
    if (err == FM_OK)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else if (err != FM_ERR_NOT_FOUND)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateNatTunnel,
                       sw,
                       table,
                       tunnel,
                       param);
    if (err == FM_OK)
    {
        natTunnel = fmAlloc(sizeof(fm_natTunnel));

        if (natTunnel == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        *natTunnel = *param;

        err = fmTreeInsert(&natTable->tunnels, tunnel, natTunnel);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmCreateNatTunnel */




/*****************************************************************************/
/** fmDeleteNatTunnel
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Delete a NAT Tunnel entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       tunnel is the tunnel ID.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or the tunnel could not be
 *                  found.
 *
 *****************************************************************************/
fm_status fmDeleteNatTunnel(fm_int sw, fm_int table, fm_int tunnel)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;
    fm_natTunnel *  natTunnel;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d, tunnel = %d\n",
                     sw, table, tunnel);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->tunnels, tunnel, (void**) &natTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteNatTunnel,
                       sw,
                       table,
                       tunnel);
    if (err == FM_OK)
    {
        err = fmTreeRemoveCertain(&natTable->tunnels, tunnel, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmDeleteNatTunnel */




/*****************************************************************************/
/** fmGetNatTunnel
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get NAT Tunnel entry configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       tunnel is the tunnel ID.
 * 
 * \param[out]      param points to caller-supplied storage where this
 *                  function should place the tunnel parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or the tunnel could not be
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatTunnel(fm_int        sw,
                         fm_int        table,
                         fm_int        tunnel,
                         fm_natTunnel *param)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;
    fm_natTunnel *  natTunnel;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d, tunnel = %d\n",
                     sw, table, tunnel);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (param == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->tunnels, tunnel, (void**) &natTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *param = *natTunnel;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTunnel */




/*****************************************************************************/
/** fmGetNatTunnelFirst
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the First NAT Tunnel entry and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[out]      firstNatTunnel points to caller-supplied storage where this
 *                  function should place the first tunnel id retrieved.
 * 
 * \param[out]      param points to caller-supplied storage where this
 *                  function should place the tunnel parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_NO_MORE if no NAT tunnels exist.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatTunnelFirst(fm_int        sw,
                              fm_int        table,
                              fm_int *      firstNatTunnel,
                              fm_natTunnel *param)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_uint64       nextKey;
    fm_natTable *   natTable;
    fm_natTunnel *  natTunnel;
    fm_treeIterator it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (firstNatTunnel == NULL) || (param == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    fmTreeIterInit(&it, &natTable->tunnels);

    err = fmTreeIterNext(&it, &nextKey, (void**) &natTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *firstNatTunnel = nextKey;
    *param = *natTunnel;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTunnelFirst */




/*****************************************************************************/
/** fmGetNatTunnelNext
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Find the Next NAT Tunnel entry and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       currentNatTunnel is the last Tunnel found by a previous
 *                  call to this function or to ''fmGetNatTunnelFirst''.
 * 
 * \param[out]      nextNatTunnel points to caller-supplied storage where this
 *                  function should place the next tunnel id.
 * 
 * \param[out]      param points to caller-supplied storage where this
 *                  function should place the tunnel parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_NO_MORE if no more NAT tunnels are available.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatTunnelNext(fm_int        sw,
                             fm_int        table,
                             fm_int        currentNatTunnel,
                             fm_int *      nextNatTunnel,
                             fm_natTunnel *param)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_uint64       nextKey;
    fm_natTable *   natTable;
    fm_natTunnel *  natTunnel;
    fm_treeIterator it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d, currentNatTunnel = %d\n",
                     sw, table, currentNatTunnel);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (nextNatTunnel == NULL) || (param == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterInitFromSuccessor(&it,
                                      &natTable->tunnels,
                                      currentNatTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterNext(&it, &nextKey, (void**) &natTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *nextNatTunnel = nextKey;
    *param = *natTunnel;

ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatTunnelNext */




/*****************************************************************************/
/** fmAddNatRule
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Add a NAT rule with the given condition, and action to
 *                  a table. Adding multiple NAT rule with the exact same
 *                  condition but different action will automatically creates
 *                  ECMP Group to hash over the possibility.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       rule is the rule ID.
 * 
 * \param[in]       condition is a condition mask (see 'fm_natCondition').
 * 
 * \param[in]       cndParam points to the fm_natConditionParam structure
 *                  (see 'fm_natConditionParam') to match against for the
 *                  given condition.
 * 
 * \param[in]       action is an action mask (see 'fm_natAction').
 * 
 * \param[in]       actParam points to the fm_natActionParam structure
 *                  (see 'fm_natActionParam') to carry extended action
 *                  configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_ALREADY_EXISTS if the rule already exists.
 * \return          FM_ERR_UNSUPPORTED if rule action does not match the table
 *                  configuration.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TABLE_FULL if the table is full.
 * \return          FM_ERR_TUNNEL_CONFLICT if tunnel condition does not match
 *                  hash lookup tunnel group condition.
 * \return          FM_ERR_TUNNEL_COUNT_FULL if no more count resources available.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 * \return          FM_ERR_TUNNEL_BIN_FULL if no more entries can be inserted into
 *                  that specific hash lookup bin.
 * \return          FM_ERR_TUNNEL_NO_ENCAP_FLOW if specified tunnel is invalid.
 *
 *****************************************************************************/
fm_status fmAddNatRule(fm_int                sw,
                       fm_int                table,
                       fm_int                rule,
                       fm_natCondition       condition,
                       fm_natConditionParam *cndParam,
                       fm_natAction          action,
                       fm_natActionParam *   actParam)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken;
    fm_natTable *   natTable;
    fm_natRule *    natRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, rule = %d, "
                     "condition = 0x%x, action = 0x%x\n",
                     sw, table, rule, condition, action);

    err          = FM_OK;
    natLockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (cndParam == NULL) || (actParam == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules, rule, (void**) &natRule);
    if (err == FM_OK)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else if (err != FM_ERR_NOT_FOUND)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    natRule = fmAlloc(sizeof(fm_natRule));

    if (natRule == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    FM_CLEAR(*natRule);

    /* Structure field might be updated by chip specific function if needed */
    natRule->condition = condition;
    natRule->cndParam = *cndParam;
    natRule->action = action;
    natRule->actParam = *actParam;

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddNatRule,
                       sw,
                       table,
                       rule,
                       natRule->condition,
                       &natRule->cndParam,
                       natRule->action,
                       &natRule->actParam);
    if (err == FM_OK)
    {
        err = fmTreeInsert(&natTable->rules, rule, natRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else
    {
        fmFree(natRule);
    }


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmAddNatRule */




/*****************************************************************************/
/** fmDeleteNatRule
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Remove a NAT rule from the table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       rule is the rule ID.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or rule could not be
 *                  found.
 *
 *****************************************************************************/
fm_status fmDeleteNatRule(fm_int sw, fm_int table, fm_int rule)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;
    fm_natRule *    natRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, rule = %d\n",
                     sw, table, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules, rule, (void**) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteNatRule,
                       sw,
                       table,
                       rule);
    if (err == FM_OK)
    {
        err = fmTreeRemoveCertain(&natTable->rules, rule, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmDeleteNatRule */




/*****************************************************************************/
/** fmGetNatRule
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get NAT rule configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       rule is the rule ID.
 * 
 * \param[out]      condition points to caller-supplied storage where this
 *                  function should place the rule condition mask.
 * 
 * \param[out]      cndParam points to caller-supplied storage where this
 *                  function should place the rule condition parameter.
 * 
 * \param[out]      action points to caller-supplied storage where this
 *                  function should place the rule action mask.
 * 
 * \param[out]      actParam points to caller-supplied storage where this
 *                  function should place the rule action parameter.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or rule could not be
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatRule(fm_int                sw,
                       fm_int                table,
                       fm_int                rule,
                       fm_natCondition *     condition,
                       fm_natConditionParam *cndParam,
                       fm_natAction *        action,
                       fm_natActionParam *   actParam)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;
    fm_natRule *    natRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, rule = %d\n",
                     sw, table, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (condition == NULL) || (cndParam == NULL) || 
         (action == NULL) || (actParam == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules, rule, (void**) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *condition = natRule->condition;
    *cndParam = natRule->cndParam;
    *action = natRule->action;
    *actParam = natRule->actParam;


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatRule */




/*****************************************************************************/
/** fmGetNatRuleFirst
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the first NAT rule and configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[out]      firstNatRule points to caller-supplied storage where this
 *                  function should place the first NAT rule retrieved.
 * 
 * \param[out]      condition points to caller-supplied storage where this
 *                  function should place the rule condition mask.
 * 
 * \param[out]      cndParam points to caller-supplied storage where this
 *                  function should place the rule condition parameter.
 * 
 * \param[out]      action points to caller-supplied storage where this
 *                  function should place the rule action mask.
 * 
 * \param[out]      actParam points to caller-supplied storage where this
 *                  function should place the rule action parameter.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_NO_MORE if no NAT rules exist.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatRuleFirst(fm_int                sw,
                            fm_int                table,
                            fm_int *              firstNatRule,
                            fm_natCondition *     condition,
                            fm_natConditionParam *cndParam,
                            fm_natAction *        action,
                            fm_natActionParam *   actParam)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;
    fm_natRule *    natRule;
    fm_uint64       nextKey;
    fm_treeIterator it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (condition == NULL) || (cndParam == NULL) || 
         (action == NULL) || (actParam == NULL) ||
         (firstNatRule == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    fmTreeIterInit(&it, &natTable->rules);

    err = fmTreeIterNext(&it, &nextKey, (void**) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *firstNatRule = nextKey;
    *condition = natRule->condition;
    *cndParam = natRule->cndParam;
    *action = natRule->action;
    *actParam = natRule->actParam;


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatRuleFirst */




/*****************************************************************************/
/** fmGetNatRuleNext
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the Next NAT rule and configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       currentNatRule is the last Rule found by a previous
 *                  call to this function or to ''fmGetNatRuleFirst''.
 * 
 * \param[out]      nextNatRule points to caller-supplied storage where this
 *                  function should place the next NAT rule found.
 * 
 * \param[out]      condition points to caller-supplied storage where this
 *                  function should place the rule condition mask.
 * 
 * \param[out]      cndParam points to caller-supplied storage where this
 *                  function should place the rule condition parameter.
 * 
 * \param[out]      action points to caller-supplied storage where this
 *                  function should place the rule action mask.
 * 
 * \param[out]      actParam points to caller-supplied storage where this
 *                  function should place the rule action parameter.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_NO_MORE if no more NAT rules are available.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatRuleNext(fm_int                sw,
                           fm_int                table,
                           fm_int                currentNatRule,
                           fm_int *              nextNatRule,
                           fm_natCondition *     condition,
                           fm_natConditionParam *cndParam,
                           fm_natAction *        action,
                           fm_natActionParam *   actParam)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_bool         natLockTaken = FALSE;
    fm_natTable *   natTable;
    fm_natRule *    natRule;
    fm_uint64       nextKey;
    fm_treeIterator it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, currentNatRule = %d\n",
                     sw, table, currentNatRule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (condition == NULL) || (cndParam == NULL) || 
         (action == NULL) || (actParam == NULL) ||
         (nextNatRule == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterInitFromSuccessor(&it,
                                      &natTable->rules,
                                      currentNatRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterNext(&it, &nextKey, (void**) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *nextNatRule = nextKey;
    *condition = natRule->condition;
    *cndParam = natRule->cndParam;
    *action = natRule->action;
    *actParam = natRule->actParam;


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatRuleNext */




/*****************************************************************************/
/** fmAddNatPrefilter
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Add a NAT prefilter entry with the given condition to
 *                  a table. This service is only available on table configured
 *                  with NAT mode ''FM_NAT_MODE_RESOURCE''. Any of the
 *                  prefilter entry hit will initiate a lookup in the second
 *                  part of the table populated with NAT rules.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       entry is the prefilter ID.
 * 
 * \param[in]       condition is a condition mask (see 'fm_natCondition').
 * 
 * \param[in]       cndParam points to the fm_natConditionParam structure
 *                  (see 'fm_natConditionParam') to match against for the
 *                  given condition.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_ALREADY_EXISTS if the prefilter already exists.
 * \return          FM_ERR_UNSUPPORTED if rule action does not match the table
 *                  configuration.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TABLE_FULL if the table is full.
 *
 *****************************************************************************/
fm_status fmAddNatPrefilter(fm_int                sw,
                            fm_int                table,
                            fm_int                entry,
                            fm_natCondition       condition,
                            fm_natConditionParam *cndParam)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natPrefilter *natPrefilter;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, entry = %d, condition = 0x%x\n",
                     sw, table, entry, condition);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (cndParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->prefilters, entry, (void**) &natPrefilter);
    if (err == FM_OK)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else if (err != FM_ERR_NOT_FOUND)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    natPrefilter = fmAlloc(sizeof(fm_natPrefilter));

    if (natPrefilter == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* Structure field might be updated by chip specific function if needed */
    natPrefilter->condition = condition;
    natPrefilter->cndParam = *cndParam;

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddNatPrefilter,
                       sw,
                       table,
                       entry,
                       natPrefilter->condition,
                       &natPrefilter->cndParam);
    if (err == FM_OK)
    {
        err = fmTreeInsert(&natTable->prefilters, entry, natPrefilter);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else
    {
        fmFree(natPrefilter);
    }


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmAddNatPrefilter */




/*****************************************************************************/
/** fmDeleteNatPrefilter
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Delete a NAT prefilter entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       entry is the prefilter ID.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or the prefilter could not be
 *                  found.
 *
 *****************************************************************************/
fm_status fmDeleteNatPrefilter(fm_int sw, fm_int table, fm_int entry)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natPrefilter *natPrefilter;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, entry = %d\n",
                     sw, table, entry);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->prefilters, entry, (void**) &natPrefilter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteNatPrefilter,
                       sw,
                       table,
                       entry);
    if (err == FM_OK)
    {
        err = fmTreeRemoveCertain(&natTable->prefilters, entry, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmDeleteNatPrefilter */




/*****************************************************************************/
/** fmGetNatPrefilter
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get NAT prefilter entry configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       entry is the prefilter ID.
 * 
 * \param[out]      condition points to caller-supplied storage where this
 *                  function should place the prefilter entry condition mask.
 * 
 * \param[out]      cndParam points to caller-supplied storage where this
 *                  function should place the prefilter entry condition
 *                  parameter.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or the prefilter could not be
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatPrefilter(fm_int                sw,
                            fm_int                table,
                            fm_int                entry,
                            fm_natCondition *     condition,
                            fm_natConditionParam *cndParam)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natPrefilter *natPrefilter;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, entry = %d\n",
                     sw, table, entry);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (condition == NULL) || (cndParam == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->prefilters, entry, (void**) &natPrefilter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *condition = natPrefilter->condition;
    *cndParam = natPrefilter->cndParam;


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatPrefilter */




/*****************************************************************************/
/** fmGetNatPrefilterFirst
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the First NAT prefilter entry and configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[out]      firstEntry points to caller-supplied storage where this
 *                  function should place the first NAT prefilter entry
 *                  retrieved.
 * 
 * \param[out]      condition points to caller-supplied storage where this
 *                  function should place the prefilter entry condition mask.
 * 
 * \param[out]      cndParam points to caller-supplied storage where this
 *                  function should place the prefilter entry condition
 *                  parameter.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_NO_MORE if no NAT prefilters exist.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 *
 *****************************************************************************/
fm_status fmGetNatPrefilterFirst(fm_int                sw,
                                 fm_int                table,
                                 fm_int *              firstEntry,
                                 fm_natCondition *     condition,
                                 fm_natConditionParam *cndParam)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natPrefilter *natPrefilter;
    fm_uint64        nextKey;
    fm_treeIterator  it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (firstEntry == NULL) || (condition == NULL) || (cndParam == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    fmTreeIterInit(&it, &natTable->prefilters);

    err = fmTreeIterNext(&it, &nextKey, (void**) &natPrefilter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *firstEntry = nextKey;
    *condition = natPrefilter->condition;
    *cndParam = natPrefilter->cndParam;


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatPrefilterFirst */




/*****************************************************************************/
/** fmGetNatPrefilterNext
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Get the Next NAT prefilter entry and configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       currentEntry is the last Entry found by a previous
 *                  call to this function or to ''fmGetNatPrefilterFirst''.
 * 
 * \param[out]      nextEntry points to caller-supplied storage where this
 *                  function should place the next NAT prefilter entry found.
 * 
 * \param[out]      condition points to caller-supplied storage where this
 *                  function should place the prefilter entry condition mask.
 * 
 * \param[out]      cndParam points to caller-supplied storage where this
 *                  function should place the prefilter entry condition
 *                  parameter.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table could not be found.
 * \return          FM_ERR_NO_MORE if no more NAT prefilters are available.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 *
 *****************************************************************************/
fm_status fmGetNatPrefilterNext(fm_int                sw,
                                fm_int                table,
                                fm_int                currentEntry,
                                fm_int *              nextEntry,
                                fm_natCondition *     condition,
                                fm_natConditionParam *cndParam)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natPrefilter *natPrefilter;
    fm_uint64        nextKey;
    fm_treeIterator  it;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, currentEntry = %d\n",
                     sw, table, currentEntry);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if ( (nextEntry == NULL) || (condition == NULL) || (cndParam == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterInitFromSuccessor(&it,
                                      &natTable->prefilters,
                                      currentEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeIterNext(&it, &nextKey, (void**) &natPrefilter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    *nextEntry = nextKey;
    *condition = natPrefilter->condition;
    *cndParam = natPrefilter->cndParam;


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatPrefilterNext */




/*****************************************************************************/
/** fmGetNatRuleCount
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Retrieve the frame and octet counts associated with an
 *                  ''FM_NAT_ACTION_COUNT'' NAT rule action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       rule is the rule ID.
 * 
 * \param[out]      counters points to a caller-supplied structure of type
 *                  ''fm_tunnelCounters'' where this function should place the
 *                  counter values.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or rule could not be found.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the rule does not have a COUNT
 *                  action.
 *
 *****************************************************************************/
fm_status fmGetNatRuleCount(fm_int             sw,
                            fm_int             table,
                            fm_int             rule,
                            fm_tunnelCounters *counters)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natRule *     natRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, rule = %d\n",
                     sw, table, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Validating the input argument */
    if (counters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules, rule, (void**) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetNatRuleCount,
                       sw,
                       table,
                       rule,
                       counters);


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmGetNatRuleCount */




/*****************************************************************************/
/** fmResetNatRuleCount
 * \ingroup nat
 *
 * \chips           FM10000
 *
 * \desc            Reset the frame and octet counts associated with an
 *                  ''FM_NAT_ACTION_COUNT'' NAT rule action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 * 
 * \param[in]       rule is the rule ID.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the table or rule could not be found.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the rule does not have a COUNT
 *                  action.
 *
 *****************************************************************************/
fm_status fmResetNatRuleCount(fm_int sw, fm_int table, fm_int rule)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;
    fm_natTable *    natTable;
    fm_natRule *     natRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT,
                     "sw = %d, table = %d, rule = %d\n",
                     sw, table, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

    err = fmTreeFind(&switchPtr->natInfo->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules, rule, (void**) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetNatRuleCount,
                       sw,
                       table,
                       rule);


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmResetNatRuleCount */




/*****************************************************************************/
/** fmDbgDumpNat
 * \ingroup diagNat
 *
 * \chips           FM10000
 *
 * \desc            Display the NAT status of a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpNat(fm_int sw)
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_bool          natLockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* This structure only gets initialized on switch family that support
     * that support NAT. */
    if (switchPtr->natInfo == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    TAKE_NAT_LOCK(sw);
    natLockTaken = TRUE;

//  FM_API_CALL_FAMILY(err,
//                     switchPtr->DbgDumpNat,
//                     sw);


ABORT:

    if (natLockTaken)
    {
        DROP_NAT_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_NAT, err);

}   /* end fmDbgDumpNat */
