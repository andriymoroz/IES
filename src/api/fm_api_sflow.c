/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_sflow.c
 * Creation Date:   May 29, 2008 
 * Description:     Generic sflow interface.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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
/** fmCreateSFlow
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create an sFlow instance.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the ID of the sFlow instance to create.
 *
 * \param[in]       sFlowType is the type of the sFlow instance to create.
 *                  See ''fm_sFlowType'' for possible values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is out of range
 *                  or the instance already exists.
 * \return          FM_ERR_TRIGGER_UNAVAILABLE if no hardware resources are
 *                  available to implement the sFlow.
 *
 *****************************************************************************/
fm_status fmCreateSFlow(fm_int sw, fm_int sFlowId, fm_sFlowType sFlowType)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d sFlowType=%d\n", 
                     sw, 
                     sFlowId, 
                     sFlowType);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->CreateSFlow, sw, sFlowId, sFlowType);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmCreateSFlow */




/*****************************************************************************/
/** fmDeleteSFlow
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete an sFlow instance.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the ID of the sFlow instance to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteSFlow(fm_int sw, fm_int sFlowId)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW, "sw=%d, sFlowId=%d\n", sw, sFlowId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DeleteSFlow, sw, sFlowId);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmDeleteSFlow */




/*****************************************************************************/
/** fmAddSFlowPort
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a logical port to an sFlow instance.
 *
 * \note            To support sFlow on a LAG, you must call this function
 *                  multiple times, adding each member physical port of the
 *                  LAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[in]       port is the logical port to add to the sFlow. Note that
 *                  port must represent a single physical port and may not be
 *                  the logical port of a LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if port is out of range.
 *
 *****************************************************************************/
fm_status fmAddSFlowPort(fm_int sw, fm_int sFlowId, fm_int port)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW, 
                     "sw=%d, sFlowId=%d, port=%d\n", 
                     sw, 
                     sFlowId, 
                     port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->AddSFlowPort, sw, sFlowId, port);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmAddSFlowPort */




/*****************************************************************************/
/** fmDeleteSFlowPort
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a logical port from an sFlow instance.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[in]       port is the logical port to delete from the sFlow
 *                  instance.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteSFlowPort(fm_int sw, fm_int sFlowId, fm_int port)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW, 
                     "sw=%d, sFlowId=%d port=%d\n", 
                     sw, 
                     sFlowId,
                     port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DeleteSFlowPort, sw, sFlowId, port);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmDeleteSFlowPort */




/*****************************************************************************/
/** fmGetSFlowPortFirst
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first port in an sFlow.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[out]      firstPort points to caller-allocated storage where
 *                  this function should place the first logical port in the 
 *                  sFlow. It will be set to -1 if no port is found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_NO_SFLOW_PORT if there are no ports in the sFlow.
 *
 *****************************************************************************/
fm_status fmGetSFlowPortFirst(fm_int   sw, 
                              fm_int   sFlowId, 
                              fm_int * firstPort)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d, firstPort=%p\n",
                     sw, 
                     sFlowId, 
                     (void *) firstPort);

    if (firstPort == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY(err, switchPtr->GetSFlowPortFirst, sw, sFlowId, firstPort);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmGetSFlowPortFirst */




/*****************************************************************************/
/** fmGetSFlowPortNext
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next port in an sFlow following a prior call 
 *                  to this function or to ''fmGetSFlowPortFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[in]       startPort is the last port found in the sFlow by a 
 *                  previous call to this function or to ''fmGetSFlowPortFirst''. 
 *
 * \param[out]      nextPort points to caller-allocated storage where this 
 *                  function should place the next port in the sFlow. Will be 
 *                  set to -1 if no more ports are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_NO_SFLOW_PORT if there are no more ports in the 
 *                  sFlow.
 *
 *****************************************************************************/
fm_status fmGetSFlowPortNext(fm_int   sw, 
                             fm_int   sFlowId, 
                             fm_int   startPort, 
                             fm_int * nextPort)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d, startPort = %d, nextPort=%p\n",
                     sw, 
                     sFlowId, 
                     startPort, 
                     (void *) nextPort);

    if (nextPort == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetSFlowPortNext, 
                       sw, 
                       sFlowId, 
                       startPort, 
                       nextPort);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmGetSFlowPortNext */




/*****************************************************************************/
/** fmGetSFlowPortList
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve a list of all ports in an sFlow.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[out]      numPorts points to caller-allocated storage where this
 *                  function should place the number of sFlow ports listed.
 *
 * \param[out]      portList points to a caller-allcoated array of size max 
 *                  where this function should place the list of sFlow
 *                  ports.
 *
 * \param[in]       max is the size of portList, being the maximum number of
 *                  sFlow ports that may be accommodated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if numPorts or portList is NULL.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the list of sFlow ports.
 *
 *****************************************************************************/
fm_status fmGetSFlowPortList(fm_int   sw, 
                             fm_int   sFlowId, 
                             fm_int * numPorts, 
                             fm_int * portList, 
                             fm_int   max)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d, numPorts=%p, portList=%p, max=%d\n",
                     sw, 
                     sFlowId, 
                     (void *) numPorts, 
                     (void *) portList, 
                     max);

    if (!numPorts || !portList)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetSFlowPortList, 
                       sw, 
                       sFlowId, 
                       numPorts,
                       portList,
                       max);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmGetSFlowPortList */




/*****************************************************************************/
/** fmSetSFlowAttribute
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set an sFlow attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[in]       attr is the sFlow attribute (see ''sFlow Attributes'')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 * \return          FM_ERR_INVALID_SFLOW_ATTR if attr is not recognized.
 * \return          FM_ERR_UNSUPPORTED if the request is not supported by
 *                  the underlying switch.
 *
 *****************************************************************************/
fm_status fmSetSFlowAttribute(fm_int sw, 
                              fm_int sFlowId, 
                              fm_int attr, 
                              void * value)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d, attr=%d, value=%p\n",
                     sw, 
                     sFlowId, 
                     attr, 
                     (void *) value);

    if (value == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetSFlowAttribute, 
                       sw, 
                       sFlowId, 
                       attr, 
                       value);
    
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmSetSFlowAttribute */




/*****************************************************************************/
/** fmGetSFlowAttribute
 * \ingroup sflow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get an sFlow attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[in]       attr is the sFlow attribute (see ''sFlow Attributes'')
 *                  to get.
 *
 * \param[out]      value points to caller-allocated storage where this 
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SFLOW_INSTANCE if sFlowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 * \return          FM_ERR_INVALID_SFLOW_ATTR if attr is not recognized.
 *
 *****************************************************************************/
fm_status fmGetSFlowAttribute(fm_int sw, 
                              fm_int sFlowId, 
                              fm_int attr, 
                              void * value)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d, attr=%d, value=%p\n",
                     sw, 
                     sFlowId, 
                     attr, 
                     (void *) value);

    if (value == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetSFlowAttribute, 
                       sw, 
                       sFlowId, 
                       attr, 
                       value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, err);

}   /* end fmGetSFlowAttribute */




/*****************************************************************************/
/** fmGetSFlowType
 * \ingroup intsflow
 *
 * \desc            Retrieve the type of the sFlow instance.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sFlowId is the sFlow instance on which to operate.
 *
 * \param[out]      sFlowType is the type of the sFlow instance.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetSFlowType(fm_int sw, fm_int sFlowId, fm_sFlowType *sFlowType)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, sFlowId=%d, sFlowType=%p\n",
                     sw,
                     sFlowId,
                     (void *) sFlowType);

    if (sFlowType == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SFLOW, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetSFlowType, sw, sFlowId, sFlowType);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fmGetSFlowType */




/*****************************************************************************/
/** fmCheckSFlowLogging
 * \ingroup intsflow
 *
 * \desc            Check if the packet is received due to a sFlow instance.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pktEvent points to the FM_EVENT_PACKET_RECV event to check.
 *
 * \param[out]      isPktSFlowLogged points to the user-allocated storage on
 *                  whether the packet is logged due to a sFlow instance.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
fm_status fmCheckSFlowLogging(fm_int            sw, 
                              fm_eventPktRecv * pktEvent, 
                              fm_bool         * isPktSFlowLogged)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SFLOW,
                     "sw=%d, pktEvent=%p\n",
                     sw,
                     (void *) pktEvent);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CheckSFlowLogging,
                       sw,
                       pktEvent,
                       isPktSFlowLogged);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SFLOW, err);

}   /* end fmCheckSFlowLogging */
