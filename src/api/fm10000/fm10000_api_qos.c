/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_qos.c
 * Creation Date:   December 13, 2013
 * Description:     FM10000 QoS API functions
 *
 * Copyright (c) 2013 - 2016, Intel Corporation
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
#include <common/fm_version.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* The maximum index of  VLAN priorities - based on a platform define */
#define FM10000_QOS_MAX_VLAN_PRI (FM_MAX_VLAN_PRIORITIES - 1)

/* The maximum index of shared mem. partition - based on a platform define */
#define FM10000_QOS_MAX_SMP      (FM_MAX_MEMORY_PARTITIONS - 1)

/* The maximum index of  switch priorities - based on a platform define */
#define FM10000_QOS_MAX_SW_PRI   (FM_MAX_SWITCH_PRIORITIES - 1)

/* The maximum index of  traffic class - based on a platform define */
#define FM10000_QOS_MAX_TC       (FM_MAX_TRAFFIC_CLASSES - 1)

/* The minimum index of  traffic class */
#define FM10000_QOS_MIN_TC       0

/* The maximum index of  DSCP priorities -  based on platform define */
#define FM10000_QOS_MAX_DSCP     (FM_MAX_DSCP_PRIORITIES - 1)

/* The maximum index of  shaping group - based on the limiter register config */
#define FM10000_QOS_MAX_SG       (FM10000_TX_RATE_LIM_USAGE_ENTRIES_0 - 1)

/* The maximum index of  PAUSE class */
#define FM10000_QOS_MAX_PC       7

/* The number of bytes per segments. */
#define BYTES_PER_SMP_SEG        FM10000_SEGMENT_SIZE

/* The number of bytes per kilo bytes. */
#define BYTES_PER_KB                    1024

/* Watermark scaling methods */
#define FM10000_QOS_SCALE_TOTAL_MEM     0
#define FM10000_QOS_SCALE_GLOBAL_WM     1

/* Total memory size specified in segments units */
#define FM10000_QOS_MAX_NUM_SEGS        FM10000_MAX_MEMORY_SEGMENTS
#define FM10000_QOS_TOTAL_MEMORY_BYTES  (FM10000_MAX_MEMORY_SEGMENTS * \
                                         BYTES_PER_SMP_SEG)

/* Number of reserved segments */
#define FM10000_QOS_NUM_RESERVED_SEGS   256

/* Maximum number of segments for most watermark settings */
#define FM10000_QOS_MAX_WATERMARK_SEGS  0x7FFF
#define FM10000_QOS_MAX_WATERMARK     SEG2BYTES(FM10000_QOS_MAX_WATERMARK_SEGS)

/* Default values for some registers settings */
#define FM10000_QOS_WATERMARK_DEFAULT   FM10000_QOS_MAX_WATERMARK_SEGS

/* Max values for Tx rate limiter*/
#define FM10000_QOS_SHAPING_GROUP_RATE_FRAC_MAX_VAL 0xFF
#define FM10000_QOS_SHAPING_GROUP_RATE_UNIT_MAX_VAL 0x7FF

/* SMP identification  as a specific number for different watermark schema */
#define FM10000_QOS_SMP_LOSSY           0
#define FM10000_QOS_SMP_LOSSLESS        1
#define FM10000_QOS_SMP_PAUSE_OFF       2
#define FM10000_QOS_SMP_DEFAULT         FM10000_QOS_SMP_LOSSY

/* Maximum values of some attributes */
#define FM10000_QOS_SCHED_GROUP_STRICT_MAX_VAL          1
#define FM10000_QOS_SCHED_GROUP_WEIGHT_MAX_VAL          0xFFFFFF
#define FM10000_QOS_SHARED_PAUSE_ENABLE_MAX_VAL     \
                                           ((1 << FM10000_MAX_SWITCH_SMPS) -1 )
#define FM10000_QOS_SHAPING_GROUP_MAX_BURST_MAX_VAL     (0x1FFF << 13)
#define FM10000_QOS_TC_ENABLE_MAX_VAL    ((1 << FM10000_MAX_TRAFFIC_CLASS) -1 )
#define FM10000_QOS_SCHED_IFGPENALTY_MAX_VAL            255
#define FM10000_QOS_SHARED_SOFT_DROP_WM_JITTER_MAX_VAL  7
#define FM10000_QOS_SWITCH_IFG_PENALTY_MAX_VAL          0xFF

#define FM10000_QOS_QUEUE_DRR_Q_UNIT                    167772


/***************************************************************************/
/* Convert from bytes to segments.
 ***************************************************************************/
#define BYTES2SEG(x)  ( ( (x) + (BYTES_PER_SMP_SEG - 1) ) / BYTES_PER_SMP_SEG )

/***************************************************************************/
/* Convert from segments to bytes.
 ***************************************************************************/
#define SEG2BYTES(x)        ( (x) * BYTES_PER_SMP_SEG )

/***************************************************************************/
/* Goto ABORT when the val exceeds valref or is less than 0.
 *                                                                      
 * This macro should be used to replace the typical pattern of going to a 
 * local ABORT label when the value exceeds specified reference value and is 
 * less than 0.
 *                                                                      
 * val is the value used for verifying.
 *                                                                      
 * valref is the refernce value.
 *                                                                      
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      
 * errstatus is the status code returned when the val exceeds valref
 *
 ***************************************************************************/
#define FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(val, valref, errcode, errstatus) \
    {                                                                          \
        if ( ((val) < 0) || ((val) > (valref)) )                               \
        {                                                                      \
            (errcode) = (errstatus);                                           \
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, (errcode));                    \
        }                                                                      \
    }  

/***************************************************************************/
/* Goto ABORT when the val exceeds valref.
 *                                                                      
 * This macro should be used to replace the typical pattern of going to a 
 * local ABORT label when the value exceeds specified reference value. 
 *                                                                      
 * val is the value used for verifying.
 *                                                                      
 * valref is the refernce value.
 *                                                                      
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      
 * errstatus is the status code returned when the val exceeds valref
 *
 ***************************************************************************/
#define FM10000_QOS_ABORT_ON_EXCEED(val, valref, errcode, errstatus)           \
    {                                                                          \
        if ( (val) > (valref) )                                                \
        {                                                                      \
            (errcode) = (errstatus);                                           \
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, (errcode));                    \
        }                                                                      \
    }  

/***************************************************************************/
/* Goto ABORT when auto pause mode is ON.
 *                                                                      
 * This macro should be used to replace the typical pattern of going to a 
 * local ABORT label when the auto pause mode is enabled. 
 *                                                                      
 * swIdx is the switch index.
 *                                                                      
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      
 * errstatus is the status code returned when the auto pause is ON.
 *
 ***************************************************************************/
#define FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(swIdx, errcode, errstatus)        \
    {                                                                          \
        fm_bool localAutoPauseMode;                                            \
        errcode =                                                              \
        fm10000GetSwitchQOS(swIdx, FM_AUTO_PAUSE_MODE, 0, &localAutoPauseMode);\
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, errcode);                          \
        if (localAutoPauseMode)                                                \
        {                                                                      \
            errcode = errstatus;                                               \
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, (errcode));                    \
        }                                                                      \
    }

/***************************************************************************/
/* Initialize priority mapper list.
 *
 * This macro should be used to replace the typical pattern of initializing 
 * priority mapper list.
 *
 * list is the priority mapper list.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_LIST_INITIALIZE(list)                          \
    FM_DLL_INIT_LIST((list), firstMapper, lastMapper)

/***************************************************************************/
/* Get the first element from the priority mapper list.
 *
 * This macro should be used to replace the typical pattern of getting 
 * the first element from the priority mapper list.
 *
 * list is the priority mapper list.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_FIRST_GET(list)                                \
    FM_DLL_GET_FIRST((list), firstMapper)

/***************************************************************************/
/* Pop one element from the priority mapper list.
 *
 * This macro should be used to replace the typical pattern of getting 
 * one element from the priority mapper list.
 *
 * list is the priority mapper list.
 * priorityMapper is the element taken from the list 
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_ENTRY_POP( list, priorityMapper)              \
    (priorityMapper) = FM10000_PRIORITY_MAPPER_FIRST_GET((list));             \
    if ((priorityMapper) != NULL)                                             \
    {                                                                         \
        FM_DLL_REMOVE_NODE((list),                                            \
                           firstMapper,                                       \
                           lastMapper,                                        \
                           (priorityMapper),                                  \
                           nextMapper,                                        \
                           previousMapper);                                   \
    }

/***************************************************************************/
/* Push one element to the priority mapper list.
 *
 * This macro should be used to replace the typical pattern of putting 
 * one element to the priority mapper list.
 *
 * list is the priority mapper list.
 * priorityMapper is the element putting into the list 
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_ENTRY_PUSH(list, priorityMapper)              \
    FM_DLL_INSERT_FIRST((list),                                               \
                        firstMapper,                                          \
                        lastMapper,                                           \
                        (priorityMapper),                                     \
                        nextMapper,                                           \
                        previousMapper);

/***************************************************************************/
/* Validate the priority mapper id.
 *
 * This macro should be used to replace the typical pattern of validating 
 * priority mapper id.
 *
 * switchExt points to the switch extension struct.
 * mapper is the id to validate 
 * status is the error status which should be reported in a case of error
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_VALIDATE(switchExt, mapper, status)           \
    if (   (mapper < 0 || mapper >= FM10000_MAX_SWITCH_PRIORITIES)            \
        || (switchExt->priorityMapSet->mappers[mapper].used == FALSE))        \
    {                                                                         \
        status = FM_ERR_INVALID_PRIORITY_MAPPER;                              \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);                          \
    }

/***************************************************************************/
/* Initialize map list of priority mapper.
 *
 * This macro should be used to replace the typical pattern of initializing 
 * map list of priority mapper.
 *
 * list is the map list of priority mapper.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_LIST_MAP_INITIALIZE(list)                      \
    FM_DLL_INIT_LIST((list), firstMap, lastMap)

/***************************************************************************/
/* Get the first element from the map list.
 *
 * This macro should be used to replace the typical pattern of getting 
 * the first element from the map list.
 *
 * list is the map list.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_FIRST_MAP_GET(list)                            \
    FM_DLL_GET_FIRST((list), firstMap)

/***************************************************************************/
/* Get the next element from the map list.
 *
 * This macro should be used to replace the typical pattern of getting 
 * the next element from the map list.
 *
 * list is the map list.
 * type is type of the list specified by fm10000_priorityMapListType.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_NEXT_MAP_GET(map, type)                        \
    FM_DLL_GET_NEXT((map), nextMap[(type)])

/***************************************************************************/
/* Push one element to the map list.
 *
 * This macro should be used to replace the typical pattern of putting 
 * one element the the map list.
 *
 * list is the map list.
 * type is type of the list specified by fm10000_priorityMapListType.
 * map is the element to put on the list.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_ENTRY_MAP_PUSH(list, type, map)               \
    FM_DLL_INSERT_FIRST((list),                                               \
                        firstMap,                                             \
                        lastMap,                                              \
                        (map),                                                \
                        nextMap[(type)],                                      \
                        previousMap[(type)]);

/***************************************************************************/
/* Remove one element from the map list.
 *
 * This macro should be used to replace the typical pattern of removing 
 * one element from the map list.
 *
 * list is the map list.
 * type is type of the list specified by fm10000_priorityMapListType.
 * map is the element to remove from the list.
 *
 ***************************************************************************/
#define FM10000_PRIORITY_MAPPER_ENTRY_MAP_REMOVE(list, type, map)             \
    FM_DLL_REMOVE_NODE((list),                                                \
                       firstMap,                                              \
                       lastMap,                                               \
                       (map),                                                 \
                       nextMap[(type)],                                       \
                       previousMap[(type)]);


/***************************************************************************/
/* Logging information related to single watermark configuration.
 *
 * This macro should be used to replace the typical pattern of logging 
 * qos related information for watermark configuration.
 *
 * string is the name of configaration entry.
 * segSize is size of the single configuration entry provided in segments units.
 *
 ***************************************************************************/
#define FM10000_QOS_LOG_INFO(string, segSize)                                 \
    FM_LOG_INFO(FM_LOG_CAT_QOS, " %-45s      %7d(%5d) \n",                    \
                (string),                                                     \
                ((segSize) * BYTES_PER_SMP_SEG),                              \
                (segSize));

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* Enumeration for internal trap classes definitions  */

enum _fm10000_trapClass
{    
    FM10000_QOS_TRAP_CLASS_CPU_MAC,
    /* HandlerActionMask = FM10000_TRIGGER_HA_TRAP_CPU */

    FM10000_QOS_TRAP_CLASS_ICMP,
    /* HandlerActionMask = FM10000_TRIGGER_HA_TRAP_ICMP_TTL */

    FM10000_QOS_TRAP_CLASS_IGMP,
    /* HandlerActionMask = FM10000_TRIGGER_HA_TRAP_IGMP */

    FM10000_QOS_TRAP_CLASS_IP_OPTION,
    /* HandlerActionMask = FM10000_TRIGGER_HA_TRAP_IP_OPTION */

    FM10000_QOS_TRAP_CLASS_MTU_VIOLATION,
    /* HandlerActionMask = FM10000_TRIGGER_HA_TRAP_MTU */

    FM10000_QOS_TRAP_CLASS_BCAST_FLOODING,
    /* Use bcastFloodingTrapTrigger*/

    FM10000_QOS_TRAP_CLASS_MCAST_FLOODING,
    /* Use mcastFloodingTrapTrigger*/

    FM10000_QOS_TRAP_CLASS_UCAST_FLOODING,
    /* Use ucastFloodingTrapTrigger */

    FM10000_QOS_TRAP_CLASS_TTL1,
    /* HandlerActionMask = FM10000_TRIGGER_HA_TRAP_TTL */

    FM10000_QOS_TRAP_CLASS_NEXTHOP_MISS,
    /* Use routingUnresolvedNextHopT2 trigger */

    FM10000_QOS_TRAP_CLASS_RESERVED_MAC,
    /* Use FM_SWITCH_RESERVED_MAC_TRAP_PRI switch priority */

    FM10000_QOS_TRAP_CLASS_MAX
};

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
static fm_status SetAutoPauseMode(fm_int sw,
                                  fm_bool defaultMaps,
                                  fm_bool enable);

static fm_status ReadCmRegMaps(fm_int sw, 
                               fm10000_wmParam *wpm);

static fm_status SetMappingTables(fm_int sw, 
                                  fm10000_wmParam *wpm);

static fm_status SetWatermarks(fm_int sw, 
                               fm10000_wmParam *wpm);

static fm_status SetDefaultMaps(fm_int smpId, 
                                fm10000_wmParam *wpm);

static fm_status GetWmParamsDisabled(fm_int          sw,
                                     fm_int          mtu, 
                                     fm_bool         defaultMaps, 
                                     fm10000_wmParam *wpm);

static fm_status GetWmParamsLossy(fm_int sw, 
                                  fm_int mtu, 
                                  fm_int scalePercent,
                                  fm_int scaleBasedOn,
                                  fm_uint32 sizeInBytes,
                                  fm_bool defaultMaps,
                                  fm10000_wmParam *wpm);

static fm_status GetWmParamsLossless(fm_int sw, 
                                     fm_int mtu, 
                                     fm_bool singlePartition,
                                     fm_uint32 sizeInBytes,
                                     fm_bool defaultMaps,
                                     fm10000_wmParam *wpm);

static fm_status GetWmParamsLossyLossless(fm_int sw, 
                                          fm_int mtu, 
                                          fm_bool defaultMaps,
                                          fm10000_wmParam *wpm);

static fm_status GetTrapClassMap(fm_int sw, 
                                 fm_int trapClass, 
                                 fm_uint *priority);

static fm_status SetTrapClassMap(fm_int sw, fm_int trapClass, fm_uint priority);

static fm_status PriorityMapperAllocate(fm_int sw,
                                        fm_int trapClass,
                                        fm_int *mapper);

static fm_status PriorityMapperFree(fm_int sw, fm_int mapper);

static fm_status PriorityMapperAddMap(fm_int               sw,
                                      fm_int               mapper,
                                      fm10000_priorityMap *map);

static fm_status PriorityMapperDeleteMap(fm_int               sw,
                                         fm_int               mapper,
                                         fm10000_priorityMap *map);

static fm_status PriorityMapperGet(fm_int sw, fm_uint priority, fm_int *mapper);

static fm_status PriorityMapperAttributeGet(fm_int sw,
                                     fm_int mapper, 
                                     fm_int attribute,
                                     void * value);

static fm_status PriorityMapperAttributeSet(fm_int sw,
                                            fm_int mapper, 
                                            fm_int attribute,
                                            void * value);

static fm_status PriorityMapperSearchByClass(fm_int  sw,
                                             fm_int  trapClass,
                                             fm_int *mapper);

static fm_status PriorityMapperTrapClassConvert(fm_int qosTrapClass,
                                                fm_int *internalTrapClass);

static fm_uint64 PriorityMapperQoSTrapClassToHaMask(fm_int trapClass);

static fm_status PriorityMapperFindMap(
                                      fm_int                       sw,
                                      fm_int                       trapClass,
                                      fm10000_internalPriorityMap **internalMap);

static fm_status PriorityMapperApplyMapSet(fm_int sw, 
                                           fm10000_internalPriorityMap * map,
                                           fm_bool enabled);

static fm_status ESchedGroupBuildActiveConfigDataStruct(fm_int  sw,
                                                        fm_int  physPort,
                                                        fm_int  grpBoundary,
                                                        fm_int  strictBits,
                                                        fm_int *drrWeightPtr);

static fm_status CheckGlobalWm(fm_int    sw,
                               fm_uint32 globalWm);

static fm_status ESchedInit(fm_int sw,
                            fm_int physPort,
                            fm_bool qosQueueEnabled);

static fm_status EschedQosQueueConfig(fm_int sw,
                                      fm_int physPort,
                                      fm_int attr,
                                      fm_qosQueueParam *param);

static fm_status TxLimiterRegConfig(fm_int sw,
                                    fm_int physPort,
                                    fm_int sg,
                                    fm_uint64 bw);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/
static fm_status SetAutoPauseMode(fm_int    sw,
                                  fm_bool   defaultMaps,
                                  fm_bool   enable)
{
    fm_switch *         switchPtr;
    fm_status           err;
    fm_status           err2;
    fm_text             wmScheme;
    fm10000_wmParam *   wpm;
    fm_int              i;
    fm_mtuEntry         mtuEntry;
    fm_uint32           maxMtu;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d defaultMaps=%d enable=%d\n", 
                 sw, defaultMaps, enable);

    /* Initialize local variables */
    err = FM_OK;
    err2 = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Allocate memory for watermark configuration */
    wpm = fmAlloc(sizeof(fm10000_wmParam));
    if (wpm == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_NO_MEM);
    }

    /* Determine the maximum switch MTU to use in the watermark calculations */ 
    maxMtu = 0;
    for (i = 0 ; i < FM10000_MTU_TABLE_ENTRIES; i++)
    {
        mtuEntry.index = i;
        err = fmGetSwitchAttribute(sw, FM_MTU_LIST, &mtuEntry);

        if ((err == FM_OK) && (mtuEntry.mtu > maxMtu))
        {
            maxMtu = mtuEntry.mtu;
        }
    }

    /* Set mtu to max size if not initialized */
    if (maxMtu == 0)
    {
        maxMtu = FM10000_MAX_FRAME_SIZE;
    }

    /* Set auto pause mode */
    switchPtr->autoPauseMode = enable;

    if (switchPtr->autoPauseMode)
    {
        /* Get watermark schema */
        wmScheme = GET_FM10000_PROPERTY()->wmSelect;

        if (!strcmp(wmScheme, "disabled"))
        {

            /* Disable all watermarks */
            err = GetWmParamsDisabled(sw, 
                                      maxMtu,
                                      defaultMaps, 
                                      wpm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        }
        else if (!strcmp(wmScheme, "lossy"))
        {

            /* Set lossy configuration */
            err = GetWmParamsLossy(sw, 
                                   maxMtu, 
                                   100,
                                   FM10000_QOS_SCALE_TOTAL_MEM, 
                                   FM10000_QOS_TOTAL_MEMORY_BYTES,
                                   defaultMaps, 
                                   wpm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        }
        else if (!strcmp(wmScheme, "lossless"))
        {

            /* Set lossless configuration */
            err = GetWmParamsLossless(sw, 
                                      maxMtu, 
                                      TRUE,
                                      FM10000_QOS_TOTAL_MEMORY_BYTES, 
                                      defaultMaps, 
                                      wpm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        }
        else if (!strcmp(wmScheme, "lossy_lossless"))
        {

            /* Half lossy, half lossless configuration */
            err = GetWmParamsLossyLossless(sw, 
                                           maxMtu, 
                                           defaultMaps, 
                                           wpm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        }
        else
        {

            FM_LOG_ERROR(FM_LOG_CAT_QOS,
                         "Unrecognized value for API attribute %s: %s\n",
                         FM_AAK_API_FM10000_WMSELECT,
                         wmScheme);
            err = FM_ERR_INVALID_ATTRIB;
            goto ABORT;

        }

        /* Set watermarks */
        err = SetWatermarks(sw, wpm); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Set default mapping tables if specified */
        if (defaultMaps)
        {

            err = SetMappingTables(sw, wpm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        }
    }
    else if ( (!switchPtr->autoPauseMode) &&
              (GET_PROPERTY()->resetWmAtPauseOff) )
    {
        /* Auto Pause Mode disabled and 
           reset watermarks at autoPauseOff api attribute specified */

        /* Disable all watermarks */
        err = GetWmParamsDisabled(sw, maxMtu, defaultMaps, wpm);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Set Watermarks */
        err = SetWatermarks(sw, wpm);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Set default mapping tables if specified */
        if (defaultMaps)
        {

            err = SetMappingTables(sw, wpm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        }
    }

ABORT:
    /* Free memory allocated for watermark configuration */
    fmFree(wpm);

    FM_LOG_EXIT(FM_LOG_CAT_QOS, (err == FM_OK) ? err2 : err);

}   /* end SetAutoPauseMode */




/*****************************************************************************/
/** ReadCmRegMaps
 * \ingroup intQos
 *
 * \desc            Read CM mapping from registers and set to local structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      wpm is the user allocated watermark parameters pointer 
 *                  which points to the structure where retrieved mapping 
 *                  will be stored.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ATTRIB if pointer argument is incorrect.
 *
 *****************************************************************************/
static fm_status ReadCmRegMaps(fm_int sw, 
                               fm10000_wmParam *wpm)
{
    fm_bool         regLockTaken;
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint64       swPriToTcMap;
    fm_uint32       tcToSmpMap;
    fm_uint32       addr;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);

    /* Initialize local variables */
    err = FM_OK;
    regLockTaken = FALSE;

    /* Check argument pointer*/
    if (wpm == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_INVALID_ATTRIB);
    }

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Get register lock to ensure both mappings is get for the same 
       configuration */
    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Read CM_APPLY_SWITCH_PRI_TO_TC */
    addr = FM10000_CM_APPLY_SWITCH_PRI_TO_TC(0);
    err = switchPtr->ReadUINT64(sw, addr, &swPriToTcMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Read CM_APPLY_TC_TO_SMP */
    addr = FM10000_CM_APPLY_TC_TO_SMP();
    err = switchPtr->ReadUINT32(sw, addr, &tcToSmpMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Drop register lock */
    FM_FLAG_DROP_REG_LOCK(sw);

    /* When both readings finished save results in the structure */
    wpm->cmSwPriTcMap = swPriToTcMap;
    wpm->cmTcSmpMap = tcToSmpMap;

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end ReadCmRegMaps */




/*****************************************************************************/
/** SetMappingTables
 * \ingroup intQos
 *
 * \desc            Set the mapping configuration into the chip registers
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       wpm is a pointer to the watermark parameters to set
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetMappingTables(fm_int sw, 
                                  fm10000_wmParam *wpm)
{
    fm_status       err;
    fm_int          cpi;
    fm_int          physPort;
    fm_int          logPort;
    fm_switch *     switchPtr;
    fm_bool         regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);

    /* Initialize local variables */
    err = FM_OK;
    regLockTaken = FALSE;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Acquire register lock */
    FM_FLAG_TAKE_REG_LOCK(sw);

    /* CM_APPLY_SWITCH_PRI_TO_TC */
    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_CM_APPLY_SWITCH_PRI_TO_TC(0), 
                                 wpm->cmSwPriTcMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* CM_SWEEPER_SWITCH_PRI_TO_TC */
    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_CM_SWEEPER_SWITCH_PRI_TO_TC(0), 
                                 wpm->cmSwPriTcMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* CM_APPLY_TC_TO_SMP */
    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_CM_APPLY_TC_TO_SMP(), 
                                 wpm->cmTcSmpMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* CM_SWEEPER_TC_TO_SMP */
    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_CM_SWEEPER_TC_TO_SMP(), 
                                 wpm->cmTcSmpMap); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* CM_TC_PC_MAP */
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_CM_TC_PC_MAP(physPort), 
                                     wpm->cmTcPcMap[physPort]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* CM_PC_SMP_MAP */
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_CM_PC_SMP_MAP(physPort), 
                                     wpm->cmPcSmpMap[physPort]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end SetMappingTables */




/*****************************************************************************/
/** SetWatermarks
 * \ingroup intQos
 *
 * \desc            Set the watermark configuration into chip registers
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       wpm is a pointer to the watermark parameters to set
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetWatermarks(fm_int sw, 
                               fm10000_wmParam *wpm)
{
    fm_status           err;
    fm_int              cpi;
    fm_int              physPort;
    fm_int              logPort;
    fm_int              smp;
    fm_int              swPri;
    fm_switch *         switchPtr;
    fm_bool             regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Initialize local variables */
    err = FM_OK;
    regLockTaken = FALSE;

    /* Acquire register lock */
    FM_FLAG_TAKE_REG_LOCK(sw);

    /* CM_GLOBAL_WM */
    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_CM_GLOBAL_WM(), 
                                 wpm->cmGlobalWm);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        int tc;

        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ; smp <= FM10000_QOS_MAX_SMP; smp++)
        {
            /* CM_RX_SMP_PRIVATE_WM */
            err = switchPtr->WriteUINT32(sw, 
                                    FM10000_CM_RX_SMP_PRIVATE_WM(physPort, smp), 
                                    wpm->cmRxSmpPrivateWm[physPort][smp]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            /* CM_RX_SMP_PAUSE_WM */
            err = switchPtr->WriteUINT32(sw, 
                                      FM10000_CM_RX_SMP_PAUSE_WM(physPort, smp), 
                                      wpm->cmRxSmpPauseWm[physPort][smp]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            /* CM_RX_SMP_HOG_WM */ 
            err = switchPtr->WriteUINT32(sw, 
                                        FM10000_CM_RX_SMP_HOG_WM(physPort, smp), 
                                        wpm->cmRxSmpHogWm[physPort][smp]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        }

        for (tc = 0 ; tc <= FM10000_QOS_MAX_TC; tc++)
        {
            /* CM_TX_TC_HOG_WM */
            err = switchPtr->WriteUINT32(sw, 
                                         FM10000_CM_TX_TC_HOG_WM(physPort, tc), 
                                         wpm->cmTxTcHogWm[physPort][tc]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            /* CM_TX_TC_PRIVATE_WM */
            err = switchPtr->WriteUINT32(sw, 
                                      FM10000_CM_TX_TC_PRIVATE_WM(physPort, tc), 
                                      wpm->cmTxTcPrivateWm[physPort][tc]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        }

        for (swPri = 0 ; swPri <= FM10000_QOS_MAX_SW_PRI; swPri++)
        {
            /* CM_APPLY_TX_SOFTDROP_CFG */
            err = switchPtr->WriteUINT32(sw, 
                              FM10000_CM_APPLY_TX_SOFTDROP_CFG(physPort, swPri), 
                              wpm->cmApplyTxSoftDropCfg[physPort][swPri]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        }
    }

    for (smp = 0 ; smp <= FM10000_QOS_MAX_SMP; smp++)
    {
        /* CM_SHARED_SMP_PAUSE_WM */
        err = switchPtr->WriteUINT32(sw, 
                                FM10000_CM_SHARED_SMP_PAUSE_WM(smp), 
                                wpm->cmSharedSmpPauseWm[smp]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

    for (swPri = 0 ; swPri <= FM10000_QOS_MAX_SW_PRI; swPri++)
    {
        /* CM_SHARED_WM */
        err = switchPtr->WriteUINT32(sw, 
                                FM10000_CM_SHARED_WM(swPri), 
                                wpm->cmSharedWm[swPri]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        
        /* CM_SOFTDROP_WM */ 
        err = switchPtr->WriteUINT32(sw, 
                                FM10000_CM_SOFTDROP_WM(swPri), 
                                wpm->cmSoftDropWm[swPri]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* CM_APPLY_SOFTDROP_CFG */
        err = switchPtr->WriteUINT32(sw, 
                                FM10000_CM_APPLY_SOFTDROP_CFG(swPri), 
                                wpm->cmApplySoftDropCfg[swPri]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end SetWatermarks */




/*****************************************************************************/
/** SetDefaultMaps
 * \ingroup intQos
 *
 * \desc            Initialize the CM mapping tables to their defaults.
 *
 * \param[in]       smpId  is the SMP identification for each initialization 
 *                  should be done.
 *
 * \param[out]      wpm is the user allocated watermark parameters pointer
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ATTRIB if pointer argument is incorrect.
 *
 *****************************************************************************/
static fm_status SetDefaultMaps(fm_int smpId, 
                                fm10000_wmParam *wpm)
{
    fm_int      i;
    fm_int      j;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "SMP ID=%d\n", smpId);

    /* Initialize return code */
    err = FM_OK;

    /* Check argument pointer*/
    if (wpm == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_INVALID_ATTRIB);
    }

    /* CM_APPLY_TC_TO_SMP */
    /* All traffic calsses map to SMP 0 - LOSSY. */
    for (i = 0 ; i <= FM10000_QOS_MAX_TC ; i++)
    {
        FM_SET_UNNAMED_FIELD(wpm->cmTcSmpMap, 
                             (i * FM10000_CM_APPLY_TC_TO_SMP_s_smp),
                             FM10000_CM_APPLY_TC_TO_SMP_s_smp,
                             smpId);
    }

    /* CM_APPLY_SWITCH_PRI_TO_TC */
    for (i = 0 ; i <= FM10000_QOS_MAX_SW_PRI ; i++)
    {
        /* Maps switch priorities 0 through 7 to traffic classes 
           0 through 7 respectively and swtich priorities 
           8 through 15 to traffic classes 0 through 7, 
           respectively */
        FM_SET_UNNAMED_FIELD64(wpm->cmSwPriTcMap, 
                               (i * FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc),
                               FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc,
                               (i < FM_MAX_TRAFFIC_CLASSES) ? 
                               i : i - FM_MAX_TRAFFIC_CLASSES);
    }

    /* CM_TC_PC_MAP: TC to PC mapping is 1:1*/
    for (i = 0 ; i < FM10000_CM_TC_PC_MAP_ENTRIES ; i++)
    {
        for (j = 0 ; j <= FM10000_QOS_MAX_TC; j++)
        {
            FM_SET_UNNAMED_FIELD(wpm->cmTcPcMap[i], 
                                 (j * FM10000_CM_TC_PC_MAP_s_PauseClass),
                                 FM10000_CM_TC_PC_MAP_s_PauseClass,
                                 j);
        }
    }

    /*CM_PC_SMP_MAP: Lossy will always remain pause off*/
    for (i = 0 ; i < FM10000_CM_PC_SMP_MAP_ENTRIES ; i++)
    {
        for (j = 0 ; j <= FM10000_QOS_MAX_PC; j++)
        {
            FM_SET_UNNAMED_FIELD(wpm->cmPcSmpMap[i], 
                                 (j * FM10000_CM_PC_SMP_MAP_s_SMP),
                                 FM10000_CM_PC_SMP_MAP_s_SMP,
                                 (smpId == FM10000_QOS_SMP_LOSSY) ? 
                                    FM10000_QOS_SMP_PAUSE_OFF : 
                                    smpId);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end SetDefaultMaps */




/*****************************************************************************/
/** GetWmParamsDisabled
 * \ingroup intQos
 *
 * \desc            Get the watermark parameters for the disabled scheme.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mtu is the largest MTU the switch will support in this
 *                  configuration.
 *
 * \param[in]       defaultMaps determines whether to initialize the mapping
 *                  tables to their defaults or leave as is.
 * 
 * \param[out]      wpm is the user allocated watermark parameters pointer
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetWmParamsDisabled(fm_int          sw,
                                     fm_int          mtu, 
                                     fm_bool         defaultMaps, 
                                     fm10000_wmParam *wpm)
{
    fm_int          i;
    fm_int          cpi;
    fm_int          physPort;
    fm_int          logPort;
    fm_switch *     switchPtr;
    fm_uint32       globalWm;
    fm_status       err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);

    /* Initialize return code */
    err = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Set parameters asociated with specific registers resposible for the
       watermark schema configuration */

    /*************************************************************************/
    /* CM_GLOBAL_WM */
    /* Substract reserved segments */
    globalWm = FM10000_QOS_MAX_NUM_SEGS - FM10000_QOS_NUM_RESERVED_SEGS;
    /* and buffer for in-flight packets */
    globalWm = globalWm - switchPtr->numCardinalPorts * BYTES2SEG( mtu );
    FM_SET_FIELD(wpm->cmGlobalWm, 
                 FM10000_CM_GLOBAL_WM, 
                 watermark, 
                 globalWm);

    /*************************************************************************/
    /* CM_SHARED_WM */
    for (i = 0 ; i < FM10000_CM_SHARED_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSharedWm[i], FM10000_CM_SHARED_WM,
                     watermark, FM10000_QOS_WATERMARK_DEFAULT);
    }

    /*************************************************************************/
    /* CM_SOFTDROP_WM */
    for (i = 0 ; i < FM10000_CM_SOFTDROP_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSoftDropWm[i],
                     FM10000_CM_SOFTDROP_WM,
                     SoftDropSegmentLimit, 
                     FM10000_QOS_WATERMARK_DEFAULT);

        FM_SET_FIELD(wpm->cmSoftDropWm[i],
                     FM10000_CM_SOFTDROP_WM,
                     HogSegmentLimit, 
                     FM10000_QOS_WATERMARK_DEFAULT);
    }

    /*************************************************************************/
    /* CM_APPLY_SOFTDROP_CFG */
    for (i = 0 ; i < FM10000_CM_APPLY_SOFTDROP_CFG_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmApplySoftDropCfg[i],
                     FM10000_CM_APPLY_SOFTDROP_CFG,
                     JitterBits, 
                     0x0);
    }

    /*************************************************************************/
    /* CM_SHARED_SMP_PAUSE_WM .{PauseOn, PauseOff} */
    for (i = 0 ; i < FM10000_CM_SHARED_SMP_PAUSE_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSharedSmpPauseWm[i],
                     FM10000_CM_SHARED_SMP_PAUSE_WM,
                     PauseOn, FM10000_QOS_WATERMARK_DEFAULT);

        FM_SET_FIELD(wpm->cmSharedSmpPauseWm[i],
                     FM10000_CM_SHARED_SMP_PAUSE_WM,
                     PauseOff, FM10000_QOS_WATERMARK_DEFAULT);
    }

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /*********************************************************************/
        /* CM_RX_SMP_PRIVATE_WM */
        for (i = 0 ; 
             i < FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_0 ;
             i++)
        {
            FM_SET_FIELD(wpm->cmRxSmpPrivateWm[physPort][i], 
                         FM10000_CM_RX_SMP_PRIVATE_WM,
                         watermark, FM10000_QOS_WATERMARK_DEFAULT);
        }
        /*********************************************************************/
        /* CM_TX_TC_HOG_WM */
        for (i = 0 ; i < FM10000_CM_TX_TC_HOG_WM_ENTRIES_0 ; i++)
        {
            FM_SET_FIELD(wpm->cmTxTcHogWm[physPort][i],
                    FM10000_CM_TX_TC_HOG_WM,
                    watermark, 
                    FM10000_QOS_WATERMARK_DEFAULT);
        }
        /*********************************************************************/
        /* CM_TX_TC_PRIVATE_WM*/
        for (i = 0 ; i < FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_0 ; i++)
        {
            FM_SET_FIELD(wpm->cmTxTcPrivateWm[physPort][i],
                         FM10000_CM_TX_TC_PRIVATE_WM,
                         watermark, FM10000_QOS_WATERMARK_DEFAULT);
        }
        /*********************************************************************/
        /* CM_RX_SMP_PAUSE_WM.{PauseOn, PauseOff}*/
        for (i = 0 ;
             i < FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_0 ;
             i++)
        {
            FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][i],
                         FM10000_CM_RX_SMP_PAUSE_WM,
                         PauseOn, FM10000_QOS_WATERMARK_DEFAULT);

            FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][i],
                         FM10000_CM_RX_SMP_PAUSE_WM,
                         PauseOff, FM10000_QOS_WATERMARK_DEFAULT);
        }
        /*********************************************************************/
        /* CM_RX_SMP_HOG_WM */
        for (i = 0 ;
             i < FM10000_CM_RX_SMP_HOG_WM_ENTRIES_0 ;
             i++)
        {
            FM_SET_FIELD(wpm->cmRxSmpHogWm[physPort][i], 
                         FM10000_CM_RX_SMP_HOG_WM, 
                         watermark, FM10000_QOS_WATERMARK_DEFAULT);
        }
        /*********************************************************************/
        /* CM_APPLY_TX_SOFTDROP_CFG */
        for (i = 0 ; i < FM10000_CM_APPLY_TX_SOFTDROP_CFG_ENTRIES_0 ; i++)
        {
            FM_SET_BIT(wpm->cmApplyTxSoftDropCfg[physPort][i],
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG, 
                       SoftDropOnPrivate,
                       0);
        
            FM_SET_BIT(wpm->cmApplyTxSoftDropCfg[physPort][i],
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG, 
                       SoftDropOnSmpFree,
                       0);
        }
    }

    /*************************************************************************/
    /* Set default mapping if specified */
    /*************************************************************************/
    if (defaultMaps)
    {
        err = SetDefaultMaps(FM10000_QOS_SMP_DEFAULT, wpm);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end GetWmParamsDisabled */



/*****************************************************************************/
/** GetWmParamsLossy
 * \ingroup intQos
 *
 * \desc            Get the watermark parameters for the lossy scheme.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mtu is the largest MTU the switch will support in this
 *                  configuration.
 *
 * \param[in]       scalePercent is the percentage to scale the watermarks by.
 *                  Valid values range from 1 to 100 percent.  Some values may
 *                  produce a watermark configuration that is not valid 
 *                  (generally this happens when scaling down too much).
 *                  Scaled values should be analyzed for validity.
 *                 
 *  \param[in]      scaleBasedOn determines the methodology for scaling the
 *                  watermarks.  SCALE_TOTAL_MEM will scale the watermarks
 *                  based on the total available switch memory.
 *                  SCALE_GLOBAL_WM will scale based on the global watermark,
 *                  removing the repair/head storage area of memory from the
 *                  equation.  SCALE_GLOBAL_WM is generally used when
 *                  repair/head storage is already accounted for and the scheme
 *                  produced here will be merged into a larger scheme comprised
 *                  of multiple SMPs (ie: lossy and lossless).  In this case,
 *                  CM_GLOBAL_WM will be set correctly for the multiple SMP
 *                  watermark scheme (to fit within sizeInBytes) and 
 *                  globalThisSmp will be the scaled theoretical global 
 *                  watermark for this SMP.
 *                  
 * \param[in]       sizeInBytes specifies the total available memory in bytes
 *                  to be used when determining the global watermark.  When
 *                  scaling a single partition to use all available memory this
 *                  should be TOTAL_MEMORY_BYTES.  When scaling the lossy
 *                  partition for a multiple partition system, sizeInBytes
 *                  specifies the total amount of memory to use when selecting
 *                  the CM_GLOBAL_WM value for the system.  The rest of the
 *                  partition will be scaled under globalThisSmp according to 
 *                  scalePercent.
 *
 * \param[in]       defaultMaps determines whether to initialize the mapping
 *                  tables to their defaults or leave as is.
 *
 * \param[out]      wpm is the user allocated watermark parameters pointer.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if an invalid argument value is
 *                  specified.
 *
 *****************************************************************************/
static fm_status GetWmParamsLossy(fm_int          sw, 
                                  fm_int          mtu, 
                                  fm_int          scalePercent,
                                  fm_int          scaleBasedOn,
                                  fm_uint32       sizeInBytes,
                                  fm_bool         defaultMaps,
                                  fm10000_wmParam *wpm)
{
    fm_switch *             switchPtr;

    /* Loop control variables */
    fm_int                  i;
    fm_int                  smp;
    fm_int                  cpi;
    fm_int                  physPort;
    fm_int                  logPort;
    fm_int                  numPortsThisSmp;

    /* API attributes related variables */
    fm_uint32               desiredRxSmpPrivate;
    fm_uint32               desiredTxTcHogBytes;
    fm_int                  desiredSdVsHogPercent;
    fm_int                  desiredNumJitterBits;
    fm_int                  desiredSoftDropOnPrivate;
    fm_int                  desiredSoftDropOnSmpFree;

    /* Variables to temporarily store calculations */
    fm_status               err;
    fm_uint32               segLimit;
    fm_uint32               sharedSegLimit;
    fm_uint16               globalThisSmp;
    fm_uint16               rxSmpPrivate;
    fm_uint32               overshoot;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, 
            "sw=%d mtu=%d scalePercent=%d scaleBasedOn=%d sizeInBytes=%d\n", 
            sw, mtu, scalePercent, scaleBasedOn, sizeInBytes);

    /* Initialize return code */
    err = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Get API attributes */
    desiredRxSmpPrivate = GET_FM10000_PROPERTY()->cmRxSmpPrivBytes;

    desiredTxTcHogBytes = GET_FM10000_PROPERTY()->cmTxTcHogBytes;

    desiredSdVsHogPercent = GET_FM10000_PROPERTY()->cmSmpSdVsHogPercent;

    desiredNumJitterBits = GET_FM10000_PROPERTY()->cmSmpSdJitterBits;

    desiredSoftDropOnPrivate = GET_FM10000_PROPERTY()->cmTxSdOnPrivate ? 1 : 0;
    
    desiredSoftDropOnSmpFree = GET_FM10000_PROPERTY()->cmTxSdOnSmpFree ? 1 : 0;

    /* Set parameters asociated with specific registers resposible for the
       watermark schema configuration */

    FM_LOG_INFO(FM_LOG_CAT_QOS, " Lossy configuration based on: %s,"
                "scalePercent = %d,\n"
                "\t \t \t mtu = %d, whole avialable memory: %d bytes \n",
                (scaleBasedOn == FM10000_QOS_SCALE_TOTAL_MEM)?
                "Total memory" : "Global WM",
                scalePercent,
                mtu,
                sizeInBytes);

    /*************************************************************************/
    /* CM_GLOBAL_WM */
    if (scaleBasedOn == FM10000_QOS_SCALE_TOTAL_MEM)
    {
        if (sizeInBytes == FM10000_QOS_TOTAL_MEMORY_BYTES)
        {
            segLimit = FM10000_QOS_MAX_NUM_SEGS * scalePercent / 100 - 
                       FM10000_QOS_NUM_RESERVED_SEGS;
        }
        else
        {
            segLimit = 
                     ((sizeInBytes / BYTES_PER_SMP_SEG) * scalePercent / 100) -
                     FM10000_QOS_NUM_RESERVED_SEGS;
        }

        globalThisSmp = segLimit;
    }
    else if (scaleBasedOn == FM10000_QOS_SCALE_GLOBAL_WM)
    {
        if (sizeInBytes == FM10000_QOS_TOTAL_MEMORY_BYTES)
        {
            segLimit = FM10000_QOS_MAX_NUM_SEGS - FM10000_QOS_NUM_RESERVED_SEGS;
        }
        else
        {
            segLimit = sizeInBytes / BYTES_PER_SMP_SEG - 
                       FM10000_QOS_NUM_RESERVED_SEGS;
        }

        globalThisSmp = segLimit * scalePercent / 100;
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

    FM_SET_FIELD(wpm->cmGlobalWm, FM10000_CM_GLOBAL_WM, watermark, segLimit);

    FM10000_QOS_LOG_INFO("CM_GLOBAL_WM", segLimit);

    wpm->globalThisSmp = globalThisSmp;

    FM10000_QOS_LOG_INFO("GLOBAL THIS SMP", globalThisSmp);

    /*************************************************************************/
    /* CM_RX_SMP_PRIVATE_WM */
    segLimit = desiredRxSmpPrivate / BYTES_PER_SMP_SEG;
    rxSmpPrivate = segLimit;

    FM10000_QOS_LOG_INFO("CM_RX_SMP_PRIVATE_WM", segLimit);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ; 
             smp < FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_0 ;
             smp++)
        {
            FM_SET_FIELD(wpm->cmRxSmpPrivateWm[physPort][smp], 
                         FM10000_CM_RX_SMP_PRIVATE_WM,
                         watermark, segLimit);
        }
    }
    numPortsThisSmp = switchPtr->numCardinalPorts;

    /*************************************************************************/
    /* Overshoot and CM_SHARED_WM */
    overshoot = BYTES2SEG(numPortsThisSmp * mtu);

    FM10000_QOS_LOG_INFO("OVERSHOOT", overshoot);

    sharedSegLimit = (globalThisSmp - overshoot -
                      (rxSmpPrivate * numPortsThisSmp));

    FM10000_QOS_LOG_INFO("CM_SHARED_WM", sharedSegLimit);

    for (i = 0 ; i < FM10000_CM_SHARED_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSharedWm[i], FM10000_CM_SHARED_WM,
                     watermark, sharedSegLimit);
    }

    /*************************************************************************/
    /* CM_SOFTDROP_WM.HogSegmentLimit */
    if (scaleBasedOn == FM10000_QOS_SCALE_TOTAL_MEM)
    {
        segLimit = (FM_GET_FIELD(wpm->cmGlobalWm, FM10000_CM_GLOBAL_WM, 
                                 watermark) - 
                    ((numPortsThisSmp - 1) * rxSmpPrivate));
    }
    else if (scaleBasedOn == FM10000_QOS_SCALE_GLOBAL_WM)
    {
        segLimit = globalThisSmp - ((numPortsThisSmp - 1) * rxSmpPrivate);
    }

    /* If the SD hog is greater than shared seg, then SD has to be lowered */
    if (segLimit > sharedSegLimit)
    {
        segLimit = sharedSegLimit;
    }

    for (i = 0 ; i < FM10000_CM_SOFTDROP_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSoftDropWm[i], FM10000_CM_SOFTDROP_WM,
                     HogSegmentLimit, segLimit);
    }

    FM10000_QOS_LOG_INFO("CM_SOFTDROP_WM.HogSegmentLimit", segLimit);

    /*************************************************************************/
    /* CM_SOFT_DROP_WM.SoftDropSegmentLimit */
    segLimit = FM_GET_FIELD(wpm->cmSoftDropWm[0],
                            FM10000_CM_SOFTDROP_WM,
                            HogSegmentLimit) * 
               desiredSdVsHogPercent / 100;

    for (i = 0 ; i < FM10000_CM_SOFTDROP_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSoftDropWm[i], FM10000_CM_SOFTDROP_WM,
                     SoftDropSegmentLimit, segLimit);
    }

    FM10000_QOS_LOG_INFO("CM_SOFTDROP_WM.SoftDropSegmentLimit", segLimit);

    /*************************************************************************/
    /* CM_APPLY_SOFTDROP_CFG.JitterBits */
    for (i = 0 ; i < FM10000_CM_APPLY_SOFTDROP_CFG_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmApplySoftDropCfg[i],
                     FM10000_CM_APPLY_SOFTDROP_CFG,
                     JitterBits, 
                     desiredNumJitterBits);
    }

    FM_LOG_INFO(FM_LOG_CAT_QOS, " %-45s      %7d \n",
                "CM_APPLY_SOFTDROP_CFG.JitterBits",
                desiredNumJitterBits);

    /*************************************************************************/
    /* CM_TX_TC_HOG_WM */
    segLimit = desiredTxTcHogBytes / BYTES_PER_SMP_SEG;

    FM10000_QOS_LOG_INFO("CM_TX_TC_HOG_WM", segLimit);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (i = 0 ; i < FM10000_CM_TX_TC_HOG_WM_ENTRIES_0 ; i++)
        {
            FM_SET_FIELD(wpm->cmTxTcHogWm[physPort][i],
                         FM10000_CM_TX_TC_HOG_WM,
                         watermark, segLimit);
        }
    }

    /*************************************************************************/
    /* CM_TX_TC_PRIVATE_WM */
    segLimit = FM_GET_FIELD(wpm->cmSoftDropWm[0], 
                            FM10000_CM_SOFTDROP_WM,
                            SoftDropSegmentLimit) /
               (numPortsThisSmp * FM_MAX_TRAFFIC_CLASSES);
    
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (i = 0 ; i < FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_0 ; i++)
        {
            FM_SET_FIELD(wpm->cmTxTcPrivateWm[physPort][i],
                         FM10000_CM_TX_TC_PRIVATE_WM,
                         watermark, segLimit);
        }
    }

    FM10000_QOS_LOG_INFO("CM_TX_TC_PRIVATE_WM", segLimit);

    /*************************************************************************/
    /* CM_APPLY_TX_SOFTDROP_CFG */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (i = 0 ; i < FM10000_CM_APPLY_TX_SOFTDROP_CFG_ENTRIES_0 ; i++)
        {
            FM_SET_BIT(wpm->cmApplyTxSoftDropCfg[physPort][i],
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG, 
                       SoftDropOnPrivate,
                       desiredSoftDropOnPrivate);

            FM_SET_BIT(wpm->cmApplyTxSoftDropCfg[physPort][i],
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG, 
                       SoftDropOnSmpFree,
                       desiredSoftDropOnSmpFree);
        }
    }

    FM_LOG_INFO(FM_LOG_CAT_QOS, " %-45s      %7s \n",
                "CM_APPLY_TX_SOFTDROP_CFG.SoftDropOnPrivate",
                (desiredSoftDropOnPrivate == 0)?"OFF" : "ON");
    FM_LOG_INFO(FM_LOG_CAT_QOS, " %-45s      %7s \n",
                "CM_APPLY_TX_SOFTDROP_CFG.SoftDropOnSmpFree",
                (desiredSoftDropOnSmpFree == 0)?"OFF" : "ON");

    /*************************************************************************/
    /* Set values (to default) that aren't specified in this configuration */

    /*************************************************************************/
    /* CM_RX_SMP_PAUSE_WM.{PauseOn, PauseOff}*/
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ;
             smp < FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_0 ;
             smp++)
        {
            FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][smp],
                         FM10000_CM_RX_SMP_PAUSE_WM,
                         PauseOn, 0x7fff);

            FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][smp],
                         FM10000_CM_RX_SMP_PAUSE_WM,
                         PauseOff, 0x7fff);
        }
    }

    /*************************************************************************/
    /* CM_RX_SMP_HOG_WM */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ;
             smp < FM10000_CM_RX_SMP_HOG_WM_ENTRIES_0 ;
             smp++)
        {
            FM_SET_FIELD(wpm->cmRxSmpHogWm[physPort][smp], 
                         FM10000_CM_RX_SMP_HOG_WM, 
                         watermark, 0x7fff);
        }
    }

    /*************************************************************************/
    /* CM_SHARED_SMP_PAUSE_WM .{PauseOn, PauseOff} */
    for (smp = 0 ; smp < FM10000_CM_SHARED_SMP_PAUSE_WM_ENTRIES ; smp++)
    {
        FM_SET_FIELD(wpm->cmSharedSmpPauseWm[smp],
                     FM10000_CM_SHARED_SMP_PAUSE_WM,
                     PauseOn, 0x7fff);

        FM_SET_FIELD(wpm->cmSharedSmpPauseWm[smp],
                     FM10000_CM_SHARED_SMP_PAUSE_WM,
                     PauseOff, 0x7fff);
    }

    /*************************************************************************/
    /* Set default mapping if specified */
    /*************************************************************************/
    if (defaultMaps)
    {
        err = SetDefaultMaps(FM10000_QOS_SMP_LOSSY, wpm);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end GetWmParamsLossy */




/*****************************************************************************/
/** GetWmParamsLossless
 * \ingroup intQos
 *
 * \desc            Get the watermark parameters for the lossless scheme.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mtu is the largest MTU the switch will support in this
 *                  configuration.
 *
 * \param[in]       singlePartition indicates whether sizeInBytes is specified
 *                  as a single SMP configuration (TRUE) or a dual SMP
 *                  configuration (FALSE).
 *
 * \param[in]       sizeInBytes is the size (in bytes) to fit this watermark
 *                  scheme into, or TOTAL_MEMORY_BYTES for a single RXMP 
 *                  configuration.  For a dual SMP configuration, this
 *                  would be ((CM_GLOBAL_WM - globalThisSmp) * BYTES_PER_SEG)
 *                  from the lossy function.
 * 
 * \param[in]       defaultMaps determines whether to initialize the mapping
 *                  tables to their defaults or leave as is.
 *
 * \param[out]      wpm is the user allocated watermark parameters pointer
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetWmParamsLossless(fm_int         sw, 
                                     fm_int         mtu, 
                                     fm_bool        singlePartition,
                                     fm_uint32      sizeInBytes,
                                     fm_bool        defaultMaps,
                                     fm10000_wmParam *wpm)
{
    fm_switch *             switchPtr;
    fm_uint32               smpPauseEn[FM10000_MAX_PORT + 1];

    /* Loop control variables */
    fm_int                  i;
    fm_int                  smp;
    fm_status               err;
    fm_int                  cpi;
    fm_int                  physPort;
    fm_int                  logPort;

    /* API attributes related variable */
    fm_int                  desiredPauseBufferBytes;

    /* Variables to temporarily store calculations */
    fm_uint32               segLimit;
    fm_uint16               globalThisSmp;
    fm_uint16               overshoot;
    fm_uint32               sumOfSmpPrivate;
    fm_uint16               pauseBuffer;
    fm_uint16               sharedSmpWm;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, 
                 "sw=%d mtu=%d singlePartition=%d sizeInBytes=%d\n", 
                 sw, mtu, singlePartition, sizeInBytes);

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Get API attributes */
    desiredPauseBufferBytes = GET_FM10000_PROPERTY()->cmPauseBufferBytes;

    /* Get bitmask of ouase enabled for lossless smp */
    err = fm10000GetSMPPauseState(sw, smpPauseEn);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_LOG_INFO(FM_LOG_CAT_QOS, " Lossless configuration "
                "mtu = %d, avialable memory: %d bytes \n",
                mtu,
                sizeInBytes);

    /*************************************************************************/
    /* CM_GLOBAL_WM */
    if (singlePartition)
    {
        if (sizeInBytes == FM10000_QOS_TOTAL_MEMORY_BYTES)
        {
            segLimit = FM10000_QOS_MAX_NUM_SEGS - FM10000_QOS_NUM_RESERVED_SEGS;
        }
        else
        {
            segLimit = sizeInBytes / BYTES_PER_SMP_SEG - 
                       FM10000_QOS_NUM_RESERVED_SEGS;
        }
    }
    else
    {
        segLimit = sizeInBytes / BYTES_PER_SMP_SEG;
    }

    globalThisSmp = segLimit;
    overshoot = BYTES2SEG(switchPtr->numCardinalPorts * mtu);
    FM_SET_FIELD(wpm->cmGlobalWm, FM10000_CM_GLOBAL_WM, watermark, segLimit);
    FM10000_QOS_LOG_INFO("CM_GLOBAL_WM", segLimit);

    wpm->globalThisSmp = globalThisSmp;
    FM10000_QOS_LOG_INFO("GLOBAL THIS SMP", globalThisSmp);

    FM10000_QOS_LOG_INFO("OVERSHOOT", overshoot);

    /*************************************************************************/
    /* CM_RX_SMP_PRIVATE_WM */
    sumOfSmpPrivate = 0;
    segLimit = 0;

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ; 
             smp < FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_0 ;
             smp++)
        {
            FM_SET_FIELD(wpm->cmRxSmpPrivateWm[physPort][smp], 
                         FM10000_CM_RX_SMP_PRIVATE_WM,
                         watermark, segLimit);
        }
    }

    FM10000_QOS_LOG_INFO("CM_RX_SMP_PRIVATE_WM", segLimit);

    /*************************************************************************/
    /* CM_SHARED_WM */
    segLimit = globalThisSmp - overshoot - sumOfSmpPrivate;

    for (smp = 0 ; smp < FM10000_CM_SHARED_WM_ENTRIES ; smp++)
    {
        FM_SET_FIELD(wpm->cmSharedWm[smp], FM10000_CM_SHARED_WM, 
                     watermark, segLimit);
    }

    FM10000_QOS_LOG_INFO("CM_SHARED_WM", segLimit);

    sharedSmpWm = segLimit;

    /*************************************************************************/
    /* CM_SOFT_DROP_WM.HogSegmentLimit */
    for (i = 0 ; i < FM10000_CM_SOFTDROP_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSoftDropWm[i],
                     FM10000_CM_SOFTDROP_WM,
                     HogSegmentLimit, 
                     segLimit);
    }

    FM10000_QOS_LOG_INFO("CM_SOFTDROP_WM.HogSegmentLimit", segLimit);

    /*************************************************************************/
    /* CM_SHARED_SMP_PAUSE_WM.{PauseOn, PauseOff}*/
    pauseBuffer = desiredPauseBufferBytes / BYTES_PER_SMP_SEG;

    segLimit = sharedSmpWm - (switchPtr->numCardinalPorts * pauseBuffer);

    for (smp = 0 ; smp < FM10000_CM_SHARED_SMP_PAUSE_WM_ENTRIES ; smp++)
    {
        FM_SET_FIELD(wpm->cmSharedSmpPauseWm[smp],
                     FM10000_CM_SHARED_SMP_PAUSE_WM, 
                     PauseOn, segLimit);

        FM_SET_FIELD(wpm->cmSharedSmpPauseWm[smp],
                     FM10000_CM_SHARED_SMP_PAUSE_WM, 
                     PauseOff, (segLimit / 2));
    }

    FM10000_QOS_LOG_INFO("CM_SHARED_SMP_PAUSE_WM.PauseOn", segLimit);
    FM10000_QOS_LOG_INFO("CM_SHARED_SMP_PAUSE_WM.PauseOff", (segLimit / 2));

    /*************************************************************************/
    /* CM_RX_SMP_PAUSE_WM.{PauseOn, PauseOff}*/
    segLimit = (sharedSmpWm / switchPtr->numCardinalPorts) - pauseBuffer;

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ; 
             smp < FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_0 ; 
             smp++)
        {
            if ((smpPauseEn[cpi] & (1 << smp)))
            {
                /* Set limits for Pause ON/OFF */
                FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][smp], 
                             FM10000_CM_RX_SMP_PAUSE_WM, 
                             PauseOn, segLimit);

                FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][smp], 
                             FM10000_CM_RX_SMP_PAUSE_WM, 
                             PauseOff, (segLimit / 2));
            }
            else
            {
                /* Disable Pause */
                FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][smp], 
                             FM10000_CM_RX_SMP_PAUSE_WM, 
                             PauseOn, FM10000_QOS_WATERMARK_DEFAULT);

                FM_SET_FIELD(wpm->cmRxSmpPauseWm[physPort][smp], 
                             FM10000_CM_RX_SMP_PAUSE_WM, 
                             PauseOff, FM10000_QOS_WATERMARK_DEFAULT);
            }
        }
    }

    FM10000_QOS_LOG_INFO("CM_RX_SMP_PAUSE_WM.PauseOn", segLimit);
    FM10000_QOS_LOG_INFO("CM_RX_SMP_PAUSE_WM.PauseOff", (segLimit / 2));

    /*************************************************************************/
    /* CM_RX_SMP_HOG_WM */
    segLimit = FM_GET_FIELD(wpm->cmSharedSmpPauseWm[0],
                            FM10000_CM_SHARED_SMP_PAUSE_WM,
                            PauseOn) + pauseBuffer;

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (smp = 0 ; smp < FM10000_CM_RX_SMP_HOG_WM_ENTRIES_0 ; smp++)
        {
            /* Port Pause enabled */
            FM_SET_FIELD(wpm->cmRxSmpHogWm[physPort][smp],
                         FM10000_CM_RX_SMP_HOG_WM,
                         watermark, segLimit);
        }
    }

    FM10000_QOS_LOG_INFO("CM_RX_SMP_HOG_WM", segLimit);

    /*************************************************************************/
    /* Set registers that aren't specified, disabled */

    /*************************************************************************/
    /* CM_TX_TC_HOG_WM */
  
    FM10000_QOS_LOG_INFO("CM_TX_TC_HOG_WM", segLimit);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (i = 0 ; i < FM10000_CM_TX_TC_HOG_WM_ENTRIES_0 ; i++)
        {
            FM_SET_FIELD(wpm->cmTxTcHogWm[physPort][i],
                    FM10000_CM_TX_TC_HOG_WM,
                    watermark, 
                    0x7fff);
        }
    }

    /*************************************************************************/
    /* CM_RXMP_SOFT_DROP_WM */
    for (i = 0 ; i < FM10000_CM_SOFTDROP_WM_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmSoftDropWm[i],
                     FM10000_CM_SOFTDROP_WM,
                     SoftDropSegmentLimit, 
                     0x7fff);
    }

    /*************************************************************************/
    /* CM_APPLY_SOFTDROP_CFG */
    for (i = 0 ; i < FM10000_CM_APPLY_SOFTDROP_CFG_ENTRIES ; i++)
    {
        FM_SET_FIELD(wpm->cmApplySoftDropCfg[i],
                     FM10000_CM_APPLY_SOFTDROP_CFG,
                     JitterBits, 
                     0x0);
    }

    /*************************************************************************/
    /* CM_TX_TC_PRIVATE_WM*/
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (i = 0 ; i < FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_0 ; i++)
        {
            FM_SET_FIELD(wpm->cmTxTcPrivateWm[physPort][i],
                         FM10000_CM_TX_TC_PRIVATE_WM,
                         watermark, 0x0);
        }
    }

    /*************************************************************************/
    /* CM_APPLY_TX_SOFTDROP_CFG */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        for (i = 0 ; i < FM10000_CM_APPLY_TX_SOFTDROP_CFG_ENTRIES_0 ; i++)
        {
            FM_SET_BIT(wpm->cmApplyTxSoftDropCfg[physPort][i],
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG, 
                       SoftDropOnPrivate,
                       0);

            FM_SET_BIT(wpm->cmApplyTxSoftDropCfg[physPort][i],
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG, 
                       SoftDropOnSmpFree,
                       0);
        }
    }

    /*************************************************************************/
    /* Set default mapping if specified */
    /*************************************************************************/
    if (defaultMaps)
    {
        err = SetDefaultMaps(FM10000_QOS_SMP_LOSSLESS, wpm);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end GetWmParamsLossless */




/*****************************************************************************/
/** GetWmParamsLossyLossless
 * \ingroup intQos
 *
 * \desc            Get the watermark parameters for the lossy and lossless
 *                  scheme.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mtu is the largest MTU the switch will support in this
 *                  configuration.
 *
 * \param[in]       defaultMaps determines whether to initialize the mapping
 *                  tables to their defaults or leave as is.
 * 
 * \param[out]      wpm is the user allocated watermark parameters pointer
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetWmParamsLossyLossless(fm_int            sw, 
                                          fm_int            mtu, 
                                          fm_bool           defaultMaps,
                                          fm10000_wmParam *  wpm)
{
    /* Pointers to temp watermark schama */
    fm10000_wmParam *       wpmLossless;

    /* Loop control variables */
    fm_int                  logPort;
    fm_int                  physPort;
    fm_int                  cpi;

    /* Variables for calculations/etc */
    fm_switch *             switchPtr;
    fm_uint32               sizeInBytes;
    fm_uint32               globalThisSmpLossy;
    fm_uint32               globalThisSmpLossless;
    fm_uint32               swPriIdx;
    fm_uint32               tcIdx;
    fm_uint32               targetSmp;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, 
                 "sw=%d mtu=%d defaultMaps=%d\n", 
                 sw, mtu, defaultMaps);

    /* Initialize return value */
    err = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Input configuration will be used to store Lossy configuration 
       just check if input pointer is not NULL*/
    if (wpm == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_INVALID_ARGUMENT);
    }

    /* Allocate memory for Lossless configuration */
    wpmLossless = fmAlloc(sizeof(fm10000_wmParam));
    if (wpmLossless == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_NO_MEM);
    }

    /* Get Lossy configuration */
    GetWmParamsLossy(sw, mtu, 50, FM10000_QOS_SCALE_GLOBAL_WM, 
                     FM10000_QOS_TOTAL_MEMORY_BYTES, defaultMaps, wpm);

    /* Calculate size for lossless configuration */
    sizeInBytes = ( (FM_GET_FIELD(wpm->cmGlobalWm, FM10000_CM_GLOBAL_WM, 
                                  watermark) * BYTES_PER_SMP_SEG) - 
                     wpm->globalThisSmp * BYTES_PER_SMP_SEG);

    globalThisSmpLossy = wpm->globalThisSmp;

    /* Get Lossless configuration */
    GetWmParamsLossless(sw, mtu, FALSE, sizeInBytes, defaultMaps, wpmLossless);

    globalThisSmpLossless = wpmLossless->globalThisSmp;

    /* Merge lossless configuration into lossy configuration */

    /* CM_GLOBAL_WM: adjust global waterark as a sum of globalThisSmp */
    FM_SET_FIELD(wpm->cmGlobalWm, FM10000_CM_GLOBAL_WM, watermark,
                 (globalThisSmpLossy + globalThisSmpLossless));

    /* CM_SHARED_SMP_PAUSE_WM .{PauseOn, PauseOff} */
     wpm->cmSharedSmpPauseWm[FM10000_QOS_SMP_LOSSLESS] = 
        wpmLossless->cmSharedSmpPauseWm[FM10000_QOS_SMP_LOSSLESS];

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* CM_RX_SMP_PRIVATE_WM */
        wpm->cmRxSmpPrivateWm[physPort][FM10000_QOS_SMP_LOSSLESS] = 
            wpmLossless->cmRxSmpPrivateWm[physPort][FM10000_QOS_SMP_LOSSLESS];

        /* CM_RX_SMP_PAUSE_WM */
        wpm->cmRxSmpPauseWm[physPort][FM10000_QOS_SMP_LOSSLESS] = 
            wpmLossless->cmRxSmpPauseWm[physPort][FM10000_QOS_SMP_LOSSLESS];

        /* CM_RX_SMP_HOG_WM */
        wpm->cmRxSmpHogWm[physPort][FM10000_QOS_SMP_LOSSLESS] = 
            wpmLossless->cmRxSmpHogWm[physPort][FM10000_QOS_SMP_LOSSLESS];
    }

    if (defaultMaps)
    {
        /* Set default mapping if specified */
        err = SetDefaultMaps(FM10000_QOS_SMP_LOSSY, wpm);
    }
    else
    {
        /* Read mapping from registers - store results in wpm */
        err = ReadCmRegMaps(sw, wpm);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

    /* Continue configuration merge */
    /* Go through settings indexed by switch priority */
    for (swPriIdx = 0 ; swPriIdx <= FM10000_QOS_MAX_SW_PRI ; swPriIdx++)
    {
        fm_uint32 trafficClass;

        /* Get traffic class */
        trafficClass = 
            (fm_uint32)FM_GET_UNNAMED_FIELD64(wpm->cmSwPriTcMap,
                      (swPriIdx * FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc),
                      FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc);

        /* Get target SMP */
        targetSmp = FM_GET_UNNAMED_FIELD(wpm->cmTcSmpMap, trafficClass, 1);

        /* Get lossless settings if SMP mapped to it */
        if (targetSmp == FM10000_QOS_SMP_LOSSLESS)
        {
            /* CM_SHARED_WM */
            wpm->cmSharedWm[swPriIdx] = 
                wpmLossless->cmSharedWm[swPriIdx];

            /* CM_SOFTDROP_WM */
            wpm->cmSoftDropWm[swPriIdx] = 
                wpmLossless->cmSoftDropWm[swPriIdx];

            /* CM_SOFTDROP_CFG */
            wpm->cmApplySoftDropCfg[swPriIdx] = 
                wpmLossless->cmSoftDropWm[swPriIdx];

            for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
            {
                err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, 
                                                &physPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

                /* CM_APPLY_TX_SOFTDROP_CFG */
                wpm->cmApplyTxSoftDropCfg[physPort][swPriIdx] = 
                    wpmLossless->cmApplyTxSoftDropCfg[physPort][swPriIdx];
            }
        }
    }

    /* Go through settings indexed by traffic classes */
    for (tcIdx = 0 ; tcIdx <= FM10000_QOS_MAX_TC ; tcIdx++)
    {
        /* Get target SMP */
        targetSmp = FM_GET_UNNAMED_FIELD(wpm->cmTcSmpMap, tcIdx, 1);

        /* Get lossless settings if SMP mapped to it */
        if (targetSmp == FM10000_QOS_SMP_LOSSLESS)
        {
            for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
            {
                err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, 
                                                &physPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    
                /* CM_TX_TC_HOG_WM */
                wpm->cmTxTcHogWm[physPort][tcIdx] = 
                    wpmLossless->cmTxTcHogWm[physPort][tcIdx];
    
                /* CM_TX_TC_PRIVATE_WM */
                wpm->cmTxTcPrivateWm[physPort][tcIdx] = 
                    wpmLossless->cmTxTcPrivateWm[physPort][tcIdx];
            }
        }
    }

ABORT:
    /* Free temp memory allocated for lossless config */
    fmFree(wpmLossless);

    FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_OK);

}   /* end GetWmParamsLossyLossless */




/*****************************************************************************/
/** GetTrapClassMap
 * \ingroup intQos
 *
 * \desc            Get switch priority mapping for the specified trap class.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       trapClass is the class for which switch priority is to get 
 *
 * \param[in]       priority is the switch priority with which the specified
 *                  trap class is associated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetTrapClassMap(fm_int sw, fm_int trapClass, fm_uint *priority)
{
    fm_status           status = FM_ERR_NOT_FOUND;
    fm_int              mapper;
    fm_int              intTrapClass;

    /* Convert trap class to internal trap class */
    status = PriorityMapperTrapClassConvert(trapClass, &intTrapClass);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    status = PriorityMapperSearchByClass(sw, intTrapClass, &mapper);

    if (status == FM_OK)
    {
        /* Mapper found, set output */
        status = PriorityMapperAttributeGet(sw,
                                            mapper,
                                            FM10000_PRI_MAPPER_SWPRI,
                                            (void *) priority);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end GetTrapClassMap */




/*****************************************************************************/
/** SetTrapClassMap
 * \ingroup intQos
 *
 * \desc            Associates the specified frame class with the specified
 *                  switch priority.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       trapClass is the class that is to be associated 
 *                  with the specified switch priority.
 *
 * \param[in]       priority is the switch priority with which the specified
 *                  frame class is to be associated with.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetTrapClassMap(fm_int sw, fm_int trapClass, fm_uint priority)
{
    fm_status           status = FM_OK;
    fm_uint             mapCount;
    fm_int              mapper;
    fm_int              intTrapClass;
    fm10000_priorityMap map;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d trapClass=%d priority=%u\n",
                 sw,
                 trapClass,
                 priority);

    FM_TAKE_STATE_LOCK(sw);

    /* Convert trap class to internal trap class */
    status = PriorityMapperTrapClassConvert(trapClass, &intTrapClass);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    map.trapClass = intTrapClass;
    map.priority   = priority;

    if (priority == FM_QOS_SWPRI_DEFAULT)
    {
        /* Search mapper for specified trapClass */
        status = PriorityMapperSearchByClass(sw, map.trapClass, &mapper);

        if (status == FM_OK)
        {
            /* Mapper found - get priority associated with the mapper */
            status = PriorityMapperAttributeGet(sw,
                                                mapper,
                                                FM10000_PRI_MAPPER_SWPRI,
                                                (void *) &(map.priority));
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

            /* Delete map from the mapper. */
            status = PriorityMapperDeleteMap(sw, mapper, &map);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

             /* Free the priority mapper with no more priority maps. */
            status =
                PriorityMapperAttributeGet(sw,
                                           mapper,
                                           FM10000_PRI_MAPPER_MAP_COUNT,
                                           (void *) &mapCount);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

            if (mapCount == 0)
            {
                status = PriorityMapperFree(sw, mapper);
            }

        }
        else if (status == FM_ERR_NOT_FOUND)
        {
            /* Mapper not found - the frame class is valid but has not been
               assigned to a priority mapper yet */
            status = FM_OK;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    }
    else if ( ((fm_int)priority >= 0 ) && 
              (priority < FM_SWITCH_PRIORITY_MAX) )
    {
        /* Get the mapper for specified priority */
        status = PriorityMapperGet(sw, priority, &mapper);
        
        if (status == FM_ERR_NOT_FOUND)
        {
            /* Allocate mapper because no mapper has yet been created for 
               the priority. */
            status = PriorityMapperAllocate(sw, map.trapClass, &mapper);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

            /* Set the priority for allocated mapper */
            status = PriorityMapperAttributeSet(sw,
                                                mapper,
                                                FM10000_PRI_MAPPER_SWPRI,
                                                (void *) &priority);
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

        /* Add the specified priority map to the priority mapper. */
        status = PriorityMapperAddMap(sw, mapper, &map);
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
    }

ABORT:
    FM_DROP_STATE_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end SetTrapClassMap */




/*****************************************************************************/
/** PriorityMapperAllocate
 * \ingroup intQoS
 *
 * \desc            Allocates a new priority mapper.
 *
 * \note            The state lock must be held by the caller.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trapClass is ignored.
 *
 * \param[out]      mapper points to the location to receive the priority
 *                  mapper ID.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_PRIORITY_MAPPER if no more priority mappers
 *                  can be allocated.
 *
 *****************************************************************************/
static fm_status PriorityMapperAllocate(fm_int sw,
                                        fm_int trapClass,
                                        fm_int *mapper)
{
    fm_switch *             switchPtr;
    fm_status               status = FM_ERR_NO_FREE_PRIORITY_MAPPER;
    fm10000_switch *        switchExt;
    fm10000_priorityMapper  *priorityMapper;
    fm10000_priorityMapSet  *priorityMapSet;

    FM_NOT_USED(trapClass);

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d mapper=%p\n",
                 sw,
                 (void *) mapper);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    priorityMapSet = switchExt->priorityMapSet;

    FM10000_PRIORITY_MAPPER_ENTRY_POP(&(priorityMapSet->mapperPool), 
                                      priorityMapper);

    if (priorityMapper != NULL)
    {
        priorityMapper->priority = FM_QOS_SWPRI_DEFAULT;
        priorityMapper->mapCount = 0;
        priorityMapper->used     = TRUE;
        *mapper = priorityMapper->id;
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperAllocate */




/*****************************************************************************/
/** PriorityMapperFree
 * \ingroup intQoS
 *
 * \desc            Frees the specified priority mapper.
 *
 * \note            The caller must hold the state lock or the switch lock
 *                  as a writer (for exclusive access to the switch).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the priority mapper to be freed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PRIORITY_MAPPER if an invalid priority
 *                  mapper has been specified.
 *
 *****************************************************************************/
static fm_status PriorityMapperFree(fm_int sw, fm_int mapper)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_OK;
    fm10000_internalPriorityMap  *nextMap;
    fm10000_internalPriorityMap  *priorityMap;
    fm10000_switch *             switchExt;
    fm10000_priorityMapper *     priorityMapper;
    fm10000_priorityMapSet *     priorityMapSet;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d mapper=%d\n",
                 sw,
                 mapper);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    FM10000_PRIORITY_MAPPER_VALIDATE(switchExt, mapper, status);

    priorityMapSet = switchExt->priorityMapSet;

    priorityMapper = &(priorityMapSet->mappers[mapper]);

    /***************************************************
     * Return all priority maps to the priority map
     * pool.
     **************************************************/
    priorityMap = 
                FM10000_PRIORITY_MAPPER_FIRST_MAP_GET(&(priorityMapper->maps));

    while (priorityMap != NULL)
    {
        nextMap = FM10000_PRIORITY_MAPPER_NEXT_MAP_GET(priorityMap,
                                     FM10000_PRI_MAP_LIST_MAPPER);

        FM10000_PRIORITY_MAPPER_ENTRY_MAP_REMOVE(&(priorityMapper->maps),
                                                 FM10000_PRI_MAP_LIST_MAPPER,
                                                 priorityMap);

        priorityMap->mapper   = FM10000_INVALID_PRIORITY_MAPPER;
        priorityMap->priority = FM_QOS_SWPRI_DEFAULT;

        priorityMap = nextMap;

    }

    /* Remove trigger for Priority Mapper if not removed earlier */
    if (TRUE == priorityMapper->mapTrigCreated)
    {
        status = fm10000DeleteTrigger(sw,
                                      FM10000_TRIGGER_GROUP_PRIORITY_MAP,
                                      priorityMapper->id,
                                      TRUE);
        if (status != FM_OK)
        {
            /* Just log the error and continue without error */
            FM_LOG_ERROR(FM_LOG_CAT_QOS,
                         "Fail to free trigger for priority %d: %s\n",
                         priorityMapper->priority,
                         fmErrorMsg(status));
            status = FM_OK;
        }
    }

    /***************************************************
     * Mark the priority mapper as not-in-use.
     **************************************************/
    priorityMapper->used = FALSE;

    /***************************************************
     * Return the priority mapper to the priority mapper
     * pool.
     **************************************************/
    FM10000_PRIORITY_MAPPER_ENTRY_PUSH(&(priorityMapSet->mapperPool), 
                                                                priorityMapper);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperFree */




/*****************************************************************************/
/** PriorityMapperAddMap
 * \ingroup intQos
 *
 * \desc            Adds the specified priority map to the specified priority
 *                  mapper. If the priority map has been added to a different
 *                  priority mapper previously, it will be moved to the
 *                  specified priority mapper.
 *
 * \note            Caller must hold state lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the priority mapper the specified priority map is
 *                  to be added to.
 *
 * \param[in]       map points to the priority map that is to be added to the
 *                  specified priority mapper.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PRIORITY_MAPPER if an invalid priority
 *                  mapper has been specified.
 * \return          FM_ERR_SWPRI_UNASSIGNED if no switch priority has been
 *                  assigned to the priority mapper.
 * \return          FM_ERR_INVALID_PRIORITY_MAP if the priority map either
 *                  represents an unrecognized frame class or contains a switch
 *                  priority that does not match with the priority mapper's
 *                  assigned switch priority.
 *
 *****************************************************************************/
static fm_status PriorityMapperAddMap(fm_int               sw,
                                      fm_int               mapper,
                                      fm10000_priorityMap *map)
{
    fm_switch *                  switchPtr;
    fm_status                    status;
    fm10000_internalPriorityMap *priorityMap;
    fm10000_priorityMap          previousMap;
    fm10000_priorityMapper *     priorityMapper;
    fm10000_switch *             switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d mapper=%d map=%p\n",
                 sw,
                 mapper,
                 (void *) map);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    /* Validate the mapper */
    FM10000_PRIORITY_MAPPER_VALIDATE(switchExt, mapper, status);

    /* Get the priority mapper  and validate its priority  */
    priorityMapper = &(switchExt->priorityMapSet->mappers[mapper]);
    if (priorityMapper->priority == FM_QOS_SWPRI_DEFAULT)
    {
        status = FM_ERR_SWPRI_UNASSIGNED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
    }

     /* Determine whether the specified priority map is a valid priority map.*/
    if (map->priority != priorityMapper->priority)
    {
        status = FM_ERR_INVALID_PRIORITY_MAP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
    }

    /* Find the map */
    status = PriorityMapperFindMap(sw, map->trapClass, &priorityMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    if (priorityMap->mapper == priorityMapper->id)
    {
         /* The priority map is already part of the specified mapper.*/
        status = FM_OK;
        goto ABORT;
    }
    else if (priorityMap->mapper != FM10000_INVALID_PRIORITY_MAPPER)
    {
        /* The priority map is associated with a different priority mapper.*/
        previousMap.trapClass = map->trapClass;

        status =
            PriorityMapperAttributeGet(sw,
                                       priorityMap->mapper,
                                       FM10000_PRI_MAPPER_SWPRI,
                                       (void *) &(previousMap.priority));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

        status = PriorityMapperDeleteMap(sw,
                                         priorityMap->mapper,
                                         &previousMap);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    }

     /* Associate the priority map with the specified priority mapper. */
    priorityMap->mapper   = priorityMapper->id;
    priorityMap->priority = priorityMapper->priority;

    FM10000_PRIORITY_MAPPER_ENTRY_MAP_PUSH(&(priorityMapper->maps),
                                           FM10000_PRI_MAP_LIST_MAPPER,
                                           priorityMap);

    status = PriorityMapperApplyMapSet(sw, priorityMap, TRUE);

    if (status != FM_OK)
    {
        FM10000_PRIORITY_MAPPER_ENTRY_MAP_REMOVE(&(priorityMapper->maps),
                                                 FM10000_PRI_MAP_LIST_MAPPER,
                                                 priorityMap);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
    }

    priorityMapper->mapCount += 1;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperAddMap */




/*****************************************************************************/
/** PriorityMapperDeleteMap
 * \ingroup intQoS
 *
 * \desc            Deletes the specified priority map from the specified
 *                  priority mapper.
 *
 * \note            Caller must hold state lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the priority mapper from which to delete the
 *                  specified priority map.
 *
 * \param[in]       map points to the priority map to be deleted from the
 *                  specified priority mapper.
 *
 * \result          FM_OK if successful.
 * \return          FM_ERR_SWPRI_UNASSIGNED if no switch priority has been
 *                  assigned to the priority mapper.
 * \return          FM_ERR_INVALID_PRIORITY_MAPPER if an invalid priority
 *                  mapper has been specified.
 * \result          FM_ERR_INVALID_PRIORITY_MAP if the priority map is not
 *                  associated with the priority mapper.
 *
 *****************************************************************************/
static fm_status PriorityMapperDeleteMap(fm_int               sw,
                                         fm_int               mapper,
                                         fm10000_priorityMap *map)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_ERR_INVALID_PRIORITY_MAP;
    fm10000_internalPriorityMap *priorityMap;
    fm10000_priorityMapper *     priorityMapper;
    fm10000_switch *             switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d mapper=%d map=%p\n",
                 sw,
                 mapper,
                 (void *) map);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    FM10000_PRIORITY_MAPPER_VALIDATE(switchExt, mapper, status);

    priorityMapper = &(switchExt->priorityMapSet->mappers[mapper]);

    if (priorityMapper->priority == FM_QOS_SWPRI_DEFAULT)
    {
        status = FM_ERR_SWPRI_UNASSIGNED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
    }

    priorityMap =
        FM10000_PRIORITY_MAPPER_FIRST_MAP_GET(&(priorityMapper->maps));

    while (priorityMap != NULL)
    {
        if (   (priorityMap->trapClass == map->trapClass)
            && (priorityMap->priority == map->priority))
        {
            FM10000_PRIORITY_MAPPER_ENTRY_MAP_REMOVE(&(priorityMapper->maps),
                                                    FM10000_PRI_MAP_LIST_MAPPER,
                                                    priorityMap);

            status = PriorityMapperApplyMapSet(sw, priorityMap, FALSE);

            if (status != FM_OK)
            {
                FM10000_PRIORITY_MAPPER_ENTRY_MAP_PUSH(&(priorityMapper->maps),
                                                    FM10000_PRI_MAP_LIST_MAPPER,
                                                    priorityMap);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

            }   /* end (status != FM_OK) */

            priorityMap->mapper   = FM10000_INVALID_PRIORITY_MAPPER;
            priorityMap->priority = FM_QOS_SWPRI_DEFAULT;

            priorityMapper->mapCount -= 1;

            break;

        }

        priorityMap = FM10000_PRIORITY_MAPPER_NEXT_MAP_GET(priorityMap,
                                                   FM10000_PRI_MAP_LIST_MAPPER);

    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperDeleteMap */




/*****************************************************************************/
/** PriorityMapperGet
 *
 * \desc            Retrieves the priority mapper that has been assigned the
 *                  specified switch priority.
 *
 * \note            Caller must hold state lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       priority is the switch priority to search for.
 *
 * \param[out]      mapper points to caller-allocated storage in which this
 *                  function will store the priority mapper ID.
 *
 * \result          FM_OK if successful.
 * \result          FM_ERR_INVALID_PRIORITY if an invalid switch priority has
 *                  been specified.
 * \result          FM_ERR_NOT_FOUND if no priority mapper could be found that
 *                  has been assigned the specified switch priority.
 *
 *****************************************************************************/
static fm_status PriorityMapperGet(fm_int sw, fm_uint priority, fm_int *mapper)
{
    fm_switch *             switchPtr;
    fm_status               status = FM_ERR_NOT_FOUND;
    fm_int                  i;
    fm10000_priorityMapper *priorityMapper;
    fm10000_switch *        switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d priority=%u mapper=%p\n",
                 sw,
                 priority,
                 (void *) mapper);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    for (i = 0 ; i < FM10000_MAX_SWITCH_PRIORITIES ; i++)
    {
        priorityMapper = &(switchExt->priorityMapSet->mappers[i]);

        if (priorityMapper->used && priorityMapper->priority == priority)
        {

            *mapper = i;
            status = FM_OK;
            break;
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperGet */




/*****************************************************************************/
/** PriorityMapperAttributeGet
 * \ingroup intQoS
 *
 * \desc            Retrieves a priority mapper attribute.
 *
 * \note            Caller must hold state lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the priority mapper for which to retrieve an
 *                  attribute.
 *
 * \param[in]       attribute is the priority mapper attribute to be retrieved.
 *
 * \param[out]      value points to caller-allocated storage in which this
 *                  function will store the attribute value.
 *
 * \result          FM_OK if successful.
 * \return          FM_ERR_INVALID_PRIORITY_MAPPER if an invalid priority
 *                  mapper has been specified.
 * \result          FM_ERR_INVALID_ATTRIB if the specified attribute is not a
 *                  recognized attribute.
 *
 *****************************************************************************/
static fm_status PriorityMapperAttributeGet(fm_int sw,
                                     fm_int mapper, 
                                     fm_int attribute,
                                     void * value)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_OK;
    fm10000_priorityMapper *     priorityMapper;
    fm10000_switch *             switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d mapper=%d attribute=%d value=%p\n",
                 sw,
                 mapper,
                 attribute,
                 value);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    FM10000_PRIORITY_MAPPER_VALIDATE(switchExt, mapper, status);

    priorityMapper = &(switchExt->priorityMapSet->mappers[mapper]);

    switch (attribute)
    {
        case FM10000_PRI_MAPPER_SWPRI:
            *((fm_uint *) value) = priorityMapper->priority;

            break;

        case FM10000_PRI_MAPPER_MAP_COUNT:
            *((fm_uint *) value) = priorityMapper->mapCount;

            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;

            break;

    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperAttributeGet */




/*****************************************************************************/
/** PriorityMapperAttributeSet
 * \ingroup intQoS
 *
 * \desc            Sets a priority mapper attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mapper is the priority mapper for which to set an
 *                  attribute.
 *
 * \param[in]       attribute is the priority mapper attribute to be set.
 *
 * \param[in]       value points to caller-allocated storage containing the
 *                  attribute value.
 *
 * \result          FM_OK if successful.
 * \return          FM_ERR_INVALID_PRIORITY_MAPPER if an invalid priority
 *                  mapper has been specified.
 * \result          FM_ERR_INVALID_PRIORITY if an invalid switch priority has
 *                  been specified.
 * \result          FM_ERR_INVALID_ATTRIB if the specified attribute is not a
 *                  recognized attribute.
 *
 *****************************************************************************/
static fm_status PriorityMapperAttributeSet(fm_int sw,
                                            fm_int mapper, 
                                            fm_int attribute,
                                            void * value)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_OK;
    fm_uint                      priority;
    fm_int                       i;
    fm10000_internalPriorityMap *priorityMap;
    fm10000_priorityMapper *     priorityMapper;
    fm10000_priorityMapSet *     priorityMapSet;
    fm10000_switch *             switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d mapper=%d attribute=%d value=%p\n",
                 sw,
                 mapper,
                 attribute,
                 value);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    FM10000_PRIORITY_MAPPER_VALIDATE(switchExt, mapper, status);

    priorityMapSet = switchExt->priorityMapSet;

    priorityMapper = &(priorityMapSet->mappers[mapper]);

    switch (attribute)
    {
        case FM10000_PRI_MAPPER_SWPRI:
            priority = *((fm_uint *) value);

            /***************************************************
             * Ensure that the specified priority is not already
             * associated with a different priority mapper.
             **************************************************/
            for (i = 0 ; i < FM10000_MAX_SWITCH_PRIORITIES ; i++)
            {
                if (mapper == i)
                {
                    continue;
                }

                if (   (priorityMapSet->mappers[i].used     == TRUE)
                    && (priorityMapSet->mappers[i].priority == priority))
                {
                    status = FM_ERR_INVALID_VALUE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
                }
            }

            priorityMap = 
                 FM10000_PRIORITY_MAPPER_FIRST_MAP_GET(&(priorityMapper->maps));

            while (priorityMap != NULL)
            {
                priorityMap->priority = priority;
                priorityMap = FM10000_PRIORITY_MAPPER_NEXT_MAP_GET(priorityMap,
                                                   FM10000_PRI_MAP_LIST_MAPPER);
            }

            priorityMapper->priority = priority;

            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            break;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperAttributeSet */




/*****************************************************************************/
/** PriorityMapperSearchByClass
 * \ingroup intQoS
 *
 * \desc            Retrieves the priority mapper that has been associated with
 *                  the specified frame class.
 *
 * \note            Caller must hold state lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       trapClass is the frame class to search for.
 *
 * \param[out]      mapper points to caller-allocated storage in which this
 *                  function will store the priority mapper ID.
 *
 * \return          FM_OK if successful.
 * \result          FM_ERR_NOT_FOUND if no priority mapper could be found that
 *                  has been associated with the specified frame class.
 * \return          FM_ERR_INVALID_PRIORITY_MAP if the frame class is not a
 *                  recognized frame class.
 *
 *****************************************************************************/
static fm_status PriorityMapperSearchByClass(fm_int  sw,
                                             fm_int  trapClass,
                                             fm_int *mapper)
{
    fm_status                    status;
    fm10000_internalPriorityMap *priorityMap;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d trapClass=%d mapper=%p\n",
                 sw,
                 trapClass,
                 (void *) mapper);

    status = PriorityMapperFindMap(sw, trapClass, &priorityMap);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    status = FM_ERR_NOT_FOUND;

    if (priorityMap->mapper != FM10000_INVALID_PRIORITY_MAPPER)
    {
        *mapper = priorityMap->mapper;
        status = FM_OK;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperSearchByClass */




/*****************************************************************************/
/** PriorityMapperTrapClassConvert
 * \ingroup intQoS
 *
 * \desc            Converts global QoS trap classes to internal fm10000 
 *                  classes.
 *
 * \param[in]       qosTrapClass QoS global trap class.
 *
 * \param[out]      internalTrapClass points to caller-allocated storage in 
 *                  which this the internal trap class..
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the function's argument is
 *                  incorrect.
 *
 *****************************************************************************/
static fm_status PriorityMapperTrapClassConvert(fm_int qosTrapClass,
                                                fm_int *internalTrapClass)
{
    fm_status status = FM_OK;;

    /* Check the pointer to output param */
    if (internalTrapClass == NULL)
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_INVALID_ARGUMENT);

    /* Initialize output trap class as invalid */
    *internalTrapClass = FM10000_QOS_TRAP_CLASS_MAX;

    /* Do conversion */
    switch (qosTrapClass)
    {
        case FM_QOS_TRAP_CLASS_CPU_MAC:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_CPU_MAC;
            break;

        case FM_QOS_TRAP_CLASS_ICMP:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_ICMP;
            break;

        case FM_QOS_TRAP_CLASS_IGMP:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_IGMP;
            break;

        case FM_QOS_TRAP_CLASS_IP_OPTION:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_IP_OPTION;
            break;

        case FM_QOS_TRAP_CLASS_MTU_VIOLATION:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_MTU_VIOLATION;
            break;

        case FM_QOS_TRAP_CLASS_BCAST_FLOODING:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_BCAST_FLOODING;
            break;

        case FM_QOS_TRAP_CLASS_MCAST_FLOODING:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_MCAST_FLOODING;
            break;

        case FM_QOS_TRAP_CLASS_UCAST_FLOODING:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_UCAST_FLOODING;
            break;

        case FM_QOS_TRAP_CLASS_TTL1:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_TTL1;
            break;

        case FM_QOS_TRAP_CLASS_NEXTHOP_MISS:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_NEXTHOP_MISS;
            break;

        case FM_QOS_TRAP_CLASS_IEEE:
            *internalTrapClass = FM10000_QOS_TRAP_CLASS_RESERVED_MAC;
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
     }

    return status;

}   /* end PriorityMapperTrapClassConvert */




/*****************************************************************************/
/** PriorityMapperQoSTrapClassToHaMask
 * \ingroup intQoS
 *
 * \desc            Maps QoS class trap to the HA mask of trigger.
 *
 * \param[in]       trapClass is the QoS trap class for which trigger HA mask is
 *                  to be set.
 *
 * \return          Trigger's HW defines for handler action bits
 *
 *****************************************************************************/
static fm_uint64 PriorityMapperQoSTrapClassToHaMask(fm_int trapClass)
{
    switch (trapClass)
    {
        case FM10000_QOS_TRAP_CLASS_CPU_MAC:
            return FM10000_TRIGGER_HA_TRAP_CPU;

        case FM10000_QOS_TRAP_CLASS_ICMP:
            return FM10000_TRIGGER_HA_TRAP_ICMP_TTL;

        case FM10000_QOS_TRAP_CLASS_IGMP:
            return FM10000_TRIGGER_HA_TRAP_IGMP;

        case FM10000_QOS_TRAP_CLASS_IP_OPTION:
            return FM10000_TRIGGER_HA_TRAP_IP_OPTION;

        case FM10000_QOS_TRAP_CLASS_MTU_VIOLATION:
            return FM10000_TRIGGER_HA_TRAP_MTU;

        case FM10000_QOS_TRAP_CLASS_TTL1:
            return FM10000_TRIGGER_HA_TRAP_TTL;

        default:
            return 0;

     }

}   /* end PriorityMapperQoSTrapClassToHaMask */




/*****************************************************************************/
/* PriorityMapperFindMap
 * \ingroup intQoS
 *
 * \desc            Finds the internal priority map corresponding to the
 *                  specified frame class.
 *
 * \note            Caller must hold state lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       trapClass is the frame class for which to find the
 *                  corresponding internal priority map.
 *
 * \param[out]      internalMap points to caller-allocated storage in which
 *                  this function will store a pointer to the internal priority
 *                  map.
 *
 * \return          FM_OK if an internal priority map corresponding to the
 *                  specified frame class has been found.
 * \return          FM_ERR_INVALID_PRIORITY_MAP otherwise.
 *
 *****************************************************************************/
static fm_status PriorityMapperFindMap(
                                      fm_int                       sw,
                                      fm_int                       trapClass,
                                      fm10000_internalPriorityMap **internalMap)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_ERR_INVALID_PRIORITY_MAP;
    fm10000_switch *             switchExt;
    fm10000_internalPriorityMap *priorityMap;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d trapClass=%d internalMap=%p\n",
                 sw,
                 trapClass,
                 (void *) internalMap);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    priorityMap = 
      FM10000_PRIORITY_MAPPER_FIRST_MAP_GET(&(switchExt->priorityMapSet->maps));

    while (priorityMap != NULL)
    {
        if (priorityMap->trapClass == trapClass)
        {
            *internalMap = priorityMap;

            status = FM_OK;

            break;
        }

        priorityMap = FM10000_PRIORITY_MAPPER_NEXT_MAP_GET(priorityMap,
                                                     FM10000_PRI_MAP_LIST_POOL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperFindMap */




/*****************************************************************************/
/* PriorityMapperApplyMapSet
 * \ingroup intQoS
 *
 * \desc            Applies the specified priority mapper's current priority
 *                  mapset to the mapper hardware resource.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       map is the internal priority map which mapset is to
 *                  be updated.
 *
 * \param[in]       enabled is the flag indicating if the mapset is going to 
 *                  enabled (TRUE) or disabled (FALSE).
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PriorityMapperApplyMapSet(fm_int sw, 
                                           fm10000_internalPriorityMap * map,
                                           fm_bool enabled)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_OK;
    fm_uint64                    mapperTrigHaMask;
    fm10000_switch *             switchExt;
    fm_triggerCondition          trigCond;
    fm_triggerAction             trigAction;
    fm10000_priorityMapper *     priorityMapper;
    fm_char                      trigName[20];

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d enabled=%s\n",
                 sw,
                 (enabled)?"TRUE" : "FALSE");

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;
    mapperTrigHaMask = FM_LITERAL_U64(0);

    if (map != NULL)
    {
        if (map->trapClass == FM10000_QOS_TRAP_CLASS_BCAST_FLOODING)
        {
            fm_bool bcastTrapState;

            /* Check if trap is enabled */
            fm10000GetStateBcastTrapFlooding(sw, &bcastTrapState);

            if (bcastTrapState == TRUE)
            {
               /* If yes update the priority for flooding trigger, 
                  otherwise priority will be set by enabling trap attribute */
                if (enabled)
                    fm10000SetTrapPriorityBcastFlooding(sw, map->priority);
                else
                    fm10000SetTrapPriorityBcastFlooding(sw, 
                                                        FM_QOS_SWPRI_DEFAULT);
            }

        }
        else if (map->trapClass == FM10000_QOS_TRAP_CLASS_MCAST_FLOODING)
        {
            fm_bool mcastTrapState;

            /* Check if trap is enabled */
            fm10000GetStateMcastTrapFlooding(sw, &mcastTrapState);

            if (mcastTrapState == TRUE)
            {
               /* If yes update the priority for flooding trigger, 
                  otherwise priority will be set by enabling trap attribute */
                if (enabled)
                    fm10000SetTrapPriorityMcastFlooding(sw, map->priority);
                else
                    fm10000SetTrapPriorityMcastFlooding(sw, 
                                                        FM_QOS_SWPRI_DEFAULT);
            }

        }
        else if (map->trapClass == FM10000_QOS_TRAP_CLASS_UCAST_FLOODING)
        {
            fm_bool ucastTrapState;

            /* Check if trap is enabled */
            fm10000GetStateMcastTrapFlooding(sw, &ucastTrapState);

            if (ucastTrapState == TRUE)
            {
               /* If yes update the priority for flooding trigger, 
                  otherwise priority will be set by enabling trap attribute */
                if (enabled)
                {
                    status = 
                         fm10000SetTrapPriorityUcastFlooding(sw, map->priority);
                }
                else
                {
                    status = fm10000SetTrapPriorityUcastFlooding(sw, 
                                                          FM_QOS_SWPRI_DEFAULT);
                }
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
            }
        }
        else if (map->trapClass == FM10000_QOS_TRAP_CLASS_NEXTHOP_MISS)
        {
            /* Add/remove action to the routingUnresolvedNextHop trigger */
            if (enabled)
            {
                status = fm10000SetPriorityUnresolvedNextHopTrigger(sw, 
                                                                 map->priority);
            }
            else
            {
                status = fm10000SetPriorityUnresolvedNextHopTrigger(sw, 
                                                          FM_QOS_SWPRI_DEFAULT);
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
        }
        else if (map->trapClass == FM10000_QOS_TRAP_CLASS_RESERVED_MAC)
        {

            if (enabled)
            {
                /* Get/cache the priority configured already */
                status = fm10000GetSwitchAttribute(sw,
                                                FM_SWITCH_RESERVED_MAC_TRAP_PRI,
                                                (void *)(&map->cache_priority));
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

                /* Update sw priority for reserved mac configuration */
                status = fm10000SetSwitchAttribute(sw,
                                                FM_SWITCH_RESERVED_MAC_TRAP_PRI,
                                                (void *)(&map->priority));
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
            }
            else
            {
                /* Update sw priority for reserved mac configuration */
                status = fm10000SetSwitchAttribute(sw,
                                                FM_SWITCH_RESERVED_MAC_TRAP_PRI,
                                                (void *)(&map->cache_priority));
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
            }
        }
        else
        {
            /* Set HAMask of priority mapper trigger */
            mapperTrigHaMask = 
                PriorityMapperQoSTrapClassToHaMask(map->trapClass);
        }
    }   /* end while (priorityMap != NULL) */


    /* Check if HA mask is non zero */
    if (mapperTrigHaMask)
    {
        priorityMapper = &(switchExt->priorityMapSet->mappers[map->mapper]);

        if (enabled)
        {
            /* Check if trigger for the mapper already created */
            if (FALSE == priorityMapper->mapTrigCreated)
            {
                /* Create the trigger */
                FM_SPRINTF_S(trigName,
                             sizeof(trigName),
                             "priorityMapperT%d",
                             priorityMapper->id);

                status = fm10000CreateTrigger(sw,
                                           FM10000_TRIGGER_GROUP_PRIORITY_MAP,
                                           priorityMapper->id,
                                           TRUE,
                                           trigName);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

                FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                "New Priority Mapper trigger created: %s for priority = %d \n",
                trigName,
                priorityMapper->priority);

                status = fmInitTriggerAction(sw, &trigAction);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

                status = fmInitTriggerCondition(sw, &trigCond);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);
                
                trigCond.cfg.HAMask = FM_LITERAL_U64(0);

                priorityMapper->mapTrigCreated = TRUE;
            }
        }

        /* Get priority Mapper trigger conditions actions */
        status = fm10000GetTrigger(sw, 
                           FM10000_TRIGGER_GROUP_PRIORITY_MAP, 
                           map->mapper, 
                           &trigCond,
                           &trigAction);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

        if (enabled)
        {
            trigCond.cfg.HAMask |= mapperTrigHaMask;
        }
        else
        {
            trigCond.cfg.HAMask &= ~(mapperTrigHaMask);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
        "Applying new HA mask: %016llX for Priority Mapper trigger for priority = %d \n",
        trigCond.cfg.HAMask,
        priorityMapper->priority);

        if ( trigCond.cfg.HAMask == 0 )
        {
            status = fm10000DeleteTrigger(sw,
                                          FM10000_TRIGGER_GROUP_PRIORITY_MAP,
                                          map->mapper,
                                          TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

            priorityMapper->mapTrigCreated = FALSE;
        }
        else
        {
            trigCond.cfg.rxPortset = FM_PORT_SET_ALL;

            status = fm10000SetTriggerCondition(sw,
                               FM10000_TRIGGER_GROUP_PRIORITY_MAP,
                               map->mapper,
                               &trigCond,
                               TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

            trigAction.cfg.switchPriAction = FM_TRIGGER_SWPRI_ACTION_REASSIGN;
            trigAction.param.newSwitchPri = map->priority;
            
            status = fm10000SetTriggerAction(sw,
                               FM10000_TRIGGER_GROUP_PRIORITY_MAP,
                               map->mapper,
                               &trigAction,
                               TRUE);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end PriorityMapperApplyMapSet */




/*****************************************************************************/
/** ESchedGroupVerifyConfig
 * \ingroup intQos
 *
 * \desc            Verify new egress scheduler configuration
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port whose egress scheduler configuration to
 *                  be verified
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_NEW_ESCHED_CONFIG otherwise
 *
 *****************************************************************************/
static fm_status ESchedGroupVerifyConfig(fm_int sw,
                                         fm_int physPort)
{
    fm10000_eschedConfigPerPort *eschedPtr;
    fm10000_switch *switchExt;
    fm_int          on2off = 0;
    fm_int          off2on = 0;
    fm_uint32       i;
    fm_uint32       j;
    fm_uint32       lowTCBoundary;
    fm_uint32       highTCBoundary;
    fm_status       err = FM_OK;
    fm_int          tcCounts[FM_MAX_TRAFFIC_CLASSES] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_QOS,
                  "sw=%d, physPort=%d\n",
                  sw, 
                  physPort);

    switchExt = GET_SWITCH_EXT(sw);
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );

    /******************************************************
     *  check TC boundary of new scheduler configuration
     ******************************************************/
     
    for ( i = 0 ; i < eschedPtr->numGroupsNew ; i++ )
    {
        lowTCBoundary  = eschedPtr->newSchedGroup[ i ].tcBoundaryA;
        highTCBoundary = eschedPtr->newSchedGroup[ i ].tcBoundaryB;

        if ( lowTCBoundary > FM10000_MAX_TRAFFIC_CLASS || 
             highTCBoundary > FM10000_MAX_TRAFFIC_CLASS )
        {
            FM_LOG_ERROR(FM_LOG_CAT_QOS, 
                         "Invalid group boundary in new"
                         "egress scheduler configuration of physical port %d",
                         physPort);
            err = FM_ERR_INVALID_NEW_ESCHED_CONFIG;
            goto ABORT;
        }

        if (highTCBoundary < lowTCBoundary)
        {
            eschedPtr->newSchedGroup[ i ].tcBoundaryA = highTCBoundary;
            eschedPtr->newSchedGroup[ i ].tcBoundaryB = lowTCBoundary;
            highTCBoundary = eschedPtr->newSchedGroup[ i ].tcBoundaryB;
            lowTCBoundary  = eschedPtr->newSchedGroup[ i ].tcBoundaryA;
        }

        for (j = lowTCBoundary ; j <= highTCBoundary ; j++)
        {
            tcCounts[j] ++;
        }
        
    }   /* end for ( i = 0 ; i < eschedPtr->numGroupsNew ; i++ ) */

    for ( i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++ )
    {
        if ( tcCounts[i] == 0 )
        {
            FM_LOG_ERROR(FM_LOG_CAT_QOS, 
                         "Traffic class %d of physical"
                         " port %d is not assigned to a scheduling group"
                         " in new egreess scheduler configuration", 
                         i, 
                         physPort);
            err = FM_ERR_INVALID_NEW_ESCHED_CONFIG;
            goto ABORT;
        }
        else if (tcCounts[i] > 1)
        {
            FM_LOG_ERROR(FM_LOG_CAT_QOS, 
                         "Traffic class %d of physical"
                         " port %d is assigned to multiple scheduling"
                         "  group in new egreess scheduler configuration", 
                         i, 
                         physPort);
            err = FM_ERR_INVALID_NEW_ESCHED_CONFIG;
            goto ABORT;
            
        }   /* end if (tcCounts[i] == 0) */
        
    }   /* end for ( i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++ ) */

    /******************************************************************
     *  check strict bits to make sure that there is no strict
     *  group between DRR groups.
     ******************************************************************/
     
    for ( i = 0 ; i < eschedPtr->numGroupsNew - 1 ; i++ )
    {
        if ( eschedPtr->newSchedGroup[i].strict > 
             eschedPtr->newSchedGroup[i + 1].strict )
        {
            if (off2on != 0)
            {
                FM_LOG_ERROR(FM_LOG_CAT_QOS, 
                             "Invalid Strict bit setting for port %d; only one"
                             " consecutive set of DDR groups is allowed.\n",
                             physPort);
                err = FM_ERR_INVALID_NEW_ESCHED_CONFIG;
                goto ABORT;
            }
            
            on2off++;
        }
        else if ( eschedPtr->newSchedGroup[i].strict < 
                  eschedPtr->newSchedGroup[i + 1].strict )
        {
            off2on++;
        }
        
    }   /* end for ( i = 0 ; i < eschedPtr->numGroupsNew - 1 ; i++ ) */

    if ( on2off > 1 || off2on > 1 )
    {
        FM_LOG_ERROR(FM_LOG_CAT_QOS, 
                     "Invalid Strict bit setting for port %d; only one"
                     " consecutive set of DDR groups is allowed.\n",
                     physPort);
        err = FM_ERR_INVALID_NEW_ESCHED_CONFIG;
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err)

}   /* end ESchedGroupVerifyConfig */




/*****************************************************************************/
/** ESchedGroupGetNewConfigData
 * \ingroup intQos
 *
 * \desc            Get new scheduler configuration data in form of register
 *                  values
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port whose egress scheduler configuration to
 *                  be verified
 *
 * \param[out]      grpBoundaryPtr is the pointer to a variable for group
 *                  boundary configuration
 *
 * \param[out]      priSetBoundaryPtr is the pointer to a variable for priority
 *                  set configuration
 *
 * \param[out]      strictBitsPtr is the pointer to a variable for strict
 *                  configuration
 *
 * \param[out]      drrWeight is a pointer to an array storing DRR weights
 *
 *****************************************************************************/
static void ESchedGroupGetNewConfigData(fm_int  sw,
                                        fm_int  physPort,
                                        fm_int *grpBoundaryPtr,
                                        fm_int *priSetBoundaryPtr,
                                        fm_int *strictBitsPtr,
                                        fm_int *drrWeight)
{
    fm10000_eschedConfigPerPort *eschedPtr;
    fm10000_switch *             switchExt;
    fm_uint32                    i;
    fm_uint32                    j;
    fm_uint32                    tcLowBoundary;
    fm_uint32                    tcHighBoundary;
    fm_int                       grpBoundary    = 0;
    fm_int                       priSetBoundary = 0;
    fm_int                       strictBits     = 0;

    FM_LOG_ENTRY( FM_LOG_CAT_QOS,
                  "sw=%d, physPort=%d\n",
                  sw, 
                  physPort);

    switchExt = GET_SWITCH_EXT(sw);
    eschedPtr = &( switchExt->eschedConfig[physPort] );

    for ( i = 0 ; i < eschedPtr->numGroupsNew ; i++ )
    {
        tcLowBoundary  = eschedPtr->newSchedGroup[i].tcBoundaryA;
        tcHighBoundary = eschedPtr->newSchedGroup[i].tcBoundaryB;
        
        FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_QOS, 
                               tcLowBoundary <= tcHighBoundary, 
                               (void)i,
                               "tcBoundaryA is greater than tcBoundaryB!\n");

        grpBoundary |= (1 << tcHighBoundary);

        drrWeight[tcHighBoundary] = eschedPtr->newSchedGroup[i].drrWeight;

        if (eschedPtr->newSchedGroup[i].strict == 1)
        {
            for (j = tcLowBoundary ; j <= tcHighBoundary ; j++)
            {
                strictBits |= (1 << j);
            }
        }
        
    }   /* end for ( i = 0 ; i < eschedPtr->numGroupsNew ; i++ ) */
    
    /* Per hardware specification  */    
    priSetBoundary = strictBits | (strictBits >> 1);

    *grpBoundaryPtr    = grpBoundary;
    *priSetBoundaryPtr = priSetBoundary;
    *strictBitsPtr     = strictBits;
    
ABORT:
    FM_LOG_EXIT_VOID(FM_LOG_CAT_QOS)

}   /* end ESchedGroupGetNewConfigData */




/*****************************************************************************/
/** ESchedGroupApplyNewConfig
 * \ingroup intQos
 *
 * \desc            Apply new egress scheduler configuration, i.e., writing
 *                  new configuration to registers
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port whose egress scheduler configuration to
 *                  be verified
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ESchedGroupApplyNewConfig(fm_int sw,
                                           fm_int physPort)
{
    fm_switch             *switchPtr;
    fm_uint32              regValue;
    fm_int                 i;
    fm_int                 qid;
    fm_int                 curQid;
    fm_int                 drrWeight[FM_MAX_TRAFFIC_CLASSES];
    fm_int                 grpBoundary;
    fm_int                 priSetBoundary;
    fm_int                 strictBits;
    fm_status              err;
    fm_bool                regLockTaken;
    fm_registerSGListEntry sgList;
    
    FM_LOG_ENTRY( FM_LOG_CAT_QOS,
                  "sw=%d, physPort=%d\n",
                  sw, 
                  physPort);

    /* Initialize local variables */
    err = FM_OK;
    regLockTaken = FALSE;
    grpBoundary    = 0;
    priSetBoundary = 0;
    strictBits     = 0;
    switchPtr = GET_SWITCH_PTR(sw);

    for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
    {
        drrWeight[i] = -1;
    }

    ESchedGroupGetNewConfigData(sw,
                                physPort,
                                &grpBoundary,
                                &priSetBoundary,
                                &strictBits,
                                drrWeight);

    /****************************************************************
     * Fill in registers the new configuration. If there is any error,
     * operation is aborted.
     ****************************************************************/

    /****************************************************************
     * fill PrioritySetBoundary and TcGroupBoundary fields of register 
     * SCHED_ESCHED_CFG_1
     ****************************************************************/

    FM_FLAG_TAKE_REG_LOCK(sw);

    regValue = 0;

    FM_SET_FIELD(regValue,
                 FM10000_SCHED_ESCHED_CFG_1,
                 PrioritySetBoundary,
                 priSetBoundary);

    FM_SET_FIELD(regValue,
                 FM10000_SCHED_ESCHED_CFG_1,
                 TcGroupBoundary,
                 grpBoundary);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_SCHED_ESCHED_CFG_1(physPort), 
                                 regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);


    /****************************************************************
     * fill groupBoundary fields of register 
     * FM10000_SCHED_MONITOR_DRR_CFG_PERPORT
     ****************************************************************/
    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheSchedMonitorDrrCfg,
                              1,
                              physPort,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              &regValue,
                              FALSE);

    err = fmRegCacheRead(sw, 1, &sgList, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_SET_FIELD(regValue,
                 FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                 groupBoundary,
                 grpBoundary);

    FM_SET_FIELD(regValue,
                 FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                 zeroLength,
                 strictBits);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheSchedMonitorDrrCfg,
                              1,
                              physPort,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              &regValue,
                              FALSE);

    err = fmRegCacheWrite(sw,
                          1,
                          &sgList,
                          TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);


    /*************************************************************
     * fill field strictPriority of register SCHED_ESCHED_CFG_2
     *************************************************************/
     
    err = switchPtr->ReadUINT32(sw, 
                                FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                &regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_SET_FIELD(regValue,
                 FM10000_SCHED_ESCHED_CFG_2,
                 StrictPriority,
                 strictBits);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                 regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);


    /***************************************************
     *  fill register FM10000_SCHED_MONITOR_DRR_Q_PERQ
     ***************************************************/
    qid = physPort * FM_MAX_TRAFFIC_CLASSES;

    for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
    {
        
        if (drrWeight[i] == -1)
        {
            continue;
        }

        regValue = drrWeight[i] & FM10000_QOS_MAX_DRR_WEIGHT;

        curQid = qid + i;

        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(curQid), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err)

}   /* end ESchedGroupApplyNewConfig */




/*****************************************************************************/
/** ESchedGroupRetrieveActiveConfig
 * \ingroup intQos
 *
 * \desc            Retrieve active egress scheduler configuration, i.e.,
 *                  reading registers and storing configuration information
 *                  into data structure
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port whose egress scheduler configuration to
 *                  be verified
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ESchedGroupRetrieveActiveConfig(fm_int sw,
                                                 fm_int physPort)
{
    fm_switch             *switchPtr;
    fm10000_switch        *switchExt;
    fm_status              err;
    fm_int                 grpBoundary;
    fm_int                 grpBoundary1;
    fm_int                 strictBits;
    fm_int                 drrWeight[FM_MAX_TRAFFIC_CLASSES] = {0};
    fm_uint32              regValue;
    fm_int                 i;
    fm_int                 qid;
    fm_int                 curQid;
    fm_bool                regLockTaken;
    fm_registerSGListEntry sgList;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    regLockTaken = FALSE;

    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_SCHED_ESCHED_CFG_1(physPort), 
                                &regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Get TC group boundary */
    grpBoundary = (regValue >> FM_MAX_TRAFFIC_CLASSES) & 0xff; 

    /* Get group boundary */
    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheSchedMonitorDrrCfg,
                              1,
                              physPort,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              &regValue,
                              FALSE);
    err = fmRegCacheRead(sw, 1, &sgList, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    grpBoundary1 = (regValue >> FM_MAX_TRAFFIC_CLASSES) & 0xff; 

    if (grpBoundary != grpBoundary1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_QOS, 
         "Group boundary setting in register SCHED_ESCHED_CFG_1 and "
         "SCHED_MONITOR_DRR_CFG_PERPORT is not consistent for physical port %d",
         physPort);
        goto ABORT;
    }

    /* Get strict priority bits */
    err = switchPtr->ReadUINT32(sw, 
                               FM10000_SCHED_ESCHED_CFG_2(physPort), &regValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    strictBits = regValue & 0xff;

    /* Get DRR quanta */
    qid = physPort * FM_MAX_TRAFFIC_CLASSES;

    for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
    {
        curQid = qid + i;
        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_SCHED_MONITOR_DRR_Q_PERQ(curQid), 
                                    &regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        
        drrWeight[i] = regValue & FM10000_QOS_MAX_DRR_WEIGHT;
    }

    err = ESchedGroupBuildActiveConfigDataStruct(sw,
                                                 physPort,
                                                 grpBoundary,
                                                 strictBits,
                                                 drrWeight);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    
    FM_FLAG_DROP_REG_LOCK(sw);

    return FM_OK;

ABORT:

    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    switchExt->eschedConfig[physPort].numGroupsActive = 0;

    return FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG;
    
}   /* end ESchedGroupRetrieveActiveConfig */




/*****************************************************************************/
/** ESchedGroupBuildActiveConfigDataStruct
 * \ingroup intQos
 *
 * \desc            Store active egress scheduler configuration information
 *                  into data structure
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port whose egress scheduler configuration to
 *                  be verified
 *
 * \param[in]       grpBoundary is the register value for group boundary
 *                  configuration
 *
 * \param[in]       strictBits is the register value for strict setting
 *
 * \param[in]       drrWeightPtr is a pointer to an array storing DRR weights
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ESchedGroupBuildActiveConfigDataStruct(fm_int  sw,
                                                        fm_int  physPort,
                                                        fm_int  grpBoundary,
                                                        fm_int  strictBits,
                                                        fm_int *drrWeightPtr)
{
    fm10000_switch *switchExt;
    fm_int          grpBoundaryBit;
    fm_int          strictBit;
    fm_int          i;
    fm_int          curGroupNum   = 0;
    fm_int          tcLowBoundary = 0;

    FM_NOT_USED( drrWeightPtr );

    switchExt = GET_SWITCH_EXT(sw);

    /********************************************************************
     *  fill in data structure for active egress scheduler configuration
     ********************************************************************/

    for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
    {
        grpBoundaryBit    = (grpBoundary >> i) & 0x1;
        strictBit         = (strictBits >> i) & 0x1;

        if (grpBoundaryBit == 1)
        {
            switchExt->eschedConfig[physPort].activeSchedGroup[curGroupNum].strict
                                                            = strictBit;
            switchExt->eschedConfig[physPort].activeSchedGroup[curGroupNum].tcBoundaryA
                                                            = tcLowBoundary;
            switchExt->eschedConfig[physPort].activeSchedGroup[curGroupNum].tcBoundaryB
                                                            = i;
            switchExt->eschedConfig[physPort].activeSchedGroup[curGroupNum].drrWeight
                                                            = drrWeightPtr[i];
            curGroupNum++;
            tcLowBoundary = i + 1;
        }
        
    }   /* end for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++) */

    switchExt->eschedConfig[physPort].numGroupsActive = curGroupNum;

    return FM_OK;

}   /* end ESchedGroupBuildActiveConfigDataStruct */




/*****************************************************************************/
/** ESchedInit
 * \ingroup intQos
 *
 * \desc            Initialize egress scheduler to the init values specifc 
 *                  for scheduler configuration methods.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is logical port whose egress scheduler configuration to
 *                  be verified
 *
 * \param[in]       qosQueueEnabled indicates if qosQueue configuration methods 
 *                  are enabled.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ESchedInit(fm_int sw,
                            fm_int port,
                            fm_bool qosQueueEnabled)
{
    fm_switch *    switchPtr;
    fm10000_switch *switchExt;
    fm10000_eschedConfigPerPort *eschedPtr;
    fm10000_port*  portExt;
    fm_uint32      regValue;
    fm_int         i;
    fm_int         qid;
    fm_int         physPort; 
    fm_status      err            = FM_OK;
    fm_bool        regLockTaken   = FALSE;
    
    FM_LOG_ENTRY( FM_LOG_CAT_QOS,
                  "sw=%d, physPort=%d, qosQueueEnabled=%d\n",
                  sw, 
                  port,
                  qosQueueEnabled);

    /* Get switch pointer*/
    switchPtr = GET_SWITCH_PTR(sw);
    /* Get switch extension */
    switchExt = GET_SWITCH_EXT(sw);

    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    if (err != FM_OK)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Shaper - disable shaping for all groups */
    for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
    {
        err = TxLimiterRegConfig(sw,
                                 physPort,
                                 i,
                                 FM_QOS_SHAPING_GROUP_RATE_DEFAULT);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }
    
    /* FM10000_SCHED_ESCHED_CFG_1*/
    regValue = 0;
    if (qosQueueEnabled)
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_1,
                     PrioritySetBoundary,
                     0x0);
    }
    else
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_1,
                     PrioritySetBoundary,
                     0xFF);
    }
    
    FM_SET_FIELD(regValue,
                 FM10000_SCHED_ESCHED_CFG_1,
                 TcGroupBoundary,
                 0xFF);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_SCHED_ESCHED_CFG_1(physPort), 
                                 regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* FM10000_SCHED_ESCHED_CFG_2 */
    regValue = 0;
    if (qosQueueEnabled)
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     StrictPriority,
                     0x0);

        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     TcEnable,
                     0x0);
    }
    else
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     StrictPriority,
                     0xFF);

        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     TcEnable,
                     0xFF);
    }
    
    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                 regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* FM10000_SCHED_MONITOR_DRR_CFG_PERPORT */
    regValue = 0;
    FM_SET_FIELD(regValue,
                 FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                 groupBoundary,
                 0xFF);
    if (qosQueueEnabled)
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                     zeroLength,
                     0x0);
    }
    else
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                     zeroLength,
                     0xFF);
    }
    err = switchPtr->WriteUINT32(sw, 
                                FM10000_SCHED_MONITOR_DRR_CFG_PERPORT(physPort),
                                regValue); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* FM10000_SCHED_MONITOR_DRR_Q_PERQ */
    regValue = 0;
    qid = physPort * FM_MAX_TRAFFIC_CLASSES;

    for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
    {
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_MONITOR_DRR_Q_PERQ,
                     q,
                     0x0);

        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(qid + i), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

    FM_FLAG_DROP_REG_LOCK(sw);

    /* Update configuration method flag*/
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );
    eschedPtr->qosQueueEnabled = qosQueueEnabled;

    if ( qosQueueEnabled )
    {
        /* Get port extension */
        portExt = GET_PORT_EXT(sw, port);
        /* Initialize queue parametrs */        
        eschedPtr->qosQueuesConfig.freeQoSQueueBw = (fm_uint16)portExt->speed;
        eschedPtr->qosQueuesConfig.numQoSQueues = 0;
        eschedPtr->qosQueuesConfig.tcMask = 0;
        for (i = 0 ; i < FM_MAX_TRAFFIC_CLASSES ; i++)
        {
            eschedPtr->qosQueuesConfig.qosQueuesParams[i].minBw = 0;
            eschedPtr->qosQueuesConfig.qosQueuesParams[i].tc = -1;
            eschedPtr->qosQueuesConfig.qosQueuesParams[i].queueId = -1;
        }
    }
    else
    {
        err =  ESchedGroupRetrieveActiveConfig(sw,
                                               physPort);
    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err)

}   /* end ESchedInit */




/*****************************************************************************/
/** EschedQosQueueConfig
 * \ingroup intQos
 *
 * \desc            Configure qos queue 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port on which queue will be configured
 *
 * \param[in]       attr is the attribute to configure
 *
 * \param[in]       param points to a ''fm_qosQueueParam'' structure containing
 *                  parameters of the requested queue.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status EschedQosQueueConfig(fm_int sw,
                                      fm_int physPort,
                                      fm_int attr,
                                      fm_qosQueueParam *param)
{
    fm_switch *switchPtr;
    fm10000_switch  *switchExt;
    fm10000_port *  portExt;
    fm10000_eschedConfigPerPort *eschedPtr;
    fm_uint32 regValue;
    fm_int logPort;
    fm_uint32 minBw;
    fm_uint64 maxBw;
    fm_int qid;
    fm_status  err;
    fm_bool regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d physPort=%d attr=%d param=%p\n",
                 sw,
                 physPort,
                 attr,
                 (void*)param);

    /* Initialize varables */
    err = FM_OK;
    regLockTaken = FALSE;

    /* Get switch pointer*/
    switchPtr = GET_SWITCH_PTR(sw);

    /* Get switch extension */
    switchExt = GET_SWITCH_EXT(sw);
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );

    if (param == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, FM_ERR_INVALID_ARGUMENT);
    }

    if (eschedPtr == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, FM_ERR_INVALID_ARGUMENT);
    }

    /* Set scheduler bw (percentage of port rate ) */
    err = fmMapPhysicalPortToLogical(switchPtr, physPort, &logPort);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);
    }

    portExt = GET_PORT_EXT(sw, logPort);
    minBw = (param->minBw * 100) / portExt->speed;

    /* Set shaper bw */
    if (param->maxBw == FM_QOS_QUEUE_MAX_BW_DEFAULT)
    {
        maxBw = FM_QOS_SHAPING_GROUP_RATE_DEFAULT;
    }
    else
    {
        maxBw = 1e6 * (fm_uint64)(param->maxBw);
    }

    /* Get qid for scheduler configuration */
    qid = param->tc + physPort * FM_MAX_TRAFFIC_CLASSES;
    
    FM_FLAG_TAKE_REG_LOCK(sw);

    if (attr == FM10000_QOS_ADD_QUEUE) 
    {
        /* Config limiter */
        err = TxLimiterRegConfig(sw,
                                 physPort,
                                 param->tc,
                                 maxBw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Set egress scheduler weight */
        regValue = 0;
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_MONITOR_DRR_Q_PERQ,
                     q,
                     FM10000_QOS_QUEUE_DRR_Q_UNIT * minBw);

        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(qid), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                    &regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Set TC */
        eschedPtr->qosQueuesConfig.tcMask |= (1<<param->tc);

        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     TcEnable,
                     eschedPtr->qosQueuesConfig.tcMask);
        
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Set free bandwidth */
        eschedPtr->qosQueuesConfig.freeQoSQueueBw -= param->minBw;
        /* Set queue Id (starting from 0) */
        param->queueId = eschedPtr->qosQueuesConfig.numQoSQueues;
        /* Copy queue params */
        FM_MEMCPY_S( &eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId],
                     sizeof(fm_qosQueueParam),
                     param,
                     sizeof(fm_qosQueueParam) );
        eschedPtr->qosQueuesConfig.numQoSQueues++;
        
    }
    else if (attr == FM10000_QOS_SET_QUEUE_MIN_BW) 
    {
        /* Configure scheduler */
        regValue = 0;
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_MONITOR_DRR_Q_PERQ,
                     q,
                     FM10000_QOS_QUEUE_DRR_Q_UNIT * minBw);

        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(qid), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Update free bandwidth */
        eschedPtr->qosQueuesConfig.freeQoSQueueBw += 
           eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].minBw;
        eschedPtr->qosQueuesConfig.freeQoSQueueBw -= param->minBw;
        /* Set queue Bw */
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].minBw = 
                                                               param->minBw;
        
    }
    else if (attr == FM10000_QOS_SET_QUEUE_MAX_BW) 
    {
        /* Configure limiter */
        err = TxLimiterRegConfig(
                  sw,
                  physPort,
                  eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].tc,
                  maxBw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        /* Set queue Bw */
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].maxBw = 
                                                               param->maxBw;
    }
    else if (attr == FM10000_QOS_SET_QUEUE_TRAFFIC_CLASS)
    {
        /* Configure-update limiter */    
        err = TxLimiterRegConfig(
              sw,
              physPort,
              param->tc,
              (fm_uint64)(1e6 * eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].maxBw));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        err = TxLimiterRegConfig(
                 sw,
                 physPort,
                 eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].tc,
                 FM_QOS_SHAPING_GROUP_RATE_DEFAULT);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
 
        /* Configure-update scheduler */
        regValue = 0;
        FM_SET_FIELD(regValue,
                     FM10000_SCHED_MONITOR_DRR_Q_PERQ,
                     q,
                     FM10000_QOS_QUEUE_DRR_Q_UNIT * minBw);
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(qid), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        regValue = 0;
        qid = eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].tc + 
              physPort * FM_MAX_TRAFFIC_CLASSES;
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(qid), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                    &regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Update TC in use */
        eschedPtr->qosQueuesConfig.tcMask |= (1<<param->tc);
        eschedPtr->qosQueuesConfig.tcMask ^= 
             (1<<eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].tc);

        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     TcEnable,
                     eschedPtr->qosQueuesConfig.tcMask);
        
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Update configuration: */
      
        /* Set new queue TC */
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].tc =
                                                              param->tc;
                                                              
    }
    else if (attr == FM10000_QOS_DEL_QUEUE)
    {
        /* Configure limiter */    
        TxLimiterRegConfig(sw,
                           physPort,
                           param->tc,
                           FM_QOS_SHAPING_GROUP_RATE_DEFAULT);

        /* Configure scheduler */
        regValue = 0;
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_MONITOR_DRR_Q_PERQ(qid), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                    &regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Update TC */
        eschedPtr->qosQueuesConfig.tcMask ^= (1<<param->tc);

        FM_SET_FIELD(regValue,
                     FM10000_SCHED_ESCHED_CFG_2,
                     TcEnable,
                     eschedPtr->qosQueuesConfig.tcMask);
        
        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_SCHED_ESCHED_CFG_2(physPort), 
                                     regValue); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Update free bandwidth */
        eschedPtr->qosQueuesConfig.freeQoSQueueBw += param->minBw;
        /* Update numQoSQueues */
        eschedPtr->qosQueuesConfig.numQoSQueues--;
        /* Initialize  queue */
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].minBw = 
                                                    FM_QOS_QUEUE_MIN_BW_DEFAULT;
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].maxBw = 
                                                    FM_QOS_QUEUE_MAX_BW_DEFAULT;
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].queueId = 
                                                    FM_QOS_QUEUE_ID_DEFAULT;
        eschedPtr->qosQueuesConfig.qosQueuesParams[param->queueId].tc = 
                                                    FM_QOS_QUEUE_TC_DEFAULT;
    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err)

}




/*****************************************************************************/
/** EschedQosQueueParamVerify
 * \ingroup intQos
 *
 * \desc            Verify new qos queue parameters
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is logical port on which queue needs to be verified
 *
 * \param[in]       eschedPtr points to a structure containing scheduler 
 *                  configuration.
 *
 * \param[in]       param points to a ''fm_qosQueueParam'' structure containing
 *                  parameters to be verified.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status EschedNewQosQueueParamVerify(
                                         fm_int sw,
                                         fm_int port,
                                         fm10000_eschedConfigPerPort *eschedPtr,
                                         fm_qosQueueParam *param)
{
    fm_status       err;
    fm10000_port *  portExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d param=%p\n",
                 sw,
                 port,
                 (void*)param);

    err = FM_OK;

    /* Check input */
    if (param == NULL)
    {
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check TC */
    if ( (param->tc < FM10000_QOS_MIN_TC) ||
         (param->tc > FM10000_QOS_MAX_TC) )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                     "Incorrect TC (tc=%d) specified in QoS queue for port=%d\n",
                     param->tc,
                     port);
                     
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Verify Bw */
    portExt = GET_PORT_EXT(sw, port);

    if (param->minBw > portExt->speed)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                     "Incorrect minBw (%d) specified in QoS queue for port=%d\n",
                     param->minBw,
                     port);
                     
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    if (param->maxBw != FM_QOS_QUEUE_MAX_BW_DEFAULT)
    {
        if ( (param->maxBw > portExt->speed) ||
             (param->maxBw == 0 ) ||
             (param->maxBw < param->minBw ) )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                         "Incorrect maxBw (%d) specified in QoS queue for port=%d\n",
                         param->maxBw,
                         port);
                         
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
        }
    }

    /* Return error when the mode incorrect */
    if (!eschedPtr->qosQueueEnabled)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                     "QoS Queue mode is not enabled for port=%d\n",
                     port);
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if nof queue is max  */
    if (eschedPtr->qosQueuesConfig.numQoSQueues >= FM10000_MAX_TRAFFIC_CLASS)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                     "Max QoS queues already created on port=%d\n",
                     port);
        err = FM_ERR_NO_FREE_RESOURCES;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if enough drr scheduler bandwidth on the port */
    if (eschedPtr->qosQueuesConfig.freeQoSQueueBw < param->minBw)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                     "Not enough Bw left (%d) for new Qos queue Bw requested (%d) on port=%d\n",
                     eschedPtr->qosQueuesConfig.freeQoSQueueBw,
                     param->minBw,
                     port);
        err = FM_ERR_NO_FREE_RESOURCES;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if Traffic class are free */
    if ( eschedPtr->qosQueuesConfig.tcMask & (1<<param->tc) )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                     "QoS queue with specified TC (%d) already exists on port=%d\n",
                     param->tc,
                     port);
        err = FM_ERR_NO_FREE_RESOURCES;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err)

}   /* end EschedQosQueueParamVerify */




/*****************************************************************************/
/** TxLimiterRegConfig
 * \ingroup intQos
 *
 * \desc            Configure TX Limiter, assuming register lock is taken. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is port on which queue will be configured
 *
 * \param[in]       sg is the shaping group to configure
 *
 * \param[in]       bw is shaper bandwidth to configure given in bits per second
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TxLimiterRegConfig(fm_int sw,
                                    fm_int physPort,
                                    fm_int sg,
                                    fm_uint64 bw)
{
    fm_status err;
    fm_uint32 regValue;
    fm_uint32 val;
    fm_switch *switchPtr;
    fm_float  rate;
    fm_float  fhMhz;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d physPort=%d sg=%d bw=%lld\n",
                 sw,
                 physPort,
                 sg,
                 bw);

    err = FM_OK;

    /* Get switch pointer*/
    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TX_RATE_LIM_CFG(physPort, sg), 
                                &regValue);    
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);    

    if (bw == FM_QOS_SHAPING_GROUP_RATE_DEFAULT)    
    {
        /* Defaut - disable shaping */
        FM_SET_FIELD(regValue,
                     FM10000_TX_RATE_LIM_CFG,                     
                     RateUnit,                      
                     FM10000_QOS_SHAPING_GROUP_RATE_UNIT_MAX_VAL);        

        FM_SET_FIELD(regValue,                      
                     FM10000_TX_RATE_LIM_CFG,                     
                     RateFrac,                      
                     FM10000_QOS_SHAPING_GROUP_RATE_FRAC_MAX_VAL);    
    }    
    else    
    {
        /* Calculate rateUnit and rateFraction - assuming bw was validated */
        err = fmComputeFHClockFreq(sw, &fhMhz);        
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);        
        rate = (((bw >> 3) / (fhMhz * 1e6)) * 48);        
        val = (fm_uint32) trunc(rate);        
        FM_SET_FIELD(regValue, 
                     FM10000_TX_RATE_LIM_CFG,                     
                     RateUnit, 
                     val);                     
        rate -= trunc(rate);        
        rate /= 1.0 / 256;        
        val = (fm_uint32) round(rate);        
        FM_SET_FIELD(regValue, 
                     FM10000_TX_RATE_LIM_CFG,                     
                     RateFrac, 
                     val);    
     }    

     /* Write the register */    
     err = switchPtr->WriteUINT32(sw, 
                                  FM10000_TX_RATE_LIM_CFG(physPort, sg), 
                                  regValue);    
     FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err)

}




/*****************************************************************************/
/** CheckGlobalWm
 * \ingroup intQos
 *
 * \desc            Check global watermark setting and take apropriate action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       globalWm is the global watermark setting 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CheckGlobalWm(fm_int    sw,
                               fm_uint32 globalWm)
{

    fm_switch *         switchPtr;
    fm_status           err;
    fm_text             wmScheme;
    fm10000_wmParam *   wpm;
    fm_int              i;
    fm_mtuEntry         mtuEntry;
    fm_uint32           maxMtu;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d globalWm=%d\n", 
                 sw, globalWm);

    /* Initialize local variable */
    err = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Allocate memory for watermark configuration */
    wpm = fmAlloc(sizeof(fm10000_wmParam));
    if (wpm == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_NO_MEM);
    }

    /* Get watermark schema */
    wmScheme = GET_FM10000_PROPERTY()->wmSelect;

    /* For the disabled scheme print warning when value is less than 
       auto mode calculation */
    if (!strcmp(wmScheme, "disabled"))
    {
        /* Determine the maximum switch MTU */ 
        maxMtu = 0;
        for (i = 0 ; i < FM10000_MTU_TABLE_ENTRIES; i++)
        {
            mtuEntry.index = i;
            err = fmGetSwitchAttribute(sw, FM_MTU_LIST, &mtuEntry);
        
            if ((err == FM_OK) && (mtuEntry.mtu > maxMtu))
            {
                maxMtu = mtuEntry.mtu;
            }
        }
        
        /* Set mtu to max size if not initialized */
        if (maxMtu == 0)
        {
            maxMtu = FM10000_MAX_FRAME_SIZE;
        }

        /* Print warning */
        if ( globalWm > FM10000_QOS_MAX_NUM_SEGS - 
                        FM10000_QOS_NUM_RESERVED_SEGS -
                        switchPtr->numCardinalPorts * 
                                         BYTES2SEG( maxMtu ) )
        {
            FM_LOG_WARNING(FM_LOG_CAT_QOS,
                           "Setting value for the global queue"
                           " size (sw = %d) is greater than"
                           " automatically calculated for the current "
                           "watermark scheme.\n", sw);
        }

    }

    /* Free memory allocated for watermark configuration */
    fmFree(wpm);

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end CheckGlobalWm */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000SetPortQOS
 * \ingroup intQos
 *
 * \desc            Sets the QoS attribute for a port.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port (logical) to operate on.
 *
 * \param[in]       attr is the port QoS attribute (see 'Port QoS Attributes')
 *                  to set.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Port QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the specified argument is not
 *                  valid.
 * \return          FM_ERR_INVALID_QOS_MODE if the specified QOS Mode is not
 *                  valid.
 * \return          FM_ERR_INVALID_ATTRIB if the specified attribute is not a
 *                  recognized attribute.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000SetPortQOS(fm_int sw,
                            fm_int port,
                            fm_int attr,
                            fm_int index,
                            void * value)
{
    fm_uint32              regValue;
    fm_uint64              regValue64;
    fm_uint32              addr;
    fm_uint32              val;
    fm_int                 physPort;
    fm_switch             *switchPtr;
    fm10000_switch        *switchExt;
    fm_status              err;
    fm_bool                regLockTaken;
    fm_uint32              temp1;
    fm_uint64              temp2;
    fm_float               rate;
    fm_float               fhMhz;
    fm_bool                autoPauseMode;
    fm_registerSGListEntry sgList;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d attr=%d index=%d value=%p\n",
                 sw, 
                 port, 
                 attr, 
                 index, 
                 value);

    /* Initialize variables */
    val = 0;
    regValue = 0;
    regLockTaken = FALSE;
    autoPauseMode = FALSE;
    err = FM_OK;

    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Map logical port to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    if (err != FM_OK)
    {    
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);
    }

    /* Take action for each attribute */
    switch (attr)
    {
        case FM_QOS_L2_VPRI1_MAP:
        case FM_QOS_RX_PRIORITY_MAP:
            /* Check index  not exceeding max of VLAN PRI available*/
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            val = *(fm_uint32 *) value;
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_VLAN_PRI, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = fmRegCacheReadUINT64(sw,
                                       &fm10000CacheRxVpriMap,
                                       physPort,
                                       &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_UNNAMED_FIELD64(regValue64, (index * 4), 4, val);
            /* Write the register */
            err = fmRegCacheWriteUINT64(sw,
                                        &fm10000CacheRxVpriMap,
                                        physPort,
                                        regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TX_PRIORITY_MAP:
            /* Check index  not exceeding max of VLAN PRI available*/
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI,
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            val = *(fm_uint32 *) value;
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_VLAN_PRI, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = fmRegCacheReadUINT64(sw,
                                       &fm10000CacheModVpri1Map,
                                       physPort,
                                       &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_UNNAMED_FIELD64(regValue64, (index * 4), 4, val);
            /* Write the register */
            err = fmRegCacheWriteUINT64(sw,
                                        &fm10000CacheModVpri1Map,
                                        physPort,
                                        regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TX_PRIORITY2_MAP:
            /* Check index  not exceeding max of VLAN PRI available*/
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            val = *(fm_uint32 *) value;
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_VLAN_PRI, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = fmRegCacheReadUINT64(sw,
                                       &fm10000CacheModVpri2Map,
                                       physPort,
                                       &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_UNNAMED_FIELD64(regValue64, (index * 4), 4, val);
            /* Write the register */
            err = fmRegCacheWriteUINT64(sw,
                                        &fm10000CacheModVpri2Map,
                                        physPort,
                                        regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_PC_RXMP_MAP:
            /* Check if index not exceeding max of PC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_PC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check if value not exceeding max of SMP available */
            val = *( fm_uint32 *) value;
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM_MAX_MEMORY_PARTITIONS, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_PC_SMP_MAP(physPort);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_UNNAMED_FIELD(regValue, 
                                 (index * FM10000_CM_PC_SMP_MAP_s_SMP), 
                                 FM10000_CM_PC_SMP_MAP_s_SMP, 
                                 val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_PRIVATE_PAUSE_ON_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /*Read the register*/
            addr = FM10000_CM_RX_SMP_PAUSE_WM(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, FM10000_CM_RX_SMP_PAUSE_WM,
                         PauseOn, val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            break;

        case FM_QOS_PRIVATE_PAUSE_OFF_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Read the register*/
            addr = FM10000_CM_RX_SMP_PAUSE_WM(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, FM10000_CM_RX_SMP_PAUSE_WM,
                         PauseOff, val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_RX_HOG_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Set the field */
            FM_SET_FIELD(regValue, FM10000_CM_RX_SMP_HOG_WM,
                         watermark, val);
            /* Write the register */
            addr = FM10000_CM_RX_SMP_HOG_WM(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_RX_PRIVATE_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ),
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Set the field */
            FM_SET_FIELD(regValue, FM10000_CM_RX_SMP_PRIVATE_WM,
                         watermark, val);
            /* Write the register */
            addr = FM10000_CM_RX_SMP_PRIVATE_WM(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TX_TC_PRIVATE_WM:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Set the field */
            FM_SET_FIELD(regValue, FM10000_CM_TX_TC_PRIVATE_WM,
                         watermark, val);
            /* Write the register */
            addr = FM10000_CM_TX_TC_PRIVATE_WM(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TX_SOFT_DROP_ON_PRIVATE:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI,
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM_ENABLED,
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_APPLY_TX_SOFTDROP_CFG(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_BIT(regValue,
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG,
                       SoftDropOnPrivate,
                       val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TX_SOFT_DROP_ON_RXMP_FREE:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI,
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM_ENABLED,
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_APPLY_TX_SOFTDROP_CFG(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_BIT(regValue,
                       FM10000_CM_APPLY_TX_SOFTDROP_CFG,
                       SoftDropOnSmpFree,
                       val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TX_HOG_WM:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Set the field */
            FM_SET_FIELD(regValue, FM10000_CM_TX_TC_HOG_WM,
                         watermark, val);
            addr = FM10000_CM_TX_TC_HOG_WM(physPort, index);
            /* Write the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHAPING_GROUP_MAX_BURST:
            /* Check if index not exceeding max of SG available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SG, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED(
                                   (*( (fm_uint64 *) value )), 
                                   FM10000_QOS_SHAPING_GROUP_MAX_BURST_MAX_VAL, 
                                   err, 
                                   FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_TX_RATE_LIM_CFG(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field - convert input value (bits) to 1K-byte units.*/
            val = (fm_uint32) (*( (fm_uint64 *) value ) >> 13);
            FM_SET_FIELD(regValue, FM10000_TX_RATE_LIM_CFG,
                         Capacity, val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHAPING_GROUP_RATE:
            /* Check if index not exceeding max of SG available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SG, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Get input value */
            temp2 = *( (fm_uint64 *) value );
            /* Get the register address */
            addr = FM10000_TX_RATE_LIM_CFG(physPort, index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            /* Read the register */
            err = switchPtr->ReadUINT32(sw, addr,&regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Compare if input is default */
            if (temp2 == FM_QOS_SHAPING_GROUP_RATE_DEFAULT)
            {
                /* Input value greater than port's speed - disable shaping */
                /*Set register max to disable rate limiter  */
                FM_SET_FIELD(regValue, 
                             FM10000_TX_RATE_LIM_CFG,
                             RateUnit, 
                             FM10000_QOS_SHAPING_GROUP_RATE_UNIT_MAX_VAL);
                FM_SET_FIELD(regValue, 
                             FM10000_TX_RATE_LIM_CFG,
                             RateFrac, 
                             FM10000_QOS_SHAPING_GROUP_RATE_FRAC_MAX_VAL);
            }
            else
            {
                /* Check if not exceed maximum value */
                FM10000_QOS_ABORT_ON_EXCEED(temp2, 
                                            (fm_uint64)(100 * 1e9), 
                                             err, 
                                             FM_ERR_INVALID_ARGUMENT);
                /* Update the field, before it do some calculations */
                /* Input value is the rate in bits per seond,  need to convert 
                 * to bytes */
                temp2 = temp2 >> 3;
                /* The rate needs to be converted to the number of bytes in 
                 * 48 FH clocks */
                err = fmComputeFHClockFreq(sw, &fhMhz);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
                rate = ((temp2 / (fhMhz * 1e6)) * 48);
                /* Update RateUnit*/
                val = (fm_uint32) trunc(rate);
                FM_SET_FIELD(regValue, FM10000_TX_RATE_LIM_CFG,
                             RateUnit, val);
                /* Update  RateFrac - fractional part in units of 1/256 */
                rate -= trunc(rate);
                rate /= 1.0 / 256;
                val = (fm_uint32) round(rate);
                FM_SET_FIELD(regValue, FM10000_TX_RATE_LIM_CFG,
                             RateFrac, val);
            }
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TC_PC_MAP:
            /* Check if index not exceeding max of PC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input */
            val = *( ( fm_uint32 *) value);
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_PC, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_TC_PC_MAP(physPort);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_UNNAMED_FIELD(regValue, 
                                 (index * FM10000_CM_TC_PC_MAP_s_PauseClass), 
                                  FM10000_CM_TC_PC_MAP_s_PauseClass, 
                                  val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TC_ENABLE:
            /* Check the input */
            val = *( ( fm_uint32 *) value);
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_TC_ENABLE_MAX_VAL, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_SCHED_ESCHED_CFG_2(physPort);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, 
                         FM10000_SCHED_ESCHED_CFG_2,
                         TcEnable,
                         val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SCHED_IFGPENALTY:
            /* Check input value */
            val = *( ( fm_uint32 *) value);
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_SCHED_IFGPENALTY_MAX_VAL, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the DRR register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_SCHED_MONITOR_DRR_CFG_PERPORT(physPort);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the DRR  */
            FM_SET_FIELD(regValue, 
                         FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                         ifgPenalty,
                         val);
            FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                      &fm10000CacheSchedMonitorDrrCfg,
                                      1,
                                      physPort,
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      &regValue,
                                      FALSE);

            err = fmRegCacheWrite(sw,
                                  1,
                                  &sgList,
                                  TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TC_SHAPING_GROUP_MAP:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check if value not exceeding max of SG available */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_SG, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the CM_BSG_MAP register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_BSG_MAP(physPort);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the CM_BSG_MAP  */
            FM_SET_UNNAMED_FIELD(regValue, 
                                 (index * FM10000_CM_BSG_MAP_s_BSG), 
                                 FM10000_CM_BSG_MAP_s_BSG, 
                                 val);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_APPLY_NEW_SCHED:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, FM_ERR_UNSUPPORTED);
            }
            /* Verify new scheduller configuration */
            err = ESchedGroupVerifyConfig(sw, physPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Apply new configuration */
            err = ESchedGroupApplyNewConfig(sw, physPort);
            break;

        case FM_QOS_NUM_SCHED_GROUPS:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify number of groups */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        (FM10000_QOS_MAX_SG + 1), 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Check additionally if number of queues is grater than 0 */
            if ( val < 1 )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Set the value */
            switchExt->eschedConfig[physPort].numGroupsNew = val; 
            break;

        case FM_QOS_SCHED_GROUP_STRICT:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify index */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(
                   index, 
                   (fm_int)(switchExt->eschedConfig[physPort].numGroupsNew - 1),
                   err, 
                   FM_ERR_INVALID_ARGUMENT);
            /* Verify strict property set value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val,
                                        FM10000_QOS_SCHED_GROUP_STRICT_MAX_VAL, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Set new trict property */
            switchExt->eschedConfig[physPort].newSchedGroup[index].strict
                                            = val;
            break;

        case FM_QOS_SCHED_GROUP_WEIGHT:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify index */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(
                   index, 
                   (fm_int)(switchExt->eschedConfig[physPort].numGroupsNew - 1),
                   err, 
                   FM_ERR_INVALID_ARGUMENT);
            /* Verify quanta DRR value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val,
                                        FM10000_QOS_SCHED_GROUP_WEIGHT_MAX_VAL, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Set new quanta DRR value */
            switchExt->eschedConfig[physPort].newSchedGroup[index].drrWeight
                                            = val;
            break;

        case FM_QOS_SCHED_GROUP_TCBOUNDARY_A:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify index */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(
                   index, 
                   (fm_int)(switchExt->eschedConfig[physPort].numGroupsNew -1 ),
                   err, 
                   FM_ERR_INVALID_ARGUMENT);
            /* Verify boundary value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val,
                                        FM10000_QOS_MAX_TC, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Set new boundary value */
            switchExt->eschedConfig[physPort].newSchedGroup[index].tcBoundaryA
                                            = val;
            break;

        case FM_QOS_SCHED_GROUP_TCBOUNDARY_B:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify index */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(
                   index, 
                   (fm_int)(switchExt->eschedConfig[physPort].numGroupsNew - 1), 
                   err, 
                   FM_ERR_INVALID_ARGUMENT);
            /* Verify boundary value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val,
                                        FM10000_QOS_MAX_TC, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Set new boundary value */
            switchExt->eschedConfig[physPort].newSchedGroup[index].tcBoundaryB
                                            = val;
            break;

        case FM_QOS_SHARED_PAUSE_ENABLE:
            val = *( ( fm_uint32 *) value);
            FM10000_QOS_ABORT_ON_EXCEED(val,
                                        FM10000_QOS_SHARED_PAUSE_ENABLE_MAX_VAL,
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            FM_FLAG_TAKE_REG_LOCK(sw);
            for (int i = 0; i < FM10000_CM_SHARED_SMP_PAUSE_CFG_ENTRIES; i++)
            {
                temp1= (val>>i) & 1;
                /* Update register for partition i*/
                addr = FM10000_CM_SHARED_SMP_PAUSE_CFG(i, 0);
                err = switchPtr->ReadUINT64(sw, addr, &regValue64);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
                /* Update the field */
                FM_SET_UNNAMED_FIELD64(regValue64, physPort, 1, temp1)
                err = switchPtr->WriteUINT64(sw, addr, regValue64);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            break;

        case FM_QOS_QUEUE:
            /* Verify input value  */
            FM10000_QOS_ABORT_ON_EXCEED(*( (fm_bool *) value ),
                                        FM_ENABLED, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Apply new config and initialize config regs */
            err = ESchedInit(sw, port, *( (fm_bool *) value ));
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        default:
             err = FM_ERR_INVALID_ATTRIB;
             break;
     }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000SetPortQOS */




/*****************************************************************************/
/** fm10000GetPortQOS
 * \ingroup intQos
 *
 * \desc            Get a port's QoS attribute.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port (logical) to operate on.
 *
 * \param[in]       attr is the port QoS attribute (see 'Port QoS Attributes')
 *                  to get.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Port QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[in]       value points to the attribute value to get.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG if the egress
 *                  scheduler configuration is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the specified argument is not
 *                  valid.
 * \return          FM_ERR_INVALID_ATTRIB if the specified attribute is not a
 *                  recognized attribute.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GetPortQOS(fm_int sw,
                            fm_int port,
                            fm_int attr,
                            fm_int index,
                            void * value)
{

    fm_uint32              regValue;
    fm_uint64              regValue64;
    fm_uint32              addr;
    fm_uint32              val;
    fm_int                 physPort;
    fm_switch             *switchPtr;
    fm10000_switch        *switchExt;
    fm_status              err;
    fm_float               rate;
    fm_float               fhMhz;
    fm_registerSGListEntry sgList;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d, port=%d, attr=%d, index=%d, value=%p\n",
                 sw, 
                 port, 
                 attr, 
                 index, 
                 (void *) value);

    /* Initialize variables */
    val = 0;
    err = FM_OK;

    /* Get the switch's pointer */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Map logical por to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Take action for each attribute */
    switch (attr)
    {

        case FM_QOS_L2_VPRI1_MAP:
        case FM_QOS_RX_PRIORITY_MAP:
            /* Check if index not exceeding max of VLANPRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            err = fmRegCacheReadUINT64(sw,
                                       &fm10000CacheRxVpriMap,
                                       physPort,
                                       &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Do some required conversions */
            *( (fm_uint32 *) value ) = 
                (fm_uint32) FM_GET_UNNAMED_FIELD64(regValue64,
                                                   ( index * 4 ), 
                                                   4);
            break;

        case FM_QOS_TX_PRIORITY_MAP:
            /* Check if index not exceeding max of VLANPRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            err = fmRegCacheReadUINT64(sw,
                                       &fm10000CacheModVpri1Map,
                                       physPort,
                                       &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Do some required conversions */
            *( (fm_uint32 *) value ) = 
                (fm_uint32) FM_GET_UNNAMED_FIELD64(regValue64,
                                                   ( index * 4 ), 
                                                   4);
            break;

        case FM_QOS_TX_PRIORITY2_MAP:
            /* Check if index not exceeding max of VLANPRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            err = fmRegCacheReadUINT64(sw,
                                       &fm10000CacheModVpri2Map,
                                       physPort,
                                       &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Do some required conversions */
            *( (fm_uint32 *) value ) = 
                (fm_uint32) FM_GET_UNNAMED_FIELD64(regValue64,
                                                   ( index * 4 ), 
                                                   4);
            break;

        case FM_QOS_PC_RXMP_MAP:
            /* Check if index not exceeding max of PC  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_PC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_PC_SMP_MAP(physPort);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                FM_GET_UNNAMED_FIELD(regValue,
                                     (index * FM10000_CM_PC_SMP_MAP_s_SMP),
                                     FM10000_CM_PC_SMP_MAP_s_SMP);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_RX_PRIVATE_WM:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_RX_SMP_PRIVATE_WM(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (with convertion segments to bytes) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_RX_SMP_PRIVATE_WM, 
                             watermark) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_RX_SMP_USAGE:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_RX_SMP_USAGE(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (segment units in bits 15:0) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_RX_SMP_USAGE, 
                             count) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_TX_HOG_WM:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Write the register */
            addr = FM10000_CM_TX_TC_HOG_WM(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (segment units in bits 15:0) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, FM10000_CM_TX_TC_HOG_WM,
                             watermark) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHAPING_GROUP_MAX_BURST:
            /* Check if index not exceeding max of SG available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SG, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_TX_RATE_LIM_CFG(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (convert 1K byte units to units of bits) */
            ( *( (fm_uint64 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_TX_RATE_LIM_CFG, 
                             Capacity) << 13;
            break;

        case FM_QOS_SHAPING_GROUP_RATE:
            /* Check if index not exceeding max of SG available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SG, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_TX_RATE_LIM_CFG(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Check if shaper disabled */
            if ( (FM10000_QOS_SHAPING_GROUP_RATE_UNIT_MAX_VAL ==
                 ((regValue >> FM10000_TX_RATE_LIM_CFG_l_RateUnit) & 0x7ff)) &&
                (FM10000_QOS_SHAPING_GROUP_RATE_FRAC_MAX_VAL ==
                 ((regValue >> FM10000_TX_RATE_LIM_CFG_l_RateFrac) & 0xff)) )
            {
                *( (fm_uint64 *) value ) = FM_QOS_SHAPING_GROUP_RATE_DEFAULT;
            }
            else
            {
                /* Convert to units of bits per second */
                err = fmComputeFHClockFreq(sw, &fhMhz);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
                rate  = 
                    ( (regValue >> FM10000_TX_RATE_LIM_CFG_l_RateUnit) 
                                                                   & 0x7ff ) + 
                    ( ( (regValue >> FM10000_TX_RATE_LIM_CFG_l_RateFrac) 
                                                                   & 0xff ) 
                                                                      / 256.0 );
                rate /= ( 48.0 / (fhMhz * 1e6) );
                *( (fm_uint64 *) value ) = ( (fm_uint64) round(rate) ) << 3;
            }
            break;

        case FM_QOS_TC_PC_MAP:
            /* Check if index not exceeding max of SG available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_TC_PC_MAP(physPort);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                FM_GET_UNNAMED_FIELD(regValue,
                                    (index * FM10000_CM_TC_PC_MAP_s_PauseClass),
                                     FM10000_CM_TC_PC_MAP_s_PauseClass);
            break;

        case  FM_QOS_TX_SOFT_DROP_ON_PRIVATE:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_APPLY_TX_SOFTDROP_CFG(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                FM_GET_BIT(regValue,
                           FM10000_CM_APPLY_TX_SOFTDROP_CFG,
                           SoftDropOnPrivate);
            break;

        case  FM_QOS_TX_SOFT_DROP_ON_RXMP_FREE:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_APPLY_TX_SOFTDROP_CFG(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                FM_GET_BIT(regValue,
                           FM10000_CM_APPLY_TX_SOFTDROP_CFG,
                           SoftDropOnSmpFree);
            break;

        case FM_QOS_TX_TC_PRIVATE_WM:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_TX_TC_PRIVATE_WM(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (convert to bytes) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_TX_TC_PRIVATE_WM, 
                             watermark)* BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_TX_TC_USAGE:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_TX_TC_USAGE(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (convert to bytes) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_TX_TC_USAGE, 
                             count)* BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_PRIVATE_PAUSE_ON_WM:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_RX_SMP_PAUSE_WM(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (convert to bytes) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_RX_SMP_PAUSE_WM, 
                             PauseOn)* BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_PRIVATE_PAUSE_OFF_WM:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_RX_SMP_PAUSE_WM(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (convert to bytes) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_RX_SMP_PAUSE_WM, 
                             PauseOff)* BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_RX_HOG_WM:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_RX_SMP_HOG_WM(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value (convert to bytes) */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_RX_SMP_HOG_WM, 
                             watermark)* BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_TC_ENABLE:
            /* Read the register */
            addr = FM10000_SCHED_ESCHED_CFG_2(physPort);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_SCHED_ESCHED_CFG_2,
                             TcEnable);
            break;

        case FM_QOS_ERL_USAGE:
            /* Check if index not exceeding max of SG available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SG, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_TX_RATE_LIM_USAGE(physPort, index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_TX_RATE_LIM_USAGE,
                             Units);
            break;

        case FM_QOS_SCHED_IFGPENALTY:
            /* Read the DRR register  */
            FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                      &fm10000CacheSchedMonitorDrrCfg,
                                      1,
                                      physPort,
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      &regValue,
                                      FALSE);
            err = fmRegCacheRead(sw, 1, &sgList, FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                            FM_GET_FIELD(regValue,
                                         FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                                         ifgPenalty);
            break;

        case FM_QOS_TC_SHAPING_GROUP_MAP:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the CM_BSG_MAP register */
            addr = FM10000_CM_BSG_MAP(physPort);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                            FM_GET_UNNAMED_FIELD(regValue,
                                             (index * FM10000_CM_BSG_MAP_s_BSG),
                                              FM10000_CM_BSG_MAP_s_BSG);
            break;

        case FM_QOS_RETRIEVE_ACTIVE_SCHED:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Get active Egress scheduler configuration */
            err = ESchedGroupRetrieveActiveConfig(sw, physPort);
            break;

        case FM_QOS_NUM_SCHED_GROUPS:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify active configuration */
            if (switchExt->eschedConfig[physPort].numGroupsActive == 0)
            {
                err = FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                            switchExt->eschedConfig[physPort].numGroupsActive; 
            break;

        case FM_QOS_SCHED_GROUP_STRICT:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify active configuration */
            if (switchExt->eschedConfig[physPort].numGroupsActive == 0)
            {
                err = FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Verify index */
            if (index < 0
                || index >= 
                     (fm_int) switchExt->eschedConfig[physPort].numGroupsActive)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) =
            switchExt->eschedConfig[physPort].activeSchedGroup[index].strict;
            break;

        case FM_QOS_SCHED_GROUP_WEIGHT:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify active configuration */
            if (switchExt->eschedConfig[physPort].numGroupsActive == 0)
            {
                err = FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Verify index */
            if (index < 0
                || index >= 
                     (fm_int) switchExt->eschedConfig[physPort].numGroupsActive)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Get the requested value */
            ( *( (fm_int32 *) value ) ) =
            switchExt->eschedConfig[physPort].activeSchedGroup[index].drrWeight;
            break;

        case FM_QOS_SCHED_GROUP_TCBOUNDARY_A:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify active configuration */
            if (switchExt->eschedConfig[physPort].numGroupsActive == 0)
            {
                err = FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Verify index */
            if (index < 0
                || index >= 
                     (fm_int) switchExt->eschedConfig[physPort].numGroupsActive)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
            switchExt->eschedConfig[physPort].activeSchedGroup[index].tcBoundaryA;
            break;

        case FM_QOS_SCHED_GROUP_TCBOUNDARY_B:
            /* Verify mode */
            if (switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Verify active configuration */
            if (switchExt->eschedConfig[physPort].numGroupsActive == 0)
            {
                err = FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Verify index */
            if (index < 0
                || index >= 
                   (fm_int) switchExt->eschedConfig[physPort].numGroupsActive)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
            switchExt->eschedConfig[physPort].activeSchedGroup[index].tcBoundaryB;
            break;

        case FM_QOS_SHARED_PAUSE_ENABLE:
            for (int i = 0; i < FM10000_CM_SHARED_SMP_PAUSE_CFG_ENTRIES; i++)
            {
                /* Read register for partition i*/
                addr = FM10000_CM_SHARED_SMP_PAUSE_CFG(i, 0);
                err = switchPtr->ReadUINT64(sw, addr, &regValue64);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
                /* Get the field */
                val |= (fm_uint32) (((regValue64 >> port) & 0x1) << i);
            }
            ( *( (fm_uint32 *) value ) ) = val;
            break;

        case FM_QOS_TC_ZERO_LENGTH:
            /* Read the FM10000_SCHED_MONITOR_DRR_CFG_PERPORT register */
            FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                      &fm10000CacheSchedMonitorDrrCfg,
                                      1,
                                      physPort,
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      &regValue,
                                      FALSE);
            err = fmRegCacheRead(sw, 1, &sgList, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                            FM_GET_FIELD(regValue,
                                         FM10000_SCHED_MONITOR_DRR_CFG_PERPORT,
                                         zeroLength);
            break;

        case FM_QOS_QUEUE:
            /* Get the requested value */
            ( *( (fm_bool *) value ) ) = 
                            switchExt->eschedConfig[physPort].qosQueueEnabled;
            break;

        case FM_QOS_QUEUE_FREE_BW:
            /* Verify mode */
            if (!switchExt->eschedConfig[physPort].qosQueueEnabled)
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                        switchExt->eschedConfig->qosQueuesConfig.freeQoSQueueBw;
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;
            
    }   /* end switch (attr) */

ABORT:
    FM_LOG_EXIT( FM_LOG_CAT_QOS, err )

}   /* end fm10000GetPortQOS */




/*****************************************************************************/
/** fm10000SetSwitchQOS
 * \ingroup intQos
 *
 * \desc            Set a switch global QOS attribute.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       attr is the port QoS attribute (see 'Port QoS Attributes')
 *                  to set.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Port QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the specified argument is not
 *                  valid.
 * \return          FM_ERR_INVALID_QOS_MODE if the specified QOS Mode is not
 *                  valid.
 * \return          FM_ERR_INVALID_ATTRIB if the specified attribute is not a
 *                  recognized attribute.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000SetSwitchQOS(fm_int sw,
                              fm_int attr,
                              fm_int index,
                              void * value)
{
    fm_uint32           regValue;
    fm_uint64           regValue64;
    fm_uint32           addr;
    fm_uint32           val;
    fm_uint32           temp;
    fm_status           err;
    fm_switch *         switchPtr;
    fm_bool             regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d, attr=%d, index=%d, value=%p\n",
                 sw, 
                 attr, 
                 index, 
                 (void *) value);

    /* Initialize variables */
    val = 0;
    regValue = 0;
    regLockTaken = FALSE;
    err = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);
    
    /* Take action for each attribute */
    switch (attr)
    {
        case FM_AUTO_PAUSE_MODE:
            /* Check input */
            FM10000_QOS_ABORT_ON_EXCEED(*( (fm_bool *) value ), 
                                        FM_ENABLED, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Set auto pause mode */
            err = SetAutoPauseMode(sw, 
                                   (index > 0) ? TRUE : FALSE, 
                                   *( (fm_bool *) value ));
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_DSCP_SWPRI_MAP:
            /* Check if index not exceeding max of DSCP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_DSCP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_SW_PRI, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Write the register */
            addr = FM10000_DSCP_PRI_MAP(index);
            FM_SET_FIELD(regValue, 
                         FM10000_DSCP_PRI_MAP,
                         pri,
                         val);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_PRIV_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check input value */
            FM10000_QOS_ABORT_ON_EXCEED((*( ( fm_uint32 *) value)), 
                                      SEG2BYTES(FM10000_QOS_MAX_NUM_SEGS - 
                                                FM10000_QOS_NUM_RESERVED_SEGS), 
                                      err, 
                                      FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG(*( ( fm_uint32 *) value));
            /* Perform additionally check for the global WM */
            CheckGlobalWm(sw, val);
            /* Set the field */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_GLOBAL_WM,
                         watermark,
                         val);
            /* Write the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_GLOBAL_WM();
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHARED_PAUSE_OFF_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SMP  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value  */
            FM10000_QOS_ABORT_ON_EXCEED((*( ( fm_uint32 *) value)), 
                                        FM10000_QOS_MAX_WATERMARK,
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG(*( ( fm_uint32 *) value));
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_SHARED_SMP_PAUSE_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_SHARED_SMP_PAUSE_WM,
                         PauseOff,
                         val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHARED_PAUSE_ON_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SMP  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check input value  */
            FM10000_QOS_ABORT_ON_EXCEED((*( ( fm_uint32 *) value)), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG(*( ( fm_uint32 *) value));
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_SHARED_SMP_PAUSE_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_SHARED_SMP_PAUSE_WM,
                         PauseOn,
                         val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHARED_PRI_WM:
            /* Check whether the watermarks are under automatic management */
            FM10000_QOS_ABORT_ON_AUTO_PAUSE_MODE(sw, 
                                                 err, 
                                                 FM_ERR_INVALID_QOS_MODE);
            /* Check if index not exceeding max of SWPRI  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input  */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Convert to segments and write the register */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_SHARED_WM,
                         watermark,
                         val);
            addr = FM10000_CM_SHARED_WM(index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_TC_SMP_MAP:
            /* Check if index not exceeding max of TC  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Map attribute value to register's value */
            val = *( (fm_uint32 *) value ) & 0xf;
            switch (val)
            {
                case FM_QOS_TC_SMP_0:
                    temp = 0;
                    break;
            
                case FM_QOS_TC_SMP_1:
                    temp = 1;
                    break;
            
                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    goto ABORT;
            
            }   /* end switch (val) */
            /* Read the register  - Apply module */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_APPLY_TC_TO_SMP();
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the bit */
            FM_SET_UNNAMED_FIELD(regValue, index, 1, temp);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Sweeper table should be configured the same as Apply table */
            /* Read the register  - Sweeper module */
            addr = FM10000_CM_SWEEPER_TC_TO_SMP();
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the bit */
            FM_SET_UNNAMED_FIELD(regValue, index, 1, temp);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            FM_FLAG_DROP_REG_LOCK(sw);
            /* Recalculate watermarks */
            if( switchPtr->autoPauseMode )
            {    
                err = SetAutoPauseMode(sw, 0, switchPtr->autoPauseMode);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }            
            break;

        case FM_QOS_SHARED_SOFT_DROP_WM:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input  */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_SOFTDROP_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            FM_SET_FIELD(regValue, 
                         FM10000_CM_SOFTDROP_WM, 
                         SoftDropSegmentLimit, 
                         val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHARED_SOFT_DROP_WM_HOG:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input  */
            FM10000_QOS_ABORT_ON_EXCEED(( *( (fm_uint32 *) value ) ), 
                                        FM10000_QOS_MAX_WATERMARK, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            val = BYTES2SEG( *( (fm_uint32 *) value ) );
            /* Read the register */
            FM_FLAG_TAKE_REG_LOCK(sw);
            addr = FM10000_CM_SOFTDROP_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_SOFTDROP_WM, 
                         HogSegmentLimit, 
                         val);
            /* Write the register */
            err = switchPtr->WriteUINT32(sw, addr, regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SHARED_SOFT_DROP_WM_JITTER:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                FM10000_QOS_SHARED_SOFT_DROP_WM_JITTER_MAX_VAL, 
                                err, 
                                FM_ERR_INVALID_ARGUMENT);
            /* Write the register */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_APPLY_SOFTDROP_CFG, 
                         JitterBits, 
                         val);
            addr = FM10000_CM_APPLY_SOFTDROP_CFG(index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SWPRI_TC_MAP:
            /* Check if index not exceeding max of SWPRI  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input  */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_TC, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Read the register - Apply module */
            addr = FM10000_CM_APPLY_SWITCH_PRI_TO_TC(0);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw, addr, &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            val = *( (fm_uint32 *) value );
            FM_SET_UNNAMED_FIELD64(regValue64, 
                               (index * FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc),
                               FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc, 
                               val);
            /* Write the register - Apply module */
            err = switchPtr->WriteUINT64(sw, addr, regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Sweeper table should be configured the same as Apply table */
            /* Read the register - Sweeper module */
            addr = FM10000_CM_SWEEPER_SWITCH_PRI_TO_TC(0);
            err = switchPtr->ReadUINT64(sw, addr, &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_UNNAMED_FIELD64(regValue64, 
                             (index * FM10000_CM_SWEEPER_SWITCH_PRI_TO_TC_s_tc),
                             FM10000_CM_SWEEPER_SWITCH_PRI_TO_TC_s_tc, 
                             val);
            /* Write the register - Sweeper module */
            err = switchPtr->WriteUINT64(sw, addr, regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            FM_FLAG_DROP_REG_LOCK(sw);
            /* Recalculate watermarks */
            if( switchPtr->autoPauseMode )
            {    
                err = SetAutoPauseMode(sw, 0, switchPtr->autoPauseMode);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            }            
            break;

        case FM_QOS_TRAP_CLASS_SWPRI_MAP:
            val = *( (fm_uint32 *) value );

            err = SetTrapClassMap(sw, index, val);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            break;

        case FM_QOS_VPRI_SWPRI_MAP:
            /* Check if index not exceeding max of VPRI  available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Check the input */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_SW_PRI, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Write the register */
            FM_SET_FIELD(regValue, 
                         FM10000_VPRI_PRI_MAP, 
                         pri, 
                         val);
            addr = FM10000_VPRI_PRI_MAP(index);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        case FM_QOS_SWITCH_IFG_PENALTY:
            /* Check the input value */
            val = *( (fm_uint32 *) value );
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_SWITCH_IFG_PENALTY_MAX_VAL, 
                                        err, 
                                        FM_ERR_INVALID_ARGUMENT);
            /* Set ERL config - global configuration*/
            addr = FM10000_CM_GLOBAL_CFG();
            FM_FLAG_TAKE_REG_LOCK(sw);
            /* Read the register */
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Update the field */
            FM_SET_FIELD(regValue, 
                         FM10000_CM_GLOBAL_CFG, 
                         ifgPenalty, 
                         val);
            err = switchPtr->WriteUINT32(sw, addr, regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT( FM_LOG_CAT_QOS, err )

}   /* end fm10000SetSwitchQOS */




/*****************************************************************************/
/** fm10000GetSwitchQOS
 * \ingroup intQos
 *
 * \desc            Get a switch global QOS attribute.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       attr is the port QoS attribute (see 'Port QoS Attributes')
 *                  to get.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Port QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[in]       value points to the attribute value to get.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the specified argument is not
 *                  valid.
 * \return          FM_ERR_INVALID_ATTRIB if the specified attribute is not a
 *                  recognized attribute.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GetSwitchQOS(fm_int sw,
                              fm_int attr,
                              fm_int index,
                              void * value)
{

    fm_uint32           regValue;
    fm_uint64           regValue64;
    fm_uint32           addr;
    fm_uint32           val;
    fm_uint32           temp;
    fm_status           err;
    fm_switch *         switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d, attr=%d, index=%d, value=%p\n",
                 sw, 
                 attr, 
                 index, 
                 (void *) value);

    /* Initialize variables */
    val = 0;
    err = FM_OK;

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Take action for each attribute */
    switch (attr)
    {
        case FM_AUTO_PAUSE_MODE:
            /* Get the requested value */
            *( (fm_bool *) value ) = switchPtr->autoPauseMode;
            break;

        case FM_QOS_DSCP_SWPRI_MAP:
            /* Check if index not exceeding max of DSCP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_DSCP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_DSCP_PRI_MAP(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            *( (fm_uint32 *) value ) = FM_GET_FIELD(regValue, 
                                                    FM10000_DSCP_PRI_MAP, 
                                                    pri);
            break;

        case FM_QOS_GLOBAL_USAGE:
            /* Read the register */
            addr = FM10000_CM_GLOBAL_USAGE();
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            *( (fm_uint32 *) value ) = FM_GET_FIELD(regValue, 
                                                    FM10000_CM_GLOBAL_USAGE, 
                                                    count) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_PRIV_WM:
            /* Read the register */
            addr = FM10000_CM_GLOBAL_WM();
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            *( (fm_uint32 *) value ) = FM_GET_FIELD(regValue, 
                                                 FM10000_CM_GLOBAL_WM, 
                                                 watermark) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHARED_PAUSE_OFF_WM:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_SHARED_SMP_PAUSE_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            *( (fm_uint32 *) value ) = FM_GET_FIELD(regValue, 
                                                 FM10000_CM_SHARED_SMP_PAUSE_WM,
                                                 PauseOff) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHARED_PAUSE_ON_WM:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_SHARED_SMP_PAUSE_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            *( (fm_uint32 *) value ) = FM_GET_FIELD(regValue, 
                                                 FM10000_CM_SHARED_SMP_PAUSE_WM,
                                                 PauseOn) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHARED_PRI_WM:
            /* Check if index not exceeding max of SWPRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_SHARED_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            *( (fm_uint32 *) value ) = FM_GET_FIELD(regValue, 
                                                 FM10000_CM_SHARED_WM,
                                                 watermark) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHARED_SOFT_DROP_WM:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_SOFTDROP_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_SOFTDROP_WM, 
                             SoftDropSegmentLimit) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHARED_SOFT_DROP_WM_HOG:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_SOFTDROP_WM(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_SOFTDROP_WM, 
                             HogSegmentLimit) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SHARED_SOFT_DROP_WM_JITTER:
            /* Check if index not exceeding max of SW PRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_APPLY_SOFTDROP_CFG(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_APPLY_SOFTDROP_CFG, 
                             JitterBits);
            break;

        case FM_QOS_SHARED_USAGE:
            /* Check if index not exceeding max of SMP available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SMP, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_SHARED_SMP_USAGE(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value - convert to bytes */
            ( *( (fm_uint32 *) value ) ) =
                FM_GET_FIELD(regValue, 
                             FM10000_CM_SHARED_SMP_USAGE, 
                             count) * BYTES_PER_SMP_SEG;
            break;

        case FM_QOS_SWPRI_TC_MAP:
            /* Check if index not exceeding max of SWPRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_SW_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_APPLY_SWITCH_PRI_TO_TC(0);
            err = switchPtr->ReadUINT64(sw, addr, &regValue64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = 
                FM_GET_UNNAMED_FIELD64(regValue64,
                               (index * FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc),
                                        FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc);
            break;

        case FM_QOS_TC_SMP_MAP:
            /* Check if index not exceeding max of TC available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_TC, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_CM_APPLY_TC_TO_SMP();
            err = switchPtr->ReadUINT32(sw, addr, &regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            /* Get the requested value */
            val = FM_GET_UNNAMED_FIELD(regValue, index, 1);
            switch (val)
            {
                case 0:
                    temp = FM_QOS_TC_SMP_0;
                    break;
                case 1:
                    temp = FM_QOS_TC_SMP_1;
                    break;
                default:
                    temp = FM_QOS_TC_SMP_INVALID;
                    break;
            }
            *( (fm_uint32 *) value ) = temp;
            break;

        case FM_QOS_TRAP_CLASS_SWPRI_MAP:
            /* Get switch priority mapping for trap class */
            err = GetTrapClassMap(sw, index, &val);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            *( (fm_uint32 *) value ) = val;
            break;

        case FM_QOS_VPRI_SWPRI_MAP:
            /* Check if index not exceeding max of VLANPRI available */
            FM10000_QOS_ABORT_ON_EXCEED_OR_LESS_0(index, 
                                                  FM10000_QOS_MAX_VLAN_PRI, 
                                                  err, 
                                                  FM_ERR_INVALID_ARGUMENT);
            /* Read the register */
            addr = FM10000_VPRI_PRI_MAP(index);
            err = switchPtr->ReadUINT32(sw, addr, &regValue); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS_FM4000, err);
            /* Get the requested value */
            ( *( (fm_uint32 *) value ) ) = FM_GET_FIELD(regValue, 
                                                         FM10000_VPRI_PRI_MAP, 
                                                         pri);
            break;

        case FM_QOS_SWITCH_IFG_PENALTY:
            /* Read ERL config - global configuration*/
            addr = FM10000_CM_GLOBAL_CFG();
            /* Read the register */
            err = switchPtr->ReadUINT32(sw, addr, &regValue);
             FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
             /* Get the requested value */
             ( *( (fm_uint32 *) value ) ) = FM_GET_FIELD(regValue, 
                                                      FM10000_CM_GLOBAL_CFG,
                                                      ifgPenalty);
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

ABORT:

    FM_LOG_EXIT( FM_LOG_CAT_QOS, err )

}   /* end fm10000GetSwitchQOS */




/*****************************************************************************/
/** fm10000GetSMPPauseState
 * \ingroup intQos
 *
 * \desc            Retrieve the FM_PORT_SMP_LOSSLESS_PAUSE for all ports
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      pauseEnable is a bitmask array of size FM10000_MAX_PORT to
 *                  hold FM_PORT_SMP_LOSSLESS_PAUSE attribute of the ports
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GetSMPPauseState(fm_int sw, fm_uint32 *pauseEnable)
{
    fm_int      cpi;
    fm_int      port;
    fm_status   err;
    fm_switch   *switchPtr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw = %d smp_pause_en=%p",
                 sw,
                 (void *) pauseEnable);

    if (pauseEnable == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_ERR_INVALID_ARGUMENT);
    }

    /* Get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Get (through all ports) SMP_LOSSLESS_PAUSE bit indication */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        err = fm10000GetPortAttribute(sw, 
                                      port, 
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_SMP_LOSSLESS_PAUSE, 
                                      &pauseEnable[cpi]);
        if (err != FM_OK)
        {
            /* Attribute not criticl for wm scheme configuration, if ports are 
               not ready just report warning and set default to zero */
            FM_LOG_WARNING(FM_LOG_CAT_QOS, 
                           "Unable to get FM_PORT_SMP_LOSSLESS_PAUSE attribute "
                            "for port %d, setting to disabled.\n", cpi);
            pauseEnable[cpi] = 0;
        }
    }

    /* Return OK regardless of port attributes getting results */
    err = FM_OK;

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000GetSMPPauseState */




/*****************************************************************************/
/** fm10000GetPauseResendInterval
 * \ingroup intQos
 *
 * \desc            Calculate and return the pause resend interval in
 *                  nanoseconds based on the specified pause resend interval
 *                  time value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port (logical) to operate on.
 *
 * \param[out]      timeNs is the time in nanoseconds for the specified time
 *                  value and switch configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the specified argument is not
 *                  valid.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GetPauseResendInterval(fm_int      sw,
                                        fm_int      port,
                                        fm_uint32 * timeNs)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_int              physPort;
    fm_uint32           addr;
    fm_uint32           reg;
    fm_float            FHClock;
    fm_uint32           nofSweeperPorts;
    fm_uint32           resendInterval;
    fm_bool             regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d\n",
                 sw,
                 port);

    /* Check and initialize output */
    if ( timeNs == NULL) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }
    *timeNs = 0;

    /* Get the switch's pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Map logical por to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Get reference clock - frequency in MHz*/
    err = fm10000ComputeFHClockFreq(sw, &FHClock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Get number of sweeper ports */
    addr = FM10000_CM_GLOBAL_CFG();
    err = switchPtr->ReadUINT32(sw, addr, &reg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    nofSweeperPorts= FM_GET_FIELD(reg, FM10000_CM_GLOBAL_CFG, NumSweeperPorts);

    /* Finally read resend value */
    addr = FM10000_CM_PAUSE_RESEND_INTERVAL(physPort);
    err = switchPtr->ReadUINT32(sw, addr, &reg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    resendInterval = FM_GET_FIELD(reg, 
                                  FM10000_CM_PAUSE_RESEND_INTERVAL, 
                                  INTERVAL);

    /* Calculate resend time [ns] */
    *timeNs = nofSweeperPorts * (1e3 / FHClock) * resendInterval;

    FM_LOG_DEBUG(FM_LOG_CAT_QOS, 
                 "Pause interval[ns] =%d = interval(%d) * nofSweepPorts(%d) * "
                 "1000 / FHClock(%f)[MHz]\n",
                 *timeNs, 
                 resendInterval, 
                 nofSweeperPorts, 
                 FHClock);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000GetPauseResendInterval */




/*****************************************************************************/
/** fm10000SetPauseResendInterval
 * \ingroup intQos
 *
 * \desc            Calculate and set the pause resend interval based on 
 *                  specified pause resend interval value in nanoseconds.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port (logical) to operate on.
 *
 * \param[in]       timeNs is the time in nanoseconds for the specified time
 *                  value and switch configuration.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000SetPauseResendInterval(fm_int      sw,
                                        fm_int      port,
                                        fm_uint32   timeNs)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_int              physPort;
    fm_uint32           addr;
    fm_uint32           reg;
    fm_float            FHClock;
    fm_uint32           nofSweeperPorts;
    fm_uint32           resendInterval;
    fm_bool             regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d, timeNs=%d\n",
                 sw,
                 port,
                 timeNs);

    /* Get the switch's pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* Map logical por to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Get reference clock - frequency in MHz*/
    err = fm10000ComputeFHClockFreq(sw, &FHClock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Get number of sweeper ports */
    addr = FM10000_CM_GLOBAL_CFG();
    err = switchPtr->ReadUINT32(sw, addr, &reg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    nofSweeperPorts= FM_GET_FIELD(reg, FM10000_CM_GLOBAL_CFG, NumSweeperPorts);


    /* Finally set resend value */
    resendInterval = (timeNs * FHClock) / (1e3 * nofSweeperPorts );
    resendInterval &= 0xFFFF;
    addr = FM10000_CM_PAUSE_RESEND_INTERVAL(physPort);
    err = switchPtr->WriteUINT32(sw, addr, resendInterval);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_LOG_DEBUG(FM_LOG_CAT_QOS, 
                 "Pause interval =%d = timeInterval[ns](%d) * FHClock(%f)[MHz]"
                 " / nofSweepPorts(%d) * 1000\n",
                 resendInterval,
                 timeNs, 
                 FHClock, 
                 nofSweeperPorts);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000SetPauseResendInterval */




/*****************************************************************************/
/** fm10000SetPauseQuantaCoefficients
 * \ingroup intQoS
 *
 * \desc            Set the pause quanta values (PauseQuantaMultiplier and 
 *                  PauseQuantaDivisor) according to the port speed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port (logical) to operate on.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000SetPauseQuantaCoefficients(fm_int sw, fm_int port)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm10000_port *      portExt;
    fm_int              physPort;
    fm_uint32           addr;
    fm_uint32           reg;
    fm_uint32           pauseQuantaMultiplier;
    fm_uint32           pauseQuantaDivisor;
    fm_bool             regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d\n",
                 sw,
                 port);

    err = FM_OK;

    /* Get the switch's and port's pointer */
    switchPtr = GET_SWITCH_PTR(sw);
    portExt   = GET_PORT_EXT(sw, port);

    /* Map logical por to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Initialize coefficients to defaults (setting for 10G speed )*/
    pauseQuantaMultiplier = 160;
    pauseQuantaDivisor = 4;

    /* Specify coefficients based on port's speed */
    switch (portExt->speed)
    {
        case 100000: /* 100G */
            pauseQuantaMultiplier = 64;
            pauseQuantaDivisor = 16;
            break;
        case 40000:  /* 40G */
            pauseQuantaMultiplier = 160;
            pauseQuantaDivisor = 16;
            break;
        case 25000:  /* 25G */
            pauseQuantaMultiplier = 160;
            pauseQuantaDivisor = 10;
            break;
        case 10000:  /* 10G */
            /* Defaults already set */
            break;
        case 1000:   /* 1G */
            pauseQuantaMultiplier = 400;
            pauseQuantaDivisor = 1;
            break;
        case 100:    /* 100M */
            pauseQuantaMultiplier = 4000;
            pauseQuantaDivisor = 1;
            break;
        case 10:     /* 10M */
            pauseQuantaMultiplier = 40000;
            pauseQuantaDivisor = 1;
            break;
        default: 
            FM_LOG_DEBUG(FM_LOG_CAT_QOS,
                         "Unsupported speed = %d setting default value \n",
                         portExt->speed);
            break;
    }

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Read PAUSE CFG register */
    addr = FM10000_CM_PAUSE_CFG(physPort);
    err = switchPtr->ReadUINT32(sw, addr, &reg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Set coefficients fields */
    FM_SET_FIELD(reg, 
                 FM10000_CM_PAUSE_CFG, 
                 PauseQuantaMultiplier, 
                 pauseQuantaMultiplier);

    FM_SET_FIELD(reg, 
                 FM10000_CM_PAUSE_CFG, 
                 PauseQuantaDivisor, 
                 pauseQuantaDivisor);

    /* Write the register */
    err = switchPtr->WriteUINT32(sw, addr, reg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000SetPauseQuantaCoefficients */




/*****************************************************************************/
/** fm10000InitializeCM
 * \ingroup intQos
 *
 * \desc            Initializes Congestion Management unit during boot time.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000InitializeCM(fm_int sw)
{
        fm_status           err;
        fm_int              i;
        fm_uint32           rv;
        fm_uint32           refDiv;
        fm_uint32           outDiv;
        fm_uint32           fbDiv4;
        fm_uint32           fbDiv255;
        fm_switch *         switchPtr;
        fm10000_switch *    switchExt;
        fm_bool             regLockTaken;
        fm_uint32           rvMult[4] = {0,0,0,0};
    
        FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);
    
        err       = FM_OK;
        regLockTaken = FALSE;
        switchPtr = GET_SWITCH_PTR(sw);
        switchExt = GET_SWITCH_EXT(sw);
    
        FM_FLAG_TAKE_REG_LOCK(sw);
    
        /* Clear CM_PAUSE_RCV_STATE which is not covered during BIST memory init */
        for (i = 0; i < FM10000_CM_PAUSE_RCV_STATE_ENTRIES; i++)
        {
            err = switchPtr->WriteUINT32Mult(sw, 
                                             FM10000_CM_PAUSE_RCV_STATE(i, 0), 
                                             FM10000_CM_PAUSE_RCV_STATE_WIDTH,  
                                             rvMult);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        }
    
        /* Set the global watermark to a conservative value until initialized by
         * QOS API. 
         * wm = maxNumSegs - maxInFlightSegs - 256 */
        rv = FM10000_MAX_MEMORY_SEGMENTS - 
             (FM10000_NUM_PORTS * (FM10000_MAX_FRAME_SIZE / FM10000_SEGMENT_SIZE)) -
            FM10000_QOS_NUM_RESERVED_SEGS;
    
        err = switchPtr->WriteUINT32(sw, FM10000_CM_GLOBAL_WM(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    
        /* Set PAUSE base frequency */
        err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_CTRL(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
        
        refDiv = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, RefDiv);
        fbDiv4 = FM_GET_BIT(rv, FM10000_PLL_FABRIC_CTRL, FbDiv4);
        fbDiv255 = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, FbDiv255);
        outDiv = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, OutDiv);
    
        rv = 0;
        FM_SET_FIELD(rv, 
                     FM10000_CM_PAUSE_BASE_FREQ, 
                     M, 
                     (fbDiv255 * (2 * (fbDiv4 + 1) )) );
    
        FM_SET_FIELD(rv, 
                     FM10000_CM_PAUSE_BASE_FREQ, 
                     N, 
                     (refDiv * outDiv) );
        
        err = switchPtr->WriteUINT32(sw, FM10000_CM_PAUSE_BASE_FREQ(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    
        /* Enable CM Sweepers */
        err = switchPtr->ReadUINT32(sw, FM10000_CM_GLOBAL_CFG(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    
        FM_SET_BIT(rv, FM10000_CM_GLOBAL_CFG, WmSweeperEnable, 1);
        FM_SET_BIT(rv, FM10000_CM_GLOBAL_CFG, PauseGenSweeperEnable, 1);
        FM_SET_BIT(rv, FM10000_CM_GLOBAL_CFG, PauseRecSweeperEnable, 1);
        FM_SET_FIELD(rv, FM10000_CM_GLOBAL_CFG, NumSweeperPorts, FM10000_NUM_PORTS);
    
        err = switchPtr->WriteUINT32(sw, FM10000_CM_GLOBAL_CFG(), rv);
    
    ABORT:
        if (regLockTaken)
        {
            DROP_REG_LOCK(sw);
        }
    
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000InitializeCM */




/*****************************************************************************/
/** fm10000InitQOS
 * \ingroup intQos
 *
 * \desc            Initializes QOS to defaults.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000InitQOS(fm_int sw)
{
    fm_status       err = FM_OK;
    fm_bool         autoPauseMode;

    /* Setting FM_AUTO_PAUSE_MODE will initialize the watermarks to their
     * defaults based on the scheme in api.FM10000.wmSelect */
    autoPauseMode = TRUE;
    err = fm10000SetSwitchQOS(sw, FM_AUTO_PAUSE_MODE, 1, &autoPauseMode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000InitQOS */




/*****************************************************************************/
/** fm10000QOSPriorityMapperAllocateResources
 * \ingroup intQoS
 *
 * \desc            Allocates resources for the priority mapper.
 *
 * \note            It is assume that the caller holds the switch lock as
 *                  a writer for exclusive access during switch initialization.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if a memory request failed.
 *
 *****************************************************************************/
fm_status fm10000QOSPriorityMapperAllocateResources(fm_int sw)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_OK;
    fm_uint                      size;
    fm_int                       i;
    fm10000_internalPriorityMap *priorityMap;
    fm10000_priorityMapper *     priorityMapper;
    fm10000_priorityMapSet *     priorityMapSet;
    fm10000_switch *             switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    size = (fm_uint) sizeof(fm10000_priorityMapSet);

    /***************************************************
     * Allocate memory for the priority map set and
     * initialize this block of memory.
     **************************************************/
    switchExt->priorityMapSet = (fm10000_priorityMapSet *) fmAlloc(size);

    priorityMapSet = switchExt->priorityMapSet;

    if (priorityMapSet == NULL)
    {
        status = FM_ERR_NO_MEM;

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

    }

    memset((void *) priorityMapSet, 0, sizeof(fm10000_priorityMapSet));

    /***************************************************
     * Initialize all priority mappers and add the
     * priority mappers to the priority mapper pool.
     **************************************************/
    FM10000_PRIORITY_MAPPER_LIST_INITIALIZE(&(priorityMapSet->mapperPool));

    for (i = 0 ; i < FM10000_MAX_SWITCH_PRIORITIES; i++)
    {
        priorityMapper = &(priorityMapSet->mappers[i]);

        priorityMapper->id = i;

        FM10000_PRIORITY_MAPPER_LIST_MAP_INITIALIZE(&(priorityMapper->maps));

        FM10000_PRIORITY_MAPPER_ENTRY_PUSH(&(priorityMapSet->mapperPool),
                                           priorityMapper);
    }   

    /***************************************************
     * Instantiate all priority maps.
     **************************************************/
    FM10000_PRIORITY_MAPPER_LIST_MAP_INITIALIZE(&(priorityMapSet->maps));

    size = (fm_uint) sizeof(fm10000_internalPriorityMap);

    for (i = 0 ; i < FM10000_QOS_TRAP_CLASS_MAX ; i++)
    {
        priorityMap = (fm10000_internalPriorityMap *) fmAlloc(size);

        if (priorityMap == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, status);

        }   /* end if (priorityMap == NULL) */

        priorityMap->trapClass = i;
        priorityMap->priority   = FM_QOS_SWPRI_DEFAULT;
        priorityMap->mapper     = FM10000_INVALID_PRIORITY_MAPPER;

        FM10000_PRIORITY_MAPPER_ENTRY_MAP_PUSH(&(priorityMapSet->maps),
                                               FM10000_PRI_MAP_LIST_POOL,
                                               priorityMap);
    }   

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end fm10000QOSPriorityMapperAllocateResources */




/*****************************************************************************/
/** fm10000QOSPriorityMapperFreeResources
 * \ingroup intQoS
 *
 * \desc            Frees the resources that have been allocated for the
 *                  priority mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000QOSPriorityMapperFreeResources(fm_int sw)
{
    fm_switch *                  switchPtr;
    fm_status                    status = FM_OK;
    fm10000_internalPriorityMap  *nextMap;
    fm10000_internalPriorityMap  *priorityMap;
    fm10000_switch *             switchExt;
    fm_int                       i;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    if (switchExt->priorityMapSet == NULL)
    {
        /* Either not created yet or already freed */
        FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_OK);
    }

    /* Free all priority mappers. This should return all
       priority maps to the map pool */
    for (i = 0 ; i < FM10000_MAX_SWITCH_PRIORITIES ; i++)
    {
        if (switchExt->priorityMapSet->mappers[i].used)
        {
            status = PriorityMapperFree(sw, i);
            if (status != FM_OK)
            {
                /* just log the error and continue */
                FM_LOG_ERROR(FM_LOG_CAT_QOS,
                             "Fail to free mapper priority %d: %s\n",
                             i, fmErrorMsg(status)); 
            }
        }
    }

    priorityMap = 
      FM10000_PRIORITY_MAPPER_FIRST_MAP_GET(&(switchExt->priorityMapSet->maps));

    while (priorityMap != NULL)
    {

        nextMap = FM10000_PRIORITY_MAPPER_NEXT_MAP_GET(priorityMap,
                                                     FM10000_PRI_MAP_LIST_POOL);
        fmFree((void *) priorityMap);
        priorityMap = nextMap;

    }

    fmFree(switchExt->priorityMapSet);
    switchExt->priorityMapSet = NULL;

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT(FM_LOG_CAT_QOS, status);

}   /* end fm10000QOSPriorityMapperFreeResources */




/*****************************************************************************/
/** fm10000AddQueueQOS
 * \ingroup intQos
 *
 * \desc            Creates qos queue for specified port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       param points to a ''fm_qosQueueParam'' structure containing
 *                  parameters of the requested queue.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any argument is invalid.
 * \return          FM_ERR_NO_FREE_RESOURCES if no resources avialable.
 *
 *****************************************************************************/
fm_status fm10000AddQueueQOS(fm_int sw,
                             fm_int port,
                             fm_qosQueueParam *param)
{
    fm_switch *switchPtr;
    fm10000_switch  * switchExt;
    fm10000_eschedConfigPerPort *eschedPtr;
    fm_int physPort;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d param=%p\n",
                 sw,
                 port,
                 (void*)param);

    err = FM_OK;

    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Map logical port to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);
    }

    /* Get pointer to esched configuration  */
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );

    /* General queue parameters verification */
    err = EschedNewQosQueueParamVerify(sw, port, eschedPtr, param);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    /* Configure new queue */
    FM_LOG_INFO(FM_LOG_CAT_QOS, 
                "Adding new qos queue on port %d: TC=%d, %d < Bw[Mb/s] < %d \n",
                port,
                param->tc,
                param->minBw,
                param->maxBw);

    err = EschedQosQueueConfig(sw, physPort, FM10000_QOS_ADD_QUEUE, param);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000AddQueueQOS */




/*****************************************************************************/
/** fm10000DeleteQueueQOS
 * \ingroup intQos
 *
 * \desc            Deletes qos queue for specified port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       queueId is the queue Id (unique for the port).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_NOT_FOUND if the queue was not found.
 * \return          FM_ERR_INVALID_ARGUMENT if any argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000DeleteQueueQOS(fm_int sw,
                                fm_int port,
                                fm_int queueId)
{
    fm_switch * switchPtr;
    fm10000_switch  * switchExt;
    fm10000_eschedConfigPerPort *eschedPtr;
    fm_int physPort;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d queueId=%d\n",
                 sw,
                 port,
                 queueId);
                 
    err = FM_OK;

    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Map logical port to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    if (err != FM_OK)
    {    
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);
    }

    /* Get pointer to esched configuration  */
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );

    /* Return error when the mode incorrect */
    if (!eschedPtr->qosQueueEnabled)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if nof queue is zero  */
    if (eschedPtr->qosQueuesConfig.numQoSQueues == 0)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if queue exists  */
    if (eschedPtr->qosQueuesConfig.qosQueuesParams[queueId].queueId == 
                                                     FM_QOS_QUEUE_ID_DEFAULT)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Delete the queue, update the configuration  */
    err = EschedQosQueueConfig(sw, 
                          physPort, 
                          FM10000_QOS_DEL_QUEUE, 
                         &eschedPtr->qosQueuesConfig.qosQueuesParams[queueId]);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000DeleteQueueQOS */




/*****************************************************************************/
/** fm10000SetAttributeQueueQOS
 * \ingroup intQos
 *
 * \desc            Set a QoS queue's attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       queueId is the queue Id.
 *
 * \param[in]       attr is the QoS queue attribute (see ''fm_qosQueueAttr'')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if queueId or value is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetAttributeQueueQOS(fm_int sw,
                                      fm_int port,
                                      fm_int queueId, 
                                      fm_int attr, 
                                      void  *value)
{
    fm_switch *switchPtr;
    fm10000_switch  * switchExt;
    fm10000_eschedConfigPerPort *eschedPtr;
    fm_int          physPort;
    fm_qosQueueParam param;
    fm_uint32 val;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d queueId=%d attr=%d value=%p\n",
                 sw,
                 port,
                 queueId,
                 attr,
                 value);

    err = FM_OK;
    
    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Map logical port to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    if (err != FM_OK)
    {    
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);
    }

    /* Get pointer to esched configuration  */
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );

    /* Return error when the mode incorrect */
    if (!eschedPtr->qosQueueEnabled)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if nof queue is zero  */
    if (eschedPtr->qosQueuesConfig.numQoSQueues == 0)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if queue exists  */
    if (eschedPtr->qosQueuesConfig.qosQueuesParams[queueId].queueId == 
                                                     FM_QOS_QUEUE_ID_DEFAULT)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Update the queue, update the configuration  */
    param.queueId = queueId;

    switch (attr)
    {
        case FM_QOS_QUEUE_MIN_BW:
            val = *(fm_uint32 *) value;
            param.minBw = val;
            /* Configure queue */
            err = EschedQosQueueConfig(sw, 
                                  physPort, 
                                  FM10000_QOS_SET_QUEUE_MIN_BW, 
                                  &param);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            
        break;

        case FM_QOS_QUEUE_MAX_BW:
            val = *(fm_uint32 *) value;
            param.maxBw = val;
            /* Configure queue */
            err = EschedQosQueueConfig(sw, 
                                  physPort, 
                                  FM10000_QOS_SET_QUEUE_MAX_BW, 
                                  &param);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            
        break;

        case FM_QOS_QUEUE_TRAFFIC_CLASS:
            val = *(fm_uint32 *) value;
            FM10000_QOS_ABORT_ON_EXCEED(val, 
                                        FM10000_QOS_MAX_TC, 
                                        err, 
                                        FM_ERR_INVALID_VALUE);
            param.tc = (fm_int)val;
            /* Configure queue */
            err = EschedQosQueueConfig(sw, 
                                  physPort, 
                                  FM10000_QOS_SET_QUEUE_TRAFFIC_CLASS, 
                                  &param);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
            
        break;

        default:
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, FM_ERR_INVALID_ATTRIB);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000SetAttributeQueueQOS */




/*****************************************************************************/
/** fm10000GetAttributeQueueQOS
 * \ingroup intQos
 *
 * \desc            Get a QoS queue's attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       queueId is the queue Id.
 *
 * \param[in]       attr is the QoS queue attribute (see ''fm_qosQueueAttr'')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to get.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if queueId or value is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetAttributeQueueQOS(fm_int sw,
                                      fm_int port,
                                      fm_int queueId, 
                                      fm_int attr, 
                                      void  *value)
{
    fm_switch *switchPtr;
    fm10000_switch  * switchExt;
    fm10000_eschedConfigPerPort *eschedPtr;
    fm_int          physPort;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS,
                 "sw=%d port=%d queueId=%d attr=%d value=%p\n",
                 sw,
                 port,
                 queueId,
                 attr,
                 value);

    err = FM_OK;
    
    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Map logical port to physical*/
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    if (err != FM_OK)
    {    
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err);
    }

    /* Get pointer to esched configuration  */
    eschedPtr = &( switchExt->eschedConfig[ physPort ] );

    /* Return error when the mode incorrect */
    if (!eschedPtr->qosQueueEnabled)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if nof queue is zero  */
    if (eschedPtr->qosQueuesConfig.numQoSQueues == 0)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if queueId in valid range is zero  */
    if ((queueId < 0) || (queueId >= FM_MAX_TRAFFIC_CLASSES))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    /* Check if queueId is non default (queue was added)  */
    if (eschedPtr->qosQueuesConfig.qosQueuesParams[queueId].queueId == 
                                                FM_QOS_QUEUE_ID_DEFAULT)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_EXIT(FM_LOG_CAT_QOS, err)
    }

    switch (attr)
    {
        case FM_QOS_QUEUE_MIN_BW:
            *( (fm_uint32 *) value ) = 
                eschedPtr->qosQueuesConfig.qosQueuesParams[queueId].minBw;
            break;
        
        case FM_QOS_QUEUE_MAX_BW:
            *( (fm_uint32 *) value ) = 
                eschedPtr->qosQueuesConfig.qosQueuesParams[queueId].maxBw;
            break;

        case FM_QOS_QUEUE_TRAFFIC_CLASS:
            *( (fm_int *) value ) = 
                (fm_int)eschedPtr->qosQueuesConfig.qosQueuesParams[queueId].tc;
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000GetAttributeQueueQOS */




/*****************************************************************************/
/** fm10000DbgDumpQueueQOS
 * \ingroup intDiagQos
 *
 * \desc            Dumps QOS queues parameters
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpQueueQOS(fm_int sw)
{

    fm_status                       err;
    fm_switch *                     switchPtr;
    fm10000_switch *                switchExt;
    fm10000_eschedConfigPerPort *   eschedPtr;
    fm_int                          physPort;
    fm_int                          logPort;
    fm_int                          cpi;
    fm_int                          qId;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d \n", sw);

    err = FM_OK;

    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    FM_LOG_PRINT(" QOS queues for sw=%d:\n", sw);
    FM_LOG_PRINT("+------+----+----+-------------+-------------+\n");
    FM_LOG_PRINT("| Port | ID | TC | minBw[Mb/s] | maxBw[Mb/s] |\n");
    FM_LOG_PRINT("+------+----+----+-------------+-------------+\n");

    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* Get pointer to esched configuration  */
        eschedPtr = &( switchExt->eschedConfig[ physPort ] );

        if (eschedPtr->qosQueueEnabled)
        {
            if ( eschedPtr->qosQueuesConfig.numQoSQueues == 0 )
            {
                FM_LOG_PRINT("|  %2d  |                 -                   |\n",
                             logPort);
                FM_LOG_PRINT("+------+----+----+-------------+-------------+\n");
            }
            else
            {
                for (qId = 0;
                      qId < eschedPtr->qosQueuesConfig.numQoSQueues;
                      qId++)
                {
                    FM_LOG_PRINT("|  %2d  | %2d | %2d |   %6d    |   %6d    |\n",
                       logPort,
                       eschedPtr->qosQueuesConfig.qosQueuesParams[qId].queueId,
                       eschedPtr->qosQueuesConfig.qosQueuesParams[qId].tc,
                       eschedPtr->qosQueuesConfig.qosQueuesParams[qId].minBw,
                       eschedPtr->qosQueuesConfig.qosQueuesParams[qId].maxBw);
                    FM_LOG_PRINT("+------+----+----+-------------+-------------+\n");
                }
            }
       }
       else
       {
           FM_LOG_PRINT("|  %2d  |              disabled               |\n",
                        logPort);
            
            FM_LOG_PRINT("+------+----+----+-------------+-------------+\n");
       }
       
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000DbgDumpQueueQOS */




/*****************************************************************************/
/** fm10000DbgDumpMemoryUsage 
 * \ingroup intQos
 *
 * \desc            Dumps the memory usage counters for all ports.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpMemoryUsage(fm_int sw)
{
    fm_uint32     total;
    fm_uint32     sharedSmp0;
    fm_uint32     sharedSmp1;
    fm_switchInfo info;
    fm_int        cpi;
    fm_int        port;
    fm_int        physPort;
    fm_int        tc;
    fm_uint32     rxSmp0;
    fm_uint32     rxSmp1;
    fm_uint32     txTC[8];
    fm_switch    *switchPtr = GET_SWITCH_PTR(sw);


    fmGetSwitchInfo(sw, &info);
    fmGetSwitchQOS(sw, FM_QOS_GLOBAL_USAGE, 0, &total);
    fmGetSwitchQOS(sw, FM_QOS_SHARED_USAGE, 0, &sharedSmp0);
    fmGetSwitchQOS(sw, FM_QOS_SHARED_USAGE, 1, &sharedSmp1);


    FM_LOG_PRINT("Memory usage for Sw %d when Total Memory = %4d \n", 
                  sw,
                  info.memorySize);

    FM_LOG_PRINT("|------+-------------+-------------|\n");
    FM_LOG_PRINT("| Port |    SMP 0    |    SMP 1    |\n");
    FM_LOG_PRINT("|------+-------------+-------------|\n");
    FM_LOG_PRINT("| Total memory usage:              |\n");
    FM_LOG_PRINT("|  -   |          %7d          |\n", 
                 total);
    FM_LOG_PRINT("|------+-------------+-------------|\n");
    FM_LOG_PRINT("| Shared memory partition usage   :|\n");
    FM_LOG_PRINT("|  -   |    %5d    |    %5d    |\n", 
                 sharedSmp0,
                 sharedSmp1);
    FM_LOG_PRINT("|------+-------------+-------------|\n");
    FM_LOG_PRINT("| Received memory usage :          |\n");

    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);

        fmGetPortQOS(sw, port, FM_QOS_RX_SMP_USAGE, 0, &rxSmp0);
        rxSmp0 /= 1024;
        fmGetPortQOS(sw, port, FM_QOS_RX_SMP_USAGE, 1, &rxSmp1);
        rxSmp1 /= 1024;

        FM_LOG_PRINT("|  %2d  |    %5d    |    %5d    |\n", 
                     port,
                     rxSmp0,
                     rxSmp1);
    }
    FM_LOG_PRINT("|------+-------------+-------------|\n");

    FM_LOG_PRINT("|------+------+------+------+------+------+------+------"
                 "+------|\n");
    FM_LOG_PRINT("| Memory usage per traffic class and tx port:           "
                 "       |\n");
    FM_LOG_PRINT("|------+------+------+------+------+------+------+------"
                 "+------|\n");
    FM_LOG_PRINT("| Port | TC:0 | TC:1 | TC:2 | TC:3 | TC:4 | TC:5 | TC:6 "
                 "| TC:7 |\n");
    FM_LOG_PRINT("|------+------+------+------+------+------+------+------"
                 "+------|\n");
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);

        for (tc = 0 ; tc < 8 ; tc++)
        {
            fmGetPortQOS(sw, port, FM_QOS_TX_TC_USAGE, tc, &txTC[tc]);
            txTC[tc] /= 1024;
        }

        FM_LOG_PRINT("|  %2d  | %4d | %4d | %4d | %4d | %4d | %4d | %4d "
                     "| %4d |\n",
                     port, txTC[0], txTC[1], txTC[2], txTC[3], txTC[4], 
                     txTC[5], txTC[6], txTC[7]);
    }
    FM_LOG_PRINT("|------+------+------+------+------+------+------+------"
                 "+------|\n");
    return FM_OK;

}   /* end fm10000DbgDumpMemoryUsage */




/*****************************************************************************/
/** fm10000DbgDumpWatermarks 
 * \ingroup intQos
 *
 * \desc            Dumps the watermark values for all ports.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpWatermarks(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_uint32       globalWm;
    fm_uint32       globalPauseOnWm;
    fm_uint32       globalPauseOffWm;
    fm_uint32       globalPauseOnWm1;
    fm_uint32       globalPauseOffWm1;
    fm_uint32       globalSharedWm[16];
    fm_int          swPri;
    fm_int          smp;
    fm_int          cpi;
    fm_int          port;
    fm_int          tc;
    fm_uint32       rxPrivWm;
    fm_uint32       rxHogWm;
    fm_uint32       rxPauseOnWm;
    fm_uint32       rxPauseOffWm;
    fm_uint32       txtcPrivWm[8];
    fm_uint32       txHogWm;

    switchPtr = GET_SWITCH_PTR(sw);

    fmGetSwitchQOS(sw, FM_QOS_PRIV_WM, 0, &globalWm);
    fmGetSwitchQOS(sw, FM_QOS_SHARED_PAUSE_ON_WM, 0, &globalPauseOnWm);
    fmGetSwitchQOS(sw, FM_QOS_SHARED_PAUSE_OFF_WM, 0, &globalPauseOffWm);

    fmGetSwitchQOS(sw, FM_QOS_SHARED_PAUSE_ON_WM, 1, &globalPauseOnWm1);
    fmGetSwitchQOS(sw, FM_QOS_SHARED_PAUSE_OFF_WM, 1, &globalPauseOffWm1);

    /* Global watermarks */
    FM_LOG_PRINT("Global Wm: %d  SMP0 Global Pause On: %d  SMP0 Global Pause Off: %d\n",
                 globalWm/1024, 
                 globalPauseOnWm/1024, 
                 globalPauseOffWm/1024);

    FM_LOG_PRINT("                 SMP1 Global Pause On: %d  SMP1 Global Pause Off: %d\n",
                 globalPauseOnWm1/1024, 
                 globalPauseOffWm1/1024);

    /* Soft drop watermarks per swpri */
    for ( swPri = 0 ; swPri < 16 ; swPri++ )
    {
        fmGetSwitchQOS(sw, FM_QOS_SHARED_PRI_WM, swPri, &globalSharedWm[swPri]);
    }

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("                     Global Shared Wm per Switch Priority\n");
    FM_LOG_PRINT("   0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15\n");
    FM_LOG_PRINT("-------------------------------------------------------------------------------\n");
    FM_LOG_PRINT("%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n",
                 globalSharedWm[0]/1024, globalSharedWm[1]/1024,
                 globalSharedWm[2]/1024, globalSharedWm[3]/1024,
                 globalSharedWm[4]/1024, globalSharedWm[5]/1024,
                 globalSharedWm[6]/1024, globalSharedWm[7]/1024,
                 globalSharedWm[8]/1024, globalSharedWm[9]/1024,
                 globalSharedWm[10]/1024, globalSharedWm[11]/1024,
                 globalSharedWm[12]/1024, globalSharedWm[13]/1024,
                 globalSharedWm[14]/1024, globalSharedWm[15]/1024);

    /* Shared partition watermarks per smp */

    for ( smp = 0 ; smp <= 1 ; smp++ )
    {
        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("Po |  Rx SMP%d  |  Rx Pause | Tx SMP%d |                 TX-TC Priv             \n", smp, smp);
        FM_LOG_PRINT("rt | Priv  Hog |   On  Off |   Hog   |  Tc0  Tc1  Tc2  Tc3  Tc4  Tc5  Tc6  Tc7\n");
        FM_LOG_PRINT("---+-----------+-----------+---------+----------------------------------------\n");

        for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
        {
            port = GET_LOGICAL_PORT(sw, cpi);

            fmGetPortQOS(sw, port, FM_QOS_RX_PRIVATE_WM, smp, &rxPrivWm);
            fmGetPortQOS(sw, port, FM_QOS_RX_HOG_WM, smp, &rxHogWm);

            for (tc = 0 ; tc < 8 ; tc++)
            {
                fmGetPortQOS(sw, port, FM_QOS_TX_TC_PRIVATE_WM, tc, &txtcPrivWm[tc]);
            }

            fmGetPortQOS(sw, port, FM_QOS_TX_HOG_WM, smp, &txHogWm);

            fmGetPortQOS(sw, port, FM_QOS_PRIVATE_PAUSE_ON_WM, smp, &rxPauseOnWm);
            fmGetPortQOS(sw, port, FM_QOS_PRIVATE_PAUSE_OFF_WM, smp, &rxPauseOffWm);

            FM_LOG_PRINT("%2d | %4d %4d | %4d %4d |  %4d   | %4d %4d %4d %4d %4d %4d %4d %4d\n",
                         port, rxPrivWm/1024, rxHogWm/1024,
                         rxPauseOnWm / 1024, rxPauseOffWm/1024,
                         txHogWm/1024,
                         txtcPrivWm[0]/1024, txtcPrivWm[1]/1024,
                         txtcPrivWm[2]/1024, txtcPrivWm[3]/1024,
                         txtcPrivWm[4]/1024, txtcPrivWm[5]/1024,
                         txtcPrivWm[6]/1024, txtcPrivWm[7]/1024);
        }
    }

    return FM_OK;

}   /* end fm10000DbgDumpWatermarks */




/*****************************************************************************/
/** fm10000DbgDumpWatermarksV2
 * \ingroup intDiagQos
 *
 * \desc            Dumps the congestion management watermarks
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       rxPort is the RX port to filter by.  If out of range, 
 *                  all RX ports will be selected.
 *                  This is the logical port.
 *
 * \param[in]       txPort is the TX port to filter by.  If out of range, 
 *                  all TX ports will be selected.
 *                  This is the logical port.
 *
 * \param[in]       smp is the shared memory partition to filter by. If out of 
 *                  range, all SMPs will be selected.
 *
 * \param[in]       txTc is the traffic class to filter by. If out of 
 *                  range, all TCs will be selected.
 *
 * \param[in]       islPri is the ISL priority to filter by. If out of range,
 *                  all ISL Priorities will be selected.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpWatermarksV3(fm_int sw, fm_int rxPort, fm_int txPort,
                                     fm_int smp, fm_int txTc, fm_int islPri)
{
    fm_status                       err = FM_OK;
    fm_uint32                       wmData;
    fm_switch *                     switchPtr;
    fm10000_switch *                switchPtrExt;
    fm_int                          i;
    fm_int                          rx;
    fm_int                          tx;
    fm_char                         wmStr[100];
    fm_int                          cpi;
    fm_int                          physPort;
    fm_bool                         regLockTaken = FALSE;
 
    FM_LOG_ENTRY(FM_LOG_CAT_QOS, 
                 "sw=%d rxPort=%d txPort=%d smp=%d txTc=%d islPri=%d\n", 
                 sw, rxPort, txPort, smp, txTc, islPri);
    
    switchPtr = GET_SWITCH_PTR(sw);
    switchPtrExt = GET_SWITCH_EXT(sw);


    FM_LOG_PRINT("%-55s %-14s\n",
                 "Watermark", "SegmentLimit");
    FM_LOG_PRINT("----------------------------------------------------------"
                 "--------------------\n");
    FM_LOG_PRINT("%d Byte Segment Watermarks:\n", BYTES_PER_SMP_SEG);

    /* Take register lock */
    FM_FLAG_TAKE_REG_LOCK(sw);

    /* CM_GLOBAL_WM */
    err = switchPtr->ReadUINT32(sw, FM10000_CM_GLOBAL_WM(), &wmData);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
            "Global",
            FM_GET_FIELD(wmData, FM10000_CM_GLOBAL_WM, watermark),
            FM_GET_FIELD(wmData, FM10000_CM_GLOBAL_WM, watermark));

    /* CM_SHARED_WM */
    for (i = 0 ; i < FM10000_CM_SHARED_WM_ENTRIES ; i++)
    {
        if ( ((islPri >= 0)  &&
             (islPri < FM10000_CM_SHARED_WM_ENTRIES) &&
             (islPri != i)) || (islPri == -2) )
        {
            continue;
        }

        err = switchPtr->ReadUINT32(sw, FM10000_CM_SHARED_WM(i), &wmData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        FM_SPRINTF_S(wmStr, sizeof(wmStr), "Shared [islPri=%d]", i);
        FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData, FM10000_CM_SHARED_WM, watermark),
                FM_GET_FIELD(wmData, FM10000_CM_SHARED_WM, watermark));
    }

    /* CM_PORT_RXMP_PRIVATE_WM */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &rx, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ( (rxPort >= 0) &&
             (rxPort < FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_1) &&
             (rxPort != rx)) || (rxPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_0 ; i++)
        {
            if ( ( (smp >= 0) &&
                 (smp < FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_0) &&
                 (smp != i)) || (smp == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw,
                                  FM10000_CM_RX_SMP_PRIVATE_WM(physPort, i),
                                  &wmData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr), 
                         "Port SMP Private [logPort=%d (physPort=%d), smp=%d]",
                         rx,
                         physPort,
                         i);
            FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                    wmStr,
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_PRIVATE_WM,
                                 watermark),
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_PRIVATE_WM,
                                 watermark));
        }
    }

    /* CM_RX_SMP_HOG_WM */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &rx, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ( (rxPort >= 0) &&
             (rxPort < FM10000_CM_RX_SMP_HOG_WM_ENTRIES_1) &&
             (rxPort != rx)) || (rxPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_RX_SMP_HOG_WM_ENTRIES_0 ; i++)
        {
            if ( ((smp >= 0)  &&
                 (smp < FM10000_CM_RX_SMP_HOG_WM_ENTRIES_0) &&
                 (smp != i)) || (smp == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw,
                                      FM10000_CM_RX_SMP_HOG_WM(physPort, i),
                                      &wmData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port SMP Hog [logPort=%d (physPort=%d), smp=%d]",
                         rx,
                         physPort,
                         i);
            FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                    wmStr,
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_HOG_WM,
                                 watermark),
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_HOG_WM,
                                 watermark));
        }
    }

    /* CM_RX_SMP_PAUSE_WM */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &rx, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ((rxPort >= 0) &&
             (rxPort < FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_1) &&
             (rxPort != rx)) || (rxPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_0 ; i++)
        {
            if ( ((smp >= 0) &&
                 (smp < FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_0) &&
                 (smp != i)) || (smp == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw,
                                 FM10000_CM_RX_SMP_PAUSE_WM(physPort, i),
                                 &wmData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port SMP Pause On [logPort=%d (physPort=%d), smp=%d]",
                         rx,
                         physPort,
                         i);
            FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                    wmStr,
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_PAUSE_WM,
                                 PauseOn),
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_PAUSE_WM,
                                 PauseOn));
            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port SMP Pause Off [logPort=%d (physPort=%d), smp=%d]",
                         rx,
                         physPort,
                         i);
            FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                    wmStr,
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_PAUSE_WM,
                                 PauseOff),
                    FM_GET_FIELD(wmData,
                                 FM10000_CM_RX_SMP_PAUSE_WM,
                                 PauseOff));
        }
    }


    /* CM_SHARED_SMP_PAUSE_WM */
    for (i = 0 ; i < FM10000_CM_SHARED_SMP_PAUSE_WM_ENTRIES ; i++)
    {
        if ( ( (smp >= 0) &&
             (smp < FM10000_CM_SHARED_SMP_PAUSE_WM_ENTRIES) &&
             (smp != i)) || (smp == -2) )
        {
            continue;
        }

        err = switchPtr->ReadUINT32(sw,
                                    FM10000_CM_SHARED_SMP_PAUSE_WM(i),
                                    &wmData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        FM_SPRINTF_S(wmStr, sizeof(wmStr), "Shared SMP Pause On [smp=%d]", i);
        FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData,
                             FM10000_CM_SHARED_SMP_PAUSE_WM,
                             PauseOn),
                FM_GET_FIELD(wmData,
                             FM10000_CM_SHARED_SMP_PAUSE_WM,
                             PauseOn));

        FM_SPRINTF_S(wmStr, sizeof(wmStr), "Shared SMP Pause Off [smp=%d]", i);
        FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData,
                             FM10000_CM_SHARED_SMP_PAUSE_WM,
                             PauseOff),
                FM_GET_FIELD(wmData,
                             FM10000_CM_SHARED_SMP_PAUSE_WM,
                             PauseOff));
    }

    /* CM_RXMP_SOFT_DROP_WM */
    for (i = 0 ; i < FM10000_CM_SOFTDROP_WM_ENTRIES ; i++)
    {
        if ( ((islPri >= 0) &&
             (islPri < FM10000_CM_SOFTDROP_WM_ENTRIES) &&
             (islPri != i)) || (islPri == -2) )
        {
            continue;
        }

        err = switchPtr->ReadUINT32(sw,
                                    FM10000_CM_SOFTDROP_WM(i),
                                    &wmData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        FM_SPRINTF_S(wmStr, 
                     sizeof(wmStr), 
                     "Soft Drop [islPri=%d].SoftDropSegmentLimit", 
                     i);
        FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData,
                             FM10000_CM_SOFTDROP_WM,
                             SoftDropSegmentLimit),
                FM_GET_FIELD(wmData,
                             FM10000_CM_SOFTDROP_WM,
                             SoftDropSegmentLimit));

        FM_SPRINTF_S(wmStr, 
                     sizeof(wmStr), 
                     "Soft Drop [islPri=%d].HogSegmentLimit", 
                     i);
        FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData,
                             FM10000_CM_SOFTDROP_WM,
                             HogSegmentLimit),
                FM_GET_FIELD(wmData,
                             FM10000_CM_SOFTDROP_WM,
                             HogSegmentLimit));
    }

    /* CM_PORT_TX_TC_PRIVATE_WM*/
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &tx, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ((txPort >= 0) &&
             (txPort < FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_1) &&
             (txPort != tx)) || (txPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_0 ; i++)
        {
            if ( ((txTc >= 0) &&
                 (txTc < FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_0) &&
                 (txTc != i)) || (txTc == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw,
                                       FM10000_CM_TX_TC_PRIVATE_WM(physPort, i),
                                       &wmData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port TX TC Private [logPort=%d (physPort=%d),txtc=%d]",
                         tx,
                         physPort,
                         i);
            FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData,
                             FM10000_CM_TX_TC_PRIVATE_WM,
                             watermark),
                FM_GET_FIELD(wmData,
                             FM10000_CM_TX_TC_PRIVATE_WM,
                             watermark));

        }
    }


    /* CM_TX_TC_HOG_WM */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        fmMapCardinalPortInternal(switchPtr, cpi, &tx, &physPort);

        if ( ( (txPort >= 0) &&
             (txPort < FM10000_CM_TX_TC_HOG_WM_ENTRIES_1) &&
             (txPort != tx) ) || (txPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_TX_TC_HOG_WM_ENTRIES_0 ; i++)
        {
            if ( ( (txTc >= 0) &&
                 (txTc < FM10000_CM_TX_TC_HOG_WM_ENTRIES_0) &&
                 (txTc != i)) || (txTc == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw,
                                       FM10000_CM_TX_TC_HOG_WM(physPort, i),
                                       &wmData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port TX TC Hog [logPort=%d (physPort=%d),txtc=%d]",
                         tx,
                         physPort,
                         i);
            FM_LOG_PRINT("%-55s %5d [0x%04x]\n",
                wmStr,
                FM_GET_FIELD(wmData,
                             FM10000_CM_TX_TC_HOG_WM,
                             watermark),
                FM_GET_FIELD(wmData,
                             FM10000_CM_TX_TC_HOG_WM,
                             watermark));
        }
    }
    FM_LOG_PRINT("--- Watermarks Dump Complete ---\n");

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000DbgDumpWatermarksV3 */

/*****************************************************************************/
/** fm10000DbgDumpPortIdxMap
 * \ingroup intQos
 *
 * \desc            Display the contents of a port indexed mapping table
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port to display the map for
 * 
 * \param[in]       attr is the switch attribute that correlates to the 
 *                  mapping table to display.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpPortIdxMap(fm_int sw, fm_int port, fm_int attr)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_uint32       cmTcPcMap;
    fm_uint32       cmPcRxmpMap;
    fm_int          tc;
    fm_int          pc;
    fm_int          rxmp;
    fm_int          physPort;
    fm_bool         regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d port=%d attr=%d\n", sw, port, attr);
    
    switchPtr = GET_SWITCH_PTR(sw);

    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    if (attr == FM_QOS_TC_PC_MAP)
    {
        FM_LOG_PRINT("%-12s %-12s\n", "TC", "PC");
        FM_LOG_PRINT("------------------------------------------------------"
                     "------------------------\n");

        FM_FLAG_TAKE_REG_LOCK(sw);

        /* Reading the port attribute for each index would cause multiple
         * register reads.  Instead, read once and pull the data as
         * necessary. */
        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_CM_TC_PC_MAP(physPort),
                                    &cmTcPcMap); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        FM_FLAG_DROP_REG_LOCK(sw);

        for (tc = 0 ; tc < FM_MAX_TRAFFIC_CLASSES ; tc++)
        {
            pc = FM_GET_UNNAMED_FIELD(cmTcPcMap,
                                      (tc * FM10000_CM_TC_PC_MAP_s_PauseClass),
                                      FM10000_CM_TC_PC_MAP_s_PauseClass);
            FM_LOG_PRINT("%-12d %-12d\n", tc, pc);
        }
    }
    else if (attr == FM_QOS_PC_RXMP_MAP)
    {
        FM_LOG_PRINT("%-12s %-12s\n", "PC", "RXMP");
        FM_LOG_PRINT("------------------------------------------------------"
                     "------------------------\n");
        
        FM_FLAG_TAKE_REG_LOCK(sw);

        /* Reading the port attribute for each index would cause multiple
         * register reads.  Instead, read once and pull the data as
         * necessary. */
        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_CM_PC_SMP_MAP(physPort),
                                    &cmPcRxmpMap); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        FM_FLAG_DROP_REG_LOCK(sw);

        for (pc = 0 ; pc <= FM10000_QOS_MAX_PC ; pc++)
        {
            rxmp = FM_GET_UNNAMED_FIELD(cmPcRxmpMap,
                                        (pc * FM10000_CM_PC_SMP_MAP_s_SMP),
                                        FM10000_CM_PC_SMP_MAP_s_SMP);
            if (rxmp >= FM_MAX_MEMORY_PARTITIONS)
                FM_LOG_PRINT("%-12d %-12s\n", pc, "OFF");
            else
                FM_LOG_PRINT("%-12d %-12d\n", pc, rxmp);
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }


ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000DbgDumpPortIdxMap */




/*****************************************************************************/
/** fm10000DbgDumpSwpriMap
 * \ingroup intQos
 *
 * \desc            Display the contents of a switch priority (ISLPRI) based
 *                  mapping table
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       attr is the switch attribute that correlates to the 
 *                  mapping table to display.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpSwpriMap(fm_int sw, fm_int attr)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_uint64       cmSwpriMap;
    fm_uint32       cmTcMap;
    fm_int          i;
    fm_int          x;
    fm_int          tc;
    fm_int          islTc;
    fm_int          islSmp;
    fm_bool         regLockTaken = FALSE;

    /* Mapping of smp to isl priority for the given tc
     * (or -1 if no rxmp was found for the isl that maps from the tc) */
    fm_int          smpIslMap[FM_MAX_MEMORY_PARTITIONS];

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d attr=%d\n", sw, attr);

    switchPtr = GET_SWITCH_PTR(sw);

    if (attr == FM_QOS_TC_SMP_MAP)
    {
        FM_LOG_PRINT("%-8s  %-68s\n",
                     "TC", 
                     "Shared Memory Partitions");

        FM_LOG_PRINT("------------------------------------------------------"
                     "--------------------\n");
        
        FM_FLAG_TAKE_REG_LOCK(sw);

        /* _SWITCH_PRI_TO_TC */
        err = switchPtr->ReadUINT64(sw, 
                                    FM10000_CM_APPLY_SWITCH_PRI_TO_TC(0), 
                                    &cmSwpriMap);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        /* TC_TO_SMP */
        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_CM_APPLY_TC_TO_SMP(),
                                    &cmTcMap);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        FM_FLAG_DROP_REG_LOCK(sw);

        for (tc = 0 ; tc < FM10000_MAX_TRAFFIC_CLASS ; tc++)
        {
            /* Initialize the rxmpIslMap[] array to -1 for each entry */
            memset(smpIslMap, -1, sizeof(smpIslMap));

            for (i = 0 ; i <= FM10000_MAX_SWITCH_PRIORITIES ; i++)
            {

                islTc = FM_GET_UNNAMED_FIELD64(
                                  cmSwpriMap,
                                  (i * FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc),
                                  FM10000_CM_APPLY_SWITCH_PRI_TO_TC_s_tc);

                if (islTc == tc)
                {
                    /* This isl maps to tc, save the SMP for it */
                    islSmp = FM_GET_UNNAMED_FIELD(
                                             cmTcMap, 
                                             tc, 
                                             FM10000_CM_APPLY_TC_TO_SMP_s_smp);

                    /* Ensure the SMP fits within the array index range.
                     * If it's not valid, just skip it. */
                    if ( (islSmp >= 0) && (islSmp < FM10000_MAX_SWITCH_SMPS) )
                    {
                        smpIslMap[islSmp] = i;
                    }
                }
            }

            FM_LOG_PRINT("%-8d ", tc);
            for (i = 0, x = 0 ; i < FM10000_MAX_SWITCH_SMPS ; i++, x += 3)
            {
                if (smpIslMap[i] != -1)
                {
                    FM_LOG_PRINT("%2d ", i);
                }

                /* this likely won't happen, but wrap if necessary */
                if (x >= 68)
                {
                    FM_LOG_PRINT("\n%-8s ", "");
                }
            }
            FM_LOG_PRINT("\n");
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000DbgDumpSwpriMap */




/*****************************************************************************/
/** fm1000DbgDumpMemoryUsageV3
 * \ingroup intDiagQos
 *
 * \desc            Dumps the congestion management usage levels for the 
 *                  specified port and memory partition/bsg.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       rxPort is the receive port to filter by.  If rxPort is 
 *                  outside of the valid port range, all rxPorts will be used.
 *                  This is the logical port.
 *
 * \param[in]       txPort is the transmit port to filter by.  If txPort is 
 *                  outside of the valid port range, all txPorts will be used.
 *                  This is the logical port.
 *
 * \param[in]       smp is the shared memory partition to filter by. If out of 
 *                  range, all SMPs will be selected.
 *
 * \param[in]       txTc is the traffic class to filter by. If out of 
 *                  range, all TCs will be selected.
 *
 * \param[in]       bsg is the bandwidth sharing group to filter by.  
 *                  If bsg is outside of the valid range, all bsg usages will
 *                  be used.
 *
 * \param[in]       useSegments is whether to display the memory usage in 
 *                  segments (TRUE) or in frames (FALSE).
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpMemoryUsageV3(fm_int  sw, 
                                      fm_int  rxPort, 
                                      fm_int  txPort,
                                      fm_int  smp, 
                                      fm_int  txTc, 
                                      fm_int  bsg,
                                      fm_bool useSegments)
{
    fm_status                       err = FM_OK;
    fm_uint32                       uData;
    fm_uint32                       seg;
    fm_switch *                     switchPtr;
    fm_int                          port;
    fm_int                          cpi;
    fm_int                          i;
    fm_char                         wmStr[100];
    fm_int                          physPort;
    fm_bool                         regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, 
                 "sw=%d rxPort=%d txPort=%d smp=%d txTc=%d bsg=%d "
                 "useSegments=%d\n", 
                 sw, rxPort, txPort, smp, txTc, bsg, useSegments);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_FLAG_TAKE_REG_LOCK(sw);

    if (useSegments)
    {
        FM_LOG_PRINT("%-54s %16s\n",
                     "Usage", "SegmentCount");
    }
    else
    {
        FM_LOG_PRINT("%-54s %16s\n",
                     "Usage", "KBytes");
    }
    FM_LOG_PRINT("----------------------------------------------------------"
                 "--------------------\n");

    /* CM_GLOBAL_USAGE */
    err = switchPtr->ReadUINT32(sw, FM10000_CM_GLOBAL_USAGE(), &uData);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    seg = FM_GET_FIELD64(uData, FM10000_CM_GLOBAL_USAGE, count);

    if (useSegments)
    {
        FM_LOG_PRINT("%d Byte Segment Usages:\n", BYTES_PER_SMP_SEG);
        FM_LOG_PRINT("%-54s %6d [0x%05x]\n",
                     "Global", 
                     seg, 
                     seg);
    }
    else
    {
        seg = ceil((seg * BYTES_PER_SMP_SEG) / BYTES_PER_KB);
        FM_LOG_PRINT("%-54s %16d\n", "Global", seg);
    }

    /* CM_SHARED_SMP_USAGE */
    for (i = 0 ; i < FM10000_CM_SHARED_SMP_USAGE_ENTRIES ; i++)
    {
        if ( ( (smp >= 0) && 
             (smp < FM10000_CM_SHARED_SMP_USAGE_ENTRIES) &&
             (smp != i)) || (smp == -2) )
        {
            continue;
        }

        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_CM_SHARED_SMP_USAGE(i), 
                                    &uData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        seg = FM_GET_FIELD(uData, 
                           FM10000_CM_SHARED_SMP_USAGE, 
                           count);

        FM_SPRINTF_S(wmStr, sizeof(wmStr), "Shared Usage [smp=%d]", i);
        if (useSegments)
        {
            FM_LOG_PRINT("%-54s %6d [0x%05x]\n",
                wmStr,
                seg,
                seg);
        }
        else
        {
            seg = ceil((seg * BYTES_PER_SMP_SEG) / BYTES_PER_KB);
            FM_LOG_PRINT("%-54s %16d\n",
                         wmStr,
                         seg);
        }
    }

    /* CM_RX_SMP_USAGE */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ( (rxPort >= 0) && 
             (rxPort < FM10000_CM_RX_SMP_USAGE_ENTRIES_1) &&
             (rxPort != port)) || (rxPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_RX_SMP_USAGE_ENTRIES_0 ; i++)
        {
            if ( ((smp >= 0) && 
                 (smp < FM10000_CM_RX_SMP_USAGE_ENTRIES_0) &&
                 (smp != i)) || (smp == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw, 
                                       FM10000_CM_RX_SMP_USAGE(physPort, i), 
                                       &uData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            seg = FM_GET_FIELD(uData, 
                               FM10000_CM_RX_SMP_USAGE, 
                               count);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port SMP Usage [logPort=%d (physPort=%d), smp=%d]", 
                         port, 
                         physPort, 
                         i);

            if (useSegments)
            {
                FM_LOG_PRINT("%-54s %6d [0x%05x]\n",
                             wmStr,
                             seg,
                             seg);
            }
            else
            {
                seg = ceil((seg * BYTES_PER_SMP_SEG) / BYTES_PER_KB);
                FM_LOG_PRINT("%-54s %16d\n",
                             wmStr,
                             seg);
            }
        }
    }

    /* TX_RATE_LIM_USAGE */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);

        if ( ((txPort >= 0) && 
             (txPort < FM10000_TX_RATE_LIM_USAGE_ENTRIES_1) &&
             (txPort != port)) || (txPort == -2) )
        {
            continue;
        }
        
        for (i = 0 ; i < FM10000_TX_RATE_LIM_USAGE_ENTRIES_0 ; i++)
        {
            if ( ((bsg >= 0) && 
                 (bsg < FM10000_TX_RATE_LIM_USAGE_ENTRIES_0) &&
                 (bsg != i)) || (bsg == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw, 
                                       FM10000_TX_RATE_LIM_USAGE(physPort, i), 
                                       &uData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

            seg = FM_GET_FIELD(uData, FM10000_TX_RATE_LIM_USAGE, Units);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "ERL Usage Units [logPort=%d (physPort=%d), bsg=%d]", 
                         port, 
                         physPort, 
                         i);
            FM_LOG_PRINT("%-54s %8d [0x%07x]\n", wmStr, seg, seg);
        }
    }


    /* CM_SMP_USAGE */
    for (i = 0 ; i < FM10000_CM_SMP_USAGE_ENTRIES ; i++)
    {
        if ( ( (smp >= 0) && 
             (smp < FM10000_CM_SMP_USAGE_ENTRIES) &&
             (smp != i)) || (smp == -2) )
        {
            continue;
        }

        err = switchPtr->ReadUINT32(sw, FM10000_CM_SMP_USAGE(i), &uData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        seg = FM_GET_FIELD(uData, FM10000_CM_SMP_USAGE, count);

        FM_SPRINTF_S(wmStr, sizeof(wmStr), "SMP Usage [smp=%d]", i);
        if (useSegments)
        {
            FM_LOG_PRINT("%-54s %6d [0x%05x]\n", wmStr, seg, seg);
        }
        else
        {
            seg = (seg * BYTES_PER_SMP_SEG) / BYTES_PER_KB;
            FM_LOG_PRINT("%-54s %16d\n", wmStr, seg);
        }

    }

    /* CM_TX_TC_USAGE */
    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {

        err = fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ( (txPort >= 0) && 
             (txPort < FM10000_CM_TX_TC_USAGE_ENTRIES_1) &&
             (txPort != port)) || (txPort == -2) )
        {
            continue;
        }

        for (i = 0 ; i < FM10000_CM_TX_TC_USAGE_ENTRIES_0 ; i++)
        {
            if ( ( (txTc >= 0) && 
                 (txTc < FM10000_CM_TX_TC_USAGE_ENTRIES_0) &&
                 (txTc != i)) || (txTc == -2) )
            {
                continue;
            }

            err = switchPtr->ReadUINT32(sw, 
                                        FM10000_CM_TX_TC_USAGE(physPort, 
                                                               i), 
                                        &uData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
                
            seg = FM_GET_FIELD(uData, 
                               FM10000_CM_TX_TC_USAGE, 
                               count);

            FM_SPRINTF_S(wmStr,
                         sizeof(wmStr),
                         "Port TX TC Usage [logPort=%d (physPort=%d), txTc=%d]", 
                         port, 
                         physPort, 
                         i);
            if (useSegments)
            {
                FM_LOG_PRINT("%-54s %6d [0x%05x]\n", wmStr, seg, seg);
            }
            else
            {
                seg = (seg * BYTES_PER_SMP_SEG) / BYTES_PER_KB;
                FM_LOG_PRINT("%-54s %16d\n", wmStr, seg);
            }
        }
    }
    FM_LOG_PRINT("--- Memory Usage Dump Complete ---\n");
    
ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000DbgDumpMemoryUsageV3 */




/*****************************************************************************/
/** fm10000DbgDumpQOS
 * \ingroup intDiagQos
 *
 * \desc            Dumps the QOS debug state
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port to dump the state for.
 *                  This is a logical port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpQOS(fm_int sw, fm_int port)
{
    fm_status      err = FM_OK;
    fm_uint32      uData;
    fm_uint64      pauseRcvState[FM10000_CM_PAUSE_RCV_STATE_WIDTH / 2];
    fm_switch *    switchPtr;
    fm_int         pc;
    fm_char        wmStr[100];
    fm_int         physPort;
    fm_int         logPort;
    fm_int         cpi;
    fm_bool        regLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "sw=%d\n", sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_FLAG_TAKE_REG_LOCK(sw);

    FM_LOG_PRINT("%-66s %-30s\n",
                 "State", "Value");
    FM_LOG_PRINT("----------------------------------------------------------"
                 "--------------------\n");

    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

        if ( ( (port >= 0) && 
             (physPort < FM10000_CM_PAUSE_RCV_STATE_ENTRIES) &&
             (port != logPort)) || (port == -2) )
        {
            continue;
        }

        /* CM_PAUSE_RECV_STATE 128b per port */
        err = switchPtr->ReadUINT64Mult(sw, 
                                        FM10000_CM_PAUSE_RCV_STATE(physPort, 0),
                                        (FM10000_CM_PAUSE_RCV_STATE_WIDTH / 2),
                                         pauseRcvState);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_QOS, err);

        for (pc = 0 ; pc <= FM10000_QOS_MAX_PC; pc++)
        {
            
            uData = fmMulti64BitWordBitfieldGet32(pauseRcvState, 
                                                  ((pc + 1) * 16 - 1),
                                                  pc * 16);
            FM_SPRINTF_S(wmStr,
                     sizeof(wmStr),
                     "Pause rcv timer state [logPort=%d (physPort=%d), pc=%d]", 
                     logPort, 
                     physPort,
                     pc);
            FM_LOG_PRINT("%-66s %8d [0x%06x]\n",
                          wmStr,
                          uData,
                          uData);
        }
    }

    FM_LOG_PRINT("--- QOS Dump Complete ---\n");

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_QOS, err);

}   /* end fm10000DbgDumpQOS */

