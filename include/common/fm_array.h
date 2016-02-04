/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_array.h
 * Creation Date:   April 12, 2010
 * Description:     File containing declarations for miscellaneous utility
 *                  macros and functions
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

#ifndef __FM_FM_ARRAY_H
#define __FM_FM_ARRAY_H

/***************************************************
 * Helper macros and structs for declaring large 
 * local arrays.
 **************************************************/

/**
 * fm_cleanupListEntry 
 *  
 * Defines an entry in the cleanup list for this function. Used by the 
 * FM_ALLOC_TEMP_ARRAY and FM_FREE_TEMP_ARRAY macros. 
 */
typedef struct _fm_cleanupListEntry
{
    void *memory;               /* to be freed with free() */
    struct _fm_cleanupListEntry *nextPtr;

} fm_cleanupListEntry;


/**
 * Allocates a temporary array and adds it to the cleanup list for the 
 * current function. 
 *  
 * \note            The calling function should define a local variable
 *                  with a statement of the form
 *                      <i>fm_cleanupListEntry* cleanupList = NULL;</i>
 *  
 * \param[out]      lvalue is the pointer variable that receives the 
 *                  address of the temporary array.
 *  
 * \param[in]       type is the type of each element of the array. 
 *  
 * \param[in]       num is the number of elements to allocate. 
 */
#define FM_ALLOC_TEMP_ARRAY(lvalue, type, num)                  \
    {                                                           \
        fm_cleanupListEntry *cleanupEntry =                     \
            (fm_cleanupListEntry *) malloc(sizeof(fm_cleanupListEntry));                \
        if (cleanupEntry == NULL)                               \
        {                                                       \
            err = FM_ERR_NO_MEM;                                \
            goto ABORT;                                         \
        }                                                       \
        (lvalue) = (type *) malloc( (num) * sizeof(type) );     \
        if ((lvalue) == NULL)                                   \
        {                                                       \
            free(cleanupEntry);                                 \
            err = FM_ERR_NO_MEM;                                \
            goto ABORT;                                         \
        }                                                       \
        cleanupEntry->memory = (lvalue);                        \
        cleanupEntry->nextPtr = cleanupList;                    \
        cleanupList = cleanupEntry;                             \
    }                                                           \
    memset( (lvalue), 0, (num) * sizeof(type) )


/**
 * Frees the temporary arrays on the cleanup list for this function. 
 *  
 * Should be called on exit from any function that uses the
 * FM_ALLOC_TEMP_ARRAY macro. 
 */
#define FM_FREE_TEMP_ARRAYS()                \
    {                                        \
        fm_cleanupListEntry *tmp;            \
        while (cleanupList != NULL)          \
        {                                    \
            tmp = cleanupList;               \
            cleanupList = tmp->nextPtr;      \
            free(tmp->memory);               \
            free(tmp);                       \
        }                                    \
    }

#endif /* __FM_FM_ARRAY_H */
