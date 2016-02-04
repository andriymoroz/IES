/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lbg.c
 * Creation Date:   April 3, 2008
 * Description:     Functions for managing load balancing groups.
 *
 * Copyright (c) 2005 - 2016, Intel Corporation
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

/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCreateLBGInt
 * \ingroup intLbg
 *
 * \desc            Create a new load balancing group. Note that this function
 *                  assumes that the caller has validated and protected the
 *                  switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   lbgNumber points to caller allocated storage where this
 *                  function should store the handle of the newly created
 *                  LBG for non-stacking. For stacking mode, the caller
 *                  should specify the desired lbgNumber to be used.
 *
 * \param[in]       params points to the parameter structure for LBGs.
 *
 * \param[in]       stacking is TRUE if this is a stacking LBG, FALSE if not.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if lbgNumber is NULL.
 * \return          FM_ERR_NO_MEM if no memory for LBG structures.
 *
 *****************************************************************************/
fm_status fmCreateLBGInt(fm_int sw, 
                         fm_int *lbgNumber, 
                         fm_LBGParams *params, 
                         fm_bool stacking)
{
    fm_int       err = FM_OK;
    fm_switch *  switchPtr;
    fm_LBGParams globalParams;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%p\n",
                 sw, (void *) lbgNumber);
    
    switchPtr = GET_SWITCH_PTR(sw);

    if (!stacking)
    {
        *lbgNumber = FM_LOGICAL_PORT_ANY;
    }

    if (params == NULL)
    {
        FM_CLEAR(globalParams);
        globalParams.mode = switchPtr->lbgInfo.mode;
        params = &globalParams;
    }

    FM_API_CALL_FAMILY(err, switchPtr->CreateLBG, sw, lbgNumber, params);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmCreateLBGInt */




/*****************************************************************************/
/** fmGetLBGRouteData
 * \ingroup intLbg
 *
 * \desc            This helper method is used by the ACL compiler to retrieve
 *                  information about the LBG in order to appropriately set up
 *                  the route to logical port action.  
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[out]      routeType points to caller allocated storage where the
 *                  routing method used by the load balancing group is
 *                  written.
 *
 * \param[out]      routeData points to caller allocated storage where either
 *                  the ARP base index or glort is written to.
 *
 * \param[out]      dataCount points to caller allocated storage where the
 *                  number of used ARP entries is written to. This argument
 *                  is only written to when routeType is FM_LBG_ROUTE_ARP.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetLBGRouteData(fm_int sw, 
                            fm_int lbgNumber,
                            fm_LBGRouteType *routeType, 
                            fm_int *routeData,
                            fm_int *dataCount)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, routeType=%p, "
                 "routeData=%p, dataCount=%p\n",
                 sw, lbgNumber, (void *) routeType,
                 (void *) routeData, (void *) dataCount);

    if (!routeType || !routeData || !dataCount)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetLBGRouteData,
                       sw, 
                       lbgNumber, 
                       routeType, 
                       routeData,
                       dataCount);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGRouteData */




/*****************************************************************************/
/** fmCreateLBG
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Creates a load balancing group.
 *
 * \note            Deprecated in favor of ''fmCreateLBGExt''.
 *
 * \note            This function should not be used after calling
 *                  ''fmAllocateStackLBGs'' (see ''Stacking and GloRT 
 *                  Management''). Instead, use ''fmCreateStackLBG''.
 * 
 * \note            On FM4000 and FM6000, the LBG handle returned by
 *                  this function is a logical port number. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      lbgNumber points to caller allocated storage where this
 *                  function should place the handle of the newly
 *                  created LBG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if lbgNumber is NULL.
 * \return          FM_ERR_NO_MEM if no memory for LBG structures.
 *
 *****************************************************************************/
fm_status fmCreateLBG(fm_int sw, fm_int *lbgNumber)
{
    fm_status err = FM_OK;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%p\n",
                     sw, (void *) lbgNumber);

    if (!lbgNumber)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    err = fmCreateLBGInt(sw, lbgNumber, NULL, FALSE);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmCreateLBG */




/*****************************************************************************/
/** fmCreateLBGExt
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Creates a load balancing group. This function is similar
 *                  to ''fmCreateLBG'', but provides additional parameters
 *                  for the configuration of the resulting LBG.
 *
 * \note            On FM4000 and FM6000, this function should not be used
 *                  after calling ''fmAllocateStackLBGs'' (see ''Stacking
 *                  and GloRT Management''). Use ''fmCreateStackLBGExt''
 *                  instead.
 *
 * \note            On FM10000, if ''fmAllocateStackLBGs'' was previously
 *                  called, this function should not be used to create
 *                  LBGs of mode ''FM_LBG_MODE_MAPPED_L234HASH'' (see
 *                  ''Stacking and GloRT Management'').
 *                  Use ''fmCreateStackLBGExt'' instead.
 *
 * \note            On FM4000 and FM6000, the LBG handle returned by
 *                  this function is a logical port number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      lbgNumber points to caller allocated storage where this
 *                  function should place the handle of the newly
 *                  created LBG.
 *
 * \param[in]       params points to the LBG parameter structure, used for
 *                  certain LBG modes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if lbgNumber is NULL or if
 *                  one of the parameters is invalid. 
 * \return          FM_ERR_NO_MEM if no memory for LBG structures.
 *
 *****************************************************************************/
fm_status fmCreateLBGExt(fm_int sw, fm_int *lbgNumber, fm_LBGParams *params)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%p, params=%p\n",
                     sw, (void *) lbgNumber, (void *) params);

    if (!lbgNumber || !params)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    err = fmCreateLBGInt(sw, lbgNumber, params, FALSE);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmCreateLBGExt */




/*****************************************************************************/
/** fmDeleteLBG
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes a load balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG to delete, as returned
 *                  by ''fmCreateLBG'' or ''fmCreateLBGExt'', or specified
 *                  in the call to ''fmCreateStackLBG''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteLBG(fm_int sw, fm_int lbgNumber)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d\n",
                     sw, lbgNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DeleteLBG, sw, lbgNumber);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmDeleteLBG */




/*****************************************************************************/
/** fmAddLBGPort
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Adds a new member port to a load balancing group. The
 *                  LBG must not be in the active state when adding ports.
 *                  Ports are added in the FM_LBG_PORT_STANDBY mode. This
 *                  function only applies to load balancing group modes
 *                  ''FM_LBG_MODE_NPLUS1'' and ''FM_LBG_MODE_REDIRECT''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       port is the logical port number of the member port to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_LBG_STATE if the LBG is currently in the
 *                  active state.
 * \return          FM_ERR_NO_MEM if no memory available for data structures.
 *
 *****************************************************************************/
fm_status fmAddLBGPort(fm_int sw, fm_int lbgNumber, fm_int port)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, port=%d\n",
                     sw, lbgNumber, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    /* Support for (non-cpu) cardinal and remote ports */
    if ( !fmIsValidPort(sw, port, ALLOW_REMOTE) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->AddLBGPort, sw, lbgNumber, port);

    FM_DROP_LBG_LOCK(sw);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmAddLBGPort */




/*****************************************************************************/
/** fmDeleteLBGPort
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes a load balancing group member port. The
 *                  LBG must not be in the active state when deleting ports.
 *                  This function only applies to load balancing group modes
 *                  ''FM_LBG_MODE_NPLUS1'' and ''FM_LBG_MODE_REDIRECT''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       port is the logical port number of the member port to
 *                  delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_LBG_STATE if the LBG is currently in the
 *                  active state.
 * \return          FM_ERR_PORT_NOT_LBG_MEMBER if port is not a member of the
 *                  LBG.
 *
 *****************************************************************************/
fm_status fmDeleteLBGPort(fm_int sw, fm_int lbgNumber, fm_int port)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, port=%d\n",
                     sw, lbgNumber, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    /* Support for (non-cpu) cardinal and remote ports */
    if ( !fmIsValidPort(sw, port, ALLOW_REMOTE) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DeleteLBGPort, sw, lbgNumber, port);

    FM_DROP_LBG_LOCK(sw);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmDeleteLBGPort */




/*****************************************************************************/
/** fmGetLBGList
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            List all existing load balancing groups.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numLBGs points to caller-allocated storage where this
 *                  function should place the number of LBGs listed.
 *
 * \param[out]      lbgList points to a caller-allocated array of size max 
 *                  where this function should place the list of existing 
 *                  LBG handles.
 *
 * \param[in]       max is the size of lbgList, being the maximum number of
 *                  LBG handles that may be accommodated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if numLBGs or lbgList is NULL.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the list of LBGs.
 *
 *****************************************************************************/
fm_status fmGetLBGList(fm_int sw, fm_int *numLBGs, fm_int *lbgList, fm_int max)
{
    fm_status       err = FM_OK;
    fm_LBGInfo *    info;
    fm_treeIterator iter;
    fm_uint64       nextKey;
    void *          nextData;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, numLBGs=%p, lbgList=%p, max=%d\n",
                     sw, (void *) numLBGs, (void *) lbgList, max);

    if (!lbgList || !numLBGs)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    info = GET_LBG_INFO(sw);

    *numLBGs = 0;

    fmTreeIterInit(&iter, &info->groups);

    err = fmTreeIterNext(&iter, &nextKey, &nextData);

    while (err == FM_OK)
    {
        if (*numLBGs < max)
        {
            lbgList[(*numLBGs)++] = (fm_uint32) nextKey;
    
            err = fmTreeIterNext(&iter, &nextKey, &nextData);
        }
        else
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }
    }

    /* This is the only valid error */
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }
    
    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGList */




/*****************************************************************************/
/** fmGetLBGFirst
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first existing load balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstLbgNumber points to caller-allocated storage where
 *                  this function should place the first existing LBG 
 *                  handle. If no load balancing groups exist, 
 *                  firstLbgNumber will be set to -1.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if firstLbgNumber is NULL.
 * \return          FM_ERR_NO_MORE if there are no LBGs.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if an LBG was
 *                  created or deleted while searching for the first LBG.
 *
 *****************************************************************************/
fm_status fmGetLBGFirst(fm_int sw, fm_int *firstLbgNumber)
{
    fm_status       err = FM_OK;
    fm_LBGInfo *    info;
    fm_treeIterator iter;
    fm_uint64       nextKey;
    void *          nextData;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, firstLbgNumber=%p\n",
                     sw, (void *) firstLbgNumber);

    if (!firstLbgNumber)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    info = GET_LBG_INFO(sw);

    fmTreeIterInit(&iter, &info->groups);

    err = fmTreeIterNext(&iter, &nextKey, &nextData);

    *firstLbgNumber = (err != FM_ERR_NO_MORE) ?
                      ( (fm_int32) nextKey ) :
                      -1;

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGFirst */




/*****************************************************************************/
/** fmGetLBGNext
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next existing load balancing group, following 
 *                  a prior call to this function or to ''fmGetLBGFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentLbgNumber is the last LBG found by a previous call 
 *                  to this function or to ''fmGetLBGFirst''.
 *
 * \param[out]      nextLbgNumber points to caller-allocated storage where this 
 *                  function should place the next existing LBG. Will be set 
 *                  to -1 if no more LBGs found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if nextLbgNumber is NULL.
 * \return          FM_ERR_NO_MORE if there are no more LBGs.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if an LBG was
 *                  created or deleted while searching for the next LBG.
 *****************************************************************************/
fm_status fmGetLBGNext(fm_int sw, fm_int currentLbgNumber,
                       fm_int *nextLbgNumber)
{
    fm_status       err = FM_OK;
    fm_LBGInfo *    info;
    fm_treeIterator iter;
    fm_uint64       nextKey;
    void *          nextData;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, currentLbgNumber=%d, nextLbgNumber=%p\n",
                     sw, currentLbgNumber, (void *) nextLbgNumber);

    if (!nextLbgNumber)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    info = GET_LBG_INFO(sw);

    err = fmTreeIterInitFromKey(&iter, &info->groups, currentLbgNumber);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    
    err = fmTreeIterNext(&iter, &nextKey, &nextData);

    if (err == FM_OK)
    {
        err = fmTreeIterNext(&iter, &nextKey, &nextData);

        *nextLbgNumber = (err != FM_ERR_NO_MORE) ?
                         ( (fm_int32) nextKey ) :
                         -1;
    }

ABORT:
    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGNext */




/*****************************************************************************/
/** fmGetLBGPortList
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            List all member ports in a load balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[out]      numPorts points to caller-allocated storage where this
 *                  function should place the number of LBG member ports listed.
 *
 * \param[out]      portList points to a caller-allcoated array of size max 
 *                  where this function should place the list of LBG member
 *                  ports.
 *
 * \param[in]       max is the size of portList, being the maximum number of
 *                  LBG member ports that may be accommodated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if numPorts or portList is NULL.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the list of LBG member ports.
 *****************************************************************************/
fm_status fmGetLBGPortList(fm_int sw, fm_int lbgNumber, fm_int *numPorts,
                           fm_int *portList, fm_int max)
{
    fm_status        err = FM_OK;
    fm_LBGInfo *     info;
    fm_LBGGroup *    group;
    fm_intLBGMember *member;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, numPorts=%p, portList=%p, max=%d\n",
                     sw, lbgNumber, (void *) numPorts, (void *) portList, max);

    if (!numPorts || !portList)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);

    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_LBG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    for (*numPorts = 0, member = group->firstMember ;
         member ;
         member = member->nextMember)
    {
        if (*numPorts < max)
        {
            portList[(*numPorts)++] = member->lbgMemberPort;
        }
        else
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }
    }

ABORT:
    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGPortList */




/*****************************************************************************/
/** fmGetLBGPortFirst
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first member port of a load balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[out]      firstLbgPort points to caller-allocated storage where
 *                  this function should place the first member port of the
 *                  specified LBG. If LBG has no member ports, firstLbgPort 
 *                  will be set to -1.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if firstLbgPort is NULL.
 * \return          FM_ERR_NO_MORE if there are no member ports in the LBG.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if an LBG was
 *                  created or deleted while searching for the first member
 *                  port.
 *****************************************************************************/
fm_status fmGetLBGPortFirst(fm_int sw, fm_int lbgNumber, fm_int *firstLbgPort)
{
    fm_status    err = FM_OK;
    fm_LBGInfo * info;
    fm_LBGGroup *group;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, firstLbgPort=%p\n",
                     sw, lbgNumber, (void *) firstLbgPort);

    if (!firstLbgPort)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);

    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_LBG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    if (group->firstMember)
    {
        *firstLbgPort = group->firstMember->lbgMemberPort;
    }
    else
    {
        err = FM_ERR_NO_MORE;
        *firstLbgPort = -1;
    }

ABORT:
    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGPortFirst */




/*****************************************************************************/
/** fmGetLBGPortNext
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next member port of a load balancing group, 
 *                  following a prior call to this function or to 
 *                  ''fmGetLBGPortFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       currentLbgPort is the last LBG member port found by a 
 *                  previous call to this function or to ''fmGetLBGPortFirst''.
 *
 * \param[out]      nextLbgPort points to caller-allocated storage where this 
 *                  function should place the next LBG member port. Will be set 
 *                  to -1 if no more member ports found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if nextLbgPort is NULL.
 * \return          FM_ERR_NO_MORE if there are no more member ports in the LBG.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if an LBG member port was 
 *                  added or deleted while searching for the next member port.
 *****************************************************************************/
fm_status fmGetLBGPortNext(fm_int sw, fm_int lbgNumber, fm_int currentLbgPort,
                           fm_int *nextLbgPort)
{
    fm_status        err = FM_OK;
    fm_LBGInfo *     info;
    fm_LBGGroup *    group;
    fm_intLBGMember *member;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, currentLbgPort=%d, nextLbgPort=%p\n",
                     sw, lbgNumber, currentLbgPort, (void *) nextLbgPort);

    if (!nextLbgPort)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);

    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_LBG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    member = group->firstMember;

    while ( member && (member->lbgMemberPort != currentLbgPort) )
    {
        member = member->nextMember;
    }

    if (!member ||
        (member->lbgMemberPort != currentLbgPort ) ||
        !member->nextMember)
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    *nextLbgPort = member->nextMember->lbgMemberPort;

ABORT:
    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGPortNext */




/*****************************************************************************/
/** fmSetLBGAttribute
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a load balancing group attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       attr is the load balancing group attribute (see 
 *                  ''Load Balancing Group Attributes'') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 * \return          FM_ERR_UNSUPPORTED if attr is not recognized.
 * \return          FM_ERR_READONLY_ATTRIB if attr is read-only.
 *
 *****************************************************************************/
