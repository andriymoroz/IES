/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_alos_rand.c
 * Creation Date:  January 30, 2013
 * Description:    ALOS routines for dealing with rand abstractly
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
static pthread_mutex_t rand_lock;
static fm_uint         rand_seed = 1;
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
/** fmRand
 * \ingroup intAlosRand
 *
 * \desc            pseudo-random number generator 
 *
 * \return          a pseudo-random integer between 0 and RAND_MAX inclusive
 *                  (i.e., the mathematical range [0, RAND_MAX]). 
 *
 *
 *****************************************************************************/
fm_int fmRand(void)
{ 
    fm_int randResult;
    int ret;

    ret = pthread_mutex_lock(&rand_lock);
    if (ret == 0)
    {
        randResult = rand_r(&rand_seed);
        ret = pthread_mutex_unlock(&rand_lock);
        if (ret != 0)
        {
             FM_LOG_ERROR(FM_LOG_CAT_ALOS,
                          "Error %d from pthread_mutex_unlock\n",
                          ret);
        }
    }
    else
    {
        randResult = 0;
        FM_LOG_ERROR(FM_LOG_CAT_ALOS,
                     "Error %d from pthread_mutex_lock\n",
                     ret);
    }
    return randResult;
}


/*****************************************************************************/
/** fmAlosRandInit
 * \ingroup intAlosRand
 *
 * \desc            Initializes the rand subsystem.
 *
 * \note            This is an internal function that should only be called
 *                  from fm_alos_init.c.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmAlosRandInit(void)
{

    if (pthread_mutex_init(&rand_lock, NULL) != 0)
    {
         FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_LOCK_INIT);
    }
    return FM_OK;
}
