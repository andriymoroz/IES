/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_policer.c
 * Creation Date:  May 31, 2013
 * Description:    Low-level API for manipulating Policers
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM_API_REQUIRE(expr, failCode)       \
    if ( !(expr) )                           \
    {                                        \
        err = failCode;                      \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU,  \
                            failCode);       \
    }

/*****************************************************************************
 * Local function prototypes
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
 
 

/*****************************************************************************/
/** fmSupportsPolicer
 * \ingroup intLowlevPol10k
 *
 * \desc            Determines whether the specified switch has an FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          TRUE if the switch has an FFU.
 * \return          FALSE if the switch does not have an FFU.
 *
 *****************************************************************************/
static inline fm_bool fmSupportsPolicer(fm_int sw)
{
    return ( (fmRootApi->fmSwitchStateTable[sw]->switchFamily ==
              FM_SWITCH_FAMILY_FM10000) );

}   /* end fmSupportsPolicer */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000PolicerInit
 * \ingroup intLowlevPol10K
 *
 * \desc            Private initialization function called from 
 *                  ''fm10000PostBootSwitch''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000PolicerInit(fm_int sw)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint64  freq;
    fm_uint64  period;
    fm_float   fhMhz;
    fm_uint64  periodCfg = 0LL;;

    switchPtr = GET_SWITCH_PTR(sw);

    err = fm10000ComputeFHClockFreq(sw, &fhMhz);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    /* Set the Sweeper to a 2.5KHz regardless of the frame handler clock */
    freq = fhMhz * 1e6;
    period = (freq * 4) / 10000;

    FM_SET_FIELD64(periodCfg,
                   FM10000_POLICER_SWEEPER_PERIOD_CFG,
                   Period,
                   period);
    FM_SET_BIT64(periodCfg,
                 FM10000_POLICER_SWEEPER_PERIOD_CFG,
                 SweeperEnable,
                 1);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_POLICER_SWEEPER_PERIOD_CFG(0),
                                 periodCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:

    return err;

}   /* end fm10000PolicerInit */




/*****************************************************************************/
/** fm10000ConvertPolicerRate
 * \ingroup lowlevPol10K
 * 
 * \desc            Converts a policer rate value to its mantissa and exponent.
 *
 * \param[in]       rate is the rate to be converted (in kb/s).
 *
 * \param[out]      mantissa points to a caller-supplied location to receive
 *                  the 4-bit mantissa.
 *
 * \param[out]      exponent points to a caller-supplied location to receive
 *                  the 5-bit exponent.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConvertPolicerRate(fm_uint32 rate,
                                    fm_uint*  mantissa,
                                    fm_uint*  exponent)
{
    /* rate in kb/s, POLICER_SWEEPER_PERIOD_CFG.Period = xxx ==> freq 2.5 KHz */
    fm_uint   e = 0;
    fm_uint64 m;
    fm_int    sweeperFreq = 2500; // 2.5KHz

    if (rate == 0)
    {
        *mantissa = 0;
        *exponent = 0;
        return FM_OK;
    }

    m = ((fm_uint64)rate * 1000LL) / ((fm_uint64)sweeperFreq * 8LL);

    /* Shift bits out of the mantissa and increment the exponent
     * until the mantissa will fit in four bits. */
    while (m > 15)
    {
        m >>= 1;
        e++;
    }

    /* Ensure that the mantissa is >= 1. */
    if (m < 1)
    {
        m = 1;
    }

    /* Make sure the exponent will fit in 5 bits. If it won't,
     * set the mantissa and the exponent to their maximum values. */
    if (e > 31)
    {
        m = 15;
        e = 31;
    }

    *mantissa = m;
    *exponent = e;

    return FM_OK;

}   /* end fm10000ConvertPolicerRate */



/*****************************************************************************/
/** fm10000ConvertPolicerCapacity
 * \ingroup lowlevPol10K
 * 
 * \desc            Converts a policer capacity value to mantissa and exponent.
 *
 * \param[in]       capacity is the capacity to be converted (in bytes).
 *
 * \param[out]      mantissa points to a caller-supplied location to receive
 *                  the 4-bit mantissa.
 *
 * \param[out]      exponent points to a caller-supplied location to receive
 *                  the 5-bit exponent.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConvertPolicerCapacity(fm_uint32 capacity,
                                        fm_uint * mantissa,
                                        fm_uint * exponent)
{
    /* capacity in bytes */
    fm_uint   e = 0;
    fm_uint   m;

    if (capacity == 0)
    {
        *mantissa = 0;
        *exponent = 0;
        return FM_OK;
    }

    m = capacity;

    /* Shift bits out of the mantissa and increment the exponent
     * until the mantissa will fit in four bits. */
    while (m > 15)
    {
        m >>= 1;
        e++;
    }

    /* Ensure that the mantissa is >= 1. */
    if (m < 1)
    {
        m = 1;
    }

    /* Make sure the exponent will fit in 5 bits. If it won't,
     * set the mantissa and the exponent to their maximum values. */
    if (e > 31)
    {
        m = 15;
        e = 31;
    }

    *mantissa = m;
    *exponent = e;

    return FM_OK;

}   /* end fm10000ConvertPolicerCapacity */



/*****************************************************************************/
/** fm10000SetPolicerCounter
 * \ingroup lowlevPol10K
 *
 * \desc            Set a policer counter value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       index is the policer entry within the bank and ranges from
 *                  0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[in]       frameCount is the frame count to set.
 *
 * \param[in]       byteCount is the octet count to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerCounter(fm_int                sw,
                                   fm_fm10000PolicerBank bank,
                                   fm_uint16             index,
                                   fm_uint64             frameCount,
                                   fm_uint64             byteCount)
{
    return fm10000SetPolicerCounters(sw,
                                     bank,
                                     index,
                                     1,
                                     &frameCount,
                                     &byteCount);
    
}   /* end fm10000SetPolicerCounter */



/*****************************************************************************/
/** fm10000GetPolicerCounter
 * \ingroup lowlevPol10K
 *
 * \desc            Get a policer counter value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       index is the policer entry within the bank and ranges from
 *                  0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[out]      frameCount points to caller-allocated storage where this
 *                  function should place the frame count.
 *
 * \param[out]      byteCount points to caller-allocated storage where this
 *                  function should place the octet count.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetPolicerCounter(fm_int                sw,
                                   fm_fm10000PolicerBank bank,
                                   fm_uint16             index,
                                   fm_uint64 *           frameCount,
                                   fm_uint64 *           byteCount)
{
    return fm10000GetPolicerCounters(sw,
                                     bank,
                                     index,
                                     1,
                                     frameCount,
                                     byteCount);
    
}   /* end fm10000GetPolicerCounter */



/*****************************************************************************/
/** fm10000SetPolicerCounters
 * \ingroup lowlevPol10K
 *
 * \desc            Set the values of multiple consecutive policer counters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       firstIndex is the index of the first policer entry to set
 *                  and ranges from 0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[in]       numCounters is the number of counters to set. firstIndex
 *                  + numCounters - 1 must be less than or equal to the max
 *                  policer index.
 *
 * \param[in]       frameCounts is an array of length numCounters containing
 *                  the frame counts to set for each policer entry.
 *
 * \param[in]       byteCounts is an array of length numCounters containing
 *                  the octet counts to set for each policer entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if firstIndex is invalid
 *                  or firstIndex + numCounters - 1 is greater than
 *                  FM_FM10000_MAX_POLICER_INDEX.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerCounters(fm_int                sw,
                                    fm_fm10000PolicerBank bank,
                                    fm_uint16             firstIndex,
                                    fm_uint16             numCounters,
                                    const fm_uint64 *     frameCounts,
                                    const fm_uint64 *     byteCounts)
{
    fm_uint16              maxCounterIndex;
    fm_registerSGListEntry sgList;
    fm_uint32             *data;
    fm_uint32             *dataPtr;
    fm_cleanupListEntry *  cleanupList = NULL;
    fm_status              err = FM_OK;
    fm_uint32              index;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, bank = %d, firstIndex = %d, numCounters = %d\n",
                  sw,
                  bank,
                  firstIndex,
                  numCounters );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /**************************************************
     * sanity check on the arguments
     **************************************************/

    FM_API_REQUIRE(bank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        maxCounterIndex = FM_FM10000_MAX_POLICER_4K_INDEX;
    }
    else
    {
        maxCounterIndex = FM_FM10000_MAX_POLICER_512_INDEX;
    }

    FM_API_REQUIRE(firstIndex + numCounters <= 
                   maxCounterIndex + 1,
                   FM_ERR_INVALID_ARGUMENT);

    FM_API_REQUIRE(numCounters != 0,
                   FM_ERR_INVALID_ARGUMENT );

    /**************************************************
     * let's get started: first allocate a temp array 
     * to fill out all register values before the
     * actual write operation
     **************************************************/
    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numCounters * FM10000_POLICER_STATE_4K_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CachePolicerState4k,
                                  numCounters,
                                  firstIndex,
                                  bank,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);
    }
    else
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numCounters * FM10000_POLICER_STATE_512_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CachePolicerState512,
                                  numCounters,
                                  firstIndex,
                                  bank - FM_FM10000_POLICER_BANK_512_1,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);
    }

    dataPtr = data;

    for (index = 0 ; index < numCounters ; index++)
    {
        FM_ARRAY_SET_FIELD64(dataPtr,
                             FM10000_POLICER_STATE_4K,
                             Count1,
                             frameCounts[index]);

        FM_ARRAY_SET_FIELD64(dataPtr,
                             FM10000_POLICER_STATE_4K,
                             Count2,
                             byteCounts[index]);

        dataPtr += FM10000_POLICER_STATE_4K_WIDTH;
    }

    /* now write to the register and/or their cached values */
    err = fmRegCacheWrite(sw, 1, &sgList, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);
    
}   /* end fm10000SetPolicerCounters */



/*****************************************************************************/
/** fm10000GetPolicerCounters
 * \ingroup lowlevPol10K
 *
 * \desc            Get the values of multiple consecutive policer counters. 
 *
 * \note            For each array function argument below, the element placed 
 *                  in the array at index 0 will correspond to the policer 
 *                  identified by firstIndex, index 1 will correspond to 
 *                  firstIndex + 1, etc., up to the element at index 
 *                  numCounters - 1, which will correspond to the policer 
 *                  identified by firstIndex + numCounters - 1.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       firstIndex is the index of the first policer entry to set
 *                  and ranges from 0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[in]       numCounters is the number of counters to set. firstIndex
 *                  + numCounters - 1 must be less than or equal to the max
 *                  policer index.
 *
 * \param[out]      frameCounts points to a caller-allocated array, numCounters
 *                  elements in length, into which each counter's frame count
 *                  will be placed by this function. 
 *
 * \param[out]      byteCounts points to a caller-allocated array, numCounters
 *                  elements in length, into which each counter's octet count
 *                  will be placed by this function. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if firstIndex is invalid
 *                  or firstIndex + numCounters - 1 is greater than
 *                  FM_FM10000_MAX_POLICER_INDEX.
 *
 *****************************************************************************/
fm_status fm10000GetPolicerCounters(fm_int                sw,
                                    fm_fm10000PolicerBank bank,
                                    fm_uint16             firstIndex,
                                    fm_uint16             numCounters,
                                    fm_uint64 *           frameCounts,
                                    fm_uint64 *           byteCounts)
{
    fm_uint16              maxCounterIndex;
    fm_registerSGListEntry sgList;
    fm_uint32             *data;
    fm_uint32             *dataPtr;
    fm_cleanupListEntry *  cleanupList = NULL;
    fm_status              err = FM_OK;
    fm_uint32              index;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, bank = %d, firstIndex = %d, numCounters = %d\n",
                  sw,
                  bank,
                  firstIndex,
                  numCounters );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /**************************************************
     * sanity check on the arguments
     **************************************************/

    FM_API_REQUIRE(bank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        maxCounterIndex = FM_FM10000_MAX_POLICER_4K_INDEX;
    }
    else
    {
        maxCounterIndex = FM_FM10000_MAX_POLICER_512_INDEX;
    }

    FM_API_REQUIRE(firstIndex + numCounters <= 
                   maxCounterIndex + 1,
                   FM_ERR_INVALID_ARGUMENT);

    FM_API_REQUIRE(numCounters != 0,
                   FM_ERR_INVALID_ARGUMENT );

    /**************************************************
     * let's get started: first allocate a temp array 
     * to fill out all register values before the
     * actual write operation
     **************************************************/
    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numCounters * FM10000_POLICER_STATE_4K_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CachePolicerState4k,
                                  numCounters,
                                  firstIndex,
                                  bank,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);
    }
    else
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numCounters * FM10000_POLICER_STATE_512_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CachePolicerState512,
                                  numCounters,
                                  firstIndex,
                                  bank - FM_FM10000_POLICER_BANK_512_1,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);
    }

    err = fmRegCacheRead(sw, 1, &sgList, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    dataPtr = data;

    for (index = 0 ; index < numCounters ; index++)
    {
        frameCounts[index] = FM_ARRAY_GET_FIELD64(dataPtr,
                                                  FM10000_POLICER_STATE_4K,
                                                  Count1);

        byteCounts[index] = FM_ARRAY_GET_FIELD64(dataPtr,
                                                 FM10000_POLICER_STATE_4K,
                                                 Count2);

        dataPtr += FM10000_POLICER_STATE_4K_WIDTH;
    }


ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetPolicerCounters */



/*****************************************************************************/
/** fm10000SetPolicer
 * \ingroup lowlevPol10K
 *
 * \desc            Set a policer value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       index is the policer entry within the bank and ranges from
 *                  0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[in]       committed is the policer state for the committed rate.
 *
 * \param[in]       excess is the policer state for the excess rate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if index is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of committed or
 *                  excess is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetPolicer(fm_int                    sw,
                            fm_fm10000PolicerBank     bank,
                            fm_uint16                 index,
                            const fm_ffuPolicerState *committed,
                            const fm_ffuPolicerState *excess)
{
    return fm10000SetPolicers(sw,
                              bank,
                              index,
                              1,
                              committed,
                              excess);
    
}   /* end fm10000SetPolicer */



/*****************************************************************************/
/** fm10000GetPolicer
 * \ingroup lowlevPol10K
 *
 * \desc            Get a policer value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       index is the policer entry within the bank and ranges from
 *                  0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[out]      committed is the policer state for the committed rate.
 *
 * \param[out]      excess is the policer state for the excess rate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if index is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetPolicer(fm_int                sw,
                            fm_fm10000PolicerBank bank,
                            fm_uint16             index,
                            fm_ffuPolicerState *  committed,
                            fm_ffuPolicerState *  excess)
{
    return fm10000GetPolicers(sw,
                              bank,
                              index,
                              1,
                              committed,
                              excess);
    
}   /* end fm10000GetPolicer */



/*****************************************************************************/
/** fm10000SetPolicers
 * \ingroup lowlevPol10K
 *
 * \desc            Set the values of multiple consecutive policers. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to set (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       firstIndex is the index of the first policer entry to set
 *                  and ranges from 0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[in]       numPolicers is the number of policers to set. firstIndex
 *                  + numPolicers - 1 must be less than or equal to the max
 *                  policer index.
 *
 * \param[in]       committed is an array of length numPolicers containing
 *                  the policer state for the committed rate.
 *
 * \param[in]       excess is an array of length numPolicers containing
 *                  the policer state for the excess rate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if firstIndex is invalid
 *                  or firstIndex + numPolicers - 1 is greater than
 *                  FM_FM10000_MAX_POLICER_INDEX.
 * \return          FM_ERR_INVALID_ARGUMENT if any member of committed or
 *                  excess is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetPolicers(fm_int                    sw,
                             fm_fm10000PolicerBank     bank,
                             fm_uint16                 firstIndex,
                             fm_uint16                 numPolicers,
                             const fm_ffuPolicerState *committed,
                             const fm_ffuPolicerState *excess)
{

    fm_uint16              maxPolicerIndex;
    fm_registerSGListEntry sgList[2];
    fm_uint32             *data;
    fm_uint32             *dataPtr;
    fm_cleanupListEntry *  cleanupList = NULL;
    fm_status              err = FM_OK;
    fm_uint32              index;
    fm_uint32              policerCfgEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, bank = %d, firstIndex = %d, numPolicers = %d\n",
                  sw,
                  bank,
                  firstIndex,
                  numPolicers );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /**************************************************
     * sanity check on the arguments
     **************************************************/

    FM_API_REQUIRE(bank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        maxPolicerIndex = FM_FM10000_MAX_POLICER_4K_INDEX;
    }
    else
    {
        maxPolicerIndex = FM_FM10000_MAX_POLICER_512_INDEX;
    }

    FM_API_REQUIRE(firstIndex + numPolicers <= 
                   maxPolicerIndex + 1,
                   FM_ERR_INVALID_ARGUMENT);

    FM_API_REQUIRE(numPolicers != 0,
                   FM_ERR_INVALID_ARGUMENT );

    /**************************************************
     * let's get started: first allocate a temp array 
     * to fill out all register values before the
     * actual write operation
     **************************************************/
    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numPolicers * (FM10000_POLICER_CFG_4K_WIDTH +
                                           FM10000_POLICER_APPLY_CFG_4K_WIDTH));

        FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                                  &fm10000CachePolicerCfg4k,
                                  numPolicers,
                                  firstIndex,
                                  bank,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);

        dataPtr = data + (numPolicers * FM10000_POLICER_CFG_4K_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                                  &fm10000CachePolicerApplyCfg4k,
                                  numPolicers,
                                  firstIndex,
                                  bank,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);
    }
    else
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numPolicers * (FM10000_POLICER_CFG_512_WIDTH +
                                           FM10000_POLICER_APPLY_CFG_512_WIDTH));

        FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                                  &fm10000CachePolicerCfg512,
                                  numPolicers,
                                  firstIndex,
                                  bank - FM_FM10000_POLICER_BANK_512_1,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);

        dataPtr = data + (numPolicers * FM10000_POLICER_CFG_512_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                                  &fm10000CachePolicerApplyCfg512,
                                  numPolicers,
                                  firstIndex,
                                  bank - FM_FM10000_POLICER_BANK_512_1,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);
    }

    dataPtr = data;

    for (index = 0 ; index < numPolicers ; index++)
    {
        policerCfgEntry = 0;

        /* Validates and set all the fields for each policer configured
         * entries. This apply to both committed and excess rate. */
        FM_API_REQUIRE(committed[index].rateMantissa <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             RateMantissa),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     RateMantissa,
                     committed[index].rateMantissa);

        FM_API_REQUIRE(committed[index].rateExponent <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             RateExponent),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     RateExponent,
                     committed[index].rateExponent);

        FM_API_REQUIRE(committed[index].capacityMantissa <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             CapacityMantissa),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     CapacityMantissa,
                     committed[index].capacityMantissa);

        FM_API_REQUIRE(committed[index].capacityExponent <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             CapacityExponent),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     CapacityExponent,
                     committed[index].capacityExponent);

        FM_ARRAY_SET_FIELD(dataPtr,
                           FM10000_POLICER_CFG_4K,
                           Committed,
                           policerCfgEntry);

        /* Now proceed with the excess rate. */
        policerCfgEntry = 0;

        FM_API_REQUIRE(excess[index].rateMantissa <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             RateMantissa),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     RateMantissa,
                     excess[index].rateMantissa);

        FM_API_REQUIRE(excess[index].rateExponent <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             RateExponent),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     RateExponent,
                     excess[index].rateExponent);

        FM_API_REQUIRE(excess[index].capacityMantissa <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             CapacityMantissa),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     CapacityMantissa,
                     excess[index].capacityMantissa);

        FM_API_REQUIRE(excess[index].capacityExponent <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_CFG_ENTRY,
                                             CapacityExponent),
                       FM_ERR_INVALID_ARGUMENT);
        FM_SET_FIELD(policerCfgEntry,
                     FM10000_POLICER_CFG_ENTRY,
                     CapacityExponent,
                     excess[index].capacityExponent);

        FM_ARRAY_SET_FIELD(dataPtr,
                           FM10000_POLICER_CFG_4K,
                           Excess,
                           policerCfgEntry);

        dataPtr += FM10000_POLICER_CFG_4K_WIDTH;
    }

    /* Now Proceed with the FM10000_POLICER_APPLY_CFG updates */
    for (index = 0 ; index < numPolicers ; index++)
    {
        FM_API_REQUIRE(committed[index].action < FM_FFU_POLICER_ACTION_MAX,
                       FM_ERR_INVALID_ARGUMENT);

        if (committed[index].action == FM_FFU_POLICER_ACTION_DROP)
        {
            FM_ARRAY_SET_BIT(dataPtr,
                             FM10000_POLICER_APPLY_CFG_4K,
                             CommittedAction,
                             1);
        }

        FM_API_REQUIRE(excess[index].action < FM_FFU_POLICER_ACTION_MAX,
                       FM_ERR_INVALID_ARGUMENT);

        if (excess[index].action == FM_FFU_POLICER_ACTION_DROP)
        {
            FM_ARRAY_SET_BIT(dataPtr,
                             FM10000_POLICER_APPLY_CFG_4K,
                             ExcessAction,
                             1);
        }

        dataPtr += FM10000_POLICER_APPLY_CFG_512_WIDTH;
    }

    /* now write to the register and/or their cached values */
    err = fmRegCacheWrite(sw, 2, sgList, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetPolicers */



/*****************************************************************************/
/** fm10000GetPolicers
 * \ingroup lowlevPol10K
 *
 * \desc            Get the values of multiple consecutive policers. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank identifies which policer bank to get (see 
 *                  ''fm_fm10000PolicerBank'').
 *
 * \param[in]       firstIndex is the index of the first policer entry to get
 *                  and ranges from 0 to FM_FM10000_MAX_POLICER_4K_INDEX or
 *                  FM_FM10000_MAX_POLICER_512_INDEX depending on the bank type
 *                  selected.
 *
 * \param[in]       numPolicers is the number of policers to get.           \lb
 *                  firstIndex + numPolicers - 1 must be less than or equal 
 *                  to the max policer index.
 *
 * \param[out]      committed is an array of length numPolicers containing
 *                  the policer state for the committed rate.
 *
 * \param[out]      excess is an array of length numPolicers containing
 *                  the policer state for the excess rate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_COUNTER_BANK if bank is invalid.
 * \return          FM_ERR_INVALID_COUNTER_INDEX if firstIndex is invalid
 *                  or firstIndex + numPolicers - 1 is greater than
 *                  FM_FM10000_MAX_POLICER_INDEX.
 *
 *****************************************************************************/
fm_status fm10000GetPolicers(fm_int                sw,
                             fm_fm10000PolicerBank bank,
                             fm_uint16             firstIndex,
                             fm_uint16             numPolicers,
                             fm_ffuPolicerState *  committed,
                             fm_ffuPolicerState *  excess)
{
    fm_uint16              maxPolicerIndex;
    fm_registerSGListEntry sgList[2];
    fm_uint32             *data;
    fm_uint32             *dataPtr;
    fm_cleanupListEntry *  cleanupList = NULL;
    fm_status              err = FM_OK;
    fm_uint32              index;
    fm_uint32              policerCfgEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, bank = %d, firstIndex = %d, numPolicers = %d\n",
                  sw,
                  bank,
                  firstIndex,
                  numPolicers );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /**************************************************
     * sanity check on the arguments
     **************************************************/

    FM_API_REQUIRE(bank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        maxPolicerIndex = FM_FM10000_MAX_POLICER_4K_INDEX;
    }
    else
    {
        maxPolicerIndex = FM_FM10000_MAX_POLICER_512_INDEX;
    }

    FM_API_REQUIRE(firstIndex + numPolicers <= 
                   maxPolicerIndex + 1,
                   FM_ERR_INVALID_ARGUMENT);

    FM_API_REQUIRE(numPolicers != 0,
                   FM_ERR_INVALID_ARGUMENT );

    /**************************************************
     * let's get started: first allocate a temp array 
     * to fill out all register values before the
     * actual write operation
     **************************************************/
    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numPolicers * (FM10000_POLICER_CFG_4K_WIDTH +
                                           FM10000_POLICER_APPLY_CFG_4K_WIDTH));

        FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                                  &fm10000CachePolicerCfg4k,
                                  numPolicers,
                                  firstIndex,
                                  bank,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);

        dataPtr = data + (numPolicers * FM10000_POLICER_CFG_4K_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                                  &fm10000CachePolicerApplyCfg4k,
                                  numPolicers,
                                  firstIndex,
                                  bank,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);
    }
    else
    {
        FM_ALLOC_TEMP_ARRAY(data, 
                            fm_uint32,
                            numPolicers * (FM10000_POLICER_CFG_512_WIDTH +
                                           FM10000_POLICER_APPLY_CFG_512_WIDTH));

        FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                                  &fm10000CachePolicerCfg512,
                                  numPolicers,
                                  firstIndex,
                                  bank - FM_FM10000_POLICER_BANK_512_1,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  data,
                                  FALSE);

        dataPtr = data + (numPolicers * FM10000_POLICER_CFG_512_WIDTH);

        FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                                  &fm10000CachePolicerApplyCfg512,
                                  numPolicers,
                                  firstIndex,
                                  bank - FM_FM10000_POLICER_BANK_512_1,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);
    }

    err = fmRegCacheRead(sw, 2, sgList, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    dataPtr = data;

    for (index = 0 ; index < numPolicers ; index++)
    {
        /* Process the Committed entry first */
        policerCfgEntry = FM_ARRAY_GET_FIELD(dataPtr,
                                             FM10000_POLICER_CFG_4K,
                                             Committed);

        committed[index].rateMantissa = FM_GET_FIELD(policerCfgEntry,
                                                     FM10000_POLICER_CFG_ENTRY,
                                                     RateMantissa);

        committed[index].rateExponent = FM_GET_FIELD(policerCfgEntry,
                                                     FM10000_POLICER_CFG_ENTRY,
                                                     RateExponent);

        committed[index].capacityMantissa = FM_GET_FIELD(policerCfgEntry,
                                                         FM10000_POLICER_CFG_ENTRY,
                                                         CapacityMantissa);

        committed[index].capacityExponent = FM_GET_FIELD(policerCfgEntry,
                                                         FM10000_POLICER_CFG_ENTRY,
                                                         CapacityExponent);

        /* Process the Excess entry after */
        policerCfgEntry = FM_ARRAY_GET_FIELD(dataPtr,
                                             FM10000_POLICER_CFG_4K,
                                             Excess);

        excess[index].rateMantissa = FM_GET_FIELD(policerCfgEntry,
                                                  FM10000_POLICER_CFG_ENTRY,
                                                  RateMantissa);

        excess[index].rateExponent = FM_GET_FIELD(policerCfgEntry,
                                                  FM10000_POLICER_CFG_ENTRY,
                                                  RateExponent);

        excess[index].capacityMantissa = FM_GET_FIELD(policerCfgEntry,
                                                      FM10000_POLICER_CFG_ENTRY,
                                                      CapacityMantissa);

        excess[index].capacityExponent = FM_GET_FIELD(policerCfgEntry,
                                                      FM10000_POLICER_CFG_ENTRY,
                                                      CapacityExponent);

        dataPtr += FM10000_POLICER_CFG_4K_WIDTH;
    }

    /* Now Read the FM10000_POLICER_APPLY_CFG */
    for (index = 0 ; index < numPolicers ; index++)
    {
        if (FM_ARRAY_GET_BIT(dataPtr,
                             FM10000_POLICER_APPLY_CFG_4K,
                             CommittedAction))
        {
            committed[index].action = FM_FFU_POLICER_ACTION_DROP;
        }
        else
        {
            committed[index].action = FM_FFU_POLICER_ACTION_MARK_DOWN;
        }

        if (FM_ARRAY_GET_BIT(dataPtr,
                             FM10000_POLICER_APPLY_CFG_4K,
                             ExcessAction))
        {
            excess[index].action = FM_FFU_POLICER_ACTION_DROP;
        }
        else
        {
            excess[index].action = FM_FFU_POLICER_ACTION_MARK_DOWN;
        }

        dataPtr += FM10000_POLICER_APPLY_CFG_512_WIDTH;
    }


ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetPolicers */



/*****************************************************************************/
/** fm10000SetPolicerConfig
 * \ingroup lowlevFfu10k
 *
 * \desc            Sets the configuration for a bank of counters/policers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank is the counter bank on which to operate.
 *
 * \param[in]       indexLastPolicer defines the index of the last policer.
 *
 * \param[in]       ingressColorSource determines what defines the color.
 *
 * \param[in]       markDSCP indicates the Differentiated Services
 *                  Code Point should be changed when the color changes.
 *
 * \param[in]       markSwitchPri indicates the switch priority should
 *                  be changed when the color changes.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ARGUMENT if any other parameter is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerConfig(fm_int                sw,
                                  fm_fm10000PolicerBank bank,
                                  fm_uint16             indexLastPolicer,
                                  fm_ffuColorSource     ingressColorSource,
                                  fm_bool               markDSCP,
                                  fm_bool               markSwitchPri,
                                  fm_bool               useCache)
{
    fm_status err = FM_OK;
    fm_uint32 value[FM10000_POLICER_CFG_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "bank = %u, "
                  "indexLastPolicer = %u, "
                  "ingressColorSource = %u, "
                  "markDSCP = %s, "
                  "maskSwitchPri = %s, "
                  "useCache = %s\n",
                  sw,
                  bank,
                  indexLastPolicer,
                  ingressColorSource,
                  FM_BOOLSTRING(markDSCP),
                  FM_BOOLSTRING(markSwitchPri),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /**************************************************
     * sanity check on the arguments
     **************************************************/

    FM_API_REQUIRE(bank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    if (bank <= FM_FM10000_POLICER_BANK_4K_2)
    {
        FM_API_REQUIRE(indexLastPolicer <= FM_FM10000_MAX_POLICER_4K_INDEX,
                       FM_ERR_INVALID_ARGUMENT);
    }
    else
    {
        FM_API_REQUIRE(indexLastPolicer <= FM_FM10000_MAX_POLICER_512_INDEX,
                       FM_ERR_INVALID_ARGUMENT);
    }

    FM_API_REQUIRE(ingressColorSource < FM_COLOR_SOURCE_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    FM_ARRAY_SET_FIELD(value, FM10000_POLICER_CFG, IndexLastPolicer, indexLastPolicer);
    FM_ARRAY_SET_FIELD(value, FM10000_POLICER_CFG, IngressColorSource, ingressColorSource);
    FM_ARRAY_SET_BIT(value, FM10000_POLICER_CFG, MarkDSCP, markDSCP);
    FM_ARRAY_SET_BIT(value, FM10000_POLICER_CFG, MarkSwitchPri, markSwitchPri);

    err = fmRegCacheWriteSingle1D(sw,
                                  &fm10000CachePolicerCfg,
                                  value,
                                  bank,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetPolicerConfig */



/*****************************************************************************/
/** fm10000GetPolicerConfig
 * \ingroup lowlevFfu10k
 *
 * \desc            Gets the configuration for a bank of counters/policers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bank is the counter bank on which to operate.
 *
 * \param[out]      indexLastPolicer defines the index of the last policer.
 *
 * \param[out]      ingressColorSource determines what defines the color.
 *
 * \param[out]      markDSCP indicates the Differentiated Services
 *                  Code Point should be changed when the color changes.
 *
 * \param[out]      markSwitchPri indicates the switch priority should
 *                  be changed when the color changes.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ARGUMENT if any other parameter is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000GetPolicerConfig(fm_int                sw,
                                  fm_fm10000PolicerBank bank,
                                  fm_uint16 *           indexLastPolicer,
                                  fm_ffuColorSource *   ingressColorSource,
                                  fm_bool *             markDSCP,
                                  fm_bool *             markSwitchPri,
                                  fm_bool               useCache)
{
    fm_status err = FM_OK;
    fm_uint32 value[FM10000_POLICER_CFG_WIDTH];

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "bank = %u, "
                  "useCache = %s\n",
                  sw,
                  bank,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /**************************************************
     * sanity check on the arguments
     **************************************************/

    FM_API_REQUIRE(bank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    err = fmRegCacheReadSingle1D(sw,
                                 &fm10000CachePolicerCfg,
                                 value,
                                 bank,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    *indexLastPolicer = FM_ARRAY_GET_FIELD(value, FM10000_POLICER_CFG, IndexLastPolicer);
    *ingressColorSource = FM_ARRAY_GET_FIELD(value, FM10000_POLICER_CFG, IngressColorSource);
    *markDSCP = FM_ARRAY_GET_BIT(value, FM10000_POLICER_CFG, MarkDSCP);
    *markSwitchPri = FM_ARRAY_GET_BIT(value, FM10000_POLICER_CFG, MarkSwitchPri);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetPolicerConfig */



/*****************************************************************************/
/** fm10000SetPolicerDSCPDownMap
 * \ingroup lowlevPol10K
 *
 * \desc            Sets the map used to downgrade DSCP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is an array of 64 bytes, each containing
 *                  a six-bit value.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_DSCP_MAP if the table entries are
 *                  not in the range 0 - 63.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerDSCPDownMap(fm_int         sw,
                                       const fm_byte *table,
                                       fm_bool        useCache)
{
    fm_uint32              value[FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES *
                                 FM10000_POLICER_DSCP_DOWN_MAP_WIDTH];
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              i;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "table = "
                  "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
                  "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
                  "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
                  "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u, "
                  "useCache = %s\n",
                  sw,
                  table[0], table[1], table[2], table[3],
                  table[4], table[5], table[6], table[7],
                  table[8], table[9], table[10], table[11],
                  table[12], table[13], table[14], table[15],
                  table[16], table[17], table[18], table[19],
                  table[20], table[21], table[22], table[23],
                  table[24], table[25], table[26], table[27],
                  table[28], table[29], table[30], table[31],
                  table[32], table[33], table[34], table[35],
                  table[36], table[37], table[38], table[39],
                  table[40], table[41], table[42], table[43],
                  table[44], table[45], table[46], table[47],
                  table[48], table[49], table[50], table[51],
                  table[52], table[53], table[54], table[55],
                  table[56], table[57], table[58], table[59],
                  table[60], table[61], table[62], table[63],
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_CLEAR(value);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CachePolicerDscpDownMap,
                              FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES,
                              0,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              value,
                              FALSE);

    for (i = 0 ; i < FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES ; i++)
    {
        FM_API_REQUIRE(table[i] <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_DSCP_DOWN_MAP,
                                             NewDSCP),
                       FM_ERR_INVALID_DSCP_MAP);
        FM_ARRAY_SET_FIELD(value + (i * FM10000_POLICER_DSCP_DOWN_MAP_WIDTH),
                           FM10000_POLICER_DSCP_DOWN_MAP,
                           NewDSCP,
                           table[i]);
    }

    err = fmRegCacheWrite(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetPolicerDSCPDownMap */



/*****************************************************************************/
/** fm10000GetPolicerDSCPDownMap
 * \ingroup lowlevPol10K
 *
 * \desc            Gets the map used to downgrade DSCP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      table is an array of 64 bytes, each containing
 *                  a six-bit value.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000GetPolicerDSCPDownMap(fm_int   sw,
                                       fm_byte *table,
                                       fm_bool  useCache)
{
    fm_uint32              value[FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES *
                                 FM10000_POLICER_DSCP_DOWN_MAP_WIDTH];
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              i;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, useCache = %s\n",
                  sw,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CachePolicerDscpDownMap,
                              FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES,
                              0,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              value,
                              FALSE);

    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    for (i = 0 ; i < FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES ; i++)
    {
        table[i] = FM_ARRAY_GET_FIELD(value + (i * FM10000_POLICER_DSCP_DOWN_MAP_WIDTH),
                                      FM10000_POLICER_DSCP_DOWN_MAP,
                                      NewDSCP);
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetPolicerDSCPDownMap */



/*****************************************************************************/
/** fm10000SetPolicerSwPriDownMap
 * \ingroup lowlevPol10K
 *
 * \desc            Sets the map used to downgrade switch priority.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is an array of 16 bytes, each containing
 *                  a four-bit value.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SWPRI_MAP if the table entries are
 *                  not in the range 0 - 15.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerSwPriDownMap(fm_int         sw,
                                        const fm_byte *table,
                                        fm_bool        useCache)
{
    fm_uint32              value[FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES *
                                 FM10000_POLICER_SWPRI_DOWN_MAP_WIDTH];
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              i;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "table = "
                  "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u, "
                  "useCache = %s\n",
                  sw,
                  table[0], table[1], table[2], table[3],
                  table[4], table[5], table[6], table[7],
                  table[8], table[9], table[10], table[11],
                  table[12], table[13], table[14], table[15],
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_CLEAR(value);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CachePolicerSwpriDownMap,
                              FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES,
                              0,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              value,
                              FALSE);

    for (i = 0 ; i < FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES ; i++)
    {
        FM_API_REQUIRE(table[i] <=
                       FM_FIELD_UNSIGNED_MAX(FM10000_POLICER_SWPRI_DOWN_MAP,
                                             NewSwitchPriority),
                       FM_ERR_INVALID_SWPRI_MAP);
        FM_ARRAY_SET_FIELD(value + (i * FM10000_POLICER_SWPRI_DOWN_MAP_WIDTH),
                           FM10000_POLICER_SWPRI_DOWN_MAP,
                           NewSwitchPriority,
                           table[i]);
    }

    err = fmRegCacheWrite(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetPolicerSwPriDownMap */



/*****************************************************************************/
/** fm10000GetPolicerSwPriDownMap
 * \ingroup lowlevPol10K
 *
 * \desc            Gets the map used to downgrade switch priority.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      table is an array of 16 bytes, each containing
 *                  a four-bit value.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000GetPolicerSwPriDownMap(fm_int   sw,
                                        fm_byte *table,
                                        fm_bool  useCache)
{
    fm_uint32              value[FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES *
                                 FM10000_POLICER_SWPRI_DOWN_MAP_WIDTH];
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              i;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, useCache = %s\n",
                  sw,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CachePolicerSwpriDownMap,
                              FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES,
                              0,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              value,
                              FALSE);

    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    for (i = 0 ; i < FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES ; i++)
    {
        table[i] = FM_ARRAY_GET_FIELD(value + (i * FM10000_POLICER_SWPRI_DOWN_MAP_WIDTH),
                                      FM10000_POLICER_SWPRI_DOWN_MAP,
                                      NewSwitchPriority);
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetPolicerSwPriDownMap */



/*****************************************************************************/
/** fm10000SetPolicerOwnership
 * \ingroup lowlevPol10K
 *
 * \desc            Sets which software component (as defined by the owner)
 *                  has control of the given policer bank.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       owner represents a valid software component.
 *
 * \param[in]       policerBank is the resource to mark ownership on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_FFU_RES_OWNED if this resource is already owned.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerOwnership(fm_int                sw,
                                     fm_ffuOwnerType       owner,
                                     fm_fm10000PolicerBank policerBank)
{
    fm10000_switch *switchExt = NULL;
    fm_fm10000PolOwnershipInfo *info = NULL;
    fm_status       err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, owner = %d, policerBank = %d\n",
                  sw,
                  owner,
                  policerBank );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(owner < FM_FFU_OWNER_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    FM_API_REQUIRE(policerBank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    switchExt = GET_SWITCH_EXT(sw);
    info      = &switchExt->polOwnershipInfo;

    if ( (info->bankOwner[policerBank] != FM_FFU_OWNER_NONE) &&
        (info->bankOwner[policerBank] != owner) && 
        ( owner != FM_FFU_OWNER_NONE) )
    {
        err = FM_ERR_FFU_RES_OWNED;
        goto ABORT;
    }

    info->bankOwner[policerBank] = owner;

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetPolicerOwnership */



/*****************************************************************************/
/** fm10000GetPolicerOwnership 
 * \ingroup lowlevPol10K
 *
 * \desc            Gets which software component has control of the given
 *                  policer bank.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      owner points to caller allocated storage where the owner
 *                  of the given resource is written to.
 *
 * \param[in]       policerBank is the resource to check ownership for.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetPolicerOwnership(fm_int                sw,
                                     fm_ffuOwnerType *     owner,
                                     fm_fm10000PolicerBank policerBank)
{
    fm10000_switch *switchExt = NULL;
    fm_fm10000PolOwnershipInfo *info = NULL;
    fm_status       err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, policerBank = %d\n",
                  sw,
                  policerBank );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsPolicer(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(owner, FM_ERR_INVALID_ARGUMENT);

    FM_API_REQUIRE(policerBank < FM_FM10000_POLICER_BANK_MAX,
                   FM_ERR_INVALID_ARGUMENT);

    switchExt = GET_SWITCH_EXT(sw);
    info      = &switchExt->polOwnershipInfo;

    *owner = info->bankOwner[policerBank];

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetPolicerOwnership */

