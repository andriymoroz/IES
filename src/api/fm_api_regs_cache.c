/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_api_regs_cache.c
 * Creation Date:   April 12, 2010
 * Description:     File containing a number of utility functions to allow
 *                  other modules and applications to access a Register Cache
 *
 * Copyright (c) 2007 - 2015, Intel Corporation.
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

#define DEBUG_PARITY        FALSE

#define CACHE_BURST_SIZE    512


/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

static fm_int fmGenerateCacheInitializer(fm_int                   sw,
                                         const fm_cachedRegs    **regs,
                                         fm_registerSGListEntry  *sgList);

static fm_status fmRegCacheInitKeyValid(fm_int sw, const fm_cachedRegs **regs);

static fm_status fmRegCacheFreeKeyValid(fm_int sw, const fm_cachedRegs **regs);

static fm_uint32 fmRegCacheComputeOffset(const fm_uint32     *idx,
                                         const fm_cachedRegs *reg);

static fm_uint32 fmRegCacheComputeAddr(const fm_uint32     *idx,
                                       const fm_cachedRegs *reg);

static fm_status fmRegCacheReadC(fm_int                        sw,
                                 fm_int                        nEntries,
                                 const fm_registerSGListEntry *sgList);

static void fmRegCacheConvSGEntry(fm_int                        sw,
                                  fm_int *                      nDest,
                                  const fm_registerSGListEntry *src,
                                  fm_scatterGatherListEntry    *dest,
                                  fm_bool                       trimUnchanged);

static fm_bool fmRegCacheStrideIsContiguous(const fm_cachedRegs *regs);

static fm_int fmRegCacheConvSGList(fm_int                        sw,
                                   fm_int                        nSrc,
                                   const fm_registerSGListEntry *src,
                                   fm_scatterGatherListEntry    *dest,
                                   fm_bool                       trimUnchanged);

static fm_status fmRegCacheReadNC(fm_int                        sw,
                                  fm_int                        nEntries,
                                  const fm_registerSGListEntry *sgList);

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmGenerateCacheInitializer
 * \ingroup intRegCache
 *
 * \desc            Builds up a scatter-gather list suitable for
 *                  reading all of the cached registers from the
 *                  hardware into the cache.  This is done after chip reset
 *                  to make sure the cache is initialized properly.
 *                  (The other approach would be to initialize the cache
 *                  to what the hardware defaults are known to be, but
 *                  actually reading the values from the hardware is
 *                  easier and less error-prone.)
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regs is a NULL-terminated array of pointers to
 *                  fm_cachedRegs structures, which describe all of the
 *                  registers that need to be cached.
 *
 * \param[out]      sgList points to a sufficiently-size array of
 *                  fm_registerSGListEntry structures, which are filled
 *                  out by this function.  If sgList is NULL, then no
 *                  entries are written, but the number of entries needed
 *                  is still returned.  This allows fmGenerateCacheInitializer
 *                  to be called twice, first to determine how many
 *                  entries are needed, and then again to fill them out.
 *
 * \return          The number of entries in the scatter-gather list.
 *
 *****************************************************************************/
static fm_int fmGenerateCacheInitializer(fm_int                  sw,
                                         const fm_cachedRegs **  regs,
                                         fm_registerSGListEntry *sgList)
{
    fm_int      result = 0;
    fm_uint32   firstDimension;
    fm_uint32   secondDimension;
    fm_uint32   thirdDimension;
    fm_uint32   i;
    fm_uint32   j;
    fm_uint32   k;
    fm_uint32   idx[FM_REGS_CACHE_MAX_INDICES];
    fm_uint32   regAddr;
    fm_uint32   *cacheAddr;
    fm_uint32   offset;

    /* No entries if the register list is NULL. */
    if (regs == NULL)
    {
        return 0;
    }

    /* Scan the list of Cached Register descriptors */
    while (*regs != NULL)
    {

        firstDimension = (**regs).nElements[0];

        /**************************************************
         * Determine if it is a 2D register set. 
         * If it is, 'secondDimension' will be set to the 
         * number of elements along that dimension 
         * (second index, i.e. idx[1]) 
         **************************************************/
        if ( (**regs).nIndices < 2 )
        {
            secondDimension = 1;
        }
        else
        {
            secondDimension  = (**regs).nElements[1];
        }

        /**************************************************
         * Determine if it is a 3D register set 
         * If it is, 'thirdDimension' will be set to the 
         * number of element along that dimension 
         * (third index i.e. idx[2]) 
         **************************************************/
        if ( (**regs).nIndices < 3 )
        {
            thirdDimension = 1;
        }
        else
        {
            thirdDimension  = (**regs).nElements[2];
        }

        /* how do we want to initialize the register cache? */
        if ( (**regs).getCache.defaults == NULL )
        {
            /* there's no value for this register set in defaults table     */
            /* initialize the cache by doing a hard read from the registers */

            /**************************************************************
             * Scan this register set in the 2nd and 3rd dimension 
             * The number of registers in the 1st dimension determines 
             * the single largest block of registers associated with a 
             * single entry in sgList. Those will be stored in contiguous
             * locations in cache whether or not they are contiguous in 
             * the hardware registers address space. But if they are, 
             * read/write operations will take full advantage of the 
             * scatter-gather approach 
             *************************************************************/
            
            for (j = 0 ; j < secondDimension ; j++)
            {
                for (k = 0 ; k < thirdDimension ; k++)
                {
                    /********************************************
                     * Are we just counting register blocks 
                     * (i.e. sgList entries we need) or actually 
                     * filling those out? 
                     ********************************************/
                    
                    if (sgList != NULL)
                    {
                        /* we'll initialize the cache by reading current */
                        /* values from the actual registers              */
                        sgList[result].registerSet = *regs;
                        sgList[result].count       = (**regs).nElements[0];
                        sgList[result].idx[0]      = 0;      
                        sgList[result].idx[1]      = j;
                        sgList[result].idx[2]      = k;

                        /**************************************************
                         * Use the indices to determine the offset in cache 
                         * of this register block (subset of the current 
                         * register set) 
                         **************************************************/
                        
                        sgList[result].data        =
                            (**regs).getCache.data(sw) +
                            fmRegCacheComputeOffset(sgList[result].idx, *regs);
                    }

                    /* One more register block/sgList entry */ 
                    result++;
                }
            }

        }   /* end if ( (**regs).getCache.defaults == NULL ) */
        else
        {
            /**************************************************
             * We are going to initialize this register set 
             * from the table of defaults 
             **************************************************/

            for (i = 0 ; i < firstDimension ; i++)
            {
                idx[0] = i;
                for (j = 0 ; j < secondDimension ; j++)
                {
                    idx[1] = j;
                    for (k = 0 ; k < thirdDimension ; k++)
                    {
                        idx[2] = k;
                        offset  = fmRegCacheComputeOffset(idx, *regs);
                        regAddr = fmRegCacheComputeAddr(idx, *regs);
                        cacheAddr = (**regs).getCache.data(sw);
                        cacheAddr += offset;
                        *cacheAddr = (**regs).getCache.defaults(regAddr);

                    }  /* end for ( i == 0; ... ) */

                }   /* end for ( j == 0; ... ) */

            }   /* end for ( k == 0; ... ) */
            
        }   /* end else */

        /* next register set in the descriptor list */
        regs++;
    }

    /**************************************************
     * We're done, just return the number of sgList
     * entries we need or that we just filled out 
     **************************************************/
    
    return result;

}   /* end fmGenerateCacheInitializer */




/*****************************************************************************/
/** fmRegCacheInitKeyValid
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            This function initializes the KeyValid bit arrays for
 *                  all applicable cached registers, i.e. those that have
 *                  the valid() method initialized to be non NULL. Typically
 *                  it'll be used for CAM-type registers for FM6000 or later
 *                  chipsets.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regs is a NULL-terminated array of pointers to
 *                  fm_cachedRegs structures, which describe all of the
 *                  registers that need to be cached.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if the memory allocation for at least
 *                  one bit array failed.
 *
 *****************************************************************************/
