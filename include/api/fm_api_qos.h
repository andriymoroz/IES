/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_qos.h
 * Creation Date:   May 26, 2005
 * Description:     Contains functions dealing with the QOS settings,
 *                  i.e. watermarks, priority maps, etc.
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

#ifndef __FM_FM_API_QOS_H
#define __FM_FM_API_QOS_H


/* The default value for QoS queue id. */
#define FM_QOS_QUEUE_ID_DEFAULT                     -1

/* The default value for QoS queue Traffic Class parameter. */
#define FM_QOS_QUEUE_TC_DEFAULT                     -1

/* The default value for QoS queue minimum bandwidth parameter. */
#define FM_QOS_QUEUE_MIN_BW_DEFAULT                 0x0

/* The default value for QoS queue maximum bandwidth parameter. */
#define FM_QOS_QUEUE_MAX_BW_DEFAULT                 0xffffffff


/**************************************************/
/** \ingroup typeEnum
 * Used as an argument to ''fmSetAttributeQueueQOS''.
 * and ''fmGetAttributeQueueQOS'' These enumerated 
 * values are used to configure an QoS queue.
 **************************************************/
typedef enum
{
    /** Type fm_uint32: Specifies (in Mb/s) the bandwidth that is guaranteed
      * by the DRR algorithm for the QoS queue's eligible traffic. The value
      * should not exceed the free egress bandwidth, which can be found by
      * using the ''fmGetPortQOS'' function to retrieve the
      * ''FM_QOS_QUEUE_FREE_BW'' attribute. The initial value of the queue's
      * attribute value is set when the queue is created by
      * ''fmAddQueueQOS'' function. */
    FM_QOS_QUEUE_MIN_BW,

    /** Type fm_uint32: Specifies (in Mb/s) bandwidth which is limited by the
      * shaper for the QoS queue.
      * The value may range from 1 to a value equal to the port's speed,
      * or can be set to FM_QOS_QUEUE_MAX_BW_DEFAULT, which disables shaping
      * on the queue.
      * The initial value is set when the queue is created by the
      * ''fmAddQueueQOS'' function. */
    FM_QOS_QUEUE_MAX_BW,

    /** Type fm_int: Traffic Class associated with the QoS queue. May range
      * from 0 to 7.
      * The initial value of the queue's attribute value is set when the queue
      * is created by the ''fmAddQueueQOS'' function. */
    FM_QOS_QUEUE_TRAFFIC_CLASS,

    /* ---- Add new qos queue attributes above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_QOS_QUEUE_MAX

} fm_qosQueueAttr;



/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmAddQueueQOS''.
 *  Contains set of parameters required for QoS 
 *  queue configuration.
 **************************************************/
typedef struct _fm_qosQueueParam
{
    /** QoS queue Id specifies a unique per-port identifier for the created
      * queue, and is set by the ''fmAddQueueQOS'' function. The value may
      * range from 0 to 7. The Id is used as an argument by several
      * functions for further QoS queue configuration:
      * ''fmSetAttributeQueueQOS'', ''fmGetAttributeQueueQOS'' or when
      * removing the specified queue: ''fmDeleteQueueQOS''. */
    fm_int      queueId;

    /** Traffic Class associated with the queue and should be set from the
      * range 0 to 7. */
    fm_int      tc;

    /** Minimum bandwidth specifies (in Mb/s) bandwidth that is guaranteed
      * by the DRR algorithm for the QoS queue's eligible traffic. The value
      * of the parameter should not exceed the free egress bandwidth (not
      * allocated by other QoS queues) of the specified port. Initially (no
      * queue exists), maximum value may be equal to the port speed. The
      * current free egress bandwidth may be determined by using the
      * ''fmGetPortQOS'' function to retrieve the ''FM_QOS_QUEUE_FREE_BW''
      * attribute. The value should not be greater than maximum bandwidth
      * (maxBw) specified for the queue. */
    fm_uint32   minBw;

    /** Maximum bandwidth specifies (in Mb/s) the bandwidth which is limited
      * by the shaper for the QoS queue. The value of the parameter should
      * not be lower than the than minimum bandwidth (minBw) specified for
      * the queue, and may range from 1 to a value equalling the port's
      * speed, or can be set as FM_QOS_QUEUE_MAX_BW_DEFAULT which disables
      * shaping on the queue. */
    fm_uint32   maxBw;

} fm_qosQueueParam;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

/* functions to set and get port specific QOS parameters */
fm_status fmSetPortQOS(fm_int sw,
                       fm_int port,
                       fm_int attr,
                       fm_int index,
                       void * value);
fm_status fmGetPortQOS(fm_int sw,
                       fm_int port,
                       fm_int attr,
                       fm_int index,
                       void * value);


/* functions to set and get global switch level QOS parameters */
fm_status fmSetSwitchQOS(fm_int sw, fm_int attr, fm_int index, void *value);
fm_status fmGetSwitchQOS(fm_int sw, fm_int attr, fm_int index, void *value);
fm_status fmGetMemoryUsage(fm_int  sw,
                           fm_int  port,
                           fm_int  partition,
                           fm_int *globalUsage,
                           fm_int *partUsage,
                           fm_int *rxUsage,
                           fm_int *rxPartUsage,
                           fm_int *txUsage,
                           fm_int *txPartUsage,
                           fm_int *txPerClassUsage);
fm_status fmAddQDM(fm_int sw, fm_int port, fm_int tc, fm_int weight, fm_int cnt);
fm_status fmDelQDM(fm_int sw, fm_int port, fm_int tc);
fm_status fmResetQDM(fm_int sw, fm_int port, fm_int tc);
fm_status fmGetQDM(fm_int sw, fm_int port, fm_int tc, fm_int *delay);

/* functions to add and delete QOS queues */
fm_status fmAddQueueQOS(fm_int sw, fm_int port, fm_qosQueueParam *param);
fm_status fmDeleteQueueQOS(fm_int sw, fm_int port, fm_int queueId);

/* functions to set and get QOS queue specific parameters */
fm_status fmSetAttributeQueueQOS(fm_int sw,
                                 fm_int port,
                                 fm_int queueId, 
                                 fm_int attr, 
                                 void  *value);
fm_status fmGetAttributeQueueQOS(fm_int sw,
                                 fm_int port,
                                 fm_int queueId, 
                                 fm_int attr, 
                                 void  *value);

#endif /* __FM_FM_API_QOS_H */
