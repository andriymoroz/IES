/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_init.c
 * Creation Date:   September 16, 2005
 * Description:     Initialization for ALOS module.
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

#include <fm_sdk_int.h>
#include <dlfcn.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

fm_rootAlos *fmRootAlos = NULL;

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
/** fmAlosRootInit
 * \ingroup intAlosInit
 *
 * \desc            Initialize ALOS state.
 *
 * \param[in]       None.
 *
 * \return          FM_OK if successful. 
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
static fm_status fmAlosRootInit(void)
{
    fm_status           err;
    int                 ret;
    pthread_mutexattr_t attr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "(no arguments)\n");

    /* Allocate memory for ALOS state. */
    fmRootAlos = fmAlloc( sizeof(fm_rootAlos) );

    if (fmRootAlos == NULL)
    {
        /* Could not get memory. */
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
    }

    err = fmAlosLockInit();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ALOS, err);

    if ( pthread_mutexattr_init(&attr) != 0 )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_LOCK_INIT);
    }

    /*
     * From this point on, attr has been initialized, and we may ABORT.
     */

    if ( pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0 )
    {
        err = FM_ERR_LOCK_INIT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_SEM, err);
    }

    ret = pthread_mutex_init(&fmRootAlos->treeTreeLock, &attr);
    if (ret != 0)
    {
        err = FM_ERR_LOCK_INIT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_DEBUG, err);
    }

    /* Initialize the tree of trees */
    fmTreeInit(&fmRootAlos->treeTree);

    err = fmAlosRwlockInit();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

    err = fmAlosSemInit();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

    err = fmAlosLoggingInit();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

    err = fmInitializeApiProperties();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

    err = fmInitDynamicLoadLibs();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

    err = fmAlosRandInit();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

    err = fmAlosTimeInit();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);

ABORT:
    pthread_mutexattr_destroy(&attr);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);

}   /* end fmAlosRootInit */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmOSInitialize
 * \ingroup alosInit
 *
 * \desc            Initialize the operating system abstraction module.
 *                                                                      \lb\lb
 *                  One of the design goals of the API is to have it
 *                  support multiple simultaneous client processes. To this
 *                  end, it is necessary that a common API state be shared
 *                  among all calling processes. This is achieved by 
 *                  sharing the API's data memory so it is accessible from
 *                  any calling process. This function calls shmget and shmat
 *                  for this purpose. 
 *
 * \param           None.
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fmOSInitialize()
{
    fm_status      err;
    static fm_bool initializedAlready = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "(no arguments)\n");

    if (initializedAlready)
    {
        /* ALOS has already been initialized. */
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);
    }
    else
    {
        initializedAlready = TRUE;
    }

    /* Map API memory as shared for the calling process. */
    err = fmMemInitialize();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ALOS, err);

    /* Register the calling proccess */
    err = fmAlosThreadInit();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ALOS, err);

    /* Initialize fmRootAlos */
    err = fmGetRoot("alos", (void **) &fmRootAlos, fmAlosRootInit);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ALOS, err);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);

}   /* end fmOSInitialize */


/*****************************************************************************/
/** fmInitProcess
 * \ingroup alosInit
 *
 * \desc            Performs per-calling process ALOS initialization.
 *                  Presently, all that is done is to initialize thread
 *                  local storage, which is needed by the lock inversion
 *                  defense.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmInitProcess(void)
{
    fm_status err;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "(no arguments)\n");
    
    /* Initialize thread lock collection */
    err = fmInitThreadLockCollection();
    
    FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    
}   /* end fmInitProcess */