static fm_status fmRegCacheInitKeyValid(fm_int sw, const fm_cachedRegs **regs)
{
    fm_int      i;
    fm_uint32   keyValidBitArraySize;
    fm_bitArray *keyValidBitArrayPtr;
    fm_status   err = FM_OK;

    /* Scan the list of Cached Register descriptors */
    while ( *regs != NULL )
    {
        /**************************************************
         * determine the size of the key valid bit array, 
         * if any. It'll have a valid bit for each of the 
         * entries in any given register set. Initialize 
         * it, if needed 
         **************************************************/
        if ( (**regs).getCache.valid  != NULL )
        {
            keyValidBitArrayPtr = (**regs).getCache.valid(sw);
            if ( keyValidBitArrayPtr != NULL )
            {
                /* determine the bit array size based on the nr of entries */
                keyValidBitArraySize = 2;
                for (i = 0 ; i < (**regs).nIndices; i++ )
                {
                    keyValidBitArraySize *= (**regs).nElements[i];
                }

                /* now create the bit array */
                err = fmCreateBitArray(keyValidBitArrayPtr, 
                                       keyValidBitArraySize); 

                /* did that work? */
                if ( err != FM_OK )
                {
                    /* no, stop here and flag the error */
                    break;
                }
            }
        }
        /* next register set in the descriptor list */
        regs++;
    }
    return err;

}   /* end fmRegCacheInitKeyValid */




/*****************************************************************************/
/** fmRegCacheFreeKeyValid
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            This function frees the KeyValid bit arrays for
 *                  all applicable cached registers, i.e. those that have
 *                  the valid() method initialized to be non NULL. Typically
 *                  it'll be used for CAM-type registers for FM6000 or later
 *                  chipsets
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regs is a NULL-terminated array of pointers to
 *                  fm_cachedRegs structures, which describe all of the
 *                  registers that need to be cached.
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status fmRegCacheFreeKeyValid(fm_int sw, const fm_cachedRegs **regs)
{
    fm_bitArray *keyValidBitArrayPtr;
    fm_status    err = FM_OK;

    /* Scan the list of Cached Register descriptors */
    while (*regs != NULL)
    {
        /**************************************************
         * determine the size of the key valid bit array, 
         * if any. It'll have a valid bit for each of the 
         * entries in any given register set. Initialize 
         * it, if needed 
         **************************************************/
        if ( (**regs).getCache.valid  != NULL )
        {
            keyValidBitArrayPtr = (**regs).getCache.valid(sw);
            if ( keyValidBitArrayPtr != NULL )
            {
                /* now delete the bit array */
                err = fmDeleteBitArray(keyValidBitArrayPtr); 

                /* did that work? */
                if ( err != FM_OK )
                {
                    /* no, stop here and flag the error */
                    break;
                }
            }
        }

        /* next register set in the descriptor list */
        regs++;
    }

    return err;

}   /* end fmRegCacheFreeKeyValid */




/*****************************************************************************/
/** fmRegCacheComputeOffset
 * \ingroup intRegCache
 *
 * \desc            Computes the offset into the cache, given the
 *                  indices of the register.
 *
 * \param[in]       idx is the indices of the register.
 *
 * \param[in]       reg indicates which register.
 *
 * \return          offest of the register in the cache.
 *
 *****************************************************************************/
static fm_uint32 fmRegCacheComputeOffset(const fm_uint32 *    idx,
                                         const fm_cachedRegs *reg)
{
    fm_int    i;
    fm_uint32 result  = 0;
    fm_uint32 product = reg->nWords;
    fm_int    nIndices;

    /* NOTE: a more robust solution would be to pass the actual array size,
     * and test against this value instead of FM_REGS_CACHE_MAX_INDICES. */
    nIndices = reg->nIndices;
    if (nIndices > FM_REGS_CACHE_MAX_INDICES)
    {
#if 0
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                     "reg->nIndices (%d) > MAX_INDICES (%d)\n",
                     nIndices,
                     FM_REGS_CACHE_MAX_INDICES);
#endif
        nIndices = FM_REGS_CACHE_MAX_INDICES;
    }

    for (i = 0 ; i < nIndices ; i++)
    {
        result  += idx[i] * product;
        product *= reg->nElements[i];
    }

    return result;

}   /* end fmRegCacheComputeOffset */




/*****************************************************************************/
/** fmRegCacheComputeAddr
 * \ingroup intRegCache
 *
 * \desc            Computes the address of a register, given the
 *                  indices of the register.
 *
 * \param[in]       idx is the indices of the register.
 *
 * \param[in]       reg indicates which register.
 *
 * \return          address of the register.
 *
 *****************************************************************************/
static fm_uint32 fmRegCacheComputeAddr(const fm_uint32 *    idx,
                                       const fm_cachedRegs *reg)
{
    fm_int    i;
    fm_uint32 result = reg->baseAddr;
    fm_int    nIndices;

    /* NOTE: a more robust solution would be to pass the actual array size,
     * and test against this value instead of FM_REGS_CACHE_MAX_INDICES. */
    nIndices = reg->nIndices;
    if (nIndices > FM_REGS_CACHE_MAX_INDICES)
    {
#if 0
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                     "reg->nIndices (%d) > MAX_INDICES (%d)\n",
                     nIndices,
                     FM_REGS_CACHE_MAX_INDICES);
#endif
        nIndices = FM_REGS_CACHE_MAX_INDICES;
    }

    for (i = 0 ; i < nIndices ; i++)
    {
        result += idx[i] * reg->stride[i];
    }

    return result;

}   /* end fmRegCacheComputeAddr */




/*****************************************************************************/
/** fmRegCacheReadC
 * \ingroup intRegCache
 *
 * \desc            Reads from the cache using a scatter-gather list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nEntries is the number of entries in sgList.
 *
 * \param[in]       sgList is the list of contiguous ranges of data
 *                  to read from the cache.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmRegCacheReadC(fm_int                        sw,
                                 fm_int                        nEntries,
                                 const fm_registerSGListEntry *sgList)
{
    fm_int     i;
    fm_int     j;
    fm_uint32 *cache;

    for (i = 0 ; i < nEntries ; i++)
    {
        cache  = sgList[i].registerSet->getCache.data(sw);
        cache += fmRegCacheComputeOffset(sgList[i].idx,
                                         sgList[i].registerSet);

        for (j = 0 ;
             j < ((fm_int) sgList[i].count *
                  (fm_int) sgList[i].registerSet->nWords) ;
             j++)
        {
            sgList[i].data[j] = *cache++;
        }
    }

    return FM_OK;

}   /* end fmRegCacheReadC */




/*****************************************************************************/
/** fmRegCacheConvSGEntry
 * \ingroup intRegCache
 *
 * \desc            Converts a single scatter-gather list entry from
 *                  high-level cached register format to low-level format.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   nDest is incremented if this entry is not suppressed.
 *
 * \param[in]       src is the entry in cached register format.
 *
 * \param[out]      dest is where the converted entry is written.
 *
 * \param[in]       trimUnchanged indicates that the entry should be
 *                  shrunk if part of it matches the cache, or suppressed
 *                  completely if it entirely matches the cache.
 *
 * \return          None
 *
 *****************************************************************************/
static void fmRegCacheConvSGEntry(fm_int                        sw,
                                  fm_int *                      nDest,
                                  const fm_registerSGListEntry *src,
                                  fm_scatterGatherListEntry *   dest,
                                  fm_bool                       trimUnchanged)
{
    fm_registerSGListEntry entry;
    fm_int                 i;
    fm_int                 j;
    fm_uint32 *            cache;
    fm_bool                isEqual;
    fm_bool                allEqual;
    fm_int                 nWords;
    fm_int                 highestNotEqual = 0;
    fm_int                 lowestNotEqual  = 0;

    entry  = *src;
    nWords = src->registerSet->nWords;

    /*****************************************************
     *  do we want a hard, unconditional access to the
     *  registers or rather a soft, smart access based
     *  on the actual differences vs their cached values?
     ****************************************************/
    
    if (trimUnchanged)
    {
        /**************************************************
         * soft, smart read. The register block is trimmed 
         * to the largest one containing all registers that 
         * we know will need to change, based on their 
         * currently cached value. This could be as small 
         * as no registers at all 
         **************************************************/
        
        /* let's find the cache offset and read from there */
        cache  = src->registerSet->getCache.data(sw);
        cache += fmRegCacheComputeOffset(src->idx,
                                         src->registerSet);

        /* Assume no change to start with */
        allEqual = TRUE;

        /* Scan the register block */
        for (i = 0 ; i < (fm_int) src->count ; i++)
        {
            /* Assume this register hasn't changed to start with */
            isEqual = TRUE;

            /* scan all words of this register, one by one */
            for (j = 0 ; j < nWords ; j++)
            {
                /* is this word different? */
                if (cache[j + i * nWords] != src->data[j + i * nWords])
                {
                    /* yes, this register changed */
                    isEqual = FALSE;
                }
            }

            if (!isEqual)
            {
                /* register changed, mark the top of the trim area */
                highestNotEqual = i;
                allEqual        = FALSE;
            }

            if (allEqual)
            {
                /****************************************
                 * No register has changed for now, move 
                 * the bottom of the trim area 
                 ****************************************/
                lowestNotEqual = i + 1;
            }

        }   /* end for (i = 0 ; i < (fm_int) src->count ; i++) */

        /* we scanned the whole register block, any changes? */
        if (allEqual)
        {
            /* no, nothing to do then */
            entry.count = 0;
        }
        else
        {
            /* yes, modify the sgList entry based on the trim area */
            entry.idx[0] += lowestNotEqual;
            entry.count   = 1 + highestNotEqual - lowestNotEqual;
            entry.data   += lowestNotEqual * nWords;
        }

    }   /* end if (trimUnchanged) */

    /**************************************************
     * Now, if there are any changes, let's fill out the 
     * low-level scatter-gather entry, unless all what 
     * we're doing is to count how many low-level entries 
     * are needed for allocation purposes (dest == NULL)
     * In that case we'll be back here soon to prepare 
     * for the actual register access 
     **************************************************/
    
    if (entry.count > 0)
    {
        if (dest != NULL)
        {
            /* we're actually accessing the registers, not just counting */
            dest[*nDest].addr = fmRegCacheComputeAddr(entry.idx,
                                                      entry.registerSet);
            dest[*nDest].count = entry.count * nWords;
            dest[*nDest].data  = entry.data;
        }

        (*nDest)++;
    }

}   /* end fmRegCacheConvSGEntry */




/*****************************************************************************/
/** fmRegCacheStrideIsContiguous
 * \ingroup intRegCache
 *
 * \desc            Determines whether consecutive register indices
 *                  are actually contiguous in the register address
 *                  space.
 *
 * \param[in]       regs is the register to examine the stride of.
 *
 * \return          TRUE if consecutive indices are contiguous in memory.
 * \return          FALSE if not.
 *
 *****************************************************************************/
static fm_bool fmRegCacheStrideIsContiguous(const fm_cachedRegs *regs)
{
    return regs->nWords == regs->stride[0];

}   /* end fmRegCacheStrideIsContiguous */




/*****************************************************************************/
/** fmRegCacheConvSGList
 * \ingroup intRegCache
 *
 * \desc            Converts a scatter-gather list from the higher-level
 *                  cached register-specific format to the lower-level
 *                  format that contains actual addresses.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nSrc is the number of entries in the "src" list.
 *
 * \param[in]       src is the scatter-gather list in cached register format.
 * 
 * \param[out]      dest is where the converted scatter-gather list
 *                  will be written.
 *
 * \param[in]       trimUnchanged indicates that entries should be
 *                  shrunk or removed if the data to be written matches
 *                  the data already in the cache.
 *
 * \return          number of entries in the "dest" list.
 *
 *****************************************************************************/
static fm_int fmRegCacheConvSGList(fm_int                        sw,
                                   fm_int                        nSrc,
                                   const fm_registerSGListEntry *src,
                                   fm_scatterGatherListEntry    *dest,
                                   fm_bool                       trimUnchanged)
{
    fm_int                 nDest = 0;
    fm_int                 i;
    fm_int                 j;
    fm_registerSGListEntry single;
    fm_bool                trimIt;

    /* Scan the list of input sgList entries */
    for (i = 0 ; i < nSrc ; i++)
    {
        if (trimUnchanged && !src[i].rewriting)
        {
            trimIt = TRUE;
        }
        else
        {
            trimIt = FALSE;
        }

        /**************************************************
         * if for this entry all registers are contiguous 
         * in the address space take advantage of scatter- 
         * gather 
         **************************************************/
        
        if ( fmRegCacheStrideIsContiguous(src[i].registerSet) )
        {
            /**************************************************
             * They are contiguous, this sgList entry will 
             * generate a block access into the register 
             * address space. scatter-gather will be used 
             * if enabled for this switch type using the 
             * low-level format  
             **************************************************/
            fmRegCacheConvSGEntry(sw,
                                  &nDest,
                                  src + i,
                                  dest,
                                  trimIt );
        }
        else
        {
            /********************************************************
             * This sgList entry is associated with a register block 
             * made of non-contiguous registers. The entry will 
             * generate as many single-register accesses as indicated 
             * in the 'count' field 
             ********************************************************/
            for (j = 0 ; j < (fm_int) src[i].count ; j++)
            {
                single        = src[i];
                single.idx[0] = src[i].idx[0] + j;
                single.count  = 1;

                /* BUG? it doesn't seem to account for the  */
                /* size of the register (in terms of Words) */ 
                single.data   = &src[i].data[j*src[i].registerSet->nWords]; 

                /* convert it in low-level format */
                fmRegCacheConvSGEntry(sw, 
                                      &nDest,
                                      &single,
                                      dest,
                                      trimIt );
            }
        }

    }   /* end for (i = 0 ; i < nSrc ; i++) */

    return nDest;

}   /* end fmRegCacheConvSGList */




/*****************************************************************************/
/** fmRegCacheReadNC
 * \ingroup intRegCache
 *
 * \desc            Read a scatter-gather list from the cached registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nEntries is the number of entries in sgList.
 *
 * \param[in]       sgList is the list of contiguous ranges of data
 *                  to read from the cached registers.
 *
 * \return          status code
 *
 *****************************************************************************/
static fm_status fmRegCacheReadNC(fm_int                        sw,
                                  fm_int                        nEntries,
                                  const fm_registerSGListEntry *sgList)
{
    fm_int                     nSGEntries;
    fm_scatterGatherListEntry *hwSGList;
    fm_status                  err;
    fm_cleanupListEntry *      cleanupList = NULL;

    nSGEntries = fmRegCacheConvSGList(sw,
                                      nEntries,
                                      sgList,
                                      NULL,
                                      FALSE);

    if (nSGEntries > 0)
    {
        FM_ALLOC_TEMP_ARRAY(hwSGList, fm_scatterGatherListEntry, nSGEntries);

        fmRegCacheConvSGList(sw, 
                             nEntries, 
                             sgList, 
                             hwSGList, 
                             FALSE);

        err = fmReadScatterGather(sw, nSGEntries, hwSGList);
    }
    else
    {
        err = FM_OK;
    }

ABORT:
    FM_FREE_TEMP_ARRAYS();

    return err;

}   /* end fmRegCacheReadNC */



 
/*****************************************************************************/
/** IsScatterGatherListCorrect
 * \ingroup intRegCache
 *
 * \desc            Check an array of scatter-gather entries for correctness
 *
 * \param[in]       sgListPtr points to a caller-supplied array of
 *                  scatter-gather entries
 *
 * \param[in]       nEntries the number of entries in sgList.
 *
 * \return          TRUE if all entries in the list are correct
 * \return          FALSE if at least one of the entries is incorrect
 *
 *****************************************************************************/
static fm_bool IsScatterGatherListCorrect(const fm_registerSGListEntry *sgListPtr, 
                                          fm_int                        nEntries)
{
    fm_int i;
    fm_int index;
    fm_bool correct = TRUE;

    /* for all entries in the list... */
    for (i = 0 ; i < nEntries ; i++)
    {
        /* ... and for all dimensions used by this register set... */
        for (index = 0 ; index < sgListPtr[i].registerSet->nIndices ; index++)
        {
            /* ..bail out if the array is out of bounds */
            if ( sgListPtr[i].idx[index] >= 
                 sgListPtr[i].registerSet->nElements[index] ) 
            {
                correct = FALSE;
                break;
            }
        }
    }

    return correct;

}   /* end IsScatterGatherListCorrect */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmInitRegisterCache
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Initializes the Register Cache.
 *
 * \param[in]       sw is the switch whose cache is being initialized.
 * 
 * \return          FM_OK: if successful
 * \return          FM_ERR_INVALID_SWITCH: if the switch ID is invalid
 * \return          FM_ERR_NO_MEM: if there's no memory for the initialization
 * \return          FM_ERR_INVALID_ARGUMENT: if the word count in any register 
 *                  description is wrong
 * \return          FM_ERR_BAD_IOCTL: driver error during the first read
 * \return          FM_FAIL: any other error
 *
 *****************************************************************************/
fm_status fmInitRegisterCache(fm_int sw)
{
    fm_status             err = FM_OK;
    fm_int                nEntries;
    fm_switch            *switchPtr;
    const fm_cachedRegs **cachedRegs;


    /* get the switch pointer and initialize its cache */
    VALIDATE_SWITCH_INDEX(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    cachedRegs = (const fm_cachedRegs **)(switchPtr->CachedRegisterList);
    if (cachedRegs == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "CachedRegisterList is NULL\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* First, count how many cached register we have total */
    nEntries = fmGenerateCacheInitializer(sw, cachedRegs, NULL);
             
    /* any cached registers? */
    if (nEntries > 0)
    {
        fm_cleanupListEntry *      cleanupList = NULL;
        fm_registerSGListEntry *   entries;

        /* Yes, allocate and format a scatter-gather list for those */

        FM_ALLOC_TEMP_ARRAY(entries, fm_registerSGListEntry, nEntries);
        
        memset(entries, 0, sizeof(fm_registerSGListEntry) * nEntries);
        fmGenerateCacheInitializer(sw, cachedRegs, entries );

        /* Initialize the keyValid bit arrarys */
        err = fmRegCacheInitKeyValid(sw, cachedRegs);

        /* If that worked, perform the first read of into the cache */ 
        if ( err == FM_OK )
        {
            err = fmRegCacheRead(sw, nEntries, entries, FALSE);
        }

        /* relinquish the memory allocated for the scatter-gather list */
        FM_FREE_TEMP_ARRAYS();
    }

ABORT:
    return err;

}   /* end fmInitRegisterCache */




/*****************************************************************************/
/** fmFreeRegisterCache
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Function that frees memory allocated for the Register Cache
 *
 * \param[in]       sw is the switch whose cache is being freed.
 * 
 * \return          FM_OK: if successful
 * \return          FM_ERR_INVALID_SWITCH: if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT: if the word count in any register 
 *                  description is wrong
 * \return          FM_FAIL: any other error
 *
 *****************************************************************************/
fm_status fmFreeRegisterCache(fm_int sw)
{
    fm_status             err = FM_OK;
    fm_switch            *switchPtr;
    const fm_cachedRegs **cachedRegs;


    /* get the switch pointer and point to its cache */
    VALIDATE_SWITCH_INDEX(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    cachedRegs = (const fm_cachedRegs **)(switchPtr->CachedRegisterList);

    fmGenerateCacheInitializer(sw, cachedRegs, NULL);

    /* Free the cache key valids */
    err = fmRegCacheFreeKeyValid(sw, cachedRegs);
             
    return err;

}   /* end fmFreeRegisterCache */




/*****************************************************************************/
/** fmRegCacheRead
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Read a scatter-gather list from the hardware registers
 *                  or their cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nEntries is the number of entries in sgList.
 *
 * \param[in]       sgList is the list of contiguous ranges of data
 *                  to read from the cached registers.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          status code
 *
 *****************************************************************************/
fm_status fmRegCacheRead(fm_int                        sw,
                         fm_int                        nEntries,
                         const fm_registerSGListEntry *sgList,
                         fm_bool                       useCache)
{
    fm_status err;

    /* Sanity check on the scatter-gather list */
    if ( !IsScatterGatherListCorrect(sgList, nEntries) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "incorrect scatter-gather entry!\n");
        FM_LOG_CALL_STACK(FM_LOG_CAT_SWITCH, FM_LOG_LEVEL_ERROR);
        return FM_ERR_INVALID_ARGUMENT;
    }

    TAKE_REG_LOCK(sw);   /* make access atomic */

    if (useCache)
    {
        err = fmRegCacheReadC(sw, nEntries, sgList);
    }
    else
    {
        err = fmRegCacheReadNC(sw, nEntries, sgList);
    }

    DROP_REG_LOCK(sw);

    return err;

}   /* end fmRegCacheRead */




/*****************************************************************************/
/** fmRegCacheWrite
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Write a scatter-gather list to the hardware registers,
 *                  keeping a copy in their cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nEntries is the number of entries in sgList.
 *
 * \param[in]       sgList is the list of contiguous ranges of data
 *                  to write to the register set.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          status code
 *
 *****************************************************************************/
fm_status fmRegCacheWrite(fm_int                        sw,
                          fm_int                        nEntries,
                          const fm_registerSGListEntry *sgList,
                          fm_bool                       useCache)
{
    fm_int                     nSGEntries;
    fm_scatterGatherListEntry *hwSGList;
    fm_status                  err;
    fm_int                     i;
    fm_int                     j;
    fm_uint32 *                cache;
    fm_cleanupListEntry *      cleanupList = NULL;

    /* Sanity check on the scatter-gather list */
    if ( !IsScatterGatherListCorrect(sgList, nEntries) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "incorrect scatter-gather entry!\n");
        FM_LOG_CALL_STACK( FM_LOG_CAT_SWITCH, FM_LOG_LEVEL_ERROR );
        return FM_ERR_INVALID_ARGUMENT;
    }

    TAKE_REG_LOCK(sw);   /* make access atomic */

    /**************************************************
     * Write to the hardware
     **************************************************/

    nSGEntries = fmRegCacheConvSGList(sw,
                                      nEntries,
                                      sgList,
                                      NULL,
                                      useCache);

    if (nSGEntries > 0)
    {
        FM_ALLOC_TEMP_ARRAY(hwSGList, fm_scatterGatherListEntry, nSGEntries);
        fmRegCacheConvSGList(sw, 
                             nEntries, 
                             sgList, 
                             hwSGList, 
                             useCache);
        err = fmWriteScatterGather(sw, nSGEntries, hwSGList);
    }
    else
    {
        err = FM_OK;
    }

    /**************************************************
     * Update the cache
     **************************************************/

    if (err == FM_OK)
    {
        for (i = 0 ; i < nEntries ; i++)
        {
            cache  = sgList[i].registerSet->getCache.data(sw);
            cache += fmRegCacheComputeOffset(sgList[i].idx,
                                             sgList[i].registerSet);

            for (j = 0 ;
                 j < ((fm_int) sgList[i].count *
                      (fm_int) sgList[i].registerSet->nWords) ;
                 j++)
            {
                *cache++ = sgList[i].data[j];
            }
        }
    }

ABORT:
    FM_FREE_TEMP_ARRAYS();

    DROP_REG_LOCK(sw);

    return err;

}   /* end fmRegCacheWrite */




/*****************************************************************************/
/** fmRegCacheReadSingle1D
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            This is just a variant of ''fmRegCacheRead'' that reads
 *                  from a single mono-dimensional register (i.e. only one
 *                  index is needed to access it).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet pointer to the cached register set
 *
 * \param[in, out]  data is the pointer to the storage location where the
 *                  content of register (or its cache) will be copied to
 * 
 * \param[in]       idx is the index of the register
 * 
 * \param[in]       useCache If true, the value will be read from cache only.
 *                  If FALSE it'll be read from the actual hardware register
 *
 * \return          status code
 *
 *****************************************************************************/
fm_status fmRegCacheReadSingle1D(fm_int               sw,
                                 const fm_cachedRegs *regSet,
                                 fm_uint32 *          data,
                                 fm_uint32            idx,
                                 fm_bool              useCache)
{
    fm_registerSGListEntry entry;

    /* Ensure that data is initialized */
    if (data != NULL)
    {
        *data = 0;
    }

    /* sanity checks on the regSet argument */
    if ( regSet == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( regSet->nIndices > 1 )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "fmRegCacheReadSingle1D: not a 1D register: 0x%08X!\n",
                     regSet->baseAddr);
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = data;
    entry.count       = 1;
    entry.idx[0]      = idx;
    entry.idx[1]      = FM_REGS_CACHE_INDEX_UNUSED;
    entry.idx[2]      = FM_REGS_CACHE_INDEX_UNUSED;
    entry.rewriting   = FALSE;

    return fmRegCacheRead(sw, 1, &entry, useCache);

}   /* end fmRegCacheReadSingle1D */





/*****************************************************************************/
/** fmRegCacheWriteSingle1D
 * \ingroup intRegCache
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            This is just a variant of ''fmRegCacheWrite'' that writes
 *                  to a single mono-dimensional register (one index only is
 *                  needed to access it).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet pointer to the cached register set
 *
 * \param[in]       data is the pointer of the data to be written to register
 *
 * \param[in]       idx is the index of the register
 * 
 * \param[in]       useCache If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          status code
 *
 *****************************************************************************/
fm_status fmRegCacheWriteSingle1D(fm_int               sw,
                                  const fm_cachedRegs *regSet,
                                  const fm_uint32 *    data,
                                  fm_uint32            idx,
                                  fm_bool              useCache)
{
    fm_registerSGListEntry entry;

    /* sanity checks on the regSet argument */
    if ( regSet == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( regSet->nIndices > 1 )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "fmRegCacheWriteSingle1D: not a 1D register: 0x%08X!\n",
                     regSet->baseAddr);
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = (fm_uint32 *) data; /* cast away const */
    entry.count       = 1;
    entry.idx[0]      = idx;
    entry.idx[1]      = FM_REGS_CACHE_INDEX_UNUSED;
    entry.idx[2]      = FM_REGS_CACHE_INDEX_UNUSED;
    entry.rewriting   = FALSE;

    return fmRegCacheWrite(sw, 1, &entry, useCache);

}   /* end fmRegCacheWriteSingle1D */




/*****************************************************************************/
/** fmRegCacheReadUINT64
 * \ingroup intRegCache
 *
 * \chips           FM10000
 * 
 * \desc            Reads a 64-bit register table entry from the cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register cache descriptor.
 * 
 * \param[in]       idx is the index of the entry to be read.
 *
 * \param[out]      value points to the location where the contents of the
 *                  register will be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheReadUINT64(fm_int               sw,
                               const fm_cachedRegs *regSet,
                               fm_uint32            idx,
                               fm_uint64 *          value)
{
    fm_uint32   regData[2];
    fm_status   err;

    if (regSet == NULL || value == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (regSet->nWords != 2)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "nWords (%d) != 2\n", regSet->nWords);
        return FM_ERR_INVALID_ARGUMENT;
    }

    err = fmRegCacheReadSingle1D(sw, regSet, regData, idx, TRUE);

    if (err == FM_OK)
    {
        *value = ( ((fm_uint64) regData[1]) << 32 ) | ((fm_uint64) regData[0]);
    }
    else
    {
        *value = 0;
    }

    return err;

}   /* end fmRegCacheReadUINT64 */




/*****************************************************************************/
/** fmRegCacheWriteUINT64
 * \ingroup intRegCache
 *
 * \chips           FM10000
 * 
 * \desc            Writes a 64-bit register table entry through the cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register cache descriptor.
 * 
 * \param[in]       idx is the index of the entry to be written.
 *
 * \param[in]       value is the value to be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheWriteUINT64(fm_int               sw,
                                const fm_cachedRegs *regSet,
                                fm_uint32            idx,
                                fm_uint64            value)
{
    fm_uint32   regData[2];

    if (regSet == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (regSet->nWords != 2)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "nWords (%d) != 2\n", regSet->nWords);
        return FM_ERR_INVALID_ARGUMENT;
    }

    regData[0] = (fm_uint32) (value & FM_LITERAL_U64(0xFFFFFFFF));
    regData[1] = (fm_uint32) (value >> 32);

    return fmRegCacheWriteSingle1D(sw, regSet, regData, idx, TRUE);

}   /* end fmRegCacheWriteUINT64 */




/*****************************************************************************/
/** fmRegCacheReadUINT64V2
 * \ingroup intRegCache
 *
 * \chips           FM10000
 * 
 * \desc            Reads a 64-bit register table entry from the cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register cache descriptor.
 * 
 * \param[in]       idx1 is the outer index of the entry to be read.
 * 
 * \param[in]       idx0 is the inner index of the entry to be read.
 *
 * \param[out]      value points to the location where the contents of the
 *                  register will be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheReadUINT64V2(fm_int               sw,
                                 const fm_cachedRegs *regSet,
                                 fm_uint32            idx1,
                                 fm_uint32            idx0,
                                 fm_uint64 *          value)
{
    fm_registerSGListEntry  entry;
    fm_uint32               regData[2];
    fm_status               err;

    if (regSet == NULL || value == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (regSet->nWords != 2)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "nWords (%d) != 2\n", regSet->nWords);
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (regSet->nIndices != 2)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, 
                     "nIndices (%d) != 2\n",
                     regSet->nIndices);
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = regData;
    entry.count       = 1;
    entry.idx[0]      = idx0;
    entry.idx[1]      = idx1;
    entry.idx[2]      = FM_REGS_CACHE_INDEX_UNUSED;
    entry.rewriting   = FALSE;

    err = fmRegCacheRead(sw, 1, &entry, TRUE);

    if (err == FM_OK)
    {
        *value = ( ((fm_uint64) regData[1]) << 32 ) | ((fm_uint64) regData[0]);
    }
    else
    {
        *value = 0;
    }

    return err;

}   /* end fmRegCacheReadUINT64V2 */




/*****************************************************************************/
/** fmRegCacheWriteUINT64V2
 * \ingroup intRegCache
 *
 * \chips           FM10000
 * 
 * \desc            Writes a 64-bit register table entry through the cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register cache descriptor.
 * 
 * \param[in]       idx1 is the outer index of the entry to be written.
 * 
 * \param[in]       idx0 is the inner index of the entry to be written.
 *
 * \param[in]       value is the value to be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheWriteUINT64V2(fm_int               sw,
                                  const fm_cachedRegs *regSet,
                                  fm_uint32            idx1,
                                  fm_uint32            idx0,
                                  fm_uint64            value)
{
    fm_registerSGListEntry  entry;
    fm_uint32               regData[2];

    if (regSet == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (regSet->nWords != 2)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "nWords (%d) != 2\n", regSet->nWords);
        return FM_ERR_INVALID_ARGUMENT;
    }

    regData[0] = (fm_uint32) (value & FM_LITERAL_U64(0xFFFFFFFF));
    regData[1] = (fm_uint32) (value >> 32);

    if (regSet->nIndices != 2)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, 
                     "nIndices (%d) != 2\n",
                     regSet->nIndices);
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = regData;
    entry.count       = 1;
    entry.idx[0]      = idx0;
    entry.idx[1]      = idx1;
    entry.idx[2]      = FM_REGS_CACHE_INDEX_UNUSED;
    entry.rewriting   = FALSE;

    return fmRegCacheWrite(sw, 1, &entry, TRUE);

}   /* end fmRegCacheWriteUINT64V2 */




/*****************************************************************************/
/** fmRegCacheUpdateSingle1D
 * \ingroup intRegCache
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            This function updates the cache for a single register
 *                  belonging to a mono-dimensional register set.
 *
 * \param[in]       sw is the register on which to operate
 * \param[in]       regSet is the register set descriptor
 * \param[in]       data is the data to be written to cache
 * \param[in]       idx is the index of this register in its register set
 * 
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if this register set is not
 *                  mono-dimensional or the index is out of range
 *
 *****************************************************************************/
fm_status fmRegCacheUpdateSingle1D(fm_int               sw,
                                   const fm_cachedRegs *regSet,     
                                   fm_uint32            data, 
                                   fm_uint32            idx)
{
    fm_uint32 *cache;
    fm_uint32 indices[FM_REGS_CACHE_MAX_INDICES] = {idx, 0, 0};

    /* sanity checks on the regSet argument */
    if ( regSet == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }
    if ( regSet->nIndices > 1 || idx >= regSet->nElements[0] )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "fmRegCacheUpdateSingle1D: not a 1D register: 0x%08X!\n",
                     regSet->baseAddr);
        return FM_ERR_INVALID_ARGUMENT;
    }

    cache   = regSet->getCache.data( sw );
    cache  += fmRegCacheComputeOffset( indices, regSet );
    *cache  = data;

    return FM_OK;

}   /* end fmRegCacheUpdateSingle1D */




/*****************************************************************************/
/** fmRegCacheIsAddrRangeCached
 * \ingroup intRegCache
 *
 * \desc            Determines whether the specified address range is in
 *                  the register cache.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lowAddr is the lower address in the range.
 * 
 * \param[in]       hiAddr is the upper address in the range.
 *
 * \param[out]      cached points to a Boolean that will be set to TRUE
 *                  if the range is cached, FALSE otherwise.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheIsAddrRangeCached(fm_int     sw, 
                                      fm_uint32  lowAddr, 
                                      fm_uint32  hiAddr,
                                      fm_bool   *cached) 
{
    fm_cachedRegs *cachedReg;
    fm_uint32      baseAddr;
    fm_uint32      maxAddr;
    fm_uint32      idx[FM_REGS_CACHE_MAX_INDICES];
    fm_byte        i;
    fm_switch      *switchPtr;

   /* get the switch pointer and initialize its cache */
    VALIDATE_SWITCH_INDEX(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    cachedReg = *(switchPtr->CachedRegisterList);    

    /* scan the whole cached register set list */
    *cached = FALSE;
    while ( cachedReg != NULL )
    {
        baseAddr = cachedReg->baseAddr;
        for (i = 0 ; i < cachedReg->nIndices ; i++)
        {
            idx[i] = cachedReg->nElements[i]-1;
        }
        maxAddr   = fmRegCacheComputeAddr( idx, cachedReg );
        maxAddr  += cachedReg->nWords - 1;
        if ( (baseAddr <= lowAddr) && (lowAddr <= maxAddr) &&
             (baseAddr <= hiAddr ) && (hiAddr  <= maxAddr) )
        {
            *cached = TRUE;
            break;
        }
        cachedReg++;
    }

    return FM_OK;

}   /* end fmRegCacheIsAddrRangeCached */




/*****************************************************************************/
/** fmRegCacheReadKeyValid
 * \ingroup intRegCache
 *
 * \chips           FM6000, FM10000
 *
 * \desc            This function sets the local cache of the two bits 
 *                  corresponding to Bit0 of key and keyInvert in a
 *                  CAM-type register entry. The array holds two bits
 *                  per register entry
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet pointer to the cached register set
 * 
 * \param[in]       idx is a pointer to a caller-allocated storage containing
 *                  up to FM_REGS_CACHE_MAX_INDICES indices that identify a
 *                  single register in its set. It is up to the call to ensure
 *                  the array is adequately sized to contain all indices needed
 *                  by the register set. Extra indices will be ignored
 *
 * \param[out]      valid is a pointer to a caller-allocated storage that will
 *                  hold the returned value for the KeyValid bit pair
 * 
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if one or more of the arguments
 *                  are invalid. That includes the case where the register set
 *                  pointed to by regSet does not have a local KeyValid cache
 * \return          FM_ERR_INVALID_SWITCH is the switch ID is not valid
 *
 *****************************************************************************/
fm_status fmRegCacheReadKeyValid(fm_int                sw,
                                 const fm_cachedRegs  *regSet,
                                 fm_uint32            *idx,
                                 fm_regsCacheKeyValid *valid)
{
    fm_status    err;
    fm_uint32    bitOffset;
    fm_uint32    product;
    fm_int       nIndices;
    fm_int       i;
    fm_byte      bitPair;
    fm_bitArray *bitArray;
    fm_bool      bitValue;

    /* validate the switch index */
    VALIDATE_SWITCH_INDEX(sw);

    /* sanity check on the arguments */
    if ( regSet == NULL || regSet->getCache.valid == NULL ||
         idx == NULL    || valid == NULL  )
    {
        /* invalid */
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* NOTE: a more robust solution would be to pass the actual array size,
     * and test against this value instead of FM_REGS_CACHE_MAX_INDICES. */
    nIndices = regSet->nIndices;
    if (nIndices > FM_REGS_CACHE_MAX_INDICES)
    {
#if 0
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                     "regSet->nIndices (%d) > MAX_INDICES (%d)\n",
                     nIndices,
                     FM_REGS_CACHE_MAX_INDICES);
#endif
        nIndices = FM_REGS_CACHE_MAX_INDICES;
    }

    /* make sure no index is out of range */
    for (i = 0 ; i < nIndices ; i++)
    {
        if (idx[i] >= regSet->nElements[i])
        {
            return FM_ERR_INVALID_ARGUMENT;
        }
    }

    /* finally, make sure this register set as an associated bit array */
    bitArray = regSet->getCache.valid(sw);
    if ( bitArray == NULL )
    {
        return  FM_ERR_INVALID_ARGUMENT;
    }

    /* determine the offset of the bit pair in the cache */
    bitOffset = 0;
    product   = 2;
    for ( i = 0 ; i < nIndices ; i++ )
    {
        bitOffset += product*idx[i];  
        product   *= regSet->nElements[i];
    }

    /* get the value of bit at position bitOffset+1 */
    err = fmGetBitArrayBit( bitArray, bitOffset+1, &bitValue );
    if ( err == FM_OK )
    {
        bitPair = 0;

        /* is it a '1'? */
        if (bitValue == TRUE)
        {
            /* yes */
            bitPair = 2;
        }

        /* get the value of bit at position bitOffset */
        fmGetBitArrayBit( bitArray, bitOffset, &bitValue );

        /* is it a '1'? */
        if ( bitValue == TRUE )
        {
            /* yes */
            bitPair += 1;
        }

        switch ( bitPair )
        {
            /* Bit0 of key and keyInvert are both '0' */
            case 0:
                *valid = FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_0;
                break;

            /* Bit0 of key is '1'; Bit0 of keyInvert is '0' */
            case 1:
                *valid = FM_REGS_CACHE_KEY_IS_1;
                break;

            /* Bit0 of key is '0'; Bit0 of keyInvert is '1' */
            case 2:
                *valid = FM_REGS_CACHE_KEYINVERT_IS_1;
                break;

            /* Bit0 of key and keyInvert are both '1' */
            case 3:
                *valid = FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1;
                break;

        }   /* end switch (bitPair) */

    }   /* end if (err == FM_OK) */

    return err;

}   /* end fmRegCacheReadKeyValid */




/*****************************************************************************/
/** fmRegCacheWriteKeyValid
 * \ingroup intRegCache
 *
 * \chips           FM6000, FM10000
 *
 * \desc            This function sets the local cache of the two bits 
 *                  corresponding to Bit0 of key and keyInvert in a
 *                  CAM-type register entry. The array holds two bits
 *                  per register entry
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet pointer to the cached register set
 * 
 * \param[in]       idx is a pointer to a caller-allocated storage containing
 *                  up to FM_REGS_CACHE_MAX_INDICES indices that identify a
 *                  single register in its set. It is up to the call to ensure
 *                  the array is adequately sized to contain all indices needed
 *                  by the register set. Extra indices will be ignored
 *
 * \param[out]      valid is the value of the KeyValid bit pair to be stored
 *                  to the local cache 
 * 
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if one or more of the arguments
 *                  are invalid. That includes the case where the register set
 *                  pointed to by regSet does not have a local KeyValid cache
 * \return          FM_ERR_INVALID_SWITCH is the switch ID is not valid
 *
 *****************************************************************************/
fm_status fmRegCacheWriteKeyValid(fm_int               sw,             
                                  const fm_cachedRegs *regSet,   
                                  fm_uint32           *idx,     
                                  fm_regsCacheKeyValid valid)
{
    fm_status    err;
    fm_uint32    bitOffset;
    fm_uint32    product;
    fm_int       nIndices;
    fm_int       i;
    fm_bitArray *bitArray;
    fm_bool      bitValue0;
    fm_bool      bitValue1;

    /* validate the switch index */
    VALIDATE_SWITCH_INDEX(sw);

    /* sanity check on the arguments */
    if ( regSet == NULL || regSet->getCache.valid == NULL || idx == NULL )
    {
        /* invalid */
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* NOTE: a more robust solution would be to pass the actual array size,
     * and test against this value instead of FM_REGS_CACHE_MAX_INDICES. */
    nIndices = regSet->nIndices;
    if (nIndices > FM_REGS_CACHE_MAX_INDICES)
    {
#if 0
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                     "regSet->nIndices (%d) > MAX_INDICES (%d)\n",
                     nIndices,
                     FM_REGS_CACHE_MAX_INDICES);
#endif
        nIndices = FM_REGS_CACHE_MAX_INDICES;
    }

    /* make sure no index is out of range */
    for (i = 0 ; i < nIndices ; i++)
    {
        if (idx[i] >= regSet->nElements[i])
        {
            return FM_ERR_INVALID_ARGUMENT;
        }
    }

    /* all argument are valid... */
    bitArray = regSet->getCache.valid(sw);
    if ( bitArray == NULL )
    {
        /* ...but this register set doesn't have a valid bit array */
        return  FM_ERR_INVALID_ARGUMENT;
    }

    /* determine the offset of the bit pair in the cache */
    bitOffset = 0;
    product   = 2;
    for ( i = 0 ; i < nIndices ; i++ )
    {
        bitOffset += product*idx[i];  
        product   *= regSet->nElements[i];
    }

    switch ( valid )
    {
        case FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_0:
            /* Bit0 of key and keyInvert are both '0' */
            bitValue0 = FALSE;
            bitValue1 = FALSE;
            break;
    
        case FM_REGS_CACHE_KEY_IS_1:
            /* Bit0 of key is '1'; Bit0 of keyInvert is '0' */
            bitValue0 = TRUE;
            bitValue1 = FALSE;
            break;

        case FM_REGS_CACHE_KEYINVERT_IS_1:
            /* Bit0 of key is '0'; Bit0 of keyInvert is '1' */
            bitValue0 = FALSE;
            bitValue1 = TRUE;
            break;

        case FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1:
            /* Bit0 of key and keyInvert are both '1' */
            bitValue0 = TRUE;
            bitValue1 = TRUE;
            break;
                           
        default:
            /* This shouldn't happen, assert if it does */
            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_GENERAL, 
                                   FALSE, 
                                   err = FM_FAIL, 
                                   "fmRegCacheWriteKeyValid: unexpected "
                                   "value for valid field: %d\n",
                                   valid);
            break;

    }   /* end switch (valid) */
    

    /* set the bit at offset 'bitOffset+1' first */
    err = fmSetBitArrayBit( bitArray, bitOffset + 1, bitValue1 );
    if ( err == FM_OK )
    {
        /* now set the bit at offset 'bitOffset' */
        fmSetBitArrayBit( bitArray, bitOffset, bitValue0 );
    }

ABORT:
    return err;

}   /* end fmRegCacheWriteKeyValid */




/*****************************************************************************/
/** fmRegCacheRestoreKeyValid
 * \ingroup intRegCache
 *
 * \chips           FM6000, FM10000
 *
 * \desc            This function takes a key/keyInvert pair and restores its
 *                  Bit0 pair to make it valid. It is up to the caller to
 *                  perform the actual write operation to the register 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet pointer to the cached register set
 * 
 * \param[in]       idx is a pointer to a caller-allocated storage containing
 *                  up to FM_REGS_CACHE_MAX_INDICES indices that identify a
 *                  single register in its set. It is up to the call to ensure
 *                  the array is adequately sized to contain all indices needed
 *                  by the register set. Extra indices will be ignored
 * 
 * \param[in, out]  key pointer to the key value
 *
 * \param[in,out]   keyInvert pointer to the inverted key value
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if one or more of the arguments
 *                  are invalid. That includes the case where the register set
 *                  pointed to by regSet does not have a local KeyValid cache
 * \return          FM_ERR_INVALID_SWITCH is the switch ID is not valid
 * \return          FM_ERR_INVALID_RULE if the rule has never been set,
 *                  therefore its valid value can't be restored. 
 *
 *****************************************************************************/
fm_status fmRegCacheRestoreKeyValid(fm_int               sw,
                                    const fm_cachedRegs *regSet,
                                    fm_uint32           *idx,
                                    fm_uint32           *key,
                                    fm_uint32           *keyInvert)

{
    fm_status            err;
    fm_regsCacheKeyValid valid;

    if ( key == NULL || keyInvert == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    err = fmRegCacheReadKeyValid(sw, regSet, idx, &valid);

    if ( err == FM_OK )
    {
        /* set Bit0 of key if necessary */
        if ( valid == FM_REGS_CACHE_KEY_IS_1 ||
             valid == FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1)
        {
            *key |= FM_LITERAL_64(1);
        }

        /* set Bit0 of keyInvert if necessary */
        if ( valid == FM_REGS_CACHE_KEYINVERT_IS_1 ||
             valid == FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1)
        {
            *keyInvert |= FM_LITERAL_64(1);
        }

    }   /* end if ( err == FM_OK ) */

    return err;

}   /* end fmRegCacheRestoreKeyValid */




/*****************************************************************************/
/** fmRegCacheCopyKeyValid
 * \ingroup intRegCache
 *
 * \chips           FM6000, FM10000
 *
 * \desc            This function takes a key/keyInvert pair from a given
 *                  register and copies to a different register of the same
 *                  set. It is up to the caller to perform the actual
 *                  write operation to the destination register 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet pointer to the cached register set
 * 
 * \param[in]       fromIdx is a pointer to a caller-allocated storage containing
 *                  up to FM_REGS_CACHE_MAX_INDICES indices that identify the
 *                  source register. It is up to the caller to ensure
 *                  the array is adequately sized to contain all indices needed
 *                  by the register set. Extra indices will be ignored
 * 
 * \param[in]       toIdx is a pointer to a caller-allocated storage containing
 *                  up to FM_REGS_CACHE_MAX_INDICES indices that identify the
 *                  destination register . It is up to the caller to ensure
 *                  the array is adequately sized to contain all indices needed
 *                  by the register set. Extra indices will be ignored
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if one or more of the arguments
 *                  are invalid. That includes the case where the register set
 *                  pointed to by regSet does not have a local KeyValid cache
 * \return          FM_ERR_INVALID_SWITCH is the switch ID is not valid
 *
 *****************************************************************************/
fm_status fmRegCacheCopyKeyValid(fm_int                sw,
                                 const fm_cachedRegs  *regSet,
                                 fm_uint32            *fromIdx,
                                 fm_uint32            *toIdx)
{
    fm_status err;
    fm_regsCacheKeyValid valid;

    /* simply, read from the source register...*/
    err = fmRegCacheReadKeyValid(sw, regSet, fromIdx, &valid);
    if ( err == FM_OK )
    {
        /* ... and if everything worked write to the destination register */
        err = fmRegCacheWriteKeyValid(sw, regSet, toIdx, valid);
    }
    return err;

}   /* end fmRegCacheCopyKeyValid */




/*****************************************************************************/
/** fmRegCacheWriteFromCache
 * \ingroup intRegCache
 *
 * \chips           FM10000
 *
 * \desc            Writes one or more consecutive register entries to the
 *                  hardware from the register cache.
 * 
 * \note            This function is intended for use by the parity error
 *                  repair handler, to restore the contents of a register
 *                  table from the software cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register set descriptor.
 * 
 * \param[in]       indices points to an array specifying the indices of the
 *                  first entry to write.
 * 
 * \param[in]       nEntries is the number of register entries to write.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheWriteFromCache(fm_int                sw,
                                   const fm_cachedRegs * regSet,
                                   const fm_uint32 *     indices,
                                   fm_int                nEntries)
{
    fm_switch *                 switchPtr;
    fm_scatterGatherListEntry   sgList[1];
    fm_registerSGListEntry      entry;
    fm_int                      sgListSize;
    fm_uint32                   regAddr;
    fm_uint32                   numWords;
    fm_uint32 *                 cachePtr;
    fm_uint32                   burstSize;
    fm_uint32                   wcount;
    fm_status                   err;

    switchPtr = GET_SWITCH_PTR(sw);

    if (regSet == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = NULL;
    entry.count       = nEntries;
    entry.idx[0]      = indices[0];
    entry.idx[1]      = indices[1];
    entry.idx[2]      = indices[2];
    entry.rewriting   = FALSE;

    if ( !IsScatterGatherListCorrect(&entry, 1) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "invalid scatter-gather list\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.data  = regSet->getCache.data(sw);
    entry.data += fmRegCacheComputeOffset(indices, regSet);

    /* Find out how many scatter-gather entries we will need. */
    sgListSize = fmRegCacheConvSGList(sw, 1, &entry, NULL, FALSE);

    if (sgListSize != 1)
    {
        /* Note that this is not a catastrophic error, it just means that the
         * function needs to be updated to handle multiple entries. */
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "sgListSize (%d) != 1\n", sgListSize);
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Get the actual addresses. */
    fmRegCacheConvSGList(sw, 1, &entry, sgList, FALSE);

    regAddr  = sgList[0].addr;
    numWords = sgList[0].count;
    cachePtr = sgList[0].data;

    /* The number of words to write at a time. Make it an multiple of the
     * entry size, so we restore the register table in integral units. */
    burstSize = (CACHE_BURST_SIZE / regSet->nWords) * regSet->nWords;

#if (DEBUG_PARITY)
    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "burstSize = %u iterations = %u\n",
                 burstSize,
                 (numWords + burstSize - 1) / burstSize);
#endif

    while (numWords != 0)
    {
        wcount = (numWords > burstSize) ? burstSize : numWords;
        if (wcount == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "wcount = 0\n");
            return FM_FAIL;
        }

#if (DEBUG_PARITY)
        FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                     "restoring regAddr %08x wcount %u\n",
                     regAddr,
                     wcount);
#endif

        TAKE_REG_LOCK(sw);
        err = switchPtr->WriteUINT32Mult(sw, regAddr, wcount, cachePtr);
        DROP_REG_LOCK(sw);

        if (err != FM_OK)
        {
            return err;
        }

        regAddr  += wcount;
        cachePtr += wcount;
        numWords -= wcount;
    }

    return FM_OK;

}   /* end fmRegCacheWriteFromCache */




/*****************************************************************************/
/** fmRegCacheComputeChecksum
 * \ingroup intRegCache
 *
 * \chips           FM10000
 *
 * \desc            Computes the CRM checksum for a block of consecutive
 *                  entries in the register cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register set descriptor.
 * 
 * \param[in]       indices points to an array specifying the indices of the
 *                  first entry to write.
 * 
 * \param[in]       nEntries is the number of register entries to write.
 * 
 * \param[out]      checksum points to the location to receive the checksum.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRegCacheComputeChecksum(fm_int                sw,
                                    const fm_cachedRegs * regSet,
                                    const fm_uint32 *     indices,
                                    fm_int                nEntries,
                                    fm_uint32 *           checksum)
{
    fm_scatterGatherListEntry   sgList[1];
    fm_registerSGListEntry      entry;
    fm_int                      sgListSize;
    fm_uint32                   numWords;
    fm_uint32 *                 cachePtr;
    fm_uint32                   value;

    FM_NOT_USED(sw);

    if (regSet == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = NULL;
    entry.count       = nEntries;
    entry.idx[0]      = indices[0];
    entry.idx[1]      = indices[1];
    entry.idx[2]      = indices[2];
    entry.rewriting   = FALSE;

    if ( !IsScatterGatherListCorrect(&entry, 1) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "invalid scatter-gather list\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.data  = regSet->getCache.data(sw);
    entry.data += fmRegCacheComputeOffset(indices, regSet);

    /* Find out how many scatter-gather entries we will need. */
    sgListSize = fmRegCacheConvSGList(sw, 1, &entry, NULL, FALSE);

    if (sgListSize != 1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "sgListSize (%d) != 1\n", sgListSize);
        return FM_ERR_INVALID_ARGUMENT;
    }

    fmRegCacheConvSGList(sw, 1, &entry, sgList, FALSE);

    numWords = sgList[0].count;
    cachePtr = sgList[0].data;
    value    = 0;

    while (numWords != 0)
    {
        value ^= *cachePtr;
        cachePtr++;
        numWords--;
    }

#if 0
    FM_LOG_PRINT("idx=[%d,%d,%d] nEntries=%d regAddr=%08x cachePtr=%p count=%u "
                 "value=%08x\n",
                 indices[0], indices[1], indices[2],
                 nEntries,
                 sgList[0].addr,
                 (void *)sgList[0].data,
                 sgList[0].count,
                 value);
#endif

    *checksum = value;

    return FM_OK;

}   /* end fmRegCacheComputeChecksum */




/*****************************************************************************/
/** fmDbgDumpRegCacheEntry
 * \ingroup intRegCache
 *
 * \chips           FM10000
 *
 * \desc            Generates and dumps a scatter-gather list for a block
 *                  of consecutive entries in the register cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regSet points to the register set descriptor.
 * 
 * \param[in]       indices points to an array specifying the indices of the
 *                  first entry to write.
 * 
 * \param[in]       nEntries is the number of register entries to write.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpRegCacheEntry(fm_int                sw,
                                 const fm_cachedRegs * regSet,
                                 const fm_uint32 *     indices,
                                 fm_int                nEntries)
{
    fm_scatterGatherListEntry   sgList[10];
    fm_scatterGatherListEntry * sgItem;
    fm_registerSGListEntry      entry;
    fm_int                      sgListSize;
    fm_int                      i;

    FM_NOT_USED(sw);

    if (regSet == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.registerSet = regSet;
    entry.data        = NULL;
    entry.count       = nEntries;
    entry.idx[0]      = indices[0];
    entry.idx[1]      = indices[1];
    entry.idx[2]      = indices[2];
    entry.rewriting   = FALSE;

    if ( !IsScatterGatherListCorrect(&entry, 1) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "invalid scatter-gather list\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    entry.data  = regSet->getCache.data(sw);
    entry.data += fmRegCacheComputeOffset(indices, regSet);

    sgListSize = fmRegCacheConvSGList(sw, 1, &entry, NULL, FALSE);

    if (sgListSize < 1 || sgListSize > (fm_int)FM_NENTRIES(sgList))
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                     "invalid sgListsize (%d)\n", sgListSize);
        return FM_ERR_INVALID_ARGUMENT;
    }

    fmRegCacheConvSGList(sw, 1, &entry, sgList, FALSE);

    FM_LOG_PRINT("No  Regaddr    Count\n");
    FM_LOG_PRINT("--  --------  ------\n");

    for (i = 0 ; i < sgListSize ; i++)
    {
        sgItem = &sgList[i];

        FM_LOG_PRINT("%2d  %08x  %6d\n", i, sgItem->addr, sgItem->count);
    }

    return FM_OK;

}   /* end fmDbgDumpRegCacheEntry */

