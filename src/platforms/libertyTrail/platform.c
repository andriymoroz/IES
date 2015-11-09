/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform.c
 * Creation Date:   June 2, 2014
 * Description:     Platform specific implementation.
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

#include <fm_sdk_fm10000_int.h>
#include <sys/io.h>
#include <platforms/util/fm_util_device_lock.h>

#include <platforms/util/fm_util.h>
#include <platforms/util/fm10000/fm10000_util_spi.h>

#ifdef FM_LT_WHITE_MODEL_SUPPORT
#include <platforms/common/model/fm10000/fm10000_model_types.h>
#endif


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM10000_MEMORY_SIZE 0x04000000 /* 64MB */

#define MAX_BUF_SIZE        256

#ifdef FM_LT_WHITE_MODEL_SUPPORT
#define PLATFORM_MODEL_TOPOLOGY_FILE    "multi_node_topology.txt"
#endif


#define NVM_NAME                              "NVM"


#define SW_MEM_SIZE         0x04000000


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/* this contains all global (cross-process) platform state */
fm_rootPlatform *fmRootPlatform;

/* Platform process state per switch */
fm_platformProcessState *fmPlatformProcessState;


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static void **desiredMemmapAddr;

static fm_bool masterInstance = FALSE;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/* RoundUp
 *
 * \desc            Takes an address (which must already be page-aligned),
 *                  adds length to it, and rounds up to the next page-aligned
 *                  address.
 *
 * \param[in]       addr is a page-aligned address.
 *
 * \param[in]       length is the number of bytes to add to addr.
 *
 * \return          the page-aligned result.
 *
 *****************************************************************************/
static void *RoundUp(void *addr, fm_uint length)
{
    unsigned char *ptr  = addr;
    fm_uint        mask = getpagesize() - 1;

    length = ( (length - 1) | mask ) + 1;
    ptr   += length;
    return (void *) ptr;

}   /* end RoundUp */




/*****************************************************************************/
/* ChooseMappedMemoryAddresses
 *
 * \desc            Initialize the mappedMemDesiredAddr array.
 *
 * \param           None
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ChooseMappedMemoryAddresses(void)
{
    fm_status status;
    void *    addr;
    fm_int    i;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    /**************************************************
     * We need to map the device memory at fixed
     * locations (instead of letting mmap choose)
     * because pointers to the packet memory might be
     * passed between processes, therefore the addresses
     * must be consistent.
     **************************************************/
    status = fmGetAvailableSharedVirtualBaseAddress(&addr);

    if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    for (i = 0 ; i < FM_PLAT_NUM_SW ; i++)
    {
        desiredMemmapAddr[i] = addr;
        addr = RoundUp(addr, FM10000_MEMORY_SIZE);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end ChooseMappedMemoryAddresses */




/*****************************************************************************/
/** ConnectToDevMem
 *
 * \desc            Open and memory map to /dev/mem.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ConnectToDevMem(fm_int sw)
{
    fm_platformProcessState *pp;
    fm_platformCfgSwitch    *swCfg;
    fm_uint64                memOffset;
    fm_char                  strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t                  strErrNum;
    void *                   addr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    pp = GET_PLAT_PROC_STATE(sw);

    if (pp->fd >= 0)
    {
        /* Already connected to dev/mem */
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    /* The address offset is after the name delimeted by : */
    memOffset = strtoull(swCfg->devMemOffset, NULL, 16);

    if (memOffset == ULLONG_MAX)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Can't decode switch#%d memory offset '%s' - %s\n",
                     sw,
                     swCfg->devMemOffset,
                     strErrBuf);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);        
    }

    if (memOffset == 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Invalid switch#%d memory offset '%s'\n",
                     sw,
                     swCfg->devMemOffset);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL); 
    }

    FM_LOG_PRINT("Switch#%d memory is located at 0x%llx\n", sw, memOffset);

    /* Check IO permissions to be able to open /dev/mem */
    if (iopl(3))
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Cannot get I/O permissions.\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    pp->fd = open ("/dev/mem", O_RDWR);
    if (pp->fd < 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Unable to open /dev/mem\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    /* Memory map the switch memory */
    addr = mmap(desiredMemmapAddr[sw],
                SW_MEM_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                pp->fd,
                memOffset);

    if (addr == MAP_FAILED)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Can't map switch#%d memory - %s\n",
                     sw,
                     strErrBuf);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }
    else if (addr != desiredMemmapAddr[sw])
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Can't map switch#%d memory - "
                     "desired address was %p but got %p\n",
                     sw,
                     desiredMemmapAddr[sw],
                     addr);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    GET_PLAT_STATE(sw)->switchMem = addr;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end ConnectToDevMem */



/*****************************************************************************/
/** DisconnectFromDevMem
 *
 * \desc            Memory unmap and close connection to /dev/mem.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DisconnectFromDevMem(fm_int sw)
{
    fm_platformProcessState *pp;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    pp = GET_PLAT_PROC_STATE(sw);

    if (pp->fd >= 0)
    {
        /* Memory unmap for the switch memory */
        munmap((void *)GET_PLAT_STATE(sw)->switchMem, SW_MEM_SIZE);

        /* Close connection */
        close(pp->fd);
        pp->fd = -1;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end DisconnectFromDevMem */




/*****************************************************************************/
/** ConnectToHostDriver
 *
 * \desc            Open and memory map to the host driver.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ConnectToHostDriver(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_status             status;
    fm_text               uioDevName;
    fm_text               netDevName;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    if (strcmp(swCfg->uioDevName, FM_AAD_API_PLATFORM_UIO_DEV_NAME) != 0)
    {
        uioDevName = swCfg->uioDevName;
    }
    else
    {
        uioDevName = NULL;
    }

    if (strcmp(swCfg->netDevName, FM_AAD_API_PLATFORM_NETDEV_NAME) != 0)
    {
        netDevName = swCfg->netDevName;
    }
    else
    {
        netDevName = NULL;
    }

    status = fmPlatformHostDrvOpen(sw,
                                   swCfg->mgmtPep, 
                                   netDevName, 
                                   uioDevName, 
                                   desiredMemmapAddr[sw]);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end ConnectToHostDriver */




/*****************************************************************************/
/** DisconnectFromHostDriver
 *
 * \desc            Memory unmap and close connection to host driver.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DisconnectFromHostDriver(fm_int sw)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    status = fmPlatformHostDrvClose(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end DisconnectFromHostDriver */




/*****************************************************************************/
/** ConnectToPCIE
 *
 * \desc            Open and memory map to the host driver or /dev/mem.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ConnectToPCIE(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    if (strcmp(swCfg->devMemOffset, FM_AAD_API_PLATFORM_DEVMEM_OFFSET) != 0)
    {
        status = ConnectToDevMem(sw);
    }
    else
    {
        status = ConnectToHostDriver(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end ConnectToPCIE */




/*****************************************************************************/
/** DisconnectFromPCIE
 *
 * \desc            Memory unmap and close connection to host driver or /dev/mem.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DisconnectFromPCIE(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    if (strcmp(swCfg->devMemOffset, FM_AAD_API_PLATFORM_DEVMEM_OFFSET) != 0)
    {
        status = DisconnectFromDevMem(sw);
    }
    else
    {
        status = DisconnectFromHostDriver(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end DisconnectFromPCIE */



/*****************************************************************************/
/* SetRegAccessMode
 *
 * \desc            Set the register access mode (EBI/PCIe/I2C)
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       mode is the register access mode
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the switchPtr is NULL
 *
 *****************************************************************************/ 
static fm_status SetRegAccessMode(fm_int sw, fm_int mode)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d, mode = %d\n", sw, mode);

    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    status = FM_OK;

    switch (mode)
    {
            case FM_PLAT_REG_ACCESS_PCIE:
            switchPtr->WriteUINT32       = fmPlatformWriteCSR;
            switchPtr->ReadUINT32        = fmPlatformReadCSR;
            switchPtr->MaskUINT32        = fmPlatformMaskCSR;
            switchPtr->WriteUINT32Mult   = fmPlatformWriteCSRMult;
            switchPtr->ReadUINT32Mult    = fmPlatformReadCSRMult;
            switchPtr->WriteUINT64       = fmPlatformWriteCSR64;
            switchPtr->ReadUINT64        = fmPlatformReadCSR64;
            switchPtr->WriteUINT64Mult   = fmPlatformWriteCSRMult64;
            switchPtr->ReadUINT64Mult    = fmPlatformReadCSRMult64;
            switchPtr->WriteRawUINT32    = fmPlatformWriteRawCSR;
            switchPtr->WriteRawUINT32Seq = fmPlatformWriteRawCSRSeq;
            switchPtr->ReadRawUINT32     = fmPlatformReadRawCSR;
            switchPtr->ReadEgressFid     = fmPlatformReadCSR;
            switchPtr->ReadIngressFid    = fmPlatformReadCSR64;
            break;

        case FM_PLAT_REG_ACCESS_I2C:
            switchPtr->WriteUINT32       = fmPlatformI2cWriteCSR;
            switchPtr->ReadUINT32        = fmPlatformI2cReadCSR;
            switchPtr->MaskUINT32        = fmPlatformI2cMaskCSR;
            switchPtr->WriteUINT32Mult   = fmPlatformI2cWriteCSRMult;
            switchPtr->ReadUINT32Mult    = fmPlatformI2cReadCSRMult;
            switchPtr->WriteUINT64       = fmPlatformI2cWriteCSR64;
            switchPtr->ReadUINT64        = fmPlatformI2cReadCSR64;
            switchPtr->WriteUINT64Mult   = fmPlatformI2cWriteCSRMult64;
            switchPtr->ReadUINT64Mult    = fmPlatformI2cReadCSRMult64;
            switchPtr->WriteRawUINT32    = NULL;
            switchPtr->WriteRawUINT32Seq = NULL;
            switchPtr->ReadRawUINT32     = NULL;
            switchPtr->ReadEgressFid     = fmPlatformI2cReadCSR;
            switchPtr->ReadIngressFid    = fmPlatformI2cReadCSR64;
            break;

        case FM_PLAT_REG_ACCESS_EBI:
            switchPtr->WriteUINT32       = fmPlatformEbiWriteCSR;
            switchPtr->ReadUINT32        = fmPlatformEbiReadCSR;
            switchPtr->MaskUINT32        = fmPlatformEbiMaskCSR;
            switchPtr->WriteUINT32Mult   = fmPlatformEbiWriteCSRMult;
            switchPtr->ReadUINT32Mult    = fmPlatformEbiReadCSRMult;
            switchPtr->WriteUINT64       = fmPlatformEbiWriteCSR64;
            switchPtr->ReadUINT64        = fmPlatformEbiReadCSR64;
            switchPtr->WriteUINT64Mult   = fmPlatformEbiWriteCSRMult64;
            switchPtr->ReadUINT64Mult    = fmPlatformEbiReadCSRMult64;
            switchPtr->WriteRawUINT32    = fmPlatformEbiWriteRawCSR;
            switchPtr->WriteRawUINT32Seq = fmPlatformEbiWriteRawCSRSeq;
            switchPtr->ReadRawUINT32     = fmPlatformEbiReadRawCSR;
            switchPtr->ReadEgressFid     = fmPlatformEbiReadCSR;
            switchPtr->ReadIngressFid    = fmPlatformEbiReadCSR64;
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end SetRegAccessMode */




/*****************************************************************************/
/* FileLockingInit
 *
 * \desc            Determine if file locking is required.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/ 
static fm_status FileLockingInit(fm_int sw)
{
    int     fd;
    fm_text filename;

    GET_PLAT_PROC_STATE(sw)->fileLock = -1;

    filename = (FM_PLAT_GET_CFG)->fileLockName;

    if (strlen(filename) > 0)
    {
        fd = open(filename, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
        if (fd < 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Unable to open filelock name: [%s]\n",
                         filename);

            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
        }

        GET_PLAT_PROC_STATE(sw)->fileLock = fd;
    }

    return FM_OK;

}   /* end FileLockingInit */



/*****************************************************************************/
/* SetI2cBusSpeed
 *
 * \desc            Set the I2C bus speed.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/ 
static fm_status SetI2cBusSpeed(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_status   status;
    fm_uint32   regValue;

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

#ifdef FM_SUPPORT_SWAG
    /* No need to do anything here for SWAG switch */
    if (swCfg->switchRole == FM_SWITCH_ROLE_SWAG)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }
#endif

    /* set the I2C bus speed */
    status = fmReadUINT32(sw, FM10000_I2C_CFG(), &regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    FM_SET_FIELD(regValue, FM10000_I2C_CFG, Divider,  swCfg->i2cClkDivider);
    status = fmWriteUINT32(sw, FM10000_I2C_CFG(), regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    return status;

}   /* end SetI2cBusSpeed */


/*****************************************************************************/
/* ResetI2cDevices
 *
 * \desc            Perform a reset sequence of the I2C devices attached
 *                  to the siwtch I2C bus.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/ 
static fm_status ResetI2cDevices(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_status             status;

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    if ( swCfg->gpioI2cReset != FM_PLAT_UNDEFINED )
    {
        /* There is a GPIO used for I2C reset.
           Configure it as output and apply the reset (0) */
        status = fmPlatformGpioSetDirection(sw,
                                            swCfg->gpioI2cReset,
                                            FM_PLAT_GPIO_DIR_OUTPUT,
                                            0);

        /* Wait 1 msec (PCA spec specifies reset pulse width = 4 ns and
           and reset time = 100 ns) */
        fmDelayBy(0,1000000);

        /* De-assert (1) the reset signal */
        status = fmPlatformGpioSetValue(sw, swCfg->gpioI2cReset, 1);

        FM_LOG_PRINT("\nPerform I2C reset through FM10000 GPIO %d\n", 
                     swCfg->gpioI2cReset);
    }
    else
    {
        status = FM_OK;
    }

    return status;

}   /* end ResetI2cDevices */




/*****************************************************************************/
/* SetVoltageScaling
 *
 * \desc            Set the switch voltage regulator module with the nominal
 *                  VDDF and VDDS supply voltages programmed into the on-chip
 *                  fuse box.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/ 
static fm_status SetVoltageScaling(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_platformLib *      libFunc;
    fm_int                vddsResId;
    fm_int                vddfResId;
    fm_uint32             vdds;
    fm_uint32             vddf;
    fm_bool               defVal;
    fm_status             status;

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    vddsResId = swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDS];
    vddfResId = swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDF];

    if ( libFunc->SetVrmVoltage && ( vddsResId != -1 || vddfResId != -1 ) )
    {
        status = fmPlatformGetNominalSwitchVoltages(sw, &vdds, &vddf, &defVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Set the VRM if the fuse box is programmed with voltage scaling
           values (!defVal), otherwise set the defaut values per the
           useDefVoltages configuration. */
        if ( !defVal || swCfg->vrm.useDefVoltages )
        {
            /* If one of VRM resource ID is -1, that probably means both the
               VDDS and VDDF supplies are joined at the board level and
               supplied from a single source. In that case the highest value of
               VDDS or VDDF is used. */
            if ( vddsResId == -1 || vddfResId == -1 )
            {
                vdds = (vdds >= vddf) ? vdds : vddf;
                vddf = vdds;
            }

            if ( vddsResId != -1 )
            {
                /* Set VDDS */
                status = fmPlatformSetVrmVoltage(sw, FM_PLAT_VRM_VDDS, vdds);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }

            if ( vddfResId != -1 )
            {
                /* Set VDDF */
                status = fmPlatformSetVrmVoltage(sw, FM_PLAT_VRM_VDDF, vddf);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }
        }
    }
    else
    {
        status = FM_OK;
    }

ABORT:
    return status;

}   /* end SetVoltageScaling */




/*****************************************************************************/
/* PlatformProcessInitialize
 *
 * \desc            Platform initialization that needs to be done in each
 *                  process (e. g. open the device and map the memory.)
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_OPENING_DEVICE_NODE if device can't be opened.
 * \return          FM_FAIL if the memory can't be mapped.
 *
 *****************************************************************************/
static fm_status PlatformProcessInitialize(void)
{
    fm_status status;
    fm_int    numSwitches;
    fm_int    sw;
    fm_platformCfg *platCfg;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    numSwitches = FM_PLAT_NUM_SW;

    /* Allocate and initialize fm_platformProcessState */
    fmPlatformProcessState = fmAlloc( numSwitches * sizeof(fm_platformProcessState) );

    if (fmPlatformProcessState == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    desiredMemmapAddr = fmAlloc( numSwitches * sizeof(void *) );

    if (desiredMemmapAddr == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    status = ChooseMappedMemoryAddresses();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    memset( fmPlatformProcessState,
            0, 
            numSwitches * sizeof(fm_platformProcessState) );

    for (sw = 0 ; sw < FM_PLAT_NUM_SW ; sw++)
    {
        /* Init to -1 to indicate not connected to host driver */
        fmPlatformProcessState[sw].fd = -1;
    }

    platCfg = FM_PLAT_GET_CFG;

    if (strcmp(platCfg->ebiDevName, FM_AAD_API_PLATFORM_EBI_DEV_NAME) != 0)
    {
        /* Should this be called per switch or one for all switches */
        status = fmPlatformEbiInit(0, platCfg->ebiDevName);
    }

     FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (fmPlatformProcessState != NULL)
    {
        fmFree(fmPlatformProcessState);
        fmPlatformProcessState = NULL;
    }

    if (desiredMemmapAddr != NULL)
    {
        fmFree(desiredMemmapAddr);
        desiredMemmapAddr = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end PlatformProcessInitialize */





/*****************************************************************************/
/* fmPlatformSendSwitchEvent
 *
 * \desc            Sends a switch inserted/removed event.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       eventType is the event type.
 *
 * \return          status code.
 *
 *****************************************************************************/
static fm_status fmPlatformSendSwitchEvent(fm_int sw, fm_int eventType)
{
    fm_status               status = FM_OK;
    fm_event *              event;
    fm_eventSwitchInserted *insert;
    fm_eventSwitchRemoved * remove;


    /* Allocate an event buffer.  This is priority high because we don't
     * want to be throttled
     */
    event = fmAllocateEvent(sw,
                            FM_EVID_SYSTEM,
                            eventType,
                            FM_EVENT_PRIORITY_HIGH);

    if (!event)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unable to allocate event for switch insertion/removal\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }


    /* Populate the event structure */
    switch (eventType)
    {
        case FM_EVENT_SWITCH_INSERTED:
            insert        = &event->info.fpSwitchInsertedEvent;

            insert->model         = -1;
            insert->slot          = sw;
            insert->family  = GET_PLAT_STATE(sw)->family;
            insert->model   = GET_PLAT_STATE(sw)->model;
            insert->version = GET_PLAT_STATE(sw)->version;
            break;

        case FM_EVENT_SWITCH_REMOVED:
            remove       = &event->info.fpSwitchRemovedEvent;
            remove->slot = sw;
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            break;

    }   /* end switch (eventType) */

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Send the event */
    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSendSwitchEvent */




/*****************************************************************************/
/* NotifyInterrupt
 *
 * \desc            Notify API interrupt thread.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       intrStatus is the current interrupt status. It is used
 *                  along with msg for logging purpose.
 *
 * \param[in]       msg is a message to display.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void NotifyInterrupt(fm_int sw, fm_uint intrStatus, fm_text msg)
{
    fm_platformState *ps;


    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                 "%s: Got interrupt ! (intrStatus %d)\n",
                 msg, intrStatus);

    ps = GET_PLAT_STATE(sw);


    TAKE_PLAT_LOCK(ps->sw, FM_PLAT_INFO);

    ps->intrSource |= FM_INTERRUPT_SOURCE_ISR;

    DROP_PLAT_LOCK(ps->sw, FM_PLAT_INFO);

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                  "Signaling semaphore\n");

    fmSignalSemaphore(&fmRootApi->intrAvail);

}   /* end NotifyInterrupt */




/*****************************************************************************/
/* CheckInterrupt
 *
 * \desc            Check if there is a interrupt pending.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       msg is a message to display.
 *
 * \return          NONE.
 *
 * *****************************************************************************/
static fm_uint32 CheckInterrupt(fm_int sw, fm_text msg)
{
    fm_switch *      switchPtr;
    fm10000_switch * switchExt;
    fm_uint32        intrStatus;


    PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    /* Switch not up Yet... */
    if (switchPtr == NULL)
    {
        UNPROTECT_SWITCH(sw);
        return 1;
    }

    switchExt = switchPtr->extension;

    if (fm10000PollInterrupt(sw,
                             switchExt->interruptMaskValue,
                             &intrStatus,
                             GET_SWITCH_PTR(sw)->ReadUINT32))
    {
        intrStatus = 0;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Check for pending interrupt\n");
    if (intrStatus)
    {
        NotifyInterrupt(sw, intrStatus, msg);
    }

    return intrStatus;

}   /* end CheckInterrupt */



#ifdef FM_SUPPORT_SWAG

/*****************************************************************************/
/* SendSwitchEvent
 *
 * \desc            Sends a switch inserted/removed event.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       eventType is the event type.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SendSwitchEvent(fm_int sw, fm_int eventType)
{
    fm_status               err;
    fm_event *              event;
    fm_eventSwitchInserted *insert;
    fm_eventSwitchRemoved * remove;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d, eventType=%d\n", sw, eventType);

    err = FM_OK;

    /* Allocate an event buffer.  This is priority high because we don't
     * want to be throttled
     */
    event = fmAllocateEvent(sw, FM_EVID_SYSTEM, eventType, FM_EVENT_PRIORITY_HIGH);

    if (event == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unable to allocate event for switch insertion/removal\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }


    /* Populate the event structure */
    switch (eventType)
    {
        case FM_EVENT_SWITCH_INSERTED:
            insert        = &event->info.fpSwitchInsertedEvent;
            insert->model = -1;
            insert->slot  = sw;
            break;

        case FM_EVENT_SWITCH_REMOVED:
            remove       = &event->info.fpSwitchRemovedEvent;
            remove->slot = sw;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;
    }

    /* Send the event */
    fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end SendSwitchEvent */




/*****************************************************************************/
/* PlatformInitMultiSwitchTopology
 *
 * \desc            Function to determine which multi-node/multi-switch
 *                  topology to use and configure the white model
 *                  packet queue appropriately. It will also build the
 *                  necessary switch-aggregate structures needed to allow
 *                  for SWAG initialization, if SWAG support was requested.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PlatformInitMultiSwitchTopology(void)
{
    fm_int                sw;
    fm_int                swagId;
    fm_status             err;
    fm_platformCfg *      platCfg;
    fm_platformCfgSwitch *swCfg;
#ifdef FM_LT_WHITE_MODEL_SUPPORT
    FILE *                fp;
    fm_int                i;
    fm_int                j;
    fm_bool               duplicate;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "no args\n");

    /* Default to no SWAG switch. */
    fmRootPlatform->swagId = -1;

    platCfg = FM_PLAT_GET_CFG;

    if ( (platCfg->topology != FM_SWAG_TOPOLOGY_FAT_TREE) &&
         (platCfg->topology != FM_SWAG_TOPOLOGY_MESH) )
    {
        /* Unsupported topology */
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    /*************************************************************************
     * Generate the model topology file.
    **************************************************************************/
    fp = fopen(PLATFORM_MODEL_TOPOLOGY_FILE, "w");

    if (fp == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    for (sw = 0 ; sw < FM_PLAT_NUM_SW ; sw++)
    {
        swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
        if (swCfg->switchRole == FM_SWITCH_ROLE_SWAG)
        {
            break;
        }
    }

    if (sw == FM_PLAT_NUM_SW)
    {
        fclose(fp);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_SWITCH_AGGREGATE);
    }

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    for (i = 0 ; i < swCfg->numPorts ; i++)
    {
        if (swCfg->ports[i].swagLink.type == FM_SWAG_LINK_INTERNAL)
        {
            duplicate = FALSE;

            for (j = 0 ; j < i ; j++)
            {
                if (swCfg->ports[j].swagLink.type == FM_SWAG_LINK_INTERNAL)
                {
                    if ( (swCfg->ports[j].swagLink.swId == swCfg->ports[i].swagLink.partnerSwitch) &&
                         (swCfg->ports[j].swagLink.swPort == swCfg->ports[i].swagLink.partnerPort) )
                    {
                        duplicate = TRUE;
                    }
                    else if ( (swCfg->ports[j].swagLink.partnerSwitch == swCfg->ports[i].swagLink.swId) &&
                              (swCfg->ports[j].swagLink.partnerPort == swCfg->ports[i].swagLink.swPort) )
                    {
                        duplicate = TRUE;
                    }
                }
            }
            if (!duplicate)
            {
                fprintf(fp,
                        "%d,%d,%d,%d,%d,%d\n",
                        sw,
                        swCfg->ports[i].swagLink.swId,
                        swCfg->ports[i].swagLink.swPort,
                        sw,
                        swCfg->ports[i].swagLink.partnerSwitch,
                        swCfg->ports[i].swagLink.partnerPort);
            }
        }
    }

    fclose(fp);

    /* Set the topologySet flag. This confirms that we have built the
     * model topology file based upon the requested topology. This will
     * cause the FM_PLATFORM_ATTR_TOPOLOGY processing to ignore the
     * topology file provided by the user and use the file we just built
     * instead. */
    fmRootPlatform->topologySet = TRUE;

#endif

    for (sw = 0 ; sw < FM_PLAT_NUM_SW ; sw++)
    {
        swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

        if (swCfg->switchRole == FM_SWITCH_ROLE_SWAG)
        {
            /* Create a switch aggregate for the unit */
            err = fmCreateSWAG(platCfg->topology,
                               NULL,
                               &swagId);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

            err = SendSwitchEvent(swagId, FM_EVENT_SWITCH_INSERTED);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end PlatformInitMultiSwitchTopology */

#endif  /* end FM_SUPPORT_SWAG */




/*****************************************************************************/
/** LoadPropertiesFromFile
 * \ingroup intPlatform
 *
 * \desc            Loads attributes into the attribute subsystem by calling
 *                  the set and get methods on attributes read in from a text
 *                  file.
 *                                                                      \lb\lb
 *                  The expected file format for the database is as follows:
 *                                                                      \lb\lb
 *                  [key] [type] [value]
 *                                                                      \lb\lb
 *                  Where key is a dotted string, type is one of int, bool or
 *                  float, and value is the value to set. Space is the only
 *                  valid separator, but multiple spaces are allowed.
 *
 * \param[in]       fileName is the full path to a text file to load.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status LoadPropertiesFromFile(fm_text fileName)
{
    fm_status status;
    FILE     *fp;
    fm_int    lineNo = 0;
    fm_char   line[FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN];
    fm_int    numAttr = 0;
    fm_int    strLen;
    fm_int    cnt;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "fileName=%s\n", fileName);

    fp = fopen(fileName, "rt");

    if (!fp)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                     "Unable to open attribute database %s\n",
                     fileName);

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    while ( fgets(line, FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN, fp) )
    {
        lineNo++;

        /* check for comment line */
        if (line[0] == '#')
        {
            continue;
        }

        strLen = strlen(line);
        if (strLen <= 1)
        {
            continue;
        }

        /* blank line */
        for (cnt = 0 ; cnt < strLen ; cnt++)
        {
            if (!isspace(line[cnt]))
            {
                break;
            }
        }
        if (cnt == strLen)
        {
            continue;
        }

        status = fmPlatformLoadPropertiesFromLine(line);
        if (status == FM_OK)
        {
            numAttr++;
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Error reading from line %d\n",
                         status);
        }

    }   /* end while ( fgets(line, FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN, fp) ) */

    fclose(fp);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Loaded %d attributes from %s\n",
                 numAttr, fileName);

    if (FM_PLAT_GET_CFG->debug & CFG_DBG_CONFIG)
    {
        fmPlatformCfgDump();
    }

    fmPlatformCfgVerifyAndUpdate();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end LoadPropertiesFromFile */





/*****************************************************************************/
/* fmPlatformRootInit
 *
 * \desc            Platform initialization that is only done once for all
 *                  processes.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation error.
 * \return          FM_ERR_UNINITIALIZED if ALOS not intialized.
 * \return          FM_ERR_INVALID_ARGUMENT if lck is NULL.
 * \return          FM_ERR_LOCK_INIT if unable to initialize lock.
 * \return          FM_ERR_CANT_IDENTIFY_SWITCH if unable to identify switch.
 *
 *****************************************************************************/
static fm_status fmPlatformRootInit(void)
{
    fm_status             status = FM_OK;
    fm_int                sw;
    fm_platformState *    ps;
    FILE                 *fp;
    fm_int                i;
    fm_int                size;
    fm_char               objName[40];
    fm_voidptr            packetBufferPool;
    fm_text               attrFile;
    fm_text               attrFile2;
    fm_char               nvmHeaderName[FM_UIO_MAX_NAME_SIZE];

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    /* Allocate and initialize fm_rootPlatform */
    fmRootPlatform = fmAlloc( sizeof(fm_rootPlatform) );

    if (fmRootPlatform == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }

    memset( fmRootPlatform, 0, sizeof(fm_rootPlatform) );

    status = fmPlatformCfgInit();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* load attributes from standard file */
    LoadPropertiesFromFile("fm_api_attributes.cfg");

    /* Loading platform configuration, higher priority than api */
    attrFile = getenv("FM_LIBERTY_TRAIL_CONFIG_FILE");

    if (attrFile != NULL && strlen(attrFile) > 0)
    {
        FM_LOG_PRINT("Loading %s\n", attrFile);

        FM_STRNCPY_S(nvmHeaderName,
                     sizeof(nvmHeaderName),
                     NVM_NAME,
                     sizeof(NVM_NAME));
        FM_STRNCAT_S(nvmHeaderName,
                     sizeof(nvmHeaderName),
                     "-",
                     sizeof("-"));
                     
        if (strcmp(attrFile, NVM_NAME) == 0)
        {
            status = fmPlatformLoadPropertiesFromNVM(NULL);
        }
        else if (strncmp(attrFile,
                         nvmHeaderName,
                         FM_STRNLEN_S(nvmHeaderName, sizeof(nvmHeaderName))) == 0)
        {
            status =
                fmPlatformLoadPropertiesFromNVM(attrFile +
                                                FM_STRNLEN_S(nvmHeaderName,
                                                             sizeof(nvmHeaderName)));
        }
        else
        {
            fp = fopen(attrFile, "rt");

            if (!fp)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                             "Unable to open file %s\n",
                             attrFile);

                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
            }

            size = fread(objName, 1, 2, fp);
            fclose(fp);

            if (!(isprint(objName[0]) || isspace(objName[0])) || 
                !(isprint(objName[1]) || isspace(objName[1])))
            {
                /* File is binary, so load TLV */
                status = fmPlatformLoadTlvFile(attrFile);
            }
            else
            {
                status = LoadPropertiesFromFile(attrFile);
            }
        }
    }
    else
    {
        /* Use the default platform configuration file */
        attrFile = "fm_platform_attributes.cfg";
        FM_LOG_PRINT("Loading %s\n", attrFile);

        status = LoadPropertiesFromFile(attrFile);

        if (status)
        {
            /* Try one level up for TestPoint */
            attrFile2 = "../fm_platform_attributes.cfg";
            FM_LOG_INFO(FM_LOG_CAT_PLATFORM,
                        "Unable to load file '%s'. Trying '%s'.\n",
                        attrFile,
                        attrFile2);
            status = LoadPropertiesFromFile(attrFile2);
        }
    }

    if (status)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to load platform configuration file '%s'.\n",
                     attrFile);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    /* Load attributes from custom file.
       TestPoint sets it to local_attributes.cfg */
    attrFile = getenv("FM_API_ATTR_FILE");

    if (attrFile != NULL)
    {
        status = LoadPropertiesFromFile(attrFile);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    /* Allocate and initialize fm_platformState */
    size = FM_PLAT_NUM_SW * sizeof(fm_platformState);
    fmRootPlatform->platformState = fmAlloc(size);

    if (fmRootPlatform->platformState == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }

    memset(fmRootPlatform->platformState, 0, size);

    /* Allocate and initialize buffer memory */
    packetBufferPool = fmAlloc(FM_BUFFER_SIZE_BYTES * FM_NUM_BUFFERS);

    if (packetBufferPool == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }

    status = fmPlatformInitBuffers(packetBufferPool);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Init per process data for first process here */
    status = PlatformProcessInitialize();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /***************************************************
     * Set up initial attribute values for this platform.
     **************************************************/

    for (i = 0 ; i < FM_MAX_SWITCH_AGGREGATES + 1 ; i++)
    {
        fmRootPlatform->swagNumbers[i] = -1;
    }

    /* Fetches the attribute api.platform.model.switchType and fills in the
     * switchType in platform_state */
#ifdef FM_LT_WHITE_MODEL_SUPPORT
    status = fmPlatformModelGetSwitchTypes();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
#endif

#ifdef FM_SUPPORT_SWAG
    status = PlatformInitMultiSwitchTopology();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
#endif

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    status = fmPlatformModelInitPacketQueue(FM_FIRST_FOCALPOINT);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
#endif

    /* Load and initialize the shared-lib for the first switch only. */
    status = fmPlatformLibLoad(0);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    for (sw = FM_FIRST_FOCALPOINT ; sw < fmRootPlatform->cfg.numSwitches ; sw++)
    {
        status = FileLockingInit(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        ps = GET_PLAT_STATE(sw);
        ps->sw = sw;
        ps->maxPorts = FM10000_MAX_PORT + 1;

        /* Initialize the port lookup tables. */
        status = fmPlatformInitPortTables(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Allocate manual scheduler mode token list */
        status = fmPlatformAllocateSchedulerResources(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Intialize port transceiver and PHY mgmt */
        status = fmPlatformMgmtInit(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Intialize LED handling */
        status = fmPlatformLedInit(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Indicate that the mmap hasn't been done yet for the PCIE region */
        ps->switchMem = NULL;

        /* initialize locks */
        for (i = 0 ; i < FM_MAX_PLAT_LOCKS ; i++)
        {
            FM_SNPRINTF_S(objName, sizeof(objName), "PlatformAccess%d", sw);

            if (i == FM_MEM_TYPE_CSR)
            {
                status = fmCreateLockV2("Platform Register Access",
                                        sw,
                                        FM_LOCK_PREC_PLATFORM,
                                        &ps->accessLocks[i]);
            }
            else
            {
                status = fmCreateLock(objName, &ps->accessLocks[i]);
            }

            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        ps->intrSource = FM_INTERRUPT_SOURCE_NONE;

        /* load bypass mode */
        ps->bypassEnable = GET_PROPERTY()->byPassEnable;

        if (ps->bypassEnable)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Enabling HA bypass mode\n");
        }

#ifdef FM_SUPPORT_SWAG
        if (ps->switchType == FM_PLATFORM_SWITCH_TYPE_SWAG)
        {
            continue;
        }
#endif

#ifdef FM_LT_WHITE_MODEL_SUPPORT
        status = fmPlatformModelInitSwitch(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
#else

        if (GET_PROPERTY()->autoInsertSwitches)
        {
            /***************************************************
             * The switch is locally present (hard-wired) on
             * the board, so insert it immediately.
             **************************************************/

            status = fmPlatformSwitchInsert(sw);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

#endif /* #else FM_LT_WHITE_MODEL_SUPPORT */

    }   /* for (sw = 0 ; sw < FM_PLAT_NUM_SW ; sw++) */

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformRootInit */




/*****************************************************************************/
/* PlatformSwitchPostInitialize
 *
 * \desc            Called after initializing a switch device. This
 *                  function is responsible for initializing any aspect
 *                  of the platform required to occur after the device
 *                  has already been booted.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status PlatformSwitchPostInitialize(fm_int sw)
{
    fm_status  status;
    fm_text    pktIface;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Send the default port configuration read from
     * the platform configuration file.
     **************************************************/
    status = fmPlatformPortInitialize(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    pktIface = GET_PROPERTY()->pktInterface;

    if (!strcmp(pktIface, "pti"))
    {
        switchPtr->SendPackets = fm10000PTISendPackets;

        /* Initialize PTI packet transfer now that the switch is up. 
         * The packet should not have an FCS, it will be added and 
         * calculated by PTI */
        status = fm10000PTIInitialize(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

#if 0
    if (GET_PROPERTY()->enableRefClock)
    {
        step   = GET_FM10000_PROPERTY()->systimeStep;

        status = fmPlatformInitializeClock(sw, step);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
#endif
 
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end PlatformSwitchPostInitialize */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


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
    fm_status             status;
    fm_int                sw;
    fm_platformCfgSwitch *swCfg;
    fm_bool               isFirstProcess;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "nSwitches = %p\n", (void *) nSwitches);

    /* no switches found yet */
    *nSwitches = 0;

#if INSTRUMENT_LOG_LEVEL > 0
    fmPlatformOpenInstrumentation();
#endif

    status = fmGetRoot("platform-libertyTrail",
                       (void **) &fmRootPlatform,
                       fmPlatformRootInit);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    *nSwitches = FM_PLAT_NUM_SW;

    isFirstProcess =
        (FM_DLL_GET_FIRST( (&fmRootApi->localDeliveryThreads), head ) == NULL);

    /* The first process has been handled in RootInit */
    if (!isFirstProcess)
    {
        PlatformProcessInitialize();
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Load and initialize the shared-lib for the first switch only. */
        status = fmPlatformLibLoad(0);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Perform per switch operations */
        for (sw = FM_FIRST_FOCALPOINT ; sw < fmRootPlatform->cfg.numSwitches ; sw++)
        {
            if ( (swCfg = FM_PLAT_GET_SWITCH_CFG(sw)) == NULL )
            {
                continue;
            }

            status = FileLockingInit(sw);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Having family different than UNKNOWN indicates the switch
               has been identified so it is inserted. */
            if (GET_PLAT_STATE(sw)->family != FM_SWITCH_FAMILY_UNKNOWN)
            {
                /* That switch is already inserted */
                if (swCfg->regAccess == FM_PLAT_REG_ACCESS_PCIE)
                {
                    /* Hook up to the host kernel driver or /dev/mem */
                    status = ConnectToPCIE(sw);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
                }
            }
        }
    }
    else
    {
        /* GET_PLAT_PROC_STATE(FM_FIRST_FOCALPOINT)->fd) is not yet initialized here */
        masterInstance = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformInitialize */




/*****************************************************************************/
/* fmPlatformSwitchPreInsert
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
    fm_platformCfgSwitch *     swCfg;
    fm_status                  status = FM_OK;
#ifndef FM_LT_WHITE_MODEL_SUPPORT
    fm_platformState *         ps;
    fm_registerReadUINT32Func  readFunc;
    fm_registerWriteUINT32Func writeFunc;
    fm_bool                    swIsr;
    fm_uint32                  pcieIsrMask;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    if (swCfg == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

#ifdef FM_SUPPORT_SWAG
    /* No need to do anything here for SWAG switch */
    if (swCfg->switchRole == FM_SWITCH_ROLE_SWAG)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }
#endif

#ifndef FM_LT_WHITE_MODEL_SUPPORT

    swIsr = (swCfg->pcieISR == FM_PLAT_PCIE_ISR_SW) ? TRUE : FALSE;

    switch (swCfg->bootMode)
    {
        case FM_PLAT_BOOT_MODE_SPI:
            /* The cold init sequence has been done from the SPI flash (NVM) */
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "Boot mode set to SPI\n");
            break;

        case FM_PLAT_BOOT_MODE_EBI:
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "Boot mode set to EBI\n");
            status = fm10000ColdResetInit(sw,
                                          &swCfg->bootCfg,
                                          swIsr,
                                          fmPlatformEbiReadRawCSR,
                                          fmPlatformEbiWriteRawCSR);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            break;

        case FM_PLAT_BOOT_MODE_I2C:         
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "Boot mode set to I2C\n");
            status = fm10000ColdResetInit(sw,
                                          &swCfg->bootCfg,
                                          swIsr,
                                          fmPlatformI2cReadPreBootCSR,
                                          fmPlatformI2cWritePreBootCSR);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid boot mode provided by property %s\n",
                         FM_AAK_API_PLATFORM_BOOT_MODE);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            break;
    }

    switch (swCfg->regAccess)
    {
        case FM_PLAT_REG_ACCESS_PCIE:
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "Register access mode set to PCIE\n");

            /* Hook up to the host kernel driver or /dev/mem */
            status = ConnectToPCIE(sw);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            readFunc  = fmPlatformReadRawCSR;
            writeFunc = fmPlatformWriteRawCSR;
            break;

        case FM_PLAT_REG_ACCESS_EBI:
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "Register access mode set to EBI\n");
            readFunc  = fmPlatformEbiReadRawCSR;
            writeFunc = fmPlatformEbiWriteRawCSR;
            break;

        case FM_PLAT_REG_ACCESS_I2C:
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "Register access mode set to I2C\n");
            readFunc  = fmPlatformI2cReadCSR;
            writeFunc = fmPlatformI2cWriteCSR;
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid reg access mode provided by property %s\n",
                         FM_AAK_API_PLATFORM_REGISTER_ACCESS);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            break;
    }

    if (masterInstance)
    {
        /* Check for multiple master instance running using the same device.
         * We only prevent if multiple masters are accessing the same uio device.
         * There could be multiple switch in the system so we will allow
         * different master instance running on different uio device.
         * However this will not protect against multiple master instance
         * running on different uio device on the same chip.
         */
        if (GET_PLAT_PROC_STATE(FM_FIRST_FOCALPOINT)->fd > 0)
        {
            if (fmUtilDeviceLockIsTaken(GET_PLAT_PROC_STATE(FM_FIRST_FOCALPOINT)->fd))
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                             "Access to switch device is locked by another process.\n");
                exit(0);
            }
            /* Master process take lock to avoid another master instance running again */
            status = fmUtilDeviceLockTake(GET_PLAT_PROC_STATE(FM_FIRST_FOCALPOINT)->fd);
        }
    }

    /* Default net dev is not set */
    if (strcmp(swCfg->netDevName, FM_AAD_API_PLATFORM_NETDEV_NAME) != 0)
    {
        status = fmRawPacketSocketHandlingInitialize(sw,
                                                     FALSE,
                                                     swCfg->netDevName);
    }

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, 
                     "Could not initialize raw socket for sw=%d, netdev=%s\n",
                     sw,
                     swCfg->netDevName);
        status = FM_OK;
    }

    if (swIsr && swCfg->bootMode == FM_PLAT_BOOT_MODE_SPI)
    {
        /* Select the desired interrupt mask register */
        if (swCfg->msiEnabled)
        {
            pcieIsrMask = (FM10000_UTIL_PCIE_BSM_INT_PCIE_0 << swCfg->mgmtPep);
        }
        else
        {
            pcieIsrMask = FM10000_UTIL_PCIE_BSM_INT_INT_PIN;
        }

        status = fm10000SetupPCIeISR(sw,
                                     pcieIsrMask,
                                     readFunc,
                                     writeFunc);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    ps = GET_PLAT_STATE(sw);

    /* Identify the switch */
    status = fm10000GetSwitchId(sw,
                                &ps->family,
                                &ps->model,
                                &ps->version,
                                readFunc,
                                writeFunc);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (ps->family == FM_SWITCH_FAMILY_UNKNOWN)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, "Unknown switch family, exiting\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

#endif

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSwitchPreInsert */




/*****************************************************************************/
/* fmPlatformSwitchInsert
 * \ingroup platform
 *
 * \desc            Called when the application inserts a switch in the system.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchInsert(fm_int sw)
{
    fm_status                  status = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);
    
    /* Generate the switch inserted event for this switch. */
    status = fmPlatformSendSwitchEvent(sw, FM_EVENT_SWITCH_INSERTED);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSwitchInsert */




/*****************************************************************************/
/* fmPlatformSwitchRemove
 * \ingroup platform
 *
 * \desc            Called when the application removes a switch from the
 *                  system.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchRemove(fm_int sw)
{
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    /* Generate the switch removed event for this switch. */
    status = fmPlatformSendSwitchEvent(sw, FM_EVENT_SWITCH_REMOVED);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSwitchRemove */




/*****************************************************************************/
/* fmPlatformSwitchInitialize
 * \ingroup platform
 *
 * \desc            Called by the API in response to an FM_EVENT_SWITCH_INSERTED
 *                  event (see ''Event Identifiers''). This function performs
 *                  platform specific initializations in the switch state table,
 *                  particularly to initialize the function pointers for
 *                  accessing the hardware device registers.
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
    fm_platformCfgSwitch *swCfg;
    fm_status             err;
    fm_switch *           switchPtr;
    fm_text               pktIface;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    switch (GET_PLAT_STATE(sw)->family)
    {
        case FM_SWITCH_FAMILY_FM10000:
            switchPtr->ConvertTimestamp  = NULL;
            switchPtr->ReceivePacket     = NULL;
            switchPtr->ProcessMgmtPacket = NULL;

            pktIface = GET_PROPERTY()->pktInterface;

            if (strcmp(pktIface, "pti") == 0)
            {
                switchPtr->SendPackets   = fm10000PTISendPackets;
            }
            else
            {
                switchPtr->SendPackets   = fmRawPacketSocketSendPackets;
                if ((GET_PLAT_RAW_LISTENER(sw))->handle)
                {
                    switchPtr->isRawSocketInitialized = FM_ENABLED;
                }
            }

            swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
            if (swCfg == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
            }

            err = SetRegAccessMode(sw, swCfg->regAccess);
            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
            }

#ifdef FM_LT_WHITE_MODEL_SUPPORT
            switchPtr->DbgDumpParity        = fm10000DbgDumpParity;
            switchPtr->DbgInjectParityError = fm10000ModelDbgInjectParityError;
#endif
            break;

#ifdef FM_SUPPORT_SWAG
        case FM_SWITCH_FAMILY_SWAG:
            swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
            if (swCfg == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
            }

            switchPtr->ReceivePacket           = NULL;
            switchPtr->WriteUINT32             = NULL;
            switchPtr->ReadUINT32              = NULL;
            switchPtr->MaskUINT32              = NULL;
            switchPtr->WriteUINT32Mult         = NULL;
            switchPtr->ReadUINT32Mult          = NULL;
            switchPtr->WriteUINT64             = NULL;
            switchPtr->ReadUINT64              = NULL;
            switchPtr->WriteUINT64Mult         = NULL;
            switchPtr->ReadUINT64Mult          = NULL;
            switchPtr->ReadEgressFid           = NULL;
            switchPtr->ReadIngressFid          = NULL;
            switchPtr->switchModel             = FM_SWITCH_MODEL_SWAG_C;
            switchPtr->switchVersion           = FM_SWITCH_VERSION_SWAG_1;
            switchPtr->switchFamily            = FM_SWITCH_FAMILY_SWAG;
            /* Remove the first CPU port from the compute since this one uses
             * the slot 0 */
            switchPtr->maxPhysicalPort         = swCfg->numPorts - 1;
            switchPtr->macTableSize            = FM10000_MAX_ADDR;
            switchPtr->macTableBankCount       = FM10000_MAC_ADDR_BANK_COUNT;
            switchPtr->macTableBankSize        = FM10000_MAC_ADDR_BANK_SIZE;
            switchPtr->vlanTableSize           = FM_MAX_VLAN;
            switchPtr->maxVlanCounter          = FM10000_MAX_VLAN_COUNTER;
            switchPtr->mirrorTableSize         = FM10000_MAX_MIRRORS_GRP;
            switchPtr->maxPhysicalLags         = FM_MAX_NUM_LAGS;
            switchPtr->maxPhysicalPortsPerLag  = FM_MAX_NUM_LAG_MEMBERS;
            switchPtr->maxRoutes               = FM_MAX_ROUTES;
            switchPtr->maxArpEntries           = FM_MAX_ARPS;
            switchPtr->maxIpInterfaces         = FM_MAX_IP_INTERFACES;
            switchPtr->maxVirtualRouters       = FM10000_MAX_VIRTUAL_ROUTERS;
            switchPtr->policerBanks            = FM10000_MAX_POLICER_BANKS;
            switchPtr->maxSTPInstances         = FM10000_MAX_STP_INSTANCE;
            switchPtr->maxEcmpGroupSize        = FM10000_MAX_ECMP_GROUP_SIZE;

            break;
#endif

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid switch family\n");
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
            break;

    }   /* end switch (GET_PLAT_STATE(sw)->family) */

    switchPtr->cpuPort = swCfg->cpuPort;
    switchPtr->msiEnabled = swCfg->msiEnabled;
    switchPtr->msiPep = swCfg->mgmtPep;
    switchPtr->fhClock = swCfg->fhClock;

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    err = fmPlatformModelSwitchInitialize(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSwitchInitialize */




/*****************************************************************************/
/* fmPlatformSwitchInserted
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
    fm_platformCfgSwitch  *swCfg;
    fm_platformLib        *libFunc;
    fm_status              status;
#ifndef FM_LT_WHITE_MODEL_SUPPORT
    fm_platformState      *ps;
#endif

#ifdef FM_SUPPORT_SWAG
    fm_platformCfg        *platCfg;
    fm_swagTopologySolver  solver;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    if (swCfg == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

#ifdef FM_SUPPORT_SWAG
    if (GET_PLAT_STATE(sw)->family == FM_SWITCH_FAMILY_SWAG)
    {
        platCfg = FM_PLAT_GET_CFG;

        if ( (platCfg->topology != FM_SWAG_TOPOLOGY_FAT_TREE) &&
             (platCfg->topology != FM_SWAG_TOPOLOGY_MESH) )
        {
            /* Unsupported topology */
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
        }

        /* Set swag topology */
        status = fmSetSWAGTopology(sw, platCfg->topology);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Get default solver for specified topology */
        status = fmGetSWAGDefaultTopologySolver(platCfg->topology, &solver);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Set the default swag topology solver function */
        status = fmSetSWAGTopologySolver(sw, solver);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Allow the platform to complete SWAG initialization */
        status = fmPlatformSWAGInitialize(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
#endif

    /* Initialize switch GPIO before calling InitSwitch since one GPIO might
       be used to reset the I2C devices (PCA), and those I2C devices
       will be accessed from the LT shared library. */
    status = fmPlatformGpioInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Set the I2C bus speed on the switch. */
    status = SetI2cBusSpeed(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Perform a reset of the I2C devices attached to the switch. */
    status = ResetI2cDevices(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Initialize the switch in the shared lib */
    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    if ( libFunc->InitSwitch )
    {
        status = libFunc->InitSwitch(swCfg->swNum);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    /* Set voltage scaling */
    status = SetVoltageScaling(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

#ifndef FM_LT_WHITE_MODEL_SUPPORT
    ps = GET_PLAT_STATE(sw);

    if (ps->switchType != FM_PLATFORM_SWITCH_TYPE_SWAG)
    {
        /* create thread to listen for when an interrupt occurs on that sw */
        status = fmCreateThread("interrupt_listener",
                                FM_EVENT_QUEUE_SIZE_NONE,
                                &fmPlatformInterruptListener,
                                ps,
                                &GET_PLAT_PROC_STATE(sw)->intrListener);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSwitchInserted */




/*****************************************************************************/
/* fmPlatformSwitchPreInitialize
 * \ingroup platform
 *
 * \desc            Called prior to initializing a switch device. This
 *                  function is responsible for initializing any aspect
 *                  of the platform required to bring up the device, for
 *                  example, applying power, deasserting the reset signal,
 *                  etc.
 *
 * \param[in]       sw is the switch about to be initialized.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchPreInitialize(fm_int sw)
{
    fm_int             tcpPort;
    fm_platformState * ps;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    fmPlatformModelSwitchPreInitialize(sw);
#endif

    ps = GET_PLAT_STATE(sw);

    if (ps->switchType != FM_PLATFORM_SWITCH_TYPE_SWAG)
    {
        tcpPort = GET_PROPERTY()->sbusServerPort;

        if (tcpPort)
        {
            fm10000SbusServerStart(tcpPort);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSwitchPreInitialize */




/*****************************************************************************/
/* fmPlatformSwitchPostInitialize
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
    fm_status       status;
    fm_platformLib *libFunc;
#ifdef FM_SUPPORT_SWAG
    fm_bool         isMaster;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

#ifdef FM_SUPPORT_SWAG
    if (sw == fmRootPlatform->swagId)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }
#endif

    if ( (sw < FM_FIRST_FOCALPOINT) || (sw > FM_PLAT_NUM_SW) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    switch (GET_PLAT_STATE(sw)->family)
    {
        case FM_SWITCH_FAMILY_FM10000:
            status = PlatformSwitchPostInitialize(sw);
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid switch family\n");
            status = FM_FAIL;

    }   /* end switch (GET_PLAT_STATE(sw)->family) */

    fmPlatformMgmtEnableInterrupt(sw);

    /* make sure the interrupts are enabled */
    fmPlatformEnableInterrupt(sw, FM_INTERRUPT_SOURCE_ISR);
    
#if FM_SUPPORT_SWAG
    if (fmRootPlatform->swagId >= 0)
    {
        if (sw == 1)
        {
            /* The middle switch is the master */
            isMaster = TRUE;
        }
        else
        {
            /* Side switches are slaves */
            isMaster = FALSE;
        }

        status = fmSetSWAGSwitchMaster(fmRootPlatform->swagId,
                                       sw,
                                       isMaster);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
#endif

    /* call the platform lib if needed */
    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( libFunc->PostInit )
    {
        status = libFunc->PostInit(sw, 1);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSwitchPostInitialize */




/*****************************************************************************/
/* fmPlatformCreateSWAG
 * \ingroup platform
 *
 * \desc            Requests the platform to assign a new switch aggregate
 *                  switch number.
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
#if FM_SUPPORT_SWAG

    fm_status             status;
    fm_int                i;
    fm_int                swagNumIndex;
    fm_platformCfgSwitch *swCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "swagSw=%p\n", (void *) swagSw);

    status = FM_ERR_INVALID_SWITCH;

    if (swagSw == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    for (swagNumIndex = 0 ; swagNumIndex < FM_MAX_SWITCH_AGGREGATES ; swagNumIndex++)
    {
        if (fmRootPlatform->swagNumbers[swagNumIndex] < 0)
        {
            break;
        }
    }

    if (swagNumIndex >= FM_MAX_SWITCH_AGGREGATES)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    /* First look for a defined SWAG switch in the switch role table */
    for (i = 0 ; i < FM_PLAT_NUM_SW ; i++)
    {
        swCfg = FM_PLAT_GET_SWITCH_CFG(i);

        if (swCfg->switchRole == FM_SWITCH_ROLE_SWAG)
        {
            fmRootPlatform->swagNumbers[swagNumIndex] = i;
            fmRootPlatform->platformState[i].family     = FM_SWITCH_FAMILY_SWAG;
            fmRootPlatform->platformState[i].model      = FM_SWITCH_MODEL_SWAG_C;
            fmRootPlatform->platformState[i].switchType = FM_PLATFORM_SWITCH_TYPE_SWAG;

            fmRootPlatform->platformState[i].version = FM_SWITCH_VERSION_SWAG_1;

            *swagSw = i;

            if (fmRootPlatform->swagId < 0)
            {
                fmRootPlatform->swagId = i;
            }

            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
        }
        /* For Physical switches family,mode,etc are obtained using fm10000GetSwitchId in 
           fmPlatformSwitchPreInsert. */
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

#else

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "swagSw=%p\n", (void *) swagSw);

    FM_NOT_USED(swagSw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);

#endif

}   /* end fmPlatformCreateSWAG */




/*****************************************************************************/
/* fmPlatformDeleteSWAG
 * \ingroup platform
 *
 * \desc            Instructs the platform to release a switch aggregate
 *                  switch number.
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
#if FM_SUPPORT_SWAG

    fm_status status = FM_ERR_INVALID_SWITCH;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    for (i = 0 ; i < FM_MAX_SWITCH_AGGREGATES ; i++)
    {
        if (fmRootPlatform->swagNumbers[i] == sw)
        {
            fmRootPlatform->swagNumbers[i] = -1;
            status                         = FM_OK;
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

#else

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    FM_NOT_USED(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);

#endif

}   /* end fmPlatformDeleteSWAG */




#ifdef FM_SUPPORT_SWAG

/*****************************************************************************/
/* fmPlatformSWAGInitialize
 * \ingroup platform
 *
 * \desc            Called by the API during switch inserted event processing
 *                  if the switch is a SWAG.  This function performs platform-
 *                  specific initialization for the SWAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformSWAGInitialize(fm_int sw)
{
    fm_status             status;
    fm_int                i;
    fmSWAG_switch        *switchExt;
    fm_platformCfgSwitch *swCfg;
    fm_platformState     *ps;
    fm_switch            *swPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d\n",
                 sw);

    if (sw != fmRootPlatform->swagId)
    {
        /* Not the default SWAG, do nothing here. */
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    switchExt = GET_SWITCH_EXT(sw);
    switchExt->requiredSwitchMask = 0;

    /* Initialize default values for switch aggregate stack glort range */
    status = fmSetStackGlortRange(sw, 0x00000000, 0x00007fff);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Add all physical switches to the Switch Aggregate */
    for (i = FM_FIRST_FOCALPOINT ; i < FM_PLAT_NUM_SW ; i++)
    {
        swCfg = FM_PLAT_GET_SWITCH_CFG(i);
        ps    = GET_PLAT_STATE(i);

        /* fmRootPlatform->platformState[i].switchType is not set by
         * the time this function is called. Hence switchRole is used to 
         * identify physical switches for addition. */
        if (swCfg->switchRole != FM_SWITCH_ROLE_SWAG)
        {
            switchExt->requiredSwitchMask |= 1 << i;
            swPtr = fmRootApi->fmSwitchStateTable[i];

            /* SWAG member is not associated to SWAG so update this
             * information for fmAddSWAGSwitch purposes. */
            if (swPtr != NULL)
            {
                swPtr->swag = -1;
            }

            status = fmAddSWAGSwitch(sw, i, swCfg->switchRole);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* SWAG member is now associated to SWAG. */
            if (swPtr != NULL)
            {
                swPtr->swag = sw;
            }
        }
    }

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    for (i = 0 ; i < swCfg->numPorts ; i++)
    {
        if (swCfg->ports[i].swagLink.type != FM_SWAG_LINK_UNDEFINED)
        {
            status = fmAddSWAGLink(sw, &swCfg->ports[i].swagLink);
            if (status == FM_ERR_ALREADYUSED_PORT)
            {
                status = FM_OK;
            }
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
    }

    swPtr = GET_SWITCH_PTR(sw);

    switch (swPtr->switchModel)
    {
        case FM_SWITCH_MODEL_SWAG_A:
            swPtr->vlanLearningMode   = FM_VLAN_LEARNING_MODE_SHARED;
            swPtr->sharedLearningVlan = 1;
            break;

        case FM_SWITCH_MODEL_SWAG_B:
        case FM_SWITCH_MODEL_SWAG_C:
            swPtr->vlanLearningMode   = FM_VLAN_LEARNING_MODE_INDEPENDENT;
            swPtr->sharedLearningVlan = 1;
            break;

        default:
            status = FM_FAIL;
            FM_LOG_EXIT(FM_LOG_CAT_SWAG, status);
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSWAGInitialize */

#endif  /* end FM_SUPPORT_SWAG */




/*****************************************************************************/
/* fmPlatformSetBypassMode
 * \ingroup platform
 *
 * \desc            This service enables bypass mode for the given switch
 *                  on platforms which support the feature.  Typically the
 *                  mode is set to TRUE sometime during the platform
 *                  initialization procedure, and is set to FALSE later
 *                  from the application.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       mode is the bypass mode to set.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetBypassMode(fm_int sw, fm_bool mode)
{

    GET_PLAT_STATE(sw)->bypassEnable = mode;

    return FM_OK;

}   /* end fmPlatformSetBypassMode */




/*****************************************************************************/
/* fmPlatformBypassEnabled
 * \ingroup platform
 *
 * \desc            Called by both platform and API services
 *                  to query whether the platform is currently in bypass mode.
 *                  The value FALSE doubles as both as an indicator that
 *                  bypass mode is not supported as well as the case that
 *                  bypass mode is supported but not enabled.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          TRUE if bypass mode is enabled, FALSE otherwise.
 *
 *****************************************************************************/
fm_bool fmPlatformBypassEnabled(fm_int sw)
{

    return GET_PLAT_STATE(sw)->bypassEnable;

}   /* end fmPlatformBypassEnabled */




/*****************************************************************************/
/* fmPlatformReset
 * \ingroup platform
 *
 * \desc            Assert the reset signal into the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformReset(fm_int sw)
{
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    FM_NOT_USED(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformReset */




/*****************************************************************************/
/* fmPlatformRelease
 * \ingroup platform
 *
 * \desc            Deassert the reset signal into the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformRelease(fm_int sw)
{
    fm_status                   err = FM_OK;
#ifdef FM_SUPPORT_SWAG
    fm_int                      i;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if ( (sw < 0) || (sw >= FM_PLAT_NUM_SW) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

#ifdef FM_SUPPORT_SWAG
    if (sw == fmRootPlatform->swagId)
    {
        for (i = FM_FIRST_FOCALPOINT ; i < FM_PLAT_NUM_SW ; i++)
        {
            err = fmPlatformRelease(i);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        }

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }
#endif

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    err = fmPlatformModelRelease(sw);
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformRelease */




#ifndef FM_LT_WHITE_MODEL_SUPPORT
/*****************************************************************************/
/* fmPlatformGetInterrupt
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
 * \param[out]      intrSrc is the resultant source
 *                  trigger.  On this platform, only FM_INTERRUPT_SOURCE_ISR is
 *                  supported.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetInterrupt(fm_int sw, fm_uint intrType, fm_uint *intrSrc)
{
    fm_platformState *ps;
    fm_status         status = FM_OK;

    FM_NOT_USED(intrType);

    if (fmRootPlatform == NULL)
    {
        *intrSrc = FM_INTERRUPT_SOURCE_NONE;
        goto ABORT;
    }

    ps = GET_PLAT_STATE(sw);

    /*  Check for pending interrupt source before taking platform lock.
     *  If an interrupt is pending, we will have to take the lock before
     *  we can clear it, but this will save a lot of lock take/drop
     *  operations and is safe to do.
     */
    if (ps->intrSource == FM_INTERRUPT_SOURCE_NONE)
    {
        *intrSrc = FM_INTERRUPT_SOURCE_NONE;
        goto ABORT;
    }

    TAKE_PLAT_LOCK(sw, FM_PLAT_INFO);

    /* grab source and clear */
    *intrSrc = ps->intrSource;

    ps->intrSource = FM_INTERRUPT_SOURCE_NONE;

    DROP_PLAT_LOCK(sw, FM_PLAT_INFO);

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_PLATFORM,
                           (*intrSrc != FM_INTERRUPT_SOURCE_NONE),
                           status = FM_FAIL,
                           "ASSERTION FAILURE: "
                           "no interrupt source but semaphore signaled!\n");

ABORT:
    FM_LOG_DEBUG(FM_LOG_CAT_EVENT,
                 "sw %d intrType %d intrSrc 0x%x\n",
                 sw, intrType, *intrSrc);

    return FM_OK;

}   /* end fmPlatformGetInterrupt */




/*****************************************************************************/
/* fmPlatformEnableInterrupt
 * \ingroup platform
 *
 * \desc            Enable the interrupt signal from the switch device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       intrTypes is a mask of the different interrupt types to
 *                  enable.  Only FM_INTERRUPT_SOURCE_ISR is supported in
 *                  this platform.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformEnableInterrupt(fm_int sw, fm_uint intrTypes)
{
    fm_status        status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_INTR,
                         "sw = %d, intrTypes = %u\n",
                         sw,
                         intrTypes);

    /* MSI interrupts are edge triggered, so the interrupt must be cleared
     * before the driver will get another interrupt
     */
    if (CheckInterrupt(sw, "CHECK1"))
    {
        status = FM_OK;
    }
    else
    {
        /* No interrupt pending, then enable interrupt in the driver */
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Enable Host driver interrupt\n");

        status = fmPlatformHostDrvEnableInterrupt(sw, intrTypes);

        /* Do again just to be sure, if there is a timing issue of interrupt
         * pending before driver enable interrupt
         */
        CheckInterrupt(sw, "CHECK2");
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_INTR, status);

}   /* end fmPlatformEnableInterrupt */




/*****************************************************************************/
/* fmPlatformDisableInterrupt
 * \ingroup platform
 *
 * \desc            Disable the interrupt signal from the switch device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       intrTypes is a mask of the different interrupt types to
 *                  enable.  Only FM_INTERRUPT_SOURCE_ISR is supported in
 *                  this platform.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformDisableInterrupt(fm_int sw, fm_uint intrTypes)
{
    fm_status status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_INTR,
                         "sw = %d, intrTypes = %u\n",
                         sw,
                         intrTypes);

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Disable Host driver interrupt\n");

    status = fmPlatformHostDrvDisableInterrupt(sw, intrTypes);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_INTR, status);

}   /* end fmPlatformDisableInterrupt */

#endif  /* ifndef FM_LT_WHITE_MODEL_SUPPORT */



/*****************************************************************************/
/* fmPlatformInterruptListener
 * \ingroup intPlatform
 *
 * \desc            thread to wait for interrupts and trigger the user-side
 *                  interrupt handler
 *
 * \param[in]       args contains thread-initialization parameters
 *
 * \return          None.
 *
 *****************************************************************************/
void *fmPlatformInterruptListener(void *args)
{
    fm_platformCfgSwitch *swCfg;
    fm_platformState *    ps;
    fm_thread *           thread;
    fm_uint               intrStatus;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm_status             status;
    fm_int                intrPeriodMsec;
    fm_int                intrPeriodSec;
    fm_int                intrPeriodNsec;
    fm_int                msiEnabled;
    fm_int                intrTimeoutSec;
    fm_int                maxTimeoutCnt;

    intrStatus = 0;
    msiEnabled = FALSE;
    /* grab arguments */
    thread = FM_GET_THREAD_HANDLE(args);
    ps     = FM_GET_THREAD_PARAM(fm_platformState, args);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "thread = %s, ps = %p, ps->sw = %d\n",
                 thread->name,
                 (void *) ps,
                 ps->sw);

    swCfg = FM_PLAT_GET_SWITCH_CFG(ps->sw);

    if (swCfg->intrPollPeriodMsec <= 0)
        swCfg->intrPollPeriodMsec = 1;

    msiEnabled = swCfg->msiEnabled;
    if (!msiEnabled)
        FM_LOG_PRINT("Using interrupt polling with period set to %d msec\n", 
                     swCfg->intrPollPeriodMsec);

    intrTimeoutSec     = 1;
    maxTimeoutCnt      = swCfg->intrTimeoutCnt;
    ps->intrTimeoutCnt = 0;

    while (1)
    {
        if ( !SWITCH_LOCK_EXISTS(ps->sw) )
        {
            /* The switch doesn't exist yet??? */
            break;
        }

        if (!msiEnabled)
        {
            /* Use interrupt polling */
            intrPeriodMsec = swCfg->intrPollPeriodMsec;
            intrPeriodSec  = intrPeriodMsec / 1000;
            intrPeriodNsec = (intrPeriodMsec % 1000) * 1000 * 1000;
            fmDelay(intrPeriodSec, intrPeriodNsec);
        }

        PROTECT_SWITCH(ps->sw);
        switchPtr = GET_SWITCH_PTR(ps->sw);

        /* Switch was removed, kill the thread */
        if (switchPtr == NULL)
        {
            UNPROTECT_SWITCH(ps->sw);
            break;
        }

        /* In MSI mode do not wait on interrupt if the switch is not UP.
           The same for polling mode, unless the PCIe ISR is done in software,
           in such a case the BSM interrupts are handled below. */
        if ( ( msiEnabled && switchPtr->state != FM_SWITCH_STATE_UP ) ||
             ( !msiEnabled && switchPtr->state != FM_SWITCH_STATE_UP &&
               swCfg->pcieISR != FM_PLAT_PCIE_ISR_SW ) )
        {
            UNPROTECT_SWITCH(ps->sw);
            fmDelay(5, 0);
            continue;
        }

        if (!msiEnabled)
        {
            /* Use interrupt polling */
            switchExt = switchPtr->extension;
            status = fm10000PollInterrupt(ps->sw,
                                          switchExt->interruptMaskValue,
                                          &intrStatus,
                                          switchPtr->ReadUINT32);
            if (status != FM_OK || intrStatus == 0)
            {
                UNPROTECT_SWITCH(ps->sw);
                continue;
            }

            if (intrStatus & FM10000_UTIL_INT_STATUS_BSM)
            {
                /* At least one BSM interrupt pending */
                fm10000PCIeISR(ps->sw,
                               &swCfg->bootCfg,
                               switchPtr->ReadUINT32,
                               switchPtr->WriteUINT32);

                /* Clear the BSM bit */
                intrStatus &= ~FM10000_UTIL_INT_STATUS_BSM;
            }
            UNPROTECT_SWITCH(ps->sw);
        }
        else if (swCfg->regAccess == FM_PLAT_REG_ACCESS_PCIE)
        {
            UNPROTECT_SWITCH(ps->sw);

            /* Check for host driver interrupt */
            if (fmPlatformHostDrvWaitForInterrupt(ps->sw,
                                                  intrTimeoutSec,
                                                  &intrStatus))
            {
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                             "Getting Host driver interrupt failed\n");
                continue;
            }
            else if (intrStatus == 0 && maxTimeoutCnt > 0)
            {
                /***************************************************
                 * To prevent for interrupt issue, the following 
                 * solution is used: 
                 *  
                 * Use polling to see if there's a pending interrupt. 
                 * If there is one, then increment an interrupt 
                 * timeout counter. If that counter reach a certain 
                 * limit then a warning is displayed and the pending 
                 * interrupts are processed as usual, which will 
                 * eventually re-enable the interrupt in the driver. 
                 **************************************************/

                PROTECT_SWITCH(ps->sw);
                switchPtr = GET_SWITCH_PTR(ps->sw);

                /* Switch was removed, kill the thread */
                if (switchPtr == NULL)
                {
                    UNPROTECT_SWITCH(ps->sw);
                    break;
                }

                switchExt = switchPtr->extension;

                /* Poll for interrupt pending */
                if (fm10000PollInterrupt(ps->sw,
                                         switchExt->interruptMaskValue,
                                         &intrStatus,
                                         switchPtr->ReadUINT32))
                {
                    intrStatus = 0;
                }
                UNPROTECT_SWITCH(ps->sw);

                if (intrStatus != 0)
                {
                    /* We got an interrupt timeout but there is pending
                       interrupt. Increment the counter. */
                    ps->intrTimeoutCnt++;

                    /* Did we reach the timeout cnt limit? */
                    if (ps->intrTimeoutCnt >= maxTimeoutCnt)
                    {
                        /* Yes, display a warning and keep intrStatus as is
                           so the pending interrupts will be treated below. */
#if 0
                        FM_LOG_PRINT("Interrupt issue detected on sw %d, "
                                     "re-enable interrupt.\n",
                                     ps->sw);
#endif
                        ps->intrTimeoutCnt = 0;
                    }
                    else
                    {
                        /* No the limit is not reached, clear intrStatus so
                           pending interrupts are not handled below.
                           They will normally be handled upon the next call to
                           fmPlatformHostDrvWaitForInterrupt above, unless the
                           interrupts are no longer working. */
                        intrStatus = 0;
                    }
                }
            }
            else
            {
                /* Clear the interrupt timeout counter */
                ps->intrTimeoutCnt = 0;
            }
        }
        else
        {
            UNPROTECT_SWITCH(ps->sw);
            fmDelay(5, 0);
            continue;
        }

        if ( intrStatus != 0 )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                         "Sw %d: Got interrupt! (intrStatus %d)\n",
                         ps->sw, intrStatus);

            TAKE_PLAT_LOCK(ps->sw, FM_PLAT_INFO);

            ps->intrSource |= FM_INTERRUPT_SOURCE_ISR;

            DROP_PLAT_LOCK(ps->sw, FM_PLAT_INFO);

            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                         "Signaling semaphore\n");

            fmSignalSemaphore(&fmRootApi->intrAvail);
        }

#if 0
        if ( FM_IOCTL_WAIT_INTERRUPT_IS_MGMT_INTR(intrStatus) )
        {
            /* If mgmt is the only interrupt, must have mgmt enable the interrupt again */
            fmPlatformMgmtSignalInterrupt( ps->sw, !FM_IOCTL_WAIT_INTERRUPT_IS_HW_INTR(intrStatus) );
        }
#endif
    }   /* end while (1) */

    fmExitThread(thread);

    return NULL;

}   /* end fmPlatformInterruptListener */




#ifndef FM_LT_WHITE_MODEL_SUPPORT
/*****************************************************************************/
/* fmPlatformTriggerInterrupt
 * \ingroup intPlatform
 *
 * \desc            Artificially signals the interrupt semaphore.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       intrTypes is a mask of the different interrupt types to
 *                  trigger.  Only FM_INTERRUPT_SOURCE_API is supported in
 *                  this platform.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformTriggerInterrupt(fm_int sw, fm_uint intrTypes)
{
    fm_platformState *ps = GET_PLAT_STATE(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, intrTypes = %u\n",
                 sw,
                 intrTypes);

    if (intrTypes & FM_INTERRUPT_SOURCE_API)
    {
        TAKE_PLAT_LOCK(sw, FM_PLAT_INFO);

        ps->intrSource |= FM_INTERRUPT_SOURCE_API;

        DROP_PLAT_LOCK(sw, FM_PLAT_INFO);

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT,
                     "fmPlatformTriggerInterrupt: signaling semaphore\n");

        fmSignalSemaphore(&fmRootApi->intrAvail);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformTriggerInterrupt */
#endif  /* ifndef FM_LT_WHITE_MODEL_SUPPORT */



/*****************************************************************************/
/* fmPlatformSendPackets
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
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr)
    {
        switch (GET_PLAT_STATE(sw)->family)
        {
            case FM_SWITCH_FAMILY_FM10000:

                if (switchPtr->SendPackets)
                {
                    return switchPtr->SendPackets(sw);
                }
                else
                {
                    return FM_FAIL;
                }

            default:
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Invalid switch family\n");

        }   /* end switch (GET_PLAT_STATE(sw)->family) */

    }

    return FM_FAIL;

}   /* end fmPlatformSendPackets */




/*****************************************************************************/
/* fmPlatformSwitchTerminate
 * \ingroup intPlatform
 *
 * \desc            Called after shutting down a switch device. This
 *                  function is responsible for any platform-specific actions
 *                  that must be taken when a switch is removed from the
 *                  system, for example, turning off power, asserting the reset
 *                  signal, etc.
 *
 * \param[in]       sw is the switch that has been shut down.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchTerminate(fm_int sw)
{
#ifndef FM_LT_WHITE_MODEL_SUPPORT
    fm_platformCfgSwitch *swCfg;
#endif
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

#if INSTRUMENT_LOG_LEVEL > 0
    fmPlatformCloseInstrumentation();
#endif

#ifdef FM_LT_WHITE_MODEL_SUPPORT
    status = fmGenericPacketDestroy(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
#else
    /* raw socket thread should terminate when switch state == NULL */
    if ((GET_PLAT_RAW_LISTENER(sw))->handle)
    {
        fmWaitThreadExit(&GET_PLAT_PROC_STATE(sw)->rawsocketThread);
    }

    status = fmRawPacketSocketDestroy(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* interrupt thread should terminate when switch state == NULL */
    if ((GET_PLAT_INTR_LISTENER(sw))->handle)
    {
        fmWaitThreadExit(&GET_PLAT_PROC_STATE(sw)->intrListener);
    }

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    if (swCfg == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (swCfg->regAccess == FM_PLAT_REG_ACCESS_PCIE)
    {
        /* Disconnect from the host kernel driver or /dev/mem */
        status = DisconnectFromPCIE(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSwitchTerminate */




/*****************************************************************************/
/* fmPlatformReceiveProcess
 * \ingroup intPlatform
 *
 * \desc            Processes the received packet and enqueues it to a higher
 *                  layer.
 *                  Select the proper switch specific function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       buffer points to the receive packet buffer. If an ISL tag
 *                  is present in the data buffer, the ISL tag should be in
 *                  network-byte order.
 *
 * \param[in]       pIslTag points to the host-byte order ISL tag if an ISL tag
 *                  is not present in the data buffer. Set to NULL if an ISL
 *                  tag is present in the data buffer. If an ISL tag is not
 *                  present in the data buffer, the data buffer must have
 *                  enough space for a VLAN tag to be inserted.
 *
 * \param[in]       flags to pass to the function, if any.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformReceiveProcess(fm_int     sw,
                                   fm_buffer *buffer,
                                   fm_uint32 *pIslTag,
                                   fm_uint    flags)
{
    switch (GET_PLAT_STATE(sw)->family)
    {
        case FM_SWITCH_FAMILY_FM10000:
            /* Flags are ignored */
            return fm10000PacketReceiveProcess(sw, buffer, pIslTag, NULL);

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid switch family\n");

    }   /* end switch (GET_PLAT_STATE(sw)->family) */

    return FM_FAIL;

}   /* end fmPlatformReceiveProcess */




/*****************************************************************************/
/* fmPlatformReceiveProcessV2
 * \ingroup intPlatform
 *
 * \desc            Processing the received packet and enqueue to higher layer.
 *                  Select the proper switch specific function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       buffer points to the receive packet buffer. If ISL tag is
 *                  present in data buffer, the ISL tags should be in
 *                  network-byte order.
 *
 * \param[in]       pIslTag points to the ISL tag, if not present if data buffer.
 *                  Set to NULL, if ISL tag is present in data buffer. If ISL
 *                  tag is not present in the data buffer, the data buffer must
 *                  have enough space for vlantag to be inserted. This tags
 *                  should be in host-byte order.
 *
 * \param[in]       sbData is a pointer to the caller allocated storage where
 *                  any sideband data is stored. Can be set to NULL if there
 *                  is no sideband data to carry. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformReceiveProcessV2(fm_int              sw,
                                     fm_buffer *         buffer,
                                     fm_uint32 *         pIslTag,
                                     fm_pktSideBandData *sbData)
{
    switch (GET_PLAT_STATE(sw)->family)
    {
        case FM_SWITCH_FAMILY_FM10000:
            return fm10000PacketReceiveProcess(sw, buffer, pIslTag, sbData);

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid switch family\n");

    }   /* end switch (GET_PLAT_STATE(sw)->family) */

    return FM_FAIL;

}   /* end fmPlatformReceiveProcessV2 */




/*****************************************************************************/
/** fmPlatformGetAttribute
 * \ingroup platformApp
 *
 * \desc            Gets a platform attribute.
 *
 * \param[in]       sw is the switch on which to operate.  Not all
 *                  attributes will use this argument. See the attribute's
 *                  documentation for specific use.
 *
 * \param[in]       index refers to the logical entity on which to
 *                  operate.  Not all attributes will use this argument.
 *                  See the attribute's documentation for specific use.
 *                  If the attribute is a port attribute, then the index
 *                  is the logical port.
 *
 * \param[in]       attr is the attribute constant.
 *
 * \param[in]       value points to caller allocated storage where the value
 *                  will be written.  See the attribute documentation for
 *                  what to use.
 *
 * \return          FM_OK on success.
 * \return          FM_ERR_INVALID_ARGUMENT on an invalid attribute.
 * \return          FM_ERR_UNSUPPORTED if the platform does not support
 *                  this function.
 *
 *****************************************************************************/
fm_status fmPlatformGetAttribute(fm_int    sw,
                                 fm_int    index,
                                 fm_uint32 attr,
                                 void *    value)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(index);
    FM_NOT_USED(attr);
    FM_NOT_USED(value);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);

}   /* end fmPlatformGetAttribute */




