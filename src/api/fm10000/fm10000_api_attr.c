/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_attr.c
 * Creation Date:   May 13th, 2013
 * Description:     Functions for manipulating high level attributes of a switch
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM10000_NB_HASH_ROT                 4
#define BITS_IN_UINT32                      32

#define DEFAULT_MAX_MTU_BYTES               0x3fff

/* Default FM_SWITCH_MPLS_ETHER_TYPES. */
#define DEFAULT_MPLS_TAG_1                  0x8847
#define DEFAULT_MPLS_TAG_2                  0x8848


#define VALIDATE_VALUE_IS_BOOL(value)                                          \
    if (!(*((fm_bool *) (value)) == FM_ENABLED ||                              \
        *((fm_bool *) (value)) == FM_DISABLED))                                \
    {                                                                          \
        err = FM_ERR_INVALID_ARGUMENT;                                         \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);                             \
    }

/* IP Options trigger rule numbers. */
enum
{
    FM10000_TRIGGER_RULE_IP_OPTIONS1 = 10,
    FM10000_TRIGGER_RULE_IP_OPTIONS2 = 20,
};

#define UPDATE_ROUTE_TCAM_SLICE(firstPartition,             \
                                lastPartition,              \
                                firstRouteSlice,            \
                                lastRouteSlice)             \
    if (firstPartition >= 0)                                \
    {                                                       \
        if (firstPartition < firstRouteSlice)               \
        {                                                   \
            firstRouteSlice = firstPartition;               \
        }                                                   \
                                                            \
        if (lastPartition > lastRouteSlice)                 \
        {                                                   \
            lastRouteSlice  = lastPartition;                \
        }                                                   \
    }                                                           


#define DO_SLICES_OVERLAP(owner1First,                      \
                          owner1Last,                       \
                          owner2First,                      \
                          owner2Last,                       \
                          overlap)                          \
    if ( (owner1First < 0) || (owner2First < 0) )           \
    {                                                       \
        overlap = FALSE;                                    \
    }                                                       \
    else if (owner1First < owner2First)                     \
    {                                                       \
        if (owner2First < owner1Last)                       \
        {                                                   \
            overlap = TRUE;                                 \
        }                                                   \
        else                                                \
        {                                                   \
            overlap = FALSE;                                \
        }                                                   \
    }                                                       \
    else if (owner1First > owner2First)                     \
    {                                                       \
        if (owner1First < owner2Last)                       \
        {                                                   \
            overlap = TRUE;                                 \
        }                                                   \
        else                                                \
        {                                                   \
            overlap = FALSE;                                \
        }                                                   \
    }                                                       \
    else                                                    \
    {                                                       \
        overlap = TRUE;                                     \
    }


#define DETERMINE_SLICE_CHANGES(oldSliceCount,          \
                                newSliceCount,          \
                                oldFirstSlice,          \
                                oldLastSlice,           \
                                newFirstSlice,          \
                                newLastSlice,           \
                                grow,                   \
                                shrink,                 \
                                moving)                 \
    if (newSliceCount < oldSliceCount)                  \
    {                                                   \
        shrink = TRUE;                                  \
        grow   = FALSE;                                 \
    }                                                   \
    else if (newSliceCount > oldSliceCount)             \
    {                                                   \
        shrink = FALSE;                                 \
        grow   = TRUE;                                  \
    }                                                   \
    else                                                \
    {                                                   \
        shrink = FALSE;                                 \
        grow   = FALSE;                                 \
    }                                                   \
                                                        \
    if ( (oldFirstSlice >= 0)                           \
        && (newFirstSlice >= 0)                         \
        && (newFirstSlice != oldFirstSlice)             \
        && (newLastSlice != oldLastSlice) )             \
    {                                                   \
        moving = TRUE;                                  \
    }                                                   \
    else                                                \
    {                                                   \
        moving = FALSE;                                 \
    }


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_trapCodeMapping trapCodeMappingTable[] =
{
    /* Code ID                          Code */
    { FM_TRAPCODE_ICMP_TTL,             FM10000_TRAP_ICMP_TTL},
    { FM_TRAPCODE_IEEE,                 FM10000_TRAP_IEEE_MAC},
    { FM_TRAPCODE_CPU,                  FM10000_TRAP_CPU_MAC},
    { FM_TRAPCODE_FFU,                  FM10000_TRAP_FFU},
    { FM_TRAPCODE_IP_OPTION,            FM10000_TRAP_IP_OPT},
    { FM_TRAPCODE_MTU,                  FM10000_TRAP_MTU},
    { FM_TRAPCODE_IGMP,                 FM10000_TRAP_IGMP},
    { FM_TRAPCODE_TTL,                  FM10000_TRAP_TTL},

    { FM_TRAPCODE_LOG_INGRESS_FFU,      FM10000_MIRROR_FFU_CODE},
    { FM_TRAPCODE_LOG_ARP_REDIRECT,     FM10000_MIRROR_ARP_CODE},

    /* Defines 2 trap codes related to unresolved ARP entry */
    { FM_TRAPCODE_L3_ROUTED_NO_ARP_0,   0xA2},
    { FM_TRAPCODE_L3_ROUTED_NO_ARP_1,   0xA3},

    /* Defines 4 Trap codes with custom usage from TE0 */
    { FM_TRAPCODE_TE_0_BASE,            FM10000_TUNNEL_0_TRAP_CODE_BASE},
    /* Defines 4 Trap codes with custom usage from TE1 */
    { FM_TRAPCODE_TE_1_BASE,            FM10000_TUNNEL_1_TRAP_CODE_BASE},

    /* Defines 16 Trap codes with custom usage from Mirror */
    { FM_TRAPCODE_MIRROR_BASE,          FM10000_MIRROR_CPU_CODE_BASE},

};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** CreateIpOptionsTrigger1
 * \ingroup intSwitch
 *
 * \desc            Creates the first IP Options trigger.
 * 
 *                  The global IP options trap (SYS_CFG_ROUTER.trapIPOptions)
 *                  applies to all frames, including ones that have not been
 *                  routed. This trigger untraps all switched IP frames that
 *                  are trapped because they contain IP options.
 *                  See Bugzero 2296, 2362. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateIpOptionsTrigger1(fm_int sw)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR, "sw=%d\n", sw);

    /**************************************************
     * Create the trigger.
     **************************************************/

    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_IP_OPTIONS,
                               FM10000_TRIGGER_RULE_IP_OPTIONS1,
                               TRUE,
                               "ipOptionsTrigger1");
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /**************************************************
     * Configure trigger condition.
     **************************************************/

    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /* Match on switched IP frames. */
    trigCond.cfg.matchRoutedMask = FM_TRIGGER_SWITCHED_FRAMES;

    /* Match frames trapped due to IP Options being present. */
    trigCond.cfg.HAMask = FM_TRIGGER_HA_TRAP_IP_OPTION;

    /* Match on all received ports. */
    trigCond.cfg.rxPortset = FM_PORT_SET_ALL;

    /**************************************************
     * Configure trigger action.
     **************************************************/

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG;

    /**************************************************
     * Apply trigger configuration.
     **************************************************/

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_IP_OPTIONS,
                                  FM10000_TRIGGER_RULE_IP_OPTIONS1,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_IP_OPTIONS,
                                     FM10000_TRIGGER_RULE_IP_OPTIONS1,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end CreateIpOptionsTrigger1 */




/*****************************************************************************/
/** CreateIpOptionsTrigger2
 * \ingroup intSwitch
 *
 * \desc            Creates the second IP Options trigger.
 * 
 *                  This trigger selectively untraps routed IP frames that are
 *                  trapped due to IP options. When the global IP options trap
 *                  is set, all IP frames are trapped.  This trigger allows
 *                  the user to trap (1) all, (2) none, (3) unicast frames, or 
 *                  (4) multicast frames; See Bugzero 2296, 2362. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       classMask is the frame class mask for the trigger.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateIpOptionsTrigger2(fm_int sw, fm_uint32 classMask)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR, "sw=%d classMask=0x%02x\n", sw, classMask);

    /**************************************************
     * Create the trigger.
     **************************************************/

    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_IP_OPTIONS,
                               FM10000_TRIGGER_RULE_IP_OPTIONS2,
                               TRUE,
                               "ipOptionsTrigger2");
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /**************************************************
     * Configure trigger condition.
     **************************************************/

    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /* Match on routed IP frames. */
    trigCond.cfg.matchRoutedMask = FM_TRIGGER_ROUTED_FRAMES;

    /* Apply to specified class of frames (unicast, multicast, both). */
    trigCond.cfg.matchFrameClassMask = classMask;

    /* Match frames trapped due to IP Options being present. */
    trigCond.cfg.HAMask = FM_TRIGGER_HA_TRAP_IP_OPTION;

    /* Match on all received ports. */
    trigCond.cfg.rxPortset = FM_PORT_SET_ALL;

    /**************************************************
     * Configure trigger action.
     **************************************************/

    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG;

    /**************************************************
     * Apply trigger configuration.
     **************************************************/

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_IP_OPTIONS,
                                  FM10000_TRIGGER_RULE_IP_OPTIONS2,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_IP_OPTIONS,
                                     FM10000_TRIGGER_RULE_IP_OPTIONS2,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end CreateIpOptionsTrigger2 */




/*****************************************************************************/
/** SetIpOptionsDisposition
 * \ingroup intSwitch
 *
 * \desc            Sets the FM_IP_OPTIONS_DISPOSITION attribute
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       disp is the value to be assigned to the attribute.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetIpOptionsDisposition(fm_int sw, fm_int disp)
{
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_uint32           classMask;
    fm_uint32           rv32;
    fm_status           err;
    fm_bool             haveLock;
    fm_bool             trapEnabled;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR, "sw=%d disp=%d\n", sw, disp);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    haveLock = FALSE;

    switch (disp)
    {
        case FM_IP_OPTIONS_FWD:
        case FM_IP_OPTIONS_TRAP:
            classMask = 0;
            break;

        case FM_IP_OPTIONS_TRAP_UCST:
            /* Don't trap multicast frames. */
            classMask = FM_TRIGGER_FRAME_CLASS_MCAST;
            break;

        case FM_IP_OPTIONS_TRAP_MCST:
            /* Don't trap unicast frames. */
            classMask = FM_TRIGGER_FRAME_CLASS_UCAST;
            break;

        default:
            err = FM_ERR_INVALID_VALUE;
            goto ABORT;

    }   /* end switch (disp) */

    /**************************************************
     * We must protect the read-modify-write cycle on
     * SYS_CFG_ROUTER with routingLock rather then
     * regLock because routingLock is used to manage 
     * this register when changing router attributes.
     **************************************************/

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
    haveLock = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_SYS_CFG_ROUTER(), &rv32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    trapEnabled = (disp != FM_IP_OPTIONS_FWD);
    FM_SET_BIT(rv32, FM10000_SYS_CFG_ROUTER, trapIPOptions, trapEnabled);

    err = switchPtr->WriteUINT32(sw, FM10000_SYS_CFG_ROUTER(), rv32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /* Also update the (deprecated) router option. */
    switchPtr->routerTrapIpOptions = trapEnabled;

    /**************************************************
     * If trapping is enabled (not forwarding), we
     * need to configure the IP Options trigger.
     **************************************************/

    if (trapEnabled)
    {
        /**************************************************
         * Get the trigger configuration.
         **************************************************/

        err = fm10000GetTrigger(sw,
                                FM10000_TRIGGER_GROUP_IP_OPTIONS,
                                FM10000_TRIGGER_RULE_IP_OPTIONS2,
                                &trigCond,
                                &trigAction);

        if (err == FM_OK)
        {
            /**************************************************
             * Update trigger condition.
             **************************************************/

            trigCond.cfg.matchFrameClassMask = classMask;

            err = fm10000SetTriggerCondition(sw,
                                             FM10000_TRIGGER_GROUP_IP_OPTIONS,
                                             FM10000_TRIGGER_RULE_IP_OPTIONS2,
                                             &trigCond,
                                             TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
        }
        else if (err == FM_ERR_INVALID_TRIG)
        {
            /**************************************************
             * Create the IP Options trigger set.
             **************************************************/

            err = CreateIpOptionsTrigger1(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            err = CreateIpOptionsTrigger2(sw, classMask);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
        }

    }   /* end if (trapEnabled) */

    /* Cache the attribute value. */
    switchExt->ipOptionsDisposition = disp;

ABORT:
    if (haveLock)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetIpOptionsDisposition */




/*****************************************************************************/
/** SetFrameAgingTime
 * \ingroup intSwitch
 *
 * \desc            Sets the FM_FRAME_AGING_TIME_MSEC attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       value is the frame timeout in milliseconds.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if the parameter value is invalid.
 *
 *****************************************************************************/
static fm_status SetFrameAgingTime(fm_int sw, fm_int value)
{
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_float            freqMHz;
    fm_uint32           timeoutMult;
    fm_uint32           rv;
    fm_status           err;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ATTR, "sw=%d value=%d\n", sw, value);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    if (value < 0)
    {
        err = FM_ERR_INVALID_VALUE;
        goto ABORT;
    }

    err = switchPtr->ReadUINT32(sw, FM10000_FRAME_TIME_OUT(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /* Get the switch clock frequency in MHz. */
    err = fm10000ComputeFHClockFreq(sw, &freqMHz);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    if (freqMHz == 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ATTR, "switch clock frequency is zero!\n");
        err = FM_ERR_ASSERTION_FAILED;
        goto ABORT;
    }

    /* Convert to switch time units / 2048. */
    timeoutMult = (fm_uint32)round(
        ( (fm_float)value * freqMHz * 1000.0 ) / 2048.0 );

    if (timeoutMult >= FM_FIELD_UNSIGNED_MAX(FM10000_FRAME_TIME_OUT, timeoutMult))
    {
        err = FM_ERR_INVALID_VALUE;
        goto ABORT;
    }

    FM_SET_FIELD(rv, FM10000_FRAME_TIME_OUT, timeoutMult, timeoutMult);

    err = switchPtr->WriteUINT32(sw, FM10000_FRAME_TIME_OUT(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    switchExt->frameAgingTime = value;
//      (fm_uint32)round(((fm_float)timeoutMult * 2048.0) / (freqMHz * 1000.0));

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetFrameAgingTime */




/*****************************************************************************/
/** WriteReservedMac
 * \ingroup intSwitch
 *
 * \desc            Writes the specified reserved MAC address configuration
 *                  to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the index of the reserved MAC address.
 * 
 * \param[in]       action is the action to be performed.
 *
 * \param[in]       useTrapPri specifies whether to use the configured
 *                  trap priority.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteReservedMac(fm_int  sw,
                                  fm_int  index,
                                  fm_int  action,
                                  fm_bool useTrapPri)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint32       reservedMacCtrl[FM10000_IEEE_RESERVED_MAC_ACTION_WIDTH];
    fm_int          firstBit;
    fm_int          arrayIndex;
    fm_int          arrayBit;
    fm_uint64       rv64;
    fm_status       err;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /**************************************************
     * Configure the MAC address action.
     **************************************************/

    err = switchPtr->ReadUINT32Mult(sw, 
                                    FM10000_IEEE_RESERVED_MAC_ACTION(0), 
                                    FM10000_IEEE_RESERVED_MAC_ACTION_WIDTH,
                                    reservedMacCtrl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /* There are two bits per reserved MAC entry */
    firstBit   = index * 2;
    arrayIndex = firstBit / BITS_IN_UINT32;
    arrayBit   = firstBit % BITS_IN_UINT32;

    reservedMacCtrl[arrayIndex] &= ~(0x3 << arrayBit);
    reservedMacCtrl[arrayIndex] |= ((action & 3) << arrayBit);

    err = switchPtr->WriteUINT32Mult(sw, 
                                     FM10000_IEEE_RESERVED_MAC_ACTION(0), 
                                     FM10000_IEEE_RESERVED_MAC_ACTION_WIDTH,
                                     reservedMacCtrl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /**************************************************
     * Configure the trap priority.
     **************************************************/

    err = switchPtr->ReadUINT64(sw,
                                FM10000_IEEE_RESERVED_MAC_TRAP_PRIORITY(0),
                                &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    if (useTrapPri)
    {
        rv64 |= FM_LITERAL_U64(1) << index;
    }
    else
    {
        rv64 &= ~(FM_LITERAL_U64(1) << index);
    }

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_IEEE_RESERVED_MAC_TRAP_PRIORITY(0),
                                 rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /**************************************************
     * Cache the configuration.
     **************************************************/

    switchExt->reservedMacAction[index] = action;
    switchExt->reservedMacUsePri[index] = useTrapPri;

ABORT:
    return err;

}   /* end WriteReservedMac */




/*****************************************************************************/
/** SetReservedMacCfg
 * \ingroup intSwitch
 *
 * \desc            Sets the configuration of a reserved MAC address.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the low-order octet of the reserved MAC address
 *                  to be configured.
 * 
 * \param[in]       action is the action to be performed. See ''fm_resMacAction''
 *                  for legal values.
 *
 * \param[in]       useTrapPri is TRUE if switch should use the configured
 *                  trap priority for the frame.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetReservedMacCfg(fm_int  sw,
                                   fm_int  index,
                                   fm_int  action,
                                   fm_bool useTrapPri)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw=%d index=0x%02x action=%d useTrapPri=%s\n",
                 sw,
                 index,
                 action,
                 FM_BOOLSTRING(useTrapPri));

    if ( (index  < 0 || index  >= FM_NUM_RESERVED_MACS) ||
         (action < 0 || action >= FM_RES_MAC_ACTION_MAX) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    TAKE_REG_LOCK(sw);
    err = WriteReservedMac(sw, index, action, useTrapPri);
    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetReservedMacCfg */




/*****************************************************************************/
/** GetReservedMacCfg
 * \ingroup intSwitch
 *
 * \desc            Gets the configuration of a reserved MAC address.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   resMac points to the structure to be filled in with the
 *                  MAC address configuration.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetReservedMacCfg(fm_int  sw, fm_reservedMacCfg * resMac)
{
    fm10000_switch *switchExt;

    if (resMac == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (resMac->index  < 0 || resMac->index  >= FM_NUM_RESERVED_MACS)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switchExt = GET_SWITCH_EXT(sw);

    resMac->action     = switchExt->reservedMacAction[resMac->index];
    resMac->useTrapPri = switchExt->reservedMacUsePri[resMac->index];

    return FM_OK;

}   /* end GetReservedMacCfg */




/*****************************************************************************/
/** SetReservedMacAction
 * \ingroup intSwitch
 *
 * \desc            Sets the action to take for a block of reserved MACs.
 *                  Reserved macs are in the following range
 *                  01:80:C2:00:00:00 - 01:80:C2:00:00:3F
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       first is the suffix (last byte) of the first reserved MAC
 *                  for which the action should be set. 
 * 
 * \param[in]       last is the suffix (last byte) of the last reserved MAC for
 *                  which the action should be set. To set the action for
 *                  only one MAC, last should be equal to first.
 * 
 * \param[in]       action is the action to take when a frame has a
 *                  reserved DMAC between 'first' and 'last'. Possible
 *                  actions are: switch, trap, drop or log.
 * 
 * \param[in]       usePri is TRUE if the switch should use the configured
 *                  trap priority for the frame.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if first or last is not in a
 *                  valid range ( 0x00 - 0x3F )
 *
 *****************************************************************************/
static fm_status SetReservedMacAction(fm_int    sw,
                                      fm_uint32 first,
                                      fm_uint32 last, 
                                      fm_uint32 action,
                                      fm_bool   usePri)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint32       reservedMacCtrl[FM10000_IEEE_RESERVED_MAC_ACTION_WIDTH];
    fm_uint64       reservedMacPri;
    fm_uint32       suffix;
    fm_uint32       firstBit;
    fm_uint32       arrayIndex;
    fm_uint32       arrayBit;
    fm_bool         regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, 
                 "sw = %d, first = %d, last = %d, action = %d, usePri = %d\n",
                 sw, 
                 first, 
                 last, 
                 action,
                 usePri);

    /* Validation */
    if ( (first > FM_RES_MAC_INDEX_MAX) ||
         (last  > FM_RES_MAC_INDEX_MAX) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    if (action >= FM_RES_MAC_ACTION_MAX)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    regLockTaken = FALSE;
    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32Mult(sw, 
                                    FM10000_IEEE_RESERVED_MAC_ACTION(0), 
                                    FM10000_IEEE_RESERVED_MAC_ACTION_WIDTH,
                                    reservedMacCtrl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = switchPtr->ReadUINT64(sw,
                                FM10000_IEEE_RESERVED_MAC_TRAP_PRIORITY(0),
                                &reservedMacPri);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    for (suffix = first; suffix <= last; suffix++)
    {
        /* There are two bits per reserved MAC entry */
        firstBit   = suffix * 2;
        arrayIndex = firstBit / BITS_IN_UINT32;
        arrayBit   = firstBit % BITS_IN_UINT32;

        reservedMacCtrl[arrayIndex] &= ~(0x3 << arrayBit);
        reservedMacCtrl[arrayIndex] |= ((0x3 & action) << arrayBit);
        reservedMacPri &= (FM_LITERAL_U64(1) << suffix);

        switchExt->reservedMacAction[suffix] = action;
        switchExt->reservedMacUsePri[suffix] = usePri;
    }

    err = switchPtr->WriteUINT32Mult(sw, 
                                     FM10000_IEEE_RESERVED_MAC_ACTION(0), 
                                     FM10000_IEEE_RESERVED_MAC_ACTION_WIDTH,
                                     reservedMacCtrl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_IEEE_RESERVED_MAC_TRAP_PRIORITY(0),
                                 reservedMacPri);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

ABORT:
    
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end SetReservedMacAction */




/*****************************************************************************/
/** ValidateL2HashKeyMasks
 * \ingroup intSwitch
 *
 * \desc            Validate the L2 Hashing key masks' configurations.
 *
 * \param[in]       l2HashKey points to the l2 hash key structure
 *                  to validate against.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if the mask is not set properly
 *
 *****************************************************************************/
static fm_status ValidateL2HashKeyMasks(fm_L2HashKey *l2HashKey)
{
    fm_status err = FM_OK;

    if ( l2HashKey->DMACMask > FM_LITERAL_64(0xffffffffffff) ||
         l2HashKey->SMACMask > FM_LITERAL_64(0xffffffffffff) ||
         l2HashKey->etherTypeMask > 0xffff ||
         l2HashKey->vlanPri1Mask > 0xf ||
         l2HashKey->vlanId1Mask > 0xfff ||
         l2HashKey->vlanPri2Mask > 0xf ||
         l2HashKey->vlanId2Mask > 0xfff )
    {
        err = FM_ERR_INVALID_VALUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end ValidateL2HashKeyMasks */




/*****************************************************************************/
/** ValidateL3HashCfgMasks
 * \ingroup intSwitch
 *
 * \desc            Validate the L3 Hashing config masks' configurations.
 *
 * \param[in]       l3HashCfg points to the l3 hash key structure
 *                  to validate against.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if the mask is not set properly
 *
 *****************************************************************************/
static fm_status ValidateL3HashCfgMasks(fm_L3HashConfig *l3HashCfg)
{
    fm_status err = FM_OK;

    if ( l3HashCfg->DIPMask > 0xffff ||
         l3HashCfg->SIPMask > 0xffff ||
         l3HashCfg->L4SrcMask > 0xffff ||
         l3HashCfg->L4DstMask > 0xffff ||
         l3HashCfg->protocolMask > 0xff )
    {
        err = FM_ERR_INVALID_VALUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end ValidateL3HashCfgMasks */




/*****************************************************************************/
/** GetL2Hash
 * \ingroup intSwitch
 *
 * \desc            Get the L2 Hashing key configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       l2HashKey points to the l2 hash key structure
 *                  to fill.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetL2Hash(fm_int        sw,
                           fm_L2HashKey *l2HashKey)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  hashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, l2HashKey = %p\n",
                 sw,
                 (void *) l2HashKey);

    if (l2HashKey == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_L234_HASH_CFG(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    l2HashKey->SMACMask = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseSMAC );

    l2HashKey->DMACMask = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseDMAC );

    l2HashKey->etherTypeMask = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseType );

    l2HashKey->vlanId1Mask = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseVID );

    l2HashKey->vlanPri1Mask = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseVPRI );

    l2HashKey->symmetrizeMAC = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, Symmetric );

    l2HashKey->useL3Hash = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseL34 );

    l2HashKey->useL2ifIP = FM_GET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseL2ifIP );

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end GetL2Hash */




/*****************************************************************************/
/** GetL2HashRot
 * \ingroup intSwitch
 *
 * \desc            Get the L2 Hashing rotation configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       rot refer to the rotation to retreive. Can be either A
 *                  or B.
 *
 * \param[in]       l2HashRot points to the l2 hash rotation structure
 *                  to fill.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetL2HashRot(fm_int        sw,
                              fm_int        rot,
                              fm_L2HashRot *l2HashRot)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  hashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, rot = %d, l2HashRot = %p\n",
                 sw,
                 rot,
                 (void *) l2HashRot);

    if (l2HashRot == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_L234_HASH_CFG(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    if (rot == FM_HASH_ROTATION_A)
    {
        l2HashRot->hashRotation = FM_GET_FIELD( hashCfg, 
                                                FM10000_L234_HASH_CFG, 
                                                RotationA );
    }
    else if (rot == FM_HASH_ROTATION_B)
    {
        l2HashRot->hashRotation = FM_GET_FIELD( hashCfg, 
                                                FM10000_L234_HASH_CFG, 
                                                RotationB );
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end GetL2HashRot */




/*****************************************************************************/
/** GetL3Hash
 * \ingroup intSwitch
 *
 * \desc            Get the L3 Hashing configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       l3HashCfg points to the l3 hash cfg structure
 *                  to fill.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetL3Hash(fm_int           sw,
                           fm_L3HashConfig *l3HashCfg)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  hashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, l3HashCfg = %p\n",
                 sw,
                 (void *) l3HashCfg);

    if (l3HashCfg == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_L34_HASH_CFG(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    l3HashCfg->SIPMask = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseSIP );

    l3HashCfg->DIPMask = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseDIP );

    l3HashCfg->L4SrcMask = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseL4SRC );

    l3HashCfg->L4DstMask = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseL4DST );

    l3HashCfg->protocolMask = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UsePROT );

    l3HashCfg->symmetrizeL3Fields = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, Symmetric );

    l3HashCfg->symmetrizeL4Fields = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, Symmetric );

    l3HashCfg->ECMPRotation = FM_GET_FIELD( hashCfg, FM10000_L34_HASH_CFG, ECMP_Rotation );

    l3HashCfg->protocol1 = FM_GET_FIELD( hashCfg, FM10000_L34_HASH_CFG, PROT1 );
  
    l3HashCfg->protocol2 = FM_GET_FIELD( hashCfg, FM10000_L34_HASH_CFG, PROT2 );

    l3HashCfg->useProtocol1 = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UsePROT1 );

    l3HashCfg->useProtocol2 = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UsePROT2 );

    l3HashCfg->useTCP = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseTCP );

    l3HashCfg->useUDP = FM_GET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseUDP );

    err = switchPtr->ReadUINT32(sw, FM10000_L34_FLOW_HASH_CFG_1(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    l3HashCfg->DSCPMask = FM_GET_FIELD( hashCfg, FM10000_L34_FLOW_HASH_CFG_1, DiffServMask );

    l3HashCfg->ISLUserMask = FM_GET_FIELD( hashCfg, FM10000_L34_FLOW_HASH_CFG_1, UserMask );

    err = switchPtr->ReadUINT32(sw, FM10000_L34_FLOW_HASH_CFG_2(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    l3HashCfg->flowMask = FM_GET_FIELD( hashCfg, FM10000_L34_FLOW_HASH_CFG_2, FlowLabelMask );

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end GetL3Hash */




/*****************************************************************************/
/** SetL2Hash
 * \ingroup intSwitch
 *
 * \desc            Set the L2 Hashing key configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       l2HashKey points to the l2 hash key structure
 *                  to configure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetL2Hash(fm_int        sw,
                           fm_L2HashKey *l2HashKey)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  hashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, l2HashKey = %p\n",
                 sw,
                 (void *) l2HashKey);

    if (l2HashKey == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    if ( (l2HashKey->useL3Hash == 0) &&
         (l2HashKey->useL2ifIP == 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    VALIDATE_VALUE_IS_BOOL(&l2HashKey->symmetrizeMAC);
    VALIDATE_VALUE_IS_BOOL(&l2HashKey->useL3Hash);
    VALIDATE_VALUE_IS_BOOL(&l2HashKey->useL2ifIP);

    err = ValidateL2HashKeyMasks(l2HashKey);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    err = switchPtr->ReadUINT32(sw, FM10000_L234_HASH_CFG(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseSMAC,
                l2HashKey->SMACMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseDMAC,
                l2HashKey->DMACMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseType,
                l2HashKey->etherTypeMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseVID,
                l2HashKey->vlanId1Mask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseVPRI,
                l2HashKey->vlanPri1Mask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, Symmetric,
                l2HashKey->symmetrizeMAC ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseL34,
                l2HashKey->useL3Hash ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L234_HASH_CFG, UseL2ifIP,
                l2HashKey->useL2ifIP ? 1 : 0 );

    err = switchPtr->WriteUINT32(sw, FM10000_L234_HASH_CFG(), hashCfg);

ABORT:

    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetL2Hash */




/*****************************************************************************/
/** SetL2HashRot
 * \ingroup intSwitch
 *
 * \desc            Set the L2 Hashing rotation configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       rot refer to the rotation to configure. Can be either A
 *                  or B.
 *
 * \param[in]       l2HashRot points to the l2 hash rotation structure
 *                  to configure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetL2HashRot(fm_int        sw,
                              fm_int        rot,
                              fm_L2HashRot *l2HashRot)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  hashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, rot = %d, l2HashRot = %p\n",
                 sw,
                 rot,
                 (void *) l2HashRot);

    if (l2HashRot == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    if (l2HashRot->hashRotation >= FM10000_NB_HASH_ROT)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_VALUE);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_L234_HASH_CFG(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    if (rot == FM_HASH_ROTATION_A)
    {
        if ( l2HashRot->hashRotation < 4 )
        {
            FM_SET_FIELD( hashCfg, FM10000_L234_HASH_CFG, RotationA,
                          l2HashRot->hashRotation );
        }
        else
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
        }
    }
    else if (rot == FM_HASH_ROTATION_B)
    {
        if ( l2HashRot->hashRotation < 4 )
        {
            FM_SET_FIELD( hashCfg, FM10000_L234_HASH_CFG, RotationB,
                      l2HashRot->hashRotation );
        }
        else
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
    }

    err = switchPtr->WriteUINT32(sw, FM10000_L234_HASH_CFG(), hashCfg);

ABORT:

    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetL2HashRot */




/*****************************************************************************/
/** SetL3Hash
 * \ingroup intSwitch
 *
 * \desc            Set the L3 Hashing configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       l3HashCfg points to the l3 hash cfg structure
 *                  to configure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetL3Hash(fm_int           sw,
                           fm_L3HashConfig *l3HashCfg)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  hashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, l3HashCfg = %p\n",
                 sw,
                 (void *) l3HashCfg);

    if (l3HashCfg == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    if ( l3HashCfg->DSCPMask > 0xff )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_VALUE);
    }
  
    if ( l3HashCfg->ECMPRotation > 0x03 )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_VALUE);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    VALIDATE_VALUE_IS_BOOL(&l3HashCfg->useTCP);
    VALIDATE_VALUE_IS_BOOL(&l3HashCfg->useUDP);
    VALIDATE_VALUE_IS_BOOL(&l3HashCfg->useProtocol1);
    VALIDATE_VALUE_IS_BOOL(&l3HashCfg->useProtocol2);
    VALIDATE_VALUE_IS_BOOL(&l3HashCfg->symmetrizeL3Fields);

    err = ValidateL3HashCfgMasks(l3HashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    err = switchPtr->ReadUINT32(sw, FM10000_L34_HASH_CFG(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseSIP,
                l3HashCfg->SIPMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseDIP,
                l3HashCfg->DIPMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseL4SRC,
                l3HashCfg->L4SrcMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseTCP,
                l3HashCfg->useTCP ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseUDP,
                l3HashCfg->useUDP ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UseL4DST,
                l3HashCfg->L4DstMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UsePROT,
                l3HashCfg->protocolMask ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, Symmetric,
                l3HashCfg->symmetrizeL3Fields ? 1 : 0 );

    FM_SET_FIELD( hashCfg, FM10000_L34_HASH_CFG, ECMP_Rotation,
                  l3HashCfg->ECMPRotation );

    if (l3HashCfg->protocol1 > 255)
    {
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG,err);
    }
    FM_SET_FIELD( hashCfg, FM10000_L34_HASH_CFG, PROT1,
                  l3HashCfg->protocol1 );

    if (l3HashCfg->protocol2 > 255)
    {
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG,err);
    }
    FM_SET_FIELD( hashCfg, FM10000_L34_HASH_CFG, PROT2,
                  l3HashCfg->protocol2 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UsePROT1,
                l3HashCfg->useProtocol1 ? 1 : 0 );

    FM_SET_BIT( hashCfg, FM10000_L34_HASH_CFG, UsePROT2,
                l3HashCfg->useProtocol2 ? 1 : 0 );

    err = switchPtr->WriteUINT32(sw, FM10000_L34_HASH_CFG(), hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    err = switchPtr->ReadUINT32(sw, FM10000_L34_FLOW_HASH_CFG_1(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    FM_SET_FIELD( hashCfg, FM10000_L34_FLOW_HASH_CFG_1, DiffServMask,
                  l3HashCfg->DSCPMask );

    if (l3HashCfg->ISLUserMask > 0xff)
    {
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG,err);
    }
    FM_SET_FIELD( hashCfg, FM10000_L34_FLOW_HASH_CFG_1, UserMask,
                  l3HashCfg->ISLUserMask );

    err = switchPtr->WriteUINT32(sw, FM10000_L34_FLOW_HASH_CFG_1(), hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    err = switchPtr->ReadUINT32(sw, FM10000_L34_FLOW_HASH_CFG_2(), &hashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    if (l3HashCfg->flowMask > 0xfffff)
    {
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG,err);
    }
    FM_SET_FIELD( hashCfg, FM10000_L34_FLOW_HASH_CFG_2, FlowLabelMask,
                  l3HashCfg->flowMask );

    err = switchPtr->WriteUINT32(sw, FM10000_L34_FLOW_HASH_CFG_2(), hashCfg);

ABORT:

    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetL3Hash */




/*****************************************************************************/
/** GetSliceAllocations
 * \ingroup intSwitch
 *
 * \desc            Get the current FFU slice allocations 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sliceAlloc points to the slice allocations structure
 *                  to be filled with new information.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if sliceAlloc contains invalid
 *                  allocation values.
 *
 *****************************************************************************/
static fm_status GetSliceAllocations(fm_int                  sw,
                                     fm_ffuSliceAllocations *sliceAlloc)
{
    fm_status      err;
    fm10000_switch *switchExt;
    fm_int         routeFirstSlice;
    fm_int         routeLastSlice;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw = %d, sliceAlloc = %p\n",
                 sw,
                 (void *) sliceAlloc);

    if (sliceAlloc == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    switchExt = GET_SWITCH_EXT(sw);

    err = fm10000GetFFUSliceOwnership(sw,
                                     FM_FFU_OWNER_ROUTING,
                                     &routeFirstSlice,
                                     &routeLastSlice);

    if (err != FM_OK)
    {
        if (err != FM_ERR_NO_FFU_RES_FOUND)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
        }
        
        sliceAlloc->ipv4UnicastFirstSlice   = -1;
        sliceAlloc->ipv4UnicastLastSlice    = -1;
        sliceAlloc->ipv4MulticastFirstSlice = -1;
        sliceAlloc->ipv4MulticastLastSlice  = -1;
        sliceAlloc->ipv6UnicastFirstSlice   = -1;
        sliceAlloc->ipv6UnicastLastSlice    = -1;
        sliceAlloc->ipv6MulticastFirstSlice = -1;
        sliceAlloc->ipv6MulticastLastSlice  = -1;
    }
    else
    {
        sliceAlloc->ipv4UnicastFirstSlice   = switchExt->routeStateTable.ipv4UcastFirstTcamSlice;
        sliceAlloc->ipv4UnicastLastSlice    = switchExt->routeStateTable.ipv4UcastLastTcamSlice;
        sliceAlloc->ipv4MulticastFirstSlice = switchExt->routeStateTable.ipv4McastFirstTcamSlice;
        sliceAlloc->ipv4MulticastLastSlice  = switchExt->routeStateTable.ipv4McastLastTcamSlice;
        sliceAlloc->ipv6UnicastFirstSlice   = switchExt->routeStateTable.ipv6UcastFirstTcamSlice;
        sliceAlloc->ipv6UnicastLastSlice    = switchExt->routeStateTable.ipv6UcastLastTcamSlice;
        sliceAlloc->ipv6MulticastFirstSlice = switchExt->routeStateTable.ipv6McastFirstTcamSlice;
        sliceAlloc->ipv6MulticastLastSlice  = switchExt->routeStateTable.ipv6McastLastTcamSlice;
    }

    err = fm10000GetFFUSliceOwnership(sw,
                                     FM_FFU_OWNER_ACL,
                                     &sliceAlloc->aclFirstSlice,
                                     &sliceAlloc->aclLastSlice);

    if (err != FM_OK)
    {
        if (err != FM_ERR_NO_FFU_RES_FOUND)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
        }

        sliceAlloc->aclFirstSlice = -1;
        sliceAlloc->aclLastSlice  = -1;
    }

    sliceAlloc->cVlanFirstSlice       = -1;
    sliceAlloc->cVlanLastSlice        = -1;
    sliceAlloc->bstRoutingFirstSlice  = -1;
    sliceAlloc->bstRoutingLastSlice   = -1;
    
    if (err == FM_ERR_NO_FFU_RES_FOUND)
    {
        err = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end GetSliceAllocations */




/*****************************************************************************/
/** SetSliceAllocations
 * \ingroup intSwitch
 *
 * \desc            Attempts to adjust FFU slice allocations on-the-fly
 *                  without affecting traffic.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sliceAlloc points to the new slice allocations to be
 *                  applied to the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if sliceAlloc contains invalid
 *                  allocation values.
 *
 *****************************************************************************/
static fm_status SetSliceAllocations(fm_int                  sw,
                                     fm_ffuSliceAllocations *sliceAlloc)
{
    fm_status              err;
    fm_int                 numOldRouteSlices;
    fm_int                 numNewRouteSlices;
    fm_int                 numOldIPv4UnicastSlices;
    fm_int                 numNewIPv4UnicastSlices;
    fm_int                 numOldIPv4MulticastSlices;
    fm_int                 numNewIPv4MulticastSlices;
    fm_int                 numOldIPv6UnicastSlices;
    fm_int                 numNewIPv6UnicastSlices;
    fm_int                 numOldIPv6MulticastSlices;
    fm_int                 numNewIPv6MulticastSlices;
    fm_int                 numOldAclSlices;
    fm_int                 numNewAclSlices;
    fm_bool                routesMoving;
    fm_bool                routesShrinking;
    fm_bool                routesGrowing;
    fm_bool                ipv4UnicastRoutesMoving;
    fm_bool                ipv4UnicastRoutesShrinking;
    fm_bool                ipv4UnicastRoutesGrowing;
    fm_bool                ipv4MulticastRoutesMoving;
    fm_bool                ipv4MulticastRoutesShrinking;
    fm_bool                ipv4MulticastRoutesGrowing;
    fm_bool                ipv6UnicastRoutesMoving;
    fm_bool                ipv6UnicastRoutesShrinking;
    fm_bool                ipv6UnicastRoutesGrowing;
    fm_bool                ipv6MulticastRoutesMoving;
    fm_bool                ipv6MulticastRoutesShrinking;
    fm_bool                ipv6MulticastRoutesGrowing;
    fm_bool                aclsMoving;
    fm_bool                aclsShrinking;
    fm_bool                aclsGrowing;
    fm_int                 routeFirstTcamSlice;
    fm_int                 routeLastTcamSlice;
    fm_int                 routeOldFirstSlice;
    fm_int                 routeOldLastSlice;
    fm_ffuSliceAllocations oldSliceAlloc;
    fm_int                 aclSlicesInUse;
    fm_bool                overlap;
    fm_bool                simulated;

    FM_LOG_ENTRY( FM_LOG_CAT_ATTR,
                 "sw = %d, sliceAlloc = %p\n",
                 sw,
                 (void *) sliceAlloc );

    if (sliceAlloc == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
    }

    /* Get old slice boundaries */
    err = fm10000GetFFUSliceOwnership(sw,
                                      FM_FFU_OWNER_ROUTING,
                                      &routeOldFirstSlice,
                                      &routeOldLastSlice);

    if ( (err != FM_ERR_NO_FFU_RES_FOUND) && (err != FM_OK) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
    }
    else if (err != FM_OK)
    {
        routeOldFirstSlice = -1;
        routeOldLastSlice = -1;
    }
    
    err = GetSliceAllocations(sw, &oldSliceAlloc);

    if ( (err != FM_ERR_NO_FFU_RES_FOUND) && (err != FM_OK) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
    }
    
    if (oldSliceAlloc.aclFirstSlice == -1)
    {
        aclSlicesInUse = 0;
    }
    else
    {
        aclSlicesInUse = fm10000GetACLInUseSliceCount(sw);
    }

    routeFirstTcamSlice = 9999;
    routeLastTcamSlice = -1;

    UPDATE_ROUTE_TCAM_SLICE(sliceAlloc->ipv4UnicastFirstSlice, 
                            sliceAlloc->ipv4UnicastLastSlice,
                            routeFirstTcamSlice, 
                            routeLastTcamSlice);

    UPDATE_ROUTE_TCAM_SLICE(sliceAlloc->ipv4MulticastFirstSlice, 
                            sliceAlloc->ipv4MulticastLastSlice,
                            routeFirstTcamSlice, 
                            routeLastTcamSlice);

    UPDATE_ROUTE_TCAM_SLICE(sliceAlloc->ipv6UnicastFirstSlice, 
                            sliceAlloc->ipv6UnicastLastSlice,
                            routeFirstTcamSlice, 
                            routeLastTcamSlice);

    UPDATE_ROUTE_TCAM_SLICE(sliceAlloc->ipv6MulticastFirstSlice, 
                            sliceAlloc->ipv6MulticastLastSlice,
                            routeFirstTcamSlice, 
                            routeLastTcamSlice);

    FM_LOG_DEBUG(FM_LOG_CAT_ATTR,
                 "Slice Allocation Changes Detected:\n"
                 "  routeOldFirstSlice %2d, routeOldLastSlice %2d, "
                 "routeFirstTcamSlice %2d, routeLastTcamSlice %2d\n"
                 "  oldSliceAlloc.ipv4UnicastFirstSlice %2d, "
                 "oldSliceAlloc.ipv4UnicastLastSlice %2d, "
                 "sliceAlloc->ipv4UnicastFirstSlice %2d, "
                 "sliceAlloc->ipv4UnicastLastSlice %2d\n"
                 "  oldSliceAlloc.ipv4MulticastFirstSlice %2d, "
                 "oldSliceAlloc.ipv4MulticastLastSlice %2d, "
                 "sliceAlloc->ipv4MulticastFirstSlice %2d, "
                 "sliceAlloc->ipv4MulticastLastSlice %2d\n"
                 "  oldSliceAlloc.ipv6UnicastFirstSlice %2d, "
                 "oldSliceAlloc.ipv6UnicastLastSlice %2d, "
                 "sliceAlloc->ipv6UnicastFirstSlice %2d, "
                 "sliceAlloc->ipv6UnicastLastSlice %2d\n"
                 "  oldSliceAlloc.ipv6MulticastFirstSlice %2d, "
                 "oldSliceAlloc.ipv6MulticastLastSlice %2d, "
                 "sliceAlloc->ipv6MulticastFirstSlice %2d, "
                 "sliceAlloc->ipv6MulticastLastSlice %2d\n"
                 "  oldSliceAlloc.aclFirstSlice   %2d, "
                 "oldSliceAlloc.aclLastSlice   %2d, "
                 "sliceAlloc->aclFirstSlice   %2d, "
                 "sliceAlloc->aclLastSlice   %2d, "
                 "aclSlicesInUse %d\n",
                 routeOldFirstSlice,
                 routeOldLastSlice,
                 routeFirstTcamSlice,
                 routeLastTcamSlice,
                 oldSliceAlloc.ipv4UnicastFirstSlice,
                 oldSliceAlloc.ipv4UnicastLastSlice,
                 sliceAlloc->ipv4UnicastFirstSlice,
                 sliceAlloc->ipv4UnicastLastSlice,
                 oldSliceAlloc.ipv4MulticastFirstSlice,
                 oldSliceAlloc.ipv4MulticastLastSlice,
                 sliceAlloc->ipv4MulticastFirstSlice,
                 sliceAlloc->ipv4MulticastLastSlice,
                 oldSliceAlloc.ipv6UnicastFirstSlice,
                 oldSliceAlloc.ipv6UnicastLastSlice,
                 sliceAlloc->ipv6UnicastFirstSlice,
                 sliceAlloc->ipv6UnicastLastSlice,
                 oldSliceAlloc.ipv6MulticastFirstSlice,
                 oldSliceAlloc.ipv6MulticastLastSlice,
                 sliceAlloc->ipv6MulticastFirstSlice,
                 sliceAlloc->ipv6MulticastLastSlice,
                 oldSliceAlloc.aclFirstSlice,
                 oldSliceAlloc.aclLastSlice,
                 sliceAlloc->aclFirstSlice,
                 sliceAlloc->aclLastSlice,
                 aclSlicesInUse);

    /* Make sure there is no overlap with the new boundaries */

    /* compare new routes with new ACLs */
    DO_SLICES_OVERLAP(routeFirstTcamSlice,
                      routeLastTcamSlice,
                      sliceAlloc->aclFirstSlice,
                      sliceAlloc->aclLastSlice,
                      overlap);

    if (overlap)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_SLICE);
    }

    /* Determine what is being done with routing slices */
    numOldRouteSlices         = 0;
    numOldIPv4UnicastSlices   = 0;
    numOldIPv4MulticastSlices = 0;
    numOldIPv6UnicastSlices   = 0;
    numOldIPv6MulticastSlices = 0;
    numNewRouteSlices         = 0;
    numNewIPv4UnicastSlices   = 0;
    numNewIPv4MulticastSlices = 0;
    numNewIPv6UnicastSlices   = 0;
    numNewIPv6MulticastSlices = 0;

    if (routeOldFirstSlice >= 0)
    {
        numOldRouteSlices = routeOldLastSlice - routeOldFirstSlice + 1;
    }

    if (oldSliceAlloc.ipv4UnicastFirstSlice >= 0)
    {
        numOldIPv4UnicastSlices = oldSliceAlloc.ipv4UnicastLastSlice
            - oldSliceAlloc.ipv4UnicastFirstSlice + 1;
    }

    if (oldSliceAlloc.ipv4MulticastFirstSlice >= 0)
    {
        numOldIPv4MulticastSlices = oldSliceAlloc.ipv4MulticastLastSlice
            - oldSliceAlloc.ipv4MulticastFirstSlice + 1;
    }

    if (oldSliceAlloc.ipv6UnicastFirstSlice >= 0)
    {
        numOldIPv6UnicastSlices = oldSliceAlloc.ipv6UnicastLastSlice
            - oldSliceAlloc.ipv6UnicastFirstSlice + 1;
    }

    if (oldSliceAlloc.ipv6MulticastFirstSlice >= 0)
    {
        numOldIPv6MulticastSlices = oldSliceAlloc.ipv6MulticastLastSlice
            - oldSliceAlloc.ipv6MulticastFirstSlice + 1;
    }

    if (routeFirstTcamSlice <= routeLastTcamSlice)
    {
        numNewRouteSlices = routeLastTcamSlice - routeFirstTcamSlice + 1;
    }

    if (sliceAlloc->ipv4UnicastFirstSlice >= 0)
    {
        numNewIPv4UnicastSlices = sliceAlloc->ipv4UnicastLastSlice
            - sliceAlloc->ipv4UnicastFirstSlice + 1;
    }

    if (sliceAlloc->ipv4MulticastFirstSlice >= 0)
    {
        numNewIPv4MulticastSlices = sliceAlloc->ipv4MulticastLastSlice
            - sliceAlloc->ipv4MulticastFirstSlice + 1;
    }

    if (sliceAlloc->ipv6UnicastFirstSlice >= 0)
    {
        numNewIPv6UnicastSlices = sliceAlloc->ipv6UnicastLastSlice
            - sliceAlloc->ipv6UnicastFirstSlice + 1;
    }

    if (sliceAlloc->ipv6MulticastFirstSlice >= 0)
    {
        numNewIPv6MulticastSlices = sliceAlloc->ipv6MulticastLastSlice
            - sliceAlloc->ipv6MulticastFirstSlice + 1;
    }

    DETERMINE_SLICE_CHANGES(numOldRouteSlices,
                            numNewRouteSlices,
                            routeOldFirstSlice,
                            routeOldLastSlice,
                            routeFirstTcamSlice,
                            routeLastTcamSlice,
                            routesGrowing,
                            routesShrinking,
                            routesMoving);

    DETERMINE_SLICE_CHANGES(numOldIPv4UnicastSlices,
                            numNewIPv4UnicastSlices,
                            oldSliceAlloc.ipv4UnicastFirstSlice,
                            oldSliceAlloc.ipv4UnicastLastSlice,
                            sliceAlloc->ipv4UnicastFirstSlice,
                            sliceAlloc->ipv4UnicastLastSlice,
                            ipv4UnicastRoutesGrowing,
                            ipv4UnicastRoutesShrinking,
                            ipv4UnicastRoutesMoving);

    DETERMINE_SLICE_CHANGES(numOldIPv4MulticastSlices,
                            numNewIPv4MulticastSlices,
                            oldSliceAlloc.ipv4MulticastFirstSlice,
                            oldSliceAlloc.ipv4MulticastLastSlice,
                            sliceAlloc->ipv4MulticastFirstSlice,
                            sliceAlloc->ipv4MulticastLastSlice,
                            ipv4MulticastRoutesGrowing,
                            ipv4MulticastRoutesShrinking,
                            ipv4MulticastRoutesMoving);

    DETERMINE_SLICE_CHANGES(numOldIPv6UnicastSlices,
                            numNewIPv6UnicastSlices,
                            oldSliceAlloc.ipv6UnicastFirstSlice,
                            oldSliceAlloc.ipv6UnicastLastSlice,
                            sliceAlloc->ipv6UnicastFirstSlice,
                            sliceAlloc->ipv6UnicastLastSlice,
                            ipv6UnicastRoutesGrowing,
                            ipv6UnicastRoutesShrinking,
                            ipv6UnicastRoutesMoving);

    DETERMINE_SLICE_CHANGES(numOldIPv6MulticastSlices,
                            numNewIPv6MulticastSlices,
                            oldSliceAlloc.ipv6MulticastFirstSlice,
                            oldSliceAlloc.ipv6MulticastLastSlice,
                            sliceAlloc->ipv6MulticastFirstSlice,
                            sliceAlloc->ipv6MulticastLastSlice,
                            ipv6MulticastRoutesGrowing,
                            ipv6MulticastRoutesShrinking,
                            ipv6MulticastRoutesMoving);

    /* Determine what is being done with ACL slices */
    if (oldSliceAlloc.aclFirstSlice >= 0)
    {
        numOldAclSlices = oldSliceAlloc.aclLastSlice - oldSliceAlloc.aclFirstSlice + 1;
    }
    else
    {
        numOldAclSlices = 0;
    }

    if (sliceAlloc->aclFirstSlice >= 0)
    {
        numNewAclSlices = sliceAlloc->aclLastSlice
            - sliceAlloc->aclFirstSlice + 1;
    }
    else
    {
        numNewAclSlices = 0;
    }

    DETERMINE_SLICE_CHANGES(numOldAclSlices,
                            numNewAclSlices,
                            oldSliceAlloc.aclFirstSlice,
                            oldSliceAlloc.aclLastSlice,
                            sliceAlloc->aclFirstSlice,
                            sliceAlloc->aclLastSlice,
                            aclsGrowing,
                            aclsShrinking,
                            aclsMoving);

    /* For now, prohibit straight moves for acls -
     * only growing and shrinking allowed. */
    if ( ( aclsMoving && (aclSlicesInUse > 0) ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ATTR, FM_ERR_INVALID_SLICE);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_ATTR,
                 "Slice Changes Inferred from slice allocations:\n"
                 "  routesShrinking %d, routesGrowing %d, routesMoving %d\n"
                 "  ipv4UnicastRoutesShrinking %d, ipv4UnicastRoutesGrowing %d, "
                 "ipv4UnicastRoutesMoving %d\n"
                 "  ipv4MulticastRouteShrinking %d, ipv4MulticastRoutesGrowing %d, "
                 "ipv4MulticastRoutesMoving %d\n"
                 "  ipv6UnicastRoutesShrinking %d, ipv6UnicastRoutesGrowing %d, "
                 "ipv6UnicastRoutesMoving %d\n"
                 "  ipv6MulticastRouteShrinking %d, ipv6MulticastRoutesGrowing %d, "
                 "ipv6MulticastRoutesMoving %d\n"
                 "  aclsShrinking   %d, aclsGrowing   %d, aclsMoving   %d\n",
                 routesShrinking,
                 routesGrowing,
                 routesMoving,
                 ipv4UnicastRoutesShrinking,
                 ipv4UnicastRoutesGrowing,
                 ipv4UnicastRoutesMoving,
                 ipv4MulticastRoutesShrinking,
                 ipv4MulticastRoutesGrowing,
                 ipv4MulticastRoutesMoving,
                 ipv6UnicastRoutesShrinking,
                 ipv6UnicastRoutesGrowing,
                 ipv6UnicastRoutesMoving,
                 ipv6MulticastRoutesShrinking,
                 ipv6MulticastRoutesGrowing,
                 ipv6MulticastRoutesMoving,
                 aclsShrinking,
                 aclsGrowing,
                 aclsMoving);

    /* Simulate all moves - shrink first, then grow.
     * If the simulations are all successful, do it again for real. */
    simulated   = TRUE;

    while (1)
    {
        if (routesShrinking
            || ipv4UnicastRoutesShrinking
            || ipv4MulticastRoutesShrinking
            || ipv6UnicastRoutesShrinking
            || ipv6MulticastRoutesShrinking)
        {
            err = fm10000RoutingProcessFFUPartitionChange(sw,
                                                          sliceAlloc,
                                                          simulated);

            if (err != FM_OK)
            {
                if (!simulated)
                {
                    FM_LOG_FATAL( FM_LOG_CAT_ATTR,
                                 "Routing Partition Change (shrink) failed "
                                 "after successful simulation! err = %d (%s)\n",
                                 err,
                                 fmErrorMsg(err) );
                }

                FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
            }
        }

        if (aclsShrinking)
        {
            err = fm10000AclProcessFFUPartitionChange(sw,
                                                      sliceAlloc,
                                                      simulated);

            if (err != FM_OK)
            {
                if (!simulated)
                {
                    FM_LOG_FATAL( FM_LOG_CAT_ATTR,
                                 "ACL Partition Change (shrink) failed after "
                                 "successful simulation! err = %d (%s)\n",
                                 err,
                                 fmErrorMsg(err) );
                }

                FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
            }
        }

        if (routesGrowing || routesMoving
            || ipv4UnicastRoutesGrowing || ipv4UnicastRoutesMoving
            || ipv4MulticastRoutesGrowing || ipv4MulticastRoutesMoving
            || ipv6UnicastRoutesGrowing || ipv6UnicastRoutesMoving
            || ipv6MulticastRoutesGrowing || ipv6MulticastRoutesMoving)
        {
            err = fm10000RoutingProcessFFUPartitionChange(sw,
                                                          sliceAlloc,
                                                          simulated);

            if (err != FM_OK)
            {
                if (!simulated)
                {
                    FM_LOG_FATAL( FM_LOG_CAT_ATTR,
                                 "Routing Partition Change (grow) failed after "
                                 "successful simulation! err = %d (%s)\n",
                                 err,
                                 fmErrorMsg(err) );
                }

                FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
            }
        }

        if (aclsGrowing || aclsMoving)
        {
            err = fm10000AclProcessFFUPartitionChange(sw,
                                                      sliceAlloc,
                                                      simulated);

            if (err != FM_OK)
            {
                if (!simulated)
                {
                    FM_LOG_FATAL( FM_LOG_CAT_ATTR,
                                 "ACL Partition Change (grow) failed after "
                                 "successful simulation! err = %d (%s)\n",
                                 err,
                                 fmErrorMsg(err) );
                }

                FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
            }
        }

        if (simulated)
        {
            /* Simulations were successful. */
            simulated   = FALSE;

            /* Release old slice ownership rules. */
            if (routeOldFirstSlice >= 0)
            {
                err = fm10000SetFFUSliceOwnership(sw,
                                                  FM_FFU_OWNER_NONE,
                                                  routeOldFirstSlice,
                                                  routeOldLastSlice);

                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
                }
            }

            if (oldSliceAlloc.aclFirstSlice >= 0)
            {
                err = fm10000SetFFUSliceOwnership(sw,
                                                  FM_FFU_OWNER_NONE,
                                                  oldSliceAlloc.aclFirstSlice,
                                                  oldSliceAlloc.aclLastSlice);

                /* Restore FFU Slice Ownership */
                if (err != FM_OK)
                {
                    if (routeOldFirstSlice >= 0)
                    {
                        fm10000SetFFUSliceOwnership(sw,
                                                    FM_FFU_OWNER_ROUTING,
                                                    routeOldFirstSlice,
                                                    routeOldLastSlice);
                    }
                    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
                }
            }

            /* Try to take new slice ownerships. */
            if (routeFirstTcamSlice >= 0)
            {
                err = fm10000SetFFUSliceOwnership(sw,
                                                  FM_FFU_OWNER_ROUTING,
                                                  routeFirstTcamSlice,
                                                  routeLastTcamSlice);

                /* Restore FFU Slice Ownership */
                if (err != FM_OK)
                {
                    if (routeOldFirstSlice >= 0)
                    {
                        fm10000SetFFUSliceOwnership(sw,
                                                    FM_FFU_OWNER_ROUTING,
                                                    routeOldFirstSlice,
                                                    routeOldLastSlice);
                    }

                    if (oldSliceAlloc.aclFirstSlice >= 0)
                    {
                        fm10000SetFFUSliceOwnership(sw,
                                                    FM_FFU_OWNER_ACL,
                                                    oldSliceAlloc.aclFirstSlice,
                                                    oldSliceAlloc.aclLastSlice);
                    }
                    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
                }
            }

            if (sliceAlloc->aclFirstSlice >= 0)
            {
                err = fm10000SetFFUSliceOwnership(sw,
                                                  FM_FFU_OWNER_ACL,
                                                  sliceAlloc->aclFirstSlice,
                                                  sliceAlloc->aclLastSlice);

                /* Restore FFU Slice Ownership */
                if (err != FM_OK)
                {
                    if (routeFirstTcamSlice >= 0)
                    {
                        fm10000SetFFUSliceOwnership(sw,
                                                    FM_FFU_OWNER_NONE,
                                                    routeFirstTcamSlice,
                                                    routeLastTcamSlice);
                    }

                    if (routeOldFirstSlice >= 0)
                    {
                        fm10000SetFFUSliceOwnership(sw,
                                                    FM_FFU_OWNER_ROUTING,
                                                    routeOldFirstSlice,
                                                    routeOldLastSlice);
                    }

                    if (oldSliceAlloc.aclFirstSlice >= 0)
                    {
                        fm10000SetFFUSliceOwnership(sw,
                                                    FM_FFU_OWNER_ACL,
                                                    oldSliceAlloc.aclFirstSlice,
                                                    oldSliceAlloc.aclLastSlice);
                    }
                    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);
                }
            }
        }
        else
        {
            /* Just did it for real, all done */
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end SetSliceAllocations */




/*****************************************************************************/
/** UpdatePerPortAttribute
 * \ingroup intSwitch
 *
 * \desc            Applies a port attribute to all ports. 
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       attr is the port attribute to configre
 *
 * \param[in]       value points to the storage that contains the port
 *                  attribute value to set
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdatePerPortAttribute(fm_int sw, fm_int attr, void *value)
{

    fm_switch *     switchPtr;
    fm_status       err;
    fm_int          i;
    fm_int          j;
    fm_int          cpi;
    fm_int          port;
    fm_int          lagList[FM_MAX_NUM_LAGS];
    fm_int          nLags;
    fm_int          lagPortList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int          nLagPorts;
    fm_int          portSkipList[FM10000_NUM_PORTS];
    fm_int          nSkipPorts = 0;
    fm_bool         portIsInLag = FALSE;
                
    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw=%d attr=%d value=%p\n", 
                 sw, attr, value);

    switchPtr = GET_SWITCH_PTR(sw);

    switch (attr)
    {
        case FM_PORT_BCAST_FLOODING:
        case FM_PORT_MCAST_FLOODING:
        case FM_PORT_UCAST_FLOODING:
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
    }

    /* Get a list of LAG logical ports. */
    err = fmGetLAGList(sw, &nLags, lagList, FM_MAX_NUM_LAGS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    /* Set the port attribute for each lag. This must be done since the
     * attributes are on a per-lag basis. */
    for (i = 0; i < nLags; i++)
    {
        err = fm10000SetPortAttribute(sw, 
                                      lagList[i], 
                                      FM_PORT_ACTIVE_MAC, 
                                      FM_PORT_LANE_NA, 
                                      attr,
                                      value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

        /* Retrieve the LAG member ports to add them to the skip list. */
        err = fmGetLAGPortList(sw, 
                               lagList[i], 
                               &nLagPorts, 
                               lagPortList, 
                               FM_MAX_NUM_LAG_MEMBERS);

        for (j = 0; j < nLagPorts; j++)
        {
            portSkipList[nSkipPorts++] = lagPortList[j];
        }

    }   /* end for (i = 0; i < nLags; i++) */

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        portIsInLag = FALSE;

        for (j = 0; j < nSkipPorts; j++)
        {
            if (port == portSkipList[j])
            {
                portIsInLag = TRUE;
                break;
            }
        }

        if (portIsInLag)
        {
            /* Skip this port as it has been done when setting the port
             * attribute for the lag port. */
            continue;
        }

        err = fm10000SetPortAttribute(sw, 
                                      port, 
                                      FM_PORT_ACTIVE_MAC, 
                                      FM_PORT_LANE_NA, 
                                      attr,
                                      value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    }   /* end for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++) */
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end UpdatePerPortAttribute */





/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000GetSwitchAttribute
 * \ingroup intSwitch
 *
 * \desc            Retrieve a switch attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch attribute to retrieve
 *                  (see 'Switch Attributes').
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function is to place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fm10000GetSwitchAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm10000_switch *   switchExt;
    fm_bool            regLockTaken;
    fm_uint32          reg32;
    fm_uint64          reg64;
    fm_mtuEntry *      mtuEntry;
    fm_vlanEtherType * vlanEtherType;
    fm_mplsEtherType * mplsEtherType;
    fm_customTag     * customTag;
    fm_parserDiCfg   * parserDiCfg;
    fm_macaddr *       macAddr;
    fm_vlanMapEntry *  mapEntry;
    fm_sglortVsiMap *  sglortVsiMap;
    fm_fm10000TeSGlort teSglort;
    fm_vsiData *       vsiData;
    fm_fm10000TeTepCfg teTepCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw=%d attr=%d value=%p\n",
                 sw, attr, value);

    switchPtr    = GET_SWITCH_PTR(sw);
    switchExt    = GET_SWITCH_EXT(sw);
    regLockTaken = FALSE;
    err          = FM_OK;

    switch (attr)
    {
        case FM_VLAN_TYPE:
            *( (fm_int *) value ) = FM_VLAN_MODE_8021Q;
            break;

        case FM_SPANNING_TREE_MODE:
            *( (fm_stpMode *) value ) = switchPtr->stpMode;
            break;

        case FM_VLAN_LEARNING_MODE:
            *( (fm_vlanLearningMode *) value ) = switchPtr->vlanLearningMode;
            break;

        case FM_VLAN_LEARNING_SHARED_VLAN:
            *( (fm_int *) value ) = switchPtr->sharedLearningVlan;
            break;

        case FM_SWITCH_PARSER_VLAN_ETYPES:
            vlanEtherType = (fm_vlanEtherType *) value;

            if ( (vlanEtherType->index < 0) ||
                 (vlanEtherType->index > FM_MAX_PARSER_VLAN_ETYPE_INDEX ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            vlanEtherType->etherType =
                switchExt->parserVlanEtherTypes[vlanEtherType->index];
            break;

        case FM_SWITCH_MODIFY_VLAN_ETYPES:
            vlanEtherType = (fm_vlanEtherType *) value;

            if ( (vlanEtherType->index < 0) ||
                 (vlanEtherType->index > FM_MAX_MODIFY_VLAN_ETYPE_INDEX ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            vlanEtherType->etherType =
                switchExt->modVlanEtherTypes[vlanEtherType->index];
            break;

        case FM_CPU_MAC:
            macAddr = (fm_macaddr *) value;

            err = switchPtr->ReadUINT64(sw, FM10000_CPU_MAC(0), &reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            *macAddr = FM_GET_FIELD64(reg64, FM10000_CPU_MAC, MacAddr);
            break;

        case FM_MTU_LIST:
            mtuEntry = (fm_mtuEntry *) value;

            if (mtuEntry->index >= FM10000_MTU_TABLE_ENTRIES)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                break;
            }

            err = switchPtr->ReadUINT32(sw,
                                        FM10000_MTU_TABLE(mtuEntry->index),
                                        &reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            mtuEntry->mtu = FM_GET_FIELD(reg32, FM10000_MTU_TABLE, mtu);
            break;

        case FM_LAG_MODE:
            *( (fm_lagMode *) value ) = switchPtr->lagInfoTable.lagMode;
            break;

        case FM_LAG_PRUNING:
            /* User value is reverse of saved info */
            *( (fm_bool *) value ) = !switchPtr->lagInfoTable.pruningDisabled;
            break;

        case FM_IP_OPTIONS_DISPOSITION:
            *( (fm_uint32 *) value) = switchExt->ipOptionsDisposition;
            break;

        case FM_FFU_SLICE_ALLOCATIONS:
            err = GetSliceAllocations(sw, (fm_ffuSliceAllocations *) value);
            break;

        case FM_L2_HASH_KEY:
            err = GetL2Hash(sw, (fm_L2HashKey *) value);
            break;

        case FM_L2_HASH_ROT_A:
            err = GetL2HashRot(sw, FM_HASH_ROTATION_A, (fm_L2HashRot *) value);
            break;

        case FM_L2_HASH_ROT_B:
            err = GetL2HashRot(sw, FM_HASH_ROTATION_B, (fm_L2HashRot *) value);
            break;

        case FM_L3_HASH_CONFIG:
            err = GetL3Hash(sw, (fm_L3HashConfig *) value);
            break;

        case FM_LBG_MODE:
            *( (fm_LBGMode *) value ) = switchPtr->lbgInfo.mode;
            break;

        case FM_REDIRECT_CPU_TRAFFIC:
            *( (fm_int *) value ) = switchPtr->cpuPort;
            break;

        case FM_BCAST_FLOODING:
            *( (fm_int *) value ) = switchExt->bcastFlooding;
            break;

        case FM_MCAST_FLOODING:
            *( (fm_int *) value ) = switchExt->mcastFlooding;
            break;

        case FM_UCAST_FLOODING:
            *( (fm_int *) value ) = switchExt->ucastFlooding;
            break;

        case FM_FRAME_AGING_TIME_MSEC:
            *( (fm_int *) value ) = switchExt->frameAgingTime;
            break;

        case FM_SWITCH_TRAP_CODE:
            {
                fm_trapType type = ((fm_trapCodeMapping *) value )->type;
                fm_int *    codePtr = &((fm_trapCodeMapping *) value )->code;
        
                err = fm10000GetSwitchTrapCode(sw, type, codePtr);
            }
            break;

        case FM_SWITCH_TRAP_TYPE:
            {
                fm_int       code = ((fm_trapCodeMapping *) value )->code;
                fm_trapType *typePtr = &((fm_trapCodeMapping *) value )->type;
        
                err = fm10000GetSwitchTrapType(sw, code, typePtr);
            }
            break;

        case FM_TRAP_IEEE_LACP:
            *( (fm_bool *) value ) = switchExt->trapLacp;
            break;

        case FM_TRAP_IEEE_BPDU:
            *( (fm_bool *) value ) = switchExt->trapBpdu;
            break;

        case FM_TRAP_IEEE_GARP:
            *( (fm_bool *) value ) = switchExt->trapGarp;
            break;

        case FM_TRAP_IEEE_8021X:
            *( (fm_bool *) value ) = switchExt->trap8021X;
            break;

        case FM_TRAP_MTU_VIOLATIONS:
            *( (fm_bool *) value ) = switchExt->trapMtuViolations;
            break;

        case FM_TRAP_PLUS_LOG:
            *( (fm_bool *) value ) = switchExt->trapPlusLog;
            break;

        case FM_DROP_PAUSE:
            *( (fm_bool *) value ) = switchExt->dropPause;
            break;

        case FM_DROP_INVALID_SMAC:
            *( (fm_bool *) value ) = switchExt->dropInvalidSmac;
            break;
 
        case FM_SWITCH_PARSER_CUSTOM_TAG:
            customTag = (fm_customTag *) value;

            if ( (customTag->index < 0) ||
                 (customTag->index > FM_MAX_CUSTOM_TAG_INDEX ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            customTag->customTagConfig = switchExt->customTag[customTag->index];
            break;

        case FM_SWITCH_PARSER_DI_CFG:
            parserDiCfg = (fm_parserDiCfg *) value;

            if ( (parserDiCfg->index < 0) ||
                 (parserDiCfg->index > FM_MAX_PARSER_DI_CFG_INDEX ))
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            parserDiCfg->parserDiCfgFields =
                switchExt->parserDiCfg[parserDiCfg->index];
            break;

        case FM_SWITCH_MPLS_ETHER_TYPES:
            mplsEtherType = (fm_mplsEtherType *) value;

            if ( (mplsEtherType->index < 0) ||
                 (mplsEtherType->index >= FM_NUM_MPLS_ETHER_TYPES ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            mplsEtherType->etherType =
                switchExt->mplsEtherType[mplsEtherType->index];
            break;

        case FM_SWITCH_PAUSE_SMAC:
            macAddr = (fm_macaddr *) value;

            err = switchPtr->ReadUINT64(sw, FM10000_MOD_PAUSE_SMAC(0), &reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            *macAddr = FM_GET_FIELD64(reg64, FM10000_MOD_PAUSE_SMAC, SMAC);
            break;

        case FM_SWITCH_RESERVED_MAC_CFG:
            err = GetReservedMacCfg(sw, (fm_reservedMacCfg *) value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            break;

        case FM_SWITCH_RESERVED_MAC_TRAP_PRI:
            err = switchPtr->ReadUINT32(sw,
                                        FM10000_IEEE_RESERVED_MAC_CFG(),
                                        &reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            *( (fm_uint32 *) value ) =
                FM_GET_FIELD(reg32, FM10000_IEEE_RESERVED_MAC_CFG, trapPri);
            break;

        case FM_SWITCH_VXLAN_MAC:
        case FM_SWITCH_VXLAN_DIP:
            /* VN attributes. */
            err = FM_ERR_UNSUPPORTED;
            break;

        case FM_SWITCH_TUNNEL_DEST_UDP_PORT:
            *( (fm_uint32 *) value ) = switchExt->vnVxlanUdpPort;
            break;

        case FM_SWITCH_GENEVE_TUNNEL_DEST_UDP_PORT:
            *( (fm_uint32 *) value ) = switchExt->vnGeneveUdpPort;
            break;

        case FM_SWITCH_VLAN1_MAP:
            mapEntry = ( (fm_vlanMapEntry *) value);

            if (mapEntry->entry >= FM10000_MOD_VLAN_TAG_VID1_MAP_ENTRIES)
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            err = switchPtr->ReadUINT64(sw,
                                        FM10000_MOD_VLAN_TAG_VID1_MAP(mapEntry->entry, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            mapEntry->value = FM_GET_FIELD64(reg64, FM10000_MOD_VLAN_TAG_VID1_MAP, VID);
            break;

        case FM_SWITCH_VLAN2_MAP:
            mapEntry = ( (fm_vlanMapEntry *) value);

            if (mapEntry->entry >= FM10000_MOD_VID2_MAP_ENTRIES)
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            err = switchPtr->ReadUINT64(sw, 
                                        FM10000_MOD_VID2_MAP(mapEntry->entry, 0), 
                                        &reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            mapEntry->value = FM_GET_FIELD64(reg64, FM10000_MOD_VID2_MAP, VID);
            break;

        case FM_SWITCH_SGLORT_VSI_MAP:
            sglortVsiMap = ( (fm_sglortVsiMap *) value );

            if (sglortVsiMap == NULL)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            FM_CLEAR(teSglort);

            err = fm10000GetTeSGlort(sw,
                                     sglortVsiMap->te,
                                     sglortVsiMap->index,
                                     &teSglort,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            sglortVsiMap->sglortValue = teSglort.glortValue;
            sglortVsiMap->sglortMask  = teSglort.glortMask;
            sglortVsiMap->vsiStart    = teSglort.vsiStart;
            sglortVsiMap->vsiLen      = teSglort.vsiLength;
            sglortVsiMap->vsiOffset   = teSglort.vsiOffset;
            break;

        case FM_SWITCH_VSI_DATA:
            vsiData = ( (fm_vsiData *) value );

            if (vsiData == NULL)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            FM_CLEAR(teTepCfg);

            err = fm10000GetTeDefaultTep(sw,
                                         vsiData->te,
                                         vsiData->vsi,
                                         &teTepCfg,
                                         TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            vsiData->encapSip = teTepCfg.srcIpAddr;
            vsiData->encapVni = teTepCfg.vni;
            break;

        case FM_SWITCH_PEP_TIMESTAMP_TRAP_ID:
            *( (fm_int *) value) = switchExt->pepToPepTimestampTrapcodeId;
            break;

        case FM_SWITCH_RX_PKT_DROP_UNKNOWN_PORT:
            *( (fm_bool *)value ) = switchExt->dropPacketUnknownPort;
            break;

        case FM_SWITCH_ETH_TIMESTAMP_OWNER:
            *( (fm_int *) value) = switchExt->ethTimestampsOwnerPort;
            break;

        case FM_SWITCH_TX_TIMESTAMP_MODE:
            *( (fm_bool *)value ) = switchExt->txTimestampMode;
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end fm10000GetSwitchAttribute */




/*****************************************************************************/
/** fm10000SetBoolSwitchAttribute
 * \ingroup intSwitch
 *
 * \desc            Sets a Boolean switch attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch attribute to set (see 'Switch Attributes').
 *
 * \param[in]       value is the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT value is invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fm10000SetBoolSwitchAttribute(fm_int sw, fm_int attr, fm_bool value)
{
    return fm10000SetSwitchAttribute(sw, attr, &value);
}




/*****************************************************************************/
/** fm10000SetIntSwitchAttribute
 * \ingroup intSwitch
 *
 * \desc            Sets an integer switch attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch attribute to set (see 'Switch Attributes').
 *
 * \param[in]       value is the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT value is invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fm10000SetIntSwitchAttribute(fm_int sw, fm_int attr, fm_int value)
{
    return fm10000SetSwitchAttribute(sw, attr, &value);
}




/*****************************************************************************/
/** fm10000SetSwitchAttribute
 * \ingroup intSwitch
 *
 * \desc            Set a switch attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the switch attribute to set (see 'Switch Attributes').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if the attribute does not exist
 *                  or is unsupported.
 * \return          FM_ERR_READONLY_ATTRIB if the attribute is read-only.
 *
 *****************************************************************************/
fm_status fm10000SetSwitchAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_status           err;
    fm_bool             regLockTaken;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_uint32           reg32;
    fm_uint64           reg64;
    fm_uint32           tmpUint32;
    fm_int              intValue;
    fm_int              portValue;
    fm_bool             tmpBool;
    fm_vlanLearningMode mode;
    fm_mtuEntry *       mtuEntry;
    fm_vlanEtherType *  vlanEtherType;
    fm_mplsEtherType *  mplsEtherType;
    fm_stpMode          stpMode;
    fm_customTag *      customTag;
    fm_parserDiCfg *    parserDiCfg;
    fm_macaddr *        macAddr;
    fm_vlanMapEntry *   mapEntry;
    fm10000_vlanEntry * vlanEntry;
    fm_sglortVsiMap *   sglortVsiMap;
    fm_fm10000TeSGlort  teSglort;
    fm_vsiData *        vsiData;
    fm_fm10000TeTepCfg  teTepCfg;
    fm_uint32           fieldMask;
    fm_int              pepToPepTimestampTrapcodeId;
    fm_portType         portType;
    fm_bool             addToMcastGroup;
    fm_int              previousValue;

    FM_LOG_ENTRY(FM_LOG_CAT_ATTR,
                 "sw=%d attr=%d value=%p\n",
                 sw, attr, value);

    switchPtr    = GET_SWITCH_PTR(sw);
    switchExt    = GET_SWITCH_EXT(sw);
    err          = FM_ERR_UNSUPPORTED;
    regLockTaken = FALSE;

    switch (attr)
    {
        case FM_VLAN_TYPE:
            if (*( (fm_int *) value ) == FM_VLAN_MODE_8021Q)
            {
                err = FM_OK;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_SPANNING_TREE_MODE:
            stpMode = *((fm_stpMode *) value);

            switch (stpMode)
            {
                case FM_SPANNING_TREE_MULTIPLE:
                    err = fm10000EnableMultipleSpanningTreeMode(sw);
                    break;

                case FM_SPANNING_TREE_SHARED:
                    err = fm10000EnableSharedSpanningTreeMode(sw);
                    break;

                case FM_SPANNING_TREE_PER_VLAN:
                    err = FM_ERR_UNSUPPORTED;
                    break;
            }

            if (err == FM_OK)
            {
                switchPtr->stpMode = stpMode;
            }
            break;

        case FM_VLAN_LEARNING_MODE: 
            mode = *( (fm_vlanLearningMode *) value );

            /* Only do something if the mode is different */
            if (mode != switchPtr->vlanLearningMode)
            {
                fm_vlanLearningMode oldMode;

                oldMode = switchPtr->vlanLearningMode;
                switchPtr->vlanLearningMode = mode;

                err = fm10000ConfigureVlanLearningMode(sw, mode);
                if (err != FM_OK)
                {
                    switchPtr->vlanLearningMode = oldMode;
                    fm10000ConfigureVlanLearningMode(sw, oldMode);
                }
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

                err = fmDeleteAllAddressesInternal(sw);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }
            err = FM_OK;
            break;

        case FM_VLAN_LEARNING_SHARED_VLAN:
            switchPtr->sharedLearningVlan = *( (fm_int *) value );

            /* Reconfigure the learning mode for the new vlan to take effect. */
            if (switchPtr->vlanLearningMode == FM_VLAN_LEARNING_MODE_SHARED)
            {
                err = fm10000ConfigureVlanLearningMode(sw,
                                                       switchPtr->vlanLearningMode);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }
            else
            {
                err = FM_OK;
            }
            break;

        case FM_SWITCH_PARSER_VLAN_ETYPES:
            vlanEtherType = (fm_vlanEtherType *) value;

            if ( (vlanEtherType->index < 0) ||
                 (vlanEtherType->index > FM_MAX_PARSER_VLAN_ETYPE_INDEX ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            reg32 = 0;
            FM_SET_FIELD(reg32,
                         FM10000_PARSER_VLAN_TAG,
                         Tag,
                         vlanEtherType->etherType);

            err = switchPtr->WriteUINT32(sw,
                                         FM10000_PARSER_VLAN_TAG(vlanEtherType->index),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            switchExt->parserVlanEtherTypes[vlanEtherType->index] = vlanEtherType->etherType;
            break;

        case FM_SWITCH_MODIFY_VLAN_ETYPES:
            vlanEtherType = (fm_vlanEtherType *) value;

            if ( (vlanEtherType->index < 0) ||
                 (vlanEtherType->index > FM_MAX_MODIFY_VLAN_ETYPE_INDEX ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            reg64 = 0;
            FM_SET_FIELD64(reg64,
                           FM10000_MOD_VLAN_ETYPE,
                           TagType,
                           vlanEtherType->etherType);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_MOD_VLAN_ETYPE(vlanEtherType->index, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            switchExt->modVlanEtherTypes[vlanEtherType->index] = vlanEtherType->etherType;
            break;

        case FM_CPU_MAC:
            macAddr = (fm_macaddr *) value;

            reg64 = 0;
            FM_SET_FIELD64(reg64, FM10000_CPU_MAC, MacAddr, *macAddr);
            err = switchPtr->WriteUINT64(sw, FM10000_CPU_MAC(0), reg64);
            break;

        case FM_MTU_LIST:
            mtuEntry = (fm_mtuEntry *) value;

            if (mtuEntry->index >= FM10000_MTU_TABLE_ENTRIES)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                break;
            }

            if (mtuEntry->mtu > DEFAULT_MAX_MTU_BYTES)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                break;
            }

            reg32 = 0;
            FM_SET_FIELD(reg32, FM10000_MTU_TABLE, mtu, mtuEntry->mtu);
            err = switchPtr->WriteUINT32(sw,
                                         FM10000_MTU_TABLE(mtuEntry->index),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_LAG_MODE:
            switch ( *( (fm_lagMode *) value ) )
            {
                case FM_MODE_DYNAMIC:
                case FM_MODE_STATIC:
                    switchPtr->lagInfoTable.lagMode = *( (fm_lagMode *) value );
                    /* Unconditionally trap LACP frames */
                    tmpBool = TRUE;
                    err = fm10000SetSwitchAttribute(sw, FM_TRAP_IEEE_LACP, &tmpBool);
                    break;
                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    break;
            }
            break;

        case FM_LAG_PRUNING:
            /* User input is reverse of the flag, 
             * since we want pruning enabled with zero value */
            VALIDATE_VALUE_IS_BOOL(value);
            switchPtr->lagInfoTable.pruningDisabled = !*( (fm_bool *)value );
            err = FM_OK;
            break;

        case FM_IP_OPTIONS_DISPOSITION:
            err = SetIpOptionsDisposition( sw, *((fm_int *) value) );
            break;

        case FM_FFU_SLICE_ALLOCATIONS:
            err = SetSliceAllocations(sw, (fm_ffuSliceAllocations *) value);
            break;

        case FM_L2_HASH_KEY:       
            err = SetL2Hash(sw, (fm_L2HashKey *) value);
            break;

        case FM_L2_HASH_ROT_A:
            err = SetL2HashRot(sw, FM_HASH_ROTATION_A, (fm_L2HashRot *) value);
            break;

        case FM_L2_HASH_ROT_B:
            err = SetL2HashRot(sw, FM_HASH_ROTATION_B, (fm_L2HashRot *) value);
            break;

        case FM_L3_HASH_CONFIG:
            err = SetL3Hash(sw, (fm_L3HashConfig *) value);
            break;

        case FM_LBG_MODE:
            switch ( *( (fm_lbgMode *) value ) )
            {
                case FM_LBG_MODE_REDIRECT:
                case FM_LBG_MODE_MAPPED:
                    switchPtr->lbgInfo.mode = *( (fm_lbgMode *) value );
                    err = FM_OK;
                    break;
                case FM_LBG_MODE_NPLUS1:
                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    break;
            }
            break;

        case FM_REDIRECT_CPU_TRAFFIC:
            intValue = *( (fm_int *) value);
            
            if (intValue < 0)
            {
                err = FM_ERR_INVALID_PORT;
            }
            else
            {
                err = fm10000RedirectCpuTrafficToPort(sw, intValue);
            }
            break;

        case FM_BCAST_FLOODING:
            intValue = *( (fm_int *) value);
            previousValue = switchExt->bcastFlooding;

            switch (intValue)
            {
                case FM_BCAST_DISCARD:
                    portValue = FM_PORT_BCAST_DISCARD;
                    addToMcastGroup = FALSE;
                    break;

                case FM_BCAST_FWD:
                    /* Simulate FWD_EXCPU for port purpose in order
                     * to disable all triggers */
                    portValue = FM_PORT_BCAST_FWD_EXCPU;
                    addToMcastGroup = TRUE;
                    break;

                case FM_BCAST_FWD_EXCPU:
                    portValue = FM_PORT_BCAST_FWD_EXCPU;
                    addToMcastGroup = FALSE;
                    break;

                case FM_BCAST_FLOODING_PER_PORT:
                    if (previousValue == FM_BCAST_FWD)
                    {
                        portValue = FM_PORT_BCAST_FWD;
                    }
                    addToMcastGroup = FALSE;
                    break;

                default:
                    err = FM_ERR_UNSUPPORTED;
                    goto ABORT;
            }

            err = fmSetMgmtPepXcastModes(sw, FM_PORT_BCAST, addToMcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            if ( (intValue != FM_BCAST_FLOODING_PER_PORT) ||
                 (previousValue == FM_BCAST_FWD) )
            {
                err = UpdatePerPortAttribute(sw, 
                                             FM_PORT_BCAST_FLOODING,
                                             &portValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            switchExt->bcastFlooding = intValue;
            err = FM_OK;
            break;

        case FM_MCAST_FLOODING:
            intValue = *( (fm_int *) value);
            previousValue = switchExt->mcastFlooding;

            switch (intValue)
            {
                case FM_MCAST_DISCARD:
                    portValue = FM_PORT_MCAST_DISCARD;
                    addToMcastGroup = FALSE;
                    break;

                case FM_MCAST_FWD:
                    /* Simulate FWD_EXCPU for port purpose in order
                     * to disable all triggers */
                    portValue = FM_PORT_MCAST_FWD_EXCPU;
                    addToMcastGroup = TRUE;
                    break;

                case FM_MCAST_FWD_EXCPU:
                    portValue = FM_PORT_MCAST_FWD_EXCPU;
                    addToMcastGroup = FALSE;
                    break;

                case FM_MCAST_FLOODING_PER_PORT:
                    if (previousValue == FM_MCAST_FWD)
                    {
                        portValue = FM_PORT_MCAST_FWD;
                    }
                    addToMcastGroup = FALSE;
                    break;

                default:
                    err = FM_ERR_UNSUPPORTED;
                    goto ABORT;
            }

            err = fmSetMgmtPepXcastModes(sw, FM_PORT_MCAST, addToMcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            if ( (intValue != FM_MCAST_FLOODING_PER_PORT) ||
                 (previousValue == FM_MCAST_FWD) )
            {
                err = UpdatePerPortAttribute(sw, 
                                             FM_PORT_MCAST_FLOODING,
                                             &portValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            switchExt->mcastFlooding = intValue;
            err = FM_OK;
            break;

        case FM_UCAST_FLOODING:
            intValue = *( (fm_int *) value);
            previousValue = switchExt->ucastFlooding;

            switch (intValue)
            {
                case FM_UCAST_DISCARD:
                    portValue = FM_PORT_UCAST_DISCARD;
                    addToMcastGroup = FALSE;
                    break;

                case FM_UCAST_FWD:
                    /* Simulate FWD_EXCPU for port purpose in order
                     * to disable all triggers */
                    portValue = FM_PORT_UCAST_FWD_EXCPU;
                    addToMcastGroup = TRUE;
                    break;

                case FM_UCAST_FWD_EXCPU:
                    portValue = FM_PORT_UCAST_FWD_EXCPU;
                    addToMcastGroup = FALSE;
                    break;

                case FM_UCAST_FLOODING_PER_PORT:
                    if (previousValue == FM_UCAST_FWD)
                    {
                        portValue = FM_PORT_UCAST_FWD;
                    }
                    addToMcastGroup = FALSE;
                    break;

                default:
                    err = FM_ERR_UNSUPPORTED;
                    goto ABORT;
            }

            err = fmSetMgmtPepXcastModes(sw, FM_PORT_FLOOD, addToMcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            if ( (intValue != FM_UCAST_FLOODING_PER_PORT) ||
                 (previousValue == FM_UCAST_FWD) )
            {
                err = UpdatePerPortAttribute(sw, 
                                             FM_PORT_UCAST_FLOODING,
                                             &portValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            switchExt->ucastFlooding = intValue;
            err = FM_OK;
            break;

        case FM_FRAME_AGING_TIME_MSEC:
            intValue = *( (fm_int *) value );
            err = SetFrameAgingTime(sw, intValue);
            break;

        case FM_TRAP_IEEE_LACP:
            VALIDATE_VALUE_IS_BOOL(value);
            switchExt->trapLacp = *( (fm_bool *) value);

            tmpUint32 = (switchExt->trapLacp) ? 
                FM_RES_MAC_ACTION_TRAP : 
                FM_RES_MAC_ACTION_SWITCH;

            err = SetReservedMacAction(sw, 
                                       FM_RES_MAC_INDEX_LACP,
                                       FM_RES_MAC_INDEX_LACP,
                                       tmpUint32,
                                       TRUE);
            break;

        case FM_TRAP_IEEE_BPDU:
            VALIDATE_VALUE_IS_BOOL(value);
            switchExt->trapBpdu = *( (fm_bool *) value);

            tmpUint32 = (switchExt->trapBpdu) ? 
                FM_RES_MAC_ACTION_TRAP : 
                FM_RES_MAC_ACTION_SWITCH;

            err = SetReservedMacAction(sw, 
                                       FM_RES_MAC_INDEX_BPDU,
                                       FM_RES_MAC_INDEX_BPDU,
                                       tmpUint32,
                                       TRUE);
            break;

        case FM_TRAP_IEEE_GARP:
            VALIDATE_VALUE_IS_BOOL(value);
            switchExt->trapGarp = *( (fm_bool *) value);

            tmpUint32 = (switchExt->trapGarp) ? 
                FM_RES_MAC_ACTION_TRAP : 
                FM_RES_MAC_ACTION_SWITCH;

            err = SetReservedMacAction(sw, 
                                       FM_RES_MAC_INDEX_GARP_MIN,
                                       FM_RES_MAC_INDEX_GARP_MAX,
                                       tmpUint32,
                                       TRUE);
            break;

        case FM_TRAP_IEEE_8021X:
            VALIDATE_VALUE_IS_BOOL(value);
            switchExt->trap8021X = *( (fm_bool *) value);

            tmpUint32 = (switchExt->trap8021X) ? 
                FM_RES_MAC_ACTION_TRAP : 
                FM_RES_MAC_ACTION_SWITCH;

            err = SetReservedMacAction(sw, 
                                       FM_RES_MAC_INDEX_802_1X,
                                       FM_RES_MAC_INDEX_802_1X,
                                       tmpUint32,
                                       TRUE);
            break;

        case FM_TRAP_MTU_VIOLATIONS:
            VALIDATE_VALUE_IS_BOOL(value);
            tmpBool = *( (fm_bool *) value);
            
            err = switchPtr->ReadUINT32(sw,
                                        FM10000_SYS_CFG_1(),
                                        &reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            FM_SET_BIT(reg32,
                       FM10000_SYS_CFG_1,
                       trapMTUViolations,
                       (tmpBool ? 1 : 0));

            err = switchPtr->WriteUINT32(sw,
                                         FM10000_SYS_CFG_1(),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            switchExt->trapMtuViolations = tmpBool;
            break;

        case FM_TRAP_PLUS_LOG:
            VALIDATE_VALUE_IS_BOOL(value);
            tmpBool = *( (fm_bool *) value);
            
            err = switchPtr->ReadUINT32(sw,
                                        FM10000_SYS_CFG_1(),
                                        &reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            FM_SET_BIT(reg32,
                       FM10000_SYS_CFG_1,
                       enableTrapPlusLog,
                       (tmpBool ? 1 : 0));

            err = switchPtr->WriteUINT32(sw,
                                         FM10000_SYS_CFG_1(),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            switchExt->trapPlusLog = tmpBool;
            break;

        case FM_DROP_PAUSE:
            VALIDATE_VALUE_IS_BOOL(value);
            tmpBool = (*( (fm_bool *) value) != 0);
            
            err = switchPtr->ReadUINT32(sw,
                                        FM10000_SYS_CFG_1(),
                                        &reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            FM_SET_BIT(reg32, FM10000_SYS_CFG_1, dropPause, tmpBool);
            FM_SET_BIT(reg32, FM10000_SYS_CFG_1, dropMacCtrlEthertype, tmpBool);

            err = switchPtr->WriteUINT32(sw,
                                         FM10000_SYS_CFG_1(),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            tmpUint32 =
                (tmpBool) ?
                FM_RES_MAC_ACTION_DROP :
                FM_RES_MAC_ACTION_SWITCH;

            err = SetReservedMacAction(sw,
                                       FM_RES_MAC_INDEX_PAUSE,
                                       FM_RES_MAC_INDEX_PAUSE,
                                       tmpUint32,
                                       FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            switchExt->dropPause = tmpBool;
            break;

        case FM_DROP_INVALID_SMAC:
            VALIDATE_VALUE_IS_BOOL(value);
            tmpBool = *( (fm_bool *) value);
            
            err = switchPtr->ReadUINT32(sw,
                                        FM10000_SYS_CFG_1(),
                                        &reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            FM_SET_BIT(reg32,
                       FM10000_SYS_CFG_1,
                       dropInvalidSMAC,
                       (tmpBool ? 1 : 0));

            err = switchPtr->WriteUINT32(sw,
                                         FM10000_SYS_CFG_1(),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            switchExt->dropInvalidSmac = tmpBool;
            break;

        case FM_SWITCH_PARSER_CUSTOM_TAG:
            customTag = (fm_customTag *) value;

            if ( (customTag->index < 0) ||
                 (customTag->index > FM_MAX_CUSTOM_TAG_INDEX ) ||
                 (customTag->customTagConfig.length > FM_MAX_CUSTOM_TAG_LENGTH) ||
                 (customTag->customTagConfig.length < FM_MIN_CUSTOM_TAG_LENGTH))
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            VALIDATE_VALUE_IS_BOOL(&customTag->customTagConfig.captureSelect);

            reg32 = 0;
            FM_SET_FIELD(reg32,
                         FM10000_PARSER_CUSTOM_TAG,
                         Tag,
                         customTag->customTagConfig.etherType);

            FM_SET_FIELD(reg32,
                         FM10000_PARSER_CUSTOM_TAG,
                         Capture,
                         customTag->customTagConfig.capture);

            FM_SET_BIT(reg32,
                       FM10000_PARSER_CUSTOM_TAG,
                       CaptureSelect,
                       (customTag->customTagConfig.captureSelect ? 1 : 0));

            FM_SET_FIELD(reg32,
                         FM10000_PARSER_CUSTOM_TAG,
                         Length,
                         customTag->customTagConfig.length);

            err = switchPtr->WriteUINT32(
                             sw,
                             FM10000_PARSER_CUSTOM_TAG(customTag->index),
                             reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            switchExt->customTag[customTag->index] = customTag->customTagConfig;
            break;

        case FM_SWITCH_PARSER_DI_CFG:
            parserDiCfg = (fm_parserDiCfg *) value;

            if ( (parserDiCfg->index < 0) ||
                 (parserDiCfg->index > FM_MAX_PARSER_DI_CFG_INDEX )) 
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }
            /* Profile 0 can't match on l4 fields since this is the default
             * profile */
            else if ( (parserDiCfg->index == 0) &&
                      (parserDiCfg->parserDiCfgFields.l4Compare ||
                       parserDiCfg->parserDiCfgFields.protocol) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            VALIDATE_VALUE_IS_BOOL(&parserDiCfg->parserDiCfgFields.enable);
            VALIDATE_VALUE_IS_BOOL(&parserDiCfg->parserDiCfgFields.captureTcpFlags);
            VALIDATE_VALUE_IS_BOOL(&parserDiCfg->parserDiCfgFields.l4Compare);

            reg64 = 0;
            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_DI_CFG,
                           Prot,
                           parserDiCfg->parserDiCfgFields.protocol);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_DI_CFG,
                           L4Port,
                           parserDiCfg->parserDiCfgFields.l4Port);

            FM_SET_FIELD64(reg64, 
                           FM10000_PARSER_DI_CFG,
                           WordOffset,
                           parserDiCfg->parserDiCfgFields.wordOffset);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_DI_CFG,
                         CaptureTCPFlags,
                         (parserDiCfg->parserDiCfgFields.captureTcpFlags ? 1 : 0));

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_DI_CFG,
                         Enable,
                         (parserDiCfg->parserDiCfgFields.enable ? 1 : 0));

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_DI_CFG,
                         L4Compare,
                         (parserDiCfg->parserDiCfgFields.l4Compare ? 1 : 0));

            err = switchPtr->WriteUINT64(
                             sw,
                             FM10000_PARSER_DI_CFG(parserDiCfg->index, 0),
                             reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            switchExt->parserDiCfg[parserDiCfg->index] = parserDiCfg->parserDiCfgFields;
            break;

        case FM_SWITCH_MPLS_ETHER_TYPES:
            mplsEtherType = (fm_mplsEtherType *) value;

            err = fm10000SetMplsEtherType(sw,
                                          mplsEtherType->index,
                                          mplsEtherType->etherType);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            break;

        case FM_SWITCH_PAUSE_SMAC:
            macAddr = (fm_macaddr *) value;

            reg64 = 0;
            FM_SET_FIELD64(reg64, FM10000_MOD_PAUSE_SMAC, SMAC, *macAddr);
            err = switchPtr->WriteUINT64(sw, FM10000_MOD_PAUSE_SMAC(0), reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_RESERVED_MAC_CFG:
            {
                fm_reservedMacCfg * resMac;

                resMac = (fm_reservedMacCfg *) value;

                if (resMac == NULL)
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    goto ABORT;
                }

                VALIDATE_VALUE_IS_BOOL(&resMac->useTrapPri);

                switch (resMac->action)
                {
                    case FM_RES_MAC_ACTION_SWITCH:
                    case FM_RES_MAC_ACTION_TRAP:
                    case FM_RES_MAC_ACTION_DROP:
                    case FM_RES_MAC_ACTION_LOG:
                        break;
                    default:
                        err = FM_ERR_INVALID_ARGUMENT;
                        goto ABORT;
                }

                err = SetReservedMacCfg(sw,
                                        resMac->index,
                                        resMac->action,
                                        resMac->useTrapPri);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

                /* Provide a degree of backward compatibility by setting
                 * the FM_TRAP_IEEE_xxxx switch attribute if one exists. */
                tmpBool = (resMac->action == FM_RES_MAC_ACTION_TRAP);

                switch (resMac->index)
                {
                    case FM_RES_MAC_INDEX_BPDU:
                        switchExt->trapBpdu = tmpBool;
                        break;

                    case FM_RES_MAC_INDEX_LACP:
                        switchExt->trapLacp = tmpBool;
                        break;

                    case FM_RES_MAC_INDEX_802_1X:
                        switchExt->trap8021X = tmpBool;
                        break;

                    case FM_RES_MAC_INDEX_GARP_MIN:
                    case FM_RES_MAC_INDEX_GARP_MAX:
                        switchExt->trapGarp = tmpBool;
                        break;

                    default:
                        break;
                }
            }
            break;

        case FM_SWITCH_RESERVED_MAC_TRAP_PRI:
            tmpUint32 = *( (fm_uint32 *) value);

            if (tmpUint32 >= 32)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            reg32 = 0;
            FM_SET_FIELD(reg32,
                         FM10000_IEEE_RESERVED_MAC_CFG,
                         trapPri,
                         tmpUint32);
            err = switchPtr->WriteUINT32(sw,
                                         FM10000_IEEE_RESERVED_MAC_CFG(),
                                         reg32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_VXLAN_MAC:
        case FM_SWITCH_VXLAN_DIP:
            /* VN attributes. */
            err = FM_ERR_UNSUPPORTED;
            break;

        case FM_SWITCH_TUNNEL_DEST_UDP_PORT:
            switchExt->vnVxlanUdpPort = *( (fm_uint32 *) value );

            err = fm10000UpdateTunnelUdpPort(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_GENEVE_TUNNEL_DEST_UDP_PORT:
            switchExt->vnGeneveUdpPort = *( (fm_uint32 *) value );

            err = fm10000UpdateTunnelUdpPort(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_TRAP_CODE:
        case FM_SWITCH_TRAP_TYPE:
            err = FM_ERR_READONLY_ATTRIB;
            break;

        case FM_SWITCH_VLAN1_MAP:
            mapEntry = ( (fm_vlanMapEntry *) value);

            if ( (mapEntry->entry >= FM10000_MOD_VLAN_TAG_VID1_MAP_ENTRIES) ||
                 (mapEntry->value >= FM10000_MOD_VLAN_TAG_VID1_MAP_ENTRIES) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            vlanEntry = GET_VLAN_EXT(sw, mapEntry->entry);

            err = switchPtr->ReadUINT64(sw,
                                        FM10000_MOD_VLAN_TAG_VID1_MAP(mapEntry->entry, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            FM_SET_FIELD64(reg64, FM10000_MOD_VLAN_TAG_VID1_MAP, VID, mapEntry->value);

            vlanEntry->egressVid = mapEntry->value;

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_MOD_VLAN_TAG_VID1_MAP(mapEntry->entry, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_VLAN2_MAP:
            mapEntry = ( (fm_vlanMapEntry *) value);

            if ((mapEntry->entry >= FM10000_MOD_VID2_MAP_ENTRIES) ||
                (mapEntry->value >= FM10000_MOD_VID2_MAP_ENTRIES))
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            err = switchPtr->ReadUINT64(sw,
                                        FM10000_MOD_VID2_MAP(mapEntry->entry, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

            FM_SET_FIELD64(reg64, FM10000_MOD_VID2_MAP, VID, mapEntry->value);

            err = switchPtr->WriteUINT64(sw, 
                                         FM10000_MOD_VID2_MAP(mapEntry->entry, 0), 
                                         reg64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_SGLORT_VSI_MAP:
            sglortVsiMap = ( (fm_sglortVsiMap *) value );

            if (sglortVsiMap == NULL)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            if ( !((sglortVsiMap->te >= 0) && (sglortVsiMap->te <= 1)) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            if ( !((sglortVsiMap->vsiLen >= 0) && (sglortVsiMap->vsiLen <= 31)) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            if ( !((sglortVsiMap->vsiOffset >= 0) && (sglortVsiMap->vsiOffset <= 255)) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            if ( !((sglortVsiMap->vsiStart >= 0) && (sglortVsiMap->vsiStart <= 31)) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            FM_CLEAR(teSglort);

            teSglort.glortValue = sglortVsiMap->sglortValue;
            teSglort.glortMask  = sglortVsiMap->sglortMask;
            teSglort.vsiStart   = sglortVsiMap->vsiStart;
            teSglort.vsiLength  = sglortVsiMap->vsiLen;
            teSglort.vsiOffset  = sglortVsiMap->vsiOffset;

            err = fm10000SetTeSGlort(sw,
                                     sglortVsiMap->te,
                                     sglortVsiMap->index,
                                     &teSglort,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_VSI_DATA:
            vsiData = ( (fm_vsiData *) value );

            if (vsiData == NULL)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            if ( !((vsiData->te >= 0) && (vsiData->te <= 1)) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            if ( !((vsiData->vsi >= 0) && (vsiData->vsi <= 255)) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }

            FM_CLEAR(teTepCfg);

            fieldMask = 0;

            if (vsiData->dataMask & FM_VSI_DATA_ENCAP_SIP)
            {
                fieldMask |= FM10000_TE_DEFAULT_TEP_SIP;

                teTepCfg.srcIpAddr = vsiData->encapSip;
            }

            if (vsiData->dataMask & FM_VSI_DATA_ENCAP_VNI)
            {
                fieldMask |= FM10000_TE_DEFAULT_TEP_VNI;

                teTepCfg.vni = vsiData->encapVni;
            }

            err = fm10000SetTeDefaultTep(sw,
                                         vsiData->te,
                                         vsiData->vsi,
                                         &teTepCfg,
                                         fieldMask,
                                         TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            break;

        case FM_SWITCH_PEP_TIMESTAMP_TRAP_ID:
            pepToPepTimestampTrapcodeId = *( (fm_int *) value);

            if ( ( pepToPepTimestampTrapcodeId < 1 ) ||
                 ( pepToPepTimestampTrapcodeId >= FM10000_MIRROR_NUM_TRAPCODE_ID ) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
            }
            else
            {
                err = FM_OK;
            }
            switchExt->pepToPepTimestampTrapcodeId = pepToPepTimestampTrapcodeId;
            break;

        case FM_SWITCH_RX_PKT_DROP_UNKNOWN_PORT:
            VALIDATE_VALUE_IS_BOOL(value);
            switchExt->dropPacketUnknownPort = *( (fm_bool *)value );
            err = FM_OK;
            break;

        case FM_SWITCH_ETH_TIMESTAMP_OWNER:
            /* Announce Tx Timestamp Mode when Ethernet Timestamp owner 
             * changes */
            if (switchExt->ethTimestampsOwnerPort != *( (fm_int *) value) )
            { 
                if (*( (fm_int *) value) != -1)
                {
                    err = fm10000GetLogicalPortAttribute(sw, 
                                                         *( (fm_int *) value), 
                                                         FM_LPORT_TYPE, 
                                                         (void *)&portType);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

                    if (portType != FM_PORT_TYPE_VIRTUAL)
                    {
                        err = FM_ERR_INVALID_PORT;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);
                    }
                }

                switchExt->ethTimestampsOwnerPort = *( (fm_int *) value);
                err = fmAnnounceTxTimestampMode(sw, switchExt->txTimestampMode);
            }
            else
            {
                err = FM_OK;
            }
            break;

        case FM_SWITCH_TX_TIMESTAMP_MODE:
            /* Announce Tx Timestamp Mode only when it changes */
            if (switchExt->txTimestampMode != *( (fm_bool *)value ) )
            {
                switchExt->txTimestampMode = *( (fm_bool *)value );
                err = fmAnnounceTxTimestampMode(sw, switchExt->txTimestampMode);
            }
            else
            {
                err = FM_OK;
            }
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */
    
ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end fm10000SetSwitchAttribute */




/*****************************************************************************/
/** fm10000SetMplsEtherType
 * \ingroup intSwitch
 *
 * \desc            Sets an MPLS Ethernet type value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index is index of the MPLS tag.
 *
 * \param[in]       etherType is the Ethernet type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT value is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMplsEtherType(fm_int sw, fm_int index, fm_uint16 etherType)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint32       reg32;
    fm_status       err;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    if ( index < 0 || index >= FM_NUM_MPLS_ETHER_TYPES )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_PARSER_MPLS_TAG(), &reg32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    if (index == 0)
    {
        FM_SET_FIELD(reg32, FM10000_PARSER_MPLS_TAG, Tag1, etherType);
    }
    else
    {
        FM_SET_FIELD(reg32, FM10000_PARSER_MPLS_TAG, Tag2, etherType);
    }

    err = switchPtr->WriteUINT32(sw, FM10000_PARSER_MPLS_TAG(), reg32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ATTR, err);

    switchExt->mplsEtherType[index] = etherType;

ABORT:
    DROP_REG_LOCK(sw);
    return err;

}   /* end fm10000SetMplsEtherType */




/*****************************************************************************/
/** fm10000GetSwitchTrapCode
 * \ingroup intSwitch
 *
 * \desc            Retrieves a trap code for a given trap type
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       type is the trap code id from the ''fm_trapType'' enum
 *
 * \param[out]      code points to caller-allocated storage where the trap
 *                  code should be stored
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRAP_CODE if the specified id does not
 *                  exist for this familly
 *
 *****************************************************************************/
fm_status fm10000GetSwitchTrapCode(fm_int sw, fm_trapType type, fm_int *code)
{
    fm_status   err = FM_OK;
    fm_uint     i;
    fm_bool     idFound = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d, type=%d, code=%p\n",
                 sw, 
                 type,
                 (void *)code);

    for (i = 0; 
         i < ( sizeof(trapCodeMappingTable) / sizeof(fm_trapCodeMapping) ); 
         i++ )
    {
        if (type == trapCodeMappingTable[i].type)
        {
            idFound = TRUE;
            *code = trapCodeMappingTable[i].code;
            break;
        }
    }

    if (!idFound)
    {
        err = FM_ERR_INVALID_TRAP_CODE;
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    
}   /* end fm10000GetSwitchTrapCode */




/*****************************************************************************/
/** fm10000GetSwitchTrapType
 * \ingroup intSwitch
 *
 * \desc            Retrieves a trap type for a given trap code
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       code is the trap code to convert
 *
 * \param[out]      type points to caller-allocated storage where the trap
 *                  type should be stored
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRAP_CODE if the specified id does not
 *                  exist for this familly
 *
 *****************************************************************************/
fm_status fm10000GetSwitchTrapType(fm_int sw, fm_int code, fm_trapType *type)
{
    fm_status   err = FM_OK;
    fm_uint      i;
    fm_bool     idFound = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d, code=%d, type=%p\n",
                 sw, 
                 code,
                 (void *) type);

    for (i = 0; 
         i < ( sizeof(trapCodeMappingTable) / sizeof(fm_trapCodeMapping) ); 
         i++ )
    {
        if (code == trapCodeMappingTable[i].code)
        {
            idFound = TRUE;
            *type = trapCodeMappingTable[i].type;
            break;
        }
    }

    if (!idFound)
    {
        err = FM_ERR_INVALID_TRAP_CODE;
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    
}   /* end fm10000GetSwitchTrapType */




/*****************************************************************************/
/** fm10000InitHashing
 * \ingroup intSwitch
 * 
 * \desc            Initialize the hashing to be inline with default values
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitHashing(fm_int sw)
{
    fm_status       err;
    fm_L2HashKey    l2HashKey;
    fm_L2HashRot    l2HashRot;
    fm_L3HashConfig l3HashCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

     /* Initialize the L2 hashing profile to match the documentation */
    l2HashKey.SMACMask = FM_LITERAL_64(0xffffffffffff);
    l2HashKey.DMACMask = FM_LITERAL_64(0xffffffffffff);
    l2HashKey.etherTypeMask = 0xffff;
    l2HashKey.vlanId1Mask = 0xfff;
    l2HashKey.vlanPri1Mask = 0xf;
    l2HashKey.vlanId2Mask = 0xfff;
    l2HashKey.vlanPri2Mask = 0xf;
    l2HashKey.symmetrizeMAC = FALSE;
    l2HashKey.useL3Hash = TRUE;
    l2HashKey.useL2ifIP = TRUE;

    err = fm10000SetSwitchAttribute(sw, FM_L2_HASH_KEY, &l2HashKey);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    l2HashRot.hashRotation = 0;

    err = fm10000SetSwitchAttribute(sw, FM_L2_HASH_ROT_A, &l2HashRot);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    l2HashRot.hashRotation = 1;

    err = fm10000SetSwitchAttribute(sw, FM_L2_HASH_ROT_B, &l2HashRot);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize the L3 hashing profile to match the documentation */
    l3HashCfg.SIPMask = 0xffff;
    l3HashCfg.DIPMask = 0xffff;
    l3HashCfg.L4SrcMask = 0xffff;
    l3HashCfg.L4DstMask = 0xffff;
    l3HashCfg.DSCPMask = 0xff;
    l3HashCfg.ISLUserMask = 0;
    l3HashCfg.protocolMask = 0xff;
    l3HashCfg.flowMask = 0xfffff;
    l3HashCfg.symmetrizeL3Fields = FALSE;
    l3HashCfg.ECMPRotation = 0;
    l3HashCfg.protocol1 = 1;
    l3HashCfg.protocol2 = 1;
    l3HashCfg.useProtocol1 = FALSE;
    l3HashCfg.useProtocol2 = FALSE;
    l3HashCfg.useTCP = TRUE;
    l3HashCfg.useUDP = TRUE;

    err = fm10000SetSwitchAttribute(sw, FM_L3_HASH_CONFIG, &l3HashCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitHashing */




/*****************************************************************************/
/** fm10000InitSwitchAttributes
 * \ingroup intSwitch
 * 
 * \desc            Sets various switch attributes to their default values.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitSwitchAttributes(fm_int sw)
{
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_status           err;
    fm_int              index;
    fm_customTagConfig  customTagConfig;
    fm_parserDiCfgFields parserDiCfgFields;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /**********************************************************************
     * Set the default values for the VLAN Ether Type in API cache. 
     * Default value for all vlan tag ether types is 0x8100.
     **********************************************************************/
    for (index = 0 ; index < FM10000_PARSER_VLAN_TAG_ENTRIES ; index++)
    {
        switchExt->parserVlanEtherTypes[index] = 0x8100;
    }

    for (index = 0 ; index < FM10000_MOD_VLAN_ETYPE_ENTRIES ; index++)
    {
        switchExt->modVlanEtherTypes[index] = 0x8100;
    }

    /**************************************************************
     * Initialize PARSER_MPLS_TAG attribute.
     **************************************************************/
    switchExt->mplsEtherType[0] = DEFAULT_MPLS_TAG_1;
    switchExt->mplsEtherType[1] = DEFAULT_MPLS_TAG_2;

    /*****************************************************
     * Set the default values for the Custom tags in cache
     *****************************************************/
    FM_CLEAR(customTagConfig);
    customTagConfig.length = 1;

    for (index = 0 ; index < FM10000_PARSER_CUSTOM_TAG_ENTRIES ; index++)
    {
        switchExt->customTag[index] = customTagConfig;
    }

    /******************************************************************
     * Set the default values for the DI parsing configuration in cache
     ******************************************************************/
    FM_CLEAR(parserDiCfgFields);
    parserDiCfgFields.wordOffset = 0xFFFFFFFF;

    for (index = 0; index < FM10000_PARSER_DI_CFG_ENTRIES ; index++)
    {
        switchExt->parserDiCfg[index] = parserDiCfgFields;
    }

    /***************************************************
     * Initialize trapping attributes
     **************************************************/
    err = SetReservedMacAction(sw,
                               FM_RES_MAC_INDEX_MIN,
                               FM_RES_MAC_INDEX_MAX,
                               FM_RES_MAC_ACTION_TRAP,
                               FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetBoolSwitchAttribute(sw, FM_TRAP_IEEE_BPDU, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetBoolSwitchAttribute(sw, FM_TRAP_IEEE_LACP, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetBoolSwitchAttribute(sw, FM_TRAP_IEEE_8021X, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetBoolSwitchAttribute(sw, FM_TRAP_IEEE_GARP, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetBoolSwitchAttribute(sw, FM_DROP_PAUSE, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    switchExt->trapMtuViolations    = TRUE;
    switchExt->trapPlusLog          = TRUE;
    switchExt->dropInvalidSmac      = TRUE;

    switchExt->bcastFlooding        = FM_BCAST_FWD_EXCPU;
    switchExt->mcastFlooding        = FM_MCAST_FWD_EXCPU;
    switchExt->ucastFlooding        = FM_UCAST_FWD_EXCPU;

    switchExt->ipOptionsDisposition = FM_IP_OPTIONS_FWD;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fm10000InitSwitchAttributes */
