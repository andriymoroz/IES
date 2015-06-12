/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_dynamicLib.c
 * Creation Date:   Mar. 15, 2011
 * Description:     This file provides dynamic library support for the API
 *                  and platform.
 *
 * Copyright (c) 2011, Intel Corporation
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
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

 
/*****************************************************************************
 * Local Functions
 *****************************************************************************/
 

/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fmLoadAndInitDynamicLib
 * \ingroup dynLib
 *
 * \desc            This function loads a dynamic-load library then optionally
 *                  calls an initialization function.
 *
 * \param[in]       libPath contains the path to the library to be loaded.
 *
 * \param[in]       initFunc contains the name of the initialization function
 *                  to be called. NULL if no initialization function should
 *                  be called.
 *
 * \param[in]       funcArg is passed through to the init function.
 *
 * \param[out]      libHandle points to caller-provided storage into which
 *                  the library's handle will be stored.
 *
 * \return          Description of one possible return value.
 * \return          Description of another possible return value.
 *
 *****************************************************************************/
fm_status fmLoadAndInitDynamicLib(fm_text libPath,
                                  fm_text initFunc,
                                  void *  funcArg,
                                  fm_int *libHandle)
{
    fm_status          status;
    fm_int             handle;
    void *             initAddr;
    fm_sdlInitFunction initFuncPtr;
    union
    {
        fm_sdlInitFunction func;
        void *             obj;
    } alias;

    FM_LOG_ENTRY( FM_LOG_CAT_PLATFORM,
                  "libPath = %p (%s), initFunc = %p (%s), funcArg = %p, "
                  "libHandle = %p\n",
                  (void *) libPath,
                  (libPath != NULL) ? libPath : "<NULL>",
                  (void *) initFunc,
                  (initFunc != NULL) ? initFunc : "<NULL>",
                  (void *) funcArg,
                  (void *) libHandle );

    status = fmOpenDynamicLoadLibrary(libPath, &handle);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    *libHandle = handle;

    if (initFunc != NULL)
    {
        status = fmGetDynamicLoadSymbol(handle, initFunc, &initAddr);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        alias.obj   = initAddr;
        initFuncPtr = alias.func;

        status = initFuncPtr(funcArg);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmLoadAndInitDynamicLib */




/*****************************************************************************/
/** fmCallDynamicLibFunc
 * \ingroup dynLib
 *
 * \desc            This calls a function in a dynamic library.
 *
 * \param[in]       libHandle contains the library's handle.
 *
 * \param[in]       funcName contains the name of the function to be called.
 *
 * \param[in]       funcArg is passed through to the function.
 *
 * \return          Description of one possible return value.
 * \return          Description of another possible return value.
 *
 *****************************************************************************/
fm_status fmCallDynamicLibFunc(fm_int libHandle,
                               fm_text funcName,
                               void *  funcArg)
{
    fm_status          status;
    void *             funcAddr;
    fm_sdlInitFunction funcPtr;
    union
    {
        fm_sdlInitFunction func;
        void *             obj;
    } alias;

    FM_LOG_ENTRY( FM_LOG_CAT_PLATFORM,
                  "libHandle = %d, funcName = %p (%s), funcArg = %p\n",
                  libHandle,
                  (void *) funcName,
                  (funcName != NULL) ? funcName : "<NULL>",
                  (void *) funcArg );

    if (funcName != NULL)
    {
        status = fmGetDynamicLoadSymbol(libHandle, funcName, &funcAddr);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        alias.obj = funcAddr;
        funcPtr   = alias.func;

        status = funcPtr(funcArg);
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmCallDynamicLibFunc */
