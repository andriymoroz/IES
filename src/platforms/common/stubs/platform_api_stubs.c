/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_api_stubs.c
 * Creation Date:   January 11, 2013
 * Description:     Default implementations of Platform API functions.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

/* Add an error check in case platform has not been updated properly */
#if defined(FM_HAVE_fmPlatformShowPortState)
#error FM_HAVE_fmPlatformShowPortState has been replaced by FM_HAVE_fmPlatformNotifyPortState
#endif


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




#if !defined(FM_HAVE_fmPlatformAddGlortToPortMapping)

/*****************************************************************************/
/** fmPlatformAddGlortToPortMapping
 * \ingroup platform
 *
 * \desc            Inform the platform code about the glort to port mapping.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       glort is the glort value being mapped. 
 *
 * \param[in]       physPort is the physical port the glort maps to. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformAddGlortToPortMapping(fm_int  sw,
                                          fm_int  glort,
                                          fm_int  physPort)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(glort);
    FM_NOT_USED(physPort);

    return FM_OK;

}   /* end fmPlatformAddGlortToPortMapping */

#endif



#if !defined(FM_HAVE_fmPlatformBypassEnabled)

/*****************************************************************************/
/** fmPlatformBypassEnabled
 * \ingroup platform
 *
 * \desc            Called by both platform and API services to query whether
 *                  the platform is currently in bypass mode. The value FALSE
 *                  indicates that either bypass mode is not supported, or
 *                  it is supported but not enabled.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          TRUE if bypass mode is enabled, FALSE otherwise.
 *
 *****************************************************************************/
fm_bool fmPlatformBypassEnabled(fm_int sw)
{
    FM_NOT_USED(sw);

    return FALSE;

}   /* end fmPlatformBypassEnabled */

#endif




#if  defined(FM_SUPPORT_SWAG)
#if !defined(FM_HAVE_fmPlatformCreateSWAG)

/*****************************************************************************/
/** fmPlatformCreateSWAG
 * \ingroup platform
 *
 * \desc            Assigns a new switch aggregate switch number.
 *
 * \param[out]      swagSw points to caller-allocated storage for the switch
 *                  number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if sw is NULL.
 * \return          FM_ERR_INVALID_SWITCH if the switch aggregate cannot be
 *                  created.
 *
 *****************************************************************************/
fm_status fmPlatformCreateSWAG(fm_int *swagSw)
{
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "swagSw=%p\n", (void *) swagSw);

    if (swagSw == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    /* First try to get the SWAG number just below the first physical switch */
    i = FM_FIRST_FOCALPOINT - 1;

    if ( (i >= 0) && (fmRootPlatform->swagNumbers[i] == -1) )
    {
        fmRootPlatform->swagNumbers[i] = i;
        *swagSw = i;
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    /* Search for an available switch aggregate */
    for (i = 0 ; i < FM_MAX_SWITCH_AGGREGATES ; i++)
    {
        if (fmRootPlatform->swagNumbers[i] == -1)
        {
            if (i < FM_FIRST_FOCALPOINT)
            {
                fmRootPlatform->swagNumbers[i] = i;
            }
            else
            {
                fmRootPlatform->swagNumbers[i] =
                    FM_LAST_FOCALPOINT + 1 + (i - FM_FIRST_FOCALPOINT);
            }

            *swagSw = fmRootPlatform->swagNumbers[i];
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_SWITCH);

}   /* end fmPlatformCreateSWAG */

#endif
#endif




#if  defined(FM_SUPPORT_SWAG)
#if !defined(FM_HAVE_fmPlatformDeleteSWAG)

/*****************************************************************************/
/** fmPlatformDeleteSWAG
 * \ingroup platform
 *
 * \desc            Releases a switch aggregate switch number.
 *
 * \param[out]      sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch aggregate number
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fmPlatformDeleteSWAG(fm_int sw)
{
    fm_status status;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    status = FM_ERR_INVALID_SWITCH;

    for (i = 0 ; i < FM_MAX_SWITCH_AGGREGATES ; i++)
    {
        if (fmRootPlatform->swagNumbers[i] == sw)
        {
            fmRootPlatform->swagNumbers[i] = -1;
            status = FM_OK;
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformDeleteSWAG */

#endif
#endif




#if !defined(FM_HAVE_fmPlatformDisableInterrupt)

/*****************************************************************************/
/* fmPlatformDisableInterrupt
 * \ingroup platform
 *
 * \desc            Disable the interrupt signal from the switch device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       intrTypes is a mask of the different interrupt types to
 *                  disable. See ''Platform Interrupt Types'' for potential
 *                  values. Note that the types of interrupts supported
 *                  are specific to the platform.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformDisableInterrupt(fm_int sw, fm_uint intrTypes)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(intrTypes);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformDisableInterrupt not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformDisableInterrupt */

#endif




#if !defined(FM_HAVE_fmPlatformEnableInterrupt)

/*****************************************************************************/
/** fmPlatformEnableInterrupt
 * \ingroup platform
 *
 * \desc            Enable the interrupt signal from the switch device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       intrTypes is a mask of the different interrupt types to
 *                  enable. See ''Platform Interrupt Types'' for potential
 *                  values. Note that the types of interrupts supported
 *                  are specific to the platform.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformEnableInterrupt(fm_int sw, fm_uint intrTypes)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(intrTypes);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformEnableInterrupt not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformEnableInterrupt */

#endif




#if !defined(FM_HAVE_fmPlatformGetInterrupt)

/*****************************************************************************/
/** fmPlatformGetInterrupt
 * \ingroup platform
 *
 * \desc            Called by the interrupt handler to identify the source of
 *                  an interrupt and dispatch the appropriate handler.
 *                                                                      \lb\lb
 *                  Typically, this function is responsible for taking the
 *                  following steps:
 *                                                                      \lb\lb
 *                  - Identify the source of the interrupt as being from a
 *                  switch device, a PHY or some other device not managed
 *                  by the API.
 *                                                                      \lb\lb
 *                  - If the source is a switch device, disable further
 *                  interrupts and dispatch the API's switch interrupt handler.
 *                  If a PHY, disable further interrupts and dispatch the
 *                  PHY interrupt handler (which may be platform-specific). If
 *                  the interrupt is from a device not managed by the
 *                  API, the appropriate platform-specific action should be
 *                  taken.
 *
 * \param[in]       sw is the switch number to get the interrupt state for
 *
 * \param[in]       intrType should contain only one bit, indicating the
 *                  interrupt to block on.
 *
 * \param[out]      intrSrc is the resultant source trigger. 
 *                  See ''Platform Interrupt Types'' for potential
 *                  values. Note that the interrupt sources supported
 *                  are specific to the platform.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetInterrupt(fm_int sw, fm_uint intrType, fm_uint *intrSrc)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(intrType);
    FM_NOT_USED(intrSrc);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformGetInterrupt not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformGetInterrupt */

#endif




#if !defined(FM_HAVE_fmPlatformGetPortCapabilities)

/*****************************************************************************/
/** fmPlatformGetPortCapabilities
 * \ingroup platform
 *
 * \desc            Returns a mask of capabilities for the given physical port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       physPort is the physical port number.
 *
 * \param[out]      capabilities points to storage where a bitmap of
 *                  capabilities will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetPortCapabilities(fm_int     sw,
                                        fm_int     physPort,
                                        fm_uint32 *capabilities)
{
    fm_switch  *switchPtr;

    if (sw < FM_FIRST_FOCALPOINT || sw > FM_LAST_FOCALPOINT)
    {
        return FM_ERR_INVALID_SWITCH;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (physPort == 0)
    {
        *capabilities = 0;
    }
    else if ( (physPort >= 1) && (physPort <= switchPtr->maxPhysicalPort) )
    {
        *capabilities = FM_PORT_CAPABILITY_LAG_CAPABLE |
                        FM_PORT_CAPABILITY_SPEED_10M |
                        FM_PORT_CAPABILITY_SPEED_100M |
                        FM_PORT_CAPABILITY_SPEED_1G |
                        FM_PORT_CAPABILITY_SPEED_10G;
    }
    else
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    return FM_OK;

}   /* end fmPlatformGetPortCapabilities */

#endif




#if  defined(FM_SUPPORT_FM2000) || defined(FM_SUPPORT_FM4000)
#if !defined(FM_HAVE_fmPlatformGetPortClockSel)

/*****************************************************************************/
/** fmPlatformGetPortClockSel
 * \ingroup platform
 * 
 * \chips           FM2000, FM4000
 *
 * \desc            Returns which clock to use (0=REFCLKA, 1=REFCLKB) for
 *                  the physical port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       physPort is the physical port number.
 *
 * \param[in]       speed of the port in Mbps.
 *
 * \param[out]      clockSel points to a fm_int which will be set to 0 or 1
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetPortClockSel(fm_int  sw,
                                    fm_int  physPort,
                                    fm_int  speed,
                                    fm_int *clockSel)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(physPort);

    *clockSel = (speed < 2500) ? 0 : 1;

    return FM_OK;

}   /* end fmPlatformGetPortClockSel */

#endif
#endif




#if  defined(FM_SUPPORT_FM6000)
#if !defined(FM_HAVE_fmPlatformGetPortDefaultSettings)

/*****************************************************************************/
/** fmPlatformGetPortDefaultSettings
 * \ingroup platform
 * 
 * \chips           FM6000
 *
 * \desc            Returns the default values for certain port attributes
 *                  as set by the platform layer at initialization for the
 *                  specified logical port.
 *                  This function is provided strictly for the benefit of
 *                  the diagnostic function ''fmDbgDumpPortMap'',
 *                  and is not called for non-FM6000 devices. 
 *                                                                      \lb\lb
 *                  Since the muxing of MACs (EPLs), lane ordering and lane 
 *                  polarity are all dependent on how a platform is physically 
 *                  wired, the platform layer will typically set the 
 *                  associated attributes at initialization time:
 *                                                                      \lb\lb
 *                  ''FM_PORT_ETHERNET_INTERFACE_MODE''                     \lb
 *                  ''FM_PORT_SELECT_ACTIVE_MAC''                           \lb
 *                  ''FM_PORT_TX_LANE_ORDERING''                            \lb
 *                  ''FM_PORT_RX_LANE_ORDERING''                            \lb
 *                  ''FM_PORT_TX_LANE_POLARITY''                            \lb
 *                  ''FM_PORT_RX_LANE_POLARITY''
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logPort is the logical port number whose values are
 *                  to be returned.
 *
 * \param[out]      activeMac points to caller-allocated storage where this
 *                  function should place the default active MAC, corresponding
 *                  to the ''FM_PORT_SELECT_ACTIVE_MAC'' port attribute.
 *
 * \param[out]      ethMode points to caller-allocated storage where this
 *                  function should place the default Ethernet mode, corresponding
 *                  to the ''FM_PORT_ETHERNET_INTERFACE_MODE'' port attribute.
 *
 * \param[out]      ordering points to caller-allocated storage where this
 *                  function should place the default RX and TX lane ordering,
 *                  corresponding to the ''FM_PORT_RX_LANE_ORDERING'' and
 *                  ''FM_PORT_TX_LANE_ORDERING'' port attributes. See
 *                  ''Platform Lane Reversal Values'' for possible values.
 *
 * \param[out]      polarity points to caller-allocated storage where this
 *                  function should place the default RX and TX lane polarity,
 *                  corresponding to the ''FM_PORT_RX_LANE_POLARITY'' and
 *                  ''FM_PORT_TX_LANE_POLARITY'' port attributes. See
 *                  ''Platform Lane Polarity Values'' for possible values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if logical port is out of range.
 *
 *****************************************************************************/
fm_status fmPlatformGetPortDefaultSettings(fm_int  sw,
                                           fm_int  logPort,
                                           fm_int *activeMac,
                                           fm_int *ethMode,
                                           fm_int *ordering,
                                           fm_int *polarity)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(logPort);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d, logPort %d\n", sw, logPort);

    *activeMac = 0;
    *ethMode   = 0;
    *ordering  = 0;
    *polarity  = 0;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformGetPortDefaultSettings */

#endif
#endif




#if  defined(FM_SUPPORT_FM6000)
#if !defined(FM_HAVE_fmPlatformGetSchedulerConfig)

/*****************************************************************************/
/** fmPlatformGetSchedulerConfig
 * \ingroup platform
 * 
 * \chips           FM6000
 *
 * \desc            Indicates to the API how the scheduler should be
 *                  initialized for a FM6000 device. Note that this
 *                  function is not called for non-FM6000 switch devices.
 *                  There are three modes supported to initialize the scheduler.
 *                  Each mode is described below:
 *                                                                      \lb\lb
 *                  In FM_SCHED_INIT_MODE_AUTOMATIC mode, the
 *                  fm_schedulerConfig structure pointer passed by the API
 *                  should be filled with 40G and 10G port capabilities.
 *                                                                      \lb\lb
 *                  Each port can be in one of three classes. The purpose of
 *                  this mode is to identify the class that each port
 *                  belongs to. The structure takes two lists of ports, a 40G
 *                  list and a 10G list. A port's appearance in one or the
 *                  other or neither list serves to identify its class. The
 *                  classes are as follows:
 *                                                                      \lb\lb
 *                  40G capable ports: a port that may be operated at 40G. It
 *                  might also be used for 10G or less. Such ports must appear
 *                  in the 40G list. Only the lowest numbered port of the four
 *                  ports sharing a MAC (EPL) is put into the 40G port list;
 *                  the other three ports are implied and should not appear in
 *                  either list. A port that appears in the 40G list should
 *                  not appear in the 10G list, even if it will sometimes
 *                  operate at 10G or less. If a port is equipped with two
 *                  MACs and may run at 40G on only one MAC and will run only
 *                  at 10G or less on the other MAC, it is still considered a
 *                  40G capable port and should appear in the 40G list only.
 *                                                                      \lb\lb
 *                  10G (or less) only ports: a port that will never be
 *                  operated at 40G, but will be used at 10G or less. Such
 *                  ports must appear in the 10G list, unless the port shares
 *                  a MAC (EPL) with another port that appears in the 40G list.
 *                                                                      \lb\lb
 *                  Unused ports: a port that will never be used at any speed
 *                  should not appear in either list.
 *                                                                      \lb\lb
 * \note            The number of 40G ports should not exceed 18 and the number
 *                  of 10G ports should not exceed 72.
 *                                                                      \lb\lb
 *                  In the FM_SCHED_INIT_MODE_MANUAL mode, the
 *                  fm_schedulerConfig structure should be filled with token
 *                  information to be directly written to the scheduler.
 *                  The API validates this data to ensure the token list does
 *                  not violate any scheduler initalization rule. The rules
 *                  are as follows:
 *                                                                      \lb\lb
 *                  1. Tokens should be loaded in alternating port color
 *                  2. Port tokens for the same PORT4 are at least four tokens
 *                     appart.
 *                  3. 40G capable ports should be spaced as evenly as possible.
 *                     Note that this rule is not validated but should be
 *                     applied.
 *
 * \note            The port numbers passed into this structure should
 *                  use physical port numbers, being the numbers of the
 *                  switch fabric's "internal ports". The ports may be listed
 *                  in any order.
 *
 *                  In the FM_SCHED_INIT_MODE_NONE mode, the API will not
 *                  initialize the scheduler, it is therefore the platform's
 *                  responsibility to do so. In this mode, it is required
 *                  to configure the nbTokens and nbSyncTokens fields.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      sc is a pointer to the caller-supplied scheduler config
 *                  structure and should be filled with the desired mode and
 *                  data. Refer to fm_schedulerConfig for details on the
 *                  limits of certains structure fields and which fields
 *                  are required in each mode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetSchedulerConfig(fm_int sw, fm_schedulerConfig *sc)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(sc);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "fmPlatformGetSchedulerConfig not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformGetSchedulerConfig */

#endif
#endif




#if !defined(FM_HAVE_fmPlatformGetSwitchPartNumber)

/*****************************************************************************/
/** fmPlatformGetSwitchPartNumber
 * \ingroup platform
 *
 * \desc            Returns the part number of the switch.
 *
 * \param[in]       sw is the switch number to operate on.
 * 
 * \param[out]      spn points to caller-supplied storage where the switch
 *                  part number should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetSwitchPartNumber(fm_int sw, fm_switchPartNum *spn)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(spn);

    return FM_ERR_UNSUPPORTED;
    
}   /* end fmPlatformGetSwitchPartNumber */

#endif




#if !defined(FM_HAVE_fmPlatformInitialize)

/*****************************************************************************/
/** fmPlatformInitialize
 * \ingroup platform
 *
 * \desc            Called as part of API initialization to perform basic
 *                  platform-specific initialization. In particular, this
 *                  function should do the following:
 *                                                                      \lb\lb
 *                  - Initialize any platform-global constructs (e.g.,
 *                  structures, variables, etc.
 *                                                                      \lb\lb
 *                  - Perform actions required by the operating system for
 *                  the API to run on this platform. For example, under
 *                  Linux, memory must be mapped into user space for use as
 *                  packet buffers, the switch device register file must be
 *                  memory mapped into user space and an IRQ must be reserved.
 *                                                                      \lb\lb
 *                  - For platforms with fixed switch devices (i.e., not
 *                  a hot-swappable bladed chassis), fmGlobalEventHandler
 *                  should be called with a FM_EVENT_SWITCH_INSERTED (see
 *                  ''Event Identifiers'') event for each switch. Note: if
 *                  fmGlobalEventHandler is called, it should be the last
 *                  thing done by this function before returning.
 *
 * \param[out]      nSwitches points to storage where this function should
 *                  place the maximum number of switch devices supported
 *                  by the platform. Note that this value may differ from the
 *                  actual number of existing devices on platforms that
 *                  support hot-swappable switches, such as bladed chassis.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformInitialize(fm_int *nSwitches)
{

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    *nSwitches = 0;

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformInitialize not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformInitialize */

#endif




#if  defined(FM_SUPPORT_FM6000)
#if !defined(FM_HAVE_fmPlatformLoadMicrocode)

/*****************************************************************************/
/** fmPlatformLoadMicrocode
 * \ingroup platform
 * 
 * \chips           FM6000
 *
 * \desc            Called by the API to request the platform layer to load
 *                  microcode into the device. This function is not called
 *                  for non-FM6000 devices.
 *
 * \param[in]       sw is the switch on which to operate. The switch
 *                  number must have already been validated.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLoadMicrocode(fm_int sw)
{
    FM_NOT_USED(sw);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformLoadMicrocode not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformLoadMicrocode */

#endif
#endif




#if !defined(FM_HAVE_fmPlatformMACMaintenanceSupported)

/*****************************************************************************/
/** fmPlatformMACMaintenanceSupported
 * \ingroup platform
 *
 * \desc            Indicates if MAC table maintenance should be performed
 *                  by the API. There are two aspects to MAC table maintenance.
 *                                                                      \lb\lb
 *                  The first aspect is whether the MAC table maintenance
 *                  thread in the API should process the MAC table on a
 *                  periodic basis or be strictly event driven. FM2000,
 *                  FM3000 and FM4000 devices generally require periodic
 *                  processing while FM6000 devices can be strictly event
 *                  driven. The API will call this function at initialization
 *                  time with the sw argument set to -1 to determine if the
 *                  thread should run periodically.
 *                                                                      \lb\lb
 *                  Return TRUE to enable periodic processing.              \lb
 *                  Return FALSE to disable periodic processing.
 *                                                                      \lb\lb
 *                  The second aspect is whether an individual switch
 *                  should be processed at all. The API will call
 *                  this function each time the maintenance thread wakes up
 *                  (whether because of an event or a timeout) for each switch
 *                  in the platform, with the sw argument set to the switch
 *                  number. Generally, this function should return TRUE for
 *                  all switches. The function might return FALSE in the case
 *                  of a switch that has learning disabled. A classic example
 *                  is a spine switch in a fat-tree architecture that is
 *                  controlled via FIBM. Spine switches may be configured to
 *                  do no learning and the FIBM traffic that would be
 *                  generated by the maintenance thread could be a significant
 *                  waste of bandwidth.
 *
 * \param[in]       sw will be -1 if the API is asking whether periodic MAC
 *                  Table maintenance is needed by any switch in the platform.
 *                  Otherwise, sw is a specific switch number, in which case
 *                  the API is asking whether this switch's MAC Table should
 *                  be processed at all.
 *
 * \return          TRUE if sw is -1 and periodic maintenance is required by at
 *                  least one switch in the platform, or if sw is positive and
 *                  that switch number should have its MAC table processed.
 * \return          FALSE if sw is -1 and periodic maintenance is not required
 *                  by any switches in the platform, or if sw is positive and
 *                  that switch number should not have its MAC table processed.
 *
 *****************************************************************************/
fm_bool fmPlatformMACMaintenanceSupported(fm_int sw)
{
    FM_NOT_USED(sw);

    return TRUE;

}   /* end fmPlatformMACMaintenanceSupported */

#endif




#if !defined(FM_HAVE_fmPlatformMapLogicalPortToPhysical)

/*****************************************************************************/
/** fmPlatformMapLogicalPortToPhysical
 * \ingroup platform
 *
 * \desc            Maps a logical port number to a (switch, physical port)
 *                  tuple.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *                                                                      \lb\lb
 *                  The (switch, physical port) tuple identifies the physical
 *                  port number of a particular switch device, as
 *                  identified on the pin-out of the device.
 *
 * \param[in]       logicalSwitch is the logical switch number.
 *
 * \param[in]       logicalPort is the logical port number.
 *
 * \param[out]      switchNum points to storage where the switch number of the
 *                  (switch, physical port) tuple should be stored.
 *
 * \param[out]      physPort points to storage where the physical port number
 *                  of the (switch, physical port) tuple should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapLogicalPortToPhysical(fm_int  logicalSwitch,
                                             fm_int  logicalPort,
                                             fm_int *switchNum,
                                             fm_int *physPort)
{
    FM_NOT_USED(logicalSwitch);
    FM_NOT_USED(logicalPort);
    FM_NOT_USED(switchNum);
    FM_NOT_USED(physPort);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformMapLogicalPortToPhysical not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformMapLogicalPortToPhysical */

#endif




#if !defined(FM_HAVE_fmPlatformMapPortLaneToEplLane)

/*****************************************************************************/
/** fmMapPortLaneToEplLane
 * \ingroup platform
 *
 * \desc            Maps a logical port lane number to a EPL lane number.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *
 * \param[in]       sw is the logical switch number.
 *
 * \param[in]       logicalPort is the logical port number.
 *
 * \param[in]       portLaneNum is the logical port lane number.
 *
 * \param[out]      eplLane points to storage where the EPL lane number
 *                  should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapPortLaneToEplLane(fm_int  sw,
                                         fm_int  logicalPort,
                                         fm_int  portLaneNum,
                                         fm_int *eplLane)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(logicalPort);
    FM_NOT_USED(portLaneNum);
    FM_NOT_USED(eplLane);

    return FM_OK;

}   /* end fmPlatformMapLogicalPortToPhysical */

#endif




#if !defined(FM_HAVE_fmPlatformMapPhysicalPortToLogical)

/*****************************************************************************/
/** fmPlatformMapPhysicalPortToLogical
 * \ingroup platform
 *
 * \desc            Maps a (switch, physical port) tuple to a logical port
 *                  number.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *                                                                      \lb\lb
 *                  The (switch, physical port) tuple identifies the physical
 *                  port number of a particular switch device, as
 *                  identified on the pin-out of the device.
 *
 * \param[in]       switchNum is the switch number of the (switch, physical
 *                  port) tuple.
 *
 * \param[in]       physPort is the physical port number of the (switch,
 *                  physical port) tuple.
 *
 * \param[out]      logicalSwitch points to storage where the corresponding
 *                  logical switch number should be stored.
 *
 * \param[out]      logicalPort points to storage where the corresponding
 *                  logical port number should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapPhysicalPortToLogical(fm_int  switchNum,
                                             fm_int  physPort,
                                             fm_int *logicalSwitch,
                                             fm_int *logicalPort)
{
    FM_NOT_USED(switchNum);
    FM_NOT_USED(physPort);
    FM_NOT_USED(logicalSwitch);
    FM_NOT_USED(logicalPort);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformMapPhysicalPortToLogical not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformMapPhysicalPortToLogical */

#endif




#if !defined(FM_HAVE_fmPlatformReceivePackets)

/*****************************************************************************/
/** fmPlatformReceivePackets
 * \ingroup platform
 *
 * \desc            Called to try and receive packets.  Responsible for
 *                  generating events for packets received.
 *
 * \param[in]       sw is the switch on which to receive packets.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformReceivePackets(fm_int sw)
{
    FM_NOT_USED(sw);

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformReceivePackets */

#endif




#if !defined(FM_HAVE_fmPlatformRelease)

/*****************************************************************************/
/** fmPlatformRelease
 * \ingroup platform
 *
 * \desc            Deassert the reset signal into the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformRelease(fm_int sw)
{
    FM_NOT_USED(sw);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, "fmPlatformRelease not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformRelease */

#endif




#if !defined(FM_HAVE_fmPlatformReset)

/*****************************************************************************/
/** fmPlatformReset
 * \ingroup platform
 *
 * \desc            Assert the reset signal into the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformReset(fm_int sw)
{
    FM_NOT_USED(sw);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, "fmPlatformReset not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformReset */

#endif




#if !defined(FM_HAVE_fmPlatformSendPacket)

/*****************************************************************************/
/** fmPlatformSendPacket
 * \ingroup platform
 *
 * \desc            Add the given packet to the internal packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       info is a pointer to associated information about
 *                  the packet including where it is going.
 *
 * \param[in]       packet is a pointer to a chain of fm_buffer structures
 *                  containing the payload.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSendPacket(fm_int         sw,
                               fm_packetInfo *info,
                               fm_buffer *    packet)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(info);
    FM_NOT_USED(packet);

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSendPacket */

#endif




#if !defined(FM_HAVE_fmPlatformSendPacketDirected)

/*****************************************************************************/
/** fmPlatformSendPacketDirected
 * \ingroup platform
 *
 * \desc            Called to add the given packet to the internal packet
 *                  queue in the directed packet send mode.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       portList points to an array of logical port numbers the
 *                  switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \param[in]       packet is a pointer to a chain of fm_buffer structures
 *                  containing the payload.
 * 
 * \param[in]       info is a pointer to the packet information structure which
 *                  contains some relevant information describing the packet.
 *                  See 'fm_packetInfoV2' for more information.
 *                  Note: the structure pointed to by info must have all fields
 *                  initialized. Any unused field should be set to zero.
 *                  Failure to initialize all fields can result in the packet
 *                  being mishandled.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSendPacketDirected(fm_int           sw,
                                       fm_int *         portList,
                                       fm_int           numPorts,
                                       fm_buffer *      packet,
                                       fm_packetInfoV2 *info)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(portList);
    FM_NOT_USED(numPorts);
    FM_NOT_USED(packet);
    FM_NOT_USED(info);

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSendPacketDirected */

#endif




#if !defined(FM_HAVE_fmPlatformSendPackets)

/*****************************************************************************/
/** fmPlatformSendPackets
 * \ingroup platform
 *
 * \desc            Called to trigger the sending (or attempt thereof) of the
 *                  current packet queue.
 *
 * \param[in]       sw is the switch on which to trigger the sending of packets.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSendPackets(fm_int sw)
{
    FM_NOT_USED(sw);

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSendPackets */

#endif




#if !defined(FM_HAVE_fmPlatformSendPacketSwitched)

/*****************************************************************************/
/** fmPlatformSendPacketSwitched
 * \ingroup platform
 *
 * \desc            Called to add the given packet to the internal packet
 *                  queue in the lookup packet send mode.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       packet is a pointer to a chain of fm_buffer structures
 *                  containing the payload.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSendPacketSwitched(fm_int sw, fm_buffer *packet)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(packet);

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSendPacketSwitched */

#endif




#if !defined(FM_HAVE_fmPlatformSendPacketISL)

/*****************************************************************************/
/** fmPlatformSendPacketISL
 * \ingroup platform
 *
 * \desc            Called to add the given packet to the internal packet
 *                  queue in the lookup packet send mode.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       islTag points to the ISL tag contents
 *
 * \param[in]       islTagFormat is the ISL tag format value.
 *
 * \param[in]       buffer points to a chain of fm_buffer structures
 *                  containing the payload.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSendPacketISL(fm_int          sw,
                                  fm_uint32 *     islTag,
                                  fm_islTagFormat islTagFormat,
                                  fm_buffer *     buffer)
{
    fm_status err;
    fm_islTag tag;

    FM_LOG_ENTRY( FM_LOG_CAT_PLATFORM,
                  "sw=%d islTag = %p islTagFormat = %d buffer=%p\n",
                  sw,
                  (void *) islTag,
                  islTagFormat,
                  (void *) buffer );

    FM_CLEAR(tag);

    switch (islTagFormat)
    {
        case FM_ISL_TAG_F64:
            tag.f64.tag[0] = islTag[0];
            tag.f64.tag[1] = islTag[1];
            break;

        case FM_ISL_TAG_F56:
            tag.f56.tag[0] = islTag[0];
            tag.f56.tag[1] = islTag[1];
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    }

    err = fmGenericSendPacketISL(sw, &tag, islTagFormat, 1, buffer);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformSendPacketISL */

#endif




#if !defined(FM_HAVE_fmPlatformSetBypassMode)

/*****************************************************************************/
/** fmPlatformSetBypassMode
 * \ingroup platform
 *
 * \desc            Enables bypass mode for the given switch on platforms
 *                  that support the feature.  Typically the mode is set to
 *                  TRUE some time during platform initialization, and is
 *                  set to FALSE later from the application.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       mode is the bypass mode to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetBypassMode(fm_int sw, fm_bool mode)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(mode);

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSetBypassMode */

#endif




#if !defined(FM_HAVE_fmPlatformSetCpuPortVlanTag)

/*****************************************************************************/
/** fmPlatformSetCpuPortVlanTag
 * \ingroup platform
 *
 * \desc            Inform the platform code about the cpu port vlan tagging
 *                  mode.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       vlanId is the vlan number.
 *
 * \param[in]       tag is TRUE if the cpu port is to transmit tagged frames
 *                  for this vlan, FALSE if it is to transmit untagged frames
 *                  for this vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetCpuPortVlanTag(fm_int sw, fm_uint16 vlanId, fm_bool tag)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(vlanId);
    FM_NOT_USED(tag);

    return FM_OK;    

}   /* end fmPlatformSetCpuPortVlanTag */

#endif




#if !defined (FM_HAVE_fmPlatformSetPortDefaultVlan)

/*****************************************************************************/
/** fmPlatformSetPortDefaultVlan
 * \ingroup platform
 *
 * \desc            Inform the platform code about the default VLAN
 *                  change on a given port.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       physPort is the physical port being updated.
 *
 * \param[in]       defaultVlan is the new default vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetPortDefaultVlan(fm_int sw,
                                       fm_int physPort,
                                       fm_int defaultVlan)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(physPort);
    FM_NOT_USED(defaultVlan);

    return FM_OK;

}   /* end fmPlatformSetPortDefaultVlan */

#endif




#if !defined(FM_HAVE_fmPlatformSetRemoteGlortToLogicalPortMapping)

/*****************************************************************************/
/** fmPlatformSetRemoteGlortToLogicalPortMapping
 * \ingroup platform
 *
 * \desc            Inform the platform code about the remote glort to logical
 *                  port mapping.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       glort is the glort value being mapped.
 *
 * \param[in]       port is the logical port the glort maps to.
 *
 * \param[in]       valid indicates whether mapping is valid or should be
 *                  invalidated
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetRemoteGlortToLogicalPortMapping(fm_int  sw,
                                                       fm_int  glort,
                                                       fm_int  port,
                                                       fm_bool valid)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(glort);
    FM_NOT_USED(port);
    FM_NOT_USED(valid);

    return FM_OK;

}   /* end fmPlatformSetRemoteGlortToLogicalPortMapping */

#endif




#if !defined(FM_HAVE_fmPlatformNotifyPortState)

/*****************************************************************************/
/** fmPlatformNotifyPortState
 * \ingroup platform
 *
 * \desc            Notify port state change from application configuration or
 *                  from port event. This function is called by the API whenever
 *                  the port state changes and could be used by the platform to
 *                  implement some actions depending on port state. An example
 *                  is changing the LED status for that port and the
 *                  transceiver enable state.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       mac is the mac number, if applicable.
 *
 * \param[in]       isConfig specifies whether the state is notified from the
 *                  port configuration or from port event notification.
 *
 * \param[in]       state indicates the port's state (see 'Port States') to
 *                  show.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformNotifyPortState(fm_int  sw,
                                    fm_int  port,
                                    fm_int  mac,
                                    fm_bool isConfig,
                                    fm_int  state)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(mac);
    FM_NOT_USED(isConfig);
    FM_NOT_USED(state);

    return FM_OK;

}   /* end fmPlatformNotifyPortState */

#endif




#if !defined(FM_HAVE_fmPlatformNotifyPortAttribute)

/*****************************************************************************/
/* fmPlatformNotifyPortAttribute
 * \ingroup platform
 *
 * \desc            This function is called by the API whenever one of the
 *                  following port attributes is configured by the
 *                  application:
 *
 *                  FM_PORT_TX_LANE_CURSOR, FM_PORT_TX_LANE_PRECURSOR
 *                  and FM_PORT_TX_LANE_POSTCURSOR.
 * 
 *                  This allow to the platform code to save the attribute
 *                  values and use them when fmPlatformMgmtNotifyEthModeChange
 *                  is called upon a FM_PORT_ETHERNET_INTERFACE_MODE change.
 * 
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       mac is the mac number, if applicable.
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. May be specified as FM_PORT_LANE_NA for non-lane
 *                  oriented attributes.
 *
 * \param[in]       attribute is the port attribute (see 'Port Attributes')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformNotifyPortAttribute(fm_int sw,
                                        fm_int port,
                                        fm_int mac,
                                        fm_int lane,
                                        fm_int attribute,
                                        void * value)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(port);
    FM_NOT_USED(mac);
    FM_NOT_USED(lane);
    FM_NOT_USED(attribute);
    FM_NOT_USED(value);

    return FM_OK;

}   /* end fmPlatformNotifyPortAttribute */

#endif


#if  defined(FM_SUPPORT_SWAG)
#if !defined(FM_HAVE_fmPlatformSWAGInitialize)

/*****************************************************************************/
/** fmPlatformSWAGInitialize
 * \ingroup platform
 *
 * \desc            Called by the API during switch inserted event processing
 *                  if the switch is a SWAG.  This function performs platform-
 *                  specific initialization for the SWAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSWAGInitialize(fm_int sw)
{
    FM_NOT_USED(sw);

    return FM_OK;

}   /* end fmPlatformSWAGInitialize */

#endif
#endif




#if !defined(FM_HAVE_fmPlatformSwitchInitialize)

/*****************************************************************************/
/** fmPlatformSwitchInitialize
 * \ingroup platform
 *
 * \desc            Called by the API in response to an FM_EVENT_SWITCH_INSERTED
 *                  event (see ''Event Identifiers''). This function performs
 *                  platform specific initializations in the switch state table,
 *                  particularly to initialize the function pointers for
 *                  accessing the hardware device registers.
 *                                                                      \lb\lb
 *                  If desired, this function can override device model
 *                  detection, if for example, an FM2224 should be treated as
 *                  as an FM2112.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchInitialize(fm_int sw)
{
    FM_NOT_USED(sw);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformSwitchInitialize not initialized!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSwitchInitialize */

#endif




#if !defined(FM_HAVE_fmPlatformSwitchPostInitialize)

/*****************************************************************************/
/** fmPlatformSwitchPostInitialize
 * \ingroup platform
 *
 * \desc            Called after initializing a switch device. This
 *                  function is responsible for initializing any aspect
 *                  of the switch that is dependent on the platform
 *                  hardware. Examples would be setting lane polarity, lane
 *                  ordering, SERDES drive strength and emphasis, LCI
 *                  endianness, etc.
 *
 * \note            The switch is not yet in an UP state when this function
 *                  is called. Operations that depend on event reporting
 *                  from the switch should not be performed by this function,
 *                  for example, retrieving port state with ''fmGetPortState''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchPostInitialize(fm_int sw)
{
    FM_NOT_USED(sw);

    /**************************************************
     * This is a non-functional stub, provided for 
     * documentation purposes. The platform layer 
     * MUST implement this function for the API to 
     * operate correctly! 
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "fmPlatformSwitchPostInitialize not implemented!\n");

    return FM_ERR_UNSUPPORTED;

}   /* end fmPlatformSwitchPostInitialize */

#endif




#if !defined(FM_HAVE_fmPlatformSwitchPreInitialize)

/*****************************************************************************/
/** fmPlatformSwitchPreInitialize
 * \ingroup platform
 *
 * \desc            Called before initializing a switch device. This
 *                  function is responsible for initializing any aspect
 *                  of the platform required to bring up the device: for
 *                  example, applying power, deasserting the reset signal,
 *                  etc.
 *
 * \param[in]       sw is the switch about to be initialized.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchPreInitialize(fm_int sw)
{
    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSwitchPreInitialize */

#endif




#if !defined(FM_HAVE_fmPlatformSwitchTerminate)

/*****************************************************************************/
/** fmPlatformSwitchTerminate
 * \ingroup platform
 *
 * \desc            Called after shutting down a switch device. This
 *                  function is responsible for any platform-specific actions
 *                  that must be taken when a switch is removed from the
 *                  system; for example, turning off power, asserting the reset
 *                  signal, etc.
 *
 * \param[in]       sw is the switch that has been shut down.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchTerminate(fm_int sw)
{
    FM_NOT_USED(sw);

#if INSTRUMENT_LOG_LEVEL > 0
    fmPlatformCloseInstrumentation();
#endif

    return FM_OK;

}   /* end fmPlatformSwitchTerminate */

#endif




/*****************************************************************************/
/** fmPlatformGetHardwareLagGlortRange
 * \ingroup platform
 *
 * \desc            Returns default lag glort range values for the platform.
 *                  Default values are ~0, which means that the entire glort
 *                  range is available. Chips which do not support the
 *                  entire glort range, such as FM10000, will override
 *                  this function witha a platform/chip-specific function
 *                  in platform_api_stubs.h.
 *
 * \param[in]       lagGlortBase points to caller-provided storage into which
 *                  the default starting glort for LAGs will be written.
 *
 * \param[in]       lagGlortCount points to caller-provided storage into which
 *                  the number of glorts which may be used for LAGs will
 *                  be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetHardwareLagGlortRange(fm_uint32 *lagGlortBase,
                                             fm_uint32 *lagGlortCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "lagGlortBase=%p, lagGlortCount=%p\n",
                 (void *) lagGlortBase,
                 (void *) lagGlortCount);

    if (lagGlortBase != NULL)
    {
        *lagGlortBase = ~0;
    }

    if (lagGlortCount != NULL)
    {
        *lagGlortCount = ~0;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmPlatformGetHardwareLagGlortRange */




/*****************************************************************************/
/** fmPlatformGetHardwareMcastGlortRange
 * \ingroup platform
 *
 * \desc            Returns default multicast glort range values for the
 *                  platform. Default values are ~0, which means that the
 *                  entire glort range is available. Chips that do not support
 *                  the entire glort range, such as FM10000, will override
 *                  this function with a platform/chip-specific function
 *                  in platform_api_stubs.h.
 *
 * \param[in]       mcastGlortBase points to caller-provided storage into
 *                  which the default starting glort for multicast groups
 *                  will be written.
 *
 * \param[in]       mcastGlortCount points to caller-provided storage into
 *                  which the number of glorts which may be used for multicast
 *                  groups will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetHardwareMcastGlortRange(fm_uint32 *mcastGlortBase,
                                               fm_uint32 *mcastGlortCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "mcastGlortBase=%p, mcastGlortCount=%p\n",
                 (void *) mcastGlortBase,
                 (void *) mcastGlortCount);

    if (mcastGlortBase != NULL)
    {
        *mcastGlortBase = ~0;
    }

    if (mcastGlortCount != NULL)
    {
        *mcastGlortCount = ~0;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fmPlatformGetHardwareMcastGlortRange */




/*****************************************************************************/
/** fmPlatformGetHardwareLbgGlortRange
 * \ingroup platform
 *
 * \desc            Returns default lbg glort range values for the platform.
 *                  Default values are ~0, which means that the entire glort
 *                  range is available. Chips which do not support the
 *                  entire glort range, such as FM10000, will override
 *                  this function witha a platform/chip-specific function
 *                  in platform_api_stubs.h.
 *
 * \param[in]       lbgGlortBase points to caller-provided storage into which
 *                  the default starting glort for LBGs will be written.
 *
 * \param[in]       lbgGlortCount points to caller-provided storage into which
 *                  the number of glorts which may be used for LBGs will
 *                  be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetHardwareLbgGlortRange(fm_uint32 *lbgGlortBase,
                                             fm_uint32 *lbgGlortCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "lbgGlortBase=%p, lbgGlortCount=%p\n",
                 (void *) lbgGlortBase,
                 (void *) lbgGlortCount);

    if (lbgGlortBase != NULL)
    {
        *lbgGlortBase = ~0;
    }

    if (lbgGlortCount != NULL)
    {
        *lbgGlortCount = ~0;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end fmPlatformGetHardwareLbgGlortRange */




#if !defined(FM_HAVE_fmPlatformSwitchInserted)

/*****************************************************************************/
/** fmPlatformSwitchInserted
 * \ingroup platform
 *
 * \desc            Called by the API in response to an FM_EVENT_SWITCH_INSERTED
 *                  event (see ''Event Identifiers''). Specifically called
 *                  once the API has completed its internal initialization.
 * 
 *                  This function performs platform specific initializations
 *                  that require the switch to be present, particularly to
 *                  initialize LT shared library which uses the switch I2C
 *                  master.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchInserted(fm_int sw)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    status = FM_OK;
#ifdef FM_SUPPORT_SWAG
    /* Allow the platform to complete SWAG initialization */
    status = fmPlatformSWAGInitialize(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSwitchInserted */

#endif




#if !defined(FM_HAVE_fmPlatformSwitchPreInsert)

/*****************************************************************************/
/** fmPlatformSwitchPreInsert
 * \ingroup platform
 *
 * \desc            Called during the insertion phase to manage booting and
 *                  PCIe init.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchPreInsert(fm_int sw)
{
    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSwitchPreInsert */

#endif




#if !defined(FM_HAVE_fmPlatformGpioInterruptHandler)

/*****************************************************************************/
/** fmPlatformGpioInterruptHandler
 * \ingroup intPlatformGpio
 *
 * \desc            First-stage switch GPIO interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGpioInterruptHandler(fm_switch *switchPtr)
{
    FM_NOT_USED(switchPtr);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", switchPtr->switchNumber);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformGpioInterruptHandler */

#endif




/*****************************************************************************/
/** fmPlatformGetHardwareMailboxGlortRange
 * \ingroup platform
 *
 * \desc            Returns default mailbox glort range values for the
 *                  platform. Default values are ~0, which means that the
 *                  entire glort range is available. Chips that do not support
 *                  the entire glort range, such as FM10000, will override
 *                  this function with a platform/chip-specific function
 *                  in platform_api_stubs.h.
 *
 * \param[out]      mailboxGlortBase points to caller-provided storage into
 *                  which the default starting glort for mailbox
 *                  will be written.
 *
 * \param[out]      mailboxGlortCount points to caller-provided storage into
 *                  which the number of glorts which may be used for 
 *                  mailbox will be written.
 *
 * \param[out]      mailboxGlortMask points to caller-provided storage into
 *                  which the range of glorts per pep which may be used for
 *                  mailbox will be written.
 *
 * \param[out]      mailboxGlortsPerPep  points to caller-provided storage into
 *                  which number of glorts available per pep will be written.
 *
 * \param[in]       numberOfSWAGMembers is the number of switch aggregate 
 *                  members. If number of switches in topology exceeds 
 *                  defined value we need to decrease number of available
 *                  glorts.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetHardwareMailboxGlortRange(fm_uint16 *mailboxGlortBase,
                                                 fm_uint16 *mailboxGlortCount,
                                                 fm_uint16 *mailboxGlortMask,
                                                 fm_uint16 *mailboxGlortsPerPep,
                                                 fm_int     numberOfSWAGMembers)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "mailboxGlortBase=%p, mailboxGlortCount=%p, mailboxGlortMask=%p, "
                 "mailboxGlortsPerPep=%p, numberOfSWAGMembers=%d\n",
                 (void *) mailboxGlortBase,
                 (void *) mailboxGlortCount,
                 (void *) mailboxGlortMask,
                 (void *) mailboxGlortsPerPep,
                 numberOfSWAGMembers);

    if ( (mailboxGlortBase == NULL) || (mailboxGlortCount == NULL)
        || (mailboxGlortMask == NULL) || (mailboxGlortsPerPep == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_ERR_INVALID_ARGUMENT);
    }

    *mailboxGlortBase    = ~0;
    *mailboxGlortMask    = ~0;
    *mailboxGlortCount   = 0;
    *mailboxGlortsPerPep = 0;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fmPlatformGetHardwareMailboxGlortRange */




/*****************************************************************************/
/** fmPlatformGetHardwareNumberOfPeps
 * \ingroup platform
 *
 * \desc            Returns number of PEP ports for the platform. 
 *                  Default value is 0, which means that there are no PEP ports
 *                  available. Chips with PEP ports available, 
 *                  such as FM10000, will override this function with a 
 *                  platform/chip-specific function in platform_api_stubs.h.
 *
 * \return          Number of PEP ports.
 *
 *****************************************************************************/
fm_uint16 fmPlatformGetHardwareNumberOfPeps(void)
{
    return 0;

}   /* end fmPlatformGetHardwareNumberOfPeps */




/*****************************************************************************/
/** fmPlatformGetHardwareMaxMailboxGlort
 * \ingroup platform
 *
 * \desc            Returns the maximum mailbox GloRT value for the platform. 
 *                  Default value is 0, which means that there are no mailbox
 *                  registers available. Chips with mailbox available, 
 *                  such as FM10000, will override this function with a 
 *                  platform/chip-specific function in platform_api_stubs.h.
 *
 * \param[out]      glort points to caller-provided storage into
 *                  which the max mailbox glort value will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetHardwareMaxMailboxGlort(fm_uint32 *glort)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "glort=%p\n",
                 (void *) glort);

    if ( glort == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_ERR_INVALID_ARGUMENT);
    }

    *glort = 0;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_OK);

}   /* end fmPlatformGetHardwareMaxMailboxGlort */




#if !defined(FM_HAVE_fmPlatformTerminate)

/*****************************************************************************/
/** fmPlatformTerminate
 * \ingroup platform
 *
 * \desc            Called as part of API termination for processes other than
 *                  the first process that have called ''fmPlatformInitialize''.
 *                  Performs basic platform-specific clean-up.
 *
 * \param[in]       None
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformTerminate(void)
{
    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformTerminate */

#endif

