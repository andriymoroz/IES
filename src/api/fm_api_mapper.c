/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mapper.c
 * Creation Date:   July 26, 2010
 * Description:     Structures and functions for dealing with Mappers
 *
 * Copyright (c) 2010 - 2014, Intel Corporation
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
/** fmGetMapperKeyAndSize
 * \ingroup intAcl
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the computed key value and size of the mapper element.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies the type of the mapper whose entry is
 *                  to be computed.
 *
 * \param[in]       value points to the mapper value to extract. The data type
 *                  of value is dependent on mapper. See ''fm_mapper''
 *                  for a description of the data type required for each
 *                  mapper type.
 * 
 * \param[out]      key is used to return the computed key value for this
 *                  specific mapper element.
 * 
 * \param[out]      size is used to return the size of the value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetMapperKeyAndSize(fm_int     sw,
                                fm_mapper  mapper,
                                void *     value,
                                fm_uint64 *key,
                                fm_int    *size)
{
    fm_status                 err = FM_OK;
    fm_sourceMapperValue *    srcMapValue;
    fm_protocolMapperValue *  protMapValue;
    fm_l4PortMapperValue *    l4PortMapValue;
    fm_macMapperValue *       macMapValue;
    fm_ethTypeValue *         ethTypeMapValue;
    fm_ipLengthMapperValue *  ipLengthMapValue;
    fm_ipAddrMapperValue *    ipAddrMapValue;
    fm_vlanMapperValue *      vlanMapValue;
    fm_switch *               switchPtr;

    switch (mapper)
    {
        case FM_MAPPER_SOURCE:
            srcMapValue = (fm_sourceMapperValue *) value;
            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(srcMapValue->sourcePort & 0xff);

            *size = sizeof(fm_sourceMapperValue);

            break;

        case FM_MAPPER_PROTOCOL:
            protMapValue = (fm_protocolMapperValue *) value;
            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(protMapValue->protocol & 0xff);

            *size = sizeof(fm_protocolMapperValue);
            break;

        case FM_MAPPER_L4_SRC:
        case FM_MAPPER_L4_DST:
            l4PortMapValue = (fm_l4PortMapperValue *) value;

            switchPtr = GET_SWITCH_PTR(sw);
            FM_API_CALL_FAMILY(err,
                               switchPtr->GetMapperL4PortKey,
                               sw,
                               mapper,
                               l4PortMapValue,
                               key);

            *size = sizeof(fm_l4PortMapperValue);

            break;

        case FM_MAPPER_MAC:
            macMapValue = (fm_macMapperValue *) value;

            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(macMapValue->mac & 0xffffffffffff);

            *size = sizeof(fm_macMapperValue);

            break;

        case FM_MAPPER_ETH_TYPE:
            ethTypeMapValue = (fm_ethTypeValue *) value;

            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(ethTypeMapValue->ethType & 0xffff);

            *size = sizeof(fm_ethTypeValue);

            break;

        case FM_MAPPER_IP_LENGTH:
            ipLengthMapValue = (fm_ipLengthMapperValue *) value;

            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(ipLengthMapValue->ipLengthStart & 0xffff) <<
                                            FM_MAPPER_IP_LENGTH_START_KEY_POS |
                   FM_LITERAL_U64(ipLengthMapValue->ipLengthEnd & 0xffff) <<
                                            FM_MAPPER_IP_LENGTH_END_KEY_POS;

            *size = sizeof(fm_ipLengthMapperValue);

            break;

        case FM_MAPPER_IP_ADDR:
            ipAddrMapValue = (fm_ipAddrMapperValue *) value;

            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(ipAddrMapValue->ipAddr.isIPv6 & 0x1) <<
                                                 FM_MAPPER_IP_ADDR_V6_KEY_POS;
            
            if (ipAddrMapValue->ipAddr.isIPv6)
            {
                *key |= (fm_uint64)fmCrc32((fm_byte *)ipAddrMapValue->ipAddr.addr, 16);
            }
            else
            {
                *key |= ipAddrMapValue->ipAddr.addr[0] & FM_LITERAL_U64(0xffffffff);
            }

            *size = sizeof(fm_ipAddrMapperValue);

            break;

        case FM_MAPPER_VLAN:
            vlanMapValue = (fm_vlanMapperValue *) value;
            *key = FM_LITERAL_U64(mapper & 0xff) << FM_MAPPER_TYPE_KEY_POS |
                   FM_LITERAL_U64(vlanMapValue->vlanId & 0xfff);

            *size = sizeof(fm_vlanMapperValue);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;

            break;

    }

ABORT:

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetMapperKeyAndSize */


/*****************************************************************************/
/** fmAddMapperEntry
 * \ingroup mapper
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies the type of the mapper whose entry is
 *                  to be added.
 *
 * \param[in]       value points to the mapper value to add. The data type
 *                  of value is dependent on mapper. See ''fm_mapper''
 *                  for a description of the data type required for each
 *                  mapper type.
 * 
 * \param[in]       mode indicates how the added mapper entry should be 
 *                  applied to the hardware: immediately or not until the 
 *                  next time ''fmApplyACL'' is called.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ARGUMENT if the mapper is invalid.
 * \return          FM_ERR_NO_FFU_RES_FOUND if the mapper is already full.
 * \return          FM_ERR_ALREADY_EXISTS if the mapper entry already exists.
 *
 *****************************************************************************/
fm_status fmAddMapperEntry(fm_int             sw,
                           fm_mapper          mapper,
                           void *             value,
                           fm_mapperEntryMode mode)
{
    fm_status                 err;
    fm_switch *               switchPtr;
    void *                    mapEntry;
    fm_uint64                 key;
    fm_int                    size;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, mapper = %d, value = %p, mode = %d\n",
                     sw,
                     mapper,
                     value,
                     mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmGetMapperKeyAndSize(sw, mapper, value, &key, &size);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    mapEntry = fmAlloc(size);
    if (mapEntry == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMCPY_S(mapEntry, size, value, size);

    err = fmTreeInsert(&switchPtr->aclInfo.mappers, key, mapEntry);
    if (err != FM_OK)
    {
        fmFree(mapEntry);
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddMapperEntry,
                       sw,
                       mapper,
                       value,
                       mode);

    if (err != FM_OK)
    {
        fmTreeRemoveCertain(&switchPtr->aclInfo.mappers, key, NULL);
        fmFree(mapEntry);
    }

ABORT:

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmAddMapperEntry */




/*****************************************************************************/
/** fmDeleteMapperEntry
 * \ingroup mapper
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a mapper entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies the type of the mapper whose entry is
 *                  to be deleted.
 *
 * \param[in]       value points to the mapper value to delete. The data type
 *                  of value is dependent on mapper. See ''fm_mapper''
 *                  for a description of the data type required for each
 *                  mapper type.
 * 
 * \param[in]       mode indicates how the deleted mapper entry should be 
 *                  applied to the hardware: immediately or not until the 
 *                  next time ''fmApplyACL'' is called.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ARGUMENT if the mapper is invalid.
 * \return          FM_ERR_NOT_FOUND if the entry to delete is not found.
 *
 *****************************************************************************/
fm_status fmDeleteMapperEntry(fm_int             sw,
                              fm_mapper          mapper,
                              void *             value,
                              fm_mapperEntryMode mode)
{
    fm_status                 err;
    fm_switch *               switchPtr;
    void *                    mapEntry;
    fm_uint64                 key;
    fm_int                    size;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, mapper = %d, value = %p, mode = %d\n",
                     sw,
                     mapper,
                     value,
                     mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmGetMapperKeyAndSize(sw, mapper, value, &key, &size);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmTreeFind(&switchPtr->aclInfo.mappers, key, &mapEntry);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteMapperEntry,
                       sw,
                       mapper,
                       value,
                       mode);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmTreeRemoveCertain(&switchPtr->aclInfo.mappers, key, NULL);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    fmFree(mapEntry);

ABORT:

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmDeleteMapperEntry */




/*****************************************************************************/
/** fmClearMapper
 * \ingroup mapper
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Clear all the entries of a mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies the type of the mapper to clear.
 * 
 * \param[in]       mode indicates how the cleared mapper entries should be 
 *                  applied to the hardware: immediately or not until the 
 *                  next time ''fmApplyACL'' is called.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ARGUMENT if mapper is invalid.
 *
 *****************************************************************************/
fm_status fmClearMapper(fm_int             sw,
                        fm_mapper          mapper,
                        fm_mapperEntryMode mode)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, mapper = %d, mode = %d\n",
                     sw,
                     mapper,
                     mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ClearMapper,
                       sw,
                       mapper,
                       mode);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    for (fmTreeIterInit(&it, &switchPtr->aclInfo.mappers) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        if (nextKey >> FM_MAPPER_TYPE_KEY_POS == FM_LITERAL_U64(mapper & 0xff))
        {
            err = fmTreeRemoveCertain(&switchPtr->aclInfo.mappers,
                                      nextKey,
                                      NULL);
            if (err != FM_OK)
            {
                goto ABORT;
            }

            fmFree(nextValue);
            fmTreeIterInit(&it, &switchPtr->aclInfo.mappers);
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        /* this means the iteration has ended normally */
        err = FM_OK;
    }

ABORT:

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmClearMapper */




/*****************************************************************************/
/** fmGetMapper
 * \ingroup mapper
 * 
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve a copy of all the entries of a specific mapper
 *                  type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies the type of the mapper to retrieve.
 * 
 * \param[out]      nEntries points to caller-allocated storage where this
 *                  function is to store the number of entries retrieved.
 * 
 * \param[out]      entries points to an array of the proper mapper type
 *                  structure that will be filled in by this function with
 *                  mapper entries. The array must be large enough to hold
 *                  maxEntries number of mapper entries. The data
 *                  type of this array is dependent on mapper. See ''fm_mapper''
 *                  for a description of the data type required for each
 *                  mapper type.
 *
 * \param[in]       maxEntries is the size of entries, being the maximum
 *                  number of mapper entries that the entries array can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if mapper is invalid.
 * \return          FM_ERR_BUFFER_FULL if the entries array is full.
 *
 *****************************************************************************/
fm_status fmGetMapper(fm_int      sw,
                      fm_mapper   mapper,
                      fm_int *    nEntries,
                      void *      entries,
                      fm_int      maxEntries)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_byte *       entriesPos = entries;
    fm_uint64       key;
    fm_int          size;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, mapper = %d, nEntries = %p, entries= %p, "
                     "maxEntries = %d\n",
                     sw,
                     mapper,
                     (void *) nEntries,
                     entries,
                     maxEntries);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    if ((nEntries == NULL) || (entries == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    *nEntries = 0;

    for (fmTreeIterInit(&it, &switchPtr->aclInfo.mappers) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        if (nextKey >> FM_MAPPER_TYPE_KEY_POS == FM_LITERAL_U64(mapper & 0xff))
        {
            if (*nEntries >= maxEntries)
            {
                err = FM_ERR_BUFFER_FULL;

                goto ABORT;
            }

            err = fmGetMapperKeyAndSize(sw, mapper, nextValue, &key, &size);
            if (err != FM_OK)
            {
                goto ABORT;
            }

            FM_MEMCPY_S(entriesPos, size, nextValue, size);
            entriesPos += size;

            (*nEntries)++;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        /* this means the iteration has ended normally */
        err = FM_OK;
    }

ABORT:

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetMapper */




/*****************************************************************************/
/** fmGetMapperSize
 * \ingroup mapper
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the size of a mapper, being the number of entries
 *                  that the mapper can support.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies the type of the mapper whose size to
 *                  get.
 *
 * \param[in]       mapperSize points to caller-allocated storage where this
 *                  function should place the mapper size.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ARGUMENT if the mapper is invalid.
 *
 *****************************************************************************/
fm_status fmGetMapperSize(fm_int     sw,
                          fm_mapper  mapper,
                          fm_uint32 *mapperSize)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, mapper = %d, mapperSize = %p\n",
                     sw,
                     mapper,
                     (void *)mapperSize);

    if (mapperSize == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetMapperSize,
                       sw,
                       mapper,
                       mapperSize);

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetMapperSize */


