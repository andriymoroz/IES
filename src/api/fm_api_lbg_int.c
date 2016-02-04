/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lbg_int.c
 * Creation Date:   October 8, 2013
 * Description:     Internal functions for managing load balancing groups.
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

typedef fm_status (*fm_LBGPortTransHandler)(fm_int sw,
                                            fm_LBGGroup *group,
                                            fm_intLBGMember *member,
                                            fm_int oldMode,
                                            fm_int newMode,
                                            fm_bool *hwDistChanged);

typedef struct _fm_LBGPortTransTable
{
    /* The group state this transition is active in */
    fm_int                 groupState;

    /* The port modes of the transition */
    fm_int                 oldMode;
    fm_int                 newMode;

    /* Function to handle the change */
    fm_LBGPortTransHandler handler;

} fm_LBGPortTransTable;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_status HandlePortActiveToFailover(fm_int sw,
                                            fm_LBGGroup *group,
                                            fm_intLBGMember *member,
                                            fm_int oldMode,
                                            fm_int newMode,
                                            fm_bool *hwDistChanged);

static fm_status HandlePortFailoverToActive(fm_int sw,
                                            fm_LBGGroup *group,
                                            fm_intLBGMember *member,
                                            fm_int oldMode,
                                            fm_int newMode,
                                            fm_bool *hwDistChanged);

static fm_status HandlePortFailoverToStandby(fm_int sw,
                                             fm_LBGGroup *group,
                                             fm_intLBGMember *member,
                                             fm_int oldMode,
                                             fm_int newMode,
                                             fm_bool *hwDistChanged);

static fm_status HandlePortStandbyToActive(fm_int sw,
                                           fm_LBGGroup *group,
                                           fm_intLBGMember *member,
                                           fm_int oldMode,
                                           fm_int newMode,
                                           fm_bool *hwDistChanged);

static fm_status HandleRedistribution(fm_int sw,
                                      fm_LBGGroup *group,
                                      fm_intLBGMember *member,
                                      fm_int oldMode,
                                      fm_int newMode,
                                      fm_bool *hwDistChanged);

static fm_status GetTransitionHandler(fm_int state, 
                                      fm_int oldMode, 
                                      fm_int newMode, 
                                      fm_LBGPortTransHandler *handler);

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_LBGPortTransTable validPortModeTransitions[] =
{
    { FM_LBG_STATE_INACTIVE, -1,                   -1,                    HandleRedistribution        },
    { FM_LBG_STATE_ACTIVE,   FM_LBG_PORT_ACTIVE,   FM_LBG_PORT_FAILOVER,  HandlePortActiveToFailover  },
    { FM_LBG_STATE_ACTIVE,   FM_LBG_PORT_FAILOVER, FM_LBG_PORT_ACTIVE,    HandlePortFailoverToActive  },
    { FM_LBG_STATE_ACTIVE,   FM_LBG_PORT_FAILOVER, FM_LBG_PORT_STANDBY,   HandlePortFailoverToStandby },
    { FM_LBG_STATE_ACTIVE,   FM_LBG_PORT_STANDBY,  FM_LBG_PORT_ACTIVE,    HandlePortStandbyToActive   },
    { FM_LBG_STATE_ACTIVE,   FM_LBG_PORT_STANDBY,  FM_LBG_PORT_INACTIVE,  NULL   },
    { FM_LBG_STATE_ACTIVE,   FM_LBG_PORT_INACTIVE, FM_LBG_PORT_STANDBY,   NULL   },
};

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** ClearUserDistribution
 * \ingroup intLbg
 *
 * \desc            Resets the user distribution to nothing (all bins are -1).
 *                  This only affects the soft-state.
 *
 * \param[out]      group is the group state object being reset.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status ClearUserDistribution(fm_LBGGroup *group)
{
    fm_int           bin;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, "group=%p\n", (void *) group);

    if (!group)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    for ( bin = 0 ; bin < group->numBins ; bin++ )
    {
        group->userDistribution[bin] = FM_PORT_DROP;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end ClearUserDistribution */




/*****************************************************************************/
/** GetNextMember
 * \ingroup intLbg
 *
 * \desc            Iterates through the member list, looking for members
 *                  of the given type.
 *
 * \param[out]      group is the group state object.
 *
 * \param[out]      currentMember is the starting member to search from.
 *
 * \param[in]       includeActive indicates whether to include active ports.
 *
 * \param[in]       includeStandby indicates whether to include standby ports
 *                  regardless of their usage state.
 *
 * \param[in]       includeStandbyFree indicates whether to include standby ports
 *                  that are not used.  Note that setting this argument to TRUE,
 *                  and includeStandby to FALSE is the appropriate way to pick
 *                  only unused standby ports.
 *
 * \param[in]       includeFailover indicates whether to include ports in
 *                  failover.
 *
 * \param[in]       includeInactive indicates whether to include inactive ports.
 *
 * \return          A pointer to the found member, or NULL if there is none.
 *
 *****************************************************************************/
static fm_intLBGMember *GetNextMember(fm_LBGGroup *group,
                                      fm_intLBGMember *currentMember,
                                      fm_bool includeActive,
                                      fm_bool includeStandby,
                                      fm_bool includeStandbyFree,
                                      fm_bool includeFailover,
                                      fm_bool includeInactive)
{
    fm_intLBGMember *startingMember;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "group=%p, currentMember=%p includeActive=%d includeStandby=%d "
                 "includeStandbyFree=%d, includeFailover=%d includeInactive=%d\n",
                 (void *) group,
                 (void *) currentMember,
                 includeActive,
                 includeStandby,
                 includeStandbyFree,
                 includeFailover,
                 includeInactive);

    /* Remember the starting point */
    if (currentMember)
    {
        startingMember = currentMember;
    }
    else
    {
        startingMember = group->firstMember;
        currentMember = group->firstMember;
    }

    /* No entries in list... */
    if (!currentMember)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LBG, NULL, "member=%p", (void *) NULL);
    }

    do
    {
        if ( (includeActive       && (currentMember->mode == FM_LBG_PORT_ACTIVE))   ||
             (includeStandby      && (currentMember->mode == FM_LBG_PORT_STANDBY))  ||
             (includeStandbyFree  && (currentMember->mode == FM_LBG_PORT_STANDBY) &&
                                     (!currentMember->standbyUsed))                 ||
             (includeFailover     && (currentMember->mode == FM_LBG_PORT_FAILOVER)) ||
             (includeInactive     && (currentMember->mode == FM_LBG_PORT_INACTIVE)) )
        {
            FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LBG,
                               currentMember,
                               "member=%p",
                               (void *) currentMember);
        }

        /* Skip to next member or loop around */
        if (currentMember->nextMember)
        {
            currentMember = currentMember->nextMember;
        }
        else
        {
            currentMember = group->firstMember;
        }

    } while (currentMember != startingMember);

    /* No entries found */
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LBG, NULL, "member=%p\n", (void *) NULL);

}   /* end GetNextMember */




/*****************************************************************************/
/** RedistributeFailoverSlot
 * \ingroup intLbg
 *
 * \desc            Redistributes the slots belonging to the given port
 *                  which has gone into failover.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      group is the group state object.
 *
 * \param[out]      member is the member state object for the port in failover.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RedistributeFailoverSlot(fm_int sw,
                                          fm_LBGGroup *group,
                                          fm_intLBGMember *member)
{
    fm_intLBGMember *memberPort;
    fm_intLBGMember *nextAvailStandby = NULL;
    fm_bool          drop = FALSE;
    fm_bool          stripeAll = FALSE;
    fm_bool          stripeStandby = FALSE;
    fm_int           newPort = -1;
    fm_int           bin;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, member=%p\n",
                 sw,
                 (void *) group,
                 (void *) member);

    FM_NOT_USED(sw);

    if ( (group->redirectMode == FM_LBG_REDIRECT_STANDBY) ||
         (group->redirectMode == FM_LBG_REDIRECT_PREFER_STANDBY) )
    {
        nextAvailStandby = GetNextMember(group,
                                         NULL,   /* Start at the first member */
                                         FALSE,  /* Skip active ports */
                                         FALSE,  /* Skip all standby ports (regardless of use) */
                                         TRUE,   /* Include unused standby ports, overrides the above */
                                         FALSE,  /* Skip failover ports */
                                         FALSE); /* Skip inactive ports */

        if (!nextAvailStandby)
        {
            /***************************************************
             * Standby only, but no standby ports available,
             * all bins using the failover member will start
             * dropping frames.
             **************************************************/
            if (group->redirectMode == FM_LBG_REDIRECT_STANDBY)
            {
                drop = TRUE;
            }
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_LBG,
                 "Processing a redirect mode of %d\n",
                 group->redirectMode);

    switch (group->redirectMode)
    {
        case FM_LBG_REDIRECT_ALL_PORTS:
            stripeAll = TRUE;
            break;

        case FM_LBG_REDIRECT_PORT:
            newPort = member->redirectTarget;
            break;

        case FM_LBG_REDIRECT_PREFER_STANDBY:
            if (nextAvailStandby)
            {
                newPort = nextAvailStandby->lbgMemberPort;
                nextAvailStandby->standbyUsed = TRUE;
            }
            else
            {
                stripeStandby = TRUE;
            }
            break;

        case FM_LBG_REDIRECT_STANDBY:
            stripeStandby = TRUE;
            break;

        default:
            FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                          FALSE,
                         "Unknown LBG redirect mode %d\n",
                         group->redirectMode);
            break;
    }

    FM_LOG_DEBUG(FM_LOG_CAT_LBG,
                 "stripeAll=%d stripeStandby=%d drop=%d newPort=%d\n",
                 stripeAll,
                 stripeStandby,
                 drop,
                 newPort);

    /***************************************************
     * Use the last member port used as starting point 
     * to improve hashing distribution (if striping 
     * required)
     **************************************************/

    if (stripeAll || stripeStandby)
    {
        memberPort = group->lastStripeMember;
    }
    else
    {
        memberPort = NULL;
    }

    for ( bin = 0 ; bin < group->numBins ; bin++ )
    {
        if ( group->hwDistribution[bin] == member->lbgMemberPort )
        {
            if (stripeAll)
            {
                memberPort = GetNextMember(group,
                                           (memberPort ? memberPort->nextMember : NULL),
                                           TRUE,   /* Include active ports */
                                           TRUE,   /* Include standby ports */
                                           FALSE,  /* Ignore standby in use */
                                           FALSE,  /* Skip failover ports */
                                           FALSE); /* Skip inactive ports */

                group->hwDistribution[bin] = (memberPort ?
                                              memberPort->lbgMemberPort :
                                              FM_PORT_DROP);

                /* Keep track of the last member port striped on */
                group->lastStripeMember = memberPort;
            }
            else if (stripeStandby)
            {
                memberPort = GetNextMember(group,
                                           (memberPort ? memberPort->nextMember : NULL),
                                           FALSE,  /* Include active ports */
                                           TRUE,   /* Include standby ports */
                                           FALSE,  /* Ignore standby in use */
                                           FALSE,  /* Skip standby ports */
                                           FALSE); /* Skip inactive ports */

                group->hwDistribution[bin] = (memberPort ?
                                              memberPort->lbgMemberPort :
                                              FM_PORT_DROP);

                /* Keep track of the last member port striped on */
                group->lastStripeMember = memberPort;
            }
            else if (newPort != -1)
            {
                group->hwDistribution[bin] = newPort;
            }
            else if (drop)
            {
                group->hwDistribution[bin] = FM_PORT_DROP;
            }
            else
            {
                FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                              FALSE,
                              "Unhandled LBG redirect case: "
                              "stripeAll=%d stripeStandby=%d "
                              "newPort=%d drop=%d\n",
                              stripeAll,
                              stripeStandby,
                              newPort,
                              drop);

                FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_FAIL);
            }

            FM_LOG_DEBUG(FM_LOG_CAT_LBG, "Replaced bin %d with port %d\n",
                         bin, group->hwDistribution[bin]);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end RedistributeFailoverSlot */




/*****************************************************************************/
/** GetTransitionHandler
 * \ingroup intLbg
 *
 * \desc            Validates that the transition is valid from oldMode to
 *                  newMode in the given state. If valid, the transition
 *                  handler is returned. 
 * 
 * \param[in]       state is the LBG state for which the transition should
 *                  be validated against.
 * 
 * \param[in]       oldMode is the old port mode.
 * 
 * \param[in]       newMode is the new port mode we want to transition to.
 * 
 * \param[out]      handler is a pointer to the caller allocated storage
 *                  where th the transition handler pointer should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetTransitionHandler(fm_int state, 
                                      fm_int oldMode, 
                                      fm_int newMode, 
                                      fm_LBGPortTransHandler *handler)
{
    fm_int    numAllowedTransitions;
    fm_int    j;

    FM_LOG_DEBUG(FM_LOG_CAT_LBG,
                 "Searching for mode transition %d to %d\n",
                 oldMode, newMode);

    numAllowedTransitions = (sizeof(validPortModeTransitions) /
                             sizeof(fm_LBGPortTransTable));

    for ( j = 0 ; j < numAllowedTransitions ; j++ )
    {
        if ( ( state        == validPortModeTransitions[j].groupState ) &&
             ( -1           == validPortModeTransitions[j].oldMode ) &&
             ( -1           == validPortModeTransitions[j].newMode ) )
        {
            break;
        }
        else if ( ( state   == validPortModeTransitions[j].groupState ) &&
                  ( oldMode == validPortModeTransitions[j].oldMode ) &&
                  ( newMode == validPortModeTransitions[j].newMode ) )
        {
            break;
        }
        else if ( ( state   == validPortModeTransitions[j].groupState ) &&
                  ( -1      == validPortModeTransitions[j].oldMode ) &&
                  ( newMode == validPortModeTransitions[j].newMode ) )
        {
            break;
        }
        else if ( ( state   == validPortModeTransitions[j].groupState ) &&
                  ( oldMode == validPortModeTransitions[j].oldMode ) &&
                  ( -1      == validPortModeTransitions[j].newMode ) )
        {
            break;
        }
    }

    if (j >= numAllowedTransitions)
    {
        return FM_ERR_INVALID_LBG_PORT_TRANS;
    }

    *handler = validPortModeTransitions[j].handler;

    return FM_OK;

}   /* end GetTransitionHandler */




/*****************************************************************************/
/** HandlePortActiveToFailover
 * \ingroup intLbg
 *
 * \desc            Handles a port transition from active into failover.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      group is the state object for this LBG.
 *
 * \param[out]      member is the LBG member state for the port that is
 *                  transitioning.
 *
 * \param[in]       oldMode is the previous LBG port mode.
 *
 * \param[in]       newMode is the new LBG port mode.
 * 
 * \param[out]      hwDistChanged point to the caller allocated storage
 *                  where this function should store the flag to indicate
 *                  that the hwDistribution has changed. This is used to
 *                  inform the caller that a HW update may be required.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandlePortActiveToFailover(fm_int sw,
                                            fm_LBGGroup *group,
                                            fm_intLBGMember *member,
                                            fm_int oldMode,
                                            fm_int newMode,
                                            fm_bool *hwDistChanged)
{
    fm_status        err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, member=%p, oldMode=%d, newMode=%d\n",
                 sw,
                 (void *) group,
                 (void *) member,
                 oldMode,
                 newMode);

    FM_NOT_USED(oldMode);
    FM_NOT_USED(newMode);

    FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                  newMode == FM_LBG_PORT_FAILOVER,
                  "New mode for HandlePortActiveToFailover "
                  "can only be failover\n");

    group->numFailover++;
    group->numActive--;

    err = RedistributeFailoverSlot(sw, group, member);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    *hwDistChanged = TRUE;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end HandlePortActiveToFailover */




/*****************************************************************************/
/** HandlePortFailoverToActive
 * \ingroup intLbg
 *
 * \desc            Handles a port transition from failover into active.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      group is the state object for this LBG.
 *
 * \param[out]      member is the LBG member state for the port that is
 *                  transitioning.
 *
 * \param[in]       oldMode is the previous LBG port mode.
 *
 * \param[in]       newMode is the new LBG port mode.
 * 
 * \param[out]      hwDistChanged point to the caller allocated storage
 *                  where this function should store the flag to indicate
 *                  that the hwDistribution has changed. This is used to
 *                  inform the caller that a HW update may be required.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandlePortFailoverToActive(fm_int sw,
                                            fm_LBGGroup *group,
                                            fm_intLBGMember *member,
                                            fm_int oldMode,
                                            fm_int newMode,
                                            fm_bool *hwDistChanged)
{
    fm_status        err = FM_OK;
    fm_int           bin;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, member=%p, oldMode=%d, newMode=%d\n",
                 sw,
                 (void *) group,
                 (void *) member,
                 oldMode,
                 newMode);

    FM_NOT_USED(oldMode);
    FM_NOT_USED(newMode);

    FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                  newMode == FM_LBG_PORT_ACTIVE,
                  "New mode for HandlePortFailoverToActive "
                  "can only be active\n");

    group->numFailover--;
    group->numActive++;

    /* Reset hw distribution for those slots back to the user dist */
    for ( bin = 0 ; bin < group->numBins ; bin++ )
    {
        if (group->userDistribution[bin] == member->lbgMemberPort)
        {
            group->hwDistribution[bin] = member->lbgMemberPort;
        }
    }

    *hwDistChanged = TRUE;
    
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end HandlePortFailoverToActive */




/*****************************************************************************/
/** HandlePortFailoverToStandby
 * \ingroup intLbg
 *
 * \desc            Handles a port transition from failover into standby.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      group is the state object for this LBG.
 *
 * \param[out]      member is the LBG member state for the port that is
 *                  transitioning.
 *
 * \param[in]       oldMode is the previous LBG port mode.
 *
 * \param[in]       newMode is the new LBG port mode.
 * 
 * \param[out]      hwDistChanged point to the caller allocated storage
 *                  where this function should store the flag to indicate
 *                  that the hwDistribution has changed. This is used to
 *                  inform the caller that a HW update may be required.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandlePortFailoverToStandby(fm_int sw,
                                             fm_LBGGroup *group,
                                             fm_intLBGMember *member,
                                             fm_int oldMode,
                                             fm_int newMode,
                                             fm_bool *hwDistChanged)
{
    fm_status        err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, member=%p, oldMode=%d, newMode=%d\n",
                 sw,
                 (void *) group,
                 (void *) member,
                 oldMode,
                 newMode);

    FM_NOT_USED(oldMode);
    FM_NOT_USED(newMode);

    FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                  newMode == FM_LBG_PORT_STANDBY,
                  "New mode for HandlePortFailoverToStandby "
                  "can only be standby\n");

    group->numFailover--;
    
    /* The port should not be in use, don't need to update HW */
    *hwDistChanged = FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end HandlePortFailoverToStandby */




/*****************************************************************************/
/** HandlePortStandbyToActive
 * \ingroup intLbg
 *
 * \desc            Handles a port transition from standby into active.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      group is the state object for this LBG.
 *
 * \param[out]      member is the LBG member state for the port that is
 *                  transitioning.
 *
 * \param[in]       oldMode is the previous LBG port mode.
 *
 * \param[in]       newMode is the new LBG port mode.
 * 
 * \param[out]      hwDistChanged point to the caller allocated storage
 *                  where this function should store the flag to indicate
 *                  that the hwDistribution has changed. This is used to
 *                  inform the caller that a HW update may be required.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandlePortStandbyToActive(fm_int sw,
                                           fm_LBGGroup *group,
                                           fm_intLBGMember *member,
                                           fm_int oldMode,
                                           fm_int newMode,
                                           fm_bool *hwDistChanged)
{
    fm_status        err = FM_OK;
    fm_int           bin;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, member=%p, oldMode=%d, newMode=%d\n",
                 sw,
                 (void *) group,
                 (void *) member,
                 oldMode,
                 newMode);

    FM_NOT_USED(oldMode);
    FM_NOT_USED(newMode);

    FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                  newMode == FM_LBG_PORT_ACTIVE,
                  "New mode for HandlePortStandbyToActive "
                  "can only be active\n");

    group->numActive++;

    member->standbyUsed = FALSE;
    
    /* Find all HW entries that are using the standby port and update the
     * user table. If this port was not being used as standby, this will 
     * not change the user dist. */
    for ( bin = 0 ; bin < group->numBins ; bin++ )
    {
        if (group->hwDistribution[bin] == member->lbgMemberPort)
        {
            group->userDistribution[bin] = member->lbgMemberPort;
        }
    }

    *hwDistChanged = FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end HandlePortStandbyToActive */




/*****************************************************************************/
/** HandleRedistribution
 * \ingroup intLbg
 *
 * \desc            Handles any port transition in inactive mode, causing a
 *                  full restriping.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      group is the state object for this LBG.
 *
 * \param[out]      member is the LBG member state for the port that is
 *                  transitioning.
 *
 * \param[in]       oldMode is the previous LBG port mode.
 *
 * \param[in]       newMode is the new LBG port mode.
 * 
 * \param[out]      hwDistChanged point to the caller allocated storage
 *                  where this function should store the flag to indicate
 *                  that the hwDistribution has changed. This is used to
 *                  inform the caller that a HW update may be required.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandleRedistribution(fm_int sw,
                                      fm_LBGGroup *group,
                                      fm_intLBGMember *member,
                                      fm_int oldMode,
                                      fm_int newMode,
                                      fm_bool *hwDistChanged)
{
    fm_status        err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, member=%p, oldMode=%d, newMode=%d\n",
                 sw, (void *) group, (void *) member, oldMode, newMode);

    FM_NOT_USED(sw);
    FM_NOT_USED(member);

    /* Update counts */
    switch (oldMode)
    {
        case FM_LBG_PORT_ACTIVE:
            group->numActive--;
            break;

        case FM_LBG_PORT_FAILOVER:
            group->numFailover--;
            break;
    }

    switch (newMode)
    {
        case FM_LBG_PORT_ACTIVE:
            group->numActive++;
            break;

        case FM_LBG_PORT_FAILOVER:
            group->numFailover++;
            break;
    }

    err = fmCommonResetLBGDistributionForRedirect(sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    *hwDistChanged = TRUE;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end HandleRedistribution */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmAllocateLBGDataStructures
 * \ingroup intLbg
 *
 * \desc            This method is called upon switch insertion.
 *
 * \param[in]       switchPtr points to the switch structure for the switch
 *                  on which to operate.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAllocateLBGDataStructures(fm_switch *switchPtr)
{
    fm_status   err  = FM_OK;
    fm_LBGInfo *info = &switchPtr->lbgInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    err = fmCreateLockV2("LBGLock", 
                         switchPtr->switchNumber,
                         FM_LOCK_PREC_LBGS,
                         &info->lbgLock);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmAllocateLBGDataStructures */




/*****************************************************************************/
/** fmFreeLBGDataStructures
 * \ingroup intLbg
 *
 * \desc            This method is called upon switch removal.
 *
 * \param[in]       switchPtr points to the switch state table of the switch.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeLBGDataStructures(fm_switch *switchPtr)
{
    fm_status   err  = FM_OK;
    fm_LBGInfo *info = &switchPtr->lbgInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    err = fmDeleteLock(&info->lbgLock);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmFreeLBGDataStructures */




/*****************************************************************************/
/** fmAllocateLBGsInt
 * \ingroup intLbg 
 *
 * \desc            Allocate LBGs given a glort range. The function returns
 *                  the base LBG number and the number of handles created.
 *                  The caller can then emumerate these handles by 'step' up to 
 *                  the number of handles allocated. These handles will have 
 *                  the same CAM resources across multiple switches, given the
 *                  input glort information is the same.
 *
 * \note            The return base handle might not be the same on different
 *                  switches. However the cam resources for 
 *                  (baseLbgNumber + n*step) will be consistent on 
 *                  different switches when using ''fmCreateStackLBG''..
 *                  In addition, this API can also be used to reserve specific
 *                  LBG resources on standalone switches. In this case,
 *                  calling ''fmCreateLBG'' will just provide a free entry
 *                  in this reserved pool first.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the starting glort to use to reserve 
 *                  for LBGs
 *
 * \param[in]       glortSize is the glort size to use. This value must be a
 *                  power of two.
 *
 * \param[out]      baseLbgNumber points to caller-allocated storage 
 *                  where this function should place the base LBG number
 *                  (handle) of the newly allocated LBGs pool.
 *
 * \param[out]      numLbgs points to caller-allocated storage 
 *                  where this function should place the number of LBGs
 *                  allocated given the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage 
 *                  where this function should place the step value, where
 *                  the caller can increment from base by to get
 *                  subsequent LBG numbers
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if baseLbgNumber, numLbgs, or 
 *                  step is NULL or input parameters fail checking.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  LBG structure.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort or port resources
 *                  required in in the given glort range is being used.
 * \return          FM_ERR_NO_LBG_RESOURCES if no more resoures are available
 *
 *****************************************************************************/
fm_status fmAllocateLBGsInt(fm_int     sw,
                            fm_uint    startGlort,
                            fm_uint    glortSize,
                            fm_int    *baseLbgNumber,
                            fm_int    *numLbgs,
                            fm_int    *step)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_int       baseHandle;
    fm_int       numHandles;
    fm_int       off;
 
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw = %d, startGlort = %u, glortSize = %u, "
                 "baseLbgNumber = %p, numLbgs = %p, step = %p\n",
                 sw,
                 startGlort,
                 glortSize,
                 (void *) baseLbgNumber,
                 (void *) numLbgs,
                 (void *) step);

    *numLbgs = 0;

    switchPtr = GET_SWITCH_PTR(sw);
 
    FM_API_CALL_FAMILY(err, 
                       switchPtr->AllocateLBGs,
                       sw,
                       startGlort,
                       glortSize,
                       &baseHandle,
                       &numHandles,
                       &off);

    if (err == FM_OK)
    {
        /* Don't need to remap, use this directly */
        *numLbgs       = numHandles;
        *baseLbgNumber = baseHandle;
        *step          = off;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmAllocateLBGsInt */




/*****************************************************************************/
/** fmFreeLBGsInt
 * \ingroup intLbg 
 *
 * \desc            Free LBGs previously created with ''fmAllocateLBGsInt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseLbgNumber is the base number previously created with
 *                  ''fmAllocateLBGsInt''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if baseLbgNumber is not found
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fmFreeLBGsInt(fm_int    sw,
                        fm_int    baseLbgNumber)
{
    fm_status   err;
    fm_switch * switchPtr;
 
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw = %d, baseLbgNumber = %d\n",
                 sw,
                 baseLbgNumber);

    switchPtr = GET_SWITCH_PTR(sw);
 
    FM_API_CALL_FAMILY(err,
                       switchPtr->FreeLBGs,
                       sw,
                       baseLbgNumber);


    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmFreeLBGsInt */




/*****************************************************************************/
/** fmCommonFindLBGMember
 * \ingroup intLbg
 *
 * \desc            Returns the member structure associated with the requested
 *                  port if found.
 *
 * \param[in]       group is a pointer to the LBG group in which the port should
 *                  be searched.
 * 
 * \param[in]       port is the port searched for.
 * 
 * \param[out]      member is the caller allocated storage where the LBG member
 *                  pointer should be stored. 
 *
 * \return          FM_OK if found
 * \return          FM_ERR_NOT_FOUND if not found
 *
 *****************************************************************************/
fm_status fmCommonFindLBGMember(fm_LBGGroup *  group, 
                                fm_int         port, 
                                fm_intLBGMember **member)
{
    fm_intLBGMember *m;

    if (member == NULL)
    {
        return FM_FAIL;
    }

    /* Iterate through the members to see if port is a member */
    m = group->firstMember; 

    while ( m != NULL )
    {
        if (m->lbgMemberPort == port)
        {
            *member = m;
            return FM_OK;
        }

        m = m->nextMember;
    }

    return FM_ERR_NOT_FOUND;

}   /* end fmCommonFindLBGMember */




/*****************************************************************************/
/** fmCommonResetLBGDistributionForRedirect
 * \ingroup intLbg
 *
 * \desc            This updates the distribution for when the
 *                  mode is FM_LBG_MODE_REDIRECT;
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group points to the state structure for the group.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCommonResetLBGDistributionForRedirect(fm_int sw, fm_LBGGroup *group)
{
    fm_status           err = FM_OK;
    fm_intLBGMember *   lbgMember;
    fm_int              bin = 0;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d group=%p\n",
                 sw, (void *) group);

    if (group->lbgMode != FM_LBG_MODE_REDIRECT)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    /* Clear old distribution */
    err = ClearUserDistribution(group);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_LBG, err);

    group->lastStripeMember = NULL;
    
    /* Only if there are valid ports to distribute to... */
    if ((group->numActive + group->numFailover) > 0)
    {
        /* Ensure all bins are filled */
        while (bin < group->numBins)
        {
            /* Start out with the actual distribution for all members */
            for ( lbgMember = group->firstMember ;
                  (lbgMember && (bin < group->numBins)) ;
                  lbgMember = lbgMember->nextMember )
            {
                /* Mark each member as not in standby use */
                lbgMember->standbyUsed = FALSE;

                if ((lbgMember->mode == FM_LBG_PORT_ACTIVE) ||
                    (lbgMember->mode == FM_LBG_PORT_FAILOVER))
                {
                    FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                                  ((bin >= 0) && (bin < group->numBins)),
                                  "During redistribution, bin has gone "
                                  "out of range: %d\n",
                                  bin);

                    group->userDistribution[bin] = lbgMember->lbgMemberPort;

                    /* Copy into the hwDistribution for now, we will update later */
                    group->hwDistribution[bin] = lbgMember->lbgMemberPort;

                    /* Keep track of the last member port striped on */
                    group->lastStripeMember = lbgMember;

                    bin++;
                }
            }
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_LBG,
                     "No active or failover bins, all bins will drop frames\n");
    }

    /* Handle failover cases */
    for ( lbgMember = group->firstMember ;
          lbgMember ;
          lbgMember = lbgMember->nextMember )
    {
        if (lbgMember->mode == FM_LBG_PORT_FAILOVER)
        {
            RedistributeFailoverSlot(sw, group, lbgMember);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmCommonResetLBGDistributionForRedirect */




/*****************************************************************************/
/** fmCommonHandleLBGPortModeTransition
 * \ingroup intLbg
 *
 * \desc            Validates and applies port mode transitions
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group points to the state structure for the group.
 * 
 * \param[in]       member points to the member who's mode should be
 *                  changed to newMode.
 * 
 * \param[in]       newMode is the newMode to transition to.
 * 
 * \param[out]      hwDistChanged point to the caller allocated storage
 *                  where this function should store the flag to indicate
 *                  that the hwDistribution has changed. This is used to
 *                  inform the caller that a HW update may be required.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCommonHandleLBGPortModeTransition(fm_int sw, 
                                              fm_LBGGroup *group, 
                                              fm_intLBGMember *member, 
                                              fm_int newMode,
                                              fm_bool *hwDistChanged)
{
    fm_status              err = FM_OK;
    fm_int                 oldMode;
    fm_LBGPortTransHandler transHandler;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, 
                 "sw = %d, group = %p, member = %p, newMode = %d\n",
                 sw, 
                 (void *) group, 
                 (void *) member, 
                 newMode);

    if ( (group == NULL) || 
         (member == NULL) ||
         (hwDistChanged == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    /* Validate that the transition is valid and retrieve handler */
    err = GetTransitionHandler(group->state, 
                               member->mode, 
                               newMode, 
                               &transHandler);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    /***************************************************
     * Update and apply the new mode
     **************************************************/

    oldMode      = member->mode;
    member->mode = newMode;

    /* Call the appropriate handler, if needed (non NULL) */
    if (transHandler != NULL)
    {
        err = transHandler(sw,
                           group,
                           member,
                           oldMode,
                           newMode,
                           hwDistChanged);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fmCommonHandleLBGPortModeTransition */



/*****************************************************************************/
/** fmResetLBGMember
 * \ingroup intLbg
 *
 * \desc            Resets LBGMember to initial values.
 *
 * \param[out]      lbgMember is the LBG member which should be reset.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmResetLBGMember(fm_LBGMember *lbgMember)
{
    fm_status  err = FM_OK;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LBG,
                         "lbgMember=%p\n",
                         (void *) lbgMember );

    if (lbgMember == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    /* Reset lbgMember to default values */
    FM_CLEAR(*lbgMember);
    lbgMember->lbgMemberType = FM_LBG_MEMBER_TYPE_PORT;
    lbgMember->port          = FM_PORT_DROP;
    lbgMember->mcastGroup    = -1;
    lbgMember->l234Lbg       = -1;
    lbgMember->tunnelGrp     = -1;
    lbgMember->tunnelRule    = -1;

ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LBG, err);

}   /* end fmResetLBGMember */




/*****************************************************************************/
/** fmCopyLBGMember
 * \ingroup intLbg
 *
 * \desc            Updates LBG member of bin based on the member type. This
 *                  function assumes that srcLbgMember is already validated.
 *
 * \param[out]      destLbgMember is the destination LBG member in which values
 *                  should be stored.
 *
 * \param[in]       srcLbgMember is the source LBG member from which values
 *                  should be retrieved.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCopyLBGMember(fm_LBGMember *destLbgMember,
                          fm_LBGMember *srcLbgMember)
{
    fm_status  err = FM_OK;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LBG,
                         "destLbgMember=%p, srcLbgMember=%p\n",
                         (void *) destLbgMember, (void *) srcLbgMember );

    if (destLbgMember == NULL ||
        srcLbgMember == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    err = fmResetLBGMember(destLbgMember);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    /* Update specific fields based on LBG member type and leave the
     * remaining fields as default value. */
    switch(srcLbgMember->lbgMemberType)
    {
        case FM_LBG_MEMBER_TYPE_PORT:
            destLbgMember->port       = srcLbgMember->port;
            break;

        case FM_LBG_MEMBER_TYPE_MAC_ADDR:
            destLbgMember->dmac       = srcLbgMember->dmac;
            destLbgMember->egressVlan = srcLbgMember->egressVlan;
            destLbgMember->vrid       = srcLbgMember->vrid;
            break;

        case FM_LBG_MEMBER_TYPE_MCAST_GROUP:
            destLbgMember->mcastGroup = srcLbgMember->mcastGroup;
            break;

        case FM_LBG_MEMBER_TYPE_L234_LBG:
            destLbgMember->l234Lbg    = srcLbgMember->l234Lbg;
            break;

        case FM_LBG_MEMBER_TYPE_TUNNEL:
            destLbgMember->tunnelGrp  = srcLbgMember->tunnelGrp;
            destLbgMember->tunnelRule = srcLbgMember->tunnelRule;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }
    destLbgMember->lbgMemberType = srcLbgMember->lbgMemberType;

ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LBG, err);

}   /* end fmCopyLBGMember */



