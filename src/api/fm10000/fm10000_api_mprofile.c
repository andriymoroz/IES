/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mprofile.c
 * Creation Date:   March 13, 2014
 * Description:     Mirror Profile management code.
 *
 * Copyright (c) 2014 - 2016, Intel Corporation
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


typedef struct
{
    fm_int  intrinsicType;
    fm_int  trapCode;

} intrinsicMirrorDesc;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


static intrinsicMirrorDesc intrinsicMirrors[] =
{
    { FM_INTRINSIC_MIRROR_FFU,          FM10000_MIRROR_FFU_CODE },
    { FM_INTRINSIC_MIRROR_RESERVED_MAC, FM10000_MIRROR_IEEE_CODE },
    { FM_INTRINSIC_MIRROR_ARP_REDIRECT, FM10000_MIRROR_ARP_CODE },
    { FM_INTRINSIC_MIRROR_ICMP,         FM10000_MIRROR_ICMP_TTL },
    { FM_INTRINSIC_MIRROR_TTL,          FM10000_MIRROR_TTL }
};

#define NUM_INTRINSIC_MIRRORS           FM_NENTRIES(intrinsicMirrors)


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** IntrinsicTypeToText
 * \ingroup intTriggerInt
 *
 * \desc            Returns the textual representation of the intrinsic
 *                  mirror type.
 * 
 * \param[in]       intrinsicType is the intrinsic mirror type.
 * 
 * \return          Textual representation of the mirror type.
 *
 *****************************************************************************/
static fm_text IntrinsicTypeToText(fm_int intrinsicType)
{

    switch (intrinsicType)
    {
        case FM_INTRINSIC_MIRROR_NONE:
            return "NONE";

        case FM_INTRINSIC_MIRROR_FFU:
            return "FFU";

        case FM_INTRINSIC_MIRROR_RESERVED_MAC:
            return "RESERVED_MAC";

        case FM_INTRINSIC_MIRROR_ARP_REDIRECT:
            return "ARP_REDIRECT";

        case FM_INTRINSIC_MIRROR_ICMP:
            return "ICMP";

        case FM_INTRINSIC_MIRROR_TTL:
            return "TTL";

        default:
            return "UNKNOWN";
    }

}   /* end IntrinsicTypeToText */




/*****************************************************************************/
/** ProfileTypeToText
 * \ingroup intTriggerInt
 *
 * \desc            Returns the textual representation of the mirror
 *                  profile entry type.
 * 
 * \param[in]       entryType is the mirror profile entry type.
 * 
 * \return          Textual representation of the profile type.
 *
 *****************************************************************************/
static fm_text ProfileTypeToText(fm_int entryType)
{

    switch (entryType)
    {
        case FM10000_MIRROR_PROFILE_NONE:
            return "NONE";

        case FM10000_MIRROR_PROFILE_INTRINSIC:
            return "INTRINSIC";

        case FM10000_MIRROR_PROFILE_MIRROR:
            return "MIRROR";

        case FM10000_MIRROR_PROFILE_LOG:
            return "LOG";

        default:
            return "UNKNOWN";
    }

}   /* end ProfileTypeToText */




/*****************************************************************************/
/** AllocProfileIndex
 * \ingroup intTriggerInt
 *
 * \desc            Allocates a hardware profile index.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      index points to the location to receive the hardware
 *                  profile index.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free hardware
 *                  profile indexes.
 *
 *****************************************************************************/
static fm_status AllocProfileIndex(fm_int sw, fm_uint32 * index)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm_int                  bitNo;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d\n", sw);
                                   
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (index == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /**************************************************
     * Find highest-numbered available profile index.
     **************************************************/

    err = fmFindLastBitInBitArray(&trigInfo->usedProfileIndex,
                                  FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES - 1,
                                  FM10000_TRIGGER_RES_FREE,
                                  &bitNo);

    if (err == FM_OK && bitNo < 0)
    {
        err = FM_ERR_NO_FREE_TRIG_RES;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Mark it as used.
     **************************************************/

    err = fmSetBitArrayBit(&trigInfo->usedProfileIndex, 
                           bitNo, 
                           FM10000_TRIGGER_RES_USED);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Return profile index to caller.
     **************************************************/

    *index = bitNo;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end AllocProfileIndex */




/*****************************************************************************/
/** FreeProfileIndex
 * \ingroup intTriggerInt
 *
 * \desc            Deallocates a hardware profile index.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       index is the hardware profile index to be freed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
static fm_status FreeProfileIndex(fm_int sw, fm_uint32 index)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%u\n", sw, index);
                                   
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (index >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    err = fmSetBitArrayBit(&trigInfo->usedProfileIndex, 
                           index, 
                           FM10000_TRIGGER_RES_FREE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end FreeProfileIndex */




/*****************************************************************************/
/** AllocProfileEntry
 * \ingroup intTriggerInt
 *
 * \desc            Allocates a mirror profile entry.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      handle points to the location to receive the handle
 *                  of the mirror profile entry.
 * 
 * \param[in]       isInternal is TRUE if the resource is being reserved
 *                  internally by the API.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free mirror
 *                  profile handles.
 *
 *****************************************************************************/
static fm_status AllocProfileEntry(fm_int      sw,
                                   fm_uint32 * handle,
                                   fm_bool     isInternal)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_int                  entryId;
    fm_status               err;
    fm_bool                 haveHandle;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    haveHandle = FALSE;

    if (handle == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /**************************************************
     * Allocate a mirror profile entry.
     **************************************************/

    /* Find first free mirror profile entry. */
    err = fmFindBitInBitArray(&trigInfo->usedProfileHandle,
                              0,
                              FM10000_TRIGGER_RES_FREE,
                              &entryId);
    if (err == FM_OK && entryId < 0)
    {
        err = FM_ERR_NO_FREE_TRIG_RES;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Mark the mirror profile entry as used. */
    err = fmSetBitArrayBit(&trigInfo->usedProfileHandle,
                           entryId,
                           FM10000_TRIGGER_RES_USED);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    haveHandle = TRUE;

    /* Save the isInternal state of the profile entry. */
    err = fmSetBitArrayBit(&trigInfo->profileHandleInternal,
                           entryId,
                           isInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Initialize the mirror profile entry.
     **************************************************/

    profEntry = &trigInfo->profileEntry[entryId];

    FM_CLEAR(*profEntry);

    profEntry->index = -1;

    /**************************************************
     * Return the mirror profile handle.
     **************************************************/

    *handle = entryId;

ABORT:
    if (err != FM_OK)
    {
        if (haveHandle)
        {
            /* Free mirror profile entry. */
            fmSetBitArrayBit(&trigInfo->usedProfileHandle,
                             entryId,
                             FM10000_TRIGGER_RES_FREE);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end AllocProfileEntry */




/*****************************************************************************/
/** FreeProfileEntry
 * \ingroup intTriggerInt
 *
 * \desc            Frees a mirror profile entry.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      handle is the mirror profile handle.
 * 
 * \param[in]       isInternal is TRUE if the resource is being reserved
 *                  internally by the API.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirror profile
 *                  handle is invalid.
 * \return          FM_ERR_INTERNAL_RESOURCE if an external caller attempts
 *                  to free an internal resource.
 *
 *****************************************************************************/
static fm_status FreeProfileEntry(fm_int    sw,
                                  fm_uint32 handle,
                                  fm_bool   isInternal)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_bool                 resInternal;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    err = fmGetBitArrayBit(&trigInfo->profileHandleInternal,
                           handle,
                           &resInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (resInternal && !isInternal)
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    profEntry->entryType = FM10000_MIRROR_PROFILE_NONE;

    err = fmSetBitArrayBit(&trigInfo->usedProfileHandle,
                           handle,
                           FM10000_TRIGGER_RES_FREE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end FreeProfileEntry */




/*****************************************************************************/
/** CreateProfileEntry
 * \ingroup intTriggerInt
 *
 * \desc            Creates a mirror profile entry.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       entryType is the type of mirror profile entry to
 *                  create.
 * 
 * \param[out]      handle points to the location to receive the handle
 *                  of the mirror profile entry.
 * 
 * \param[in]       isInternal is TRUE if the resource is being reserved
 *                  internally by the API.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free profile
 *                  handles or mirror profile indexes.
 *
 *****************************************************************************/
static fm_status CreateProfileEntry(fm_int      sw,
                                    fm_int      entryType,
                                    fm_uint32 * handle,
                                    fm_bool     isInternal)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_uint32               entryId;
    fm_uint32               profileId;
    fm_status               err;
    fm_bool                 haveEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d entryType=%s\n",
                 sw,
                 ProfileTypeToText(entryType));

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    haveEntry = FALSE;

    if (handle == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /**************************************************
     * Allocate a mirror profile entry.
     **************************************************/

    err = AllocProfileEntry(sw, &entryId, isInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    haveEntry = TRUE;

    /**************************************************
     * Allocate a mirror profile index.
     **************************************************/

    err = AllocProfileIndex(sw, &profileId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Initialize the mirror profile entry.
     **************************************************/

    profEntry = &trigInfo->profileEntry[entryId];

    profEntry->entryType = entryType;
    profEntry->index = profileId;

    trigInfo->profileMap[profileId] = entryId;

    /**************************************************
     * Return the mirror profile handle.
     **************************************************/

    *handle = entryId;

ABORT:
    if (err != FM_OK)
    {
        if (haveEntry)
        {
            FreeProfileEntry(sw, entryId, isInternal);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end CreateProfileEntry */




/*****************************************************************************/
/** ReadMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Reads a mirror profile entry from the hardware.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       index is the hardware profile index.
 * 
 * \param[out]      config points to the structure to receive the
 *                  mirror profile configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
static fm_status ReadMirrorProfile(fm_int              sw,
                                   fm_int              index,
                                   fm10000_mirrorCfg * config)
{
    fm_switch * switchPtr;
    fm_uint32   regFh;
    fm_uint64   regMod;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%d\n", sw, index);

    switchPtr = GET_SWITCH_PTR(sw);

    if (index < 0 || index >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    /**************************************************
     * Read register values.
     **************************************************/

    err = switchPtr->ReadUINT32(sw,
                                FM10000_FH_MIRROR_PROFILE_TABLE(index),
                                &regFh);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmRegCacheReadUINT64(sw,
                               &fm10000CacheModMirrorProfTable,
                               index,
                               &regMod);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    /**************************************************
     * Get FM10000_FH_MIRROR_PROFILE_TABLE.
     **************************************************/

    config->physPort = FM_GET_FIELD(regFh,
                                    FM10000_FH_MIRROR_PROFILE_TABLE,
                                    port);

    /**************************************************
     * Get FM10000_MOD_MIRROR_PROFILE_TABLE.
     **************************************************/

    config->glort = FM_GET_FIELD64(regMod,
                                   FM10000_MOD_MIRROR_PROFILE_TABLE,
                                   GLORT);

    config->truncate = FM_GET_BIT64(regMod,
                                    FM10000_MOD_MIRROR_PROFILE_TABLE,
                                    TRUNC);

    config->vlan = FM_GET_FIELD64(regMod,
                                  FM10000_MOD_MIRROR_PROFILE_TABLE,
                                  VID);

    config->vlanPri = FM_GET_FIELD64(regMod,
                                     FM10000_MOD_MIRROR_PROFILE_TABLE,
                                     VPRI);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end ReadMirrorProfile */




/*****************************************************************************/
/** ResetMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Resets a hardware mirror profile.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the hardware profile index.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
static fm_status ResetMirrorProfile(fm_int sw, fm_int index)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%d\n", sw, index);

    switchPtr = GET_SWITCH_PTR(sw);

    if (index < 0 || index >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_FH_MIRROR_PROFILE_TABLE(index),
                                 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmRegCacheWriteUINT64(sw,
                                &fm10000CacheModMirrorProfTable,
                                index,
                                0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end ResetMirrorProfile */




/*****************************************************************************/
/** WriteMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Writes a mirror profile entry to the hardware.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       index is the hardware profile index.
 * 
 * \param[in]       config points to the mirror profile configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
static fm_status WriteMirrorProfile(fm_int              sw,
                                    fm_int              index,
                                    fm10000_mirrorCfg * config)
{
    fm_switch * switchPtr;
    fm_uint32   regFh;
    fm_uint64   regMod;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%d\n", sw, index);

    switchPtr = GET_SWITCH_PTR(sw);

    if (index < 0 || index >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    /**************************************************
     * Set FM10000_FH_MIRROR_PROFILE_TABLE.
     **************************************************/

    regFh = 0;

    FM_SET_FIELD(regFh,
                 FM10000_FH_MIRROR_PROFILE_TABLE,
                 port,
                 config->physPort);

    /**************************************************
     * Set FM10000_MOD_MIRROR_PROFILE_TABLE.
     **************************************************/

    regMod = 0;

    FM_SET_FIELD64(regMod,
                   FM10000_MOD_MIRROR_PROFILE_TABLE,
                   GLORT,
                   config->glort);

    FM_SET_BIT64(regMod,
                 FM10000_MOD_MIRROR_PROFILE_TABLE,
                 TRUNC,
                 config->truncate);

    FM_SET_FIELD64(regMod,
                   FM10000_MOD_MIRROR_PROFILE_TABLE,
                   VID,
                   config->vlan);

    FM_SET_FIELD64(regMod,
                   FM10000_MOD_MIRROR_PROFILE_TABLE,
                   VPRI,
                   config->vlanPri);

    /**************************************************
     * Write register values.
     **************************************************/

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_FH_MIRROR_PROFILE_TABLE(index),
                                 regFh);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmRegCacheWriteUINT64(sw,
                                &fm10000CacheModMirrorProfTable,
                                index,
                                regMod);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end WriteMirrorProfile */




/*****************************************************************************/
/** WriteIntrinsicProfileId
 * \ingroup intTriggerInt
 *
 * \desc            Writes an intrinsic profile index to the hardware.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       intrinsicType is the intrinsic profile type.
 * 
 * \param[in]       profileId is the hardware profile index.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
static fm_status WriteIntrinsicProfileId(fm_int    sw,
                                         fm_int    intrinsicType,
                                         fm_uint32 profileId)
{
    fm_switch * switchPtr;
    fm_uint32   reg32;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d intrinsicType=%s profileId=%u\n",
                 sw,
                 IntrinsicTypeToText(intrinsicType),
                 profileId);

    switchPtr = GET_SWITCH_PTR(sw);

    if (profileId >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    err = switchPtr->ReadUINT32(sw,
                                FM10000_LOG_MIRROR_PROFILE(),
                                &reg32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    switch (intrinsicType)
    {
        case FM_INTRINSIC_MIRROR_FFU:
            FM_SET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, FFU, profileId);
            break;

        case FM_INTRINSIC_MIRROR_RESERVED_MAC:
            FM_SET_FIELD(reg32,
                         FM10000_LOG_MIRROR_PROFILE,
                         ReservedMAC,
                         profileId);
            break;

        case FM_INTRINSIC_MIRROR_ARP_REDIRECT:
            FM_SET_FIELD(reg32,
                         FM10000_LOG_MIRROR_PROFILE,
                         ARPRedirect,
                         profileId);
            break;

        case FM_INTRINSIC_MIRROR_ICMP:
            FM_SET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, ICMP, profileId);
            break;

        case FM_INTRINSIC_MIRROR_TTL:
            FM_SET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, TTL, profileId);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;

    }   /* end switch (intrinsicType) */

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_LOG_MIRROR_PROFILE(),
                                 reg32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end WriteIntrinsicProfileId */




/*****************************************************************************/
/** WriteMirrorProfileId
 * \ingroup intTriggerInt
 *
 * \desc            Writes a mirror profile index to the hardware.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the profile entry handle.
 * 
 * \param[in]       profileId is the hardware profile index.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is invalid
 *                  or the profile is the wrong type.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 * \return          FM_ERR_INVALID_TRIG if the mirroring trigger is
 *                  invalid or undefined.
 *
 *****************************************************************************/
static fm_status WriteMirrorProfileId(fm_int    sw,
                                      fm_uint32 handle,
                                      fm_uint32 profileId)
{
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_treeIterator         triggerIt;
    fm_uint64               trigKey;
    fm10000_triggerEntry *  trigEntry;
    fm_uint32               regActionMirror;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d handle=%u profileId=%u\n",
                 sw,
                 handle,
                 profileId);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (profileId >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR ||
        profEntry->index < 0)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /**************************************************
     * Enumerate each of the defined triggers.
     **************************************************/

    fmTreeIterInit(&triggerIt, &trigInfo->triggerTree);

    for ( ; ; )
    {
        /* Get next trigger from tree. */
        err = fmTreeIterNext(&triggerIt, &trigKey, (void **) &trigEntry);
        if (err == FM_ERR_NO_MORE)
        {
            /* no need to set err */
            break;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        /* It must specify a mirroring action with the specified profile. */
        if (trigEntry->action->cfg.mirrorAction != FM_TRIGGER_MIRROR_ACTION_MIRROR ||
            trigEntry->action->param.mirrorProfile != handle)
        {
            continue;
        }

        FM_LOG_DEBUG(FM_LOG_CAT_TRIGGER,
                     "Updating trigger (%d, %d) to mirrorProfile %u\n",
                     FM10000_TRIGGER_KEY_TO_GROUP(trigKey),
                     FM10000_TRIGGER_KEY_TO_RULE(trigKey),
                     profileId);

        /* Update the mirror profile index in the trigger structure. */
        trigEntry->mirrorIndex = profileId;

        /* Write the mirror profile index to the hardware. */
        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_TRIGGER_ACTION_MIRROR(trigEntry->index),
                                    &regActionMirror);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        FM_SET_FIELD(regActionMirror,
                     FM10000_TRIGGER_ACTION_MIRROR,
                     MirrorProfileIndex,
                     profileId);

        err = switchPtr->WriteUINT32(sw, 
                                     FM10000_TRIGGER_ACTION_MIRROR(trigEntry->index),
                                     regActionMirror);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    }   /* end for ( ; ; ) */

    err = FM_OK;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end WriteMirrorProfileId */




/*****************************************************************************/
/** CreateLogProfile
 * \ingroup intTriggerInt
 *
 * \desc            Creates a log action mirror profile entry.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       logIndex is the hardware profile index.
 * 
 * \param[out]      handle points to the location to receive the handle
 *                  of the mirror profile entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free mirror
 *                  profile handles.
 *
 *****************************************************************************/
static fm_status CreateLogProfile(fm_int      sw,
                                  fm_uint32   logIndex,
                                  fm_uint32 * handle)
{
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm10000_mirrorCfg       config;
    fm_uint32               entryId;
    fm_status               err;
    fm_bool                 haveProfile;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d logIndex=%u\n", sw, logIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    haveProfile = FALSE;

    if (handle == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    if (logIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    /**************************************************
     * Allocate a mirror profile entry.
     **************************************************/

    err = AllocProfileEntry(sw, &entryId, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    haveProfile = TRUE;

    /**************************************************
     * Initialize the mirror profile entry.
     **************************************************/

    profEntry = &trigInfo->profileEntry[entryId];

    FM_CLEAR(*profEntry);

    profEntry->entryType = FM10000_MIRROR_PROFILE_LOG;
    profEntry->index = logIndex;

    trigInfo->profileMap[logIndex] = entryId;

    /**************************************************
     * Initialize the hardware mirror profile.
     **************************************************/

    FM_CLEAR(config);

    err = fmMapLogicalPortToPhysical(switchPtr,
                                     switchPtr->cpuPort,
                                     &config.physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmGetLogicalPortGlort(sw,
                                switchPtr->cpuPort,
                                &config.glort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = WriteMirrorProfile(sw, logIndex, &config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Return the mirror profile handle.
     **************************************************/

    *handle = entryId;

ABORT:
    if (err != FM_OK)
    {
        if (haveProfile)
        {
            FreeProfileEntry(sw, entryId, TRUE);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end CreateLogProfile */




/*****************************************************************************/
/** DeleteProfileEntry
 * \ingroup intTriggerInt
 *
 * \desc            Deletes a profile entry.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       entryType is the type of profile entry to delete.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       isInternal is TRUE if this is an internal resource.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is invalid
 *                  or the profile is the wrong type.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 * \return          FM_ERR_INTERNAL_RESOURCE if an external caller attempts
 *                  to delete an internal API resource.
 *
 *****************************************************************************/
static fm_status DeleteProfileEntry(fm_int      sw,
                                    fm_int      entryType,
                                    fm_uint32   handle,
                                    fm_bool     isInternal)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_bool                 resInternal;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d entryType=%s handle=%u\n",
                 sw,
                 ProfileTypeToText(entryType),
                 handle);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != entryType)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    err = fmGetBitArrayBit(&trigInfo->profileHandleInternal,
                           handle,
                           &resInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (resInternal && !isInternal)
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        goto ABORT;
    }

    /**************************************************
     * Free the mirror profile index.
     **************************************************/

    if (profEntry->index >= 0)
    {
        err = ResetMirrorProfile(sw, profEntry->index);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = FreeProfileIndex(sw, profEntry->index);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        trigInfo->profileMap[profEntry->index] = -1;
        profEntry->index = -1;
    }

    /**************************************************
     * Now free the profile handle.
     **************************************************/

    err = fmSetBitArrayBit(&trigInfo->usedProfileHandle,
                           handle,
                           FM10000_TRIGGER_RES_FREE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    profEntry->entryType = FM10000_MIRROR_PROFILE_NONE;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end DeleteProfileEntry */




/*****************************************************************************/
/** MoveProfileEntry
 * \ingroup intTriggerInt
 *
 * \desc            Moves a mirror profile to a new hardware location.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       newIndex is the new hardware profile index.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is invalid
 *                  or the profile is the wrong type.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
static fm_status MoveProfileEntry(fm_int    sw,
                                  fm_uint32 handle,
                                  fm_uint32 newIndex)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm10000_mirrorCfg       config;
    fm_uint32               oldIndex;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    switch (profEntry->entryType)
    {
        case FM10000_MIRROR_PROFILE_MIRROR:
            /* Only allow MIRROR profiles to be displaced. */
            break;

        default:
            err = FM_ERR_INVALID_MIRROR_PROFILE;
            goto ABORT;

    }   /* end switch (profEntry->entryType) */

    if (newIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    oldIndex = profEntry->index;

    /**************************************************
     * Copy hardware mirror profile to new location.
     **************************************************/

    err = ReadMirrorProfile(sw, oldIndex, &config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = WriteMirrorProfile(sw, newIndex, &config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Update mirror profile entry and map.
     **************************************************/

    profEntry->index = newIndex;

    trigInfo->profileMap[oldIndex] = -1;
    trigInfo->profileMap[newIndex] = handle;

    /**************************************************
     * Update hardware references to profile index.
     **************************************************/

    if (profEntry->entryType == FM10000_MIRROR_PROFILE_INTRINSIC)
    {
        err = WriteIntrinsicProfileId(sw, profEntry->intrinsicType, newIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }
    else if (profEntry->entryType == FM10000_MIRROR_PROFILE_MIRROR)
    {
        err = WriteMirrorProfileId(sw, handle, newIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end MoveProfileEntry */




/*****************************************************************************/
/** ReserveProfileIndex
 * \ingroup intTriggerInt
 *
 * \desc            Reserves a mirror profile index. If the specified
 *                  profile is in use, it will be relocated.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the hardware profile index to be reserved.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free hardware
 *                  profile indexes.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware index
 *                  is invalid.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirror profile
 *                  handle is invalid or the profile is the wrong type.
 *
 *****************************************************************************/
static fm_status ReserveProfileIndex(fm_int sw, fm_uint32 index)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_int                  handle;
    fm_uint32               newIndex;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%u\n", sw, index);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (index >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    if (fmIsBitInBitArray(&trigInfo->usedProfileIndex, index))
    {
        /* Map profile index to mirror handle. */
        handle = trigInfo->profileMap[index];
        if (handle < 0 || handle >= FM10000_NUM_MIRROR_PROFILES)
        {
            err = FM_ERR_INVALID_MIRROR_PROFILE;
            goto ABORT;
        }

        /* If the destination isn't a MIRROR profile, we can't move it.
         * Report this as a resource exhaustion condition. */
        profEntry = &trigInfo->profileEntry[handle];
        if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR)
        {
            err = FM_ERR_NO_FREE_TRIG_RES;
            goto ABORT;
        }

        /* Assign a new hardware profile index. */
        err = AllocProfileIndex(sw, &newIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        /* Move the existing mirror profile to the new index. */
        err = MoveProfileEntry(sw, handle, newIndex);
        if (err != FM_OK)
        {
            FreeProfileIndex(sw, newIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
        }
    }
    else
    {
        /* Mark the hardware profile index as used. */
        err = fmSetBitArrayBit(&trigInfo->usedProfileIndex,
                               index,
                               FM10000_TRIGGER_RES_USED);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end ReserveProfileIndex */




/*****************************************************************************/
/** HandleLogAction
 * \ingroup intTriggerInt
 *
 * \desc            Associates a log action profile with a trigger.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       group is the trigger group number.
 * 
 * \param[in]       rule is the trigger rule number.
 * 
 * \param[in,out]   trigEntry points to the trigger entry to work on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirroring action
 *                  specifies an invalid mirrorProfile.
 *
 *****************************************************************************/
static fm_status HandleLogAction(fm_int  sw,
                                 fm_int  group,
                                 fm_int  rule,
                                 fm10000_triggerEntry * trigEntry)
{
    fm_uint32               logIndex;
    fm_uint32               handle;
    fm_status               err;
    fm_bool                 haveLogIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d group=%d rule=%d\n",
                 sw, group, rule);

    handle = 0;
    haveLogIndex = FALSE;

    /* We don't need to do anything if this is not a logging trigger. */
    if (trigEntry->action->cfg.trapAction != FM_TRIGGER_TRAP_ACTION_LOG)
    {
        err = FM_OK;
        goto ABORT;
    }

    /* We don't need to do anything if we have already defined a log
     * action mirror profile for this trigger. */
    if (trigEntry->logProfile >= 0)
    {
        err = FM_OK;
        goto ABORT;
    }

    /* The LOG action implicitly uses the mirror profile table entry
     * with the same hardware index as the trigger. */
    logIndex = trigEntry->index;

    /* Reserve the hardware mirror profile. */
    err = ReserveProfileIndex(sw, logIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    haveLogIndex = TRUE;

    /* Create a profile entry for the logging action. */
    err = CreateLogProfile(sw, logIndex, &handle);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    trigEntry->logProfile = handle;
    trigEntry->isBound = TRUE;

ABORT:
    if (err != FM_OK)
    {
        if (haveLogIndex)
        {
            FreeProfileIndex(sw, logIndex);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end HandleLogAction */




/*****************************************************************************/
/** HandleMirrorAction
 * \ingroup intTriggerInt
 *
 * \desc            Associates a mirror action profile with a trigger.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       group is the trigger group number.
 * 
 * \param[in]       rule is the trigger rule number.
 * 
 * \param[in,out]   trigEntry points to the trigger entry to work on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirroring action
 *                  specifies an invalid mirrorProfile.
 *
 *****************************************************************************/
static fm_status HandleMirrorAction(fm_int  sw,
                                    fm_int  group,
                                    fm_int  rule,
                                    fm10000_triggerEntry * trigEntry)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_uint32               handle;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d group=%d rule=%d\n",
                 sw, group, rule);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    trigEntry->mirrorIndex = 0;

    if (trigEntry->action->cfg.mirrorAction != FM_TRIGGER_MIRROR_ACTION_MIRROR)
    {
        err = FM_OK;
        goto ABORT;
    }

    handle = trigEntry->action->param.mirrorProfile;
    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR ||
        profEntry->index < 0)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    trigEntry->mirrorIndex = profEntry->index;

    err = FM_OK;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end HandleMirrorAction */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000InitMirrorProfiles
 * \ingroup intTriggerInt
 *
 * \desc            Initializes the mirror profiles.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitMirrorProfiles(fm_int sw)
{
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm10000_mirrorCfg       config;
    intrinsicMirrorDesc *   intrinsic;
    fm_uint32               handle;
    fm_uint32               mirrorIndex[NUM_INTRINSIC_MIRRORS];
    fm_uint32               cpuBaseGlort;
    fm_uint                 i;
    fm_uint32               value32;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    /**************************************************
     * Initialize the mirror profile structures.
     **************************************************/

    err = fmCreateBitArray(&trigInfo->usedProfileHandle,
                           FM10000_NUM_MIRROR_PROFILES);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->profileHandleInternal, 
                           FM10000_NUM_MIRROR_PROFILES);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->usedProfileIndex, 
                           FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    FM_MEMSET_S(&trigInfo->profileMap,
                sizeof(trigInfo->profileMap),
                -1,
                sizeof(trigInfo->profileMap));

    /**************************************************
     * Initialize the intrinsic mirror profiles.
     **************************************************/

    FM_CLEAR(config);

    err = fmMapLogicalPortToPhysical(switchPtr,
                                     switchPtr->cpuPort,
                                     &config.physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    cpuBaseGlort = switchPtr->glortInfo.cpuBase;

    for (i = 0 ; i < NUM_INTRINSIC_MIRRORS ; i++)
    {
        intrinsic = &intrinsicMirrors[i];

        /* Create mirror profile entry. */
        err = CreateProfileEntry(sw,
                                 FM10000_MIRROR_PROFILE_INTRINSIC,
                                 &handle,
                                 TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        profEntry = &trigInfo->profileEntry[handle];
        profEntry->intrinsicType = intrinsic->intrinsicType;

        /* Get hardware index for this mirror profile. */
        mirrorIndex[i] = profEntry->index;

        /* Mirror GLORT equals the CPU GLORT | Trap Code */
        config.glort = cpuBaseGlort | intrinsic->trapCode;

        err = WriteMirrorProfile(sw, mirrorIndex[i], &config);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    }   /* end for (i = 0 ; i < NUM_INTRINSIC_MIRRORS ; i++) */

    /**************************************************
     * Store the mirror profile identifiers in the
     * LOG_MIRROR_PROFILE register. 
     **************************************************/

    value32 = 0;

    FM_SET_FIELD(value32,
                 FM10000_LOG_MIRROR_PROFILE,
                 FFU,
                 mirrorIndex[0]);

    FM_SET_FIELD(value32,
                 FM10000_LOG_MIRROR_PROFILE,
                 ReservedMAC,
                 mirrorIndex[1]);

    FM_SET_FIELD(value32,
                 FM10000_LOG_MIRROR_PROFILE,
                 ARPRedirect,
                 mirrorIndex[2]);

    FM_SET_FIELD(value32,
                 FM10000_LOG_MIRROR_PROFILE,
                 ICMP,
                 mirrorIndex[3]);

    FM_SET_FIELD(value32,
                 FM10000_LOG_MIRROR_PROFILE,
                 TTL,
                 mirrorIndex[4]);

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_LOG_MIRROR_PROFILE(),
                                 value32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000InitMirrorProfiles */




/*****************************************************************************/
/** fm10000ConvertMirrorIndexToProfile
 * \ingroup intTriggerInt
 *
 * \desc            Converts a hardware mirror profile index to a mirror
 *                  profile handle.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       mirrorIndex is the hardware mirror profile index.
 * 
 * \param[out]      mirrorProfile points to the location to receive the
 *                  mirror profile handle. Will be set to zero if the
 *                  conversion is unsuccessful.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware index
 *                  is invalid.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is
 *                  invalid or the profile is the wrong type.
 *
 *****************************************************************************/
fm_status fm10000ConvertMirrorIndexToProfile(fm_int      sw,
                                             fm_uint32   mirrorIndex,
                                             fm_uint32 * mirrorProfile)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_int                  handle;

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    *mirrorProfile = 0;

    /* Parameter must be a valid mirror profile index. */
    if (mirrorIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        return FM_ERR_INVALID_PROFILE_INDEX;
    }

    /* Mirror profile map must contain a valid entry. */
    handle = trigInfo->profileMap[mirrorIndex];
    if (handle < 0)
    {
        return FM_ERR_INVALID_PROFILE_INDEX;
    }

    /* Mirror profile entry must be for MIRROR action. */
    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR)
    {
        return FM_ERR_INVALID_MIRROR_PROFILE;
    }

    /* Hardware index must match profile index. */
    if (profEntry->index != (fm_int) mirrorIndex)
    {
        return FM_ERR_INVALID_PROFILE_INDEX;
    }

    /* Return the mirror profile handle. */
    *mirrorProfile = handle;

    return FM_OK;

}   /* end fm10000ConvertMirrorIndexToProfile */




/*****************************************************************************/
/** fm10000ConvertMirrorProfileToIndex
 * \ingroup intTriggerInt
 *
 * \desc            Converts a mirror profile handle to a hardware
 *                  mirror profile index.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       mirrorProfile is the mirror profile handle.
 * 
 * \param[out]      mirrorIndex points to the location to receive the
 *                  hardware mirror profile index. Will be set to zero
 *                  if the conversion is unsuccessful
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is
 *                  invalid or the profile is the wrong type.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware index
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fm10000ConvertMirrorProfileToIndex(fm_int      sw,
                                             fm_uint32   mirrorProfile,
                                             fm_uint32 * mirrorIndex)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    *mirrorIndex = 0;

    /* Parameter must be a valid mirror profile handle. */
    if (mirrorProfile >= FM10000_NUM_MIRROR_PROFILES)
    {
        return FM_ERR_INVALID_MIRROR_PROFILE;
    }

    /* Mirror profile entry must be for MIRROR action. */
    profEntry = &trigInfo->profileEntry[mirrorProfile];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR)
    {
        return FM_ERR_INVALID_MIRROR_PROFILE;
    }

    /* Hardware index must have been assigned. */
    if (profEntry->index == -1)
    {
        return FM_ERR_INVALID_PROFILE_INDEX;
    }

    /* Return the hardware profile index. */
    *mirrorIndex = profEntry->index;

    return FM_OK;

}   /* end fm10000ConvertMirrorProfileToIndex */




/*****************************************************************************/
/** fm10000CopyMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Copies a hardware mirror profile to a new location.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       srcIndex is the hardware profile index of the source
 *                  entry.
 * 
 * \param[in]       dstIndex is the hardware profile index of the
 *                  destination entry.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
fm_status fm10000CopyMirrorProfile(fm_int sw,
                                   fm_int srcIndex,
                                   fm_int dstIndex)
{
    fm_switch * switchPtr;
    fm_status   err;
    fm_uint32   regMirrorFh;
    fm_uint64   regMirrorMod;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d srcIndex=%d dstIndex=%d\n",
                 sw,
                 srcIndex,
                 dstIndex);

    switchPtr = GET_SWITCH_PTR(sw);

    if (srcIndex < 0 || srcIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES ||
        dstIndex < 0 || dstIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    /**************************************************
     * Read mirror profile.
     **************************************************/

    err = switchPtr->ReadUINT32(sw,
                                FM10000_FH_MIRROR_PROFILE_TABLE(srcIndex),
                                &regMirrorFh);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmRegCacheReadUINT64(sw,
                               &fm10000CacheModMirrorProfTable,
                               srcIndex,
                               &regMirrorMod);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);


    /**************************************************
     * Write mirror profile.
     **************************************************/

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_FH_MIRROR_PROFILE_TABLE(dstIndex),
                                 regMirrorFh);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmRegCacheWriteUINT64(sw,
                                &fm10000CacheModMirrorProfTable,
                                dstIndex,
                                regMirrorMod);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000CopyMirrorProfile */




/*****************************************************************************/
/** fm10000CreateMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Creates a mirror profile entry.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      handle points to the location to receive the handle
 *                  of the mirror profile entry.
 * 
 * \param[in]       isInternal is TRUE if the resource is being reserved
 *                  internally by the API.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free profile
 *                  handles or mirror profile indexes.
 *
 *****************************************************************************/
fm_status fm10000CreateMirrorProfile(fm_int      sw,
                                     fm_uint32 * handle,
                                     fm_bool     isInternal)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%p\n", sw, (void *)handle);

    err = CreateProfileEntry(sw,
                             FM10000_MIRROR_PROFILE_MIRROR,
                             handle,
                             isInternal);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000CreateMirrorProfile */




/*****************************************************************************/
/** fm10000DeleteLogProfile
 * \ingroup intTriggerInt
 *
 * \desc            Deletes a log action profile entry.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the mirror profile handle.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteLogProfile(fm_int sw, fm_uint32 handle)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    err = DeleteProfileEntry(sw, FM10000_MIRROR_PROFILE_LOG, handle, TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000DeleteLogProfile */




/*****************************************************************************/
/** fm10000DeleteMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Deletes a mirror action profile entry.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       isInternal is TRUE if this is an internal resource.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteMirrorProfile(fm_int    sw,
                                     fm_uint32 handle,
                                     fm_bool   isInternal)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    err = DeleteProfileEntry(sw,
                             FM10000_MIRROR_PROFILE_MIRROR,
                             handle,
                             isInternal);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000DeleteMirrorProfile */




/*****************************************************************************/
/** fm10000FreeProfileIndex
 * \ingroup intTriggerInt
 *
 * \desc            Deallocates a hardware profile index.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       index is the hardware profile index to be freed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
fm_status fm10000FreeProfileIndex(fm_int sw, fm_uint32 index)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%u\n", sw, index);

    err = FreeProfileIndex(sw, index);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000FreeProfileIndex */




/*****************************************************************************/
/** fm10000ReserveMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Reserves a mirror profile index. If the specified
 *                  profile is in use, it will be relocated.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the hardware profile index to be reserved.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no free hardware
 *                  profile indexes.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if a mirror profile handle
 *                  is invalid or the profile is the wrong type.
 *
 *****************************************************************************/
fm_status fm10000ReserveMirrorProfile(fm_int sw, fm_uint32 index)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%u\n", sw, index);

    err = ReserveProfileIndex(sw, index);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000ReserveMirrorProfile */




/*****************************************************************************/
/** fm10000UpdateLogProfile
 * \ingroup intTriggerInt
 *
 * \desc            Updates a log action mirror profile.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       logIndex is the hardware profile index.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirror profile handle
 *                  is invalid or the profile is the wrong type.
 *
 *****************************************************************************/
fm_status fm10000UpdateLogProfile(fm_int    sw,
                                  fm_uint32 handle,
                                  fm_uint32 logIndex)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d handle=%u logIndex=%u\n",
                 sw,
                 handle,
                 logIndex);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (logIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_LOG)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry->index = logIndex;

    trigInfo->profileMap[logIndex] = handle;

    err = FM_OK;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000UpdateLogProfile */




/*****************************************************************************/
/** fm10000ReleaseMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Frees a mirror profile following a move.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       index is the hardware profile index.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware index
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fm10000ReleaseMirrorProfile(fm_int sw, fm_uint32 index)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d index=%d\n", sw, index);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (index >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    trigInfo->profileMap[index] = -1;

    err = ResetMirrorProfile(sw, index);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = FreeProfileIndex(sw, index);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000ReleaseMirrorProfile */




/*****************************************************************************/
/** fm10000SetMirrorProfileAction
 * \ingroup intTriggerInt
 *
 * \desc            Called by fm10000SetTriggerAction to implement the
 *                  log and mirror actions of a trigger.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to be holding the trigger resource lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       group is the trigger group number.
 * 
 * \param[in]       rule is the trigger rule number.
 * 
 * \param[in,out]   trigEntry points to the trigger entry to work on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirroring action
 *                  specifies an invalid mirrorProfile.
 *
 *****************************************************************************/
fm_status fm10000SetMirrorProfileAction(fm_int  sw,
                                        fm_int  group,
                                        fm_int  rule,
                                        fm10000_triggerEntry * trigEntry)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d group=%d rule=%d\n",
                 sw, group, rule);

    /* Set up for logging action. */
    err = HandleLogAction(sw, group, rule, trigEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Set up for mirror action. */
    err = HandleMirrorAction(sw, group, rule, trigEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000SetMirrorProfileAction */




/*****************************************************************************/
/** fm10000GetMirrorProfileConfig
 * \ingroup intTriggerInt
 *
 * \desc            Returns the properties of a mirror profile.
 * 
 * \note            The caller is assumed to be holding the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[out]      config points to the structure to receive the
 *                  mirror profile configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is
 *                  invalid or the profile is the wrong type.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware index
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMirrorProfileConfig(fm_int              sw,
                                        fm_uint32           handle,
                                        fm10000_mirrorCfg * config)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    TAKE_TRIGGER_LOCK(sw);

    if (config == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    err = ReadMirrorProfile(sw, profEntry->index, config);

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetMirrorProfileConfig */




/*****************************************************************************/
/** fm10000SetMirrorProfileConfig
 * \ingroup intTriggerInt
 *
 * \desc            Sets the properties of a mirror profile.
 * 
 * \note            The caller is assumed to be holding the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       config points to the mirror profile configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is
 *                  invalid or the profile is the wrong type.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware index
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetMirrorProfileConfig(fm_int              sw,
                                        fm_uint32           handle,
                                        fm10000_mirrorCfg * config)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    TAKE_TRIGGER_LOCK(sw);

    if (config == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    if (profEntry->entryType != FM10000_MIRROR_PROFILE_MIRROR)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    err = WriteMirrorProfile(sw, profEntry->index, config);

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000SetMirrorProfileConfig */




/*****************************************************************************/
/** fm10000DbgGetMirrorProfileDetail
 * \ingroup intDiag
 * 
 * \desc            Returns internal information about a mirror profile.
 * 
 * \note            The caller is assumed to be holding the switch lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       detail points to the structure to be filled in by
 *                  this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle is invalid.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
fm_status fm10000DbgGetMirrorProfileDetail(fm_int                   sw,
                                           fm_uint32                handle,
                                           fm_mirrorProfileDetail * detail)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm10000_mirrorCfg       config;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "sw=%d handle=%u\n", sw, handle);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (detail == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    FM_CLEAR(*detail);

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];

    detail->profileIndex    = profEntry->index;
    detail->entryType       = profEntry->entryType;
    detail->intrinsicType   = profEntry->intrinsicType;

    err = ReadMirrorProfile(sw, profEntry->index, &config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    detail->physPort        = config.physPort;
    detail->glort           = config.glort;

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000DbgGetMirrorProfileDetail */




/*****************************************************************************/
/** fm10000DbgGetTriggerDetail
 * \ingroup intDiag
 *
 * \desc            Returns internal information about a trigger.
 * 
 * \note            The caller is assumed to be holding the switch lock.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       group is the trigger group number.
 * 
 * \param[in]       rule is the trigger rule number.
 * 
 * \param[in]       detail points to the structure to be filled in by
 *                  this function.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgGetTriggerDetail(fm_int             sw,
                                     fm_int             group,
                                     fm_int             rule,
                                     fm_triggerDetail * detail)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_triggerEntry *  trigEntry;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d group=%d rule=%d\n",
                 sw,
                 group,
                 rule);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (detail == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    FM_CLEAR(*detail);

    err = fmTreeFind(&trigInfo->triggerTree, 
                     FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                     (void **) &trigEntry);
    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        goto ABORT;
    }

    detail->triggerIndex = trigEntry->index;
    detail->mirrorIndex  = trigEntry->mirrorIndex;
    detail->logProfile   = trigEntry->logProfile;

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000DbgGetTriggerDetail */




/*****************************************************************************/
/** fm10000DbgDumpMirrorProfile
 * \ingroup intDiag
 *
 * \desc            Dumps the mirror profile tables.
 * 
 * \note            Called through the DbgDumpMirrorProfile function pointer.
 *                  The caller is assumed to be holding the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpMirrorProfile(fm_int sw)
{
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm10000_mirrorCfg       config;
    fm_int                  profileId;
    fm_int                  entryId;
    fm_uint32               reg32;
    fm_text                 entryType;
    fm_char                 detailBuf[32];
    fm_status               err;

    const fm_text logMirrorFmt = "  %-12s : %2d\n";

    TAKE_TRIGGER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    trigInfo  = &switchExt->triggerInfo;

#if 0
    /**************************************************
     * Dump mirror profiles by handle.
     **************************************************/

    FM_LOG_PRINT("\nMIRROR PROFILE HANDLES\n\n");
    FM_LOG_PRINT("Handle  EntryType  Index\n");
    FM_LOG_PRINT("------  ---------  -----\n");

    for (entryId = 0 ; entryId < FM10000_NUM_MIRROR_PROFILES ; ++entryId)
    {
        err = fmFindBitInBitArray(&trigInfo->usedProfileHandle,
                                  entryId,
                                  TRUE,
                                  &entryId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (entryId >= FM10000_NUM_MIRROR_PROFILES || entryId < 0)
        {
            break;
        }

        profEntry = &trigInfo->profileEntry[entryId];

        FM_LOG_PRINT("%4d    %-9s  %4d \n",
                     entryId,
                     ProfileTypeToText(profEntry->entryType),
                     profEntry->index);
    }
#endif

    /**************************************************
     * Dump mirror profiles by index.
     **************************************************/

    FM_LOG_PRINT("\nMIRROR_PROFILE_TABLE\n\n");
    FM_LOG_PRINT("Index  PhysPort  Glort  Vlan  Vpri  Trunc  Handle  EntryType  Detail\n");
    FM_LOG_PRINT("-----  --------  -----  ----  ----  -----  ------  ---------  --------\n");

    for (profileId = 0 ; ; ++profileId)
    {
        err = fmFindBitInBitArray(&trigInfo->usedProfileIndex,
                                  profileId,
                                  TRUE,
                                  &profileId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (profileId >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES ||
            profileId < 0)
        {
            break;
        }

        entryType = "";
        detailBuf[0] = 0;

        entryId = trigInfo->profileMap[profileId];

        if (entryId >= 0)
        {
            profEntry = &trigInfo->profileEntry[entryId];
            entryType = ProfileTypeToText(profEntry->entryType);
            if (profEntry->entryType == FM10000_MIRROR_PROFILE_INTRINSIC)
            {
                FM_SNPRINTF_S(detailBuf,
                              sizeof(detailBuf),
                              "%s",
                              IntrinsicTypeToText(profEntry->intrinsicType));
            }
        }

        err = ReadMirrorProfile(sw, profileId, &config);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        FM_LOG_PRINT(" %3d   %5d     %04x   %4d  %3d   %-5s  %4d    %-9s  %s\n",
                     profileId,
                     config.physPort,
                     config.glort,
                     config.vlan,
                     config.vlanPri,
                     FM_BOOLSTRING(config.truncate),
                     entryId,
                     entryType,
                     detailBuf);

    }   /* end for (profileId = 0 ; ; ++profileId) */

    /**************************************************
     * Dump LOG_MIRROR_PROFILE.
     **************************************************/

    FM_LOG_PRINT("\nLOG_MIRROR_PROFILE:\n");

    err = switchPtr->ReadUINT32(sw,
                                FM10000_LOG_MIRROR_PROFILE(),
                                &reg32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    FM_LOG_PRINT(logMirrorFmt,
                 "FFU",
                 FM_GET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, FFU));

    FM_LOG_PRINT(logMirrorFmt,
                 "ReservedMAC",
                 FM_GET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, ReservedMAC));

    FM_LOG_PRINT(logMirrorFmt,
                 "ARPRedirect",
                 FM_GET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, ARPRedirect));

    FM_LOG_PRINT(logMirrorFmt,
                 "ICMP",
                 FM_GET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, ICMP));

    FM_LOG_PRINT(logMirrorFmt,
                 "TTL",
                 FM_GET_FIELD(reg32, FM10000_LOG_MIRROR_PROFILE, TTL));

ABORT:
    DROP_TRIGGER_LOCK(sw);

    return FM_OK;

}   /* end fm10000DbgDumpMirrorProfile */




/*****************************************************************************/
/** fmDbgMoveMirrorProfile
 * \ingroup intDiag
 * 
 * \chips           FM10000
 *
 * \desc            Moves a mirror profile to a new hardware location.
 * 
 * \note            Standalone function. For internal use only.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       handle is the mirror profile handle.
 * 
 * \param[in]       dstIndex is the destination hardware profile index.
 *                  May be -1, in which case the function will allocate
 *                  the profile index itself.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the handle or the
 *                  profile it references is invalid.
 * \return          FM_ERR_INVALID_PROFILE_INDEX if the hardware profile
 *                  index is invalid.
 *
 *****************************************************************************/
fm_status fmDbgMoveMirrorProfile(fm_int sw, fm_uint32 handle, fm_int dstIndex)
{
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm10000_mirrorProfile * profEntry;
    fm_uint32               oldIndex;
    fm_uint32               newIndex;
    fm_bool                 inUse;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d handle=%u dstIndex=%d\n",
                 sw,
                 handle,
                 dstIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (handle >= FM10000_NUM_MIRROR_PROFILES)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    profEntry = &trigInfo->profileEntry[handle];
    switch (profEntry->entryType)
    {
        case FM10000_MIRROR_PROFILE_INTRINSIC:
        case FM10000_MIRROR_PROFILE_MIRROR:
            break;

        default:
            err = FM_ERR_INVALID_MIRROR_PROFILE;
            goto ABORT;
    }

    if (profEntry->index < 0)
    {
        err = FM_ERR_INVALID_MIRROR_PROFILE;
        goto ABORT;
    }

    if (dstIndex >= FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES)
    {
        err = FM_ERR_INVALID_PROFILE_INDEX;
        goto ABORT;
    }
    else if (dstIndex < 0)
    {
        err = AllocProfileIndex(sw, &newIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }
    else
    {
        err = fmGetBitArrayBit(&trigInfo->usedProfileIndex, dstIndex, &inUse);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (inUse)
        {
            FM_LOG_ERROR(FM_LOG_CAT_TRIGGER,
                         "Profile index %d already in use!\n",
                         dstIndex);
            err = FM_ERR_INVALID_PROFILE_INDEX;
            goto ABORT;
        }

        FM_LOG_PRINT("Reserving profile index %d\n", dstIndex);

        err = fmSetBitArrayBit(&trigInfo->usedProfileIndex, dstIndex, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        newIndex = dstIndex;
    }

    oldIndex = profEntry->index;

    FM_LOG_PRINT("Moving %s profile %u from %u to %u\n",
                 ProfileTypeToText(profEntry->entryType),
                 handle,
                 oldIndex,
                 newIndex);

    err = MoveProfileEntry(sw, handle, newIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = FreeProfileIndex(sw, oldIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    DROP_TRIGGER_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmDbgMoveMirrorProfile */

