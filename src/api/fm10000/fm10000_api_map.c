/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_map.c
 * Creation Date:  April 18, 2013
 * Description:    Low-level API for manipulating Mappers
 *
 * Copyright (c) 2013, Intel Corporation
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM_API_REQUIRE(expr, failCode)       \
    if ( !(expr) )                           \
    {                                        \
        err = failCode;                      \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP,  \
                            failCode);       \
    }

 
/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

 
/*****************************************************************************
 * Global Variables
 *****************************************************************************/

 
/*****************************************************************************
 * Local Variables
 *****************************************************************************/

 
/*****************************************************************************
 * Local Functions
 *****************************************************************************/
 
/*****************************************************************************/
/** fmSupportsMapper
 * \ingroup intLowlevMapper10k
 *
 * \desc            Determines whether the specified switch has a Mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          TRUE if the switch has a Mapper.
 * \return          FALSE if the switch does not have a Mapper.
 *
 *****************************************************************************/
static inline fm_bool fmSupportsMapper(fm_int sw)
{
    return ( (fmRootApi->fmSwitchStateTable[sw]->switchFamily ==
              FM_SWITCH_FAMILY_FM10000) );

}   /* end fmSupportsMapper */

 
/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000SetMapSourcePort
 * \ingroup lowlevMap10K 
 *
 * \desc            Configure the source port mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the physical port whose mapper will be set.
 *
 * \param[in]       mapSrcCfg points to a structure of type
 *                  ''fm_fm10000MapSrcPortCfg'' containing the mapper 
 *                  configuration to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapSrcCfg should be written to the mapper. See
 *                  ''Source Port Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapSrcCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapSourcePort(fm_int                   sw,
                                  fm_int                   port,
                                  fm_fm10000MapSrcPortCfg *mapSrcCfg,
                                  fm_uint32                fieldSelectMask,
                                  fm_bool                  useCache)
{                                                                 
    fm_uint32 mapSrcPortRegister[FM10000_FFU_MAP_SRC_WIDTH] = {0};
    fm_status err = FM_OK;
    fm_bool   regLockTaken = FALSE;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "port         = %d, "
                  "fieldSelect  = %u, "
                  "useCache     = %s\n",
                  sw,
                  port,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }
                                                     
    /* sanity check on the arguments */
    FM_API_REQUIRE(mapSrcCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(port < FM10000_FFU_MAP_SRC_ENTRIES, FM_ERR_INVALID_PORT );

    /* check on Routable Flag */
    if (fieldSelectMask & FM_FM10000_MAP_SRC_ROUTABLE)
    {
        FM_API_REQUIRE(mapSrcCfg->routable <= 1, FM_ERR_INVALID_ARGUMENT);
    }
    if (fieldSelectMask & FM_FM10000_MAP_SRC_ID)
    {
        FM_API_REQUIRE(mapSrcCfg->mapSrc <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_SRC, MAP_SRC),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value  from the 
     * FM10000_FFU_MAP_SRC register to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM_FM10000_MAP_SRC_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_SRC register */
        err = fmRegCacheReadSingle1D(sw, 
                                     &fm10000CacheFfuMapSrc,
                                     mapSrcPortRegister,
                                     port,
                                     useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    /* do we need to update the Port ID? */
    if (fieldSelectMask & FM_FM10000_MAP_SRC_ID)
    {
        /* yes */
        FM_ARRAY_SET_FIELD(mapSrcPortRegister,
                           FM10000_FFU_MAP_SRC,
                           MAP_SRC,
                           mapSrcCfg->mapSrc);
    }

    /* do we need to update the Routable Flag? */
    if (fieldSelectMask & FM_FM10000_MAP_SRC_ROUTABLE)
    {
        /* yes */
        FM_ARRAY_SET_BIT(mapSrcPortRegister,
                         FM10000_FFU_MAP_SRC,
                         Routable,
                         mapSrcCfg->routable);
    }

    /* write to the register */
    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuMapSrc,
                                  mapSrcPortRegister,
                                  port,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000SetMapSourcePort */




/*****************************************************************************/
/** fm10000GetMapSourcePort
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the source port mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the physical port whose mapper will be retrieved.
 *
 * \param[out]      mapSrcCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapSrcPortCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapSourcePort(fm_int                   sw,
                                  fm_int                   port,
                                  fm_fm10000MapSrcPortCfg *mapSrcCfg,
                                  fm_bool                  useCache)
{
    fm_uint32 mapSrcPortRegister[FM10000_FFU_MAP_SRC_WIDTH];
    fm_status err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "port         = %d, "
                  "useCache     = %s\n",
                  sw,
                  port,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapSrcCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(port < FM10000_FFU_MAP_SRC_ENTRIES, FM_ERR_INVALID_PORT);

    /* read from the register */
    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuMapSrc,
                                 mapSrcPortRegister,
                                 port,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


    /* extract the Source Port ID */
    mapSrcCfg->mapSrc = (fm_byte) FM_ARRAY_GET_FIELD(mapSrcPortRegister,
                                                     FM10000_FFU_MAP_SRC,
                                                     MAP_SRC);

    /* extract all Source Routable Flags */
    mapSrcCfg->routable = (fm_byte) FM_ARRAY_GET_BIT(mapSrcPortRegister, 
                                                     FM10000_FFU_MAP_SRC,
                                                     Routable);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000GetMapSourcePort */




/*****************************************************************************/
/** fm10000SetMapMac
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the MAC mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  zero to FM_FM10000_MAX_MAC_MAPPER.
 *
 * \param[in]       mapMacCfg points to a structure of type
 *                  ''fm_fm10000MapMacCfg'' containing the mapper configuration
 *                  to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapMacCfg should be written to the mapper. See
 *                  ''MAC Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapMacCfg is
 *                  invalid.
 * 
 *
 *****************************************************************************/
fm_status fm10000SetMapMac(fm_int               sw,
                           fm_int               index,
                           fm_fm10000MapMacCfg *mapMacCfg,
                           fm_uint32            fieldSelectMask,
                           fm_bool              useCache)
{
    fm_uint32 mapMacRegister[FM10000_FFU_MAP_MAC_WIDTH];
    fm_status err = FM_OK;
    fm_bool   regLockTaken = FALSE;
                                  
    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw          = %d, "
                  "index       = %d, "
                  "fieldSelect = %u, "
                  "useCache    = %s\n",
                  sw,
                  index,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapMacCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_MAC_ENTRIES, FM_ERR_INVALID_SLOT);

    /* check the MAC ID for correctness */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_ID)
    {
        FM_API_REQUIRE(mapMacCfg->mapMac <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_MAC, MAP_MAC),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /* check the router flag correctness */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_ROUTER)
    {
        FM_API_REQUIRE(mapMacCfg->router <= 1, FM_ERR_INVALID_ARGUMENT);
    }

    /* check the validSMAC flag correctness */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_VALID_SMAC)
    {
        FM_API_REQUIRE(mapMacCfg->validSMAC <= 1, FM_ERR_INVALID_ARGUMENT);
    }

    /* check the validDMAC flag correctness */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_VALID_DMAC)
    {
        FM_API_REQUIRE(mapMacCfg->validDMAC <= 1, FM_ERR_INVALID_ARGUMENT);
    }

    /* Check MAC address for correctness */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_ADDR)
    {
        FM_API_REQUIRE(mapMacCfg->macAddr <= FM_FIELD_UNSIGNED_MAX64(FM10000_FFU_MAP_MAC, MAC),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /* check the MAC ignore length for correctness */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_IGNORE)
    {
        FM_API_REQUIRE(mapMacCfg->ignoreLength <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_MAC, IgnoreLength),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value  from the 
     * FM10000_FFU_MAP_MAC register to start 
     * with 
     **************************************************/
    FM_CLEAR(mapMacRegister);
    if (fieldSelectMask != FM_FM10000_MAP_MAC_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_MAC register */
        err = fmRegCacheReadSingle1D(sw, 
                                     &fm10000CacheFfuMapMac,
                                     mapMacRegister,
                                     index,
                                     useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    /* do we need to update the MAC Address? */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_ADDR)
    {
        /* yes */
        FM_ARRAY_SET_FIELD64(mapMacRegister,
                             FM10000_FFU_MAP_MAC,
                             MAC,
                             mapMacCfg->macAddr);
    }

    /* do we need to update the ignore length? */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_IGNORE)
    {
        /* yes */
        FM_ARRAY_SET_FIELD(mapMacRegister,
                           FM10000_FFU_MAP_MAC,
                           IgnoreLength,
                           mapMacCfg->ignoreLength);
    }

    /* do we need to update the validSMAC flag? */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_VALID_SMAC)
    {
        /* yes */
        FM_ARRAY_SET_BIT(mapMacRegister,
                         FM10000_FFU_MAP_MAC,
                         validSMAC,
                         mapMacCfg->validSMAC);
    }

    /* do we need to update the validDMAC flag? */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_VALID_DMAC)
    {
        /* yes */
        FM_ARRAY_SET_BIT(mapMacRegister,
                         FM10000_FFU_MAP_MAC,
                         validDMAC,
                         mapMacCfg->validDMAC);
    }

    /* do we need to update the routable flag? */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_ROUTER)
    {
        /* yes */
        FM_ARRAY_SET_BIT(mapMacRegister,
                         FM10000_FFU_MAP_MAC,
                         Router,
                         mapMacCfg->router);
    }

    /* do we need to update the Mac ID? */
    if (fieldSelectMask & FM_FM10000_MAP_MAC_ID)
    {
        /* yes */
        FM_ARRAY_SET_FIELD(mapMacRegister,
                           FM10000_FFU_MAP_MAC,
                           MAP_MAC,
                           mapMacCfg->mapMac);
    }

    /* write to the register */
    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuMapMac,
                                  mapMacRegister,
                                  index,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000SetMapMac */



/*****************************************************************************/
/** fm10000GetMapMac
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the MAC mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  zero to FM_FM10000_MAX_MAC_MAPPER.
 *
 * \param[out]      mapMacCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapMacCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapMac(fm_int               sw,
                           fm_int               index,
                           fm_fm10000MapMacCfg *mapMacCfg,
                           fm_bool              useCache)
{
    fm_uint32 mapMacRegister[FM10000_FFU_MAP_MAC_WIDTH];
    fm_status err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw          = %d, "
                  "index       = %d, "
                  "useCache    = %s\n",
                  sw,
                  index,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapMacCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_MAC_ENTRIES, FM_ERR_INVALID_SLOT);

    /* read from the register */
    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuMapMac,
                                 mapMacRegister,
                                 index,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


    /* extract the all the fields */
    mapMacCfg->macAddr = (fm_uint64) FM_ARRAY_GET_FIELD64(mapMacRegister,
                                                          FM10000_FFU_MAP_MAC,
                                                          MAC);

    mapMacCfg->ignoreLength = (fm_byte) FM_ARRAY_GET_FIELD(mapMacRegister,
                                                           FM10000_FFU_MAP_MAC,
                                                           IgnoreLength);

    mapMacCfg->validSMAC = (fm_byte) FM_ARRAY_GET_BIT(mapMacRegister,
                                                      FM10000_FFU_MAP_MAC,
                                                      validSMAC);

    mapMacCfg->validDMAC = (fm_byte) FM_ARRAY_GET_BIT(mapMacRegister,
                                                      FM10000_FFU_MAP_MAC,
                                                      validDMAC);

    mapMacCfg->router = (fm_byte) FM_ARRAY_GET_BIT(mapMacRegister,
                                                   FM10000_FFU_MAP_MAC,
                                                   Router);

    mapMacCfg->mapMac = (fm_byte) FM_ARRAY_GET_FIELD(mapMacRegister,
                                                     FM10000_FFU_MAP_MAC,
                                                     MAP_MAC);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000GetMapMac */



/*****************************************************************************/
/** fm10000SetMapEtherType
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the EtherType mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  0 to FM_FM10000_MAX_ETYPE_MAPPER.
 *
 * \param[in]       mapETypeCfg points to a structure of type
 *                  ''fm_fm10000MapETypeCfg'' containing the mapper 
 *                  configuration to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapETypeCfg should be written to the mapper. See
 *                  ''EtherType Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapETypeCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapEtherType(fm_int                 sw,
                                 fm_int                 index,
                                 fm_fm10000MapETypeCfg *mapETypeCfg,
                                 fm_uint32              fieldSelectMask,
                                 fm_bool                useCache)
{
    fm_uint32 mapEthTypeRegister[FM10000_FFU_MAP_TYPE_WIDTH] = {0};
    fm_status err = FM_OK;
    fm_bool   regLockTaken = FALSE;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "index        = %d, "
                  "fieldSelect  = %u, "
                  "useCache     = %s\n",
                  sw,
                  index,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }
                                                     
    /* sanity check on the arguments */
    FM_API_REQUIRE(mapETypeCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_TYPE_ENTRIES, FM_ERR_INVALID_SLOT);

    if (fieldSelectMask & FM_FM10000_MAP_ETYPE_ID)
    {
        FM_API_REQUIRE(mapETypeCfg->mapType <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_TYPE, MAP_TYPE),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value  from the 
     * FM10000_FFU_MAP_TYPE register to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM_FM10000_MAP_ETYPE_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_TYPE register */
        err = fmRegCacheReadSingle1D(sw, 
                                     &fm10000CacheFfuMapType,
                                     mapEthTypeRegister,
                                     index,
                                     useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    if (fieldSelectMask & FM_FM10000_MAP_ETYPE_ID)
    {
        FM_ARRAY_SET_FIELD(mapEthTypeRegister,
                           FM10000_FFU_MAP_TYPE,
                           MAP_TYPE,
                           mapETypeCfg->mapType);
    }

    if (fieldSelectMask & FM_FM10000_MAP_ETYPE_TYPE)
    {
        FM_ARRAY_SET_FIELD(mapEthTypeRegister,
                           FM10000_FFU_MAP_TYPE,
                           TYPE_XXX,
                           mapETypeCfg->ethType);
    }


    /* write to the register */
    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuMapType,
                                  mapEthTypeRegister,
                                  index,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000SetMapEtherType */



/*****************************************************************************/
/** fm10000GetMapEtherType
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the EtherType mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to get. Ranges from
 *                  0 to FM_FM10000_MAX_ETYPE_MAPPER.
 *
 * \param[out]      mapETypeCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapETypeCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapEtherType(fm_int                 sw,
                                 fm_int                 index,
                                 fm_fm10000MapETypeCfg *mapETypeCfg,
                                 fm_bool                useCache)
{
    fm_uint32 mapEthTypeRegister[FM10000_FFU_MAP_TYPE_WIDTH];
    fm_status err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "index        = %d, "
                  "useCache     = %s\n",
                  sw,
                  index,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapETypeCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_TYPE_ENTRIES, FM_ERR_INVALID_SLOT);

    /* read from the register */
    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuMapType,
                                 mapEthTypeRegister,
                                 index,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


    mapETypeCfg->mapType = (fm_byte) FM_ARRAY_GET_FIELD(mapEthTypeRegister,
                                                        FM10000_FFU_MAP_TYPE,
                                                        MAP_TYPE);

    mapETypeCfg->ethType = (fm_uint16) FM_ARRAY_GET_FIELD(mapEthTypeRegister,
                                                          FM10000_FFU_MAP_TYPE,
                                                          TYPE_XXX);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000GetMapEtherType */



/*****************************************************************************/
/** fm10000SetMapLength
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the L3 length mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  0 to FM_FM10000_MAX_LENGTH_MAPPER.
 *
 * \param[in]       mapLenCfg points to a structure of type
 *                  ''fm_fm10000MapLenCfg'' containing the mapper 
 *                  configuration to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapLenCfg should be written to the mapper. See
 *                  ''L3 Length Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapLenCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapLength(fm_int               sw,
                              fm_int               index,
                              fm_fm10000MapLenCfg *mapLenCfg,
                              fm_uint32            fieldSelectMask,
                              fm_bool              useCache)
{
    fm_uint32 mapLengthRegister[FM10000_FFU_MAP_LENGTH_WIDTH] = {0};
    fm_status err = FM_OK;
    fm_bool   regLockTaken = FALSE;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "index        = %d, "
                  "fieldSelect  = %u, "
                  "useCache     = %s\n",
                  sw,
                  index,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapLenCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_LENGTH_ENTRIES, FM_ERR_INVALID_SLOT);

    if (fieldSelectMask & FM_FM10000_MAP_LEN_BIN)
    {
        FM_API_REQUIRE(mapLenCfg->mapLength <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_LENGTH, MAP_LENGTH),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value  from the 
     * FM10000_FFU_MAP_LENGTH register to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM_FM10000_MAP_LEN_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_LENGTH register */
        err = fmRegCacheReadSingle1D(sw, 
                                     &fm10000CacheFfuMapLength,
                                     mapLengthRegister,
                                     index,
                                     useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    if (fieldSelectMask & FM_FM10000_MAP_LEN_BIN)
    {
        FM_ARRAY_SET_FIELD(mapLengthRegister,
                           FM10000_FFU_MAP_LENGTH,
                           MAP_LENGTH,
                           mapLenCfg->mapLength);
    }

    if (fieldSelectMask & FM_FM10000_MAP_LEN_LBOUND)
    {
        FM_ARRAY_SET_FIELD(mapLengthRegister,
                           FM10000_FFU_MAP_LENGTH,
                           LENGTH,
                           mapLenCfg->length);
    }


    /* write to the register */
    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuMapLength,
                                  mapLengthRegister,
                                  index,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000SetMapLength */



/*****************************************************************************/
/** fm10000GetMapLength
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the L3 length mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to get. Ranges from
 *                  0 to FM_FM10000_MAX_LENGTH_MAPPER.
 *
 * \param[out]      mapLenCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapLenCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapLength(fm_int               sw,
                              fm_int               index,
                              fm_fm10000MapLenCfg *mapLenCfg,
                              fm_bool              useCache)
{
    fm_uint32 mapLengthRegister[FM10000_FFU_MAP_LENGTH_WIDTH];
    fm_status err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "index        = %d, "
                  "useCache     = %s\n",
                  sw,
                  index,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapLenCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_LENGTH_ENTRIES, FM_ERR_INVALID_SLOT);

    /* read from the register */
    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuMapLength,
                                 mapLengthRegister,
                                 index,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


    mapLenCfg->mapLength = (fm_byte) FM_ARRAY_GET_FIELD(mapLengthRegister,
                                                        FM10000_FFU_MAP_LENGTH,
                                                        MAP_LENGTH);

    mapLenCfg->length = (fm_uint16) FM_ARRAY_GET_FIELD(mapLengthRegister,
                                                       FM10000_FFU_MAP_LENGTH,
                                                       LENGTH);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000GetMapLength */



/*****************************************************************************/
/** fm10000SetMapIp
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the IP mappers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  zero to FM_FM10000_MAX_IP_MAPPER.
 *
 * \param[in]       mapIpCfg points to a structure of type
 *                  ''fm_fm10000MapIpCfg'' containing the mapper configuration
 *                  to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapIpCfg should be written to the mapper. See
 *                  ''IP Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       live indicates whether the traffic is currently running.
 *                  Since an IP mapper entry exists in multiple registers
 *                  that cannot be updated atomically, overwriting one
 *                  valid mapper entry with another while traffic is running
 *                  could result in an undesirable "hybrid" state existing
 *                  temporarily.  To avoid this, this function can set
 *                  the old entry to invalid before writing the new entry.
 *                  (However, this will still result in the entry
 *                  temporarily "blinking out.")  If traffic is not
 *                  currently running, set live to FALSE to avoid this
 *                  special handling.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapIpCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapIp(fm_int              sw,
                          fm_int              index,
                          fm_fm10000MapIpCfg *mapIpCfg,
                          fm_uint32           fieldSelectMask,
                          fm_bool             live,
                          fm_bool             useCache)
{
    fm_registerSGListEntry sgList[4];
    fm_uint32              ipWords[FM10000_FFU_MAP_IP_LO_WIDTH +
                                   FM10000_FFU_MAP_IP_HI_WIDTH];
    fm_uint32              mapIpCfgRegister[FM10000_FFU_MAP_IP_CFG_WIDTH] = {0};
    fm_uint32              zero[FM10000_FFU_MAP_IP_CFG_WIDTH] = {0};
    fm_int                 i;
    fm_int                 j;
    fm_bool                reWrite = FALSE;
    fm_status              err = FM_OK;
    fm_bool                regLockTaken = FALSE;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw          = %d, "
                  "index       = %d, "
                  "fieldSelect = %u, "
                  "live        = %s, "
                  "useCache    = %s\n",
                  sw,
                  index,
                  fieldSelectMask,
                  FM_BOOLSTRING(live),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapIpCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_IP_CFG_ENTRIES, FM_ERR_INVALID_SLOT);

    if (fieldSelectMask & FM_FM10000_MAP_IP_VALID_SIP)
    {
        FM_API_REQUIRE(mapIpCfg->validSIP <= 1 , FM_ERR_INVALID_ARGUMENT);
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_VALID_DIP)
    {
        FM_API_REQUIRE(mapIpCfg->validDIP <= 1 , FM_ERR_INVALID_ARGUMENT);
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_ID)
    {
        FM_API_REQUIRE(mapIpCfg->mapIp <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_IP_CFG, MAP_IP),
                       FM_ERR_INVALID_ARGUMENT);
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_IGNORE)
    {
        FM_API_REQUIRE(mapIpCfg->ignoreLength <= 128, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(sgList);
    FM_CLEAR(ipWords);

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM_FM10000_MAP_IP_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                                  &fm10000CacheFfuMapIpCfg,
                                  1,
                                  index,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  mapIpCfgRegister,
                                  FALSE);

        /* read the FFU_MAP_IP_CFG, FFU_MAP_IP_LO and FFU_MAP_IP_HI registers */
        err = fmRegCacheRead(sw, 1, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }


    i = 0;
    /* invalidate the entry if we are running live */
    if (live == TRUE && (fieldSelectMask & (FM_FM10000_MAP_IP_ADDR)) != 0) 
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                                  &fm10000CacheFfuMapIpCfg,
                                  1,
                                  index,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  zero,
                                  FALSE);
        i++;
        reWrite = TRUE;
    }

    /* do we need to update the IP address? */
    if (fieldSelectMask & FM_FM10000_MAP_IP_ADDR)
    {
        for (j = 0 ; j < (mapIpCfg->ipAddr.isIPv6 ? 4 : 1) ; j++)
        {
            ipWords[j] = ntohl(mapIpCfg->ipAddr.addr[j]);
        }

        FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                                  &fm10000CacheFfuMapIpLo,
                                  1,
                                  index,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  ipWords,
                                  FALSE);

        FM_REGS_CACHE_FILL_SGLIST(&sgList[i + 1],
                                  &fm10000CacheFfuMapIpHi,
                                  1,
                                  index,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  ipWords + FM10000_FFU_MAP_IP_LO_WIDTH,
                                  FALSE);
        i += 2;
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_ID)
    {
        FM_ARRAY_SET_FIELD(mapIpCfgRegister,
                           FM10000_FFU_MAP_IP_CFG,
                           MAP_IP,
                           mapIpCfg->mapIp);
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_IGNORE)
    {
        FM_ARRAY_SET_FIELD(mapIpCfgRegister,
                           FM10000_FFU_MAP_IP_CFG,
                           IgnoreLength,
                           mapIpCfg->ignoreLength);
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_VALID_SIP)
    {
        FM_ARRAY_SET_BIT(mapIpCfgRegister,
                         FM10000_FFU_MAP_IP_CFG,
                         validSIP,
                         mapIpCfg->validSIP);
    }

    if (fieldSelectMask & FM_FM10000_MAP_IP_VALID_DIP)
    {
        FM_ARRAY_SET_BIT(mapIpCfgRegister,
                         FM10000_FFU_MAP_IP_CFG,
                         validDIP,
                         mapIpCfg->validDIP);
    }

    FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                              &fm10000CacheFfuMapIpCfg,
                              1,
                              index,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              mapIpCfgRegister,
                              reWrite);
    i++;

    err = fmRegCacheWrite(sw, i, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000SetMapIp */



/*****************************************************************************/
/** fm10000GetMapIp
 * \ingroup lowlevMap10K
 *
 * \desc            Get the IP Mapper configuration.
 * 
 * \note            The value of isIPv6 returned in the ipAddr and 
 *                  ipMask fields of mapIpCfg is to be 
 *                  considered undetermined. It is up to the calling 
 *                  application to keep track of whether a given entry of any
 *                  of the involved registers is configured for IPv6 or IPv4.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to get. Ranges from
 *                  zero to FM_FM10000_MAX_IP_MAPPER.
 *
 * \param[out]      mapIpCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapIpCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapIp(fm_int              sw,
                          fm_int              index,
                          fm_fm10000MapIpCfg *mapIpCfg,
                          fm_bool             useCache)
{
    fm_registerSGListEntry sgList[3];
    fm_uint32              ipWords[FM10000_FFU_MAP_IP_LO_WIDTH +
                                   FM10000_FFU_MAP_IP_HI_WIDTH];
    fm_uint32              mapIpCfgRegister[FM10000_FFU_MAP_IP_CFG_WIDTH];
    fm_int                 i;
    fm_status              err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw          = %d, "
                  "index       = %d, "
                  "useCache    = %s\n",
                  sw,
                  index,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapIpCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_IP_CFG_ENTRIES, FM_ERR_INVALID_SLOT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheFfuMapIpCfg,
                              1,
                              index,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              mapIpCfgRegister,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheFfuMapIpLo,
                              1,
                              index,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              ipWords,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[2],
                              &fm10000CacheFfuMapIpHi,
                              1,
                              index,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              ipWords + FM10000_FFU_MAP_IP_LO_WIDTH,
                              FALSE);

    /* read the FFU_MAP_IP_CFG, FFU_MAP_IP_LO and FFU_MAP_IP_HI registers */
    err = fmRegCacheRead(sw, 3, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    /**************************************************
     * Assume it's an IPv4 address if words 1..3 of 
     * the value are zeros 
     **************************************************/
    mapIpCfg->ipAddr.isIPv6 = FALSE;
    mapIpCfg->ipAddr.addr[0] = htonl(ipWords[0]);

    for (i = 1 ; i < 4 ; i++)
    {
        mapIpCfg->ipAddr.addr[i] = htonl(ipWords[i]);

        if (mapIpCfg->ipAddr.addr[i] != 0)
        {
            mapIpCfg->ipAddr.isIPv6 = TRUE;
        }
    }

    mapIpCfg->ignoreLength = (fm_byte) FM_ARRAY_GET_FIELD(mapIpCfgRegister,
                                                          FM10000_FFU_MAP_IP_CFG,
                                                          IgnoreLength);

    mapIpCfg->mapIp = (fm_byte) FM_ARRAY_GET_FIELD(mapIpCfgRegister,
                                                   FM10000_FFU_MAP_IP_CFG,
                                                   MAP_IP);

    mapIpCfg->validSIP = (fm_byte) FM_ARRAY_GET_BIT(mapIpCfgRegister,
                                                    FM10000_FFU_MAP_IP_CFG,
                                                    validSIP);

    mapIpCfg->validDIP = (fm_byte) FM_ARRAY_GET_BIT(mapIpCfgRegister,
                                                    FM10000_FFU_MAP_IP_CFG,
                                                    validDIP);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000GetMapIp */



/*****************************************************************************/
/** fm10000SetMapProt
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the protocol mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  0 to FM_FM10000_MAX_PROT_MAPPER.
 *
 * \param[in]       mapProtCfg points to a structure of type
 *                  ''fm_fm10000MapProtCfg'' containing the mapper 
 *                  configuration to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapProtCfg should be written to the mapper. See
 *                  ''Protocol Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapProtCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapProt(fm_int                sw,
                            fm_int                index,
                            fm_fm10000MapProtCfg *mapProtCfg,
                            fm_uint32             fieldSelectMask,
                            fm_bool               useCache)
{
    fm_uint32 mapProtRegister[FM10000_FFU_MAP_PROT_WIDTH] = {0};
    fm_status err = FM_OK;
    fm_bool   regLockTaken = FALSE;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "index        = %d, "
                  "fieldSelect  = %u, "
                  "useCache     = %s\n",
                  sw,
                  index,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }
                                                     
    /* sanity check on the arguments */
    FM_API_REQUIRE(mapProtCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_PROT_ENTRIES, FM_ERR_INVALID_SLOT);

    if (fieldSelectMask & FM_FM10000_MAP_PROT_ID)
    {
        FM_API_REQUIRE(mapProtCfg->mapProt <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_PROT, MAP_PROT),
                       FM_ERR_INVALID_ARGUMENT);
    }

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value  from the 
     * FM10000_FFU_MAP_PROT register to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM_FM10000_MAP_PROT_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_PROT register */
        err = fmRegCacheReadSingle1D(sw, 
                                     &fm10000CacheFfuMapProt,
                                     mapProtRegister,
                                     index,
                                     useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    if (fieldSelectMask & FM_FM10000_MAP_PROT_ID)
    {
        FM_ARRAY_SET_FIELD(mapProtRegister,
                           FM10000_FFU_MAP_PROT,
                           MAP_PROT,
                           mapProtCfg->mapProt);
    }

    if (fieldSelectMask & FM_FM10000_MAP_PROT_TYPE)
    {
        FM_ARRAY_SET_FIELD(mapProtRegister,
                           FM10000_FFU_MAP_PROT,
                           PROT,
                           mapProtCfg->protocol);
    }

    /* write to the register */
    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuMapProt,
                                  mapProtRegister,
                                  index,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000SetMapProt */



/*****************************************************************************/
/** fm10000GetMapProt
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the protocol mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index indicates which mapper entry to get. Ranges from
 *                  0 to FM_FM10000_MAX_PROT_MAPPER.
 *
 * \param[out]      mapProtCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapProtCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapProt(fm_int                sw,
                            fm_int                index,
                            fm_fm10000MapProtCfg *mapProtCfg,
                            fm_bool               useCache)
{
    fm_uint32 mapProtRegister[FM10000_FFU_MAP_PROT_WIDTH];
    fm_status err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "index        = %d, "
                  "useCache     = %s\n",
                  sw,
                  index,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapProtCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_PROT_ENTRIES, FM_ERR_INVALID_SLOT);

    /* read from the register */
    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuMapProt,
                                 mapProtRegister,
                                 index,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    mapProtCfg->mapProt = (fm_byte) FM_ARRAY_GET_FIELD(mapProtRegister,
                                                       FM10000_FFU_MAP_PROT,
                                                       MAP_PROT);

    mapProtCfg->protocol = (fm_byte) FM_ARRAY_GET_FIELD(mapProtRegister,
                                                        FM10000_FFU_MAP_PROT,
                                                        PROT);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000GetMapProt */



/*****************************************************************************/
/** fm10000SetMapL4Port
 * \ingroup lowlevMap10K
 *
 * \desc            Configure either of the two L4 port mappers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies which mapper to set (see 
 *                  ''fm_fm10000MapL4Port'').
 *
 * \param[in]       index indicates which mapper entry to set. Ranges from
 *                  0 to FM_FM10000_MAX_L4PORT_MAPPER.
 *
 * \param[in]       mapL4PortCfg points to a structure of type
 *                  ''fm_fm10000MapL4PortCfg'' containing the mapper 
 *                  configuration to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapL4PortCfg should be written to the mapper. See
 *                  ''L4 Port Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_MAPPER if mapper is invalid.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapL4PortCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapL4Port(fm_int                  sw,
                              fm_fm10000MapL4Port     mapper,
                              fm_int                  index,
                              fm_fm10000MapL4PortCfg *mapL4PortCfg,
                              fm_uint32               fieldSelectMask,
                              fm_bool                 useCache)
{
    fm_uint32      mapL4PortRegister[FM10000_FFU_MAP_L4_SRC_WIDTH];
    fm_cachedRegs *regDesc;
    fm_status      err = FM_OK;
    fm_bool        regLockTaken = FALSE;
                                  
    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw          = %d, "
                  "mapper      = %d, "
                  "index       = %d, "
                  "fieldSelect = %u, "
                  "useCache    = %s\n",
                  sw,
                  mapper,
                  index,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapL4PortCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_L4_SRC_ENTRIES, FM_ERR_INVALID_SLOT);
    FM_API_REQUIRE(mapper < FM_FM10000_MAP_L4PORT_MAX, FM_ERR_INVALID_MAPPER);

    if (fieldSelectMask & FM_FM10000_MAP_L4PORT_MPROT)
    {
        FM_API_REQUIRE(mapL4PortCfg->mapProt <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_L4_SRC, MAP_PROT),
                       FM_ERR_INVALID_ARGUMENT);
    }

    if (fieldSelectMask & FM_FM10000_MAP_L4PORT_VALID)
    {
        FM_API_REQUIRE(mapL4PortCfg->valid <= 1, FM_ERR_INVALID_ARGUMENT);
    }

    /* select whether to use DST vs SRC*/
    if (mapper == FM_FM10000_MAP_L4PORT_DST)
    {
        /* DST */
        regDesc = (fm_cachedRegs *)&fm10000CacheFfuMapL4Dst;
    }
    else
    {
        /* SRC */
        regDesc = (fm_cachedRegs *)&fm10000CacheFfuMapL4Src;
    }

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value  from the 
     * FM10000_FFU_MAP_L4_XXX register to start 
     * with 
     **************************************************/
    FM_CLEAR(mapL4PortRegister);
    if (fieldSelectMask != FM_FM10000_MAP_L4PORT_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_L4_XXX register */
        err = fmRegCacheReadSingle1D(sw, 
                                     regDesc,
                                     mapL4PortRegister,
                                     index,
                                     useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    if (fieldSelectMask & FM_FM10000_MAP_L4PORT_LBOUND)
    {
        FM_ARRAY_SET_FIELD(mapL4PortRegister,
                           FM10000_FFU_MAP_L4_SRC,
                           L4SRC,
                           mapL4PortCfg->lowerBound);
    }

    if (fieldSelectMask & FM_FM10000_MAP_L4PORT_MPROT)
    {
        FM_ARRAY_SET_FIELD(mapL4PortRegister,
                           FM10000_FFU_MAP_L4_SRC,
                           MAP_PROT,
                           mapL4PortCfg->mapProt);
    }

    if (fieldSelectMask & FM_FM10000_MAP_L4PORT_VALID)
    {
        FM_ARRAY_SET_BIT(mapL4PortRegister,
                         FM10000_FFU_MAP_L4_SRC,
                         VALID,
                         mapL4PortCfg->valid);
    }

    if (fieldSelectMask & FM_FM10000_MAP_L4PORT_ID)
    {
        FM_ARRAY_SET_FIELD(mapL4PortRegister,
                           FM10000_FFU_MAP_L4_SRC,
                           MAP_L4SRC,
                           mapL4PortCfg->mapL4Port);
    }

    /* write to the register */
    err = fmRegCacheWriteSingle1D(sw, 
                                  regDesc,
                                  mapL4PortRegister,
                                  index,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000SetMapL4Port */



/*****************************************************************************/
/** fm10000GetMapL4Port
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuratino of either of the two L4 port mappers. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper identifies which mapper to get (see 
 *                  ''fm_fm10000MapL4Port'').
 *
 * \param[in]       index indicates which mapper entry to get. Ranges from
 *                  0 to FM_FM10000_MAX_L4PORT_MAPPER.
 *
 * \param[out]      mapL4PortCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapL4PortCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_MAPPER if mapper is invalid.
 * \return          FM_ERR_INVALID_SLOT if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapL4PortCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapL4Port(fm_int                  sw,
                              fm_fm10000MapL4Port     mapper,
                              fm_int                  index,
                              fm_fm10000MapL4PortCfg *mapL4PortCfg,
                              fm_bool                 useCache)
{
    fm_uint32      mapL4PortRegister[FM10000_FFU_MAP_L4_SRC_WIDTH];
    fm_cachedRegs *regDesc;
    fm_status      err = FM_OK;
                                  
    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw          = %d, "
                  "mapper      = %d, "
                  "index       = %d, "
                  "useCache    = %s\n",
                  sw,
                  mapper,
                  index,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapL4PortCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_FFU_MAP_L4_SRC_ENTRIES, FM_ERR_INVALID_SLOT);
    FM_API_REQUIRE(mapper < FM_FM10000_MAP_L4PORT_MAX, FM_ERR_INVALID_MAPPER);

    /* select whether to use DST vs SRC*/
    if (mapper == FM_FM10000_MAP_L4PORT_DST)
    {
        /* DST */
        regDesc = (fm_cachedRegs *)&fm10000CacheFfuMapL4Dst;
    }
    else
    {
        /* SRC */
        regDesc = (fm_cachedRegs *)&fm10000CacheFfuMapL4Src;
    }

    /* read from the register */
    err = fmRegCacheReadSingle1D(sw, 
                                 regDesc,
                                 mapL4PortRegister,
                                 index,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);


    /* extract all the fields */
    mapL4PortCfg->lowerBound = (fm_uint16) FM_ARRAY_GET_FIELD(mapL4PortRegister,
                                                              FM10000_FFU_MAP_L4_SRC,
                                                              L4SRC);

    mapL4PortCfg->mapProt = (fm_byte) FM_ARRAY_GET_FIELD(mapL4PortRegister,
                                                         FM10000_FFU_MAP_L4_SRC,
                                                         MAP_PROT);

    mapL4PortCfg->valid = (fm_byte) FM_ARRAY_GET_BIT(mapL4PortRegister,
                                                     FM10000_FFU_MAP_L4_SRC,
                                                     VALID);

    mapL4PortCfg->mapL4Port = (fm_uint16) FM_ARRAY_GET_FIELD(mapL4PortRegister,
                                                             FM10000_FFU_MAP_L4_SRC,
                                                             MAP_L4SRC);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000GetMapL4Port */



/*****************************************************************************/
/** fm10000SetMapVID
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the VLAN mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanId is the VLAN ID to be mapped.
 *
 * \param[in]       mapVIDCfg points to a structure of type
 *                  ''fm_fm10000MapVIDCfg'' containing the mapper 
 *                  configuration to set.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  mapVIDCfg should be written to the mapper. See
 *                  ''VID Mapper Field Selectors'' for bit
 *                  definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_VLAN if vlanId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapVIDCfg is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMapVID(fm_int               sw,
                           fm_uint16            vlanId,
                           fm_fm10000MapVIDCfg *mapVIDCfg,
                           fm_uint32            fieldSelectMask,
                           fm_bool              useCache)
{
    return fm10000SetMapVIDs(sw,
                             vlanId,
                             1,
                             mapVIDCfg,
                             fieldSelectMask,
                             useCache);
    
}   /* end fm10000SetMapVID */



/*****************************************************************************/
/** fm10000GetMapVID
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the VLAN mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanId is the VLAN ID whose mapper will be retrieved.
 *
 * \param[out]      mapVIDCfg points to a caller-allocated structure of type
 *                  ''fm_fm10000MapVIDCfg'' into which the mapper's 
 *                  configuration will be placed by this function.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_VLAN if vlanId is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMapVID(fm_int               sw,
                           fm_uint16            vlanId,
                           fm_fm10000MapVIDCfg *mapVIDCfg,
                           fm_bool              useCache)
{
    return fm10000GetMapVIDs(sw,
                             vlanId,
                             1,
                             mapVIDCfg,
                             useCache);

}   /* end fm10000GetMapVID */



/*****************************************************************************/
/** fm10000SetMapVIDs
 * \ingroup lowlevMap10K
 *
 * \desc            Configure the VLAN mapper for multiple VLAN IDs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       firstVlanId is the first VLAN ID in the sequence to be 
 *                  mapped.
 *
 * \param[in]       numVlanIds is the number of VLAN IDs to be mapped.
 *
 * \param[in]       mapVIDCfg is an array of structures of type
 *                  ''fm_fm10000MapVIDCfg'', numVlanIds elements in length,
 *                  containing the mapper configuration to set for each VLAN ID. 
 *                                                                      \lb\lb
 *                  The element in the array at index 0 corresponds to the 
 *                  VLAN ID number identified by firstVlanId, index 1 
 *                  corresponds to firstVlanId + 1, etc., up to the element 
 *                  at index numVlanIds - 1, which corresponds to the VLAN ID
 *                  identified by firstVlanId + numVlanIds - 1.
 *
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  each ''fm_fm10000MapVIDCfg'' element of mapVIDCfg should 
 *                  be written to the mapper. See ''VID Mapper Field 
 *                  Selectors'' for bit definitions.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_VLAN if firstVlanId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapVIDCfg is
 *                  invalid or if firstVlanId + numVlanIds - 1 exceeds the 
 *                  maximum possible VLAN ID.
 *
 *****************************************************************************/
fm_status fm10000SetMapVIDs(fm_int               sw,
                            fm_uint16            firstVlanId,
                            fm_uint16            numVlanIds,
                            fm_fm10000MapVIDCfg *mapVIDCfg,
                            fm_uint32            fieldSelectMask,
                            fm_bool              useCache)
{
    fm_registerSGListEntry sgList;
    fm_uint32             *mapVidTableRegisters;
    fm_int                 i;
    fm_cleanupListEntry   *cleanupList = NULL;
    fm_status              err = FM_OK;
    fm_bool                regLockTaken = FALSE;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "firstVlanId  = %d, "
                  "numVlanIds   = %d, "
                  "fieldSelect  = %u, "
                  "useCache     = %s\n",
                  sw,
                  firstVlanId,
                  numVlanIds,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapVIDCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(firstVlanId + numVlanIds - 1 < FM10000_FFU_MAP_VLAN_ENTRIES,
                   FM_ERR_INVALID_VLAN);

    /* now allocate memory for the scratch register area */
    FM_ALLOC_TEMP_ARRAY(mapVidTableRegisters, 
                        fm_uint32, 
                        numVlanIds * FM10000_FFU_MAP_VLAN_WIDTH);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheFfuMapVlan,
                              numVlanIds,
                              firstVlanId,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              mapVidTableRegisters,
                              FALSE);

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM_FM10000_MAP_VID_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* read the FFU_MAP_VLAN registers */
        err = fmRegCacheRead(sw, 1, &sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    }

    for (i = 0 ; i < numVlanIds ; i++)
    {
        if (fieldSelectMask & FM_FM10000_MAP_VID_MAPPEDVID)
        {
            FM_API_REQUIRE(mapVIDCfg[i].mapVid <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_MAP_VLAN, MAP_VLAN),
                           FM_ERR_INVALID_ARGUMENT);

            FM_ARRAY_SET_FIELD(mapVidTableRegisters + (i * FM10000_FFU_MAP_VLAN_WIDTH),
                               FM10000_FFU_MAP_VLAN,
                               MAP_VLAN,
                               mapVIDCfg[i].mapVid);
        }

        if (fieldSelectMask & FM_FM10000_MAP_VID_ROUTABLE)
        {
            FM_API_REQUIRE(mapVIDCfg[i].routable <= 1, FM_ERR_INVALID_ARGUMENT);

            FM_ARRAY_SET_BIT(mapVidTableRegisters + (i * FM10000_FFU_MAP_VLAN_WIDTH),
                             FM10000_FFU_MAP_VLAN,
                             Routable,
                             mapVIDCfg[i].routable);
        }

    }   /* end for ( i = 0; i < numVlanIds; ... ) */

    /* write to the hardware registers */
    err = fmRegCacheWrite(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);



ABORT:
    FM_FREE_TEMP_ARRAYS();
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000SetMapVIDs */



/*****************************************************************************/
/** fm10000GetMapVIDs
 * \ingroup lowlevMap10K
 *
 * \desc            Get the configuration of the VLAN mapper for multiple
 *                  VLAN IDs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       firstVlanId is the first VLAN ID in the sequence of VLAN 
 *                  IDs whose mappers are to be retrieved.
 *
 * \param[in]       numVlanIds is the number of VLAN IDs whose mappers are to
 *                  be retrieved.
 *
 * \param[out]      mapVIDCfg points to a caller-allocated array of structures 
 *                  of type ''fm_fm10000MapVIDCfg'', numVlanIds elements in
 *                  length, into which the mapper's configuration for each 
 *                  VID will be placed by this function. 
 *                                                                      \lb\lb
 *                  The element placed in the array at index 0 will correspond
 *                  to the VLAN ID number identified by firstVlanId, index 1 
 *                  will correspond to firstVlanId + 1, etc., up to the element 
 *                  at index numVlanIds - 1, which will correspond to the VLAN
 *                  ID identified by firstVlanId + numVlanIds - 1.
 *
 * \param[in]       useCache indicates whether the cache is to be used.
 *                  If TRUE, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_VLAN if firstVlanId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of mapVIDCfg is
 *                  invalid or if firstVlanId + numVlanIds - 1 exceeds the 
 *                  maximum possible VLAN ID.
 *
 *****************************************************************************/
fm_status fm10000GetMapVIDs(fm_int               sw,
                            fm_uint16            firstVlanId,
                            fm_uint16            numVlanIds,
                            fm_fm10000MapVIDCfg *mapVIDCfg,
                            fm_bool              useCache)
{
    fm_registerSGListEntry sgList;
    fm_uint32             *mapVidTableRegisters;
    fm_int                 i;
    fm_cleanupListEntry   *cleanupList = NULL;
    fm_status              err = FM_OK;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw           = %d, "
                  "firstVlanId  = %d, "
                  "numVlanIds   = %d, "
                  "useCache     = %s\n",
                  sw,
                  firstVlanId,
                  numVlanIds,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(mapVIDCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(firstVlanId + numVlanIds - 1 < FM10000_FFU_MAP_VLAN_ENTRIES,
                   FM_ERR_INVALID_VLAN);

    /* now allocate memory for the scratch register area */
    FM_ALLOC_TEMP_ARRAY(mapVidTableRegisters, 
                        fm_uint32, 
                        numVlanIds * FM10000_FFU_MAP_VLAN_WIDTH);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheFfuMapVlan,
                              numVlanIds,
                              firstVlanId,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              mapVidTableRegisters,
                              FALSE);

    /* read the FFU_MAP_VLAN registers */
    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);

    /* save the register content in API format */
    for (i = 0 ; i < numVlanIds  ; i++)
    {
        mapVIDCfg[i].mapVid = (fm_uint16)
                               FM_ARRAY_GET_FIELD(mapVidTableRegisters + (i * FM10000_FFU_MAP_VLAN_WIDTH),
                                                  FM10000_FFU_MAP_VLAN,
                                                  MAP_VLAN);

        mapVIDCfg[i].routable = (fm_byte)
                                 FM_ARRAY_GET_BIT(mapVidTableRegisters + (i * FM10000_FFU_MAP_VLAN_WIDTH),
                                                  FM10000_FFU_MAP_VLAN,
                                                  Routable);

    }   /* end for ( i = 0; i < numVlanIds; ... ) */


ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);
    
}   /* end fm10000GetMapVIDs */



/*****************************************************************************/
/** fm10000SetMappOwnership
 * \ingroup lowlevMap10K
 *
 * \desc            Sets which software component (as defined by the owner)
 *                  has control of the given mapping resource.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       owner represents a valid software component.
 *
 * \param[in]       mapResource is the resource to mark ownership on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_FFU_RES_OWNED if this resource is already owned.
 *
 *****************************************************************************/
fm_status fm10000SetMapOwnership(fm_int                 sw,
                                 fm_fm10000MapOwnerType owner,
                                 fm_fm10000MapResource  mapResource)
{
    fm10000_switch             *switchExt = NULL;
    fm_fm10000MapOwnershipInfo *info = NULL;
    fm_status                   err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw    = %d, "
                  "owner = %d, "
                  "mapResource = %d\n",
                  sw,
                  owner,
                  mapResource);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    switchExt = GET_SWITCH_EXT(sw);
    info      = &switchExt->mapOwnershipInfo;

    FM_API_REQUIRE(mapResource < FM_FM10000_MAP_RESOURCE_MAX,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(owner < FM_FM10000_MAP_OWNER_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    if ( (info->mapperOwners[mapResource] != FM_FM10000_MAP_OWNER_NONE) &&
         (info->mapperOwners[mapResource] != owner) && 
         (owner != FM_FM10000_MAP_OWNER_NONE) )
    {
        err = FM_ERR_FFU_RES_OWNED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    info->mapperOwners[mapResource] = owner;

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000SetMapOwnership */



/*****************************************************************************/
/** fm10000GetMapOwnership 
 * \ingroup lowlevMap10K
 *
 * \desc            Gets which software component has control of the given
 *                  mapping resource.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      owner points to caller allocated storage where the owner
 *                  of the given resource is written to.
 *
 * \param[in]       mapResource is the resource to check ownership for.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetMapOwnership(fm_int                  sw,
                                 fm_fm10000MapOwnerType *owner,
                                 fm_fm10000MapResource   mapResource)
{
    fm10000_switch             *switchExt = NULL;
    fm_fm10000MapOwnershipInfo *info = NULL;
    fm_status                   err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_MAP,
                  "sw = %d, "
                  "owner = %p, "
                  "mapResource = %d\n",
                  sw,
                  (void *) owner,
                  mapResource);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsMapper(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAP, err);
    }

    switchExt = GET_SWITCH_EXT(sw);
    info      = &switchExt->mapOwnershipInfo;

    FM_API_REQUIRE(owner, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(mapResource < FM_FM10000_MAP_RESOURCE_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    *owner = info->mapperOwners[mapResource];


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAP, err);

}   /* end fm10000GetMapOwnership */

