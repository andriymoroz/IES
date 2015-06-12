/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_alos_alloc.h
 * Creation Date:  June 04, 2007
 * Description:    Platform-independent memory allocation.
 *
 * Copyright (c) 2007 - 2011, Intel Corporation
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

#ifndef __FM_FM_ALOS_ALLOC_H
#define __FM_FM_ALOS_ALLOC_H

/**************************************************/
/** \ingroup typeScalar
 * Since global variables are local to a specific
 * process, all global state needs to be located in
 * structures instantiated in shared memory.
 * The ''fmGetRoot'' API service allows one process
 * to discover the global structures created by another
 * process.  ''fmGetRoot'' takes a function pointer
 * of type fm_getDataRootHandler as an argument. The
 * indicated function is expected to create the root if
 * it does not already exist when ''fmGetRoot'' is
 * called.
 *                                                                      \lb\lb
 * This function returns an fm_status (See ''Status Codes'')
 * and takes no arguments.
 **************************************************/
typedef fm_status (*fm_getDataRootHandler)(void);


/** A legacy synonym for ''fmDbgDumpAllocStats''. 
 *  \ingroup macroSynonym */
#define fmPrintAllocationStatistics fmDbgDumpAllocStats


/* Public functions */
void *fmAlloc(fm_uint size);
void fmFree(void *obj);

fm_status fmGetRoot(const char *          rootName,
                    void **               rootPtr,
                    fm_getDataRootHandler rootFunc);
fm_status fmGetAvailableSharedVirtualBaseAddress(void **ptr);
fm_status fmIsMasterProcess(fm_bool *isMaster);


/***************************************************
 * Private functions
 **************************************************/
fm_status fmMemInitialize(void);


/***************************************************
 * Debug functions
 **************************************************/
void fmPrintAllocationStatistics(void);
void fmGetAllocatedMemorySize(fm_uint32 *allocMemory);
void fmDbgDumpAllocCallStacks(fm_uint bufSize);


#endif /* __FM_FM_ALOS_ALLOC_H */
