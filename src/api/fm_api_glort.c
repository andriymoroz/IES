/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:           fm_api_glort.c
 * Creation Date:  Jul 8, 2015
 * Description:    Global Resource Tag (GloRT) management functions.
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

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


#define MAX_BUF_SIZE 256

/* Macros to manipulate the glortState array to check whether a GloRT
 * is reserved or in use. */

#define FM_RELEASE_GLORT(info, glort)       \
        (info)->glortState[glort] = FM_GLORT_STATE_UNUSED

#define FM_SET_GLORT_FREE(info, glort)      \
        (info)->glortState[glort] &= ~FM_GLORT_STATE_USED_BITS

#define FM_SET_GLORT_IN_USE(info, glort)    \
        (info)->glortState[glort] |= FM_GLORT_STATE_IN_USE

#define FM_RESERVE_GLORT_MCG(info, glort)   \
        (info)->glortState[glort] |= FM_GLORT_STATE_RESV_MCG

#define FM_RESERVE_GLORT_LAG(info, glort)   \
        (info)->glortState[glort] |= FM_GLORT_STATE_RESV_LAG

#define FM_RESERVE_GLORT_LBG(info, glort)   \
        (info)->glortState[glort] |= FM_GLORT_STATE_RESV_LBG

/* Either used or reserved */
#define FM_IS_GLORT_TAKEN(info, glort)      \
        ((info)->glortState[glort] != FM_GLORT_STATE_UNUSED)

/* Free regardless of reservation */
#define FM_IS_GLORT_FREE(info, glort)       \
        (((info)->glortState[glort] & FM_GLORT_STATE_USED_BITS) == 0)

/* Check for both free and reserved */
#define FM_IS_GLORT_MCG_FREE(info, glort)   \
        ((info)->glortState[glort] == FM_GLORT_STATE_RESV_MCG)

#define FM_IS_GLORT_LAG_FREE(info, glort)   \
        ((info)->glortState[glort] == FM_GLORT_STATE_RESV_LAG)

#define FM_IS_GLORT_LBG_FREE(info, glort)   \
        ((info)->glortState[glort] == FM_GLORT_STATE_RESV_LBG)

/* Check for GloRT reservation */
#define FM_IS_GLORT_MCG_RESERVED(info, glort)   \
        (((info)->glortState[glort] & FM_GLORT_STATE_RESV_MCG) != 0)

#define FM_IS_GLORT_LAG_RESERVED(info, glort)   \
        (((info)->glortState[glort] & FM_GLORT_STATE_RESV_LAG) != 0)

#define FM_IS_GLORT_LBG_RESERVED(info, glort)   \
        (((info)->glortState[glort] & FM_GLORT_STATE_RESV_LBG) != 0)

/* Due to the problem of another thread is freeing the LAG, thus preventing
 * the code to free the allocated LAGs right after deleting all the LAGs
 * in the allocated groups, so we will need this so we can free the allocated
 * groups */
#define FM_SET_GLORT_FREE_PEND(info, glort) \
        (info)->glortState[glort] |= FM_GLORT_STATE_FREE_PEND

#define FM_IS_GLORT_FREE_PEND(info, glort)  \
        (((info)->glortState[glort] & FM_GLORT_STATE_FREE_PEND) != 0)

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/


static fm_status GetGlortRange(fm_switch *  switchPtr,
                               fm_glortType type,
                               fm_uint32 *  rangeBase,
                               fm_int *     rangeCount);
static inline fm_bool IsGlortInRange(fm_uint32 glort,
                                     fm_uint32 rangeBase,
                                     fm_int    rangeCount);
static inline fm_bool IsGlortCorrect(fm_uint32 glort);
static inline fm_bool IsGlortType(fm_switch *  switchPtr,
                                  fm_uint32    glort,
                                  fm_glortType type);
static fm_status GetGlortType(fm_switch *   switchPtr,
                              fm_uint32     glort,
                              fm_glortType *glortType);
static fm_bool IsGlortReservedForType(fm_switch *  switchPtr,
                                      fm_uint32    glort,
                                      fm_glortType type);
static fm_status RequestGlort(fm_switch *  switchPtr,
                              fm_uint32    glort,
                              fm_glortType type);
static fm_status ReleaseGlort(fm_switch *  switchPtr,
                              fm_uint32    glort,
                              fm_glortType type,
                              fm_bool      pending);
static fm_status ReserveGlort(fm_switch *  switchPtr,
                              fm_uint32    glort,
                              fm_glortType type);
static fm_status UnreserveGlort(fm_switch *  switchPtr,
                                fm_uint32    glort,
                                fm_glortType type);
static fm_status GlortTypeToText(fm_glortType glortType,
                                 rsize_t      bufSize,
                                 fm_text      name);
static fm_status GlortStateToText(fm_int  state,
                                  size_t  bufSize,
                                  fm_text name);


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
/** GetGlortRange
 * \ingroup intPort
 *
 * \desc            Returns the GloRT range of the given GloRT type.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       type identifies the GloRT range owner.
 *
 * \param[out]      rangeBase points to caller supplied storage where the
 *                  first GloRT in the range is placed.
 *
 * \param[out]      rangeCount points to caller supplied storage where the
 *                  size of the GloRT block is placed.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if both of output pointers are NULL or
 *                  the switchPtr is NULL
 *
 *****************************************************************************/
static fm_status GetGlortRange(fm_switch *  switchPtr,
                               fm_glortType type,
                               fm_uint32 *  rangeBase,
                               fm_int *     rangeCount)
{
    fm_mailboxInfo *mailboxInfo;
    fm_glortRange  *range;
    fm_glortInfo   *glortInfo;
    fm_uint32       base;
    fm_uint16       numberOfPeps;
    fm_uint32       firstGlort;
    fm_int          numGlort;

#if FM_SUPPORT_SWAG

    fmSWAG_switch  *aggregatePtr;
    fm_swagMember  *member;
    fm_int          swagMembersCount;

#endif

    if (switchPtr == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    mailboxInfo  = &switchPtr->mailboxInfo;
    range        = &switchPtr->glortRange;
    glortInfo    = &switchPtr->glortInfo;
    base         = range->glortBase;
    numberOfPeps = 0;

    if ( (rangeBase == NULL) && (rangeCount == NULL) )
    {
        return FM_ERR_UNINITIALIZED;
    }

    switch (type)
    {
        case FM_GLORT_TYPE_LAG:
            firstGlort = range->lagBaseGlort;
            numGlort   = range->lagCount;
            break;
        case FM_GLORT_TYPE_MULTICAST:
            firstGlort = range->mcastBaseGlort;
            numGlort   = range->mcastCount;
            break;
        case FM_GLORT_TYPE_LBG:
            firstGlort = range->lbgBaseGlort;
            numGlort   = range->lbgCount;
            break;
        case FM_GLORT_TYPE_PORT:
            firstGlort = range->portBaseGlort;
            numGlort   = range->portCount;
            break;
        case FM_GLORT_TYPE_SPECIAL:
            firstGlort = glortInfo->specialBase;
            numGlort   = glortInfo->specialSize;
            break;
        case FM_GLORT_TYPE_CPU:
            firstGlort = glortInfo->cpuBase;
            numGlort   = (glortInfo->cpuBase | range->glortMask)
                          - glortInfo->cpuBase + 1;
            break;
        case FM_GLORT_TYPE_PEP:
            firstGlort   = mailboxInfo->glortBase;
            numberOfPeps = FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS();

#if FM_SUPPORT_SWAG

            if (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG)
            {
                swagMembersCount = 0;
                aggregatePtr = switchPtr->extension;
                member = fmGetFirstSwitchInSWAG(aggregatePtr);

                while (member != NULL)
                {
                    swagMembersCount++;
                    member = fmGetNextSwitchInSWAG(member);
                }
                
                numberOfPeps *= swagMembersCount;
            }

#endif

            numGlort = numberOfPeps * mailboxInfo->glortsPerPep;
            break;
        default:
            firstGlort = 0;
            numGlort   = FM_MAX_GLORT;

    }

    if (rangeBase != NULL)
    {
        *rangeBase = firstGlort;
    }

    if (rangeCount != NULL)
    {
        *rangeCount = numGlort;
    }

    return FM_OK;

}   /* end GetGlortRange */




/*****************************************************************************/
/** IsGlortInRange
 * \ingroup intPort
 *
 * \desc            Check if the given GloRT is within the range.
 *
 * \param[in]       glort is the GloRT which should be checked.
 *
 * \param[in]       rangeBase is the first GloRT in the range.
 *
 * \param[in]       rangeCount is the size of the GloRT range.
 *
 * \return          TRUE if GloRT is in the specified GloRT range.
 *                  FALSE if GloRT is not in the specified GloRT range.
 *
 *****************************************************************************/
static inline fm_bool IsGlortInRange(fm_uint32 glort,
                                     fm_uint32 rangeBase,
                                     fm_int    rangeCount)
{
    return ( (glort >= rangeBase) && (glort < rangeBase + rangeCount) );

}   /* end IsGlortInRange */



/*****************************************************************************/
/** IsGlortCorrect
 * \ingroup intPort
 *
 * \desc            Check if the given GloRT is within the general GloRT range
 *                  (0 - FM_MAX_GLORT).
 *
 * \param[in]       glort is the GloRT which should be checked.
 *
 * \return          TRUE if GloRT is in the specified GloRT range.
 *                  FALSE if GloRT is not in the specified GloRT range.
 *
 *****************************************************************************/
static inline fm_bool IsGlortCorrect(fm_uint32 glort)
{
    return (glort <= FM_MAX_GLORT);

}   /* end IsGlortCorrect */




/*****************************************************************************/
/** IsGlortType
 * \ingroup intPort
 *
 * \desc            Check if the given GloRT is within the type range.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT which should be checked.
 *
 * \param[in]       type identifies the GloRT range owner.
 *
 * \return          TRUE if GloRT is in the specified GloRT range.
 *                  FALSE if GloRT is not in the specified GloRT range or if the
 *                  pointer is NULL.
 *
 *****************************************************************************/
static inline fm_bool IsGlortType(fm_switch *  switchPtr,
                                  fm_uint32    glort,
                                  fm_glortType type)
{
    fm_uint32        rangeBase;
    fm_int           rangeCount;

    rangeBase  = 0;
    rangeCount = -1;

    GetGlortRange(switchPtr, type, &rangeBase, &rangeCount);
    if ( IsGlortInRange(glort, rangeBase, rangeCount) )
    {
        return TRUE;
    }

    return FALSE;

}   /* end IsGlortType */




/*****************************************************************************/
/** GetGlortType
 * \ingroup intPort
 *
 * \desc            Returns the GloRT range of the given GloRT type.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT which should be checked.
 *
 * \param[out]      glortType points to caller supplied storage where the
 *                  type of the GloRT is placed. If type is NULL then the return
 *                  value is the only if the GloRT type was found.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL
 * \return          FM_ERR_NOT_FOUND if the type was not recognized
 *                  (FM_GLORT_TYPE_UNSPECIFIED is placed to type pointer)
 *
 *****************************************************************************/
static fm_status GetGlortType(fm_switch *   switchPtr,
                              fm_uint32     glort,
                              fm_glortType *glortType)
{
    fm_status err;
    fm_glortType type;

    err  = FM_OK;
    type = FM_GLORT_TYPE_UNSPECIFIED;

    if (switchPtr == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    /***************************************************
     * The for loop cannot be used and
     * the order of GloRT types is not insignificant,
     * because some ranges belong to other ones:
     * (a) FM_GLORT_TYPE_SPECIAL is part of FM_GLORT_TYPE_CPU
     * (b) All ranges can be treated as subset of
     *     FM_GLORT_TYPE_UNSPECIFIED (default)
     **************************************************/

    if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_LAG) )
    {
        type = FM_GLORT_TYPE_LAG;
    }
    else if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_MULTICAST) )
    {
        type = FM_GLORT_TYPE_MULTICAST;
    }
    else if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_LBG) )
    {
        type = FM_GLORT_TYPE_LBG;
    }
    else if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_PORT) )
    {
        type = FM_GLORT_TYPE_PORT;
    }
    else if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_SPECIAL) )
    {
        type = FM_GLORT_TYPE_SPECIAL;
    }
    else if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_CPU) )
    {
        type = FM_GLORT_TYPE_CPU;
    }
    else if ( IsGlortType(switchPtr, glort, FM_GLORT_TYPE_PEP) )
    {
        type = FM_GLORT_TYPE_PEP;
    }
    else
    {
        type = FM_GLORT_TYPE_UNSPECIFIED;
        err = FM_ERR_NOT_FOUND;
    }

    if (glortType != NULL)
    {
        *glortType = type;
    }


    return err;

}   /* end GetGlortType */




/*****************************************************************************/
/** IsGlortReservedForType
 * \ingroup intPort
 *
 * \desc            Check if the given GloRT is within the reserved range.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT which should be checked.
 *
 * \param[in]       type identifies the GloRT range owner.
 *
 * \return          TRUE if GloRT is in the specified GloRT range.
 *                  FALSE if GloRT is not in the specified GloRT range, if the
 *                  GloRT is invalid or if the pointer is NULL.
 *
 *****************************************************************************/
static fm_bool IsGlortReservedForType(fm_switch *  switchPtr,
                                      fm_uint32    glort,
                                      fm_glortType type)
{
    fm_logicalPortInfo* lportInfo;
    fm_mailboxInfo *    mailboxInfo;
    fm_bool             reserved;

    lportInfo   = NULL;
    mailboxInfo = NULL;
    reserved    = FALSE;

    if (!IsGlortCorrect(glort))
    {
        return FALSE;
    }

    if (switchPtr != NULL)
    {
        lportInfo   = &switchPtr->logicalPortInfo;
        mailboxInfo = &switchPtr->mailboxInfo;

        switch (type)
        {
            case FM_GLORT_TYPE_LAG:
                reserved = FM_IS_GLORT_LAG_RESERVED(lportInfo, glort);
                break;
            case FM_GLORT_TYPE_MULTICAST:
                reserved = FM_IS_GLORT_MCG_RESERVED(lportInfo, glort);
                break;
            case FM_GLORT_TYPE_LBG:
                reserved = FM_IS_GLORT_LBG_RESERVED(lportInfo, glort);
                break;
            default:
                reserved = FALSE;
                break;
       }
    }

    return reserved;
}   /* end IsGlortReservedForType */




/*****************************************************************************/
/** RequestGlort
 * \ingroup intPort
 *
 * \desc            Allocates the single GloRT. The GloRT should be released
 *                  using the ReleaseGlort function.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT to allocate.
 *
 * \param[in]       type identifies the potential GloRT range owner.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL
 * \return          FM_ERR_INVALID_ARGUMENT if the GloRT is invalid
 * \return          FM_ERR_GLORT_IN_USE if the GloRT is already used
 *                  or not destined to be allocated by the caller.
 *
 *****************************************************************************/
static fm_status RequestGlort(fm_switch *  switchPtr,
                              fm_uint32    glort,
                              fm_glortType type)
{
    fm_logicalPortInfo* lportInfo;

    if (switchPtr != NULL)
    {
        lportInfo = &switchPtr->logicalPortInfo;
    }
    else
    {
        return FM_ERR_UNINITIALIZED;
    }

    /***************************************************
     * Three conditions must be met:
     *   (a) The GloRT is in range 0 - FM_MAX_GLORT
     *   (b) The GloRT is unused
     *   (c) The GloRT is correctly reserved or in
     *       the proper range
     **************************************************/

    if (!IsGlortCorrect(glort))
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( !FM_IS_GLORT_FREE(lportInfo, glort) ||
         ( !IsGlortType(switchPtr, glort, type) &&
           !IsGlortReservedForType(switchPtr, glort, type) ) )
    {
        return FM_ERR_GLORT_IN_USE;
    }

    FM_SET_GLORT_IN_USE(lportInfo, glort);

    return FM_OK;

}   /* end RequestGlort */




/*****************************************************************************/
/** ReleaseGlort
 * \ingroup intPort
 *
 * \desc            Releases the GloRT range. The range should be allocated
 *                  using the fmRequestGlortRange function.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT to release.
 *
 * \param[in]       type identifies the potential GloRT range owner.
 *
 * \param[in]       pending indicates whether the GloRT should be released or
 *                  just marked as "free pending".
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL
 * \return          FM_ERR_INVALID_ARGUMENT if the GloRT or its type is invalid
 * \return          FM_ERR_NO_GLORTS_ALLOCATED if the GloRT was not allocated
 *                  before (or was already released).
 *
 *****************************************************************************/
static fm_status ReleaseGlort(fm_switch *  switchPtr,
                              fm_uint32    glort,
                              fm_glortType type,
                              fm_bool      pending)
{
    fm_logicalPortInfo* lportInfo;

    if (switchPtr == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    lportInfo = &switchPtr->logicalPortInfo;

    /***************************************************
     * Three conditions must be met:
     *   (a) The GloRT is in range 0 - FM_MAX_GLORT
     *   (b) The GloRT is unused
     *   (c) The GloRT is correctly reserved or in
     *       the proper range
     **************************************************/

    if ( !IsGlortCorrect(glort) ||
         ( !IsGlortType(switchPtr, glort, type) &&
           !IsGlortReservedForType(switchPtr, glort, type) ) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( FM_IS_GLORT_FREE(lportInfo, glort) ||
         (pending && FM_IS_GLORT_FREE_PEND(lportInfo, glort)) )
    {
        return FM_ERR_NO_GLORTS_ALLOCATED;
    }

    if (!pending)
    {
        FM_SET_GLORT_FREE(lportInfo, glort);
    }
    else
    {
        FM_SET_GLORT_FREE_PEND(lportInfo, glort);
    }

    return FM_OK;
}




/*****************************************************************************/
/** ReserveGlort
 * \ingroup intPort
 *
 * \desc            Reserves the single GloRT. The GloRT should be released
 *                  using the UnreserveGlort function.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT to reserve.
 *
 * \param[in]       type identifies the potential GloRT range owner
 *                  (only ''FM_GLORT_TYPE_LAG'', ''FM_GLORT_TYPE_MULTICAST'' and
 *                  ''FM_GLORT_TYPE_LBG'' are supported).
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL
 * \return          FM_ERR_INVALID_ARGUMENT if the GloRT is invalid
 * \return          FM_ERR_GLORT_IN_USE if the GloRT is already used
 *                  or not destined to be allocated by the caller
 * \return          FM_ERR_UNSUPPORTED if it is not possible to reserve
 *                  for the given type
 *
 *****************************************************************************/
static fm_status ReserveGlort(fm_switch *  switchPtr,
                              fm_uint32    glort,
                              fm_glortType type)
{
    fm_logicalPortInfo* lportInfo;

    if (switchPtr == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    lportInfo = &switchPtr->logicalPortInfo;

    /***************************************************
     * Two conditions must be met:
     *   (a) The GloRT is in range 0 - FM_MAX_GLORT
     *   (b) The GloRT is unused and not reserved
     **************************************************/

    if (!IsGlortCorrect(glort))
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (FM_IS_GLORT_TAKEN(lportInfo, glort))
    {
        return FM_ERR_GLORT_IN_USE;
    }

    switch (type)
    {
        case FM_GLORT_TYPE_LAG:
            FM_RESERVE_GLORT_LAG(lportInfo, glort);
            break;
        case FM_GLORT_TYPE_MULTICAST:
            FM_RESERVE_GLORT_MCG(lportInfo, glort);
            break;
        case FM_GLORT_TYPE_LBG:
            FM_RESERVE_GLORT_LBG(lportInfo, glort);
            break;
        default:
            return FM_ERR_UNSUPPORTED;
   }

    return FM_OK;

} /* end ReserveGlort */




/*****************************************************************************/
/** UnreserveGlort
 * \ingroup intPort
 *
 * \desc            Releases the GloRT range. The range should be allocated
 *                  using the fmRequestGlortRange function.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       glort is the GloRT to release.
 *
 * \param[in]       type identifies the GloRT range owner.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL
 * \return          FM_ERR_INVALID_ARGUMENT if the GloRT is invalid
 * \return          FM_ERR_GLORT_IN_USE if any of GloRTs is still used
 * \return          FM_ERR_NO_GLORTS_ALLOCATED if any of GloRTs was not reserved
 *                  before or was reserved by someone else.
 *
 *****************************************************************************/
static fm_status UnreserveGlort(fm_switch *  switchPtr,
                                fm_uint32    glort,
                                fm_glortType type)
{
    fm_logicalPortInfo* lportInfo;

    if (switchPtr == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    lportInfo = &switchPtr->logicalPortInfo;

    /***************************************************
     * Three conditions must be met:
     *   (a) The GloRT is in range 0 - FM_MAX_GLORT
     *   (b) The GloRT is unused
     *   (c) The GloRT is correctly reserved
     **************************************************/

    if (!IsGlortCorrect(glort))
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( !FM_IS_GLORT_FREE(lportInfo, glort) &&
         !FM_IS_GLORT_FREE_PEND(lportInfo, glort) )
    {
        return FM_ERR_GLORT_IN_USE;
    }

    if ( !IsGlortReservedForType(switchPtr, glort, type) )
    {
        return FM_ERR_NO_GLORTS_ALLOCATED;
    }

    /* the GloRT is unused anyway, so we can just set the state to 0 */
    FM_RELEASE_GLORT(lportInfo, glort);

    return FM_OK;

}   /* end UnreserveGlort */




/*****************************************************************************/
/** GlortTypeToText
 * \ingroup intPort
 *
 *
 * \desc            Returns the text representation of a GloRT type.
 *
 * \param[in]       glortType is the GloRT type.
 *
 * \param[in]       bufSize is the size of the name buffer.
 *
 * \param[out]      name points to caller supplied storage where the
 *                  GloRT type name is placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL.
 *
 *****************************************************************************/
static fm_status GlortTypeToText(fm_glortType glortType,
                                 rsize_t      bufSize,
                                 fm_text      name)
{

    fm_text        mcastText;
    fm_text        lagText;
    fm_text        lbgText;
    fm_text        portText;
    fm_text        mailboxText;
    fm_text        cpuText;
    fm_text        specialText;
    fm_text        unspecifiedText;

    mcastText       = "Multicast GloRTs";
    lagText         = "LAG GloRTs";
    lbgText         = "LBG GloRTs";
    portText        = "Port GloRTs";
    mailboxText     = "PEP GloRTs";
    cpuText         = "CPU GloRTs";
    specialText     = "Special GloRTs";
    unspecifiedText = "Unspecified GloRTs";

    if (name == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    switch (glortType)
    {
        case FM_GLORT_TYPE_LAG:
            FM_STRCPY_S(name, bufSize, lagText);
            break;
        case FM_GLORT_TYPE_MULTICAST:
            FM_STRCPY_S(name, bufSize, mcastText);
            break;
        case FM_GLORT_TYPE_LBG:
            FM_STRCPY_S(name, bufSize, lbgText);
            break;
        case FM_GLORT_TYPE_PORT:
            FM_STRCPY_S(name, bufSize, portText);
            break;
        case FM_GLORT_TYPE_SPECIAL:
            FM_STRCPY_S(name, bufSize, specialText);
            break;
        case FM_GLORT_TYPE_CPU:
            FM_STRCPY_S(name, bufSize, cpuText);
            break;
        case FM_GLORT_TYPE_PEP:
            FM_STRCPY_S(name, bufSize, mailboxText);
            break;
        default:
            FM_STRCPY_S(name, bufSize, unspecifiedText);
    }

    return FM_OK;

} /* end GlortTypeToText */





/*****************************************************************************/
/** GlortStateToText
 * \ingroup intPort
 *
 *
 * \desc            Returns the text representation of a GloRT state
 *
 * \param[in]       state is the GloRT state.
 *
 * \param[in]       bufSize is the size of name buffer.
 *
 * \param[out]      name points to caller supplied storage where the
 *                  GloRT state name is placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if a pointer is NULL.
 *
 *****************************************************************************/
static fm_status GlortStateToText(fm_int  state,
                                  size_t  bufSize,
                                  fm_text name)
{
    fm_text free;
    fm_text inUse;
    fm_text resvMCG;
    fm_text resvLAG;
    fm_text resvLBG;
    fm_text pending;

    free      = "Free, ",
    inUse     = "In use, ";
    resvMCG   = "Reserved for MCG, ";
    resvLAG   = "Reserved for LAG, ";
    resvLBG   = "Reserved for LBG, ";
    pending   = "Free pending, ";

    if (name == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    FM_STRCPY_S(name, bufSize, "");

    if (state & FM_GLORT_STATE_IN_USE)
    {
        FM_STRCAT_S(name, bufSize, inUse);
    }
    else
    {
        FM_STRCAT_S(name, bufSize, free);
    }

    if (state & FM_GLORT_STATE_RESV_MCG)
    {
        FM_STRCAT_S(name, bufSize, resvMCG);
    }
    if (state & FM_GLORT_STATE_RESV_LAG)
    {
        FM_STRCAT_S(name, bufSize, resvLAG);
    }
    if (state & FM_GLORT_STATE_RESV_LBG)
    {
        FM_STRCAT_S(name, bufSize, resvLBG);
    }
    if (state & FM_GLORT_STATE_FREE_PEND)
    {
        FM_STRCAT_S(name, bufSize, pending);
    }

    /* Remove ", " from the end */
    name[strlen(name) - 2] = '\0';

    return FM_OK;

}   /* end GlortStateToText */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fmVerifyGlortRange
 * \ingroup intPort
 *
 * \desc            Verifies that GloRT range is valid.
 *
 * \param[in]       glort is the start GloRT.
 *
 * \param[in]       size is the size of the range.
 *
 * \return          FM_OK if range is okay.
 * \return          FM_FAIL if out of physical range.
 *
 *****************************************************************************/
fm_status fmVerifyGlortRange(fm_uint32 glort, fm_int size)
{

    if ( (size <= 0) || (size > FM_MAX_GLORT + 1) )
    {
        return FM_FAIL;
    }

    if (glort > FM_MAX_GLORT)
    {
        return FM_FAIL;
    }

    if ( (glort + size - 1) > FM_MAX_GLORT )
    {
        return FM_FAIL;
    }

    return FM_OK;

}   /* end fmVerifyGlortRange */




/*****************************************************************************/
/** fmCheckGlortRangeStateInt
 * \ingroup intPort
 *
 * \desc            Compares the state of all GloRTs within the range. If
 *                  the state is exactly the same as the expected one,
 *                  then FM_OK is returned. The read switch lock should be taken
 *                  by a function caller.
 *                                                                      \lb\lb
 *                  See also fmCheckGlortRangeState.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       start is the first GloRT to check.
 *
 * \param[in]       numGlorts is the number of GloRTs in the block.
 *
 * \param[in]       state is an expected GloRT state (examples:
 *                  FM_GLORT_STATE_IN_USE, FM_GLORT_STATE_RESV_MCG etc).
 *
 * \param[in]       mask is a GloRT state bit mask.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_INVALID_STATE if any GloRT has an unexpected state
 *
 *****************************************************************************/
fm_status fmCheckGlortRangeStateInt(fm_switch *switchPtr,
                                    fm_uint32  start,
                                    fm_int     numGlorts,
                                    fm_int     state,
                                    fm_int     mask)
{
    fm_status err;
    fm_uint32 glort;
    fm_uint32 rangeEnd;

    err      = FM_OK;
    glort    = 0;
    rangeEnd = start + numGlorts - 1;

    err = fmVerifyGlortRange(start, numGlorts);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
    }

    for ( glort = start ; glort <= rangeEnd ; ++glort )
    {
        if (!IsGlortCorrect(glort))
        {
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
        }

        if ( (switchPtr->logicalPortInfo.glortState[glort] & mask) !=
             (state & mask) )
        {
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_STATE);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_OK);

}   /* end fmCheckGlortRangeStateInt */




/*****************************************************************************/
/** fmCheckGlortRangeState
 * \ingroup intPort
 *
 * \desc            Compares the state of all GloRTs within the range. If
 *                  the state is exactly the same as the expected one,
 *                  then FM_OK is returned. The read switch lock should be taken
 *                  by a function caller.
 *                                                                      \lb\lb
 *                  See also fmCheckGlortRangeStateInt.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       start is the first GloRT to check.
 *
 * \param[in]       numGlorts is the number of GloRTs in the block.
 *
 * \param[in]       state is an expected GloRT state (examples:
 *                  FM_GLORT_STATE_IN_USE, FM_GLORT_STATE_RESV_MCG etc).
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_INVALID_STATE if any GloRT has an unexpected state
 *
 *****************************************************************************/
inline fm_status fmCheckGlortRangeState(fm_switch *switchPtr,
                                        fm_uint32  start,
                                        fm_int     numGlorts,
                                        fm_int     state)
{
    /* compare all bits by default */
    return fmCheckGlortRangeStateInt(switchPtr,
                                     start,
                                     numGlorts,
                                     state,
                                     ~0);

}   /* end fmCheckGlortRangeState */




/*****************************************************************************/
/** fmCheckGlortRangeType
 * \ingroup intPort
 *
 * \desc            Compares the type of all GloRTs within the range. If
 *                  the type is the same as the expected one, then FM_OK is
 *                  returned. Reserved GloRTs are accepted as well. Note, that
 *                  some ranges are subsets of other ranges (e.g.
 *                  FM_GLORT_TYPE_SPECIAL is part of FM_GLORT_TYPE_CPU).
 *                  The read switch lock should be taken by a function
 *                  caller.
 *
 * \param[in]       switchPtr points to the switch state structure.
 *
 * \param[in]       start is the first GloRT to check.
 *
 * \param[in]       numGlorts is the number of GloRTs in the block.
 *
 * \param[in]       glortType is an expected GloRT type.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_FAIL if any GloRT has an unexpected type
 *
 *****************************************************************************/
fm_status fmCheckGlortRangeType(fm_switch *  switchPtr,
                                fm_uint32    start,
                                fm_int       numGlorts,
                                fm_glortType glortType)
{
    fm_status err;
    fm_uint32 glort;
    fm_uint32 rangeEnd;

    err      = FM_OK;
    glort    = 0;
    rangeEnd = start + numGlorts - 1;

    err = fmVerifyGlortRange(start, numGlorts);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
    }

    for ( glort = start ; glort <= rangeEnd ; ++glort )
    {
        if (!IsGlortCorrect(glort))
        {
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
        }

        if ( !IsGlortType(switchPtr, glort, glortType) &&
             !IsGlortReservedForType(switchPtr, glort, glortType) )
        {
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_FAIL);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_OK);
}   /* end fmCheckGlortRangeType */




/*****************************************************************************/
/** fmRequestGlortRange
 * \ingroup intPort
 *
 * \desc            Allocates the GloRT range. The range should be released
 *                  using the fmReleaseGlortRange function.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       start is the first GloRT to allocate.
 *
 * \param[in]       numGlorts is the size of the block to allocate.
 *
 * \param[in]       glortType identifies the potential GloRT range owner.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the range is too small
 * \return          FM_ERR_GLORT_IN_USE if any of GloRTs is already used
 *                  or not destined to be allocated by the caller.
 *
 *****************************************************************************/
fm_status fmRequestGlortRange(fm_int       sw,
                              fm_uint32    start,
                              fm_int       numGlorts,
                              fm_glortType glortType)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_mailboxInfo *    mailboxInfo;
    fm_logicalPortInfo* lportInfo;
    fm_glortRange *     range;
    fm_uint32           glort;
    fm_uint32           last;
    fm_uint32           rangeBase;
    fm_uint32           rangeMax;
    fm_int              rangeCount;


    VALIDATE_AND_PROTECT_SWITCH(sw);


    err         = FM_OK;
    switchPtr   = GET_SWITCH_PTR(sw);
    mailboxInfo = GET_MAILBOX_INFO(sw);
    lportInfo   = &switchPtr->logicalPortInfo;
    range       = &switchPtr->glortRange;
    rangeBase   = range->glortBase;
    rangeMax    = FM_MAX_GLORT;
    rangeCount  = 0;
    glort       = 0;

    last = start + numGlorts - 1;

    /***************************************************
     * Validate arguments.
     **************************************************/

    err = fmVerifyGlortRange(start, numGlorts);
    if (err != FM_OK)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (glortType >= FM_GLORT_TYPE_MAX)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (numGlorts <= 0)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    /***************************************************
     * Determine correct range boundaries.
     **************************************************/

    GetGlortRange(switchPtr, glortType, &rangeBase, &rangeCount);
    rangeMax = rangeBase + rangeCount - 1;

    if ( ( start < rangeBase ||
           last > rangeMax ) &&
         ( !IsGlortReservedForType(switchPtr, start, glortType) ||
           !IsGlortReservedForType(switchPtr, last, glortType) ) )
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_IN_USE);
    }

    if (start > last)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    /***************************************************
     * Request the GloRT range.
     **************************************************/

    for (glort = start ; glort <= last ; ++glort)
    {
        err = RequestGlort(switchPtr, glort, glortType);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

ABORT:
    if (glort != 0 && err == FM_ERR_GLORT_IN_USE)
    {
        for (--glort ; glort >= start ; --glort)
        {
            ReleaseGlort(switchPtr, glort, glortType, FALSE);
        }
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmRequestGlortRange */




/*****************************************************************************/
/** fmReleaseGlortRangeInt
 * \ingroup intPort
 *
 * \desc            Releases the GloRT range. This function changes the state of
 *                  as many GloRTs as possible and returns an error if there was
 *                  any problem.The range should be allocated using
 *                  the fmRequestGlortRange function.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       start is the first GloRT to allocate.
 *
 * \param[in]       numGlorts is the size of the block to allocate.
 *
 * \param[in]       glortType identifies the potential GloRT range owner.
 *
 * \param[in]       pending indicates whether the range should be released or
 *                  just marked as "free pending".
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_GLORT_IN_USE if any of GloRTs is not destined to
 *                  be released by the caller.
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the range is too small
 * \return          FM_ERR_NO_GLORTS_ALLOCATED if any of GloRTs was not allocated
 *                  before (or was already released).
 *
 *****************************************************************************/
fm_status fmReleaseGlortRangeInt(fm_int       sw,
                                 fm_uint32    start,
                                 fm_int       numGlorts,
                                 fm_glortType glortType,
                                 fm_bool      pending)
{
    fm_status           err;
    fm_status           tempErr;
    fm_switch *         switchPtr;
    fm_mailboxInfo *    mailboxInfo;
    fm_logicalPortInfo* lportInfo;
    fm_glortRange *     range;
    fm_uint32           glort;
    fm_uint32           last;
    fm_uint32           rangeBase;
    fm_uint32           rangeMax;
    fm_int              rangeCount;


    VALIDATE_AND_PROTECT_SWITCH(sw);


    err         = FM_OK;
    tempErr     = FM_OK;
    switchPtr   = GET_SWITCH_PTR(sw);
    mailboxInfo = GET_MAILBOX_INFO(sw);
    lportInfo   = &switchPtr->logicalPortInfo;
    range       = &switchPtr->glortRange;
    rangeBase   = range->glortBase;
    rangeMax    = FM_MAX_GLORT;
    rangeCount  = 0;
    glort       = 0;

    last = start + numGlorts - 1;

    /***************************************************
     * Validate arguments.
     **************************************************/

    err = fmVerifyGlortRange(start, numGlorts);
    if (err != FM_OK)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (glortType >= FM_GLORT_TYPE_MAX)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (numGlorts <= 0)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    /***************************************************
     * Determine correct range boundaries.
     **************************************************/

    GetGlortRange(switchPtr, glortType, &rangeBase, &rangeCount);
    rangeMax = rangeBase + rangeCount - 1;

    if ( ( start < rangeBase ||
           last > rangeMax ) &&
         ( !IsGlortReservedForType(switchPtr, start, glortType) ||
           !IsGlortReservedForType(switchPtr, last, glortType) ) )
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_IN_USE);
    }

    if (start > last)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    /***************************************************
     * Release the GloRT range and save the first error.
     **************************************************/

    for (glort = start ; glort <= last ; ++glort)
    {
        err = ReleaseGlort(switchPtr, glort, glortType, pending);
        if ( (err != FM_OK) && (tempErr == FM_OK) )
        {
            tempErr = err;
        }
    }

    if (tempErr != FM_OK)
    {
        err = tempErr;
    }

ABORT:

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReleaseGlortRangeInt */




/*****************************************************************************/
/** fmReleaseGlortRange
 * \ingroup intPort
 *
 * \desc            Releases the GloRT range. This function changes the state of
 *                  as many GloRTs as possible and returns an error if there was
 *                  any problem. The range should be allocated using
 *                  the fmRequestGlortRange function.
 *                                                                      \lb\lb
 *                  See also fmReleaseGlortRangeInt.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       start is the first GloRT to allocate.
 *
 * \param[in]       numGlorts is the size of the block to allocate.
 *
 * \param[in]       glortType identifies the potential GloRT range owner.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_GLORT_IN_USE if any of GloRTs is not destined to
 *                  be released by the caller.
 * \return          FM_ERR_NO_GLORTS_ALLOCATED if any of GloRTs was not allocated
 *                  before (or was already released).
 *
 *****************************************************************************/
fm_status fmReleaseGlortRange(fm_int       sw,
                              fm_uint32    start,
                              fm_int       numGlorts,
                              fm_glortType glortType)
{
    fm_status           err;

    err = FM_OK;

    /* By default we do not need to set "free pending" state */
    err = fmReleaseGlortRangeInt(sw,
                                 start,
                                 numGlorts,
                                 glortType,
                                 FALSE);

    return err;

}   /* end fmReleaseGlortRange */




/*****************************************************************************/
/** fmReserveGlortRange
 * \ingroup intPort
 *
 * \desc            Reserves the GloRT range. The range should be unreserved
 *                  using the fmUnreserveGlortRange function.
 *                                                                      \lb\lb
 *                  See also fmReleaseGlortRangeInt.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       start is the first GloRT to reserve. It does not have to be
 *                  within a regular range assigned to a given type.
 *
 * \param[in]       numGlorts is the size of the block to allocate.
 *
 * \param[in]       glortType identifies the new GloRT range owner.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_UNSUPPORTED if it is not possible to reserve
 *                  for the given type
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the range is too small
 * \return          FM_ERR_GLORT_IN_USE if any of GloRTs is already used
 *                  or not destined to be allocated by the caller.
 *
 *****************************************************************************/
fm_status fmReserveGlortRange(fm_int       sw,
                              fm_uint32    start,
                              fm_int       numGlorts,
                              fm_glortType glortType)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_mailboxInfo *    mailboxInfo;
    fm_logicalPortInfo* lportInfo;
    fm_glortRange *     range;
    fm_uint32           glort;
    fm_uint32           last;
    fm_uint32           rangeBase;
    fm_uint32           rangeMax;
    fm_int              rangeCount;


    VALIDATE_AND_PROTECT_SWITCH(sw);


    err         = FM_OK;
    switchPtr   = GET_SWITCH_PTR(sw);
    mailboxInfo = GET_MAILBOX_INFO(sw);
    lportInfo   = &switchPtr->logicalPortInfo;
    range       = &switchPtr->glortRange;
    rangeBase   = range->glortBase;
    rangeMax    = FM_MAX_GLORT;
    rangeCount  = FM_MAX_GLORT;
    glort       = 0;

    last = start + numGlorts - 1;

    /***************************************************
     * Validate arguments.
     **************************************************/

    err = fmVerifyGlortRange(start, numGlorts);
    if (err != FM_OK)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (glortType >= FM_GLORT_TYPE_MAX)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (numGlorts <= 0)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if ( glortType != FM_GLORT_TYPE_MULTICAST &&
         glortType != FM_GLORT_TYPE_LAG       &&
         glortType != FM_GLORT_TYPE_LBG )
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_UNSUPPORTED);
    }

    /***************************************************
     * Determine correct range boundaries.
     **************************************************/

    if ( start < rangeBase ||
         last > rangeMax )
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_IN_USE);
    }

    if (start > last)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    /***************************************************
     * Reserve the GloRT range.
     **************************************************/

    for (glort = start ; glort <= last ; ++glort)
    {
        err = ReserveGlort(switchPtr, glort, glortType);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

ABORT:
    if (glort != 0 && err == FM_ERR_GLORT_IN_USE)
    {
        for (--glort ; glort >= start ; --glort)
        {
            UnreserveGlort(switchPtr, glort, glortType);
        }
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end ReserveGlortRange */




/*****************************************************************************/
/** fmUnreserveGlortRange
 * \ingroup intPort
 *
 * \desc            Releases the reserved GloRT range. The range should be
 *                  reserved using fmReserveGlortRange function. This
 *                  function changes the state of as many GloRTs as possible and
 *                  returns an error if there was any problem.
 *                                                                      \lb\lb
 *                  See also fmReleaseGlortRangeInt.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       start is the first GloRT to release.
 *
 * \param[in]       numGlorts is the size of the block to release.
 *
 * \param[in]       glortType identifies the GloRT range owner.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the range is too small
 * \return          FM_ERR_GLORT_IN_USE if any of GloRTs is still used
 * \return          FM_ERR_NO_GLORTS_ALLOCATED if any of GloRTs was not reserved
 *                  before or was reserved by someone else.
 *
 *****************************************************************************/
fm_status fmUnreserveGlortRange(fm_int       sw,
                                fm_uint32    start,
                                fm_int       numGlorts,
                                fm_glortType glortType)
{
    fm_status           err;
    fm_status           tempErr;
    fm_switch *         switchPtr;
    fm_mailboxInfo *    mailboxInfo;
    fm_logicalPortInfo* lportInfo;
    fm_glortRange *     range;
    fm_uint32           glort;
    fm_uint32           last;
    fm_uint32           rangeBase;
    fm_uint32           rangeMax;
    fm_int              rangeCount;


    VALIDATE_AND_PROTECT_SWITCH(sw);


    err         = FM_OK;
    tempErr     = FM_OK;
    switchPtr   = GET_SWITCH_PTR(sw);
    mailboxInfo = GET_MAILBOX_INFO(sw);
    lportInfo   = &switchPtr->logicalPortInfo;
    range       = &switchPtr->glortRange;
    rangeBase   = range->glortBase;
    rangeMax    = FM_MAX_GLORT;
    rangeCount  = 0;
    glort       = 0;

    last = start + numGlorts - 1;

    /***************************************************
     * Validate arguments.
     **************************************************/

    err = fmVerifyGlortRange(start, numGlorts);
    if (err != FM_OK)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (glortType >= FM_GLORT_TYPE_MAX)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (numGlorts <= 0)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if ( glortType != FM_GLORT_TYPE_MULTICAST &&
         glortType != FM_GLORT_TYPE_LAG       &&
         glortType != FM_GLORT_TYPE_LBG )
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    /***************************************************
     * Determine correct range boundaries.
     **************************************************/

    if ( start < rangeBase ||
         last > rangeMax )
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_IN_USE);
    }

    if (start > last)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    /***************************************************
     * Release the GloRT range and save the first error.
     **************************************************/

    for (glort = start ; glort <= last ; ++glort)
    {
        err = UnreserveGlort(switchPtr, glort, glortType);
        if ( (err != FM_OK) && (tempErr == FM_OK) )
        {
            tempErr = err;
        }
    }

    if (tempErr != FM_OK)
    {
        err = tempErr;
    }

ABORT:

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmUnreserveGlortRange */




/*****************************************************************************/
/** fmFindFreeGlortRangeInt
 * \ingroup intPort
 *
 * \desc            Finds an unused block of GloRTs.
 *                                                                      \lb\lb
 *                  See also fmFindFreeGlortRange.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       numGlorts is the required number number of GloRTs in
 *                  the block.
 *
 * \param[in]       glortType identifies the potential GloRT range owner.
 *
 * \param[in]       rangeStart is the starting index for the search.
 *
 * \param[in]       rangeSize is the size of the block for the search.
 *
 * \param[in]       reserved indicates whether the unused block of GloRTs
 *                  should be a part of reserved range.
 *
 * \param[out]      startGlort points to a caller-provided unsigned integer
 *                  variable where this function will place the starting
 *                  index of the first correct unused block of GloRTs.
 *                  If startGlort is NULL then the return value is the only
 *                  indicator if the wanted range was found.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the range is too small
 * \return          FM_ERR_NOT_FOUND if the contiguous block was not found
 *
 *****************************************************************************/
fm_status fmFindFreeGlortRangeInt(fm_int       sw,
                                  fm_int       numGlorts,
                                  fm_glortType glortType,
                                  fm_uint32    rangeStart,
                                  fm_int       rangeSize,
                                  fm_bool      reserved,
                                  fm_uint32 *  startGlort)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_logicalPortInfo *lportInfo;
    fm_glortRange *     range;
    fm_uint32           glort;
    fm_int              freeCount;
    fm_uint32           start;
    fm_uint32           rangeBase;;
    fm_uint32           rangeMax;
    fm_int              rangeCount;
    fm_uint32           rangeEnd;


    VALIDATE_AND_PROTECT_SWITCH(sw);


    err        = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);
    lportInfo  = &switchPtr->logicalPortInfo;
    range      = &switchPtr->glortRange;
    rangeBase  = range->glortBase;
    rangeMax   = FM_MAX_GLORT;
    rangeCount = 0;
    glort      = 0;

    rangeEnd  = rangeStart + rangeSize - 1;
    freeCount = 0;
    start     = 0;

    /***************************************************
     * Validate arguments.
     **************************************************/

    err = fmVerifyGlortRange(rangeStart, rangeSize);
    if (err != FM_OK)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (glortType >= FM_GLORT_TYPE_MAX)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    if (numGlorts <= 0)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_INVALID_ARGUMENT);
    }

    /***************************************************
     * Determine correct range boundaries.
     **************************************************/

    /* If GloRT is reserved then it does not have to be
     * in specific range */
    if (!reserved)
    {
        GetGlortRange(switchPtr, glortType, &rangeBase, &rangeCount);
        rangeMax = rangeBase + rangeCount - 1;
    }

    if (rangeStart < rangeBase)
    {
        rangeStart = rangeBase;
    }

    if (rangeEnd > rangeMax)
    {
        rangeEnd = rangeMax;
    }

    if (rangeStart > rangeEnd)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err = FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    /***************************************************
     * Find an unused block of GloRTs.
     **************************************************/

    for (glort = rangeStart ; glort <= rangeEnd ; glort++)
    {
        /***************************************************
         * Three conditions must be met:
         *   (a) The GloRT is in range 0 - FM_MAX_GLORT
         *   (b) The GloRT is unused
         *   (c) The GloRT is correctly reserved unless
         *       reserved GloRTs are not expected
         **************************************************/

        if (!IsGlortCorrect(glort))
        {
            FM_LOG_ABORT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
        }

        if ( FM_IS_GLORT_FREE(lportInfo, glort) &&
             (reserved == IsGlortReservedForType(switchPtr, glort, glortType)) )
        {
            if (freeCount == 0)
            {
                start = glort;
            }

            ++freeCount;

            if (freeCount >= numGlorts)
            {
                break;
            }
        }
        else
        {
            freeCount = 0;
            start = 0;
        }
    }

    if (freeCount >= numGlorts)
    {
        if (startGlort != NULL)
        {
            *startGlort = start;
        }
    }
    else
    {
        err = FM_ERR_NOT_FOUND;
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmFindFreeGlortRangeInt */




/*****************************************************************************/
/** fmFindFreeGlortRange
 * \ingroup intPort
 *
 * \desc            Finds an unused block of GloRTs. Only unreserved ranges are
 *                  taken into consideration.
 *                                                                      \lb\lb
 *                  See also fmFindFreeGlortRangeInt.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       numGlorts is the required number number of GloRTs in
 *                  the block.
 *
 * \param[in]       glortType identifies the potential GloRT range owner.
 *
 * \param[out]      startGlort points to a caller-provided unsigned integer
 *                  variable where this function will place the starting
 *                  index of the first correct unused block of GloRTs.
 *                  If startGlort is NULL then the return value is the only
 *                  indicator if the wanted range was found.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch in not up
 * \return          FM_ERR_INVALID_ARGUMENT if the range is invalid
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the range is too small
 * \return          FM_ERR_NOT_FOUND if the contiguous block was not found
 *
 *****************************************************************************/
fm_status fmFindFreeGlortRange(fm_int       sw,
                               fm_int       numGlorts,
                               fm_glortType glortType,
                               fm_uint32 *  startGlort)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_glortRange *     range;
    fm_uint32           rangeBase;
    fm_uint32           rangeCount;

    err        = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);
    range      = &switchPtr->glortRange;

    rangeBase  = range->glortBase;
    rangeCount = FM_MAX_GLORT - rangeBase + 1;

    /* By default the search range is the whole glort range
     * excluding reserved GloRTs. The validation responibility
     * is shifted onto internal version of the funciton. */
    err = fmFindFreeGlortRangeInt(sw,
                                  numGlorts,
                                  glortType,
                                  rangeBase,
                                  rangeCount,
                                  FALSE,
                                  startGlort);

    return err;

}   /* end fmFindUnusedGlorts */




