/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_dynamic_load.c
 * Creation Date:   Mar. 14, 2011
 * Description:     This file implements dynamic load library support.
 *
 * Copyright (c) 2011 - 2014, Intel Corporation
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
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/
/* This variable is deliberately defined here because it must be local and
 * unique to each process. Do NOT move it into shared-memory! */
fm_uint64 fmProcessDynLoadLibStatus = FM_LITERAL_U64(0);


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
/* This variable is deliberately defined here because it must be local and
 * unique to each process. Do NOT move it into shared-memory! */
static void *ProcessHandles[FM_ALOS_INTERNAL_DYN_LOAD_LIBS] = { NULL };

 
/*****************************************************************************
 * Local Functions
 *****************************************************************************/
 

/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmInitDynamicLoadLibs
 * \ingroup intAlosDynLoadLib
 *
 * \desc            Initialize ALOS Dynamic Load Library support.
 *
 * \param[in]       None.
 *
 * \return          FM_ERR_UNINITIALIZED if fmRootAlos has not been initialized.
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 * \return          FM_ERR_LOCK_INIT if unable to initialize the access lock.
 *
 *****************************************************************************/
fm_status fmInitDynamicLoadLibs(void)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_DLLIB, "(no arguments)\n");

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_UNINITIALIZED);
    }

    FM_CLEAR(fmRootAlos->dlLibs);

    err = fmCreateLock("DynLibLock", &fmRootAlos->dlAccessLock);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, err);

}   /* end fmInitDynamicLoadLibs */




/*****************************************************************************/
/** fmOpenDynamicLoadLibs
 * \ingroup alosDynLoadLib
 *
 * \desc            Open a Dynamic Load Library.
 *
 * \param[in]       filePath points to the string containing the library path.
 *
 * \param[out]      handle points to caller-provided memory into which the
 *                  dynamic-load-library's handle will be written.
 *
 * \return          FM_ERR_UNINITIALIZED if the ALOS subsystem has not been
 *                  properly initialized.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid.
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 * \return          FM_ERR_TABLE_FULL if the dynamic-load library table is full.
 * \return          FM_ERR_NOT_FOUND if the dynamic-load library open failed.
 *
 *****************************************************************************/
