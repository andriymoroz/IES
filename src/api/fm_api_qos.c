/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_qos.c
 * Creation Date:   2005
 * Description:     Contains functions dealing with the QOS settings,
 *                  i.e. watermarks, priority maps, etc.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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
/** fmInitQOS
 * \ingroup intQos
 *
 * \desc            Initializes QOS data structures.
 *
 * \param[in]       swstate is the switch state table on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitQOS(fm_switch *swstate)
{
    FM_LOG_ENTRY(FM_LOG_CAT_QOS, "swstate=%p\n", (void *) swstate);

    FM_NOT_USED(swstate);

    FM_LOG_EXIT(FM_LOG_CAT_QOS, FM_OK);

}   /* end fmInitQOS */




/*****************************************************************************/
/** fmSetPortQOS
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a port's QOS attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate. May be the CPU
 *                  interface for some QoS attributes. See ''Port QoS Attributes''
 *                  for details.
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
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmSetPortQOS(fm_int sw,
                       fm_int port,
                       fm_int attr,
                       fm_int index,
                       void * value)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_status  err = FM_OK;
    fm_int     members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int     numMembers;
    fm_int     cnt;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d attr=%d index=%d value=%p\n",
                     sw,
                     port,
                     attr,
                     index,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU|ALLOW_LAG);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmGetLAGCardinalPortList(sw,
                                   port,
                                   &numMembers,
                                   members,
                                   FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);

    for (cnt = 0 ; cnt < numMembers ; cnt++)
    {
        port = members[cnt];

        portPtr = switchPtr->portTable[port];

        FM_API_CALL_FAMILY(err, portPtr->SetPortQOS, sw, port, attr, index, value);
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmSetPortQOS */




/*****************************************************************************/
/** fmGetPortQOS
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a port's QOS attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate. May be the CPU
 *                  interface for some QoS attributes. See ''Port QoS Attributes''
 *                  for details.
 *
 * \param[in]       attr is the port QoS attribute (see 'Port QoS Attributes')
 *                  to get.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Port QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmGetPortQOS(fm_int sw,
                       fm_int port,
                       fm_int attr,
                       fm_int index,
                       void * value)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d attr=%d index=%d value=%p\n",
                     sw,
                     port,
                     attr,
                     index,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU|ALLOW_LAG);

    switchPtr = GET_SWITCH_PTR(sw);

    if (!fmIsCardinalPort(sw, port))
    {
        /* Assume all members have the same property
         * so just get from one member
         */
        err = fmGetFirstPhysicalMemberPort(sw, port, &port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_QOS, err);
    }

    portPtr = switchPtr->portTable[port];

    FM_API_CALL_FAMILY(err, portPtr->GetPortQOS, sw, port, attr, index, value);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmGetPortQOS */




/*****************************************************************************/
/** fmSetSwitchQOS
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a switch global QOS attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch QoS attribute (see 'Switch QoS Attributes')
 *                  to set.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Switch QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmSetSwitchQOS(fm_int sw, fm_int attr, fm_int index, void *value)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d attr=%d index=%d value=%p\n",
                     sw,
                     attr,
                     index,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->SetSwitchQOS, sw, attr, index, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmSetSwitchQOS */




/*****************************************************************************/
/** fmGetSwitchQOS
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a switch global QOS attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch QoS attribute (see 'Switch QoS Attributes')
 *                  to get.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Switch QoS Attributes'' to determine the use of this
 *                  argument. Unless otherwise specified, this parameter
 *                  is ignored.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmGetSwitchQOS(fm_int sw, fm_int attr, fm_int index, void *value)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d attr=%d index=%d value=%p\n",
                     sw,
                     attr,
                     index,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetSwitchQOS, sw, attr, index, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmGetSwitchQOS */




/*****************************************************************************/
/* fmGetMemoryUsage (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Get memory usage. The memory usage is the number
 *                  of memory segment used multiplied by the
 *                  size of each segments in bytes. Note that for
 *                  switch aggregates (SWAGs), the global usage and partition
 *                  usage is from the switch used by the port. The service
 *                  skips returning usage for null pointers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       partition is the shared memory partition to read from.
 *
 * \param[in]       port is the logical port
 *
 * \param[out]      globalUsage points to caller-allocated storage where this
 *                  function should place the global memory usage.
 *
 * \param[out]      partUsage points to caller-allocated storage where this
 *                  function should place the shared partition usage.
 *
 * \param[out]      rxUsage points to caller-allocated storage where this
 *                  function should place the rx queue size.
 *
 * \param[out]      rxPartUsage points to caller-allocated storage where this
 *                  function should place the rx queue size per partition.
 *
 * \param[out]      txUsage points to caller-allocated storage where this
 *                  function should place the tx queue size.
 *
 * \param[out]      txPartUsage points to caller-allocated storage where this
 *                  function should place the tx queue size per partition.
 *
 * \param[out]      txPerClassUsage points to caller-allocated storage where this
 *                  function should place the tx queue size per traffic class. The
 *                  array must be 8 long.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmGetMemoryUsage(fm_int  sw,
                           fm_int  port,
                           fm_int  partition,
                           fm_int *globalUsage,
                           fm_int *partUsage,
                           fm_int *rxUsage,
                           fm_int *rxPartUsage,
                           fm_int *txUsage,
                           fm_int *txPartUsage,
                           fm_int *txPerClassUsage)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d partition=%d\n",
                     sw,
                     port,
                     partition);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(partition);
    FM_NOT_USED(globalUsage);
    FM_NOT_USED(partUsage);
    FM_NOT_USED(rxUsage);
    FM_NOT_USED(rxPartUsage);
    FM_NOT_USED(txUsage);
    FM_NOT_USED(txPartUsage);
    FM_NOT_USED(txPerClassUsage);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmGetMemoryUsage */




/*****************************************************************************/
/* fmGetRxRate (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Get ingress data rate measured in bits per second.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port
 *
 * \param[in]       partition is the memory partition
 *
 * \param[out]      rxRate points to caller-allocated storage where this
 *                  function should place the measured receive rate
 *                  for this partition.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PARTITION if partition is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmGetRxRate(fm_int    sw,
                      fm_int    port,
                      fm_int    partition,
                      fm_int64 *rxRate)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d partition=%d\n",
                     sw,
                     port,
                     partition);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(partition);
    FM_NOT_USED(rxRate);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmGetRxRate */




/*****************************************************************************/
/* fmGetTxRate (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Get data rate measured in bits per second. The
 *                  data rate is measured per partition. There are no
 *                  rate measurement for the unmanaged partition.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port
 *
 * \param[in]       partition is the memory partition
 *
 * \param[out]      rxRate points to caller-allocated storage where this
 *                  function should place the measured receive rate
 *                  for this partition.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PARTITION if partition is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmGetTxRate(fm_int    sw,
                      fm_int    port,
                      fm_int    partition,
                      fm_int64 *rxRate)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d partition=%d\n",
                     sw,
                     port,
                     partition);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(partition);
    FM_NOT_USED(rxRate);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmGetTxRate */




/*****************************************************************************/
/* fmAddQDM (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Add a Queue Delay Measurement.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port
 *
 * \param[in]       tc is the traffic class to measure
 *
 * \param[in]       weight is to perform an exponential-weighted average, the
 *                  will be rounded up to the next power of 2. Setting a value
 *                  of -1 causes the measurement to not do any average but
 *                  keep the maximum value instead.
 *
 * \param[in]       cnt is the number of frame to sample before stopping.
 *                  Setting the count to -1 is equivalent to infinite.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_FREE_QDM if there are no QDM free.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmAddQDM(fm_int sw, fm_int port, fm_int tc, fm_int weight, fm_int cnt)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d tc=%d weight=%d cnt=%d\n",
                     sw,
                     port,
                     tc,
                     weight,
                     cnt);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(tc);
    FM_NOT_USED(weight);
    FM_NOT_USED(cnt);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmAddQDM */




/*****************************************************************************/
/* fmDelQDM (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Delete a Queue Delay Measurement.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port
 *
 * \param[in]       tc is the traffic class to measure
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_QDM if there is no QDM matching.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmDelQDM(fm_int sw, fm_int port, fm_int tc)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS, "sw=%d port=%d tc=%d\n", sw, port, tc);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(tc);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmDelQDM */




/*****************************************************************************/
/* fmResetQDM (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Reset a Queue Delay Measurement.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port
 *
 * \param[in]       tc is the traffic class to measure
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_QDM if there is no QDM matching.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmResetQDM(fm_int sw, fm_int port, fm_int tc)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS, "sw=%d port=%d tc=%d\n", sw, port, tc);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(tc);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmResetQDM */




/*****************************************************************************/
/* fmGetQDM (Do not document until implemented)
 * \ingroup qos
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Get a Queue Delay Measurement. The delay is reported
 *                  in nanoseconds.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port
 *
 * \param[in]       tc is the traffic class to measure
 *
 * \param[out]      delay points to caller-allocated storage where this
 *                  function should place the measured average delay or
 *                  max delay.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_QDM if there is no QDM matching.
 * \return          FM_ERR_INVALID_ARGUMENT if index or value is invalid.
 *
 *****************************************************************************/
fm_status fmGetQDM(fm_int sw, fm_int port, fm_int tc, fm_int *delay)
{
    fm_status err = FM_ERR_UNSUPPORTED;

    FM_LOG_ENTRY_API(FM_LOG_CAT_QOS,
                     "sw=%d port=%d tc=%d delay=%p\n",
                     sw,
                     port,
                     tc,
                     (void *) delay);

    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(tc);
    FM_NOT_USED(delay);

    FM_LOG_EXIT_API(FM_LOG_CAT_QOS, err);

}   /* end fmGetQDM */