/*****************************************************************************/
/** fmDbgDumpGlortRanges
 * \ingroup intDebug
 *
 * \desc            Displays GloRT ranges of a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpGlortRanges(fm_int sw)
{
    fm_switch *         switchPtr;
    fm_mailboxInfo *    mailboxInfo;
    fm_glortRange *     range;
    fm_glortInfo *      info;
    fm_uint32           glort;
    fm_int              glortState;
    fm_glortType        glortType;
    fm_glortType        newType;
    fm_uint32           rangeBase;
    fm_uint32           rangeMax;
    fm_logicalPortInfo *lportInfo;
    fm_uint32           index;
    fm_bool             isStart;
    fm_bool             isEnd;
    fm_int              stateNum;
    fm_uint             rangeStart;
    fm_uint             rangeEnd;
    fm_int              rangeCount;
    fm_char             rangeName[MAX_BUF_SIZE];
    fm_char             stateName[MAX_BUF_SIZE];


    VALIDATE_AND_PROTECT_SWITCH(sw);


    switchPtr   = GET_SWITCH_PTR(sw);
    mailboxInfo = GET_MAILBOX_INFO(sw);
    range       = &switchPtr->glortRange;
    info        = &switchPtr->glortInfo;
    glortState  = 0;
    glortType   = FM_GLORT_TYPE_UNSPECIFIED;
    newType     = FM_GLORT_TYPE_UNSPECIFIED;
    lportInfo   = &switchPtr->logicalPortInfo;
    rangeBase   = range->glortBase;
    rangeMax    = rangeBase | range->glortMask;
    index       = 0;
    isStart     = TRUE;
    isEnd       = FALSE;
    stateNum    = 0;
    rangeStart  = 0;
    rangeEnd    = 0;
    rangeCount  = 0;
    glort       = 0;

    FM_LOG_PRINT("|=====|=================|=======|==========================|\n");
    FM_LOG_PRINT("| idx |     GloRT range | count |                    state |\n");

    for (glort = 0 ; glort <= FM_MAX_GLORT ; ++glort)
    {
        /* check if the GloRT is a base of new GloRT range or if the previous
         * one was the last GloRT int the range */
        isEnd = FALSE;
        GetGlortType(switchPtr, glort, &newType);
        isStart = (glortType != newType);
        if (isStart)
        {
            glortType = newType;
            GetGlortRange(switchPtr, glortType, &rangeStart, &rangeCount);

            if ( (glort != rangeStart) ||
                 (glortType == FM_GLORT_TYPE_UNSPECIFIED) )
            {
                isStart = FALSE;
                isEnd = TRUE;
            }
            else
            {
                rangeEnd = glort + rangeCount - 1;
            }

            GlortTypeToText(glortType, MAX_BUF_SIZE, rangeName);
            if (glort == rangeBase)
            {
                FM_STRCAT_S(rangeName, MAX_BUF_SIZE, " (GloRT Base)");
            }
        }

        /* We have to print something if we are at the beginning of new range
         * or if the state has changed. */
        if ( glort == 0 ||
             isStart    ||
             isEnd      ||
             (glortState != lportInfo->glortState[glort]) )
        {

            /**************************************************
             * Print the last GloRT of the same state and the
             * state name
             **************************************************/

            GlortStateToText(glortState, MAX_BUF_SIZE, stateName);

            if (stateNum == 1)
            {
                /* there is only one GloRT in the range */
                FM_LOG_PRINT("          | %5d | %24s |\n", stateNum, stateName);
            }
            else if (glort != 0)
            {
                FM_LOG_PRINT(" - 0x%-4X | %5d | %24s |\n", glort-1, stateNum, stateName);
            }

            /**************************************************
             * Print the information about the beginning of new
             * range
             **************************************************/

            if (isStart)
            {
                /* the GloRT is the base of new range */
                FM_LOG_PRINT("|=====|=================|=======|==========================|\n");
                FM_SPRINTF_S(rangeName + strlen(rangeName), MAX_BUF_SIZE, " (0x%X - 0x%X):", glort, rangeEnd);
                FM_LOG_PRINT("|     | %-50s |\n", rangeName);
            }
            else if ((glort - 1) == rangeMax)
            {
                /* the range is finished but we have not achieved a new base */
                FM_LOG_PRINT("|=MAX=|=================|=======|==========================|\n");
            }
            else if (isEnd)
            {
                /* the range is finished but we have not achieved a new base */
                FM_LOG_PRINT("|=====|=================|=======|==========================|\n");
            }

            /**************************************************
             * New state or GloRT range
             **************************************************/

            FM_LOG_PRINT("| %3u | 0x%-4X", ++index,  glort);
            glortState = lportInfo->glortState[glort];
            stateNum = 0;

        }   /* end if ( glort == 0 || ... */

        ++stateNum;

    }   /* end for (glort = 0 ; glort <= FM_MAX_GLORT ; ++glort) */

    FM_LOG_PRINT(" - 0x%-4X | %5d | %24s |\n", glort-1, stateNum, stateName);
    FM_LOG_PRINT("|=====|=================|=======|==========================|\n");

    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpGlortRanges */
