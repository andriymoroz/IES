/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_host_drv.c
 * Creation Date:   August 2014
 * Description:     Functions to interface to Host kernel driver.
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

#include <fm_sdk_int.h>
#include <dirent.h>
#include <sys/time.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define DRIVER_UIO_NAME                 "fm10k"
#define DPDK_DRIVER_UIO_NAME            "igb_uio"
#define DPDK_DRIVER_BAR4_MEM_MAP_NAME   "BAR4"
#define UIO_DEV_NAME_PREFIX             "uio"
#define FM_NETDEV_MAX_NAME_SIZE         256
#define FM_VPD_MAX_STRINNG_SIZE         256
#define FM_VPD_LARGE_RESOURCE_PROD_NAME 0x82
#define FM_VPD_LARGE_RESOURCE_READ      0x90
#define FM_VPD_LARGE_RESOURCE_WRITE     0x91
#define FM_VPD_SMALL_RESOURCE_END       0x78

#ifndef FM_INTEL_PCIE_VENDOR_ID
#define FM_INTEL_PCIE_VENDOR_ID         0x8086
#endif

#ifndef FM10000_INTEL_PCIE_DEVICE_ID
#define FM10000_INTEL_PCIE_DEVICE_ID    0x15A4
#endif

#define BOULDER_RAPIDS_DEVICE_ID    0x15D0
#define ATWOOD_CHANNEL_DEVICE_ID    0x15D5

/* The time threshold after which the VPD read
 * is considered slow. */
#define VPD_READ_SLOW_THRESHOLD_USEC    1000000

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

/*****************************************************************************/
/* ReadLineFromFile
 * \ingroup intPlatform
 *
 * \desc            Read the first line on a given file.
 *
 * \param[in]       filename is the file to read.
 *
 * \param[out]      linebuf points to caller-provided storage into which the
 *                  the read line is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ReadLineFromFile(fm_char *filename, fm_char *linebuf)
{
    fm_char *s;
    fm_int   i;
    FILE *   file;

    memset(linebuf, 0, FM_UIO_MAX_NAME_SIZE);

    file = fopen(filename,"r");
    if (!file)
        return FM_ERR_NOT_FOUND;

    s = fgets(linebuf,FM_UIO_MAX_NAME_SIZE,file);

    fclose(file);

    if (!s)
        return FM_FAIL;

    /* Find the \n and replace it by 0 to NULL terminate the string. */
    for (i=0 ; (*s) && (i < FM_UIO_MAX_NAME_SIZE) ; i++)
    {
        if (*s == '\n')
            *s = 0;
        s++;
    }

    return FM_OK;

}   /* end ReadLineFromFile */




/*****************************************************************************/
/* GetUioName
 * \ingroup intPlatform
 *
 * \desc            Read the name stored in file
 *                      /sys/class/uio/uioX/name  (where X = num)
 *
 * \param[in]       num is the UIO device number on which to operate.
 *
 * \param[out]      name points to caller-provided storage into which the
 *                  the name read is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetUioName(fm_int num, fm_char *name)
{
    fm_char filename[FM_UIO_MAX_NAME_SIZE];

    FM_SNPRINTF_S(filename, sizeof(filename), "/sys/class/uio/uio%d/name", num);

    return ReadLineFromFile(filename, name);

}   /* end GetUioName */




/*****************************************************************************/
/* GetUioVersion
 * \ingroup intPlatform
 *
 * \desc            Read the version stored in file
 *                      /sys/class/uio/uioX/version  (where X = num)
 *
 * \param[in]       num is the UIO device number on which to operate.
 *
 * \param[out]      version points to caller-provided storage into which the
 *                  the version read is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetUioVersion(fm_int num, fm_char *version)
{
    fm_char filename[FM_UIO_MAX_NAME_SIZE];

    FM_SNPRINTF_S(filename, sizeof(filename), "/sys/class/uio/uio%d/version", num);

    return ReadLineFromFile(filename, version);

}   /* end GetUioVersion */




/*****************************************************************************/
/* GetUioMemMapNum
 * \ingroup intPlatform
 *
 * \desc            Get memory mapping number (BAR4) for specified uio device.
 *
 * \param[in]       num is the UIO device number on which to operate.
.*
 * \param[in]       name points to uio device name.
 *
 * \param[out]      memMapsNum points to caller-provided storage into which the
 *                  the memory mapping number is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetUioMemMapNum(fm_int     num, 
                                 fm_char   *name, 
                                 fm_int    *memMapsNum)
{
    fm_char         path[FM_UIO_MAX_NAME_SIZE];
    struct dirent **namelist;
    int             ret;
    fm_char         barName[FM_UIO_MAX_NAME_SIZE];
    fm_status       status;
    fm_int          mapsNr;
    fm_int          n;
    fm_int          listSize;
    fm_bool         found;

    if (name == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Assuming maps0 is default */
    mapsNr = 0;
    status = FM_OK;
    *memMapsNum = mapsNr;

    /* Check if device is igb_uio */
    ret = strncmp(name, DPDK_DRIVER_UIO_NAME, sizeof(DPDK_DRIVER_UIO_NAME));
    
    if (ret == 0)
    {
        /* Scan maps directory **/
        FM_SNPRINTF_S(path, sizeof(path), "/sys/class/uio/uio%d/maps", num);
        listSize = scandir(path, &namelist, 0, alphasort);
        if (listSize < 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to open %s directory\n",
                         path);
            return FM_ERR_NOT_FOUND;
        }

        /* For igb_uio mapping for BAR4 must be found otherwise report fail */
        status = FM_ERR_NOT_FOUND;
        found = FALSE;

        for (n=0; n<listSize; n++)
        {
            if ( !found )
            {
                FM_SNPRINTF_S(path, sizeof(path), "map%d", mapsNr);
                ret = strncmp(namelist[n]->d_name, path, 4);
                if (ret == 0)
                {
                    /* Check if BAR4 */
                    FM_SNPRINTF_S(path, 
                                  sizeof(path), 
                                  "/sys/class/uio/uio%d/maps/map%d/name",
                                  num,
                                  mapsNr);

                    /* Check if name for the map is BAR4*/
                    if (FM_OK == ReadLineFromFile(path, barName))
                    {
                        ret = strncmp(barName, 
                                      DPDK_DRIVER_BAR4_MEM_MAP_NAME, 
                                      sizeof(DPDK_DRIVER_BAR4_MEM_MAP_NAME));
                        if (ret == 0)
                        {
                            /* BAR4 found */
                            *memMapsNum = mapsNr;
                            found = TRUE;
                            status = FM_OK;
                        }
                    }
                    /* Check next map only if previous found */
                    mapsNr++;
                }
                free(namelist[n]);
            }
            else
            {
                free(namelist[n]);
            }
        }
        free(namelist);
    }

    return status;

}   /* end GetUioMemMapNum */




/*****************************************************************************/
/* GetUioMemAddr
 * \ingroup intPlatform
 *
 * \desc            Read the address stored in file
 *                      /sys/class/uio/uioX/maps/map0/addr  (where X = num)
 *
 * \param[in]       num is the UIO device number on which to operate.
 *
 * \param[in]       memMapNr is memory mapping nr for the UIO device.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  the address read is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetUioMemAddr(fm_int num, fm_int memMapNr, fm_uint32 *addr)
{
    fm_char       filename[FM_UIO_MAX_NAME_SIZE];
    fm_int        ret;
    FILE *        file;
    unsigned long address;

    *addr = 0;

    FM_SNPRINTF_S(filename,
                  sizeof(filename),
                  "/sys/class/uio/uio%d/maps/map%d/addr",
                  num,
                  memMapNr);

    file = fopen(filename,"r");
    if (!file)
        return FM_ERR_NOT_FOUND;

    ret = fscanf(file,"0x%lx", &address);

    fclose(file);

    if (ret < 0)
        return FM_FAIL;

    *addr = address;

    return 0;

}   /* end GetUioMemAddr */




/*****************************************************************************/
/* GetUioMemOffset
 * \ingroup intPlatform
 *
 * \desc            Calculate the offset of mapped memory based on specified 
 *                  number of memory mapping 
 *
 * \param[in]       memMapNr is memory mapping nr for the UIO device.
 *
 * \param[out]      offset points to caller-provided storage into which the
 *                  memory offset is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetUioMemOffset(fm_int memMapsNr, fm_int *offset)
{

    if (offset == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *offset = 0;

    /* Set non-zero offset */
    if (memMapsNr > 0)
    {
        *offset = memMapsNr * getpagesize();
    }

    return FM_OK;

}   /* end GetUioMemOffset */




/*****************************************************************************/
/* GetUioMemSize
 * \ingroup intPlatform
 *
 * \desc            Read the memory size stored in file
 *                      /sys/class/uio/uioX/maps/map0/size  (where X = num)
 *
 * \param[in]       num is the UIO device number on which to operate.
 *
 * \param[out]      size points to caller-provided storage into which the
 *                  the memory size read is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status GetUioMemSize(fm_int num, fm_int memMapNr, fm_int *size)
{
    fm_char       filename[FM_UIO_MAX_NAME_SIZE];
    fm_int        ret;
    FILE *        file;
    unsigned long len;

    FM_SNPRINTF_S(filename,
                  sizeof(filename),
                  "/sys/class/uio/uio%d/maps/map%d/size",
                  num,
                  memMapNr);

    /* Read mem size */
    file = fopen(filename,"r");
    if (!file)
        return FM_ERR_NOT_FOUND;

    ret = fscanf(file,"0x%lx", &len);

    fclose(file);

    if (ret < 0)
        return FM_FAIL;

    *size = len;

    return FM_OK;

}   /* end GetUioMemSize */




/*****************************************************************************/
/* GetUioNumFromFilename
 * \ingroup intPlatform
 *
 * \desc            Extract the uio number from a string of type "uio0".
 *                  0 will be returned for uio0.
 *
 * \param[in]       name points to string (filename).
 *
 * \return          num is the uio number extracted.
 *
 *****************************************************************************/
static int GetUioNumFromFilename(fm_char *name)
{
    enum states { st_u, st_i, st_o, st_num, st_err };
    enum states state = st_u;
    fm_int      i = 0;
    fm_int      num = -1;
    fm_char     ch = name[0];

    /* Search for string starting with "uio" and get the number that follow */
    while (ch && (state != st_err))
    {
        switch (ch)
        {
            case 'u':
                state = (state == st_u) ? st_i : st_err;
                break;
            case 'i':
                state = (state == st_i) ? st_o : st_err;
                break;
            case 'o':
                state = (state == st_o) ? st_num : st_err;
                break;
            default:
                if ( (ch >= '0') && (ch <= '9') && (state == st_num) )
                {
                    num = (num < 0) ? (ch - '0') : ((num * 10) + (ch - '0'));
                }
                else
                {
                    state = st_err;
                }
        }

        i++;
        ch = name[i];
    }

    if (state == st_err)
    {
        num = -1;
    }

    return num;

}   /* end GetUioNumFromFilename */