fm_status fmOpenDynamicLoadLibrary(fm_text filePath, fm_int *handle)
{
    fm_status      err;
    fm_int         index;
    fm_dynLoadLib *lib;
    fm_int         availIndex;
    fm_int         pathLen;
    fm_bool        lockTaken;
    fm_bool        libAllocated;
    void *         libHandle;

    FM_LOG_ENTRY( FM_LOG_CAT_ALOS_DLLIB,
                  "filePath = %p (%s), handle = %p\n",
                  (void *) filePath,
                  (filePath != NULL) ? filePath : "<NULL>",
                  (void *) handle );

    lib          = NULL;
    availIndex   = -1;
    lockTaken    = FALSE;
    libAllocated = FALSE;

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_UNINITIALIZED);
    }

    if (filePath == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    if (handle == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    pathLen = strlen(filePath);

    if (pathLen <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmCaptureLock(&fmRootAlos->dlAccessLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);

    lockTaken = TRUE;

    for (index = 0 ; index < FM_ALOS_INTERNAL_DYN_LOAD_LIBS ; index++)
    {
        if (fmRootAlos->dlLibs[index] == NULL)
        {
            if (availIndex < 0)
            {
                availIndex = index;
            }
        }
        else
        {
            lib = fmRootAlos->dlLibs[index];

            if ( strcmp(filePath, lib->filePath) == 0 )
            {
                break;
            }
        }
    }

    if (index >= FM_ALOS_INTERNAL_DYN_LOAD_LIBS)
    {
        index = availIndex;
    }

    if (index < 0)
    {
        err = FM_ERR_TABLE_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
    }

    lib = fmRootAlos->dlLibs[index];

    if (lib == NULL)
    {
        lib = fmAlloc( sizeof(fm_dynLoadLib) );

        if (lib == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
        }

        libAllocated = TRUE;

        FM_CLEAR(*lib);

        lib->filePath = fmAlloc( pathLen + 1 );

        if (lib->filePath == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
        }

        FM_STRNCPY_S(lib->filePath, pathLen + 1, filePath, pathLen + 1 );

        lib->useCount = 0;

        fmRootAlos->dlLibs[index] = lib;
    }

    if ( ( fmProcessDynLoadLibStatus & (1 << index) ) == 0 )
    {
        libHandle = dlopen(filePath, RTLD_NOW | RTLD_GLOBAL);

        if (libHandle == NULL)
        {
            char *errMsg = dlerror();
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_DLLIB,
                         "Error opening library %s: %s\n",
                         filePath,
                         errMsg);

            err = FM_ERR_NOT_FOUND;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
        }

        ProcessHandles[index] = libHandle;

        lib->useCount++;
        
        fmProcessDynLoadLibStatus |= FM_LITERAL_U64(1) << index;
    }

    *handle = index;
    err     = FM_OK;


ABORT:

    if ( (err != FM_OK) && libAllocated )
    {
        if (lib->filePath != NULL)
        {
            fmFree(lib->filePath);
        }

        fmFree(lib);
    }

    if (lockTaken)
    {
        fmReleaseLock(&fmRootAlos->dlAccessLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, err);

}   /* end fmOpenDynamicLoadLibrary */




/*****************************************************************************/
/** fmCloseDynamicLoadLibrary
 * \ingroup alosDynLoadLib
 *
 * \desc            Close a Dynamic Load Library.
 *
 * \param[in]       handle contains the dynamic-load-library's handle.
 *
 * \return          FM_ERR_UNINITIALIZED if the ALOS subsystem has not been
 *                  properly initialized.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid.
 * \return          FM_OK if successful.
 * \return          FM_FAIL if the dlclose operation failed.
 *
 *****************************************************************************/
fm_status fmCloseDynamicLoadLibrary(fm_int handle)
{
    fm_status      err;
    fm_int         i;
    fm_dynLoadLib *lib;
    fm_bool        lockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_DLLIB, "handle = %d\n", handle);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_UNINITIALIZED);
    }

    if ( (handle < 0) || (handle >= FM_ALOS_INTERNAL_DYN_LOAD_LIBS) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmCaptureLock(&fmRootAlos->dlAccessLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);

    lockTaken = TRUE;

    lib = fmRootAlos->dlLibs[handle];

    if (lib == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
    }

    if ( ( fmProcessDynLoadLibStatus & (1 << handle) ) != 0 )
    {
        i = dlclose(ProcessHandles[handle]);

        if (i != 0)
        {
            lib->useCount++;
            err = FM_FAIL;

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
        }

        ProcessHandles[handle] = NULL;

        fmProcessDynLoadLibStatus &= ~(FM_LITERAL_U64(1) << handle);

        if (--lib->useCount <= 0)
        {
            fmFree(lib->filePath);
            fmFree(lib);
            fmRootAlos->dlLibs[handle] = NULL;
            err                        = FM_OK;
        }
    }
    else
    {
        err = FM_OK;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseLock(&fmRootAlos->dlAccessLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, err);

}   /* end fmCloseDynamicLoadLibrary */




/*****************************************************************************/
/** fmGetDynamicLoadSymbol
 * \ingroup alosDynLoadLib
 *
 * \desc            Find a symbol in a Dynamic Load Library.
 *
 * \param[in]       handle contains the dynamic-load-library's handle.
 *
 * \param[in]       symName points to the symbol whose address is to be retrieved.
 *
 * \param[out]      symAddr points to caller-provided memory into which the
 *                  symbol's address is written.
 *
 * \return          FM_ERR_UNINITIALIZED if the ALOS subsystem has not been
 *                  properly initialized.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid.
 * \return          FM_ERR_NOT_FOUND if the symbol wasn't found in the library.
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetDynamicLoadSymbol(fm_int  handle,
                                 fm_text symName,
                                 void ** symAddr)
{
    fm_status      err;
    fm_dynLoadLib *lib;
    void *         addr;
    fm_bool        lockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_ALOS_DLLIB,
                  "handle = %d, symName = %p (%s), symAddr = %p\n",
                  handle,
                  (void *) symName,
                  (symName != NULL) ? symName : "<NULL>",
                  (void *) symAddr);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_UNINITIALIZED);
    }

    if ( (handle < 0) || (handle >= FM_ALOS_INTERNAL_DYN_LOAD_LIBS) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    if (symName == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    if (symAddr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmCaptureLock(&fmRootAlos->dlAccessLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);

    lockTaken = TRUE;

    lib = fmRootAlos->dlLibs[handle];

    if (lib == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
    }

    addr = dlsym(ProcessHandles[handle], symName);

    if (addr == NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ALOS_DLLIB, "Unable to find symbol: %s", symName);
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
    }

    *symAddr = addr;
    err      = FM_OK;


ABORT:

    if (lockTaken)
    {
        fmReleaseLock(&fmRootAlos->dlAccessLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, err);

}   /* end fmGetDynamicLoadSymbol */




/*****************************************************************************/
/** fmLoadDynamicLoadLibrary
 * \ingroup intAlosDynLoadLib
 *
 * \desc            Load a Dynamic Load Library for a process.
 *                  This function is called when a dynamic load library
 *                  has been loaded and is in use, but the current process
 *                  has not yet loaded it.
 *
 * \param[in]       handle contains the dynamic-load-library's handle.
 *
 * \return          FM_ERR_UNINITIALIZED if the ALOS subsystem has not been
 *                  properly initialized.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid.
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmLoadDynamicLoadLibrary(fm_int handle)
{
    fm_status      err;
    fm_dynLoadLib *lib;
    fm_bool        lockTaken = FALSE;
    void *         libHandle;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_DLLIB, "handle = %d\n", handle);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_UNINITIALIZED);
    }

    if ( (handle < 0) || (handle >= FM_ALOS_INTERNAL_DYN_LOAD_LIBS) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmCaptureLock(&fmRootAlos->dlAccessLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);

    lockTaken = TRUE;

    lib = fmRootAlos->dlLibs[handle];

    if (lib == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
    }

    if ( ( fmProcessDynLoadLibStatus & (1 << handle) ) == 0 )
    {
        libHandle = dlopen(lib->filePath, RTLD_NOW | RTLD_GLOBAL);

        if (libHandle == NULL)
        {
            char *errMsg = dlerror();
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_DLLIB,
                         "Error opening library %s: %s\n",
                         lib->filePath,
                         errMsg);

            err = FM_ERR_NOT_FOUND;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_DLLIB, err);
        }

        ProcessHandles[handle] = libHandle;

        lib->useCount++;
        
        fmProcessDynLoadLibStatus |= FM_LITERAL_U64(1) << handle;
    }

    err = FM_OK;


ABORT:

    if (lockTaken)
    {
        fmReleaseLock(&fmRootAlos->dlAccessLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_DLLIB, err);

}   /* end fmLoadDynamicLoadLibrary */
