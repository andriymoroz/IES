/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_policer.c
 * Creation Date:   2007
 * Description:     Structures and functions for dealing with ACL policers.
 *
 * Copyright (c) 2007 - 2014, Intel Corporation
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

#define POLICER_VALID(pol)  ( (pol) >= 0 )

#define BANK_VALID(sw, bank)                 \
    ( (bank) == FM_POLICER_BANK_AUTOMATIC || \
     ( (bank) >= 0 &&                        \
      (bank) < fmRootApi->fmSwitchStateTable[(sw)]->policerBanks ) )

#define FM_API_PREAMBLE(fmt, ...)                       \
    fm_status err = FM_OK;                              \
    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL, fmt, __VA_ARGS__); \
    VALIDATE_AND_PROTECT_SWITCH(sw);                    \
    FM_TAKE_ACL_LOCK(sw)

#define FM_API_POSTAMBLE                               \
    goto ABORT; /* avoid warning about unused label */ \
ABORT:                                                 \
    FM_DROP_ACL_LOCK(sw);                              \
    UNPROTECT_SWITCH(sw);                              \
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err)

#define FM_API_ABORT(failCode) \
    err = failCode;            \
    goto ABORT

#define FM_API_REQUIRE(expr, failCode) \
    if ( !(expr) )                     \
    {                                  \
        err = failCode;                \
        goto ABORT;                    \
    }

/* for use in function GetAttributeLocation */
#define RETURN_POLICER_ATTRIBUTE(attrname)              \
    if (treeErr == FM_OK)                               \
    {                                                   \
        *location = &entry->attributes.attrname;        \
        *size     = sizeof(entry->attributes.attrname); \
    }                                                   \
    return treeErr

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
/** GetAttributeLocation
 * \ingroup intPolicer
 *
 * \desc            This function contains the logic for the policer
 *                  attributes in one place, instead of replicating the
 *                  logic in both the get and set functions, as is done
 *                  in other attribute systems in the API.  This function
 *                  returns the location and size of the specified attribute,
 *                  and the caller can then copy into or out of it as
 *                  appropriate.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number whose attribute is to be
 *                  accessed. This argument is ignored for global attributes.
 *
 * \param[in]       attr is the policer attribute to access (see
 *                  ''Policer Attributes'').
 *
 * \param[out]      location receives the address of the attribute
 *
 * \param[out]      size receives the size of the attribute, in bytes
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_POLICER if policer is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 *
 *****************************************************************************/
static fm_status GetAttributeLocation(fm_int  sw,
                                      fm_int  policer,
                                      fm_int  attr,
                                      void ** location,
                                      fm_int *size)
{
    void *                value;
    fm_individualPolicer *entry;
    fm_policerInfo *      info;
    fm_status             treeErr;

    info    = &GET_SWITCH_PTR(sw)->policerInfo;
    treeErr = fmTreeFind(&info->policers, policer, &value);
    entry   = (fm_individualPolicer *) value;

    if (treeErr == FM_ERR_NOT_FOUND)
    {
        treeErr = FM_ERR_INVALID_POLICER;
    }

    switch (attr)
    {
        case FM_POLICER_MKDN_DSCP:
            RETURN_POLICER_ATTRIBUTE(mkdnDscp);

        case FM_POLICER_MKDN_SWPRI:
            RETURN_POLICER_ATTRIBUTE(mkdnSwPri);

        case FM_POLICER_COLOR_SOURCE:
            RETURN_POLICER_ATTRIBUTE(colorSource);

        case FM_POLICER_CIR_ACTION:
            RETURN_POLICER_ATTRIBUTE(cirAction);

        case FM_POLICER_CIR_ACTION_DATA_DSCP:
            RETURN_POLICER_ATTRIBUTE(cirActionData.dscp);

        case FM_POLICER_CIR_ACTION_DATA_VPRI:
            RETURN_POLICER_ATTRIBUTE(cirActionData.vPri);

        case FM_POLICER_CIR_ACTION_DATA_SWPRI:
            RETURN_POLICER_ATTRIBUTE(cirActionData.swPri);

        case FM_POLICER_CIR_CAPACITY:
            RETURN_POLICER_ATTRIBUTE(cirCapacity);

        case FM_POLICER_CIR_RATE:
            RETURN_POLICER_ATTRIBUTE(cirRate);

        case FM_POLICER_EIR_ACTION:
            RETURN_POLICER_ATTRIBUTE(eirAction);

        case FM_POLICER_EIR_ACTION_DATA_DSCP:
            RETURN_POLICER_ATTRIBUTE(eirActionData.dscp);

        case FM_POLICER_EIR_ACTION_DATA_VPRI:
            RETURN_POLICER_ATTRIBUTE(eirActionData.vPri);

        case FM_POLICER_EIR_ACTION_DATA_SWPRI:
            RETURN_POLICER_ATTRIBUTE(eirActionData.swPri);

        case FM_POLICER_EIR_CAPACITY:
            RETURN_POLICER_ATTRIBUTE(eirCapacity);

        case FM_POLICER_EIR_RATE:
            RETURN_POLICER_ATTRIBUTE(eirRate);

        case FM_POLICER_SWPRI_MKDN_MAP:
            *location = info->swPriMkdnMap;
            *size     = sizeof(info->swPriMkdnMap);
            return FM_OK;

        case FM_POLICER_DSCP_MKDN_MAP:
            *location = info->dscpMkdnMap;
            *size     = sizeof(info->dscpMkdnMap);
            return FM_OK;

        default:
            return FM_ERR_INVALID_ARGUMENT;

    }   /* end switch (attr) */

}   /* end GetAttributeLocation */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmCreatePolicer
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a policer. Policers are used to manage
 *                  ingress bandwidth usage based on traffic flows
 *                  characterized by an ACL rule. Once created, a policer is
 *                  associated with an ACL rule in a call to ''fmAddACLRuleExt''.
 *                                                                      \lb\lb
 *                  Policers can be used to manage the bandwidth of a traffic
 *                  flow, or merely count frames and octets for a flow.
 *                                                                      \lb\lb
 *                  Policers are organized into "banks." Multiple ACLs can
 *                  simultaneously "hit" multiple policers, as long as they
 *                  are in different banks. The application need not be
 *                  concerned with the selection of which policers are placed
 *                  in which banks; the ACL compiler (''fmCompileACL'') will
 *                  make the placements automatically. However, not all
 *                  combinations of policer attributes are possible in hardware
 *                  (some attributes are per-bank). The user may want to
 *                  control the arrangement of policers in banks to better
 *                  understand which attribute combinations are foiling the
 *                  compiler. Whether bank selection is performed by the
 *                  application or by the ACL compiler, the choice should be
 *                  consistent for all policers created.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank is the bank in which to create the policer.
 *                  Set to FM_POLICER_BANK_AUTOMATIC to have the ACL compiler
 *                  (''fmCompileACL'') automatically assign the policer to a
 *                  bank.
 *
 * \param[in]       policer is the policer number to create. Policer numbers
 *                  are unique across all banks.
 *
 * \param[in]       config points to a structure of type ''fm_policerConfig''
 *                  that contains the initial configuration of the policer.
 *                  Alternatively, config can be NULL and the policer
 *                  configuration can be set with calls to
 *                  ''fmSetPolicerAttribute''
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_POLICER if policer already created.
 * \return          FM_ERR_INVALID_ARGUMENT if bank or policer values are
 *                  out of range.
 *
 *****************************************************************************/
fm_status fmCreatePolicer(fm_int                  sw,
                          fm_int                  bank,
                          fm_int                  policer,
                          const fm_policerConfig *config)
{
    fm_policerInfo *      info;
    fm_individualPolicer *entry;
    fm_switch *           switchPtr;

    FM_API_PREAMBLE("sw = %d, bank = %d, policer = %d, config = %p\n",
                    sw,
                    bank,
                    policer,
                    (void *) config);

    FM_API_REQUIRE(POLICER_VALID(policer) && BANK_VALID(sw, bank),
                   FM_ERR_INVALID_ARGUMENT);

    switchPtr = GET_SWITCH_PTR(sw);

    info  = &switchPtr->policerInfo;
    entry = (fm_individualPolicer *) fmAlloc( sizeof(fm_individualPolicer) );
    FM_API_REQUIRE(entry != NULL, FM_ERR_NO_MEM);

    entry->bank = bank;

    if (config == NULL)
    {
        /* default attribute values */
        entry->attributes.mkdnDscp    = FM_DISABLED;
        entry->attributes.mkdnSwPri   = FM_DISABLED;
        entry->attributes.colorSource = FM_POLICER_COLOR_SRC_GREEN;
        entry->attributes.cirAction   = FM_POLICER_ACTION_DROP;
        entry->attributes.cirCapacity = 0;
        entry->attributes.cirRate     = 0;
        entry->attributes.eirAction   = FM_POLICER_ACTION_DROP;
        entry->attributes.eirCapacity = 0;
        entry->attributes.eirRate     = 0;
        entry->attributes.cirActionData.dscp = 0;
        entry->attributes.cirActionData.swPri = 0;
        entry->attributes.cirActionData.vPri = 0;
        entry->attributes.eirActionData.dscp = 0;
        entry->attributes.eirActionData.swPri = 0;
        entry->attributes.eirActionData.vPri = 0;
    }
    else
    {
        entry->attributes = *config;
    }

    err = fmTreeInsert(&info->policers, policer, entry);

    if (err != FM_OK)
    {
        fmFree(entry);

        if (err == FM_ERR_ALREADY_EXISTS)
        {
            err = FM_ERR_INVALID_POLICER;
        }
    }

    if ( (err == FM_OK) && (switchPtr->CreatePolicer != NULL) )
    {
        /* for SWAG */
        err = switchPtr->CreatePolicer(sw, bank, policer, config);

        /* Try to return a clean state by removing the policer from switches that
         * actually created it (if some). Sorting order of creation and deletion
         * should be the same. */
        if (err != FM_OK)
        {
            switchPtr->DeletePolicer(sw, policer);
            fmTreeRemoveCertain(&info->policers, policer, fmFree);
        }
    }

    FM_API_POSTAMBLE;

}   /* end fmCreatePolicer */




/*****************************************************************************/
/** fmDeletePolicer
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a policer previously created with a call to
 *                  fmCreatePolicer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_POLICER if the specified policer number
 *                  does not exist in the specified bank.
 * \return          FM_ERR_INVALID_ARGUMENT if policer value is out of range.
 *
 *****************************************************************************/
fm_status fmDeletePolicer(fm_int sw,
                          fm_int policer)
{
    fm_policerInfo *info;
    fm_switch *     switchPtr;

    FM_API_PREAMBLE("sw = %d, policer = %d\n", sw, policer);

    FM_API_REQUIRE(POLICER_VALID(policer), FM_ERR_INVALID_ARGUMENT);

    switchPtr = GET_SWITCH_PTR(sw);

    info = &switchPtr->policerInfo;

    err = fmTreeRemove(&info->policers, policer, fmFree);

    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_POLICER;
    }

    if ( (err == FM_OK) && (switchPtr->DeletePolicer != NULL) )
    {
        /* for SWAG */
        err = switchPtr->DeletePolicer(sw, policer);
    }

    FM_API_POSTAMBLE;

}   /* end fmDeletePolicer */




/*****************************************************************************/
/** fmSetPolicerAttribute
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a policer attribute.
 *
 * \note            Some attributes are per-policer, some are per-bank and
 *                  some are global. If two policers are given conflicting
 *                  values for an attribute with bank-wide scope, the ACL
 *                  compiler (''fmCompileACL'') will try to put those policers
 *                  in different banks. If there are not enough banks in the
 *                  hardware to support all the conflicting attribute values,
 *                  the compiler will generate an error. See
 *                  ''Policer Attributes'' to determine the scope of each
 *                  attribute.
 *                                                                      \lb\lb
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number whose attribute is to be
 *                  set. This argument is ignored for global attributes.
 *
 * \param[in]       attr is the policer attribute to set (see
 *                  ''Policer Attributes'').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_POLICER if policer is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attempting to set a read-only
 *                  attribute.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 * \return          FM_ERR_INVALID_VALUE if value points to an invalid value
 *                  for the specified attribute.
 *
 *
 *****************************************************************************/
fm_status fmSetPolicerAttribute(fm_int      sw,
                                fm_int      policer,
                                fm_int      attr,
                                const void *value)
{
    void *     location;
    fm_int     size;
    fm_switch *switchPtr;

    FM_API_PREAMBLE("sw = %d, policer = %d, attr = %d, value = %p\n",
                    sw,
                    policer,
                    attr,
                    (void *) value);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    err = GetAttributeLocation(sw, policer, attr, &location, &size);

    if (err == FM_OK)
    {
        FM_MEMCPY_S(location, size, value, size);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (err == FM_OK) && (switchPtr->SetPolicerAttribute != NULL) )
    {
        /* for SWAG */
        err = switchPtr->SetPolicerAttribute(sw, policer, attr, value);
    }

    FM_API_POSTAMBLE;

}   /* end fmSetPolicerAttribute */




/*****************************************************************************/
/** fmGetPolicerAttribute
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a policer attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number whose attribute is to be
 *                  retrieved.
 *
 * \param[in]       attr is the policer attribute to get (see
 *                  ''Policer Attributes'').
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_POLICER if policer is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fmGetPolicerAttribute(fm_int     sw,
                                fm_int     policer,
                                fm_int     attr,
                                fm_voidptr value)
{
    void * location;
    fm_int size;

    FM_API_PREAMBLE("sw = %d, policer = %d, attr = %d, value = %p\n",
                    sw,
                    policer,
                    attr,
                    (void *) value);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    err = GetAttributeLocation(sw, policer, attr, &location, &size);

    if (err == FM_OK)
    {
        FM_MEMCPY_S(value, size, location, size);
    }

    FM_API_POSTAMBLE;

}   /* end fmGetPolicerAttribute */




/*****************************************************************************/
/** fmGetPolicerFirst
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first policer number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstPolicer points to caller-allocated storage
 *                  where this function should place the number of the first
 *                  policer.  Will be set to -1 if no policers are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_POLICERS if no policers found.
 *
 *****************************************************************************/
fm_status fmGetPolicerFirst(fm_int sw, fm_int *firstPolicer)
{
    fm_policerInfo *info;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_API_PREAMBLE("sw = %d, firstPolicer = %p\n",
                    sw, (void *) firstPolicer);

    if (firstPolicer == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    info = &GET_SWITCH_PTR(sw)->policerInfo;
    fmTreeIterInit(&it, &info->policers);
    err = fmTreeIterNext(&it, &nextKey, &nextValue);

    if (err == FM_OK)
    {
        *firstPolicer = nextKey;
    }
    else if (err == FM_ERR_NO_MORE)
    {
        *firstPolicer = -1;
        err           = FM_ERR_NO_POLICERS;
    }

    FM_API_POSTAMBLE;

}   /* end fmGetPolicerFirst */




/*****************************************************************************/
/** fmGetPolicerNext
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next policer number, following a prior call
 *                  to this function or to ''fmGetPolicerFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentPolicer is the last policer number
 *                  found by a previous call to this function or to
 *                  ''fmGetPolicerFirst''.
 *
 * \param[out]      nextPolicer points to caller-allocated storage
 *                  where this function should place the number of the next
 *                  policer. Will be set to -1 if no more policers found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_POLICER if currentPolicer is out
 *                  of range or is not the handle of an existing policer.
 * \return          FM_ERR_NO_POLICERS if no more policers found.
 *
 *****************************************************************************/
fm_status fmGetPolicerNext(fm_int  sw,
                           fm_int  currentPolicer,
                           fm_int *nextPolicer)
{
    fm_policerInfo *info;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_API_PREAMBLE("sw = %d, currentPolicer = %d, nextPolicer = %p\n",
                    sw,
                    currentPolicer,
                    (void *) nextPolicer);

    if (nextPolicer == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    info = &GET_SWITCH_PTR(sw)->policerInfo;
    err  = fmTreeSuccessor(&info->policers,
                           currentPolicer,
                           &nextKey,
                           &nextValue);

    if (err == FM_OK)
    {
        *nextPolicer = nextKey;
    }
    else if (err == FM_ERR_NO_MORE)
    {
        *nextPolicer = -1;
        err          = FM_ERR_NO_POLICERS;
    }
    else if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_POLICER;
    }

    FM_API_POSTAMBLE;

}   /* end fmGetPolicerNext */




/*****************************************************************************/
/** fmGetPolicerList
 * \ingroup policer
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return a list of valid policer numbers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numPolicers points to caller allocated storage where
 *                  this function should place the number of valid policers
 *                  returned in policers.
 *
 * \param[out]      policers is an array that this function will fill
 *                  with the list of valid policer numbers.
 *
 * \param[in]       max is the size of policers, being the maximum number of
 *                  policer numbers that policers can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of valid policer numbers.
 *
 *****************************************************************************/
fm_status fmGetPolicerList(fm_int  sw,
                           fm_int *numPolicers,
                           fm_int *policers,
                           fm_int  max)
{
    fm_policerInfo *info;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_API_PREAMBLE("sw = %d, numPolicers = %p, policers = %p, max = %d\n",
                    sw,
                    (void *) numPolicers,
                    (void *) policers,
                    max);

    if ((numPolicers == NULL) || (policers == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    *numPolicers = 0;
    info         = &GET_SWITCH_PTR(sw)->policerInfo;

    for (fmTreeIterInit(&it, &info->policers) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        FM_API_REQUIRE(*numPolicers < max, FM_ERR_BUFFER_FULL);
        policers[(*numPolicers)++] = nextKey;
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    FM_API_POSTAMBLE;

}   /* end fmGetPolicerList */




/*****************************************************************************/
/** fmUpdatePolicer
 * \ingroup policer
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Updates policer configuration in hardware without 
 *                  disrupting existing ACLs. The configuration is updated
 *                  with values obtained from the policer attributes. 
 *                  This function is used to update the configuration of a
 *                  policer that has already been created, and whose previous
 *                  configuration is in hardware. For setting policer
 *                  attributes see ''fmSetPolicerAttribute''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number to be updated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_POLICER if policer is not present in the
 *                  software cache or has invalid attribute values.
 * \return          FM_ERR_INVALID_ACL_IMAGE if no applied acl image is found.
 * \return          FM_ERR_INVALID_COUNTER_BANK if policer's current bank is 
 *                  invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if policer's index in the bank 
 *                  is invalid.
 * \return          FM_ERR_UNSUPPORTED if this api is not supported by this
 *                  type of switch.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_CONFLICT_POLICER_PARAM if the new configuration
 *                  conflicts with the policer bank configuration.
 *                  (FM10000 only)
 *
 *****************************************************************************/
fm_status fmUpdatePolicer(fm_int sw, fm_int policer)
{
    fm_switch      *switchPtr;
    fm_policerInfo *info;
    void           *value;

    FM_API_PREAMBLE("sw = %d, policer = %d\n",
                    sw,
                    policer);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = &switchPtr->policerInfo;

    err = fmTreeFind(&info->policers, policer, &value);

    if (value == NULL || err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_POLICER;
        FM_API_ABORT(err);
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdatePolicer,
                       sw,
                       policer);
    
    FM_API_POSTAMBLE;

}   /* end fmUpdatePolicer */




/*****************************************************************************/
/** fmInitPolicers
 * \ingroup intPolicer
 *
 * \desc            Initialize the policerInfo.
 *
 * \note            This function is called from
 *                  fmInitializeSwitchDataStructure.
 *
 * \param[out]      info is the policerInfo
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmInitPolicers(fm_policerInfo *info)
{
    fmTreeInit(&info->policers);

    return FM_OK;

}   /* end fmInitPolicers */




/*****************************************************************************/
/** fmDestroyPolicers
 * \ingroup intPolicer
 *
 * \desc            Free the memory used by policerInfo.
 *
 * \param[out]      info is the policerInfo
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmDestroyPolicers(fm_policerInfo *info)
{
    if (!fmTreeIsInitialized(&info->policers))
    {
        /* Initialization has not been called due to
         * switch bring up error, so when cleanup
         * just return here.
         */
        return FM_OK;
    }

    fmTreeDestroy(&info->policers, fmFree);

    return FM_OK;

}   /* end fmDestroyPolicers */



