/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_device_lock.c
 * Creation Date:   June 4, 2013
 * Description:     Device locking functions.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <fm_std.h>
#include <common/fm_common.h>
#include <fm_alos_logging.h>

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
/* fmUtilDeviceLockIsTaken
 * \ingroup platformUtils
 *
 * \desc            Check if POSIX lock is taken on a open file.
 *
 * \param[in]       fd is the file handle.
 *
 * \return          TRUE if locked is locked by another process.
 *
 *****************************************************************************/
fm_bool fmUtilDeviceLockIsTaken(int fd)
{
    if (lockf(fd, F_TEST, 0))
    {
        return TRUE;
    }

    return FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
   
}   /* end fmUtilDeviceLockIsTaken */




/*****************************************************************************/
/* fmUtilDeviceLockTake
 * \ingroup platformUtils
 *
 * \desc            Take a POSIX lock on a opened file.
 *                  This is used on platforms where there are resources
 *                  conflict between core switch features and system features.
 *                  The SDK platform code and system utilities application
 *                  must take this lock to access shared resources.
 *
 * \param[in]       fd is the file handle.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmUtilDeviceLockTake(int fd)
{
    char           strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t        strErrNum;

    if (lockf(fd, F_LOCK, 0))
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_FATAL( FM_LOG_CAT_PLATFORM,
                     "%s: Failed to take lock\n",
                     strErrBuf );
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
   
}   /* end fmUtilDeviceLockTake */




/*****************************************************************************/
/* fmUtilDeviceLockDrop
 * \ingroup platformUtils
 *
 * \desc            Drop a POSIX lock on a open file.
 *
 * \param[in]       fd is the file handle.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmUtilDeviceLockDrop(int fd)
{
    char           strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t        strErrNum;

    if (lockf(fd, F_ULOCK, 0))
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_FATAL( FM_LOG_CAT_PLATFORM,
                     "%s: Failed to drop lock\n",
                     strErrBuf );
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
   
}   /* end fmUtilDeviceLockDrop */

