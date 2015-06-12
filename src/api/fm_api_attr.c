/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_attr.c
 * Creation Date:   2005
 * Description:     Functions for manipulating high level attributes of a switch
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
/* Hardware constants (SYS_CFG_1) */

#define SC1_BROADCAST_DISABLE  0x8000
#define SC1_FLOOD_CTRL_MCAST   0x4000
#define SC1_FLOOD_CTRL_UCAST   0x2000
#define SC1_DROP_PAUSE         0x0400
#define SC1_REMAP_ET_SP15      0x0200
#define SC1_REMAP_CPU_SP15     0x0100
#define SC1_REMAP_IEEE_SP15    0x0080
#define SC1_BROADCAST_CTRL     0x0040
#define SC1_TRAP_8021X         0x0020
#define SC1_TRAP_IGMPV3        0x0010
#define SC1_TRAP_GARP          0x0008
#define SC1_TRAP_BPDU          0x0004
#define SC1_TRAP_LACP          0x0002
#define SC1_TRAP_OTHER         0x0001


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
/** fmGetSwitchAttribute
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
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
fm_status fmGetSwitchAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ATTR, "sw=%d attr=%d value=%p\n",
                     sw, attr, value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetSwitchAttribute, sw, attr, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ATTR, err);

}   /* end fmGetSwitchAttribute */




/*****************************************************************************/
/** fmSetSwitchAttribute
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
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
 * \return          FM_ERR_INVALID_ATTRIB if attr is not recognized.
 * \return          FM_ERR_READONLY_ATTRIB if the attribute is read-only.
 *
 *****************************************************************************/
fm_status fmSetSwitchAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm_bool                 switchLocked = FALSE;
    fm_ffuSliceAllocations *sliceAllocs;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ATTR,
                     "sw=%d attr=%d value=%p\n",
                     sw, attr, value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    switch (attr)
    {
        case FM_FFU_SLICE_ALLOCATIONS:
            sliceAllocs = (fm_ffuSliceAllocations *) value;

            if (   ( (sliceAllocs->ipv4UnicastFirstSlice < 0) &&
                     (sliceAllocs->ipv4UnicastLastSlice >= 0) )
                || ( (sliceAllocs->ipv4UnicastFirstSlice >= 0) &&
                     (sliceAllocs->ipv4UnicastLastSlice < 0) ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            if (   ( (sliceAllocs->ipv4MulticastFirstSlice < 0) &&
                     (sliceAllocs->ipv4MulticastLastSlice >= 0) )
                || ( (sliceAllocs->ipv4MulticastFirstSlice >= 0) &&
                     (sliceAllocs->ipv4MulticastLastSlice < 0) ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            if (   ( (sliceAllocs->ipv6UnicastFirstSlice < 0) &&
                     (sliceAllocs->ipv6UnicastLastSlice >= 0) )
                || ( (sliceAllocs->ipv6UnicastFirstSlice >= 0) &&
                     (sliceAllocs->ipv6UnicastLastSlice < 0) ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            if (   ( (sliceAllocs->ipv6MulticastFirstSlice < 0) &&
                     (sliceAllocs->ipv6MulticastLastSlice >= 0) )
                || ( (sliceAllocs->ipv6MulticastFirstSlice >= 0) &&
                     (sliceAllocs->ipv6MulticastLastSlice < 0) ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            if (   ( (sliceAllocs->aclFirstSlice < 0) &&
                     (sliceAllocs->aclLastSlice >= 0) )
                || ( (sliceAllocs->aclFirstSlice >= 0) &&
                     (sliceAllocs->aclLastSlice < 0) ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                goto ABORT;
            }

            LOCK_SWITCH(sw);
            switchLocked = TRUE;
            break;

    }   /* end switch (attr) */

    FM_API_CALL_FAMILY(err, switchPtr->SetSwitchAttribute, sw, attr, value);

ABORT:

    if (switchLocked)
    {
        UNLOCK_SWITCH(sw);
    }

    UNPROTECT_SWITCH(sw);


    FM_LOG_EXIT_API(FM_LOG_CAT_ATTR, err);

}   /* end fmSetSwitchAttribute */




/*****************************************************************************/
/** fmStopSwitchTraffic
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Stop the traffic going through the switch and drain the memory.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmStopSwitchTraffic(fm_int sw)
{
    fm_status               err;
    fm_switch *             switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->StopTraffic, sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmStopSwitchTraffic */




/*****************************************************************************/
/** fmRestartSwitchTraffic
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Restarts traffic on the specified switch following 
 *                  a call to fmStopSwitchTraffic.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRestartSwitchTraffic(fm_int sw)
{
    fm_status               err;
    fm_switch *             switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->RestartTraffic, sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmRestartSwitchTraffic */


