/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_ipmi.c
 * Creation Date:   January 21, 2013
 * Description:     Functions to access I2C via IPMI
 *
 * Copyright (c) 2013, Intel Corporation
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

#include <fm_sdk.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/ipmi.h>

/* This file is being shared also with other utils programs */

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/*****************************************************************************
 * It seems a delay is required to avoid the problem of getting incorrect
 * data from previous command.
 * Below is the scope trace of the I2C command reading PCA9505 at addresses
 * 0x80, 0x88, 0x98, 0xA0 continously. Intermittenly data from the current
 * register offset will show up with previous register data. And the scope
 * trace shows the previous transaction repeated twice. Not sure where is the
 * root of the issue, but by adding a delay the problem goes away.
 * 4.40E-02	  	 Write	20	A0
 * 4.42E-02	X	Read	20	00 00 FF FF 10
 * 4.81E-02	  	 Write	20	80
 * 4.83E-02	X	Read	20	FF BB BB 00 FC
 * 5.19E-02	  	 Write	20	88
 * 5.21E-02	X	Read	20	88 00 00 00 10
 * 6.28E-02	  	 Write	20	88
 * 6.30E-02	X	Read	20	88 00 00 00 10
 * 7.77E-02	  	 Write	20	98
 * 7.79E-02	X	Read	20	77 BB BB 00 EC
 * 8.06E-02	  	 Write	20	A0
 * 8.08E-02	X	Read	20	00 00 FF FF 10
 ******************************************************************************/
#define ENABLE_DELAY
#define MIN_DELAY            5000


#define DBG_MGMT_IPMI        1
#define DBG_MGMT_IPMI2       2

#define MAX_IPMI_RESP_BYTES  64
#define MAX_IPMI_DATA_BYTES  64 + 1

/* Special return code to indicate retry */
#define STATUS_IPMI_RETRY    0x12345678
#define MAX_IPMI_RETRY_CNT   3

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_int        ipmiDebug = 0;

static unsigned long ipmiSeqNum = 0;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/* DumpRequestMessage
 * \ingroup intPlatformIpmi
 *
 * \desc            Dump IPMI request message.
 *
 * \param[in]       req is the IPMI request message.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void DumpRequestMessage(struct ipmi_req req)
{
    fm_int cnt;

    FM_LOG_PRINT("  msgid     = %ld\n", req.msgid);
    FM_LOG_PRINT("  netfn     = 0x%x\n", req.msg.netfn);
    FM_LOG_PRINT("  cmd       = 0x%x\n", req.msg.cmd);
    FM_LOG_PRINT("  data_len  = %d\n", req.msg.data_len);
    FM_LOG_PRINT("  data      = ");

    for (cnt = 0 ; cnt < req.msg.data_len ; cnt++)
    {
        FM_LOG_PRINT("%02x", req.msg.data[cnt]);
    }

    FM_LOG_PRINT("\n");

} /* DumpRequestMessage */




/*****************************************************************************/
/* DumpRequestMessage
 * \ingroup intPlatformIpmi
 *
 * \desc            Dump IPMI receive message.
 *
 * \param[in]       recv is the IPMI receive message.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void DumpReceiveMessage(struct ipmi_recv recv)
{
    fm_int cnt;

    FM_LOG_PRINT("Got message:\n");
    FM_LOG_PRINT("  type      = %d\n", recv.recv_type);
    FM_LOG_PRINT("  msgid     = %ld\n", recv.msgid);
    FM_LOG_PRINT("  netfn     = 0x%x\n", recv.msg.netfn);
    FM_LOG_PRINT("  cmd       = 0x%x\n", recv.msg.cmd);

    if (recv.msg.data_len)
    {
        FM_LOG_PRINT("  data_len  = %d\n", recv.msg.data_len);
        FM_LOG_PRINT("  data      = ");

        for (cnt = 0 ; cnt < recv.msg.data_len ; cnt++)
        {
            FM_LOG_PRINT("%02x", recv.msg.data[cnt]);
        }

        FM_LOG_PRINT("\n");
    }

} /* DumpReceiveMessage */




/*****************************************************************************/
/* GetTimeIntervalMsec
 * \ingroup intPlatformIpmi
 *
 * \desc            Return the time interval between two timestamps.
 *
 * \param[in]       begin is the begin timestamp.
 *
 * \param[in]       end is the end timestamp. If NULL, use the current timestamp
 *
 * \return          Number of milli seconds.
 *
 *****************************************************************************/
static fm_uint GetTimeIntervalMsec(struct timeval *begin, struct timeval *end)
{
    struct timeval endT;
    struct timeval diff;

    if (end == NULL)
    {
        gettimeofday(&endT, NULL);
    }
    else
    {
        endT.tv_sec  = end->tv_sec;
        endT.tv_usec = end->tv_usec;
    }

    diff.tv_sec  = endT.tv_sec - begin->tv_sec;
    diff.tv_usec = endT.tv_usec - begin->tv_usec;

    return diff.tv_sec * 1000 * 1000 + diff.tv_usec;

}   /* end GetTimeIntervalMsec */




/*****************************************************************************/
/* fmPlatformIpmiWriteReadInt2
 * \ingroup intPlatformIpmi
 *
 * \desc            Write to then immediately read from an I2C device.
 *
 * \param[in]       fd is the file handle on which to operate.
 *
 * \param[in]       device is the I2C device address (0x00 - 0x7F).
 *
 * \param[in,out]   data points to an array from which data is written and
 *                  into which data is read.
 *
 * \param[in]       wl is the number of bytes to write.
 *
 * \param[in]       rl is the number of bytes to read.
 *
 * \return          FM_OK if successful.
 * \return          STATUS_IPMI_RETRY, special status to indicate
 *                  that the caller should retry.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status fmPlatformIpmiWriteReadInt2(int      fd,
                                             fm_int   device,
                                             fm_byte *data,
                                             fm_int   wl,
                                             fm_int   rl)
{
    fm_status                         status = FM_OK;
    fm_int                            cnt;
    fm_byte                           bytes[MAX_IPMI_DATA_BYTES];
    fm_char                           strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t                           strErrNum;

    struct ipmi_recv                  recv;
    struct ipmi_addr                  addr;
    struct ipmi_system_interface_addr bmc_addr;
    struct ipmi_req                   req;
    fd_set                            rset;

#ifdef ENABLE_DELAY
    static struct timeval             lastTime;
    fm_uint                           interval;

    interval = GetTimeIntervalMsec(&lastTime, NULL);

    if (interval < MIN_DELAY)
    {
        usleep(MIN_DELAY - interval);
    }

#endif


    bmc_addr.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
    bmc_addr.channel   = IPMI_BMC_CHANNEL;
    bmc_addr.lun       = 0;
    req.addr           = (unsigned char *) &bmc_addr;
    req.addr_len       = sizeof(bmc_addr);


    req.msgid     = ipmiSeqNum++;
    req.msg.netfn = IPMI_NETFN_APP_REQUEST;
    req.msg.cmd   = 0x52;

    bytes[0] = (device >> 8) & 0xFF; /* in case we want to override the default bus number */

    if (bytes[0] == 0)
    {
        bytes[0] = 0x5;                /* bus 2 of the BMC */
    }

    bytes[1] = (device & 0xFF) << 1;
    bytes[2] = rl;

    for (cnt = 0 ; cnt < wl ; cnt++)
    {
        bytes[3 + cnt] = data[cnt];
    }

    req.msg.data     = bytes;
    req.msg.data_len = 3 + wl;

    if (ipmiDebug & DBG_MGMT_IPMI)
    {
        FM_LOG_PRINT("### Request: dev 0x%x data=%02x wl %d rl %d\n", device, data[0], wl, rl);
        DumpRequestMessage(req);
    }

    if (ioctl(fd, IPMICTL_SEND_COMMAND, &req) < 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "ERROR: Send command\n");
        return FM_ERR_I2C_NO_RESPONSE;
    }

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    if (select(fd + 1, &rset, NULL, NULL, NULL) < 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "ERROR: I/O Error\n");
        return FM_ERR_I2C_NO_RESPONSE;
    }

    if (FD_ISSET(fd, &rset) == 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "ERROR: No data available");
        return FM_ERR_I2C_NO_RESPONSE;
    }

    recv.addr         = (unsigned char *) &addr;
    recv.addr_len     = sizeof(addr);
    recv.msg.data     = bytes;
    recv.msg.data_len = MAX_IPMI_DATA_BYTES;

    if (ioctl(fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv) < 0)
    {
        if (errno == EAGAIN)
        {
            return STATUS_IPMI_RETRY;
        }

        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "ERROR(%s): ioctl receive message\n", strErrBuf);
        return FM_ERR_I2C_NO_RESPONSE;
    }

#ifdef ENABLE_DELAY
    gettimeofday(&lastTime, NULL);
#endif

    if (ipmiDebug & DBG_MGMT_IPMI)
    {
        DumpReceiveMessage(recv);
    }

    if (recv.msg.data[0])
    {
        return FM_ERR_I2C_NO_RESPONSE;
    }

    if (req.msgid != recv.msgid)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Request msgid %ld is not the same as receive msgid %ld\n",
                     req.msgid, recv.msgid);
        return FM_ERR_I2C_NO_RESPONSE;
    }

    if (recv.recv_type != IPMI_RESPONSE_RECV_TYPE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Request recv_type %d is not response type\n",
                     recv.recv_type);
        return FM_ERR_I2C_NO_RESPONSE;
    }


    if ( ipmiDebug &&
        ( ( rl != (recv.msg.data_len - 1) ) || (req.msgid != recv.msgid) ) )
    {
        if ( rl != (recv.msg.data_len - 1) )
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Expected read length %d is not the same as received length %d. wl = %d\n",
                         rl, recv.msg.data_len - 1, wl);
        }

        if ( !(ipmiDebug & DBG_MGMT_IPMI) )
        {
            FM_LOG_PRINT("### Request: dev 0x%x data=%02x wl %d rl %d\n", device, data[0], wl, rl);
            DumpRequestMessage(req);
        }

        DumpReceiveMessage(recv);
    }

    /* Not sure about how to handle IPMI return valid status but no data */
    if (rl != recv.msg.data_len - 1)
    {
        return STATUS_IPMI_RETRY;
    }

    if (recv.msg.data[0])
    {
        return FM_ERR_I2C_NO_RESPONSE;
    }

    for (cnt = 0 ; cnt < rl ; cnt++)
    {
        data[cnt] = recv.msg.data[cnt + 1];
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformIpmiWriteReadInt2 */




/*****************************************************************************/
/* fmPlatformIpmiWriteReadInt
 * \ingroup intPlatformIpmi
 *
 * \desc            Write to then immediately read from an I2C device with retries.
 *
 * \param[in]       fd is the file handle on which to operate.
 *
 * \param[in]       device is the I2C device address (0x00 - 0x7F).
 *
 * \param[in,out]   data points to an array from which data is written and
 *                  into which data is read.
 *
 * \param[in]       wl is the number of bytes to write.
 *
 * \param[in]       rl is the number of bytes to read.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status fmPlatformIpmiWriteReadInt(int      fd,
                                            fm_int   device,
                                            fm_byte *data,
                                            fm_int   wl,
                                            fm_int   rl)
{
    fm_status status;
    fm_int    cnt;

    for (cnt = 0 ; cnt < MAX_IPMI_RETRY_CNT ; cnt++)
    {
        status = fmPlatformIpmiWriteReadInt2(fd, device, data, wl, rl);

        if (status != STATUS_IPMI_RETRY)
        {
            break;
        }

        /* Don't return STATUS_IPMI_RETRY error code */
        status = FM_ERR_I2C_NO_RESPONSE;
    }

    return status;

} /* end fmPlatformIpmiWriteReadInt */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformIpmiSetDebug
 * \ingroup intPlatformIpmi
 *
 * \desc            This function sets ipmiDebug value.
 *
 * \param[in]       value is the debug value.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPlatformIpmiSetDebug(fm_int value)
{
    FM_LOG_PRINT("impiDebug is set to 0x%x from 0x%x\n", value, ipmiDebug);
    ipmiDebug = value;

} /* end fmPlatformIpmiSetDebug */




/*****************************************************************************/
/* fmPlatformIpmiInit
 * \ingroup intPlatformIpmi
 *
 * \desc            This function opens the IPMI device to access the
 *                  platform peripheral via I2C.
 *
 * \param[in]       devName is the name of the device to open.
 *
 *
 * \param[in]       fd points to caller allocated storage where the file
 *                  descriptor of the ipmi device will be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformIpmiInit(fm_text devName, int *ipmiFd)
{
    int          fd;
    unsigned int myAddr = IPMI_BMC_SLAVE_ADDR;
    fm_char      strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t      strErrNum;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "ipmiFd = %p\n", (void *) ipmiFd);

    if (!ipmiFd)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    *ipmiFd = -1;

    fd = open(devName, O_RDWR);

    if (fd < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unable to open '%s' - '%s'\n",
                     devName,
                     strErrBuf);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    if (ioctl(fd, IPMICTL_SET_MY_ADDRESS_CMD, &myAddr) < 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Could not set IPMB address to 0x%x",
                     myAddr);
        close(fd);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    *ipmiFd = fd;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformIpmiInit */




/*****************************************************************************/
/* fmPlatformIpmiWriteRead
 * \ingroup intPlatformIpmi
 *
 * \desc            Write to then immediately read from an I2C device with
 *                  handling max response bytes from IPMI driver.
 *
 * \param[in]       fd is the file descriptor to the IPMI device.
 *
 * \param[in]       device is the I2C device address (0x00 - 0x7F).
 *
 * \param[in,out]   data points to an array from which data is written and
 *                  into which data is read.
 *
 * \param[in]       wl is the number of bytes to write.
 *
 * \param[in]       rl is the number of bytes to read.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformIpmiWriteRead(int      fd,
                                  fm_int   device,
                                  fm_byte *data,
                                  fm_int   wl,
                                  fm_int   rl)
{
    fm_status status = FM_OK;
    fm_int    len;
    fm_int    nWr;
    fm_int    nRd;

    if (fd < 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "Invalid file descriptor\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (rl < MAX_IPMI_RESP_BYTES)
    {
        return fmPlatformIpmiWriteReadInt(fd, device, data, wl, rl);
    }

    if (wl > 1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "ERROR: No support for wl %d and rl %d\n", wl, rl);
        return FM_ERR_INVALID_ARGUMENT;
    }

    len = 0;
    nWr = wl;

    while (len < rl)
    {
        nRd = rl - len;

        if (nRd > MAX_IPMI_RESP_BYTES)
        {
            nRd = MAX_IPMI_RESP_BYTES;
        }

        if ( ( status = fmPlatformIpmiWriteReadInt(fd, device, data + len, nWr, nRd) ) )
        {
            return status;
        }

        nWr  = 0;
        len += nRd;
    }

    return status;

} /* end fmPlatformIpmiWriteRead */