/*****************************************************************************/
/* GetUioDevInfo
 * \ingroup intPlatform
 *
 * \desc            Read the information associated to a uio device
 *
 * \param[in]       num is the UIO device number on which to operate.
 *
 * \param[out]      info points to caller-provided storage into which the
 *                  the uio device info is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetUioDevInfo(fm_int num, fm_uioDriverInfo *info)
{
    fm_status err;
    fm_int memMapNr;

    /* Get the driver name */
    err = GetUioName(num, info->name);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read UIO driver name\n");
        return err;
    }
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Driver name: %s\n", info->name);

    /* Get memory mapping nr */
    err = GetUioMemMapNum(num, info->name, &memMapNr);
    if (err != FM_OK)
    {
         FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                      "Unable to get number of memory mapping\n");
         return err;
     }

    /* Get the driver version number */
    err = GetUioVersion(num, info->version);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read UIO driver version\n");
        return err;
    }
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Driver version: %s\n", info->version);

    /* Get the driver memory size */
    err = GetUioMemSize(num, memMapNr, &info->size);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read UIO mem size\n");
        return err;
    }
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Driver memsize: 0x%x\n", info->size);

    /* Get the driver memory address */
    err = GetUioMemAddr(num, memMapNr, &info->addr);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read UIO address\n");
        return err;
    }
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Driver address: 0x%x\n", info->addr);

    /* Get the driver memory offset */
    err = GetUioMemOffset(memMapNr, &info->offset);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to get UIO memory offset\n");
        return err;
    }
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Driver memory offset: 0x%x\n", 
                 info->offset);

    return FM_OK;

}   /* end GetUioDevInfo */





/*****************************************************************************/
/* CompareUioNetDevName
 * \ingroup intPlatform
 *
 * \desc            Determine if the net device associated to the given UIO
 *                  corresponds to the given network device.
 *
 * \param[in]       num is the uio device number.
 * 
 * \param[in]       netDevName is the device name to compare with.
 *
 * \return          FM_OK if it corresponds.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 * 
 *
 *****************************************************************************/
static fm_status CompareUioNetDevName(fm_int num, fm_text netDevName)
{
    struct dirent **namelist;
    fm_char         path[FM_UIO_MAX_NAME_SIZE];
    fm_int          n;
    fm_bool         found;
    fm_status       err;
    fm_int          ret;

    if (netDevName == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_SNPRINTF_S(path, sizeof(path), "/sys/class/uio/uio%d/device/net", num);

    n = scandir(path, &namelist, 0, alphasort);
    if (n < 0)
    {
        err = FM_ERR_NOT_FOUND;
        goto ABORT;
    }

    err = FM_ERR_NOT_FOUND;
    found = FALSE;

    while(n--)
    {
        if ( !found )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "%s (num=%d)\n", 
                         namelist[n]->d_name, num);

            ret = strncmp(namelist[n]->d_name, netDevName, 32);
            if (ret == 0)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                             "Found UIO device %d\n",
                             num);
                err = FM_OK;
                found = TRUE;
            }
            free(namelist[n]);
        }
        else
        {
            free(namelist[n]);
        }
    }
    free(namelist);

ABORT:
    return err;

}   /* end CompareUioNetDevName */



/*****************************************************************************/
/* FindUioDeviceFromNetDev
 * \ingroup intPlatform
 *
 * \desc            Find the uio device associated to the given net device.
 *
 * \param[in]       netDevName is the device name on which to operate.
 * 
 * \param[in]       info points to uio driver info structure.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status FindUioDeviceFromNetDev(fm_text           netDevName, 
                                         fm_uioDriverInfo *info)
{
    struct dirent **namelist;
    fm_int          n;
    fm_int          num;
    fm_bool         found;
    fm_status       err;
    fm_int          ret;

    if (info == NULL || netDevName == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    info->uioNum = -1;

    /* Iterate the /sys/class/uio directory and find the uio corresponding
       to the net device name */

    n = scandir("/sys/class/uio", &namelist, 0, alphasort);
    if (n < 0)
    {
        err = FM_ERR_NOT_FOUND;
        goto ABORT;
    }

    err = FM_ERR_NOT_FOUND;
    found = FALSE;

    while(n--)
    {
        if ( !found &&
             (num = GetUioNumFromFilename(namelist[n]->d_name)) >= 0 )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "%s (num=%d)\n", 
                         namelist[n]->d_name, num);
            free(namelist[n]);

            /* See if this UIO corresponds to the net device */
            err = CompareUioNetDevName(num, netDevName);
            if (err == FM_OK)
            {
                /* It corresponds */
                found = TRUE;

                /* Make sure it is the fm10k driver */
                if ( (err = GetUioDevInfo(num, info)) == FM_OK )
                {
                    ret = strncmp(info->name, 
                                  DRIVER_UIO_NAME, 
                                  sizeof(DRIVER_UIO_NAME));
                    if (ret != 0)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, 
                                     "Found UIO name: %s but looking for %s\n",
                                     info->name,
                                     DRIVER_UIO_NAME);
                        err = FM_ERR_NOT_FOUND;
                    }
                    else
                    {
                        info->uioNum = num;
                        err = FM_OK;
                    }
                }
            }
        }
        else
        {
            free(namelist[n]);
        }
    }
    free(namelist);

ABORT:
    return err;

}   /* end FindUioDeviceFromNetDev */




/*****************************************************************************/
/* GetNetDevVendor
 * \ingroup intPlatform
 *
 * \desc            Read the vendor Id stored in file
 *                      /sys/class/net/<netDevName>/device/vendor
 *
 * \param[in]       netDevName points to the net device to analyse.
 *
 * \param[out]      vendorId points to caller-provided storage into which the
 *                  vendor Id read is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status GetNetDevVendor(fm_text netDevName, fm_uint16 *vendorId)
{
    fm_char       filename[FM_NETDEV_MAX_NAME_SIZE];
    fm_int        ret;
    FILE *        file;
    unsigned long vendor;

    FM_SNPRINTF_S(filename,
                  sizeof(filename),
                  "/sys/class/net/%s/device/vendor",
                  netDevName);

    file = fopen(filename,"r");
    if (!file)
        return FM_ERR_NOT_FOUND;

    ret = fscanf(file,"0x%lx", &vendor);

    fclose(file);

    if (ret < 0)
        return FM_FAIL;

    *vendorId = vendor;

    return FM_OK;

}   /* end GetNetDevVendor */




/*****************************************************************************/
/* GetNetDevDevice
 * \ingroup intPlatform
 *
 * \desc            Read the device Id stored in file
 *                      /sys/class/net/<netDevName>/device/device
 *
 * \param[in]       netDevName points to the net device to analyse.
 *
 * \param[out]      deviceId points to caller-provided storage into which the
 *                  device Id read is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status GetNetDevDevice(fm_text netDevName, fm_uint16 *deviceId)
{
    fm_char       filename[FM_NETDEV_MAX_NAME_SIZE];
    fm_int        ret;
    FILE *        file;
    unsigned long device;

    FM_SNPRINTF_S(filename,
                  sizeof(filename),
                  "/sys/class/net/%s/device/device",
                  netDevName);

    file = fopen(filename,"r");
    if (!file)
        return FM_ERR_NOT_FOUND;

    ret = fscanf(file,"0x%lx", &device);

    fclose(file);

    if (ret < 0)
        return FM_FAIL;

    *deviceId = device;

    return FM_OK;

}   /* end GetNetDevDevice */




/*****************************************************************************/
/* GetNetDevSwitchPep
 * \ingroup intPlatform
 *
 * \desc            Find the net device switch and PEP based on VPD information.
 * 
 * \param[in]       netDevName points to the net device to analyse.
 * 
 * \param[out]      netDevSw points to caller-provided storage into which the
 *                  the net device switch is stored.
 * 
 * \param[out]      netDevPep points to caller-provided storage into which the
 *                  the net device PEP is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetNetDevSwitchPep(fm_text netDevName, 
                                    fm_int *netDevSw,
                                    fm_int *netDevPep)
{
    fm_status       err;
    fm_char         filename[FM_NETDEV_MAX_NAME_SIZE];
    fm_byte         byte;
    fm_byte         byte2;
    fm_byte         byte3;
    fm_int          length;
    fm_int          ret;
    FILE *          file;
    fm_char         vpdString[FM_VPD_MAX_STRINNG_SIZE];
    fm_timestamp    ta;
    fm_timestamp    tb;
    fm_timestamp    tdiff;

    if ( (netDevSw == NULL) || (netDevPep == NULL) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_SNPRINTF_S(filename,
                  sizeof(filename),
                  "/sys/class/net/%s/device/vpd",
                  netDevName);

    file = fopen(filename,"r");
    if (!file)
        return FM_ERR_NOT_FOUND;

    /* Parse the VPD information to extract the switch and PEP of the net
     * device. The parser algorithm only manage Large resource data type
     * except for the ending tag. */
    fmGetTime(&ta);

    while (TRUE)
    {
        ret = fscanf(file,"%c", &byte);
        if (ret < 0)
        {
            err = FM_FAIL;
            goto ABORT;
        }
        if (byte == FM_VPD_LARGE_RESOURCE_PROD_NAME)
        {
            ret = fscanf(file,"%c%c", &byte, &byte2);
            if (ret < 0)
            {
                err = FM_FAIL;
                goto ABORT;
            }
            /* Always add 1 to the size to cover the end of string. The buffer
             * is large enough to not overflow even if byte == 255. */
            if (fgets(vpdString, byte + 1, file) == NULL)
            {
                err = FM_FAIL;
                goto ABORT;
            }
        }
        else if ( (byte == FM_VPD_LARGE_RESOURCE_READ) ||
                  (byte == FM_VPD_LARGE_RESOURCE_WRITE) )
        {
            ret = fscanf(file,"%c%c", &byte, &byte2);
            if (ret < 0)
            {
                err = FM_FAIL;
                goto ABORT;
            }
            /* Length encoding is pretty unusual. The first byte is the low
             * order one while the second is the high part. */
            length = (byte2 * 256) + byte;

            /* Extract the field of the LARGE_RESOURCE */
            while (length > 0)
            {
                /* ...+ 1 needed to cover for end of string. */
                if (fgets(vpdString, 2 + 1, file) == NULL)
                {
                    err = FM_FAIL;
                    goto ABORT;
                }
                length -= 2;

                /* Switch is encoded using 2 bytes. First one is always set
                 * but the second is only set if the value exceed 9. */
                if (strcmp(vpdString, "VS") == 0)
                {
                    ret = fscanf(file,"%c%c%c", &byte, &byte2, &byte3);
                    if ( (ret < 0) || (byte != 2))
                    {
                        err = FM_FAIL;
                        goto ABORT;
                    }
                    /* Convert ASCII --> Hexa */
                    if (byte3 != 0x0)
                    {
                        *netDevSw = ((byte2 - 0x30) * 10) + (byte3 - 0x30);
                    }
                    else
                    {
                        *netDevSw = (byte2 - 0x30);
                    }

                    length -= 3;
                }
                /* Pep is encoded using 2 bytes. First one is always set
                 * but the second is only set if the value exceed 9. */
                else if (strcmp(vpdString, "VP") == 0)
                {
                    ret = fscanf(file,"%c%c%c", &byte, &byte2, &byte3);
                    if ( (ret < 0) || (byte != 2))
                    {
                        err = FM_FAIL;
                        goto ABORT;
                    }
                    /* Convert ASCII --> Hexa */
                    if (byte3 != 0x0)
                    {
                        *netDevPep = ((byte2 - 0x30) * 10) + (byte3 - 0x30);
                    }
                    else
                    {
                        *netDevPep = (byte2 - 0x30);
                    }

                    length -= 3;
                }
                /* Other information are parsed but not processed */
                else
                {
                    ret = fscanf(file,"%c", &byte);
                    if (ret < 0)
                    {
                        err = FM_FAIL;
                        goto ABORT;
                    }

                    if (fgets(vpdString, byte + 1, file) == NULL)
                    {
                        err = FM_FAIL;
                        goto ABORT;
                    }
                    length -= (byte + 1);
                }
            }
        }
        else if (byte == FM_VPD_SMALL_RESOURCE_END)
        {
            if ( (*netDevSw >= 0) && (*netDevPep >= 0) )
            {
                err = FM_OK;
            }
            else
            {
                err = FM_FAIL;
            }
            break;
        }
        /* Unreconized tag type */
        else
        {
            err = FM_FAIL;
            goto ABORT;
        }
    }

    fmGetTime(&tb);

    fmSubTimestamps(&tb, &ta, &tdiff);

    if ( ((tdiff.sec * 1000000) + tdiff.usec) > VPD_READ_SLOW_THRESHOLD_USEC )
    {
        FM_LOG_WARNING(FM_LOG_CAT_PLATFORM, 
                       "Detected slow VPD read "
                       "(%" FM_FORMAT_64 "d.%06" FM_FORMAT_64 "d sec) "
                       "for {sw=%d,pep=%d} on netdev=%s\n", 
                       tdiff.sec, 
                       tdiff.usec,
                       *netDevSw, 
                       *netDevPep,
                       netDevName);
        FM_LOG_WARNING(FM_LOG_CAT_PLATFORM, 
                       "A possible cause is a low NVM SPI Speed\n");
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                 "VPD Read Time of %" FM_FORMAT_64 "d.%06" FM_FORMAT_64 "d sec on netdev=%s\n", 
                 tdiff.sec, 
                 tdiff.usec,
                 netDevName);

ABORT:
    fclose(file);
    return err;

}   /* end GetNetDevSwitchPep */




/*****************************************************************************/
/* FindNetDevFromPep
 * \ingroup intPlatform
 *
 * \desc            Find the net device associated to the given pep.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       mgmtPep is the PEP number on which to connect.
 * 
 * \param[out]      netDevName points to caller-provided storage into which the
 *                  the net device name read is stored.
 * 
 * \param[in]       netDevNameLength is the length in bytes of the netDevName
 *                  argument.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status FindNetDevFromPep(fm_int  sw,
                                   fm_int  mgmtPep, 
                                   fm_text netDevName, 
                                   fm_int  netDevNameLength)
{
    struct dirent **namelist;
    fm_int          n;
    fm_bool         found;
    fm_status       err;
    fm_uint16       vendorId = 0;
    fm_uint16       deviceId = 0;
    fm_int          netSw = -1;
    fm_int          netPep = -1;

    if (netDevName == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Iterate the /sys/class/net directory and find the net device
     * corresponding to the management PEP */
    n = scandir("/sys/class/net", &namelist, 0, alphasort);
    if (n < 0)
    {
        err = FM_ERR_NOT_FOUND;
        goto ABORT;
    }

    found = FALSE;

    while(n--)
    {
        if (!found)
        {
            err = GetNetDevVendor(namelist[n]->d_name, &vendorId);
            if ( (err != FM_OK) || (vendorId != FM_INTEL_PCIE_VENDOR_ID) )
            {
                continue;
            }

            err = GetNetDevDevice(namelist[n]->d_name, &deviceId);
            if ( (err != FM_OK) || 
                 ( (deviceId != FM10000_INTEL_PCIE_DEVICE_ID) &&
                   (deviceId != BOULDER_RAPIDS_DEVICE_ID) &&
                   (deviceId != ATWOOD_CHANNEL_DEVICE_ID) ) )
            {
                continue;
            }

            err = GetNetDevSwitchPep(namelist[n]->d_name, &netSw, &netPep);
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "%s (sw=%d, pep=%d)\n", 
                         namelist[n]->d_name,
                         netSw,
                         netPep);
            free(namelist[n]);

            /* Compare the net device sw,pep tuple to the expected one */
            if ( (err == FM_OK) && (sw == netSw) && (mgmtPep == netPep) )
            {
                found = TRUE;
                FM_STRCPY_S(netDevName, netDevNameLength, namelist[n]->d_name);
            }
        }
        else
        {
            free(namelist[n]);
        }
    }
    free(namelist);

    err = (found == TRUE) ? FM_OK : FM_ERR_NOT_FOUND;

ABORT:
    return err;

}   /* end FindNetDevFromPep */




/*****************************************************************************/
/* SetUioInterrupt
 * \ingroup platform
 *
 * \desc            Enable/Disable the interrupt signal from the switch device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       enable 0: disables interrupt, 1: enables interrupt
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status SetUioInterrupt(fm_int sw, fm_int32 enable)
{
    fm_int   rv;
    fm_char  strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t  strErrNum;

    rv = write(GET_PLAT_PROC_STATE(sw)->fd, &enable, sizeof(enable));

    if (rv < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_ERROR(FM_LOG_CAT_EVENT_INTR,
                     "Set (%d) UIO interrupt failed with '%s'\n",
                     enable,
                     strErrBuf);

        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_INTR, FM_FAIL);

    }

    return FM_OK;

}   /* end SetUioInterrupt */




/*****************************************************************************/
/*  FindUioDevice
 * \ingroup intPlatform
 *
 * \desc            Find a uio device associated to the fm10k driver.
 *
 * \param[out]      info points to uio driver info structure.
 *
 * \param[out]      uioDevName points to the UIO device name.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status FindUioDevice(fm_uioDriverInfo *info,
                               fm_char           uioDevName[FM_UIO_MAX_NAME_SIZE])
{
    struct dirent **namelist;
    fm_int          n;
    fm_int          num;
    fm_bool         found;
    fm_status       err;
    fm_int          ret;

    if ( (info == NULL) || (uioDevName == NULL) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    info->uioNum = -1;

    /* Iterate the /sys/class/uio directory and find the uio corresponding
       to the driver name */

    n = scandir("/sys/class/uio", &namelist, 0, alphasort);
    if (n < 0)
    {
        err = FM_ERR_NOT_FOUND;
        goto ABORT;
    }

    err = FM_ERR_NOT_FOUND;
    found = FALSE;

    while(n--)
    {
        if ( !found &&
             (num = GetUioNumFromFilename(namelist[n]->d_name)) >= 0 )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "%s (num=%d)\n", 
                         namelist[n]->d_name, num);

            /* Make sure it is the fm10k driver */
            if ( (err = GetUioDevInfo(num, info)) == FM_OK )
            {
                ret = strncmp(info->name, 
                              DRIVER_UIO_NAME, 
                              sizeof(DRIVER_UIO_NAME));
                if (ret == 0)
                {
                    info->uioNum = num;
                    found = TRUE;
                    FM_SPRINTF_S(uioDevName,
                                 FM_UIO_MAX_NAME_SIZE,
                                 "/dev/%s",
                                 namelist[n]->d_name);
                }
            }
        }
        
        free(namelist[n]);
    }
    free(namelist);

    if (found == TRUE)
    {
        err = FM_OK;
    }
    else
    {
        err = FM_ERR_NOT_FOUND;
    }
    

ABORT:
    return err;

}   /* end FindUioDevice */




/*****************************************************************************/
/* OpenUioDevice
 * \ingroup intPlatform
 *
 * \desc            Open file /dev/uioX (where X = num) and
 *                  return the corresponding file descriptor.
 *
 * \param[in]       num is the UIO device number on which to operate.
 * 
 * \param[in]       devName is the optional UIO device name.
 *
 * \return          fd is the file descriptor
 *
 *****************************************************************************/
static fm_int OpenUioDevice(fm_int num, fm_text devName)
{
    fm_char filename[FM_UIO_MAX_NAME_SIZE];
    fm_int  fd;
    fm_char strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t strErrNum;

    if (devName != NULL)
    {
        /* Use the provided device name */
        FM_STRCPY_S(filename, sizeof(filename), devName);
    }
    else
    {
        FM_SPRINTF_S(filename, sizeof(filename), "/dev/uio%d", num);
    }

    /* Open the UIO device. */
    fd = open(filename, O_RDWR);

    if (fd < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf,
                                  FM_STRERROR_BUF_SIZE,
                                  errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to open '%s' - '%s'\n",
                     filename,
                     strErrBuf);
    }

    return fd;

}   /* end OpenUioDevice */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformHostDrvOpen
 * \ingroup platform
 *
 * \desc            Establish connection with to host driver for the given
 *                  switch and perform the memory map of the switch memory.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       mgmtPep is the PEP number on which to connect.
 * 
 * \param[in]       netDevName is the optional net device name provided in the
 *                  configuation file.
 * 
 * \param[in]       uioDevName is the optional uio device name provided in the
 *                  configuation file.
 * 
 * \param[in]       desiredMemmapAddr is the desired memory address.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformHostDrvOpen(fm_int  sw,
                                fm_int  mgmtPep, 
                                fm_text netDevName, 
                                fm_text uioDevName, 
                                void *  desiredMemmapAddr)
{
    fm_platformProcessState *pp;
    fm_uioDriverInfo *       info;
    fm_status                err;
    void *                   addr;
    fm_char                  localNetDevName[FM_NETDEV_MAX_NAME_SIZE];
    fm_char                  strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t                  strErrNum;
    fm_platformCfgSwitch *   swCfg;
    fm_char *                startUioName;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "sw = %d mgmtPep = %d netDevName = %s uioDevName = %s memmapAddr = %p\n", 
                 sw,
                 mgmtPep,
                 netDevName,
                 uioDevName,
                 (void *)desiredMemmapAddr);

    pp = GET_PLAT_PROC_STATE(sw);

    if (pp->fd >= 0)
    {
        /* Already connected to host driver */
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    info = &pp->uioInfo;

    if (uioDevName != NULL)
    {
        /* Use the provide device name */
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "Using %s device\n", 
                     uioDevName);

        /* Search uio string in uio device name */
        startUioName = strstr(uioDevName, UIO_DEV_NAME_PREFIX);
        if(startUioName == NULL)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to localize uio string for uio device: %s\n",
                         uioDevName);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
        }

        /* Get uio number from uio device name */
        info->uioNum = GetUioNumFromFilename(startUioName);
        if(info->uioNum < 0)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                         "Unable to specify uio nr for uio device: %s\n",
                         uioDevName);
            info->uioNum = 0;
        }

        /* Get information for uio device */
        if ( (err = GetUioDevInfo(info->uioNum, info)) != FM_OK )
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to get info for uio device: %s\n",
                         uioDevName);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
        }

        FM_LOG_PRINT("Connect to host driver using %s\n", uioDevName);
        err = FM_OK;
    }
    else
    {
        if (netDevName == NULL)
        {
            /* Try to find the proper net device based on the management PEP
             * entered in the config file */
            err = FindNetDevFromPep(sw,
                                    mgmtPep,
                                    localNetDevName,
                                    FM_NETDEV_MAX_NAME_SIZE);
            if (err == FM_OK)
            {
                /* The netdev device has been found */
                FM_LOG_PRINT("Found netdev %s(derived from mgmt pep %d sw %d) \n", 
                             localNetDevName,
                             mgmtPep,
                             sw);
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                             "netdev not found for mgmt pep %d\n",
                             mgmtPep);
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
            }

            /* Update the netdev field with the device found */
            swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
            FM_STRNCPY_S(swCfg->netDevName,
                         sizeof(swCfg->netDevName),
                         localNetDevName,
                         sizeof(swCfg->netDevName));
        }
        else
        {
            FM_STRCPY_S(localNetDevName, sizeof(localNetDevName), netDevName);
        }
        /* Derive the UIO number from the network device name */
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "Derive UIO number from netDev %s\n", 
                     localNetDevName);
        err = FindUioDeviceFromNetDev(localNetDevName, info);

        if (err == FM_OK)
        {
            /* The uio device has been found */
            FM_LOG_PRINT("Connect to host driver using uio%d "
                         "(derived from netdev %s sw %d)\n", 
                         info->uioNum,
                         localNetDevName,
                         sw);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "UIO driver not found for netdev %s\n",
                         localNetDevName);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
        }
    }

    /* Open the UIO device */
    pp->fd = OpenUioDevice(info->uioNum, uioDevName);

    if (pp->fd < 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Unable to open uio%d driver\n",
                     info->uioNum);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    /* Memory map the switch memory */
    addr = mmap(desiredMemmapAddr,
                info->size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                pp->fd,
                info->offset);

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
    else if (addr != desiredMemmapAddr)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Can't map switch#%d memory - "
                     "desired address was %p but got %p\n",
                     sw,
                     desiredMemmapAddr,
                     addr);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    GET_PLAT_STATE(sw)->switchMem = addr;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformHostDrvOpen */




/*****************************************************************************/
/* fmPlatformHostDrvClose
 * \ingroup platform
 *
 * \desc            Close connection with to host driver for the given
 *                  switch and perform the memory unmap of the switch memory.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformHostDrvClose(fm_int sw)
{
    fm_platformProcessState *pp;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    pp = GET_PLAT_PROC_STATE(sw);

    if (pp->fd >= 0)
    {
        /* Memory unmap for the switch memory */
        munmap((void *)GET_PLAT_STATE(sw)->switchMem, pp->uioInfo.size);

        /* Close connection */
        close(pp->fd);
        pp->fd = -1;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformHostDrvClose */




/*****************************************************************************/
/* fmPlatformHostDrvEnableInterrupt
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
fm_status fmPlatformHostDrvEnableInterrupt(fm_int sw, fm_uint intrTypes)
{
    fm_status status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_INTR,
                         "sw = %d, intrTypes = %u\n",
                         sw,
                         intrTypes);

    if (GET_PLAT_PROC_STATE(sw)->fd >= 0 && 
        intrTypes & FM_INTERRUPT_SOURCE_ISR)
    {
        /* Enable interrupt */
        status = SetUioInterrupt(sw, 1);
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_INTR, status);

}   /* end fmPlatformHostDrvEnableInterrupt */




/*****************************************************************************/
/* fmPlatformHostDrvDisableInterrupt
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
fm_status fmPlatformHostDrvDisableInterrupt(fm_int sw, fm_uint intrTypes)
{
    fm_status status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_INTR,
                         "sw = %d, intrTypes = %u\n",
                         sw,
                         intrTypes);

    if (GET_PLAT_PROC_STATE(sw)->fd >= 0 && 
        intrTypes & FM_INTERRUPT_SOURCE_ISR)
    {
        /* Disable interrupt */
        status = SetUioInterrupt(sw, 0);
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_INTR, status);

}   /* end fmPlatformHostDrvDisableInterrupt */




/*****************************************************************************/
/* fmPlatformHostDrvWaitForInterrupt
 * \ingroup platform
 *
 * \desc            Wait for an interrupt pending for a maximum of
 *                  timeout seconds
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       timeout is the waiting period in second.
 * 
 * \param[out]      intrStatus points to caller-provided storage into which the
 *                  interrupt status is stored. Upon timeout 0 is returned.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformHostDrvWaitForInterrupt(fm_int   sw, 
                                            fm_int   timeout,
                                            fm_uint *intrStatus)
{
    fm_status status;
    fm_int32  irqCount;
    int       rv;
    int       fd;
    fd_set    rfds;
    struct    timeval tv;
    fm_char   strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t   strErrNum;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_INTR,
                         "sw = %d, intrStatus = %p\n",
                         sw,
                         (void*) intrStatus);

    fd = GET_PLAT_PROC_STATE(sw)->fd;

    if (fd < 0)
    {
        /* Not yet connected to the host driver */
        *intrStatus = 0;
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_INTR, FM_FAIL);
    }

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    /* See if the UIO dev has an interrupt pending */
    rv = select(fd+1, &rfds, NULL, NULL, &tv);

    if (rv > 0)
    {
        if (FD_ISSET(fd, &rfds) == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_INTR, "ERROR: No data available");
            *intrStatus = 0;
            FM_LOG_EXIT(FM_LOG_CAT_EVENT_INTR, FM_FAIL);
        }

        /* Perform the read on UIO device */
        rv = read(fd, &irqCount, 4);

        if (rv == 4)
        {
            *intrStatus = 1;
            status = FM_OK;
        }
        else
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

            if (strErrNum)
            {
                FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
            }

            FM_LOG_ERROR(FM_LOG_CAT_EVENT_INTR,
                         "Reading UIO interrupt failed with '%s'\n",
                         strErrBuf);

            *intrStatus = 0;
            status = FM_FAIL;
        }
    }
    else if (rv < 0)
    {
        /* Error */
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_ERROR(FM_LOG_CAT_EVENT_INTR,
                     "Fail on select() with '%s'\n",
                     strErrBuf);

        *intrStatus = 0;
        status = FM_FAIL;
    }
    else
    {
        /* No data within timeout period */
        *intrStatus = 0;
        status = FM_OK;
    }


    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_INTR, status);

}   /* end fmPlatformHostDrvWaitForInterrupt */




/*****************************************************************************/
/* fmPlatformMmapUioDevice
 * \ingroup intPlatform
 *
 * \desc            Map memory for a uio device.
 *
 * \param[in]       devName is the UIO device name.
 *
 * \param[out]      fd points to caller-provided storage into which the
 *                  the file descriptor is stored.

 * \param[out]      memmapAddr points to caller-provided storage into which
 *                  the pointer to the mapped area is stored.
 *
 * \param[out]      size points to caller-provided storage into which the
 *                  the size of the mapped area is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMmapUioDevice(fm_text devName, 
                                  fm_int *fd, 
                                  void ** memmapAddr,
                                  fm_int *size)
{
    fm_uioDriverInfo info;
    fm_char          uioDevName[FM_UIO_MAX_NAME_SIZE];
    fm_int           num;
    fm_status        err;
    fm_char          strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t          strErrNum;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "devName = %p fd = %p memmapAddr = %p size = %p\n",
                 (void*)devName,
                 (void*)fd,
                 (void*)memmapAddr,
                 (void*)size);

    if (devName == NULL)
    {
        err = FindUioDevice(&info, uioDevName);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);        
    }
    else
    {
        num = GetUioNumFromFilename(devName);
        if (num < 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, 
                         "Could not get the UIO device number from UIO device name: %s\n",
                         devName);
            
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
        }
    
        info.uioNum = num;
        err = GetUioDevInfo(num, &info);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    *fd = OpenUioDevice(info.uioNum, NULL);
    if (*fd < 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Unable to open  device uio-%d\n",
                     info.uioNum);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }




    *memmapAddr =
        mmap(NULL,
             info.size,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             *fd,
             0);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                 "Memory mapped to address %p\n",
                 *memmapAddr);

    if (*memmapAddr == MAP_FAILED)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: Can't map switch memory - %d %s\n",
                     errno,
                     strErrBuf);

        close(*fd);

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    *size = info.size;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* fmPlatformMmapUioDevice */




/*****************************************************************************/
/* fmPlatformUnmapUioDevice
 * \ingroup intPlatform
 *
 * \desc            Unmap memory for a uio device.
 *
 * \param[in]       fd is the file descriptor
 *
 * \param[in]       memmapAddr is a pointer to the mapped area.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformUnmapUioDevice(fm_int fd, void *memmapAddr, fm_int size)
{
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "fd = %d memmapAddr = %p size = %d\n",
                 fd,
                 memmapAddr,
                 size);

    munmap(memmapAddr, size);
    close(fd);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* fmPlatformUnmapUioDevice */