/*****************************************************************************/
/** fmPlatformSetAttribute
 * \ingroup platformApp
 *
 * \desc            Sets a platform attribute.
 *
 * \param[in]       sw is the switch on which to operate.  Not all
 *                  attributes will use this argument. See the attribute's
 *                  documentation for specific use.
 *
 * \param[in]       index refers to the logical entity on which to
 *                  operate.  Not all attributes will use this argument.
 *                  See the attribute's documentation for specific use.
 *                  If the attribute is a port attribute, then the index
 *                  is the logical port.
 *
 * \param[in]       attr is the attribute constant.
 *
 * \param[in]       value points to the specific value type.  See the
 *                  attribute documentation for what to use.
 *
 * \return          FM_OK on success.
 * \return          FM_ERR_INVALID_ARGUMENT on an invalid attribute.
 * \return          FM_ERR_UNSUPPORTED if the platform does not support
 *                  this function.
 *
 *****************************************************************************/
fm_status fmPlatformSetAttribute(fm_int    sw,
                                 fm_int    index,
                                 fm_uint32 attr,
                                 void *    value)
{
    fm_status err = FM_OK;

    FM_NOT_USED(sw);
    FM_NOT_USED(index);
    FM_NOT_USED(attr);
    FM_NOT_USED(value);

    switch (attr)
    {
#ifdef FM_LT_WHITE_MODEL_SUPPORT
        case FM_PLATFORM_ATTR_EGRESS_LIMIT:
            err = fmModelPacketQueueSetAttribute(FM_MODEL_PACKET_QUEUE_EGRESS_LIMIT,
                                                 value);
            break;

        case FM_PLATFORM_ATTR_TOPOLOGY:
            if (fmRootPlatform->topologySet)
            {
                /* The model topology file was built internally based upon the
                 * requested topology. Use it instead of that provided in
                 * this function call. */
                err = fmModelPacketQueueSetAttribute(FM_MODEL_PACKET_QUEUE_TOPOLOGY,
                                                     PLATFORM_MODEL_TOPOLOGY_FILE);
            }
            else
            {
                /* No pre-built model topology file, use the provided file. */
                err = fmModelPacketQueueSetAttribute(FM_MODEL_PACKET_QUEUE_TOPOLOGY,
                                                     value);
            }
            break;
#endif

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformSetAttribute */




/*****************************************************************************/
/* fmPlatformGetSwitchPartNumber
 * \ingroup platform
 *
 * \desc            Called by API services to determine the part number
 *                  of the switch.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      spn points to caller-supplied storage in which the
 *                  switch part number should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetSwitchPartNumber(fm_int sw, fm_switchPartNum *spn)
{

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, partNumber = %p\n",
                 sw, (void *) spn);

    if ( (sw < 0) || (sw >= fmRootPlatform->cfg.numSwitches) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    switch (GET_PLAT_STATE(sw)->family)
    {
        case FM_SWITCH_FAMILY_FM10000:
            *spn = FM_SWITCH_PART_NUM_FM10440;
            break;

        default:
            *spn = FM_SWITCH_PART_NUM_UNKNOWN;
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            break;

    }   /* end switch */

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformGetSwitchPartNumber */




/*****************************************************************************/
/* fmPlatformSetRegAccessMode
 * \ingroup platformApp
 *
 * \desc            Set the register access mode (EBI/PCIe/I2C)
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       mode is the register access mode
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the switchPtr is NULL
 *
 *****************************************************************************/ 
fm_status fmPlatformSetRegAccessMode(fm_int sw, fm_int mode)
{
    fm_platformCfgSwitch *swCfg;
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d, mode = %d\n", sw, mode);

    status = FM_OK;

    switch (mode)
    {
        case FM_PLAT_REG_ACCESS_PCIE:
            /* Having family different than UNKNOWN indicates the switch
               has been identified and thus it exists. */
            if (GET_PLAT_STATE(sw)->family != FM_SWITCH_FAMILY_UNKNOWN)
            {
                /* Hook up to the host kernel driver or /dev/mem */
                status = ConnectToPCIE(sw);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }
            /* break; let's go through */
        case FM_PLAT_REG_ACCESS_I2C:
        case FM_PLAT_REG_ACCESS_EBI:
            /* No need to check the returned status as it can be != FM_OK
               if the switch doesn't exist yet. */
            SetRegAccessMode(sw,mode);

            swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
            if (swCfg != NULL)
            {
                swCfg->regAccess = mode;
            }
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSetRegAccessMode */




/*****************************************************************************/
/* fmPlatformSetInterruptPollingPeriod
 * \ingroup platformApp
 *
 * \desc            Set the interrupt polling period in msec.
 *                  Set to 0 to disable polling.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       periodMsec is the polling period in msec.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the sw is invalid
 *
 *****************************************************************************/ 
fm_status fmPlatformSetInterruptPollingPeriod(fm_int sw, fm_int periodMsec)
{
    fm_platformCfgSwitch *swCfg;
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d, period=%d\n", sw, periodMsec);

    status = FM_ERR_INVALID_ARGUMENT;

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    if (swCfg != NULL)
    {
        swCfg->intrPollPeriodMsec = (periodMsec > 0) ? periodMsec : 1;
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSetInterruptPollingPeriod */




fm_status fmPlatformReadUnlockCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{
    fm_switch *switchPtr;

    if ( !SWITCH_LOCK_EXISTS(sw) )
    {
        /* The switch doesn't exist yet */
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "The switch is not inserted\n");
        return FM_ERR_UNINITIALIZED;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (switchPtr->ReadRawUINT32 != NULL)
    {
        return switchPtr->ReadRawUINT32(sw,addr,value);
    }
    else
    {
        return FM_ERR_UNSUPPORTED;
    }

}   /* end fmPlatformReadUnlockCSR */




fm_status fmPlatformWriteUnlockCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{
    fm_switch *switchPtr;

    if ( !SWITCH_LOCK_EXISTS(sw) )
    {
        /* The switch doesn't exist yet */
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "The switch is not inserted\n");
        return FM_ERR_UNINITIALIZED;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    return switchPtr->WriteRawUINT32(sw,addr,value);

}   /* end fmPlatformWriteUnlockCSR */




/*****************************************************************************/
/* fmPlatformTerminate
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

    if (fmPlatformProcessState != NULL)
    {
        fmFree(fmPlatformProcessState);
        fmPlatformProcessState = NULL;
    }

    if (desiredMemmapAddr != NULL)
    {
        fmFree(desiredMemmapAddr);
        desiredMemmapAddr = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformTerminate */

