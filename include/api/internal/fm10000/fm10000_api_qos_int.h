/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_qos_int.h
 * Creation Date:  December 13, 2013
 * Description:     FM10000.QoS API functions declarations and other definitions
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#ifndef __FM_FM10000_FM_API_QOS_INT_H
#define __FM_FM10000_FM_API_QOS_INT_H


/* Structure to hold watermark configuration values */
typedef struct _fm10000_wmParam {

    /* CM_GLOBAL_WM */    
    fm_uint32   cmGlobalWm;
    /* CM_SHARED_WM */    
    fm_uint32   cmSharedWm[FM10000_CM_SHARED_WM_ENTRIES];
    /* CM_RX_SMP_PRIVATE_WM */
    fm_uint32   cmRxSmpPrivateWm[FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_1][FM10000_CM_RX_SMP_PRIVATE_WM_ENTRIES_0];
    /* CM_RX_SMP_HOG_WM */
    fm_uint32   cmRxSmpHogWm[FM10000_CM_RX_SMP_HOG_WM_ENTRIES_1][FM10000_CM_RX_SMP_HOG_WM_ENTRIES_0];
    /* CM_SOFTDROP_WM */
    fm_uint32   cmSoftDropWm[FM10000_CM_SOFTDROP_WM_ENTRIES];
    /* CM_APPLY_TX_SOFTDROP_CFG */
    fm_uint32   cmApplyTxSoftDropCfg[FM10000_CM_APPLY_TX_SOFTDROP_CFG_ENTRIES_1][FM10000_CM_APPLY_TX_SOFTDROP_CFG_ENTRIES_0];
    /* CM_APPLY_SOFTDROP_CFG */
    fm_uint32   cmApplySoftDropCfg[FM10000_CM_APPLY_SOFTDROP_CFG_ENTRIES];
    /* CM_RX_SMP_PAUSE_WM */
    fm_uint32   cmRxSmpPauseWm[FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_1][FM10000_CM_RX_SMP_PAUSE_WM_ENTRIES_0];
    /* CM_SHARED_SMP_PAUSE_WM */
    fm_uint32   cmSharedSmpPauseWm[FM10000_CM_SHARED_SMP_PAUSE_WM_ENTRIES];
    /* CM_TX_TC_HOG_WM */
    fm_uint32   cmTxTcHogWm[FM10000_CM_TX_TC_HOG_WM_ENTRIES_1][FM10000_CM_TX_TC_HOG_WM_ENTRIES_0];
    /* CM_TX_TC_PRIVATE_WM */
    fm_uint32   cmTxTcPrivateWm[FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_1][FM10000_CM_TX_TC_PRIVATE_WM_ENTRIES_0];
    /* CM_APPLY_SWITCH_PRI_TO_TC */
    fm_uint64   cmSwPriTcMap;
    /* CM_APPLY_TC_TO_SMP */
    fm_uint32   cmTcSmpMap;
    /* CM_TC_PC_MAP */
    fm_uint32   cmTcPcMap[FM10000_CM_TC_PC_MAP_ENTRIES];
    /* CM_PC_SMP_MAP */
    fm_uint32   cmPcSmpMap[FM10000_CM_PC_SMP_MAP_ENTRIES];
    /* To keep info about single smp setting*/
    fm_uint32   globalThisSmp;

} fm10000_wmParam;

/* Invalid priority mapper id */
#define FM10000_INVALID_PRIORITY_MAPPER  (-1)

/* Structure to hold priority map  parameters */
typedef struct _fm10000_priorityMap
{
    /* The trap class to be mapped.  */
    fm_int       trapClass;
    /* The switch priority to map the trap class to.  */
    fm_uint      priority;

} fm10000_priorityMap;

/* Enumeration for priority mapper list types */
typedef enum
{

    FM10000_PRI_MAP_LIST_POOL = 0,
    FM10000_PRI_MAP_LIST_MAPPER,
    FM10000_PRI_MAP_LIST_MAX

} fm10000_priorityMapListType;

/* Structure to hold internal priority map values */
typedef struct _fm10000_internalPriorityMap
{
    /* The trap class to be mapped.  */
    fm_int                      trapClass;
    /* The switch priority to map the trap class to.  */
    fm_uint                     priority;
    /* Cached switch priority before the trap class mapped to */
    fm_uint                     cache_priority;
    /* The priority mapper for this priority map.  */
    fm_int                      mapper;
    /* The internal priority map list control fields */
    FM_DLL_DEFINE_NODE(_fm10000_internalPriorityMap,
                       nextMap[FM10000_PRI_MAP_LIST_MAX],
                       previousMap[FM10000_PRI_MAP_LIST_MAX]);

} fm10000_internalPriorityMap;

/* Structure to hold control properties of internal priority map list. */
typedef struct _fm10000_internalPriorityMapList
{

    FM_DLL_DEFINE_LIST(_fm10000_internalPriorityMap, firstMap, lastMap);

} fm10000_internalPriorityMapList;

/* Structure to hold priority mapper parameters. */
typedef struct _fm10000_priorityMapper
{
    /* The priority mapper identifier.  */
    fm_int                          id;
    /* The switch priority assigned to the priority mapper.  */
    fm_uint                         priority;
    /* The number of priority maps associated with the priority mapper.  */
    fm_uint                         mapCount;
    /* The flag indicatiing if the mapper is used */
    fm_bool                         used;
    /* The flag indicatiing if the maper trigger was already created */
    fm_bool                         mapTrigCreated;
    /* The internal priority map lists associated with the mapper  */
    fm10000_internalPriorityMapList maps;
    /* The mapper list control fields */
    FM_DLL_DEFINE_NODE(_fm10000_priorityMapper, nextMapper, previousMapper);

} fm10000_priorityMapper;

/* Structure to hold control properties of priority mapper list. */
typedef struct _fm10000_priorityMapperList
{

    FM_DLL_DEFINE_LIST(_fm10000_priorityMapper, firstMapper, lastMapper);

} fm10000_priorityMapperList;

/* Structure to hold all properties of qos priority mapper. */
typedef struct _fm10000_priorityMapSet
{
    /* Table with the pointers to the mappers  */
    fm10000_priorityMapper          mappers[FM10000_MAX_SWITCH_PRIORITIES];
    /* The list of free priority mappers.  */
    fm10000_priorityMapperList      mapperPool;
    /* The list of supported priority maps.  */
    fm10000_internalPriorityMapList maps;

} fm10000_priorityMapSet;

/* Enumeration for priority mapper attributes types */
enum _fm10000_priorityMapperAttr
{
    /** Type fm_uint: The internal switch priority assigned to the priority
     *  mapper. Frames belonging to the frame classes associated with a
     *  priority mapper will be remapped to this switch priority.  */
    FM10000_PRI_MAPPER_SWPRI = 0,
    /** Type fm_uint: The number of priority maps associated with a priority
     *  mapper.
     *                                                                   \lb\lb
     *  This attribute is read-only.  */
    FM10000_PRI_MAPPER_MAP_COUNT

};

#define FM10000_QOS_MAX_NUM_SCHED_GROUPS                FM_MAX_TRAFFIC_CLASSES
#define FM10000_QOS_MIN_NUM_SCHED_GROUPS                1
#define FM10000_QOS_MAX_DRR_WEIGHT                      0xffffff

/* Scheduling group structure */
typedef struct _fm10000_eschedGroupProperty
{

    /** Priority set number */
    fm_uint32 priSetNum;

    /** DRR weight */
    fm_int drrWeight;

    /** Strict or non-strict property */
    fm_uint32 strict;

    /** One of TC boundaries of a scheduling group */
    fm_uint32 tcBoundaryA;

    /** One of TC boundaries of a scheduling group */
    fm_uint32 tcBoundaryB;

} fm10000_eschedGroupProperty;

/* Enumeration for qos queue configuration attributes types */
enum _fm10000_qosQueueConfigAttr
{
    /* Used for adding new queue */
    FM10000_QOS_ADD_QUEUE,

    /* Used for specyfing minimum bandwidth configuration for the queue */
    FM10000_QOS_SET_QUEUE_MIN_BW,

    /* Used for specyfing maximum bandwidth configuration for the queue */
    FM10000_QOS_SET_QUEUE_MAX_BW,

    /* Used for specyfing traffic class for the queue */
    FM10000_QOS_SET_QUEUE_TRAFFIC_CLASS,

    /* Used for deleting new queue */
    FM10000_QOS_DEL_QUEUE
};

/* Qos queue configuration structure */
typedef struct _fm10000_qosQueueProperty
{
    /** Number of qos queues configured */
    fm_uint16 numQoSQueues;

    /** Free bandwidth not allocated by qos queue methods */
    fm_uint16 freeQoSQueueBw;

    /** Mask indicates which traffic classes are in use */
    fm_uint16 tcMask;

    /** Structure for holding queues parameters  */
    fm_qosQueueParam qosQueuesParams[FM_MAX_TRAFFIC_CLASSES];

} fm10000_qosQueueProperty;

/* Egress scheduler per port configuration structure */
typedef struct _fm10000_eschedConfigPerPort
{
    /** Flag indicating egress scheduler configuration mode */
    fm_bool qosQueueEnabled;

    /** Number of scheduling groups in new configuration */
    fm_uint32 numGroupsNew;

    /** Number of scheduling groups in active configuration */
    fm_uint32 numGroupsActive;

    /** New scheduling group configuration */
    fm10000_eschedGroupProperty
            newSchedGroup[FM10000_QOS_MAX_NUM_SCHED_GROUPS];

    /** Active scheduling group configuration */
    fm10000_eschedGroupProperty
            activeSchedGroup[FM10000_QOS_MAX_NUM_SCHED_GROUPS];

    /** Qos queues configuration */
    fm10000_qosQueueProperty qosQueuesConfig;

} fm10000_eschedConfigPerPort;




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

fm_status fm10000SetPortQOS(fm_int sw,
                            fm_int port,
                            fm_int attr,
                            fm_int index,
                            void * value);

fm_status fm10000GetPortQOS(fm_int sw,
                            fm_int port,
                            fm_int attr,
                            fm_int index,
                            void * value);

fm_status fm10000SetSwitchQOS(fm_int sw,
                              fm_int attr,
                              fm_int index,
                              void * value);

fm_status fm10000GetSwitchQOS(fm_int sw,
                              fm_int attr,
                              fm_int index,
                              void * value);

fm_status fm10000GetPauseResendInterval(fm_int      sw,
                                        fm_int      port,
                                        fm_uint32 * timeNs);

fm_status fm10000SetPauseResendInterval(fm_int      sw,
                                        fm_int      port,
                                        fm_uint32   timeNs);

fm_status fm10000SetPauseQuantaCoefficients(fm_int sw, fm_int port);

fm_status fm10000InitializeCM(fm_int sw);

fm_status fm10000InitQOS(fm_int sw);

fm_status fm10000QOSPriorityMapperAllocateResources(fm_int sw);

fm_status fm10000QOSPriorityMapperFreeResources(fm_int sw);

fm_status fm10000GetSMPPauseState(fm_int sw, 
                                  fm_uint32 *pauseEnable);

fm_status fm10000AddQueueQOS(fm_int sw, fm_int port, fm_qosQueueParam *param);

fm_status fm10000DeleteQueueQOS(fm_int sw, fm_int port, fm_int queueId);

fm_status fm10000SetAttributeQueueQOS(fm_int sw,
                                 fm_int port,
                                 fm_int queueId, 
                                 fm_int attr, 
                                 void  *value);

fm_status fm10000GetAttributeQueueQOS(fm_int sw,
                                 fm_int port,
                                 fm_int queueId, 
                                 fm_int attr, 
                                 void  *value);

fm_status fm10000DbgDumpQueueQOS(fm_int sw);

fm_status fm10000DbgDumpWatermarks(fm_int sw);

fm_status fm10000DbgDumpWatermarksV3(fm_int sw, fm_int rxPort, fm_int txPort,
                                     fm_int rxmp, fm_int txtc, fm_int islPri);

fm_status fm10000DbgDumpMemoryUsage(fm_int sw);

fm_status fm10000DbgDumpPortIdxMap(fm_int sw, fm_int port, fm_int attr);

fm_status fm10000DbgDumpSwpriMap(fm_int sw, fm_int attr);

fm_status fm10000DbgDumpMemoryUsageV3(fm_int  sw, 
                                      fm_int  rxPort, 
                                      fm_int  txPort,
                                      fm_int  smp, 
                                      fm_int  txTc, 
                                      fm_int  bsg,
                                      fm_bool useSegments);

fm_status fm10000DbgDumpQOS(fm_int sw, fm_int port);

#endif /* __FM_FM10000_FM_API_QOS_INT_H */