fm_status fmSetLBGAttribute(fm_int sw, 
                            fm_int lbgNumber, 
                            fm_int attr,
                            void *value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, attr=%d, value=%p\n",
                     sw, lbgNumber, attr, (void *) value);

    if (!value)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Acquire readLock on routingLock since mcast group info may
     * be accessed later. */
    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetLBGAttribute,
                       sw, 
                       lbgNumber, 
                       attr, 
                       value);

    fmReleaseReadLock(&switchPtr->routingLock);

ABORT:
    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmSetLBGAttribute */




/*****************************************************************************/
/** fmGetLBGAttribute
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a load balancing group attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       attr is the load balancing group attribute (see 
 *                  ''Load Balancing Group Attributes'') to get.
 *
 * \param[in]       value points to call-allocated storage where this function
 *                  should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 * \return          FM_ERR_UNSUPPORTED if attr is not recognized.
 *
 *****************************************************************************/
fm_status fmGetLBGAttribute(fm_int sw, fm_int lbgNumber, fm_int attr,
                            void *value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, attr=%d, value=%p\n",
                     sw, lbgNumber, attr, (void *) value);

    if (!value)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetLBGAttribute,
                       sw, 
                       lbgNumber, 
                       attr, 
                       value);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGAttribute */




/*****************************************************************************/
/** fmSetLBGPortAttribute
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a load balancing group member port attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       port is the logical port number of the member port.
 *
 * \param[in]       attr is the load balancing group port attribute (see 
 *                  ''Load Balancing Group Port Attributes'') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 * \return          FM_ERR_UNSUPPORTED if attr is not recognized.
 *
 *****************************************************************************/
fm_status fmSetLBGPortAttribute(fm_int sw,
                                fm_int lbgNumber,
                                fm_int port,
                                fm_int attr,
                                void * value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, port=%d, attr=%d, value=%p\n",
                     sw, lbgNumber, port, attr, (void *) value);

    if (!value)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( !fmIsValidPort(sw, port, ALLOW_REMOTE) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetLBGPortAttribute,
                       sw, 
                       lbgNumber, 
                       port, 
                       attr, 
                       value);

    FM_DROP_LBG_LOCK(sw);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmSetLBGPortAttribute */




/*****************************************************************************/
/** fmGetLBGPortAttribute
 * \ingroup lbg
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a load balancing group member port attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the handle of the LBG.
 *
 * \param[in]       port is the logical port number of the member port.
 *
 * \param[in]       attr is the load balancing group port attribute (see 
 *                  ''Load Balancing Group Port Attributes'') to get.
 *
 * \param[in]       value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 * \return          FM_ERR_UNSUPPORTED if attr is not recognized.
 *
 *****************************************************************************/
fm_status fmGetLBGPortAttribute(fm_int sw,
                                fm_int lbgNumber,
                                fm_int port,
                                fm_int attr,
                                void * value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_LBG,
                     "sw=%d, lbgNumber=%d, port=%d, attr=%d, value=%p\n",
                     sw, lbgNumber, port, attr, (void *) value);

    if (!value)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( !fmIsValidPort(sw, port, ALLOW_REMOTE) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetLBGPortAttribute,
                       sw, 
                       lbgNumber, 
                       port, 
                       attr, 
                       value);

    FM_DROP_LBG_LOCK(sw);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmGetLBGPortAttribute */




/*****************************************************************************/
/** fmDbgDumpLBG
 * \ingroup debug
 *
 * \desc            Prints out debugging information related to the LBG group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the group number.  Use -1 to dump information
 *                  about all LBGs.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpLBG(fm_int sw, fm_int lbgNumber)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
    fm_int      lbgList[256];
    fm_int      numLbgs;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, "sw=%d, lbg=%d\n", sw, lbgNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (lbgNumber != -1)
    {
        FM_API_CALL_FAMILY(err, switchPtr->DbgDumpLBG, sw, lbgNumber);
    }
    else
    {
        err = fmGetLBGList(sw, &numLbgs, lbgList, 256);

        if (err == FM_OK)
        {
            for (lbgNumber = 0 ; lbgNumber < numLbgs ; lbgNumber++)
            {
                /* Ignore errors here */
                FM_API_CALL_FAMILY(err,
                                   switchPtr->DbgDumpLBG,
                                   sw,
                                   lbgList[lbgNumber]);
            }
        }
    }

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmDbgDumpLBG */
