/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_types.h
 * Creation Date:   June 2, 2014
 * Description:     Platform specific definitions
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

#ifndef __FM_PLATFORM_TYPES_H
#define __FM_PLATFORM_TYPES_H

/* For registers access */
#include <platforms/common/switch/fm_regs_access_memmap.h>
#include <platforms/common/switch/fm_regs_access_ebi.h>
#include <platforms/common/switch/fm_regs_access_i2c.h>

/* For switch utility functions */
#include <platforms/common/switch/fm10000/fm10000_utils.h>
#include <platforms/util/fm10000/fm10000_util_bsm.h>

/* For packet transfer */
#include <platforms/common/packet/generic-packet/fm_generic_packet.h>
#include <platforms/common/packet/generic-rawsocket/fm_generic_rawsocket.h>
#include <platforms/common/packet/generic-pti/fm10000/fm10000_generic_pti.h>
//#include <platforms/common/packet/generic-nic/fm_generic_nic.h>
#include <platforms/common/packet/generic-packet/fm10000/fm10000_generic_tx.h>
#include <platforms/common/packet/generic-packet/fm10000/fm10000_generic_rx.h>

/* For buffer management */
#include <platforms/common/buffers/std-alloc/fm_buffer_std_alloc.h>

/* For attribute loader */
#include <platforms/common/fm_file_attr_loader.h>

/* For network sockets library */
#include <platforms/common/lib/net/fm_netsock.h>

/* Platform events */
#include <platforms/common/event/fm_platform_event.h>

/* Platform PHY/Transceiver */
#include <platforms/common/phy/fm_platform_xcvr.h>

/* For dynamic library loading (shared-lib) */
#include <platforms/common/lib/dll/fm_dynamicLib.h>

/* Headers for the Model Packet Queue */
#ifdef FM_LT_WHITE_MODEL_SUPPORT
#include <platforms/common/packet/generic-model/fm_generic_model.h>
#include <platforms/common/model/fm_model_message.h>
#include <platforms/common/model/fm_model_packet_queue.h>
#include <platforms/common/model/fm10000/fm10000_model.h>
#include <platforms/whiteModel/platform_common.h>
#include <platforms/common/model/testpoint_support.h>
#endif

/* Interfaces to host kernel driver */
#include <fm_host_drv.h>

#include <platform_attr.h>
#include <platform_buffer_defs.h>
#include <platform_config.h>
#include <platform_config_nvm.h>
#include <platform_config_tlv.h>
#include <platform_lib_api.h>
#include <platform_lib.h>
#include <platform_port.h>
#include <platform_phy.h>
#include <platform_led.h>
#include <platform_mgmt.h>
#include <platform_debug.h>
#include <platform_gpio.h>
/* Only needed for internal use of TestPoint */
#include <platform_app_api.h>

#include <net/if.h>  

/* Platform state structure */
typedef struct
{
    /* switch number represented by this device */
    fm_int                  sw;

    /* this is so the platform knows what kind of device it is */
    fm_switchFamily         family;
    fm_switchModel          model;
    fm_switchVersion        version;

    /* Type of switch FM10000 or SWAG */
    fm_platformSwitchType   switchType;

    /* bypass shunt mode */
    fm_bool                 bypassEnable;

    /* interrupt source */
    fm_uint                 intrSource;

    /* interrupt timeout counter */
    fm_int                  intrTimeoutCnt;

    /* packet handling state */
    fm_packetHandlingState  packetState;

    /* Raw packet socket file descriptor */
    fm_uint32               rawSocket;

    /* Name of raw packet socket interface  */
    fm_char                 ifaceName[IF_NAMESIZE];

    /**************************************************
     * Memory mapping
     **************************************************/
    /* pointers to switch mapped memory */
    volatile fm_uint32 *    switchMem;

    /* access lock for mapped spaces */
    fm_lock                 accessLocks[FM_MAX_PLAT_LOCKS];

    /**************************************************
     * Port mapping
     **************************************************/

    /* This is maximum physical ports */
    fm_int                  maxPorts;

    /* Physical port to logical port mapping */
    fm_int                 *physicalToLogicalPortMap;

    /* Index to the port table of given logical port */
    fm_int                 *lportToPortTableIndex;

    /**************************************************
     * Scheduler port list
     **************************************************/

    fm_schedulerPort       *schedPortList;
    fm_schedulerToken      *schedTokenList;

    /**************************************************
     *  Management
     **************************************************/
    fm_thread               mgmtThread;

    /**************************************************
     * Transceiver management
     **************************************************/
    fm_platXcvrInfo        *xcvrInfo;

    /**************************************************
     *  LED Management
     **************************************************/
    fm_thread               ledThread;
    fm_platLedInfo         *ledInfo;

    /***************************************************
     * WhiteModel info.
     **************************************************/
#ifdef FM_LT_WHITE_MODEL_SUPPORT

    /* Switch interrupt enable/disable */
    fm_bool swInterruptEnable;

    /* Chip model state object */
    void *chipModel;

    /* Packet ID for frames going through CPU port */
    fm_uint32 pktId;

    /* Function pointers for switch specific functions */
    fm_status (*ModelGetPortMap)(fm_platformPort *portMap, fm_int numPorts);
    fm_status (*ModelInitialize)(void **chipModel, fm_int sw, void * funcPtrs);
    fm_status (*ModelReset)(fm_int sw);
    fm_status (*ModelTick)(fm_int sw, fm_uint32 *interrupt);
    fm_status (*ModelWriteCSR)(fm_int sw, fm_uint32 addr, fm_uint32 newValue);
    fm_status (*ModelReadCSR)(fm_int sw, fm_uint32 addr, fm_uint32 *value);
    fm_status (*ModelReadCSR64)(fm_int sw, fm_uint32 addr, fm_uint64 *value);
    fm_status (*ModelReadCSRMult)(fm_int     sw,
                                  fm_uint32  addr,
                                  fm_int     n,
                                  fm_uint32 *value);
    fm_status (*ModelReadCSRMult64)(fm_int     sw,
                                    fm_uint32  addr,
                                    fm_int     n,
                                    fm_uint64 *value);
#endif

} fm_platformState;

typedef fm_platformState    fm_platform_state;

/* platform state which is local to a particular process */
typedef struct
{
    /* File handle to switch device */
    int fd;

    /* File locking handle used when sharing resources with other apps */
    int fileLock;

    /* UIO device information */
    fm_uioDriverInfo uioInfo;

    /* Thread handle for interrupt listener */
    fm_thread intrListener;

    /* Thread handle for raw socket listener */
    fm_thread rawsocketThread;

    /* Optional shared library functions */
    fm_platformLib libFuncs;

} fm_platformProcessState;



/* Root platform structure */
typedef struct _fm_rootPlatform
{
    /**************************************************
     * platform configuration loaded from file
     **************************************************/
    fm_platformCfg          cfg;

    /**************************************************
     * platform
     **************************************************/
    fm_platformState       *platformState;
    fm_bufferAllocState     bufferAllocState;

    fm_int                  switchBootVersion;

    /* switch aggregate information */
    fm_int                  swagNumbers[FM_MAX_SWITCH_AGGREGATES + 1];

    /* Packet Test Interface thread information */
    fm_thread               ptiThread;

    /* Not used but needed by generic packet driver code */
    fm_bool                 dmaEnabled;

    /* The SWAG switch of the platform.
       -1 if the platform does not have a SWAG switch. */
    fm_int                  swagId;

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    /***************************************************
     * WhiteModel info.
     **************************************************/

    /* The global state for the packet queue that is used
       by all model instances. */
    void *packetQueue;

    /* TestPoint support state */
    void *tpState;

    /* TRUE if the platform topology was set using the platform
     * multi-node/multi-switch support. FALSE otherwise. */
    fm_bool topologySet;
#endif

} fm_rootPlatform;



/* Global platform state (cross-process) */
extern fm_rootPlatform *fmRootPlatform;

/* Global platform process state per switch */
extern fm_platformProcessState *fmPlatformProcessState;

/* macro to check if platformState is initialized */
#define IS_PLAT_STATE_INITED   (!fmRootPlatform->platformState)

/* obtain switch memmap pointer */
#define GET_PLAT_MEMMAP_CSR(sw) (fmRootPlatform->platformState[sw].switchMem)

/* obtain platform state */
#define GET_PLAT_STATE(sw)     (&fmRootPlatform->platformState[sw])

/* obtain platform process state */
#define GET_PLAT_PROC_STATE(sw) (&fmPlatformProcessState[sw])

/* obtain interrupt listener thread handle */
#define GET_PLAT_INTR_LISTENER(sw) (&fmPlatformProcessState[sw].intrListener)

/* obtain raw socket listener thread handle */
#define GET_PLAT_RAW_LISTENER(sw) (&fmPlatformProcessState[sw].rawsocketThread)

/* obtain platform packet state */
#define GET_PLAT_PKT_STATE(sw) (&fmRootPlatform->platformState[sw].packetState)

/* macros to deal with the access locks */
#define TAKE_PLAT_LOCK(sw, type) \
    fmCaptureLock( &fmRootPlatform->platformState[(sw)].accessLocks[(type)], FM_WAIT_FOREVER);

#define DROP_PLAT_LOCK(sw, type) \
    fmReleaseLock( &fmRootPlatform->platformState[(sw)].accessLocks[(type)]);

/* access lock to I2C bus muxes */
#define TAKE_PLAT_I2C_BUS_LOCK(sw) TAKE_PLAT_LOCK(sw, FM_PLAT_I2C_BUS);

#define DROP_PLAT_I2C_BUS_LOCK(sw) DROP_PLAT_LOCK(sw, FM_PLAT_I2C_BUS);

#define BYPASS_ADDR_CHECK(addr)  TRUE

/* thread body for interrupt listener */

void *fmPlatformInterruptListener(void *args);


/**************************************************
 * platform_sched.c
 **************************************************/

fm_status fmPlatformSetRingMode(fm_int sw, fm_int mode);
fm_status fmPlatformGetRingMode(fm_int sw, fm_int *mode);

/**************************************************
 * platform_timestamp.c
 **************************************************/

fm_status fmPlatformInitializeClock(fm_int sw, fm_int step);

fm_status fm10000SbusServerStart(fm_int tcpPort);

/**************************************************
 * Required for white model build
 **************************************************/

void fmPlatformCloseInstrumentation(void);
void fmPlatformOpenInstrumentation(void);

/**************************************************
 * platform_api_stubs.c 
 **************************************************/

/* 
 * These functions have default or dummy implementations 
 * that may be used in some platforms. 
 *  
 * Comment out the macro definition to enable the stub; uncomment 
 * it to indicate that you are implementing the function in your 
 * platform layer.
 */
/* #define FM_HAVE_fmPlatformAddGlortToPortMapping */
/* #define FM_HAVE_fmPlatformSetRemoteGlortToLogicalPortMapping */
/* #define FM_HAVE_fmPlatformSetCpuPortVlanTag */
#define FM_HAVE_fmPlatformBypassEnabled
#define FM_HAVE_fmPlatformCreateSWAG
#define FM_HAVE_fmPlatformDeleteSWAG
#define FM_HAVE_fmPlatformGetPortCapabilities
/* #define FM_HAVE_fmPlatformGetPortClockSel */
#define FM_HAVE_fmPlatformGetPortDefaultSettings
#define FM_HAVE_fmPlatformGetSwitchPartNumber
/* #define FM_HAVE_fmPlatformMACMaintenanceSupported */
/* #define FM_HAVE_fmPlatformReceivePackets */
#define FM_HAVE_fmPlatformSendPacket
#define FM_HAVE_fmPlatformSendPacketDirected
#define FM_HAVE_fmPlatformSendPackets
#define FM_HAVE_fmPlatformSendPacketSwitched
/* #define FM_HAVE_fmPlatformSendPacketISL */
#define FM_HAVE_fmPlatformSetBypassMode
/* #define FM_HAVE_fmPlatformSetPortDefaultVlan */
#define FM_HAVE_fmPlatformNotifyPortState
#define FM_HAVE_fmPlatformNotifyPortAttribute
#define FM_HAVE_fmPlatformSWAGInitialize
#define FM_HAVE_fmPlatformSwitchPreInitialize
#define FM_HAVE_fmPlatformSwitchTerminate
#define FM_HAVE_fmPlatformTerminate


/* 
 * These functions are platform-specific, and must be fully
 * implemented for the API to execute. Non-functional stubs 
 * are provided for documentation purposes. 
 */ 
#define FM_HAVE_fmPlatformDisableInterrupt
#define FM_HAVE_fmPlatformEnableInterrupt
#define FM_HAVE_fmPlatformGetInterrupt
#define FM_HAVE_fmPlatformGetSchedulerConfig
#define FM_HAVE_fmPlatformInitialize
#define FM_HAVE_fmPlatformMapLogicalPortToPhysical
#define FM_HAVE_fmPlatformMapPhysicalPortToLogical
#define FM_HAVE_fmPlatformRelease
#define FM_HAVE_fmPlatformReset
#define FM_HAVE_fmPlatformSwitchInitialize
#define FM_HAVE_fmPlatformSwitchPostInitialize
#define FM_HAVE_fmPlatformSwitchInserted
#define FM_HAVE_fmPlatformSwitchPreInsert
#define FM_HAVE_fmPlatformMapPortLaneToEplLane
#define FM_HAVE_fmPlatformGpioInterruptHandler

/**************************************************
 * platform_app_stubs.c 
 **************************************************/

/* 
 * These functions have default or dummy implementations 
 * that may be used in some platforms. 
 *  
 * Comment out the macro definition to enable the stub; uncomment 
 * it to indicate that you are implementing the function in your 
 * platform layer.
 */
/* #define FM_HAVE_fmPlatformMdioRead */
/* #define FM_HAVE_fmPlatformMdioWrite */


#endif /* __FM_PLATFORM_TYPES_H */
