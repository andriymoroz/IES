/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_storm.c
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with storm control.
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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
/** fmCreateStormCtrl
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      stormController points to caller-allocated storage where
 *                  this function should place the storm controller number
 *                  (handle) of the newly created storm controller.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_FREE_STORM_CTRL if the maximum number of storm
 *                  controllers (''FM_MAX_NUM_STORM_CTRL'') have already been
 *                  created.
 * \return          FM_ERR_INVALID_ARGUMENT if stormController is NULL.
 *
 *****************************************************************************/
fm_status fmCreateStormCtrl(fm_int sw, fm_int *stormController)
{
    fm_status   err;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %p\n",
                     sw,
                     (void *) stormController);

    if (stormController == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateStormCtrl,
                       sw,
                       stormController);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmCreateStormCtrl */




/*****************************************************************************/
/** fmDeleteStormCtrl
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not the
 *                  handle of an existing storm controller.
 *
 *****************************************************************************/
fm_status fmDeleteStormCtrl(fm_int sw, fm_int stormController)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d\n",
                     sw,
                     stormController);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->DeleteStormCtrl, 
                       sw,
                       stormController);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmDeleteStormCtrl */




/*****************************************************************************/
/** fmSetStormCtrlAttribute
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a storm controller attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') whose attribute is to be set.
 *
 * \param[in]       attr is the storm controller attribute to set.
 *                  See ''Storm Controller Attributes'' for details.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not the
 *                  handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ATTRIB if attr is unrecognized.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 *
 *****************************************************************************/
fm_status fmSetStormCtrlAttribute(fm_int sw,
                                  fm_int stormController,
                                  fm_int attr,
                                  void * value)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;


    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, attr=%d, value=%p \n",
                     sw,
                     stormController,
                     attr,
                     value);

    if (value == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetStormCtrlAttribute,
                       sw,
                       stormController,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmSetStormCtrlAttribute */




/*****************************************************************************/
/** fmGetStormCtrlAttribute
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a storm controller attribute
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') whose attribute is to be
 *                  retrieved.
 *
 * \param[in]       attr is the storm controller attribute to get.
 *                  See ''Storm Controller Attributes'' for details.
 *
 * \param[out]      value points to caller-allocated storage where
 *                  this function will place the value of the attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not the
 *                  handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ATTRIB if attr is unrecognized.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlAttribute(fm_int sw,
                                  fm_int stormController,
                                  fm_int attr,
                                  void * value)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, attr=%d, value=%p \n",
                     sw,
                     stormController,
                     attr,
                     value);

    if (value == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlAttribute,
                       sw,
                       stormController,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlAttribute */




/*****************************************************************************/
/** fmGetStormCtrlList
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return a list of valid storm controller numbers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numStormControllers points to caller allocated storage where
 *                  this function should place the number of valid storm
 *                  controllers returned in stormControllers.
 *
 * \param[out]      stormControllers is an array that this function will fill
 *                  with the list of valid storm controller numbers.
 *
 * \param[in]       max is the size of stormControllers, being the maximum
 *                  number of storm controller numbers that stormControllers
 *                  can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of valid storm controller numbers.
 * \return          FM_ERR_INVALID_ARGUMENT if numStormControllers or
 *                  stormControllers is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlList(fm_int  sw,
                             fm_int *numStormControllers,
                             fm_int *stormControllers,
                             fm_int  max)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, numStormControllers = %p, stormControllers = %p, "
                     "max = %d\n",
                     sw,
                     (void *) numStormControllers,
                     (void *) stormControllers,
                     max);

    if ( (numStormControllers == NULL) || (stormControllers == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlList,
                       sw,
                       numStormControllers,
                       stormControllers,
                       max);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlList */




/*****************************************************************************/
/** fmGetStormCtrlFirst
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first storm controller number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstStormController points to caller-allocated storage
 *                  where this function should place the number of the first
 *                  storm controller.  Will be set to -1 if no storm
 *                  controllers are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_STORM_CONTROLLERS if no storm controller found.
 * \return          FM_ERR_INVALID_ARGUMENT if firstStormController is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlFirst(fm_int sw, fm_int *firstStormController)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, firstStormController = %p\n",
                     sw,
                     (void *) firstStormController);

    if (firstStormController == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlFirst,
                       sw,
                       firstStormController);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlFirst */




/*****************************************************************************/
/** fmGetStormCtrlNext
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next storm controller number, following
 *                  a prior call to this function or to
 *                  ''fmGetStormCtrlFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentStormController is the last storm controller number
 *                  found by a previous call to this function or to
 *                  ''fmGetStormCtrlFirst''.
 *
 * \param[out]      nextStormController points to caller-allocated storage
 *                  where this function should place the number of the next
 *                  storm controller. Will be set to -1 if no more storm
 *                  controllers found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if currentStormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_CONTROLLERS if no more storm controller
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if nextStormController is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlNext(fm_int  sw,
                             fm_int  currentStormController,
                             fm_int *nextStormController)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, currentStormController = %d, "
                     "nextStormController = %p\n",
                     sw,
                     currentStormController,
                     (void *) nextStormController);

    if (nextStormController == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlNext,
                       sw,
                       currentStormController,
                       nextStormController);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlNext */




/*****************************************************************************/
/** fmAddStormCtrlCondition
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a condition to a storm controller. This function may
 *                  be called multiple times to add multiple conditions to a
 *                  single storm controller. All conditions are ORed together.
 *                  As an example, setting both ''FM_STORM_COND_IGMP'' and
 *                  ''FM_STORM_COND_BROADCAST'' causes either packet type to 
 *                  trigger the storm controller. This function will 
 *                  automatically check existing conditions to prevent adding 
 *                  invalid conditions.
 * 
 * \note            The FM6000 supports a single traffic type and a single
 *                  ingress port per storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') to which the condition should
 *                  be added.
 *
 * \param[in]       condition points to an ''fm_stormCondition'' structure
 *                  describing the condition to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_STORM_COND_EXCEEDED if adding the condition exceed the
 *                  stormController's maximum condition numbers.
 * \return          FM_ERR_INVALID_STORM_COND if condition is not compatible with
 *                  the current condition configuration of stormCtrlId.
 * \return          FM_ERR_INVALID_ARGUMENT if condition is NULL.
 *
 *****************************************************************************/
fm_status fmAddStormCtrlCondition(fm_int             sw,
                                  fm_int             stormController,
                                  fm_stormCondition *condition)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, condition = %p\n",
                     sw,
                     stormController,
                     (void *) condition);

    if (condition == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddStormCtrlCondition,
                       sw,
                       stormController,
                       condition);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmAddStormCtrlCondition */




/*****************************************************************************/
/** fmDeleteStormCtrlCondition
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a condition from a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the condition
 *                  should be deleted.
 *
 * \param[in]       condition points to an ''fm_stormCondition'' structure
 *                  describing the condition to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_STORM_COND if condition does not exist
 *                  in the storm controller.
 * \return          FM_ERR_INVALID_ARGUMENT if condition is NULL.
 *
 *****************************************************************************/
fm_status fmDeleteStormCtrlCondition(fm_int             sw,
                                     fm_int             stormController,
                                     fm_stormCondition *condition)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, condition = %p\n",
                     sw,
                     stormController,
                     (void *) condition);

    if (condition == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteStormCtrlCondition,
                       sw,
                       stormController,
                       condition);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmDeleteStormCtrlCondition */




/*****************************************************************************/
/** fmGetStormCtrlConditionList
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Function to return the list of conditions that will
 *                  trigger a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the condition
 *                  list should be retrieved.
 *
 * \param[out]      numConditions points to caller-allocated storage where
 *                  this function should place the number of conditions
 *                  returned in conditionList.
 *
 * \param[out]      conditionList is an array that this function will fill with
 *                  the list of conditions associated with the storm controller.
 *
 * \param[in]       max is the size of conditionList, being the maximum number
 *                  of conditions that conditionList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of conditions.
 * \return          FM_ERR_INVALID_ARGUMENT if numConditions or conditionList
 *                  is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlConditionList(fm_int             sw,
                                      fm_int             stormController,
                                      fm_int *           numConditions,
                                      fm_stormCondition *conditionList,
                                      fm_int             max)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, numConditions = %p,"
                     "conditionList = %p, max = %d\n",
                     sw,
                     stormController,
                     (void *) numConditions,
                     (void *) conditionList,
                     max);

    if ( (numConditions == NULL) || (conditionList == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlConditionList,
                       sw,
                       stormController,
                       numConditions,
                       conditionList,
                       max);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlConditionList */




/*****************************************************************************/
/** fmGetStormCtrlConditionFirst
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first condition associated with a storm
 *                  controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which to retrieve the
 *                  first condition.
 *
 * \param[out]      firstCondition points to caller-allocated storage where
 *                  this function should place the first condition for this
 *                  storm controller.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_CONDITION if no storm controller conditions
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if firstCondition is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlConditionFirst(fm_int             sw,
                                       fm_int             stormController,
                                       fm_stormCondition *firstCondition)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, firstCondition = %p\n",
                     sw,
                     stormController,
                     (void *) firstCondition);

    if (firstCondition == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlConditionFirst,
                       sw,
                       stormController,
                       firstCondition);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlConditionFirst */




/*****************************************************************************/
/** fmGetStormCtrlConditionNext
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next condition associated with a storm
 *                  controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which to retrieve the
 *                  next condition.
 *
 * \param[in]       currentCondition points to the last condition found by a
 *                  previous call to this function or to
 *                  ''fmGetStormCtrlConditionFirst''.
 *
 * \param[out]      nextCondition points to caller-allocated storage where this
 *                  function should place the next condition for this
 *                  storm controller.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_CONDITION if no more storm controller
 *                  conditions found.
 * \return          FM_ERR_INVALID_ARGUMENT if currentCondition or nextCondition
 *                  is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlConditionNext(fm_int             sw,
                                      fm_int             stormController,
                                      fm_stormCondition *currentCondition,
                                      fm_stormCondition *nextCondition)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, currentCondition = %p, "
                     "nextCondition = %p\n",
                     sw,
                     stormController,
                     (void *) currentCondition,
                     (void *) nextCondition);

    if ( (currentCondition == NULL) || (nextCondition == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlConditionNext,
                       sw,
                       stormController,
                       currentCondition,
                       nextCondition);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlConditionNext */




/*****************************************************************************/
/** fmAddStormCtrlAction
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Add an action to a storm controller. Actions dictate what
 *                  happens when a storm condition is detected by the storm
 *                  controller (the controller's token bucket limit is reached).
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') to which the action should
 *                  be added.
 *
 * \param[in]       action points to an ''fm_stormAction'' structure
 *                  describing the action.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is out of
 *                  range or is not the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ACTION if action is not a valid stormControl
 *                  action.
 * \return          FM_ERR_STORM_ACTION_EXCEEDED if we have exceeded the maximum
 *                  allowed actions (FM4000 only).
 * \return          FM_ERR_INVALID_ARGUMENT if action is NULL.
 *
 *****************************************************************************/
fm_status fmAddStormCtrlAction(fm_int          sw,
                               fm_int          stormController,
                               fm_stormAction *action)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, action = %p\n",
                     sw,
                     stormController,
                     (void *) action);

    if (action == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddStormCtrlAction,
                       sw,
                       stormController,
                       action);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmAddStormCtrlAction */




/*****************************************************************************/
/** fmDeleteStormCtrlAction
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Delete an action from a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the action should be
 *                  deleted.
 *
 * \param[in]       action points to an ''fm_stormAction'' structure
 *                  describing the action to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_INVALID_ACTION if action is not found in the storm
 *                  controller.
 * \return          FM_ERR_INVALID_ARGUMENT if action is NULL.
 *
 *****************************************************************************/
fm_status fmDeleteStormCtrlAction(fm_int          sw,
                                  fm_int          stormController,
                                  fm_stormAction *action)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, action = %p\n",
                     sw,
                     stormController,
                     (void *) action);

    if (action == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteStormCtrlAction,
                       sw,
                       stormController,
                       action);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmDeleteStormCtrlAction */




/*****************************************************************************/
/** fmGetStormCtrlActionList
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Return a list of actions associated with a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the action should be
 *                  retrieved.
 *
 * \param[out]      numActions points to caller-allocated storage where
 *                  this function should place the number of actions
 *                  returned in actionList.
 *
 * \param[out]      actionList is an array that this function will fill with
 *                  the list of actions associated with the storm controller.
 *
 * \param[in]       max is the size of actionList, being the maximum number
 *                  of actions that actionList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of actions.
 * \return          FM_ERR_INVALID_ARGUMENT if numActions or actionList is
 *                  NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlActionList(fm_int          sw,
                                   fm_int          stormController,
                                   fm_int *        numActions,
                                   fm_stormAction *actionList,
                                   fm_int          max)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, numActions = %p, "
                     "actionList = %p, max = %d\n",
                     sw,
                     stormController,
                     (void *) numActions,
                     (void *) actionList,
                     max);

    if ( (numActions == NULL) || (actionList == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlActionList,
                       sw,
                       stormController,
                       numActions,
                       actionList,
                       max);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlActionList */




/*****************************************************************************/
/** fmGetStormCtrlActionFirst
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Retrieve the first action in a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the first action should
 *                  be retrieved.
 *
 * \param[out]      firstAction points to caller-allocated storage where this
 *                  function should place the first action from this storm
 *                  controller. Will be set to -1 if no actions are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_ACTION if no storm controller actions
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if firstAction is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlActionFirst(fm_int          sw,
                                    fm_int          stormController,
                                    fm_stormAction *firstAction)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, firstAction = %p\n",
                     sw,
                     stormController,
                     (void *) firstAction);

    if (firstAction == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlActionFirst,
                       sw,
                       stormController,
                       firstAction);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlActionFirst */




/*****************************************************************************/
/** fmGetStormCtrlActionNext
 * \ingroup storm
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Retrieve the next action in a storm controller.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stormController is the storm controller number (returned by
 *                  ''fmCreateStormCtrl'') from which the next action
 *                  should be retrieved.
 *
 * \param[in]       currentAction points to the last action found by a previous
 *                  call to this function or to ''fmGetStormCtrlActionFirst''.
 *
 * \param[out]      nextAction points to caller-allocated storage where this
 *                  function should place the next action in the storm
 *                  controller. Will be set to -1 if no more actions found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STORM_CTRL if stormController is not
 *                  the handle of an existing storm controller.
 * \return          FM_ERR_NO_STORM_ACTION if no more storm controller actions
 *                  found.
 * \return          FM_ERR_INVALID_ARGUMENT if currentAction or nextAction is NULL.
 *
 *****************************************************************************/
fm_status fmGetStormCtrlActionNext(fm_int          sw,
                                   fm_int          stormController,
                                   fm_stormAction *currentAction,
                                   fm_stormAction *nextAction)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STORM,
                     "sw = %d, stormController = %d, currentAction = %p, "
                     "nextAction = %p\n",
                     sw,
                     stormController,
                     (void *) currentAction,
                     (void *) nextAction);

    if ( (currentAction == NULL) || (nextAction == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STORM, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetStormCtrlActionNext,
                       sw,
                       stormController,
                       currentAction,
                       nextAction);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STORM, err);

}   /* end fmGetStormCtrlActionNext */




/*****************************************************************************/
/** fmDbgDumpStormCtrl
 * \ingroup diagMisc 
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Dumps the specified storm controller.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       stormController is the storm controller number to dump.
 *                  Use -1 for all controllers.
 *
 *
 *****************************************************************************/
void fmDbgDumpStormCtrl(fm_int sw, fm_int stormController)
{

    if ( (sw < 0) || (sw >= FM_MAX_NUM_SWITCHES) )
    {
        FM_LOG_PRINT("ERROR: invalid switch %d\n", sw);
        return;
    }

    PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY_VOID(GET_SWITCH_PTR(sw)->DbgDumpStormCtrl,
                            sw,
                            stormController);

    UNPROTECT_SWITCH(sw);

}   /* end fmDbgDumpStormCtrl */




/*****************************************************************************/
/** fmStormCondTypeToText
 * \ingroup intstorm
 *
 * \desc            Returns the text representation of a storm controller
 *                  condition type.
 *
 * \param[in]       type is the allocation type (see ''fm_stormCondType'').
 *
 * \return          Pointer to a string representing the condition type.
 *
 *****************************************************************************/
const char * fmStormCondTypeToText(fm_stormCondType type)
{

    switch (type)
    {
        case FM_STORM_COND_BROADCAST:
            return "BROADCAST";

        case FM_STORM_COND_IGMP:
            return "IGMP";

        case FM_STORM_COND_802_1X:
            return "802_1X";

        case FM_STORM_COND_BPDU:
            return "BPDU";

        case FM_STORM_COND_LACP:
            return "LACP";

        case FM_STORM_COND_FLOOD:
            return "FLOOD";

        case FM_STORM_COND_FLOOD_UCAST:
            return "FLOOD_UCAST";

        case FM_STORM_COND_FLOOD_MCAST:
            return "FLOOD_MCAST";

        case FM_STORM_COND_FIDFORWARD:
            return "FIDFORWARD";

        case FM_STORM_COND_FIDFORWARD_UCAST:
            return "FIDFORWARD_UCAST";

        case FM_STORM_COND_FIDFORWARD_MCAST:
            return "FIDFORWARD_MCAST";

        case FM_STORM_COND_MULTICAST:
            return "MULTICAST";

        case FM_STORM_COND_LOG_ICMP:
            return "LOG_ICMP";

        case FM_STORM_COND_TRAP_ICMP:
            return "TRAP_ICMP";

        case FM_STORM_COND_CPU:
            return "CPU";

        case FM_STORM_COND_SECURITY_VIOL_NEW_MAC:
            return "SECURITY_VIOL_NEW_MAC";

        case FM_STORM_COND_SECURITY_VIOL_MOVE:
            return "SECURITY_VIOL_MOVE";

        case FM_STORM_COND_INGRESS_PORT:
            return "INGRESS_PORT";

        case FM_STORM_COND_EGRESS_PORT:
            return "EGRESS_PORT";

        case FM_STORM_COND_UNICAST:
            return "UNICAST";

        case FM_STORM_COND_NEXTHOP_MISS:
            return "NEXTHOP_MISS";

        case FM_STORM_COND_INGRESS_PORTSET:
            return "INGRESS_PORTSET";

        case FM_STORM_COND_EGRESS_PORTSET:
            return "EGRESS_PORTSET";

        case FM_STORM_COND_RESERVED_MAC:
            return "RESERVED_MAC";

        default:
            return "Unknown";

    }   /* end switch (type) */

}   /* end fmStormCondTypeToText */




/*****************************************************************************/
/** fmStormActionTypeToText
 * \ingroup intstorm
 *
 * \desc            Returns the text representation of a storm controller
 *                  action type.
 *
 * \param[in]       type is the allocation type (see ''fm_stormActionType'').
 *
 * \return          Pointer to a string representing the action type.
 *
 *****************************************************************************/
const char * fmStormActionTypeToText(fm_stormActionType type)
{

    switch (type)
    {
        case FM_STORM_ACTION_DO_NOTHING:
            return "DO_NOTHING";

        case FM_STORM_ACTION_FILTER_PORT:
            return "FILTER_PORT";

        case FM_STORM_ACTION_FILTER_PORTSET:
            return "FILTER_PORTSET";

        default:
            return "Unknown";

    }   /* end switch (type) */

}   /* end fmStormActionTypeToText */
