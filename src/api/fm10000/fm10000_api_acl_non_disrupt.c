/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_acl_non_disrupt.c
 * Creation Date:  July 3, 2013
 * Description:    ACL code related to the non-disruptive part of the compiler
 *                 for FM10000.
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
#include <common/fm_version.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


extern const fm_byte fmAbstractToConcrete8bits[][4];
extern const fm_byte fmAbstractToConcrete4bits[][10];

/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

extern void fmFreeCompiledAclRule(void *value);
extern void fmFreeCompiledAcl(void *value);
extern void fmFreeCompiledAclInstance(void *value);
extern void fmFreeAclPortSet(void *value);
extern void fmFreeEcmpGroup(void *value);
extern void fmFreeCompiledPolicerEntry(void *value);

extern fm_status fmAddAbstractKey(fm_tree * abstractKey,
                                  fm_byte   firstAbstract,
                                  fm_byte   lastAbstract,
                                  fm_byte   bitsPerKey,
                                  fm_uint64 mask,
                                  fm_uint64 value);
extern fm_status fmAddIpAbstractKey(fm_tree * abstractKey,
                                    fm_byte   firstAbstract,
                                    fm_ipAddr mask,
                                    fm_ipAddr value);
extern fm_status fmAddDeepInsAbstractKey(fm_tree * abstractKey,
                                         fm_byte * abstractTable,
                                         fm_byte   tableSize,
                                         fm_byte * mask,
                                         fm_byte * value);

extern fm_status fmFillAbstractPortSetKeyTree(fm_aclErrorReporter *      errReport,
                                              fm_aclRule *               rule,
                                              fm_fm10000CompiledAclRule *compiledAclRule,
                                              fm_tree *                  abstractKey,
                                              fm_tree *                  portSetId);
extern fm_status fmFillAbstractKeyTree(fm_int               sw,
                                       fm_aclErrorReporter *errReport,
                                       fm_aclRule *         rule,
                                       fm_tree *            abstractKey,
                                       fm_tree *            portSetId);

extern fm_int fmCountConditionSliceUsage(fm_byte *muxSelect);
extern void fmInitializeConcreteKey(fm_byte *muxSelect);
extern fm_status fmConvertAbstractToConcreteKey(fm_tree *      abstractKeyTree,
                                                fm_byte *      muxSelect,
                                                fm_uint16 *    muxUsed);

extern fm_status fmCountActionSlicesNeeded(fm_int               sw,
                                           fm_aclErrorReporter *errReport,
                                           fm_aclRule *         rule,
                                           fm_int *             actionSlices);

extern void fmTranslateAclScenario(fm_int      sw,
                                   fm_uint32   aclScenario,
                                   fm_uint32  *validScenarios,
                                   fm_bool     egressAcl);

extern fm_status fmConfigureConditionKey(fm_int                         sw,
                                         fm_aclErrorReporter *          errReport,
                                         fm_aclRule *                   rule,
                                         fm_int                         ruleNumber,
                                         fm_fm10000CompiledAcl *        compiledAcl,
                                         fm_fm10000CompiledAclInstance *compiledAclInst);
extern fm_status fmConfigureActionData(fm_int                      sw,
                                       fm_aclErrorReporter *       errReport,
                                       fm_fm10000CompiledPolicers *policers,
                                       fm_tree *                   ecmpGroups,
                                       fm_aclRule *                rule,
                                       fm_fm10000CompiledAcl *     compiledAcl,
                                       fm_fm10000CompiledAclRule * compiledAclRule);
extern fm_status fmConfigureEgressActionData(fm_aclRule *               rule,
                                             fm_aclErrorReporter *      errReport,
                                             fm_fm10000CompiledAclRule *compiledAclRule);
extern fm_status fmConvertAclPortToPortSet(fm_int                 sw,
                                           fm_tree *              globalPortSet,
                                           fm_fm10000CompiledAcl *compiledAcl,
                                           fm_int                 mappedValue,
                                           fm_uint64              key);

extern void fmUpdateValidSlice(fm_ffuSliceInfo*        sliceInfo,
                               fm_fm10000CompiledAcls *cacls);
extern fm_status fmUpdateMasterValid(fm_int sw, fm_fm10000CompiledAcls *cacls);
extern fm_status fmUpdateBstMasterValid(fm_int sw, fm_fm10000CompiledAcls *cacls);
extern fm_status fmApplyMapSrcPortId(fm_int    sw,
                                     fm_tree * portSetIdTree);
extern void fmInitializeMuxSelect(fm_byte *srcArray,
                                  fm_byte *dstArray);

extern fm_status fmSetEaclChunkCfg(fm_int    sw,
                                   fm_int    chunk,
                                   fm_uint32 scenarios,
                                   fm_bool   cascade,
                                   fm_tree * portTree);
extern fm_status fmResetFFUSlice(fm_int sw, fm_ffuSliceInfo *sliceInfo);
extern fm_status fmResetBSTSlice(fm_int sw, fm_ffuSliceInfo *sliceInfo);

extern fm_status fmGetSlicePosition(fm_int           sw,
                                    fm_ffuSliceInfo *sliceInfo,
                                    fm_int           nextFreeConditionSlice,
                                    fm_int           nextFreeActionSlice,
                                    fm_int *         conditionSlicePos,
                                    fm_int *         actionSlicePos);
extern fm_status fmGetAclNumKey(fm_tree *  aclTree,
                                fm_int     acl,
                                fm_int     rule,
                                fm_uint64 *aclNumKey);
extern fm_status fmGetFFUSliceRange(fm_int sw, fm_int *firstSlice, fm_int *lastSlice);

fm_status fm10000NonDisruptRemoveEgressAclHole(fm_int                  sw,
                                               fm_fm10000CompiledAcls *cacls,
                                               fm_bool                 apply);
fm_status fm10000NonDisruptRemoveIngAclHole(fm_int                  sw,
                                            fm_fm10000CompiledAcls *cacls,
                                            fm_bool                 apply);
fm_status fm10000NonDisruptPackEgressAcl(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_bool                 apply);
fm_status fm10000NonDisruptPackIngAcl(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_fm10000CompiledAcl * compiledAcl,
                                      fm_bool                 apply);
fm_status fm10000NonDisruptPackGrpIngAcl(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_int                  acl,
                                         fm_bool                 apply);
fm_status fm10000NonDisruptCutEgressSlices(fm_int                  sw,
                                           fm_fm10000CompiledAcls *cacls,
                                           fm_uint32               cutSliceNum,
                                           fm_bool                 apply);
fm_status fm10000NonDisruptCutIngSlices(fm_int                  sw,
                                        fm_fm10000CompiledAcls *cacls,
                                        fm_fm10000CompiledAcl * compiledAcl,
                                        fm_int                  cutSliceNum,
                                        fm_bool                 apply);
fm_status fm10000NonDisruptRemEgressAclUnusedKey(fm_int    sw,
                                                 fm_tree * egressAcl,
                                                 fm_bool   apply);
fm_status fm10000NonDisruptRemIngAclUnusedKey(fm_int                  sw,
                                              fm_fm10000CompiledAcls *cacls,
                                              fm_int                  acl,
                                              fm_bool                 apply);
fm_status fm10000NonDisruptShiftEgressAcl(fm_int                  sw,
                                          fm_fm10000CompiledAcls *cacls,
                                          fm_fm10000CompiledAcl * compiledAcl,
                                          fm_bool                 down,
                                          fm_bool                 apply);
fm_status fm10000NonDisruptMoveEgressAcl(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_fm10000CompiledAcl * compiledAcl,
                                         fm_int                  newPos,
                                         fm_bool                 apply);
fm_status fm10000NonDisruptMoveIngAcl(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_fm10000CompiledAcl * compiledAcl,
                                      fm_int                  newPos,
                                      fm_bool                 apply);
fm_status fm10000NonDisruptAddEgressAclHole(fm_int                  sw,
                                            fm_fm10000CompiledAcls *cacls,
                                            fm_fm10000CompiledAcl * compiledAcl,
                                            fm_bool                 apply);
fm_status fm10000NonDisruptAddIngAclHole(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_fm10000CompiledAcl * compiledAcl,
                                         fm_bool                 apply);
fm_status fm10000NonDisruptRemEgressAcl(fm_int                  sw,
                                        fm_fm10000CompiledAcls *cacls,
                                        fm_int                  acl,
                                        fm_bool                 apply);
fm_status fm10000NonDisruptRemIngAcl(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_uint64               acl,
                                     fm_bool                 apply);
fm_status fm10000NonDisruptRemEgressAclRule(fm_int                 sw,
                                            fm_fm10000CompiledAcl *compiledAcl,
                                            fm_int                 rule,
                                            fm_bool                apply);
fm_status fm10000NonDisruptRemIngAclRule(fm_int                 sw,
                                         fm_fm10000CompiledAcl *compiledAcl,
                                         fm_int                 rule,
                                         fm_bool                apply);
fm_status fm10000NonDisruptRemAcls(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls,
                                   fm_int                  internalAcl,
                                   fm_bool                 apply);
fm_status fm10000NonDisruptRemMappers(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_bool                 apply);
fm_status fm10000NonDisruptCleanRoutes(fm_fm10000CompiledAcls *cacls,
                                       fm_int                  acl,
                                       fm_int                  rule,
                                       fm_int                  ecmpGrp);
fm_status fm10000NonDisruptAddMappers(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_bool                 apply);
fm_status fm10000NonDisruptAddEgressAclRule(fm_int                  sw,
                                            fm_fm10000CompiledAcls *cacls,
                                            fm_fm10000CompiledAcl * compiledAcl,
                                            fm_aclRule *            rule,
                                            fm_int                  ruleNumber,
                                            fm_bool                 apply);
fm_status fm10000NonDisruptAddSelect(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_fm10000CompiledAcl * compiledAcl,
                                     fm_aclRule *            rule,
                                     fm_bool                 apply);
fm_status fm10000NonDisruptAddIngAclRule(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_int                  aclNumber,
                                         fm_aclRule *            rule,
                                         fm_int                  ruleNumber,
                                         fm_bool                 apply);
fm_status fm10000NonDisruptAddEgressAcl(fm_int                  sw,
                                        fm_fm10000CompiledAcls *cacls,
                                        fm_acl *                acl,
                                        fm_int                  aclNumber,
                                        fm_bool                 apply);
fm_status fm10000NonDisruptAddIngAcl(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_acl *                acl,
                                     fm_int                  aclNumber,
                                     fm_bool                 apply);
fm_status fm10000NonDisruptAddAcls(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls,
                                   fm_int                  internalAcl,
                                   fm_bool                 apply);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** UpdateScenarioCfg
 * \ingroup intAcl
 *
 * \desc            Manually update the StartCompare or the StartAction field
 *                  of the FFU_SLICE_CFG register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice refer to the slices to update.
 * 
 * \param[in]       validScenarios refers to the scenarios to update.
 *
 * \param[in]       startCompare is the value to configure or -1 to let the
 *                  actual configured value.
 *
 * \param[in]       startAction is the value to configure or -1 to let the
 *                  actual configured value.
 *
 * \param[in]       actionLength is the length to set if startAction does not
 *                  equal -1.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateScenarioCfg(fm_int    sw,
                                   fm_int    slice,
                                   fm_uint32 validScenarios,
                                   fm_int    startCompare,
                                   fm_int    startAction,
                                   fm_int    actionLength)
{
    fm_status err = FM_OK;
    fm_int i;
    fm_registerSGListEntry sgList[FM10000_FFU_SLICE_CFG_ENTRIES_0 + 1];
    fm_int sgIndex = 0;
    fm_uint32 data[(FM10000_FFU_SLICE_CASCADE_ACTION_ENTRIES * FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH) +
                   (FM10000_FFU_SLICE_CFG_WIDTH * FM10000_FFU_SLICE_CFG_ENTRIES_0)];
    fm_uint32 *dataPtr = data;
    fm_bool    regLockTaken = FALSE;

    if (actionLength)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheFfuSliceCascadeAction,
                                  actionLength,
                                  slice,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr+= (FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH * actionLength);
        sgIndex++;
    }

    if ( (startCompare != -1) || (startAction != -1) )
    {
        for (i = 0 ; i < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; i++)
        {
            if (validScenarios & (1 << i))
            {
                FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                          &fm10000CacheFfuSliceCfg,
                                          1,
                                          i,
                                          slice,
                                          FM_REGS_CACHE_INDEX_UNUSED,
                                          dataPtr,
                                          FALSE);
                sgIndex++;
                dataPtr += FM10000_FFU_SLICE_CFG_WIDTH;
            }
        }
    }

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, sgIndex, sgList, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    sgIndex = 0;
    if (actionLength)
    {
        for (i = 0 ; i < actionLength ; i++)
        {
            /* FFU_SLICE_CASCADE_ACTION */
            dataPtr = sgList[sgIndex].data + (i * FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH);
            if (startAction == 0)
            {
                *dataPtr &= ~validScenarios;
            }
            else
            {
                *dataPtr |= validScenarios;
            }
        }
        sgIndex++;
    }

    if ( (startCompare != -1) || (startAction != -1) )
    {
        for (i = 0 ; i < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; i++)
        {
            if (validScenarios & (1 << i))
            {
                dataPtr   = sgList[sgIndex++].data; /* FFU_SLICE_CFG */

                if (startCompare != -1)
                {
                    FM_ARRAY_SET_BIT(dataPtr,
                                     FM10000_FFU_SLICE_CFG,
                                     StartCompare,
                                     startCompare);
                }

                if (startAction != -1)
                {
                    FM_ARRAY_SET_BIT(dataPtr,
                                     FM10000_FFU_SLICE_CFG,
                                     StartAction,
                                     startAction);
                }
            }
        }
    }

    err = fmRegCacheWrite(sw, sgIndex, sgList, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end UpdateScenarioCfg */


/*****************************************************************************/
/** IsMuxSelectPositionValid
 * \ingroup intAcl
 *
 * \desc            This function look for the possibility to use a specific
 *                  mux position for an arbitrary mux configuration.
 *
 * \param[in]       byteMux is the mux configuration to validate
 *
 * \param[in]       srcPos is the position to validate
 *
 * \param[in]       dstPos is the position to validate
 *
 * \return          TRUE if the new position if possible.
 *                  FALSE if not.
 *
 *****************************************************************************/
static fm_bool IsMuxSelectPositionValid(fm_byte byteMux,
                                        fm_int  srcPos,
                                        fm_int  dstPos)
{
    fm_int i;

    if ( (srcPos < 4) &&
         (dstPos < 4) )
    {
        for (i = 0 ; i < FM10000_FIRST_4BITS_ABSTRACT_KEY ; i++)
        {
            if ( (fmAbstractToConcrete8bits[i][srcPos] == byteMux) &&
                 (fmAbstractToConcrete8bits[i][dstPos] == byteMux) )
            {
                return TRUE;
            }
        }

        for (i = FM10000_FIRST_4BITS_ABSTRACT_KEY ; i < FM10000_NUM_ABSTRACT ; i++)
        {
            if ( (fmAbstractToConcrete4bits[i][srcPos*2] == byteMux) &&
                 (fmAbstractToConcrete4bits[i][dstPos*2] == byteMux) )
            {
                return TRUE;
            }
        }
    }
    else
    {
        for (i = FM10000_FIRST_4BITS_ABSTRACT_KEY ; i < FM10000_NUM_ABSTRACT ; i++)
        {
            if ( (fmAbstractToConcrete4bits[i][srcPos*2] == byteMux) &&
                 (fmAbstractToConcrete4bits[i][dstPos*2] == byteMux) )
            {
                return TRUE;
            }
        }
    }

    return FALSE;

}   /* end IsMuxSelectPositionValid */


/*****************************************************************************/
/** NonDisruptMoveSelect
 * \ingroup intAcl
 *
 * \desc            This function move a key in a non disturbing way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       sliceSrc is the source slice position to move from.
 *
 * \param[in]       posSrc is the source key index to move from.
 *
 * \param[in]       sliceDst is the destination slice position to move to.
 *
 * \param[in]       posDst is the destination key index to move to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NonDisruptMoveSelect(fm_int                  sw,
                                      fm_fm10000CompiledAcl * compiledAcl,
                                      fm_int                  sliceSrc,
                                      fm_int                  posSrc,
                                      fm_int                  sliceDst,
                                      fm_int                  posDst,
                                      fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itRule;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_uint64 keyValue;
    fm_uint64 keyMaskValue;
    fm_bool ruleModified;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "sliceSrc = %d, "
                 "posSrc = %d, "
                 "sliceDst = %d, "
                 "posDst = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 sliceSrc,
                 posSrc,
                 sliceDst,
                 posDst,
                 apply);

    /* Update each rule to first hit on both key configuration then remove the
     * source key from the condition. */
    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
         (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                FM_OK ; )
    {
        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

        ruleModified = FALSE;
        if (posSrc < 4)
        {
            keyValue = (compiledAclRule->sliceKey[sliceSrc].key >> (posSrc * 8)) & 0xFF;
            keyMaskValue = (compiledAclRule->sliceKey[sliceSrc].keyMask >> (posSrc * 8)) & 0xFF;
            if (keyValue || keyMaskValue)
            {
                compiledAclRule->sliceKey[sliceDst].key |= (keyValue << (posDst * 8));
                compiledAclRule->sliceKey[sliceDst].keyMask |= (keyMaskValue << (posDst * 8));
                ruleModified = TRUE;
            }
        }
        else
        {
            keyValue = (compiledAclRule->sliceKey[sliceSrc].key >> 32) & 0xFF;
            keyMaskValue = (compiledAclRule->sliceKey[sliceSrc].keyMask >> 32) & 0xFF;
            if (keyValue || keyMaskValue)
            {
                compiledAclRule->sliceKey[sliceDst].key |= (keyValue << 32);
                compiledAclRule->sliceKey[sliceDst].keyMask |= (keyMaskValue << 32);
                ruleModified = TRUE;
            }
        }

        /* Only modify a rule if the moved key is used by this rule. */
        if (apply && ruleModified)
        {
            err = fm10000SetFFURule(sw,
                                    &compiledAcl->sliceInfo,
                                    compiledAclRule->physicalPos,
                                    compiledAclRule->valid,
                                    compiledAclRule->sliceKey,
                                    compiledAclRule->actions,
                                    FALSE,
                                    TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Remove the source condition from the rule. */
        if (ruleModified)
        {
            if (posSrc < 4)
            {
                compiledAclRule->sliceKey[sliceSrc].key &= ~(FM_LITERAL_64(0xFF) << (posSrc * 8));
                compiledAclRule->sliceKey[sliceSrc].keyMask &= ~(FM_LITERAL_64(0xFF) << (posSrc * 8));
            }
            else
            {
                compiledAclRule->sliceKey[sliceSrc].key &= ~FM_LITERAL_64(0xFF00000000);
                compiledAclRule->sliceKey[sliceSrc].keyMask &= ~FM_LITERAL_64(0xFF00000000);
            }
            if (apply)
            {
                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        compiledAclRule->physicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE,
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end NonDisruptMoveSelect */


/*****************************************************************************/
/** NonDisruptMoveEgressSelect
 * \ingroup intAcl
 *
 * \desc            This function move a select configuration from one position
 *                  to the other in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   egressAcl points to the egress acl tree that contain all
 *                  the egress compiled acl structure to update.
 *
 * \param[in]       sliceSrc is the source slice position to move from.
 *
 * \param[in]       posSrc is the source key index to move from.
 *
 * \param[in]       sliceDst is the destination slice position to move to.
 *
 * \param[in]       posDst is the destination key index to move to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NonDisruptMoveEgressSelect(fm_int    sw,
                                            fm_tree * egressAcl,
                                            fm_int    sliceSrc,
                                            fm_int    posSrc,
                                            fm_int    sliceDst,
                                            fm_int    posDst,
                                            fm_bool   apply)
{
    fm_status err = FM_OK;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;
    fm_byte srcByteMux;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    fm_uint32 validScenariosTmp;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "egressAcl = %p, "
                 "sliceSrc = %d, "
                 "posSrc = %d, "
                 "sliceDst = %d, "
                 "posDst = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) egressAcl,
                 sliceSrc,
                 posSrc,
                 sliceDst,
                 posDst,
                 apply);

    /* The first egress acl compiled structure is used to represent all the
     * egress acl slice configuration since all of them share the same pool of
     * condition slices. */
    fmTreeIterInit(&itAcl, egressAcl);
    err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    srcByteMux = compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceSrc) + posSrc];

    /* Activate the key on both source and destination position. */
    compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceDst) + posDst] = srcByteMux;
    if (apply)
    {
        fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = muxSelect;
        validScenariosTmp = compiledAcl->sliceInfo.validScenarios;
        compiledAcl->sliceInfo.validScenarios = 0xffffffff;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.validScenarios = validScenariosTmp;
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    for (fmTreeIterInit(&itAcl, egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl)) ==
                FM_OK ; )
    {
        /* Activate the key on both source and destination position. */
        compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceDst) + posDst] = srcByteMux;

        err = NonDisruptMoveSelect(sw,
                                   compiledAcl,
                                   sliceSrc,
                                   posSrc,
                                   sliceDst,
                                   posDst,
                                   apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Virtually Deactivate the source position */
        compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceSrc) + posSrc] = FM10000_UNUSED_KEY;
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

    if (apply)
    {
        /* The first egress acl compiled structure is used to represent all the
         * egress acl slice configuration since all of them share the same pool
         * of condition slices. */
        fmTreeIterInit(&itAcl, egressAcl);
        err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

        /* Remove the source key from the scenario configuration. */
        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = muxSelect;
        validScenariosTmp = compiledAcl->sliceInfo.validScenarios;
        compiledAcl->sliceInfo.validScenarios = 0xffffffff;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.validScenarios = validScenariosTmp;
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end NonDisruptMoveEgressSelect */


/*****************************************************************************/
/** NonDisruptMoveIngSelect
 * \ingroup intAcl
 *
 * \desc            This function move a select configuration from one position
 *                  to the other in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       sliceSrc is the source slice position to move from.
 *
 * \param[in]       posSrc is the source key index to move from.
 *
 * \param[in]       sliceDst is the destination slice position to move to.
 *
 * \param[in]       posDst is the destination key index to move to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NonDisruptMoveIngSelect(fm_int                  sw,
                                         fm_fm10000CompiledAcl * compiledAcl,
                                         fm_int                  sliceSrc,
                                         fm_int                  posSrc,
                                         fm_int                  sliceDst,
                                         fm_int                  posDst,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;
    fm_byte srcByteMux;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "sliceSrc = %d, "
                 "posSrc = %d, "
                 "sliceDst = %d, "
                 "posDst = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 sliceSrc,
                 posSrc,
                 sliceDst,
                 posDst,
                 apply);

    srcByteMux = compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceSrc) + posSrc];

    /* Activate the key on both source and destination position. */
    compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceDst) + posDst] = srcByteMux;
    if (apply)
    {
        fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = muxSelect;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = NonDisruptMoveSelect(sw,
                               compiledAcl,
                               sliceSrc,
                               posSrc,
                               sliceDst,
                               posDst,
                               apply);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Now that each rule refer to the new key position, remove the original
     * one. */
    compiledAcl->muxSelect[(FM_FFU_SELECTS_PER_MINSLICE * sliceSrc) + posSrc] = FM10000_UNUSED_KEY;
    if (apply)
    {
        fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = muxSelect;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end NonDisruptMoveIngSelect */


/*****************************************************************************/
/** NonDisruptValidateAndMoveEgress
 * \ingroup intAcl
 *
 * \desc            This function try to fill unused key to predefined position.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   egressAcl points to the egress acl tree that contain all
 *                  the egress compiled acl structure to update.
 *
 * \param[in]       sliceSrc is the source slice position to move from.
 *
 * \param[in]       sliceDst is the destination slice position to move to.
 *
 * \param[in]       posDst is the destination key index to move to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NonDisruptValidateAndMoveEgress(fm_int    sw,
                                                 fm_tree * egressAcl,
                                                 fm_int    sliceSrc,
                                                 fm_int    sliceDst,
                                                 fm_int    posDst,
                                                 fm_bool   apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "egressAcl = %p, "
                 "sliceSrc = %d, "
                 "sliceDst = %d, "
                 "posDst = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) egressAcl,
                 sliceSrc,
                 sliceDst,
                 posDst,
                 apply);

    /* The first egress acl compiled structure is used to represent all the
     * egress acl slice configuration since all of them share the same pool of
     * condition slices. */
    fmTreeIterInit(&itAcl, egressAcl);
    err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    for (i = FM_FFU_SELECTS_PER_MINSLICE - 1 ; i >= 0 ; i--)
    {
        if ((compiledAcl->muxSelect[(sliceSrc * FM_FFU_SELECTS_PER_MINSLICE) + i] != FM10000_UNUSED_KEY) &&
            (IsMuxSelectPositionValid(compiledAcl->muxSelect[(sliceSrc * FM_FFU_SELECTS_PER_MINSLICE) + i], i, posDst)))
        {
            err = NonDisruptMoveEgressSelect(sw, egressAcl, sliceSrc, i, sliceDst, posDst, apply);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

            break;
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end NonDisruptValidateAndMoveEgress */


/*****************************************************************************/
/** NonDisruptValidateAndMoveIng
 * \ingroup intAcl
 *
 * \desc            This function try to fill unused key to predefined position.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       sliceSrc is the source slice position to move from.
 *
 * \param[in]       sliceDst is the destination slice position to move to.
 *
 * \param[in]       posDst is the destination key index to move to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NonDisruptValidateAndMoveIng(fm_int                  sw,
                                              fm_fm10000CompiledAcl * compiledAcl,
                                              fm_int                  sliceSrc,
                                              fm_int                  sliceDst,
                                              fm_int                  posDst,
                                              fm_bool                 apply)
{
    fm_status err;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "sliceSrc = %d, "
                 "sliceDst = %d, "
                 "posDst = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 sliceSrc,
                 sliceDst,
                 posDst,
                 apply);

    for (i = FM_FFU_SELECTS_PER_MINSLICE - 1 ; i >= 0 ; i--)
    {
        if ((compiledAcl->muxSelect[(sliceSrc * FM_FFU_SELECTS_PER_MINSLICE) + i] != FM10000_UNUSED_KEY) &&
            (IsMuxSelectPositionValid(compiledAcl->muxSelect[(sliceSrc * FM_FFU_SELECTS_PER_MINSLICE) + i], i, posDst)))
        {
            err = NonDisruptMoveIngSelect(sw, compiledAcl, sliceSrc, i, sliceDst, posDst, apply);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

}   /* end NonDisruptValidateAndMoveIng */


/*****************************************************************************/
/** UsedConcreteKey
 * \ingroup intAcl
 *
 * \desc            This function indicate which key are used by one or more
 *                  rule that are part of a compiled acl structure.
 *
 * \param[in]       compiledAcl points to the compiled acl structure to scan.
 *
 * \param[out]      muxSelect refer to the caller allocated structure to fill
 *                  with the used key.
 *
 * \param[in]       numCondition is the number of condition slices configured
 *                  for this specific compiled acl structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UsedConcreteKey(fm_fm10000CompiledAcl *compiledAcl,
                                 fm_byte *              muxSelect,
                                 fm_int                 numCondition)
{
    fm_status err = FM_OK;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_int i;
    fm_int j;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "compiledAcl = %p, "
                 "muxSelect = %p, "
                 "numCondition = %d\n",
                 (void*) compiledAcl,
                 (void*) muxSelect,
                 numCondition);

    /* Scan all the configured rule and mark the used key */
    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
         (err = fmTreeIterNext(&itRule, &ruleNumber, (void**) &compiledAclRule)) ==
                FM_OK ; )
    {
        for (i = 0 ; i < numCondition ; i++)
        {
            for (j = 0 ; j <= 4 ; j++)
            {
                if ((compiledAclRule->sliceKey[i].key & (FM_LITERAL_64(0xff) << (j * 8))) ||
                    (compiledAclRule->sliceKey[i].keyMask & (FM_LITERAL_64(0xff) << (j * 8))))
                {
                    muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] = 1;
                }
            }
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end UsedConcreteKey */


/*****************************************************************************/
/** InvalidateUnusedKey
 * \ingroup intAcl
 *
 * \desc            This function update the compiled acl structure key
 *                  select information.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to update.
 *
 * \param[in]       muxSelect refer to the key configuration to remove.
 *
 * \param[in]       numCondition is the number of condition slices configured
 *                  for this specific compiled acl structure.
 *
 * \return          TRUE if one or more key must be removed.
 *                  FALSE if the actual key configuration is ok.
 *
 *****************************************************************************/
static fm_bool InvalidateUnusedKey(fm_fm10000CompiledAcl *compiledAcl,
                                   fm_byte *              muxSelect,
                                   fm_int                 numCondition)
{
    fm_int i;
    fm_int j;
    fm_bool removedKey = FALSE;

    /* Scan all the configured key and invalidate the one that are not used
     * anymore. */
    for (i = 0 ; i < numCondition ; i++)
    {
        for (j = 0 ; j <= 4 ; j++)
        {
            if ((compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] != FM10000_UNUSED_KEY) &&
                (muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] == FM10000_UNUSED_KEY))
            {
                compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] = FM10000_UNUSED_KEY;
                removedKey = TRUE;
            }
        }
    }

    return removedKey;

}   /* end InvalidateUnusedKey */


/*****************************************************************************/
/** CleanPortSet
 * \ingroup intAcl
 *
 * \desc            This function remove any unused port set from the compiled
 *                  acls structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CleanPortSet(fm_int                  sw,
                              fm_fm10000CompiledAcls *cacls,
                              fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itRule;
    fm_treeIterator itAcl;
    fm_treeIterator itPortSet;
    fm_portSet *portSetEntry;
    void *nextValue;
    fm_uint64 portSetNumber;
    fm_uint64 nextPortSetNumber;
    fm_uint64 ruleNumber;
    fm_uint64 aclNumber;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_bool ruleFound;
    fm_bool aclFound;
    fm_bool portSetUpdated = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 apply);

    for (fmTreeIterInit(&itPortSet, &cacls->portSetId) ;
         (err = fmTreeIterNext(&itPortSet, &portSetNumber, &nextValue)) ==
                FM_OK ; )
    {
        portSetEntry = (fm_portSet*) nextValue;

        /* Per acl portSet configuration */
        if (portSetNumber & ((fm_uint64) FM10000_ACL_PORTSET_TYPE_GLOBAL << FM10000_ACL_PORTSET_TYPE_POS))
        {
            aclNumber = portSetNumber & 0xffffffff;

            /* Try to find its related acl to see if this port set is still
             * needed or not. */
            err = fmTreeFind(&cacls->ingressAcl,
                             FM_ACL_GET_MASTER_KEY(aclNumber),
                             &nextValue);

            if (err == FM_ERR_NOT_FOUND)
            {
                err = fmTreeIterNext(&itPortSet, &nextPortSetNumber, &nextValue);
                if (err != FM_OK)
                {
                    nextPortSetNumber = 0;
                    err = FM_OK;
                }

                cacls->usedPortSet &= ~(1 << portSetEntry->mappedValue);
                err = fmTreeRemoveCertain(&cacls->portSetId,
                                          portSetNumber,
                                          fmFreeAclPortSet);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                portSetUpdated = TRUE;

                if (nextPortSetNumber != 0)
                {
                    /* Reinitialize the tree because an element has been removed
                     * while iterating over it. */
                    err = fmTreeIterInitFromKey(&itPortSet,
                                                &cacls->portSetId,
                                                nextPortSetNumber);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    /* Done */
                    err = FM_ERR_NO_MORE;
                    break;
                }
            }
            else if (err != FM_OK)
            {
                goto ABORT;
            }
        }
        /* Per rule portSet configuration */
        else
        {
            aclFound = FALSE;
            ruleFound = FALSE;
            /* Find which ingress acl used this port set. */
            for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
                 (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
                err = fmTreeFind(compiledAcl->portSetId,
                                 portSetNumber,
                                 &nextValue);
                if (err == FM_OK)
                {
                    if (compiledAcl->aclParts == 0)
                    {
                        ruleFound = FALSE;
                    }

                    /* Find which rule used this port set. */
                    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                         (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                                FM_OK ; )
                    {
                        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                        if (compiledAclRule->portSetId == (fm_int) portSetNumber)
                        {
                            aclFound = TRUE;
                            ruleFound = TRUE;
                            break;
                        }
                    }

                    /* No rule in this acl refer to this portSet anymore. */
                    if (!ruleFound && compiledAcl->firstAclPart)
                    {
                        err = fmTreeRemoveCertain(compiledAcl->portSetId,
                                                  portSetNumber,
                                                  NULL);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        portSetUpdated = TRUE;
                    }
                }
            }

            if (!aclFound)
            {
                err = fmTreeIterNext(&itPortSet, &nextPortSetNumber, &nextValue);
                if (err != FM_OK)
                {
                    nextPortSetNumber = 0;
                    err = FM_OK;
                }

                cacls->usedPortSet &= ~(1 << portSetEntry->mappedValue);
                err = fmTreeRemoveCertain(&cacls->portSetId,
                                          portSetNumber,
                                          fmFreeAclPortSet);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                portSetUpdated = TRUE;

                if (nextPortSetNumber != 0)
                {
                    /* Reinitialize the tree because an element has been removed
                     * while iterating over it. */
                    err = fmTreeIterInitFromKey(&itPortSet,
                                                &cacls->portSetId,
                                                nextPortSetNumber);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    /* Done */
                    err = FM_ERR_NO_MORE;
                    break;
                }
            }
        }
    }
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    if (apply && portSetUpdated)
    {
        err = fmApplyMapSrcPortId(sw, &cacls->portSetId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end CleanPortSet */


/*****************************************************************************/
/** InsertPortSet
 * \ingroup intAcl
 *
 * \desc            This function add a port set element into the source port
 *                  mapper. This function will try to find a free position
 *                  that is already configured for this specific acl.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   portSetTree points to the tree containing all the port set
 *                  configured for this specific acl. The port set without
 *                  any reference to the global port set must be the one to
 *                  specify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InsertPortSet(fm_int                  sw,
                               fm_fm10000CompiledAcls *cacls,
                               fm_tree *               portSetTree,
                               fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int bits;
    fm_uint64 portSetId;
    fm_uint64 newPortSetId;
    fm_treeIterator itPort;
    void *nextValue;
    fm_portSet *portSet;
    fm_portSet *portSetEntry;
    fm_uint32 portSetPos;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);
    fm_bool portSetLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "portSetTree = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) portSetTree,
                 apply);

    /* Find the added portSet */
    for (fmTreeIterInit(&itPort, portSetTree) ;
         (err = fmTreeIterNext(&itPort, &portSetId, &nextValue)) == FM_OK ; )
    {
        /* Found */
        if (nextValue == NULL)
        {
            newPortSetId = portSetId;
            break;
        }
    }
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Is this portSet already part of another ACL? */
    if (fmTreeFind(&cacls->portSetId, newPortSetId, &nextValue) == FM_OK)
    {
        /* This portSet is already defined in the global tree so
         * extract its portSet and update the ACL tree with this
         * value. */
        portSet = (fm_portSet *) nextValue;
        err = fmTreeRemoveCertain(portSetTree,
                                  newPortSetId,
                                  NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmTreeInsert(portSetTree, newPortSetId, portSet);
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    /* PortSet slot available? */
    if ((cacls->usedPortSet & 0xf) == 0xf)
    {
        err = FM_ERR_NO_PORT_SET;
        goto ABORT;
    }

    for (bits = 0 ; bits < FM10000_ACL_PORTSET_PER_RULE_NUM ; bits++)
    {
        portSetPos = (1 << bits);
        if ((cacls->usedPortSet & portSetPos) == 0)
        {
            portSet = (fm_portSet *)fmAlloc(sizeof(fm_portSet));
            if (portSet == NULL)
            {
                err = FM_ERR_NO_MEM;
                goto ABORT;
            }
            FM_CLEAR(*portSet);

            err = fmCreateBitArray(&portSet->associatedPorts,
                                   switchPtr->numCardinalPorts);
            if (err != FM_OK)
            {
                fmFree(portSet);
                goto ABORT;
            }

            /* Protect the software state of portSetTree and its entries */
            TAKE_PORTSET_LOCK(sw);
            portSetLockTaken = TRUE;

            /* Is this portSet defined in the switch? */
            if (fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                           newPortSetId & FM_PORTSET_MASK,
                           &nextValue) == FM_OK)
            {
                portSetEntry = (fm_portSet *) nextValue;

                /* Initialize the portSet with the value defined in
                 * the portSet entry found. */
                fmCopyBitArray(&portSet->associatedPorts,
                               &portSetEntry->associatedPorts);
            }
            else
            {
                fmDeleteBitArray(&portSet->associatedPorts);
                fmFree(portSet);
                err = FM_ERR_INVALID_ACL_RULE;
                goto ABORT;
            }

            DROP_PORTSET_LOCK(sw);
            portSetLockTaken = FALSE;
            
            /* Insert this portSet in the global tree. */
            err = fmTreeInsert(&cacls->portSetId, newPortSetId, portSet);
            if (err != FM_OK)
            {
                fmDeleteBitArray(&portSet->associatedPorts);
                fmFree(portSet);
                goto ABORT;
            }

            portSet->mappedValue = bits;

            /* Update the value associated to this key in the portSet
             * tree of this specific ACL. */
            err = fmTreeRemoveCertain(portSetTree,
                                      newPortSetId,
                                      NULL);
            if (err != FM_OK)
            {
                goto ABORT;
            }
            err = fmTreeInsert(portSetTree, newPortSetId, portSet);
            if (err != FM_OK)
            {
                goto ABORT;
            }
            cacls->usedPortSet |= portSetPos;

            if (apply)
            {
                err = fmApplyMapSrcPortId(sw, &cacls->portSetId);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            break;
        }
    }

ABORT:
    if (portSetLockTaken)
    {
        DROP_PORTSET_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end InsertPortSet */


/*****************************************************************************/
/** InsertGlobalPortSet
 * \ingroup intAcl
 *
 * \desc            This function add a port set element into the source port
 *                  mapper for the global acl.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to map on
 *                  a specific set of ports.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InsertGlobalPortSet(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_fm10000CompiledAcl * compiledAcl,
                                     fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 apply);

    /* Find empty slot in MAP_SRC */
    for (i = FM10000_ACL_PORTSET_PER_RULE_FIRST;
         i < FM10000_ACL_PORTSET_PER_RULE_NUM;
         i++)
    {
        /* If a global entry found, take it. */
        if (~(cacls->usedPortSet) & (1 << i))
        {
            err = fmConvertAclPortToPortSet(sw,
                                            &cacls->portSetId,
                                            compiledAcl,
                                            i,
                                            FM10000_SPECIAL_PORT_PER_ACL_KEY);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Take the mapper position out of the free pool. */
            cacls->usedPortSet |= (1 << i);
            break;
        }
    }
    if (i == FM10000_ACL_PORTSET_PER_RULE_NUM)
    {
        err = FM_ERR_NO_PORT_SET;
        goto ABORT;
    }

    if (apply)
    {
        err = fmApplyMapSrcPortId(sw, &cacls->portSetId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end InsertGlobalPortSet */


/*****************************************************************************/
/** FindUnconfiguredAbstract
 * \ingroup intAcl
 *
 * \desc            This function extract all the abstract key that are part
 *                  of the abstractKey tree but without concrete translation
 *                  into the compiled acl structure.
 *
 * \param[in]       compiledAcl points to the compiled acl structure that
 *                  contain all the concrete elements.
 *
 * \param[in]       abstractKeyTree points to the tree of abstract key to
 *                  translate.
 *
 * \param[out]      remainAbstractKeyTree points to a tree that must be filled
 *                  with abstract key that are part of abstractKey but not in
 *                  the actual compiled acl structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FindUnconfiguredAbstract(fm_fm10000CompiledAcl *compiledAcl,
                                          fm_tree *              abstractKeyTree,
                                          fm_tree *              remainAbstractKeyTree)
{
    fm_status err = FM_OK;
    fm_uint64 abstractKey;
    void *nextValue;
    fm_treeIterator itKey;
    fm_int i;
    fm_int numCondition;
    fm_bool found;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "compiledAcl = %p, "
                 "abstractKeyTree = %p, "
                 "remainAbstractKeyTree = %p\n",
                 (void*) compiledAcl,
                 (void*) abstractKeyTree,
                 (void*) remainAbstractKeyTree);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;

    for (fmTreeIterInit(&itKey, abstractKeyTree) ;
         (err = fmTreeIterNext(&itKey, &abstractKey, &nextValue)) == FM_OK ; )
    {
        found = FALSE;

        /* 8 bits abstract key only fit in 8 bits concrete key. */
        if (abstractKey < FM10000_FIRST_4BITS_ABSTRACT_KEY)
        {
            for (i = 0 ; i < numCondition ; i++)
            {
                if ( (fmAbstractToConcrete8bits[abstractKey][0] == compiledAcl->muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE]) ||
                     (fmAbstractToConcrete8bits[abstractKey][1] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 1]) ||
                     (fmAbstractToConcrete8bits[abstractKey][2] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 2]) ||
                     (fmAbstractToConcrete8bits[abstractKey][3] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 3]) )
                {
                    found = TRUE;
                    break;
                }
            }
        }
        else
        {
            /* If the abstract 4 bits key needed is already configure as part
             * of a 8 bits key then no need to add another key. */
            for (i = 0 ; i < numCondition ; i++)
            {
                if ( (fmAbstractToConcrete4bits[abstractKey][0] == compiledAcl->muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE]) ||
                     (fmAbstractToConcrete4bits[abstractKey][1] == compiledAcl->muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE]) ||
                     (fmAbstractToConcrete4bits[abstractKey][2] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 1]) ||
                     (fmAbstractToConcrete4bits[abstractKey][3] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 1]) ||
                     (fmAbstractToConcrete4bits[abstractKey][4] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 2]) ||
                     (fmAbstractToConcrete4bits[abstractKey][5] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 2]) ||
                     (fmAbstractToConcrete4bits[abstractKey][6] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 3]) ||
                     (fmAbstractToConcrete4bits[abstractKey][7] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 3]) ||
                     (fmAbstractToConcrete4bits[abstractKey][8] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4]) ||
                     (fmAbstractToConcrete4bits[abstractKey][9] == compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4]) )

                {
                    found = TRUE;
                    break;
                }
            }
        }

        /* This abstract key don't have its translation in the current compiled
         * acl structure. */
        if (!found)
        {
            err = fmTreeInsert(remainAbstractKeyTree, abstractKey, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end FindUnconfiguredAbstract */


/*****************************************************************************/
/** fm10000NonDisruptRemoveEgressAclHole
 * \ingroup intAcl
 *
 * \desc            This function try to pack all the egress ACL into a
 *                  continuous chunk allocation which should be as optimized
 *                  as possible.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemoveEgressAclHole(fm_int                  sw,
                                               fm_fm10000CompiledAcls *cacls,
                                               fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    fm_int nextFreeChunk = FM10000_ACL_EGRESS_CHUNK_NUM - 1;
    fm_int chunkNeeded;
    fm_int i;


    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 apply);

    for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl)) == FM_OK ; )
    {
        /* Validate that each egress acl use the minimum number of chunk */
        chunkNeeded = (compiledAcl->numRules + 31) / FM10000_ACL_EGRESS_CHUNK_SIZE;
        if (chunkNeeded < compiledAcl->numChunk)
        {
            /* Deactivating the unused chunk. */
            for (i = chunkNeeded ;
                 i < compiledAcl->numChunk ; i++)
            {
                cacls->chunkValid &= ~(1 << (compiledAcl->chunk - i));
            }

            if (apply)
            {
                err = fmUpdateMasterValid(sw, cacls);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Remove any egress port association of unused egress acl. */
                for (i = chunkNeeded ; i < compiledAcl->numChunk ; i++)
                {
                    err = fmSetEaclChunkCfg(sw,
                                            compiledAcl->chunk - i,
                                            0,
                                            FALSE,
                                            NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
            compiledAcl->numChunk = chunkNeeded;
        }

        /* Only move the chunk if its optimized position is different then
         * its actual one. */
        if (nextFreeChunk != compiledAcl->chunk)
        {
            err = fm10000NonDisruptMoveEgressAcl(sw,
                                                 cacls,
                                                 compiledAcl,
                                                 nextFreeChunk,
                                                 apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        nextFreeChunk -= compiledAcl->numChunk;
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemoveEgressAclHole */


/*****************************************************************************/
/** fm10000NonDisruptRemoveIngAclHole
 * \ingroup intAcl
 *
 * \desc            This function try to pack all the ingress ACL into a
 *                  continuous slice allocation which should be as optimized
 *                  as possible.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemoveIngAclHole(fm_int                  sw,
                                            fm_fm10000CompiledAcls *cacls,
                                            fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_int nextFreeActionSlice;
    fm_int nextFreeConditionSlice;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_int actionSlicePos;
    fm_int conditionSlicePos;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 apply);

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    fmTreeIterInit(&itAcl, &cacls->egressAcl);
    err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
    /* No egress ACL, the right limit is the one configured for the
     * ACL subsystem. */
    if (err == FM_ERR_NO_MORE)
    {
        nextFreeActionSlice = lastAclSlice;
        nextFreeConditionSlice = lastAclSlice;
    }
    /* Egress ACLs all use the same slices for condition so the first
     * egress ACL slice association represent all of them. */
    else if (err == FM_OK)
    {
        nextFreeActionSlice = FM10000_ACL_EGRESS_SLICE_POS - 1;
        nextFreeConditionSlice = compiledAcl->sliceInfo.keyStart - 1;
    }
    else
    {
        goto ABORT;
    }

    for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl)) == FM_OK ; )
    {
        /* Get the next slice position for this compiled acl. */
        err = fmGetSlicePosition(sw,
                                 &compiledAcl->sliceInfo,
                                 nextFreeConditionSlice,
                                 nextFreeActionSlice,
                                 &conditionSlicePos,
                                 &actionSlicePos);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Only move if it is necessary */
        if (conditionSlicePos != compiledAcl->sliceInfo.keyStart)
        {
            /* Move the less prioritize ACL from its actual position to the
             * closest position at right. */
            err = fm10000NonDisruptMoveIngAcl(sw,
                                              cacls,
                                              compiledAcl,
                                              conditionSlicePos,
                                              apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Update the next free position for the next ACL to compile. */
        nextFreeConditionSlice = conditionSlicePos - 1;
        nextFreeActionSlice = actionSlicePos - 1;
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemoveIngAclHole */


/*****************************************************************************/
/** fm10000NonDisruptPackEgressAcl
 * \ingroup intAcl
 *
 * \desc            This function try to pack the muxSelect table of the
 *                  compiled acl structure to free slices if possible.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptPackEgressAcl(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int numCondition;
    fm_int i;
    fm_int j;
    fm_int k;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 apply);

    fmTreeIterInit(&itAcl, &cacls->egressAcl);
    err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;

    /* Try to pack the key into the last condition slices if possible. */
    for (i = numCondition - 1 ; i >= 0 ; i--)
    {
        /* When free position is detected, see if it could be filled by another
         * key configured in the previous slices. */
        for (j = FM_FFU_SELECTS_PER_MINSLICE - 1 ; j >= 0 ; j--)
        {
            if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] == FM10000_UNUSED_KEY)
            {
                for (k = 0 ; k < i ; k++)
                {
                    err = NonDisruptValidateAndMoveEgress(sw,
                                                          &cacls->egressAcl,
                                                          k,
                                                          i,
                                                          j,
                                                          apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] != FM10000_UNUSED_KEY)
                    {
                        break;
                    }
                }
            }
        }
    }

    /* Find the number of slices that are now free. */
    for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
    {
        if ( (compiledAcl->muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE] != FM10000_UNUSED_KEY) ||
             (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 1] != FM10000_UNUSED_KEY) ||
             (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 2] != FM10000_UNUSED_KEY) ||
             (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 3] != FM10000_UNUSED_KEY) ||
             (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4] != FM10000_UNUSED_KEY) )
        {
            break;
        }
    }

    if (i == FM10000_FFU_SLICE_VALID_ENTRIES)
    {
        i = compiledAcl->sliceInfo.keyEnd - compiledAcl->sliceInfo.keyStart;
    }

    /* If the removal of some key freed one or more condition slices, remove
     * those slices from the ACL. */
    if (i > 0)
    {
        err = fm10000NonDisruptCutEgressSlices(sw,
                                               cacls,
                                               i,
                                               apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Remove any hole created by the removal of condition or action slices
     * usage. */
    err = fm10000NonDisruptRemoveIngAclHole(sw, cacls, apply);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptPackEgressAcl */


/*****************************************************************************/
/** fm10000NonDisruptPackIngAcl
 * \ingroup intAcl
 *
 * \desc            This function try to pack the muxSelect table of the
 *                  compiled acl structure to free slices if possible.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptPackIngAcl(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_fm10000CompiledAcl * compiledAcl,
                                      fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int numCondition;
    fm_int packedNumCondition;
    fm_int numAction;
    fm_int packedNumAction;
    fm_int i;
    fm_int j;
    fm_int k;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 apply);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;
    numAction = compiledAcl->sliceInfo.actionEnd -
                compiledAcl->sliceInfo.keyEnd + 1;

    /* Try to pack the key into the first condition slices if possible. */
    for (i = 0 ; i < numCondition ; i++)
    {
        /* When free position is detected, see if it could be filled by another
         * key configured in the latter slices. */
        for (j = FM_FFU_SELECTS_PER_MINSLICE - 1 ; j >= 0 ; j--)
        {
            if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] == FM10000_UNUSED_KEY)
            {
                for (k = numCondition - 1 ; k > i ; k--)
                {
                    err = NonDisruptValidateAndMoveIng(sw,
                                                       compiledAcl,
                                                       k,
                                                       i,
                                                       j,
                                                       apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] != FM10000_UNUSED_KEY)
                    {
                        break;
                    }
                }
            }
        }
    }

    /* Count the number of condition slice usage with this new mux
     * select configuration. */
    packedNumCondition = fmCountConditionSliceUsage(compiledAcl->muxSelect);

    /* The number of action slice usage is driven by the acl rule that need
     * the most. */
    packedNumAction = 0;
    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
         (err = fmTreeIterNext(&itRule, &ruleNumber, (void**) &compiledAclRule)) ==
                FM_OK ; )
    {
        if (compiledAclRule->numActions > packedNumAction)
        {
            packedNumAction = compiledAclRule->numActions;
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }

    /* An ACL without rule is not supported in non disturbing compilation. */
    if (packedNumCondition == 0 ||
        packedNumAction == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_RULES_IN_ACL);
    }

    /* If the removal of some key freed one or more condition slices, remove
     * those slices from the ACL. */
    if (packedNumCondition < numCondition)
    {
        err = fm10000NonDisruptCutIngSlices(sw,
                                            cacls,
                                            compiledAcl,
                                            numCondition - packedNumCondition,
                                            apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* If the number of action slices usage has been reduced, reflect it in the
     * configuration. */
    if (packedNumAction < numAction)
    {
        for (i = packedNumAction ; i < numAction ; i++)
        {
            cacls->actionValid &= ~(1 << (compiledAcl->sliceInfo.actionEnd + i));
            compiledAcl->sliceInfo.actionEnd--;
        }

        if (apply)
        {
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = muxSelect;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* Remove any hole created by the removal of condition or action slices
     * usage. */
    err = fm10000NonDisruptRemoveIngAclHole(sw, cacls, apply);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptPackIngAcl */


/*****************************************************************************/
/** fm10000NonDisruptPackGrpIngAcl
 * \ingroup intAcl
 *
 * \desc            This function try to pack grouped ACLs after rule(s) are
 *                  deleted
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       acl is the acl id that needs to be packed.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptPackGrpIngAcl(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_int                  acl,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_uint64 aclNumKey;
    fm_uint64 ruleNumberKey;
    void *nextValue;
    fm_fm10000CompiledAcl *compiledAcl;
    fm_fm10000CompiledAcl *compiledAclNext;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_uint shiftPos;
    fm_treeIterator itRule;
    fm_uint i;
    fm_bool possibleHole;
    fm_bool ruleModif;
    fm_aclInfo *info;
    fm_acl *aclEntry;
    fm_aclRule *aclRuleEntry;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "acl = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 acl,
                 apply);

    aclNumKey = FM_ACL_GET_MASTER_KEY(acl);
    possibleHole = FALSE;
    ruleModif = FALSE;

    info = &switchPtr->aclInfo;

    /* Group the ACLs together to consume less slices as possible */
    while (TRUE)
    {
        err = fmTreeFind(&cacls->ingressAcl, aclNumKey, &nextValue);
        if (err == FM_ERR_NOT_FOUND)
        {
            /* It is possible that one of the ACL in the middle of the group
             * is not fully loaded. Start another pass... */
            if (possibleHole && ruleModif)
            {
                aclNumKey = FM_ACL_GET_MASTER_KEY(acl);
                possibleHole = FALSE;
                ruleModif = FALSE;
                continue;
            }
            else
            {
                err = FM_OK;
                break;
            }
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        compiledAcl = (fm_fm10000CompiledAcl *) nextValue;

        /* Make sure the last parts of the ACL group are fully loaded.  */
        if (compiledAcl->numRules < FM10000_MAX_RULE_PER_ACL_PART)
        {
            shiftPos = FM10000_MAX_RULE_PER_ACL_PART - compiledAcl->numRules;
            err = fmTreeFind(&cacls->ingressAcl, aclNumKey + 1LL, &nextValue);
            /* Fill up this ACL with subsequent ACLs rules */
            if (err == FM_OK)
            {
                compiledAclNext = (fm_fm10000CompiledAcl *) nextValue;
                if (compiledAclNext->numRules < shiftPos)
                {
                    shiftPos = compiledAclNext->numRules;
                    possibleHole = TRUE;
                }

                /* Move the physical index of the whole part */
                for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                     (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                            FM_OK ; )
                {
                    compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                    compiledAclRule->physicalPos += shiftPos;
                }
                if (err != FM_ERR_NO_MORE)
                {
                    goto ABORT;
                }

                /* Make room for the rules to be moved from one ACL to the
                 * other */
                if (apply && shiftPos)
                {
                    err = fm10000MoveFFURules(sw,
                                              &compiledAcl->sliceInfo,
                                              0,
                                              fmTreeSize(&compiledAcl->rules),
                                              shiftPos);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                for (i = 1 ; i <= shiftPos ; i++)
                {
                    ruleModif = TRUE;

                    fmTreeIterInit(&itRule, &compiledAclNext->rules);
                    err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                    /* Move this rule from "compiledAclNext" to the
                     * "compiledAcl" position. The rule must be converted
                     * to use the key position of this parts. */
                    err = fmTreeFind(&info->acls,
                                     compiledAcl->aclNum,
                                     (void**) &aclEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fmTreeFind(&aclEntry->rules,
                                     compiledAclRule->ruleNumber,
                                     (void**) &aclRuleEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fmTreeInsert(&compiledAcl->rules,
                                       ruleNumberKey,
                                       nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    compiledAcl->numRules++;

                    err = fmTreeRemoveCertain(&compiledAclNext->rules,
                                              ruleNumberKey,
                                              NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    compiledAclNext->numRules--;

                    err = fmConfigureConditionKey(sw,
                                                  NULL,
                                                  aclRuleEntry,
                                                  compiledAclRule->ruleNumber,
                                                  compiledAcl,
                                                  NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    if (apply)
                    {
                        err = fm10000SetFFURule(sw,
                                                &compiledAcl->sliceInfo,
                                                shiftPos - i,
                                                compiledAclRule->valid,
                                                compiledAclRule->sliceKey,
                                                compiledAclRule->actions,
                                                TRUE, /* atomically configure it */
                                                TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        err = fm10000SetFFURuleValid(sw,
                                                     &compiledAclNext->sliceInfo,
                                                     compiledAclRule->physicalPos,
                                                     FALSE,
                                                     TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    compiledAclRule->physicalPos = shiftPos - i;
                }
            }
        }

        aclNumKey++;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptPackGrpIngAcl */


/*****************************************************************************/
/** fm10000NonDisruptCutEgressSlices
 * \ingroup intAcl
 *
 * \desc            This function remove a defined number of condition slices
 *                  of the compiled acl. The removed slices are the last one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       cutSliceNum is the number of condition slices to remove.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptCutEgressSlices(fm_int                  sw,
                                           fm_fm10000CompiledAcls *cacls,
                                           fm_uint32               cutSliceNum,
                                           fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    fm_uint32 i;
    fm_uint32 j;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_uint32 numCondition;
    fm_ffuSliceInfo removedSliceInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "cutSliceNum = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 cutSliceNum,
                 apply);

    fmTreeIterInit(&itAcl, &cacls->egressAcl);
    err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;

    if ( (cutSliceNum > numCondition) ||
         (numCondition > FM10000_FFU_SLICE_VALID_ENTRIES) )
    {
        err = FM_FAIL;
        goto ABORT;
    }

    /* These slices must be reseted once unused */
    removedSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart;
    removedSliceInfo.keyEnd = removedSliceInfo.keyStart + cutSliceNum - 1;
    removedSliceInfo.actionEnd = removedSliceInfo.keyEnd;
    removedSliceInfo.validScenarios = 0xffffffff;

    /* Virtually, this function remove the last x slices but physically, the
     * removal is done on the first x one. */
    for (i = 0 ; i < cutSliceNum ; i++)
    {
        cacls->sliceValid &= ~(1 << (compiledAcl->sliceInfo.keyStart + i));
    }

    if (apply)
    {
        /* Set the "StartCompare" at the new position. At this point,
         * two comparison will be initiated, the first one will be
         * partial since only part of the condition will be part of
         * it and the second one which contain all the condition key.
         * The first one should hit even if the frame is not suppose to
         * but since the action initiation would not be done, this
         * have no effect. */
        err = UpdateScenarioCfg(sw,
                                compiledAcl->sliceInfo.keyStart + cutSliceNum,
                                0xffffffff,
                                1,
                                -1,
                                0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmUpdateMasterValid(sw, cacls);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmResetFFUSlice(sw, &removedSliceInfo);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Shift all the select key configuration of each rule by the number of
     * removed slices. */
    for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl)) == FM_OK ; )
    {
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, (void**) &compiledAclRule)) ==
                    FM_OK ; )
        {
            for (i = cutSliceNum ; i < numCondition ; i++)
            {
                compiledAclRule->sliceKey[i - cutSliceNum] = compiledAclRule->sliceKey[i];
                compiledAclRule->sliceKey[i].kase.value = 0;
                compiledAclRule->sliceKey[i].kase.mask = 0;
                compiledAclRule->sliceKey[i].key = FM_LITERAL_64(0);
                compiledAclRule->sliceKey[i].keyMask = FM_LITERAL_64(0);
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }

        for (i = cutSliceNum ; i < numCondition ; i++)
        {
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
            {
                compiledAcl->muxSelect[((i - cutSliceNum) * FM_FFU_SELECTS_PER_MINSLICE) + j] =
                    compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j];

                compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] = FM10000_UNUSED_KEY;
            }
        }
        compiledAcl->sliceInfo.keyStart += cutSliceNum;
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptCutEgressSlices */


/*****************************************************************************/
/** fm10000NonDisruptCutIngSlices
 * \ingroup intAcl
 *
 * \desc            This function remove a defined number of condition slices
 *                  of the compiled acl. The removed slices are the last one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       cutSliceNum is the number of condition slices to remove.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptCutIngSlices(fm_int                  sw,
                                        fm_fm10000CompiledAcls *cacls,
                                        fm_fm10000CompiledAcl * compiledAcl,
                                        fm_int                  cutSliceNum,
                                        fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int i;
    fm_int j;
    fm_int numCondition;
    fm_int remainNumCondition;
    fm_treeIterator itRule;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_ffuSliceInfo removedSliceInfo;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "cutSliceNum = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 cutSliceNum,
                 apply);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;
    remainNumCondition = numCondition - cutSliceNum;

    /* These slices must be reseted once unused */
    removedSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart;
    removedSliceInfo.keyEnd = removedSliceInfo.keyStart + cutSliceNum - 1;
    removedSliceInfo.actionEnd = removedSliceInfo.keyEnd;
    removedSliceInfo.validScenarios = 0xffffffff;

    /* Keep the same ordering if all the remaining condition slices fit into
     * the space freed by the removed slices. */
    if (cutSliceNum >= remainNumCondition)
    {
        /* Update the scenario configuration to match on both set of key. */
        for (i = 0 ; i < remainNumCondition ; i++)
        {
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
            {
                compiledAcl->muxSelect[((i + cutSliceNum) * FM_FFU_SELECTS_PER_MINSLICE) + j] =
                    compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j];
            }
        }

        if (apply)
        {
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = muxSelect;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Virtually remove the slices from the compiled acl structure. */
        for (i = 0 ; i < cutSliceNum ; i++)
        {
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
            {
                compiledAcl->muxSelect[((remainNumCondition + i) *
                                       FM_FFU_SELECTS_PER_MINSLICE) + j] = FM10000_UNUSED_KEY;
            }

            cacls->sliceValid &= ~(1 << (compiledAcl->sliceInfo.keyStart + i));
        }

        /* Shift the starting position by the number of removed slices. */
        compiledAcl->sliceInfo.keyStart += cutSliceNum;

        if (apply)
        {
            /* Configure the rule to hit on both set of key */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        compiledAclRule->physicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE,
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            /* Remove the original set of key from the comparison. */
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = muxSelect;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    /* Only move the first x number of slices to cut the unused one. */
    else
    {
        /* Configure the rule to hit on both set of key. */
        for (i = 0 ; i < cutSliceNum ; i++)
        {
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
            {
                compiledAcl->muxSelect[((i + remainNumCondition) * FM_FFU_SELECTS_PER_MINSLICE) + j] =
                    compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j];
            }
        }

        /* Update Key to fit this new position */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                    FM_OK ; )
        {
            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
            for (i = 0 ; i < cutSliceNum ; i++)
            {
                compiledAclRule->sliceKey[i + remainNumCondition] = compiledAclRule->sliceKey[i];
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }

        if (apply)
        {
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = muxSelect;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        for (i = 0 ; i < remainNumCondition ; i++)
        {
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
            {
                compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] =
                    compiledAcl->muxSelect[((i + cutSliceNum) * FM_FFU_SELECTS_PER_MINSLICE) + j];
            }
        }
        /* Virtually remove the slices from the compiled acl structure. */
        for (i = 0 ; i < cutSliceNum ; i++)
        {
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
            {
                compiledAcl->muxSelect[((remainNumCondition + i) *
                                       FM_FFU_SELECTS_PER_MINSLICE) + j] = FM10000_UNUSED_KEY;
            }

            cacls->sliceValid &= ~(1 << (compiledAcl->sliceInfo.keyStart + i));
        }

        /* Shift the starting position by the number of removed slices. */
        compiledAcl->sliceInfo.keyStart += cutSliceNum;

        /* Update Key to fit this new position */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                    FM_OK ; )
        {
            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
            for (i = 0 ; i < remainNumCondition ; i++)
            {
                compiledAclRule->sliceKey[i] = compiledAclRule->sliceKey[i + cutSliceNum];
            }
            for (; i < numCondition ; i++)
            {
                compiledAclRule->sliceKey[i].kase.value = 0;
                compiledAclRule->sliceKey[i].kase.mask = 0;
                compiledAclRule->sliceKey[i].key = FM_LITERAL_64(0);
                compiledAclRule->sliceKey[i].keyMask = FM_LITERAL_64(0);
            }

            /* Configure the rule to hit on both set of key */
            if (apply)
            {
                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        compiledAclRule->physicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE,
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }
        err = FM_OK;

        /* Remove the original set of keys from the comparison. */
        if (apply)
        {
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = muxSelect;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    if (apply)
    {
        err = fmResetFFUSlice(sw, &removedSliceInfo);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptCutIngSlices */


/*****************************************************************************/
/** fm10000NonDisruptRemEgressAclUnusedKey
 * \ingroup intAcl
 *
 * \desc            This function update the muxSelect table of the compiled
 *                  acl structure with the remaining key used by each rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   egressAcl points to the egress acl tree that contain all
 *                  the egress compiled acl structure to update.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemEgressAclUnusedKey(fm_int    sw,
                                                 fm_tree * egressAcl,
                                                 fm_bool   apply)
{
    fm_status err = FM_OK;
    fm_int numCondition;
    fm_bool removedKey;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    fm_fm10000CompiledAcl *compiledAcl;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;
    fm_uint32 validScenariosTmp;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "egressAcl = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) egressAcl,
                 apply);

    fmInitializeConcreteKey(muxSelect);

    /* Find which key are not used by any of the egress acl rule. */
    for (fmTreeIterInit(&itAcl, egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl)) ==
                FM_OK ; )
    {
        numCondition = compiledAcl->sliceInfo.keyEnd -
                       compiledAcl->sliceInfo.keyStart + 1;
        err = UsedConcreteKey(compiledAcl, muxSelect, numCondition);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }

    removedKey = FALSE;

    /* Update each egress acl compiled structure by removing the unused key. */
    for (fmTreeIterInit(&itAcl, egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl)) ==
                FM_OK ; )
    {
        numCondition = compiledAcl->sliceInfo.keyEnd -
                       compiledAcl->sliceInfo.keyStart + 1;
        removedKey |= InvalidateUnusedKey(compiledAcl, muxSelect, numCondition);
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

    if (apply && removedKey)
    {
        /* The first egress acl compiled structure is used to represent all the
         * egress acl slice configuration since all of them share the same pool
         * of condition slices. */
        fmTreeIterInit(&itAcl, egressAcl);
        err = fmTreeIterNext(&itAcl, &aclNumber, (void**) &compiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = muxSelect;
        validScenariosTmp = compiledAcl->sliceInfo.validScenarios;
        compiledAcl->sliceInfo.validScenarios = 0xffffffff;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.validScenarios = validScenariosTmp;
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemEgressAclUnusedKey */


/*****************************************************************************/
/** fm10000NonDisruptRemIngAclUnusedKey
 * \ingroup intAcl
 *
 * \desc            This function update the muxSelect table of the compiled
 *                  acl structure with the remaining key used by each rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       acl is the acl id that needs to be updated.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemIngAclUnusedKey(fm_int                  sw,
                                              fm_fm10000CompiledAcls *cacls,
                                              fm_int                  acl,
                                              fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int numCondition;
    fm_bool removedKey;
    fm_fm10000CompiledAcl *compiledAcl;
    fm_uint64 aclNumKey;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    fm_byte muxSelectTmp[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "acl = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 acl,
                 apply);

    aclNumKey = FM_ACL_GET_MASTER_KEY(acl);
    err = fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;

    fmInitializeConcreteKey(muxSelect);

    /* Find which key are not used by any of those ingress acl rule. */
    while (fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl) == FM_OK)
    {
        err = UsedConcreteKey(compiledAcl, muxSelect, numCondition);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        aclNumKey++;
    }

    aclNumKey = FM_ACL_GET_MASTER_KEY(acl);
    /* Remove the unused select key from the compiled acl structure. */
    while (fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl) == FM_OK)
    {
        removedKey = InvalidateUnusedKey(compiledAcl, muxSelect, numCondition);

        if (apply && removedKey)
        {
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelectTmp);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = muxSelectTmp;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        aclNumKey++;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemIngAclUnusedKey */


/*****************************************************************************/
/** fm10000NonDisruptShiftEgressAcl
 * \ingroup intAcl
 *
 * \desc            This function shift an egress ACL by only one position
 *                  (one chunk). This modification must be done in a non
 *                  disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       down is TRUE to indicate a shift down operation or FALSE
 *                  for the other way.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptShiftEgressAcl(fm_int                  sw,
                                          fm_fm10000CompiledAcls *cacls,
                                          fm_fm10000CompiledAcl * compiledAcl,
                                          fm_bool                 down,
                                          fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_int chunk;
    fm_int duplicatePhysical;
    fm_int currentChunk;
    fm_int duplicateChunk;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "down = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 down,
                 apply);

    for (i = 0 ; i < compiledAcl->numChunk ; i++)
    {
        /* To shift down, start at the last chunk */
        if (down)
        {
            currentChunk = compiledAcl->chunk - i;
            duplicateChunk = currentChunk + 1;
        }
        /* To shift up, start at the first chunk */
        else
        {
            currentChunk = compiledAcl->chunk - compiledAcl->numChunk + 1 + i;
            duplicateChunk = currentChunk - 1;
        }

        if (apply)
        {
            /* Only Copy rule that are part of the proper chunk */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, (void*) &compiledAclRule)) ==
                        FM_OK ; )
            {
                chunk = compiledAclRule->physicalPos / FM10000_ACL_EGRESS_CHUNK_SIZE;
                if (chunk == currentChunk)
                {
                    if (down)
                    {
                        duplicatePhysical = compiledAclRule->physicalPos + FM10000_ACL_EGRESS_CHUNK_SIZE;
                    }
                    else
                    {
                        duplicatePhysical = compiledAclRule->physicalPos - FM10000_ACL_EGRESS_CHUNK_SIZE;
                    }
                    err = fm10000SetFFURule(sw,
                                            &compiledAcl->sliceInfo,
                                            duplicatePhysical,
                                            compiledAclRule->valid,
                                            compiledAclRule->sliceKey,
                                            compiledAclRule->actions,
                                            FALSE, /* Live */
                                            TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fm10000SetFFUEaclAction(sw,
                                                  duplicatePhysical,
                                                  compiledAclRule->egressDropActions,
                                                  TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }
        }

        if (apply)
        {
            /* Configure the egress acl port association for this
             * duplicate chunk. */
            err = fmSetEaclChunkCfg(sw,
                                    duplicateChunk,
                                    compiledAcl->sliceInfo.validScenarios,
                                    (currentChunk == compiledAcl->chunk) ? TRUE : FALSE,
                                    compiledAcl->portSetId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Activate this duplicate chunk while deactivating the old one. */
        cacls->chunkValid &= ~(1 << currentChunk);
        cacls->chunkValid |= 1 << duplicateChunk;

        if (apply)
        {
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Remove any egress port association of unused egress acl. */
            err = fmSetEaclChunkCfg(sw, currentChunk, 0, FALSE, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Initialize all rule to invalid for this removed ACL */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, (void*) &compiledAclRule)) ==
                    FM_OK ; )
        {
            chunk = compiledAclRule->physicalPos / FM10000_ACL_EGRESS_CHUNK_SIZE;
            if (chunk == currentChunk)
            {
                if (apply)
                {
                    err = fm10000SetFFURuleValid(sw,
                                                 &compiledAcl->sliceInfo,
                                                 compiledAclRule->physicalPos,
                                                 FALSE,
                                                 TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                if (down)
                {
                    compiledAclRule->physicalPos += FM10000_ACL_EGRESS_CHUNK_SIZE;
                }
                else
                {
                    compiledAclRule->physicalPos -= FM10000_ACL_EGRESS_CHUNK_SIZE;
                }
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }
        err = FM_OK;
    }

    if (down)
    {
        compiledAcl->chunk++;
    }
    else
    {
        compiledAcl->chunk--;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptShiftEgressAcl */


/*****************************************************************************/
/** fm10000NonDisruptMoveEgressAcl
 * \ingroup intAcl
 *
 * \desc            This function try to move a compiled ACL from its actual
 *                  position to the new one passed in argument. This
 *                  modification must be done in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       newPos is the first chunk position where to
 *                  move the compiled acl to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptMoveEgressAcl(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_fm10000CompiledAcl * compiledAcl,
                                         fm_int                  newPos,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_int offset;
    fm_int i;
    fm_int duplicatePhysical;
    fm_bool down;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "newPos = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 newPos,
                 apply);

    if ((newPos < (compiledAcl->numChunk - 1)) ||
        (newPos >= FM10000_ACL_EGRESS_CHUNK_NUM))
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_ACLS_TOO_BIG);
    }

    /* No movement required */
    if (newPos == compiledAcl->chunk)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }
    /* Move the egress ACL down */
    if (newPos > compiledAcl->chunk)
    {
        offset = newPos - compiledAcl->chunk;
        down = TRUE;
    }
    /* Move the egress ACL up */
    else
    {
        offset = compiledAcl->chunk - newPos;
        down = FALSE;
    }

    /* Shift the egress ACL to the new position */
    if (offset < compiledAcl->numChunk)
    {
        for (i = 0 ; i < offset ; i++)
        {
            err = fm10000NonDisruptShiftEgressAcl(sw,
                                                  cacls,
                                                  compiledAcl,
                                                  down,
                                                  apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    /* Move the egress ACL entirely */
    else
    {
        if (apply)
        {
            /* Duplicate each rule individually to the new position. */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, (void*) &compiledAclRule)) ==
                        FM_OK ; )
            {
                if (down)
                {
                    duplicatePhysical = compiledAclRule->physicalPos +
                                        (offset * FM10000_ACL_EGRESS_CHUNK_SIZE);
                }
                else
                {
                    duplicatePhysical = compiledAclRule->physicalPos -
                                        (offset * FM10000_ACL_EGRESS_CHUNK_SIZE);
                }
                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        duplicatePhysical,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE, /* Live */
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000SetFFUEaclAction(sw,
                                              duplicatePhysical,
                                              compiledAclRule->egressDropActions,
                                              TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            /* Configure the egress port association of all the duplicated
             * chunk. */
            for (i = 0 ; i < compiledAcl->numChunk ; i++)
            {
                err = fmSetEaclChunkCfg(sw,
                                        newPos - i,
                                        compiledAcl->sliceInfo.validScenarios,
                                        (i == 0) ? TRUE : FALSE,
                                        compiledAcl->portSetId);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        /* Activate this duplicate chunk while deactivating the old one. */
        for (i = 0 ;
             i < compiledAcl->numChunk ; i++)
        {
            cacls->chunkValid &= ~(1 << (compiledAcl->chunk - i));
            cacls->chunkValid |= 1 << (newPos - i);
        }

        if (apply)
        {
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Remove any egress port association of unused egress acl. */
            for (i = 0 ; i < compiledAcl->numChunk ; i++)
            {
                err = fmSetEaclChunkCfg(sw,
                                        compiledAcl->chunk - i,
                                        0,
                                        FALSE,
                                        NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        /* Initialize all rule to invalid for this removed ACL */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, (void*) &compiledAclRule)) ==
                    FM_OK ; )
        {
            if (apply)
            {
                err = fm10000SetFFURuleValid(sw,
                                             &compiledAcl->sliceInfo,
                                             compiledAclRule->physicalPos,
                                             FALSE,
                                             TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            if (down)
            {
                compiledAclRule->physicalPos += (offset * FM10000_ACL_EGRESS_CHUNK_SIZE);
            }
            else
            {
                compiledAclRule->physicalPos -= (offset * FM10000_ACL_EGRESS_CHUNK_SIZE);
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }
        err = FM_OK;
        compiledAcl->chunk = newPos;
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptMoveEgressAcl */


/*****************************************************************************/
/** fm10000NonDisruptMoveIngAcl
 * \ingroup intAcl
 *
 * \desc            This function try to move a compiled ACL from its actual
 *                  position to the new one passed in argument. This
 *                  modification must be done in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       newPos is the first condition slice position where to
 *                  move the compiled acl to.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptMoveIngAcl(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_fm10000CompiledAcl * compiledAcl,
                                      fm_int                  newPos,
                                      fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_int offset;
    fm_int numCondition;
    fm_int numAction;
    fm_int i;
    fm_int j;
    fm_int k;
    fm_ffuSliceInfo tmpSliceInfo;
    fm_byte tmpMuxSelect[FM_FFU_SELECTS_PER_MINSLICE];
    fm_fm10000FfuSliceKey tmpSliceKey;
    fm_ffuAction tmpAction;
    fm_treeIterator itRule;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_byte muxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    fm_ffuCaseLocation tmpCaseLocation[FM10000_FFU_SLICE_VALID_ENTRIES];

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "newPos = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 newPos,
                 apply);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;
    numAction = compiledAcl->sliceInfo.actionEnd -
                compiledAcl->sliceInfo.keyEnd + 1;

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* No movement required */
    if (newPos == compiledAcl->sliceInfo.keyStart)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }
    /* Move the ACL from left to right */
    if (newPos > compiledAcl->sliceInfo.keyStart)
    {
        /* Compute the offset from the actual position */
        offset = newPos - compiledAcl->sliceInfo.keyStart;

        if (compiledAcl->sliceInfo.actionEnd + offset > lastAclSlice)
        {
            err = FM_ERR_ACLS_TOO_BIG;
            goto ABORT;
        }

        /* If the offset is less then the number of condition slices required,
         * the compiler needs to copy the first x number of condition slices
         * to the unconfigured slices to move to. */
        if (offset < numCondition)
        {
            /* Shift condition slices and wrap first to the last */
            for (i = 0 ; i < offset ; i++)
            {
                /* Update MuxSelect position */
                for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE; j++)
                {
                    tmpMuxSelect[j] = compiledAcl->muxSelect[j];
                }

                for (j = 0 ; j < (numCondition - 1) ; j++)
                {
                    for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE ; k++)
                    {
                        compiledAcl->muxSelect[(j * FM_FFU_SELECTS_PER_MINSLICE) + k] =
                            compiledAcl->muxSelect[((j + 1) * FM_FFU_SELECTS_PER_MINSLICE) + k];
                    }
                }

                for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE; k++)
                {
                    compiledAcl->muxSelect[(j * FM_FFU_SELECTS_PER_MINSLICE) + k] =
                        tmpMuxSelect[k];
                }

                /* Update Key to fit this new position */
                for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                     (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                            FM_OK ; )
                {
                    compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                    tmpSliceKey = compiledAclRule->sliceKey[0];
                    for (j = 0 ; j < (numCondition - 1) ; j++)
                    {
                        compiledAclRule->sliceKey[j] = compiledAclRule->sliceKey[j+1];
                    }
                    compiledAclRule->sliceKey[j] = tmpSliceKey;
                }
                if (err != FM_ERR_NO_MORE)
                {
                    goto ABORT;
                }
                err = FM_OK;
            }
        }
        /* If the offset is less then the maximum number of actions required,
         * the compiler needs to move some actions positions. */
        if (offset < numAction)
        {

            /* Update the action ordering */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                if (compiledAclRule->numActions > offset)
                {
                    for (i = 0 ; i < offset ; i++)
                    {
                        tmpAction = compiledAclRule->actions[0];
                        for (j = 0 ; j < compiledAclRule->numActions - 1 ; j++)
                        {
                            compiledAclRule->actions[j] = compiledAclRule->actions[j+1];
                        }
                        compiledAclRule->actions[j] = tmpAction;
                    }
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }
            err = FM_OK;
        }

        /* Apply this modification to the hardware only if the "apply" flag is
         * active. */
        if (apply)
        {
            /* Configure the copy of the actual rule to the new hardware
             * position. If the offset is less then the maximum condition or
             * action slices utilization, those rule configuration should
             * only affect the unactive slices since all the key and action
             * configuration for the shared part of the slice must remain
             * exactly the same. */
            tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart + offset;
            tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd + offset;
            tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.actionEnd + offset;

            for (i = 0 ; i < numCondition ; i++)
            {
                tmpCaseLocation[i] = FM_FFU_CASE_NOT_MAPPED;
            }
            tmpSliceInfo.caseLocation = tmpCaseLocation;
            tmpSliceInfo.kase = compiledAcl->sliceInfo.kase;
            tmpSliceInfo.validScenarios = compiledAcl->sliceInfo.validScenarios;
            tmpSliceInfo.validLow = compiledAcl->sliceInfo.validLow;
            tmpSliceInfo.validHigh = compiledAcl->sliceInfo.validHigh;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                err = fm10000SetFFURule(sw,
                                        &tmpSliceInfo,
                                        compiledAclRule->physicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE,
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            /* Translate the virtual mux selection to a concrete one. */
            fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

            if (offset < numCondition)
            {
                tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart + numCondition;
                tmpSliceInfo.selects = &muxSelect[(numCondition - offset) * FM_FFU_SELECTS_PER_MINSLICE];

                /* Hack to only configure the key part of the slice */
                if (tmpSliceInfo.keyEnd > 0)
                {
                    tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd - 1;
                }
            }
            else
            {
                tmpSliceInfo.selects = muxSelect;
            }

            err = fm10000ConfigureFFUSlice(sw,
                                           &tmpSliceInfo,
                                           TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);


            /* Clear start compare for the first configured slice if the
             * moved ACL is not usable alone. */
            if (offset < numCondition)
            {
                err = UpdateScenarioCfg(sw,
                                        tmpSliceInfo.keyStart,
                                        tmpSliceInfo.validScenarios,
                                        0,
                                        -1,
                                        0);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.actionEnd + offset;

                err = UpdateScenarioCfg(sw,
                                        tmpSliceInfo.keyEnd,
                                        tmpSliceInfo.validScenarios,
                                        -1,
                                        1,
                                        tmpSliceInfo.actionEnd - tmpSliceInfo.keyEnd + 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        /* Activate the new configured slices. In case the old and the
         * new ACL variant are separated, both will hit and will have
         * the same action configuration. In case the old and the new
         * ACL variant shared some conditions or actions slices, duplicate
         * condition should hit properly and two actions sequence would
         * be initiated with the same action configuration. */
        for (i = compiledAcl->sliceInfo.keyStart + offset ;
             i <= compiledAcl->sliceInfo.keyEnd + offset ; i++)
        {
            cacls->sliceValid |= (1 << i);
        }
        for (i = compiledAcl->sliceInfo.keyEnd + offset ;
             i <= compiledAcl->sliceInfo.actionEnd + offset ; i++)
        {
            cacls->actionValid |= (1 << i);
        }

        if (apply)
        {
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Manually hack the scenario configuration in case the actual ACL
             * configuration is shared between the old and the new layout. */
            if (offset < numCondition)
            {
                /* Clear the first action initiation. */
                err = UpdateScenarioCfg(sw,
                                        compiledAcl->sliceInfo.keyEnd,
                                        compiledAcl->sliceInfo.validScenarios,
                                        -1,
                                        0,
                                        offset);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Set the "StartCompare" at the new position. At this point,
                 * two comparison will be initiated, the first one will be
                 * partial since only part of the condition will be part of
                 * it and the second one which contain all the condition key.
                 * The first one should hit even if the frame is not suppose to
                 * but since the action initiation would not be done, this
                 * have no effect. */
                err = UpdateScenarioCfg(sw,
                                        compiledAcl->sliceInfo.keyStart + offset,
                                        compiledAcl->sliceInfo.validScenarios,
                                        1,
                                        -1,
                                        0);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Configure "tmpSliceInfo" with all the condition slices
                 * that needs to be freed. */
                tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart;
                tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyStart + offset - 1;
                tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd;
                tmpSliceInfo.validScenarios = 0xffffffff;
            }
            else
            {
                /* Configure "tmpSliceInfo" with all the condition and action
                 * slices that needs to be freed. */
                tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart;
                tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd;
                tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd;
                tmpSliceInfo.validScenarios = 0xffffffff;
            }
        }

        /* Remove the old slices from the FFU_SLICE_MASTER_VALID
         * register. */
        if (offset < numCondition)
        {
            for (i = compiledAcl->sliceInfo.keyStart ;
                 i < compiledAcl->sliceInfo.keyStart + offset ; i++)
            {
                cacls->sliceValid &= ~(1 << i);
            }
            for (i = compiledAcl->sliceInfo.keyEnd ;
                 i < compiledAcl->sliceInfo.keyEnd + offset ; i++)
            {
                cacls->actionValid &= ~(1 << i);
            }
        }
        else
        {
            for (i = compiledAcl->sliceInfo.keyStart ;
                 i <= compiledAcl->sliceInfo.keyEnd ; i++)
            {
                cacls->sliceValid &= ~(1 << i);
            }

            if (offset < numAction)
            {
                for (i = compiledAcl->sliceInfo.keyEnd ;
                     i < compiledAcl->sliceInfo.keyEnd + offset ; i++)
                {
                    cacls->actionValid &= ~(1 << i);
                }
            }
            else
            {
                for (i = compiledAcl->sliceInfo.keyEnd ;
                     i <= compiledAcl->sliceInfo.actionEnd ; i++)
                {
                    cacls->actionValid &= ~(1 << i);
                }
            }
        }

        if (apply)
        {
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmResetFFUSlice(sw, &tmpSliceInfo);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        compiledAcl->sliceInfo.keyStart += offset;
        compiledAcl->sliceInfo.keyEnd += offset;
        compiledAcl->sliceInfo.actionEnd += offset;
    }
    /* Move the ACL from right to left */
    else
    {
        /* Compute the offset from the actual position */
        offset = compiledAcl->sliceInfo.keyStart - newPos;

        if (compiledAcl->sliceInfo.keyStart - offset < firstAclSlice)
        {
            err = FM_ERR_ACLS_TOO_BIG;
            goto ABORT;
        }

        /* Non disruptive shifting of the ACL. */
        if (offset < (numCondition + numAction - 1))
        {
            /* Shift this ACL one slice at a time */
            for (i = 0 ; i < offset ; i++)
            {
                /* Update MuxSelect position by shifting all the select from
                 * 1 slice and wrap the last slice to the first one. */
                for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE; j++)
                {
                    tmpMuxSelect[j] = compiledAcl->muxSelect[((numCondition - 1) *
                                                             FM_FFU_SELECTS_PER_MINSLICE) + j];
                }

                for (j = numCondition - 1 ; j > 0 ; j--)
                {
                    for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE ; k++)
                    {
                        compiledAcl->muxSelect[(j * FM_FFU_SELECTS_PER_MINSLICE) + k] =
                            compiledAcl->muxSelect[((j - 1) * FM_FFU_SELECTS_PER_MINSLICE) + k];
                    }
                }

                for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE; k++)
                {
                    compiledAcl->muxSelect[k] = tmpMuxSelect[k];
                }

                /* Update Key to fit this new position */
                for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                     (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                            FM_OK ; )
                {
                    compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                    tmpSliceKey = compiledAclRule->sliceKey[numCondition - 1];
                    for (j = numCondition - 1 ; j > 0 ; j--)
                    {
                        compiledAclRule->sliceKey[j] = compiledAclRule->sliceKey[j-1];
                    }
                    compiledAclRule->sliceKey[0] = tmpSliceKey;

                    tmpAction = compiledAclRule->actions[compiledAclRule->numActions - 1];
                    for (j = compiledAclRule->numActions - 1 ; j > 0 ; j--)
                    {
                        compiledAclRule->actions[j] = compiledAclRule->actions[j-1];
                    }
                    compiledAclRule->actions[0] = tmpAction;
                }
                if (err != FM_ERR_NO_MORE)
                {
                    goto ABORT;
                }
                err = FM_OK;

                /* Apply this modification to the hardware only if the "apply"
                 * flag is active. */
                if (apply)
                {
                    /* This apply should only add the condition of the last
                     * slice to the unactivated added one. */
                    tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart - 1;
                    tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd - 1;
                    for (j = 0 ; j < numCondition ; j++)
                    {
                        tmpCaseLocation[j] = FM_FFU_CASE_NOT_MAPPED;
                    }
                    tmpSliceInfo.caseLocation = tmpCaseLocation;
                    tmpSliceInfo.kase = compiledAcl->sliceInfo.kase;
                    tmpSliceInfo.validScenarios = compiledAcl->sliceInfo.validScenarios;
                    tmpSliceInfo.validLow = compiledAcl->sliceInfo.validLow;
                    tmpSliceInfo.validHigh = compiledAcl->sliceInfo.validHigh;

                    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                         (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                                FM_OK ; )
                    {
                        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                        /* Limit the action number to the one configured for
                         * any specific rule. */
                        tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd +
                                                 compiledAclRule->numActions - 1;
                        err = fm10000SetFFURule(sw,
                                                &tmpSliceInfo,
                                                compiledAclRule->physicalPos,
                                                compiledAclRule->valid,
                                                compiledAclRule->sliceKey,
                                                compiledAclRule->actions,
                                                FALSE,
                                                TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    if (err != FM_ERR_NO_MORE)
                    {
                        goto ABORT;
                    }

                    /* Translate the virtual mux selection to a concrete one. */
                    fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

                    tmpSliceInfo.selects = muxSelect;
                    tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart - 1;
                    tmpSliceInfo.keyEnd = tmpSliceInfo.keyStart;

                    /* Use this hack to only update key part of the FFU Slice */
                    if (tmpSliceInfo.keyEnd > 0)
                    {
                        tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd - 1;
                    }
                    /* Slice 0 can't uses this trick but no other ACL are
                     * located on least precedence slice */
                    else
                    {
                        tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd;
                    }

                    err = fm10000ConfigureFFUSlice(sw,
                                                   &tmpSliceInfo,
                                                   TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    if (tmpSliceInfo.keyEnd == 0)
                    {
                        err = UpdateScenarioCfg(sw,
                                                tmpSliceInfo.keyStart,
                                                tmpSliceInfo.validScenarios,
                                                -1,
                                                0,
                                                1);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Set proper value back */
                    tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd;
                }

                /* Virtually activate the new condition and action slice. */
                cacls->sliceValid |= (1 << (compiledAcl->sliceInfo.keyStart - 1));
                cacls->actionValid |= (1 << (compiledAcl->sliceInfo.keyEnd - 1));

                if (apply)
                {
                    /* At this point the old and the new condition and action
                     * slices are active. */
                    err = fmUpdateMasterValid(sw, cacls);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Clear the old startCompare position */
                    err = UpdateScenarioCfg(sw,
                                            compiledAcl->sliceInfo.keyStart,
                                            compiledAcl->sliceInfo.validScenarios,
                                            0,
                                            -1,
                                            0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Use the new action sequence starting one slice prior to
                     * the old one. */
                    err = UpdateScenarioCfg(sw,
                                            compiledAcl->sliceInfo.keyEnd - 1,
                                            compiledAcl->sliceInfo.validScenarios,
                                            -1,
                                            1,
                                            0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Clear the old startAction */
                    err = UpdateScenarioCfg(sw,
                                            compiledAcl->sliceInfo.keyEnd,
                                            compiledAcl->sliceInfo.validScenarios,
                                            -1,
                                            0,
                                            0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Activate the new action slice */
                    err = UpdateScenarioCfg(sw,
                                            compiledAcl->sliceInfo.keyEnd - 1,
                                            compiledAcl->sliceInfo.validScenarios,
                                            -1,
                                            1,
                                            compiledAcl->sliceInfo.actionEnd -
                                            compiledAcl->sliceInfo.keyEnd + 1);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                /* Virtually deactivate the old condition and action slice. */
                cacls->sliceValid &= ~(1 << (compiledAcl->sliceInfo.keyEnd));
                cacls->actionValid &= ~(1 << (compiledAcl->sliceInfo.actionEnd));

                if (apply)
                {
                    /* Deactivate and reset the old condition and action
                     * slice. */
                    err = fmUpdateMasterValid(sw, cacls);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyEnd;
                    tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd;
                    tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.keyEnd;
                    tmpSliceInfo.validScenarios = 0xffffffff;

                    err = fmResetFFUSlice(sw, &tmpSliceInfo);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                /* Update the compiled acl structure to reflect the new acl
                 * position. */
                compiledAcl->sliceInfo.keyStart--;
                compiledAcl->sliceInfo.keyEnd--;
                compiledAcl->sliceInfo.actionEnd--;

                if (apply)
                {
                    /* Remove any duplicated action data. */
                    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                         (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                                FM_OK ; )
                    {
                        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                        err = fm10000SetFFURule(sw,
                                                &compiledAcl->sliceInfo,
                                                compiledAclRule->physicalPos,
                                                compiledAclRule->valid,
                                                compiledAclRule->sliceKey,
                                                compiledAclRule->actions,
                                                FALSE,
                                                TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    if (err != FM_ERR_NO_MORE)
                    {
                        goto ABORT;
                    }
                    err = FM_OK;
                }
            }
        }
        /* Copy the entire ACL at the new position and invalidate the old
         * one. */
        else
        {
            /* Copy the acl with the same condition and action ordering. */
            tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart - offset;
            tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd - offset;
            tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.actionEnd - offset;

            for (i = 0 ; i < numCondition ; i++)
            {
                tmpCaseLocation[i] = FM_FFU_CASE_NOT_MAPPED;
            }
            tmpSliceInfo.caseLocation = tmpCaseLocation;
            tmpSliceInfo.kase = compiledAcl->sliceInfo.kase;
            tmpSliceInfo.validScenarios = compiledAcl->sliceInfo.validScenarios;
            tmpSliceInfo.validLow = compiledAcl->sliceInfo.validLow;
            tmpSliceInfo.validHigh = compiledAcl->sliceInfo.validHigh;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                if (apply)
                {
                    err = fm10000SetFFURule(sw,
                                            &tmpSliceInfo,
                                            compiledAclRule->physicalPos,
                                            compiledAclRule->valid,
                                            compiledAclRule->sliceKey,
                                            compiledAclRule->actions,
                                            FALSE,
                                            TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }
            err = FM_OK;

            if (apply)
            {
                /* Translate the virtual mux selection to a concrete one. */
                fmInitializeMuxSelect(compiledAcl->muxSelect, muxSelect);

                tmpSliceInfo.selects = muxSelect;

                err = fm10000ConfigureFFUSlice(sw,
                                               &tmpSliceInfo,
                                               TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Virtually activate the new condition and action slices while
             * deactivating the old one. */
            for (i = compiledAcl->sliceInfo.keyStart ;
                 i <= compiledAcl->sliceInfo.keyEnd ; i++)
            {
                cacls->sliceValid |= (1 << (i - offset));
                cacls->sliceValid &= ~(1 << i);
            }
            for (i = compiledAcl->sliceInfo.keyEnd ;
                 i <= compiledAcl->sliceInfo.actionEnd ; i++)
            {
                cacls->actionValid |= (1 << (i - offset));
                cacls->actionValid &= ~(1 << i);
            }

            if (apply)
            {
                /* Atomically update the masterValid register to use the new
                 * acl position instead of the old one and reset the old
                 * condition slices. */
                err = fmUpdateMasterValid(sw, cacls);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fmResetFFUSlice(sw, &compiledAcl->sliceInfo);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            /* Update the compiled acl structure to reflect the new acl
             * position. */
            compiledAcl->sliceInfo.keyStart -= offset;
            compiledAcl->sliceInfo.keyEnd -= offset;
            compiledAcl->sliceInfo.actionEnd -= offset;
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptMoveIngAcl */


/*****************************************************************************/
/** fm10000NonDisruptAddEgressAclHole
 * \ingroup intAcl
 *
 * \desc            This function try to create a hole on top of the compiled
 *                  ACL. If an ACL is already located at this position, this one
 *                  will be moved. This modification must be done in a non
 *                  disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure that needs
 *                  a hole on top. If NULL, the hole will be created at the
 *                  last chunk.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddEgressAclHole(fm_int                  sw,
                                            fm_fm10000CompiledAcls *cacls,
                                            fm_fm10000CompiledAcl * compiledAcl,
                                            fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itAcl;
    fm_uint64 nextKey;
    void *nextValue;
    fm_fm10000CompiledAcl *nextCompiledAcl;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 apply);

    /* Move all the acls by 1 chunk. */
    for (fmTreeIterInitBackwards(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &nextKey, &nextValue)) ==
                FM_OK ; )
    {
        nextCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
        if ( (compiledAcl == NULL) ||
             (nextCompiledAcl->aclNum > compiledAcl->aclNum) )
        {
            err = fm10000NonDisruptMoveEgressAcl(sw,
                                                 cacls,
                                                 nextCompiledAcl,
                                                 nextCompiledAcl->chunk - 1,
                                                 apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddEgressAclHole */


/*****************************************************************************/
/** fm10000NonDisruptAddIngAclHole
 * \ingroup intAcl
 *
 * \desc            This function try to create a hole left to the compiled ACL.
 *                  If an ACL is already located at this position, this one
 *                  will be moved. This modification must be done in a non
 *                  disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure that needs
 *                  a hole at its left. If NULL, the hole will be created
 *                  between the last ingress and the first egress acl.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddIngAclHole(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_fm10000CompiledAcl * compiledAcl,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itAcl;
    fm_uint64 nextKey;
    void *nextValue;
    fm_fm10000CompiledAcl *nextCompiledAcl = NULL;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 apply);

    /* Move all the acls by 1. */
    for (fmTreeIterInitBackwards(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &nextKey, &nextValue)) ==
                FM_OK ; )
    {
        nextCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
        if ( (compiledAcl == NULL) ||
             (nextCompiledAcl->aclNum > compiledAcl->aclNum) ||
             ( (nextCompiledAcl->aclNum == compiledAcl->aclNum) &&
               (nextCompiledAcl->aclParts > compiledAcl->aclParts) ) )
        {
            err = fm10000NonDisruptMoveIngAcl(sw,
                                              cacls,
                                              nextCompiledAcl,
                                              nextCompiledAcl->sliceInfo.keyStart - 1,
                                              apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddIngAclHole */


/*****************************************************************************/
/** fm10000NonDisruptRemEgressAcl
 * \ingroup intAcl
 *
 * \desc            This function try to remove an ACL from the compiled ACL
 *                  structure in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       acl is the ACL Id to remove.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemEgressAcl(fm_int                  sw,
                                        fm_fm10000CompiledAcls *cacls,
                                        fm_int                  acl,
                                        fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_int i;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "acl = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 acl,
                 apply);

    /* Deactivate the ACL to delete atomically */
    err = fmTreeFind(&cacls->egressAcl, acl, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    for (i = 0 ;
         i < compiledAcl->numChunk ; i++)
    {
        cacls->chunkValid &= ~(1 << (compiledAcl->chunk - i));
    }

    if (apply)
    {
        err = fmUpdateMasterValid(sw, cacls);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Remove any egress port association of unused egress acl. */
        for (i = 0 ; i < compiledAcl->numChunk ; i++)
        {
            err = fmSetEaclChunkCfg(sw,
                                    compiledAcl->chunk - i,
                                    0,
                                    FALSE,
                                    NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Initialize all rule to invalid for this removed ACL */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, (void*) &compiledAclRule)) ==
                    FM_OK ; )
        {
            err = fm10000SetFFURuleValid(sw,
                                         &compiledAcl->sliceInfo,
                                         compiledAclRule->physicalPos,
                                         FALSE,
                                         TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }
    }

    /* If this removed egress acl is the last one, all the egress slice usage
     * must be freed. */
    if (fmTreeSize(&cacls->egressAcl) == 1)
    {
        for (i = compiledAcl->sliceInfo.keyStart ;
             i <= compiledAcl->sliceInfo.keyEnd ; i++)
        {
            cacls->sliceValid &= ~(1 << i);
        }
        for (i = compiledAcl->sliceInfo.keyEnd ;
             i <= compiledAcl->sliceInfo.actionEnd ; i++)
        {
            cacls->actionValid &= ~(1 << i);
        }

        /* Apply this modification to the hardware only if the "apply" flag is
         * active. */
        if (apply)
        {
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmResetFFUSlice(sw, &compiledAcl->sliceInfo);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    err = fmTreeRemoveCertain(&cacls->egressAcl, acl, fmFreeCompiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Pack the rule together */
    err = fm10000NonDisruptRemoveEgressAclHole(sw, cacls, apply);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemEgressAcl */


/*****************************************************************************/
/** fm10000NonDisruptRemIngAcl
 * \ingroup intAcl
 *
 * \desc            This function try to remove an ACL from the compiled ACL
 *                  structure in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       acl is the ACL Id to remove.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemIngAcl(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_uint64               acl,
                                     fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAcl* nextCompiledAcl = NULL;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "acl = %lld, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 acl,
                 apply);

    /* Deactivate the ACL to delete atomically */
    err = fmTreeFind(&cacls->ingressAcl, acl, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    for (i = compiledAcl->sliceInfo.keyStart ;
         i <= compiledAcl->sliceInfo.keyEnd ; i++)
    {
        cacls->sliceValid &= ~(1 << i);
    }
    for (i = compiledAcl->sliceInfo.keyEnd ;
         i <= compiledAcl->sliceInfo.actionEnd ; i++)
    {
        cacls->actionValid &= ~(1 << i);
    }

    /* First ACL part moved to the next set */
    if ( compiledAcl->firstAclPart && (compiledAcl->aclParts > 0) )
    {
        err = fmTreeFind(&cacls->ingressAcl, acl - 1LL, (void**) &nextCompiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        nextCompiledAcl->firstAclPart = TRUE;
    }

    /* Apply this modification to the hardware only if the "apply" flag is
     * active. */
    if (apply)
    {
        err = fmUpdateMasterValid(sw, cacls);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmResetFFUSlice(sw, &compiledAcl->sliceInfo);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeRemoveCertain(&cacls->ingressAcl, acl, fmFreeCompiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Pack the slices together */
    err = fm10000NonDisruptRemoveIngAclHole(sw, cacls, apply);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemIngAcl */


/*****************************************************************************/
/** fm10000NonDisruptRemEgressAclRule
 * \ingroup intAcl
 *
 * \desc            This function try to remove an ACL rule from an egress ACL
 *                  structure in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       rule is the ACL rule to remove.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemEgressAclRule(fm_int                 sw,
                                            fm_fm10000CompiledAcl *compiledAcl,
                                            fm_int                 rule,
                                            fm_bool                apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_uint64 removedRule;
    fm_uint64 nextRule;
    fm_int newPhysicalPos;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "rule = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 rule,
                 apply);

    err = fmTreeFind(&compiledAcl->rules, rule, (void**) &compiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Atomically invalidate the ACL rule to remove if the "apply" flag is
     * active. */
    if (apply)
    {
        err = fm10000SetFFURuleValid(sw,
                                     &compiledAcl->sliceInfo,
                                     compiledAclRule->physicalPos,
                                     FALSE,
                                     TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    removedRule = rule;

    /* Move all the lower priority rules for one step. */
    while (TRUE)
    {
        err = fmTreeSuccessor(&compiledAcl->rules,
                              removedRule,
                              &nextRule,
                              (void**) &compiledAclRule);
        /* No more rule to move */
        if (err == FM_ERR_NO_MORE)
        {
            break;
        }
        else if (err == FM_OK)
        {
            newPhysicalPos = compiledAclRule->physicalPos + 1;

            if (apply)
            {
                /* The new physical position should already be invalid. */
                err = fm10000SetFFUEaclAction(sw,
                                              newPhysicalPos,
                                              compiledAclRule->egressDropActions,
                                              TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        newPhysicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        TRUE, /* atomically configure it */
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Invalidate the old position. */
                err = fm10000SetFFURuleValid(sw,
                                             &compiledAcl->sliceInfo,
                                             compiledAclRule->physicalPos,
                                             FALSE,
                                             TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            compiledAclRule->physicalPos = newPhysicalPos;
            removedRule = nextRule;
        }
        else
        {
            goto ABORT;
        }
    }

    /* Finally remove the rule from the tree. */
    err = fmTreeRemoveCertain(&compiledAcl->rules, rule, fmFreeCompiledAclRule);
    compiledAcl->numRules--;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemEgressAclRule */


/*****************************************************************************/
/** fm10000NonDisruptRemIngAclRule
 * \ingroup intAcl
 *
 * \desc            This function try to remove an ACL rule from an ingress ACL
 *                  structure in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       rule is the ACL rule to remove.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemIngAclRule(fm_int                 sw,
                                         fm_fm10000CompiledAcl *compiledAcl,
                                         fm_int                 rule,
                                         fm_bool                apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_uint64 ruleNumber;
    fm_uint16 numRules;
    fm_treeIterator itRule;
    fm_uint16 fromIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "compiledAcl = %p, "
                 "rule = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) compiledAcl,
                 rule,
                 apply);

    err = fmTreeFind(&compiledAcl->rules, rule, (void**) &compiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Atomically invalidate the ACL rule to remove if the "apply" flag is
     * active. */
    if (apply)
    {
        err = fm10000SetFFURuleValid(sw,
                                     &compiledAcl->sliceInfo,
                                     compiledAclRule->physicalPos,
                                     FALSE,
                                     TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    numRules = 0;
    fromIndex = 0;

    for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
         (err = fmTreeIterNext(&itRule, &ruleNumber, (void**) &compiledAclRule)) ==
                FM_OK ; )
    {
        if (ruleNumber < (fm_uint) rule)
        {
            fromIndex = compiledAclRule->physicalPos;
            compiledAclRule->physicalPos--;
            numRules++;
        }
        else
        {
            err = FM_ERR_NO_MORE;
            break;
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }

    if (apply && numRules)
    {
        err = fm10000MoveFFURules(sw,
                                  &compiledAcl->sliceInfo,
                                  fromIndex,
                                  numRules,
                                  fromIndex - 1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Finally remove the rule from the tree. */
    err = fmTreeRemoveCertain(&compiledAcl->rules, rule, fmFreeCompiledAclRule);
    compiledAcl->numRules--;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemIngAclRule */


/*****************************************************************************/
/** fm10000NonDisruptRemAcls
 * \ingroup intAcl
 *
 * \desc            This function try to remove any ACL or ACL rule that are
 *                  present into the compiled ACL structure but not in the
 *                  actual ACL configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 * 
 * \param[in]       internalAcl is the internal ACL to compile/apply. Setting
 *                  this value to -1 will compile/apply all the non internal
 *                  ACL.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemAcls(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls,
                                   fm_int                  internalAcl,
                                   fm_bool                 apply)
{
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    fm_uint64 aclNumberFirstPart = 0LL;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_aclInfo *info;
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_acl *acl;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);
    fm_bool removedRule = FALSE;
    fm_bool InitPolicersClean = FALSE;
    fm_bool InitPortSetsClean = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "internalAcl = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 internalAcl,
                 apply);

    info = &switchPtr->aclInfo;

    /* Scan all the ingress ACL from the compiled ACL structure and try to
     * match the ACL Id to an active ACL Id in the actual configuration. */
    for (fmTreeIterInitBackwards(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        /* Non Disruptive ACL does not supports Instance for now */
        if (compiledAcl->aclInstance != FM_ACL_NO_INSTANCE)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Internal ACL must only be processed if specified */
        if (compiledAcl->internal)
        {
            if ( (internalAcl == -1) || (internalAcl != compiledAcl->aclNum) )
            {
                continue;
            }
        }
        /* Non Internal ACL must only be processed if internalAcl == -1 */
        else
        {
            if (internalAcl != -1)
            {
                continue;
            }
        }

        /* ACL Id part of the compiled ACL structure and the actual
         * configuration, see if all the ACL rule related to this ACL Id are
         * also present in the actual ACL configuration. */
        err = fmTreeFind(&info->acls, compiledAcl->aclNum, (void**) &acl);
        if (err == FM_OK)
        {
            for (fmTreeIterInit(&itRule, &acl->removedRules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                err = fmTreeFind(&compiledAcl->rules,
                                 ruleNumber,
                                 (void**) &compiledAclRule);
                /* Found an ACL Rule that need to be remove from the compiled
                 * ACL structure. */
                if (err == FM_OK)
                {
                    /* At least one rule was removed from this ACL. */
                    removedRule = TRUE;

                    /* Remove any reference to policer index */
                    err = fm10000NonDisruptCleanPolicerRules(sw,
                                                             cacls,
                                                             compiledAclRule,
                                                             0xf,
                                                             apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    if (compiledAclRule->portSetId != FM_PORT_SET_ALL)
                    {
                        InitPortSetsClean = TRUE;
                    }

                    /* Remove this Rule */
                    err = fm10000NonDisruptRemIngAclRule(sw,
                                                         compiledAcl,
                                                         ruleNumber,
                                                         apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else if (err != FM_ERR_NOT_FOUND)
                {
                    goto ABORT;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            /* Process grouped ACLs as a whole */
            if (fmTreeFind(&cacls->ingressAcl, aclNumber - 1LL, &nextValue) == FM_OK)
            {
                if (compiledAcl->firstAclPart)
                {
                    aclNumberFirstPart = aclNumber;
                }
                continue;
            }

            /* Look for unused key configured */
            if (removedRule)
            {
                removedRule = FALSE;

                if (apply)
                {
                    fmTreeDestroy(&acl->removedRules, NULL);
                    fmTreeInit(&acl->removedRules);
                }

                /* Grouped ACLs */
                if (!compiledAcl->firstAclPart)
                {
                    err = fm10000NonDisruptPackGrpIngAcl(sw,
                                                         cacls,
                                                         compiledAcl->aclNum,
                                                         apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Remove unused keys except if specific attribute tells
                     * not to do so.*/
                    if (!compiledAcl->aclKeepUnusedKeys)
                    {
                        err = fm10000NonDisruptRemIngAclUnusedKey(sw,
                                                                  cacls,
                                                                  compiledAcl->aclNum,
                                                                  apply);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    aclNumber = aclNumberFirstPart;

                    /* Process each ACLs that are part of this group */
                    while (fmTreeFind(&cacls->ingressAcl, aclNumber, (void**) &compiledAcl) == FM_OK)
                    {
                        if (fmTreeSize(&compiledAcl->rules))
                        {
                            /* Optimize keys assignation except if specific
                             * attribute tells otherwise. */
                            if (!compiledAcl->aclKeepUnusedKeys)
                            {
                                err = fm10000NonDisruptPackIngAcl(sw,
                                                                  cacls,
                                                                  compiledAcl,
                                                                  apply);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                            }
                        }
                        else
                        {
                            err = fm10000NonDisruptRemIngAcl(sw,
                                                             cacls,
                                                             aclNumber,
                                                             apply);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            fmTreeIterInitBackwards(&itAcl, &cacls->ingressAcl);
                        }

                        /* Process next ACL that is part of this group */
                        aclNumber--;
                    }
                }
                else if (fmTreeSize(&compiledAcl->rules))
                {
                    /* Remove unused keys except if specific attribute tells
                     * not to do so.*/
                    if (!compiledAcl->aclKeepUnusedKeys)
                    {
                        err = fm10000NonDisruptRemIngAclUnusedKey(sw,
                                                                  cacls,
                                                                  compiledAcl->aclNum,
                                                                  apply);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        err = fm10000NonDisruptPackIngAcl(sw,
                                                          cacls,
                                                          compiledAcl,
                                                          apply);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                }
                else
                {
                    /* Remove empty ACL only if this specific attribute is
                     * not set. */
                    if (!compiledAcl->aclKeepUnusedKeys)
                    {
                        goto REM_ING_ACL;
                    }
                }
            }
        }
        /* ACL Id not part of the actual ACL configuration, remove it from the
         * compiled ACL structure. */
        else if (err == FM_ERR_NOT_FOUND)
        {

REM_ING_ACL:
            /* Remove this ingress ACL */
            err = fm10000NonDisruptRemIngAcl(sw, cacls, aclNumber, apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Removal of an ACL always trigger a full policer clean */
            InitPolicersClean = TRUE;
            InitPortSetsClean = TRUE;

            /* Reinitialize the tree because an element has been removed while
             * iterating over it. */
            fmTreeIterInitBackwards(&itAcl, &cacls->ingressAcl);
        }
        else
        {
            goto ABORT;
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }

    if (InitPortSetsClean)
    {
        err = CleanPortSet(sw, cacls, apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (InitPolicersClean)
    {
        err = fm10000NonDisruptCleanPolicers(sw, cacls, apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Scan all the egress ACL from the compiled ACL structure and try to
     * match the ACL Id to an active ACL Id in the actual configuration. */
    for (fmTreeIterInitBackwards(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        /* Internal ACL must only be processed if specified */
        if (compiledAcl->internal)
        {
            if ( (internalAcl == -1) || (internalAcl != compiledAcl->aclNum) )
            {
                continue;
            }
        }
        /* Non Internal ACL must only be processed if internalAcl == -1 */
        else
        {
            if (internalAcl != -1)
            {
                continue;
            }
        }

        /* ACL Id part of the compiled ACL structure and the actual
         * configuration, see if all the ACL rule related to this ACL Id are
         * also present in the actual ACL configuration. */
        err = fmTreeFind(&info->acls, aclNumber, (void**) &acl);
        if (err == FM_OK)
        {
            removedRule = FALSE;
            for (fmTreeIterInitBackwards(&itRule, &acl->removedRules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                err = fmTreeFind(&compiledAcl->rules, ruleNumber, (void**) &compiledAclRule);
                /* Found an ACL Rule that need to be remove from the compiled
                 * ACL structure. */
                if (err == FM_OK)
                {
                    /* At least one rule was removed from this ACL. */
                    removedRule = TRUE;

                    /* Remove this Rule */
                    err = fm10000NonDisruptRemEgressAclRule(sw,
                                                            compiledAcl,
                                                            ruleNumber,
                                                            apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else if (err != FM_ERR_NOT_FOUND)
                {
                    goto ABORT;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            /* Look for unused key configured */
            if (removedRule)
            {
                if (apply)
                {
                    fmTreeDestroy(&acl->removedRules, NULL);
                    fmTreeInit(&acl->removedRules);
                }

                if (fmTreeSize(&compiledAcl->rules))
                {
                    /* Pack the rule together */
                    err = fm10000NonDisruptRemoveEgressAclHole(sw, cacls, apply);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Remove unused keys except if specific attribute tells
                     * not to do so.*/
                    if (!compiledAcl->aclKeepUnusedKeys)
                    {
                        /* Remove the unused select key from the egress acl
                         * scenario. */
                        err = fm10000NonDisruptRemEgressAclUnusedKey(sw,
                                                                     &cacls->egressAcl,
                                                                     apply);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        /* Pack the egress and ingress slices */
                        err = fm10000NonDisruptPackEgressAcl(sw, cacls, apply);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                }
                else
                {
                    /* Remove empty ACL only if this specific attribute is
                     * not set. */
                    if (!compiledAcl->aclKeepUnusedKeys)
                    {
                        goto REM_EGRESS_ACL;
                    }
                }
            }
        }
        /* ACL Id not part of the actual ACL configuration, remove it from the
         * compiled ACL structure. */
        else if (err == FM_ERR_NOT_FOUND)
        {

REM_EGRESS_ACL:
            /* Remove this egress ACL */
            err = fm10000NonDisruptRemEgressAcl(sw, cacls, aclNumber, apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (fmTreeSize(&cacls->egressAcl))
            {
                /* Remove the unused select key from the egress acl scenario. */
                err = fm10000NonDisruptRemEgressAclUnusedKey(sw,
                                                             &cacls->egressAcl,
                                                             apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Pack the egress and ingress slices */
                err = fm10000NonDisruptPackEgressAcl(sw, cacls, apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else
            {
                /* Only pack the ingress slices */
                err = fm10000NonDisruptRemoveIngAclHole(sw, cacls, apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Reinitialize the tree because an element has been removed while
             * iterating over it. */
            fmTreeIterInitBackwards(&itAcl, &cacls->egressAcl);
        }
        else
        {
            goto ABORT;
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemAcls */


/*****************************************************************************/
/** fm10000NonDisruptRemMappers
 * \ingroup intAcl
 *
 * \desc            This function try to remove any mapper configuration that
 *                  are present into the hardware but not in the actual mapper
 *                  configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptRemMappers(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itMappers;
    fm_uint64 mapperId;
    fm_uint64 nextMapperId;
    void *mapperValue;
    void *globalValue;
    void *nextValue;
    fm_uint64 computedMapperId;
    fm_int sizeOfMapper;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 apply);

    for (fmTreeIterInit(&itMappers, &cacls->mappers) ;
         (err = fmTreeIterNext(&itMappers, &mapperId, &mapperValue)) == FM_OK ; )
    {
        err = fmGetMapperKeyAndSize(sw,
                                    (mapperId >> FM_MAPPER_TYPE_KEY_POS) & FM_LITERAL_64(0xff),
                                    mapperValue,
                                    &computedMapperId,
                                    &sizeOfMapper);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Remove the mapper entry that are only defined in the hardware. */
        err = fmTreeFind(&switchPtr->aclInfo.mappers,
                         computedMapperId,
                         &globalValue);
        /* The key are present in both tree, is the value identical? */
        if ((err == FM_OK) &&
            (memcmp(mapperValue, globalValue, sizeOfMapper) == 0))
        {
            continue;
        }

        err = fmTreeIterNext(&itMappers, &nextMapperId, &nextValue);
        if (err != FM_OK)
        {
            nextMapperId = 0;
            err = FM_OK;
        }

        /* Remove it from the hardware */
        if (apply)
        {
            err = fm10000DeleteMapperEntry(sw,
                                           (mapperId >> FM_MAPPER_TYPE_KEY_POS) & FM_LITERAL_64(0xff),
                                           mapperValue,
                                           FM_MAPPER_ENTRY_MODE_APPLY);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            /* Virtually remove it from the compiled acls structure. */
            err = fmTreeRemoveCertain(&cacls->mappers,
                                      mapperId,
                                      fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        if (nextMapperId != 0)
        {
            /* Reinitialize the tree because an element has been removed
             * while iterating over it. */
            err = fmTreeIterInitFromKey(&itMappers,
                                        &cacls->mappers,
                                        nextMapperId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            /* Done */
            err = FM_ERR_NO_MORE;
            break;
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptRemMappers */


/*****************************************************************************/
/** fm10000NonDisruptCleanRoutes
 * \ingroup intAcl
 *
 * \desc            This function clean the ecmp route tree from all the
 *                  removed acl and rule.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 * 
 * \param[in]       acl can be set to do deep analysis if the acl/rule tuple
 *                  is found. -1 Value must be set to avoid specific deep
 *                  analysis.
 * 
 * \param[in]       rule can be set to do deep analysis if the acl/rule tuple
 *                  is found. -1 Value must be set to avoid specific deep
 *                  analysis.
 * 
 * \param[in]       ecmpGrp can be set to do deep analysis if the acl/rule
 *                  tuple is found. -1 Value must be set to avoid specific deep
 *                  analysis.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptCleanRoutes(fm_fm10000CompiledAcls *cacls,
                                       fm_int                  acl,
                                       fm_int                  rule,
                                       fm_int                  ecmpGrp)
{
    fm_status err = FM_OK;
    fm_dlist* ecmpList;
    fm_dlist_node* node;
    fm_fm10000AclRule* ecmpRule;
    fm_treeIterator itEcmp;
    fm_uint64 ecmpGroupId;
    fm_uint64 nextEcmpGroupId;
    void* nextValue;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_uint64 aclNumKey;
    fm_tree *aclTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "cacls = %p, acl = %d, rule = %d, ecmpGrp = %d\n",
                 (void*) cacls, acl, rule, ecmpGrp);

    for (fmTreeIterInit(&itEcmp, &cacls->ecmpGroups) ;
         (err = fmTreeIterNext(&itEcmp, &ecmpGroupId, &nextValue)) == FM_OK ; )
    {
        ecmpList = (fm_dlist *) nextValue;

        node = FM_DLL_GET_FIRST( (ecmpList), head );
        while (node != NULL)
        {
            ecmpRule = (fm_fm10000AclRule *) node->data;

            /* Bypass any further analysis if not needed */
            if ( (acl >= 0) && ((acl != ecmpRule->aclNumber) ||
                                (rule != ecmpRule->ruleNumber)) )
            {
                node = FM_DLL_GET_NEXT(node, next);
                continue;
            }

            aclNumKey = FM_ACL_GET_MASTER_KEY(ecmpRule->aclNumber);

            aclTree = &cacls->ingressAcl;
            err = fmTreeFind(aclTree,
                             aclNumKey,
                             (void**) &compiledAcl);

            if (err == FM_ERR_NOT_FOUND)
            {
                fmDListRemove(ecmpList, node);
                fmFree(ecmpRule);
                node = FM_DLL_GET_FIRST( (ecmpList), head );
                continue;
            }
            else if (err == FM_OK)
            {
                err = fmGetAclNumKey(aclTree,
                                     ecmpRule->aclNumber,
                                     ecmpRule->ruleNumber,
                                     &aclNumKey);
                if (err == FM_ERR_NOT_FOUND)
                {
                    fmDListRemove(ecmpList, node);
                    fmFree(ecmpRule);
                    node = FM_DLL_GET_FIRST( (ecmpList), head );
                    continue;
                }
                else if (err != FM_OK)
                {
                    goto ABORT;
                }
                else
                {
                    /* Proceed with deep analysis to validate ECMP Group
                     * are still the same. Take note that acl and rule was
                     * already validated to be the exact same. */
                    if (acl >= 0)
                    {
                        /* Not the same, remove it */
                        if ( (ecmpGrp < 0) ||
                             ((fm_uint)ecmpGrp != ecmpGroupId) )
                        {
                            fmDListRemove(ecmpList, node);
                            fmFree(ecmpRule);
                            node = FM_DLL_GET_FIRST( (ecmpList), head );
                            continue;
                        }
                    }
                }
            }
            else if (err != FM_OK)
            {
                goto ABORT;
            }
            node = FM_DLL_GET_NEXT(node, next);
        }
        if (fmDListSize(ecmpList) == 0)
        {
            err = fmTreeIterNext(&itEcmp, &nextEcmpGroupId, &nextValue);
            if (err != FM_OK)
            {
                nextEcmpGroupId = 0;
                err = FM_OK;
            }

            err = fmTreeRemove(&cacls->ecmpGroups, ecmpGroupId, fmFreeEcmpGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (nextEcmpGroupId != 0)
            {
                /* Reinitialize the tree because an element has been removed
                 * while iterating over it. */
                err = fmTreeIterInitFromKey(&itEcmp,
                                            &cacls->ecmpGroups,
                                            nextEcmpGroupId);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else
            {
                /* Done */
                err = FM_ERR_NO_MORE;
                break;
            }
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptCleanRoutes */


/*****************************************************************************/
/** fm10000NonDisruptAddMappers
 * \ingroup intAcl
 *
 * \desc            This function try to add any mapper configuration that
 *                  are present into the actual mapper configuration but not
 *                  in the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddMappers(fm_int                  sw,
                                      fm_fm10000CompiledAcls *cacls,
                                      fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itMappers;
    fm_uint64 mapperId;
    void *mapperValue;
    void *globalValue;
    fm_uint64 computedMapperId;
    fm_int sizeOfMapper;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 apply);

    for (fmTreeIterInit(&itMappers, &switchPtr->aclInfo.mappers) ;
         (err = fmTreeIterNext(&itMappers, &mapperId, &globalValue)) == FM_OK ; )
    {
        err = fmGetMapperKeyAndSize(sw,
                                    (mapperId >> FM_MAPPER_TYPE_KEY_POS) & FM_LITERAL_64(0xff),
                                    globalValue,
                                    &computedMapperId,
                                    &sizeOfMapper);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Add the mapper entry that are only defined in the actual mapper
         * configuration. */
        err = fmTreeFind(&cacls->mappers,
                         computedMapperId,
                         &mapperValue);

        /* The key are present in both tree, is the value identical? */
        if ((err == FM_OK) &&
            (memcmp(globalValue, mapperValue, sizeOfMapper) == 0))
        {
            continue;
        }

        /* Add it to the hardware */
        if (apply)
        {
            err = fm10000AddMapperEntry(sw,
                                        (mapperId >> FM_MAPPER_TYPE_KEY_POS) & FM_LITERAL_64(0xff),
                                        globalValue,
                                        FM_MAPPER_ENTRY_MODE_APPLY);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            mapperValue = fmAlloc(sizeOfMapper);
            if (mapperValue == NULL)
            {
                goto ABORT;
            }
            FM_MEMCPY_S(mapperValue, sizeOfMapper, globalValue, sizeOfMapper);
            /* Virtually add it to the compiled acls structure. */
            err = fmTreeInsert(&cacls->mappers, mapperId, mapperValue);
            if (err != FM_OK)
            {
                fmFree(mapperValue);
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddMappers */


/*****************************************************************************/
/** fm10000NonDisruptAddEgressAclRule
 * \ingroup intAcl
 *
 * \desc            This function try to add an ACL rule that is present into
 *                  the actual ACL configuration but not in the egress
 *                  compiled ACL structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  configuration to define.
 *
 * \param[in]       ruleNumber is the rule Id to add.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddEgressAclRule(fm_int                  sw,
                                            fm_fm10000CompiledAcls *cacls,
                                            fm_fm10000CompiledAcl * compiledAcl,
                                            fm_aclRule *            rule,
                                            fm_int                  ruleNumber,
                                            fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_fm10000CompiledAclRule *newCompiledAclRule = NULL;
    void *nextValue;
    fm_treeIterator itRule;
    fm_uint64 ruleNumberKey;
    fm_int newPhysicalPos;
    fm_int duplicatePhysicalPos;
    fm_tree abstractKey;
    fm_tree remainAbstractKey;
    fm_int numCondition;
    fm_int newNumCondition;
    fm_uint64 nextKey;
    fm_fm10000CompiledAcl *nextCompiledAcl;
    fm_treeIterator itAcl;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_int i;
    fm_int j;
    fm_int k;
    fm_fm10000FfuSliceKey tmpSliceKey;
    fm_ffuSliceInfo tmpSliceInfo;
    fm_ffuCaseLocation tmpCaseLocation[FM10000_FFU_SLICE_VALID_ENTRIES];
    fm_byte tmpMuxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    fm_int newEgressCondStart;
    const fm_byte *muxSelectsPtrs;
    fm_uint32 validScenariosTmp;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "rule = %p, "
                 "ruleNumber = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 (void*) rule,
                 ruleNumber,
                 apply);

    FM_CLEAR(abstractKey);
    FM_CLEAR(remainAbstractKey);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;

    /* Look if the compiledAcl have space for a new rule */
    if (compiledAcl->numRules >= FM10000_MAX_RULE_PER_ACL_PART)
    {
        err = FM_ERR_UNSUPPORTED;
        goto ABORT;
    }

    /* Allocate memory space for this new acl rule. */
    newCompiledAclRule = fmAlloc(sizeof(fm_fm10000CompiledAclRule));
    if (newCompiledAclRule == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_CLEAR(*newCompiledAclRule);

    /* Basic rule configuration */
    newCompiledAclRule->aclNumber = compiledAcl->aclNum;
    newCompiledAclRule->ruleNumber = ruleNumber;
    newCompiledAclRule->valid = rule->state;
    newCompiledAclRule->portSetId = FM_PORT_SET_ALL;
    newCompiledAclRule->physicalPos = -1;

    fmTreeInit(&abstractKey);

    /* Translate all the rule condition in abstract key. */
    err = fmFillAbstractKeyTree(sw,
                                NULL,
                                rule,
                                &abstractKey,
                                NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    fmTreeInit(&remainAbstractKey);

    /* Extract all the abstract keys that are already supported in this compiled
     * acl and return only the one that needs to be added. */
    err = FindUnconfiguredAbstract(compiledAcl,
                                   &abstractKey,
                                   &remainAbstractKey);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Find space for those new abstract keys */
    err = fmConvertAbstractToConcreteKey(&remainAbstractKey,
                                         compiledAcl->muxSelect,
                                         compiledAcl->muxUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Find the new condition slice usage after the added key. */
    newNumCondition = fmCountConditionSliceUsage(compiledAcl->muxSelect);

    /* Find the number of new slices needed */
    if (newNumCondition > numCondition)
    {
        newEgressCondStart = FM10000_ACL_EGRESS_SLICE_POS - newNumCondition + 1;

        fmTreeIterInit(&itAcl, &cacls->ingressAcl);
        err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl);
        /* Ingress ACL use the needed slices */
        if (err == FM_OK)
        {
            /* Find by how many slices this ingress ACL must be shifted. */
            while (nextCompiledAcl->sliceInfo.keyEnd >= newEgressCondStart)
            {
                err = fm10000NonDisruptAddIngAclHole(sw, cacls, NULL, apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
        else
        {
            err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (firstAclSlice > newEgressCondStart)
            {
                err = FM_ERR_ACLS_TOO_BIG;
                goto ABORT;
            }
        }

        /* All the egress ACL share the same sets of slices. */
        for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
             (err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl)) == FM_OK ; )
        {
            /* Move all the added condition slices as the first condition of the
             * ACL. */
            for (i = 0 ; i < newNumCondition - numCondition ; i++)
            {
                /* Update MuxSelect position */
                for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE; j++)
                {
                    tmpMuxSelect[j] = nextCompiledAcl->muxSelect[((newNumCondition - 1) *
                                                                 FM_FFU_SELECTS_PER_MINSLICE) + j];
                }

                for (j = newNumCondition - 1 ; j > 0 ; j--)
                {
                    for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE ; k++)
                    {
                        nextCompiledAcl->muxSelect[(j * FM_FFU_SELECTS_PER_MINSLICE) + k] =
                            nextCompiledAcl->muxSelect[((j - 1) * FM_FFU_SELECTS_PER_MINSLICE) + k];
                    }
                }

                for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE; k++)
                {
                    nextCompiledAcl->muxSelect[k] = tmpMuxSelect[k];
                }

                /* Update Key to fit this new position */
                for (fmTreeIterInit(&itRule, &nextCompiledAcl->rules) ;
                     (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                            FM_OK ; )
                {
                    compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                    tmpSliceKey = compiledAclRule->sliceKey[newNumCondition - 1];
                    for (j = newNumCondition - 1 ; j > 0 ; j--)
                    {
                        compiledAclRule->sliceKey[j] = compiledAclRule->sliceKey[j-1];
                    }
                    compiledAclRule->sliceKey[0] = tmpSliceKey;
                }
                if (err != FM_ERR_NO_MORE)
                {
                    goto ABORT;
                }
            }
            if (apply)
            {
                /* Configure the added condition for each rule. */
                tmpSliceInfo.keyStart = newEgressCondStart;
                tmpSliceInfo.keyEnd = nextCompiledAcl->sliceInfo.keyEnd;
                tmpSliceInfo.actionEnd = nextCompiledAcl->sliceInfo.actionEnd;

                for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
                {
                    tmpCaseLocation[i] = FM_FFU_CASE_NOT_MAPPED;
                }
                tmpSliceInfo.caseLocation = tmpCaseLocation;
                tmpSliceInfo.kase = 0;
                tmpSliceInfo.validScenarios = 0xffffffff;
                tmpSliceInfo.validLow = nextCompiledAcl->sliceInfo.validLow;
                tmpSliceInfo.validHigh = nextCompiledAcl->sliceInfo.validHigh;

                for (fmTreeIterInit(&itRule, &nextCompiledAcl->rules) ;
                     (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                            FM_OK ; )
                {
                    compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                    err = fm10000SetFFURule(sw,
                                            &tmpSliceInfo,
                                            compiledAclRule->physicalPos,
                                            compiledAclRule->valid,
                                            compiledAclRule->sliceKey,
                                            compiledAclRule->actions,
                                            FALSE,
                                            TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                if (err != FM_ERR_NO_MORE)
                {
                    goto ABORT;
                }
            }
        }

        /* Configure the scenario of the unactivated slices */
        tmpSliceInfo.keyStart = newEgressCondStart;
        tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyStart - 1;
        tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.keyStart - 1;

        if (apply)
        {
            /* Translate the virtual mux selection to a concrete one. */
            fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelect);

            tmpSliceInfo.selects = tmpMuxSelect;

            /* Use this hack to only update key part of the FFU Slice */
            if (tmpSliceInfo.keyEnd > 0)
            {
                tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd - 1;
            }

            err = fm10000ConfigureFFUSlice(sw,
                                           &tmpSliceInfo,
                                           TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (tmpSliceInfo.keyEnd == 0)
            {
                err = UpdateScenarioCfg(sw,
                                        tmpSliceInfo.keyEnd,
                                        0xffffffff,
                                        -1,
                                        0,
                                        1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Set proper value back */
            tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd;
        }

        /* Virtually activate the slices that contain the new condition key. */
        for (i = tmpSliceInfo.keyStart ;
             i <= tmpSliceInfo.keyEnd ; i++)
        {
            cacls->sliceValid |= (1 << i);
        }

        if (apply)
        {
            /* Activate the new slices in hardware. */
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Clear the old startCompare position. */
            err = UpdateScenarioCfg(sw,
                                    compiledAcl->sliceInfo.keyStart,
                                    0xffffffff,
                                    0,
                                    -1,
                                    0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Update all the egress compiled acl structure to reflect the
         * new acl position. */
        for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
              (err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl)) == FM_OK ; )
        {
            nextCompiledAcl->sliceInfo.keyStart = newEgressCondStart;
        }
    }

    /* Apply any mux select update that may remain */
    if (fmTreeSize(&remainAbstractKey))
    {
        if (apply)
        {
            /* Translate the virtual mux selection to a concrete one. */
            fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = tmpMuxSelect;
            validScenariosTmp = compiledAcl->sliceInfo.validScenarios;
            compiledAcl->sliceInfo.validScenarios = 0xffffffff;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.validScenarios = validScenariosTmp;
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        /* Update all the egress compiled acl structure to reflect the
         * new mux configuration. */
        for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
              (err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl)) == FM_OK ; )
        {
            if (nextCompiledAcl != compiledAcl)
            {
                FM_MEMCPY_S( nextCompiledAcl->muxSelect,
                             sizeof(nextCompiledAcl->muxSelect),
                             compiledAcl->muxSelect,
                             sizeof(compiledAcl->muxSelect) );
            }
        }
    }

    if ((compiledAcl->numRules > 0) &&
        (compiledAcl->numRules % FM10000_ACL_EGRESS_CHUNK_SIZE == 0))
    {
        /* A new chunk is needed for this new rule */
        err = fm10000NonDisruptAddEgressAclHole(sw,
                                                cacls,
                                                compiledAcl,
                                                apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        cacls->chunkValid |= 1 << (compiledAcl->chunk - compiledAcl->numChunk);

        if (apply)
        {
            err = fmSetEaclChunkCfg(sw,
                                    compiledAcl->chunk - compiledAcl->numChunk,
                                    compiledAcl->sliceInfo.validScenarios,
                                    FALSE,
                                    compiledAcl->portSetId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        compiledAcl->numChunk++;
    }

    /* If the inserted rule is the most prioritize of all, its position must
     * be at the end of the last chunk. */
    newPhysicalPos = (compiledAcl->chunk * FM10000_ACL_EGRESS_CHUNK_SIZE) + 31;

    /* Move rule up to free up the space needed for this new rule. */
    for (fmTreeIterInitBackwards(&itRule, &compiledAcl->rules) ;
         (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                FM_OK ; )
    {
        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

        if (compiledAclRule->ruleNumber > ruleNumber)
        {
            duplicatePhysicalPos = compiledAclRule->physicalPos - 1;

            if (apply)
            {
                err = fm10000SetFFUEaclAction(sw,
                                              duplicatePhysicalPos,
                                              compiledAclRule->egressDropActions,
                                              TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        duplicatePhysicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        TRUE, /* atomically configure it */
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000SetFFURuleValid(sw,
                                             &compiledAcl->sliceInfo,
                                             compiledAclRule->physicalPos,
                                             FALSE,
                                             TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            compiledAclRule->physicalPos = duplicatePhysicalPos;
        }
        /* Found the proper space for this inserted rule */
        else
        {
            newPhysicalPos = compiledAclRule->physicalPos - 1;

            err = FM_ERR_NO_MORE;
            break;
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }

    /* Configure the added rule */
    err = fmTreeInsert(&compiledAcl->rules,
                       ruleNumber,
                       newCompiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    compiledAcl->numRules++;

    newCompiledAclRule->physicalPos = newPhysicalPos;

    /* Configure the condition key and key mask based on the entered
     * acl rule. */
    err = fmConfigureConditionKey(sw,
                                  NULL,
                                  rule,
                                  ruleNumber,
                                  compiledAcl,
                                  NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Configure the action data based on the entered acl rule. */
    err = fmConfigureEgressActionData(rule, NULL, newCompiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (apply)
    {
        err = fm10000SetFFUEaclAction(sw,
                                      newCompiledAclRule->physicalPos,
                                      newCompiledAclRule->egressDropActions,
                                      TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fm10000SetFFURule(sw,
                                &compiledAcl->sliceInfo,
                                newCompiledAclRule->physicalPos,
                                newCompiledAclRule->valid,
                                newCompiledAclRule->sliceKey,
                                newCompiledAclRule->actions,
                                TRUE, /* Live */
                                TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    if (fmTreeIsInitialized(&abstractKey))
    {
        fmTreeDestroy(&abstractKey, NULL);
    }

    if (fmTreeIsInitialized(&remainAbstractKey))
    {
        fmTreeDestroy(&remainAbstractKey, NULL);
    }

    /* Free the allocated memory structure of the compiled rule if this rule
     * was not inserted in any other compiled acl structure. */
    if ( newCompiledAclRule && (newCompiledAclRule->physicalPos < 0) )
    {
        fmFree(newCompiledAclRule);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddEgressAclRule */


/*****************************************************************************/
/** fm10000NonDisruptAddSelect
 * \ingroup intAcl
 *
 * \desc            This function try to add an ACL rule that is present into
 *                  the actual ACL configuration but not in the ingress
 *                  compiled ACL structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  configuration to define.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddSelect(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_fm10000CompiledAcl * compiledAcl,
                                     fm_aclRule *            rule,
                                     fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_fm10000CompiledAclRule newCompiledAclRule;
    void *nextValue;
    fm_treeIterator itRule;
    fm_uint64 ruleNumberKey;
    fm_tree abstractKey;
    fm_tree remainAbstractKey;
    fm_uint initialPortSetNum;
    fm_int numCondition;
    fm_int newNumCondition;
    fm_int numAction;
    fm_int newNumAction;
    fm_uint64 nextKey;
    fm_fm10000CompiledAcl *nextCompiledAcl;
    fm_treeIterator itAcl;
    fm_int aclLimit = -1;
    fm_int unusedLimit;
    fm_int freeHole;
    fm_int newSlices = 0;
    fm_int actionSliceShift = 0;
    fm_int i;
    fm_int j;
    fm_int k;
    fm_ffuCaseLocation tmpCaseLocation[FM10000_FFU_SLICE_VALID_ENTRIES];
    fm_byte tmpMuxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    fm_fm10000FfuSliceKey tmpSliceKey;
    fm_ffuSliceInfo tmpSliceInfo;
    const fm_byte *muxSelectsPtrs;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "compiledAcl = %p, "
                 "rule = %p, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) compiledAcl,
                 (void*) rule,
                 apply);

    numCondition = compiledAcl->sliceInfo.keyEnd -
                   compiledAcl->sliceInfo.keyStart + 1;

    numAction = compiledAcl->sliceInfo.actionEnd -
                compiledAcl->sliceInfo.keyEnd + 1;

    fmTreeInit(&abstractKey);
    FM_CLEAR(remainAbstractKey);

    initialPortSetNum = fmTreeSize(compiledAcl->portSetId);

    /* Translate all the rule condition in abstract key. */
    err = fmFillAbstractKeyTree(sw,
                                NULL,
                                rule,
                                &abstractKey,
                                compiledAcl->portSetId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* New PortSet has been defined */
    if (fmTreeSize(compiledAcl->portSetId) > initialPortSetNum)
    {
        /* Insert the required portSet into the mapper if needed. */
        err = InsertPortSet(sw,
                            cacls,
                            compiledAcl->portSetId,
                            apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Add all the abstract key related to the portSet if needed. */
    err = fmFillAbstractPortSetKeyTree(NULL,
                                       rule,
                                       &newCompiledAclRule,
                                       &abstractKey,
                                       compiledAcl->portSetId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    fmTreeInit(&remainAbstractKey);
    /* Extract all the abstract key that are already supported in this compiled
     * acl and return only the one that needs to be added. */
    err = FindUnconfiguredAbstract(compiledAcl,
                                   &abstractKey,
                                   &remainAbstractKey);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Find space for those new abstract keys */
    err = fmConvertAbstractToConcreteKey(&remainAbstractKey,
                                         compiledAcl->muxSelect,
                                         compiledAcl->muxUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Find the new condition slice usage after the added key. */
    newNumCondition = fmCountConditionSliceUsage(compiledAcl->muxSelect);

    /* See if the new rule needs more action slice then allocated. */
    err = fmCountActionSlicesNeeded(sw, NULL, rule, &newNumAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Find the number of new slices needed */
    if (newNumAction > numAction)
    {
        /* Find the next action limit */
        err = fmTreePredecessor(&cacls->ingressAcl,
                                FM_ACL_GET_MASTER_KEY(compiledAcl->aclNum) +
                                                     (fm_uint64)compiledAcl->aclParts,
                                &nextKey,
                                &nextValue);
        /* No other ingress acl at right. */
        if (err == FM_ERR_NO_MORE)
        {
            /* The next action limit can be an egress acl. */
            fmTreeIterInit(&itAcl, &cacls->egressAcl);
            err = fmTreeIterNext(&itAcl, &nextKey, &nextValue);
            if (err == FM_OK)
            {
                nextCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
                aclLimit = nextCompiledAcl->sliceInfo.keyEnd - 1;
            }
            /* No egress acl, the limit is definitively the configured FFU
             * slice allocation. */
            else
            {
                err = fmGetFFUSliceRange(sw, &unusedLimit, &aclLimit);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
        else if (err == FM_OK)
        {
            /* The slice before the first action of the acl at right is the
             * limit. */
            nextCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
            aclLimit = nextCompiledAcl->sliceInfo.keyEnd - 1;
        }
        else
        {
            goto ABORT;
        }

        /* Reflect those new actions in a slice ressource requirment. */
        freeHole = aclLimit - compiledAcl->sliceInfo.actionEnd;

        if (freeHole < (newNumAction - numAction))
        {
            actionSliceShift = (newNumAction - numAction) - freeHole;
            newSlices += actionSliceShift;
        }
    }
    freeHole = 0;

    /* Find the number of new slices needed */
    if (newNumCondition > numCondition)
    {
        /* Find the previous condition limit */
        err = fmTreeSuccessor(&cacls->ingressAcl,
                              FM_ACL_GET_MASTER_KEY(compiledAcl->aclNum) +
                                                   (fm_uint64)compiledAcl->aclParts,
                              &nextKey,
                              &nextValue);
        /* No other ACL at left, the limit is the configured one. */
        if (err == FM_ERR_NO_MORE)
        {
            err = fmGetFFUSliceRange(sw, &aclLimit, &unusedLimit);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else if (err == FM_OK)
        {
            /* The slice after the last condition slice of the previous acl is
             * the limit. */
            nextCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
            aclLimit = nextCompiledAcl->sliceInfo.keyEnd + 1;
        }
        else
        {
            goto ABORT;
        }

        freeHole = compiledAcl->sliceInfo.keyStart - aclLimit;

        if (freeHole < (newNumCondition - numCondition))
        {
            newSlices += (newNumCondition - numCondition) - freeHole;
        }
    }

    /* Only push ingress ACLs if needed */
    if (((cacls->sliceValid & (1 << (compiledAcl->sliceInfo.keyStart - 1))) |
         (cacls->actionValid & (1 << (compiledAcl->sliceInfo.keyEnd - 1)))) != 0)
    {
        /* Need to allocate space for this ACL */
        for (i = 0 ; i < newSlices ; i++)
        {
            err = fm10000NonDisruptAddIngAclHole(sw, cacls, compiledAcl, apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* If the number of condition slices has grown */
    if (newNumCondition > numCondition)
    {
        /* Move all the added condition slices as the first condition of the
         * ACL. */
        for (i = 0 ; i < newNumCondition - numCondition ; i++)
        {
            /* Update MuxSelect position */
            for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE; j++)
            {
                tmpMuxSelect[j] = compiledAcl->muxSelect[((newNumCondition - 1) *
                                                         FM_FFU_SELECTS_PER_MINSLICE) + j];
            }

            for (j = newNumCondition - 1 ; j > 0 ; j--)
            {
                for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE ; k++)
                {
                    compiledAcl->muxSelect[(j * FM_FFU_SELECTS_PER_MINSLICE) + k] =
                        compiledAcl->muxSelect[((j - 1) * FM_FFU_SELECTS_PER_MINSLICE) + k];
                }
            }

            for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE; k++)
            {
                compiledAcl->muxSelect[k] = tmpMuxSelect[k];
            }

            /* Update Key to fit this new position */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                tmpSliceKey = compiledAclRule->sliceKey[newNumCondition - 1];
                for (j = newNumCondition - 1 ; j > 0 ; j--)
                {
                    compiledAclRule->sliceKey[j] = compiledAclRule->sliceKey[j-1];
                }
                compiledAclRule->sliceKey[0] = tmpSliceKey;
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }
        }
        if (apply)
        {
            /* Configure the added condition for each rule. */
            tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart -
                                    (newNumCondition - numCondition);
            tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd;
            tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.actionEnd;

            for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
            {
                if (i < (newNumCondition - numCondition))
                {
                    tmpCaseLocation[i] = FM_FFU_CASE_NOT_MAPPED;
                }
                else
                {
                    tmpCaseLocation[i] = compiledAcl->sliceInfo.caseLocation[i - (newNumCondition - numCondition)];
                }
            }
            tmpSliceInfo.caseLocation = tmpCaseLocation;
            tmpSliceInfo.kase = compiledAcl->sliceInfo.kase;
            tmpSliceInfo.validScenarios = compiledAcl->sliceInfo.validScenarios;
            tmpSliceInfo.validLow = compiledAcl->sliceInfo.validLow;
            tmpSliceInfo.validHigh = compiledAcl->sliceInfo.validHigh;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                err = fm10000SetFFURule(sw,
                                        &tmpSliceInfo,
                                        compiledAclRule->physicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE,
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }
        }

        /* Configure the scenario of the unactivated slices */
        tmpSliceInfo.keyStart = compiledAcl->sliceInfo.keyStart -
                                (newNumCondition - numCondition);
        tmpSliceInfo.keyEnd = compiledAcl->sliceInfo.keyStart - 1;
        tmpSliceInfo.actionEnd = compiledAcl->sliceInfo.keyStart - 1;
        if (apply)
        {
            /* Translate the virtual mux selection to a concrete one. */
            fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelect);

            tmpSliceInfo.selects = tmpMuxSelect;

            /* Use this hack to only update key part of the FFU Slice */
            if (tmpSliceInfo.keyEnd > 0)
            {
                tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd - 1;
            }

            err = fm10000ConfigureFFUSlice(sw,
                                           &tmpSliceInfo,
                                           TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (tmpSliceInfo.keyEnd == 0)
            {
                err = UpdateScenarioCfg(sw,
                                        tmpSliceInfo.keyEnd,
                                        tmpSliceInfo.validScenarios,
                                        -1,
                                        0,
                                        1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Set proper value back */
            tmpSliceInfo.actionEnd = tmpSliceInfo.keyEnd;
        }

        /* Virtually activate the slices that contain the new condition key. */
        for (i = tmpSliceInfo.keyStart ;
             i <= tmpSliceInfo.keyEnd ; i++)
        {
            cacls->sliceValid |= (1 << i);
        }

        if (apply)
        {
            /* Activate the new slices in hardware. */
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Clear the old startCompare position. */
            err = UpdateScenarioCfg(sw,
                                    compiledAcl->sliceInfo.keyStart,
                                    compiledAcl->sliceInfo.validScenarios,
                                    0,
                                    -1,
                                    0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Update the compiled acl structure to reflect the new acl position. */
        compiledAcl->sliceInfo.keyStart -= (newNumCondition - numCondition);

        err = fmGetFFUSliceRange(sw, &aclLimit, &unusedLimit);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (aclLimit > compiledAcl->sliceInfo.keyStart)
        {
            err = FM_ERR_ACLS_TOO_BIG;
            goto ABORT;
        }
    }

    /* If the number of action slices has grown */
    if (newNumAction > numAction)
    {
        /* The number of slice to shift for the action part only. */
        if (actionSliceShift)
        {
            err = fm10000NonDisruptMoveIngAcl(sw,
                                              cacls,
                                              compiledAcl,
                                              compiledAcl->sliceInfo.keyStart -
                                              actionSliceShift,
                                              apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        compiledAcl->sliceInfo.actionEnd += (newNumAction - numAction);

        for (i = compiledAcl->sliceInfo.keyEnd ;
             i <= compiledAcl->sliceInfo.actionEnd ; i++)
        {
            cacls->actionValid |= (1 << i);
        }

        if (apply)
        {
            /* Clear the new action slice */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        compiledAclRule->physicalPos,
                                        compiledAclRule->valid,
                                        compiledAclRule->sliceKey,
                                        compiledAclRule->actions,
                                        FALSE,
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Update the number of action slices of this acl. */
            err = UpdateScenarioCfg(sw,
                                    compiledAcl->sliceInfo.keyEnd,
                                    compiledAcl->sliceInfo.validScenarios,
                                    -1,
                                    1,
                                    newNumAction);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* Apply any mux select update that may remain */
    if (apply && fmTreeSize(&remainAbstractKey))
    {
        /* Translate the virtual mux selection to a concrete one. */
        fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelect);

        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = tmpMuxSelect;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    if (fmTreeIsInitialized(&abstractKey))
    {
        fmTreeDestroy(&abstractKey, NULL);
    }

    if (fmTreeIsInitialized(&remainAbstractKey))
    {
        fmTreeDestroy(&remainAbstractKey, NULL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddSelect */


/*****************************************************************************/
/** fm10000NonDisruptAddIngAclRule
 * \ingroup intAcl
 *
 * \desc            This function try to add an ACL rule that is present into
 *                  the actual ACL configuration but not in the ingress
 *                  compiled ACL structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       aclNumber is the acl Id attached to the rule to add.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  configuration to define.
 *
 * \param[in]       ruleNumber is the rule Id to add.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddIngAclRule(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_int                  aclNumber,
                                         fm_aclRule *            rule,
                                         fm_int                  ruleNumber,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAclRule *compiledAclRule = NULL;
    fm_fm10000CompiledAclRule *nextCompiledAclRule;
    fm_fm10000CompiledAclRule *newCompiledAclRule = NULL;
    void *nextValue;
    fm_treeIterator itRule;
    fm_uint64 ruleNumberKey;
    fm_uint64 nextRuleNumberKey;
    fm_int newPhysicalPos = 0;
    fm_fm10000CompiledAcl *compiledAcl;
    fm_int i;
    fm_uint64 aclNumKey;
    fm_int firstCondSlice;
    fm_int firstActionSlice;
    fm_int lastActionSlice;
    fm_fm10000CompiledAcl *nextCompiledAcl;
    fm_byte tmpMuxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;
    fm_bool found;
    fm_aclInfo *info;
    fm_acl *aclEntry;
    fm_aclRule *aclRuleEntry;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt;
    fm_uint16 numRules;
    fm_bool strictCount;
    fm_fm10000CompiledPolicers *policers;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_uint32 freeCondMask;
    fm_uint32 freeActMask;


    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "aclNumber = %d, "
                 "rule = %p, "
                 "ruleNumber = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 aclNumber,
                 (void*) rule,
                 ruleNumber,
                 apply);

    switchExt = (fm10000_switch *) switchPtr->extension;
    info = &switchPtr->aclInfo;

    aclNumKey = FM_ACL_GET_MASTER_KEY(aclNumber);
    while (fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl) == FM_OK)
    {
        err = fm10000NonDisruptAddSelect(sw, cacls, compiledAcl, rule, apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        aclNumKey++;
    }

    aclNumKey--;
    err = fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Allocate memory space for this new acl rule. */
    newCompiledAclRule = fmAlloc(sizeof(fm_fm10000CompiledAclRule));
    if (newCompiledAclRule == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_CLEAR(*newCompiledAclRule);

    /* Basic rule configuration */
    newCompiledAclRule->aclNumber = aclNumber;
    newCompiledAclRule->ruleNumber = ruleNumber;
    newCompiledAclRule->valid = rule->state;
    newCompiledAclRule->portSetId = FM_PORT_SET_ALL;
    newCompiledAclRule->physicalPos = -1;

    err = fmCountActionSlicesNeeded(sw,
                                    NULL,
                                    rule,
                                    &newCompiledAclRule->numActions);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (newCompiledAclRule->numActions > FM10000_ACL_MAX_ACTIONS_PER_RULE)
    {
        err = FM_ERR_INVALID_ACL_RULE;
        goto ABORT;
    }

    if (compiledAcl->numRules == FM10000_MAX_RULE_PER_ACL_PART)
    {
        err = fmGetSlicePosition(sw,
                                 &compiledAcl->sliceInfo,
                                 compiledAcl->sliceInfo.keyStart - 1,
                                 compiledAcl->sliceInfo.keyEnd - 1,
                                 &firstCondSlice,
                                 &firstActionSlice);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        lastActionSlice = firstActionSlice +
                          compiledAcl->sliceInfo.actionEnd -
                          compiledAcl->sliceInfo.keyEnd;

        /* Make sure the whole condition/action ffu slice range is free */
        freeCondMask = 0;
        for (i = firstCondSlice ; i <= firstActionSlice ; i++)
        {
            freeCondMask |= (1 << i);
        }

        freeActMask = 0;
        for (i = firstActionSlice ; i <= lastActionSlice ; i++)
        {
            freeActMask |= (1 << i);
        }

        while (((cacls->sliceValid & freeCondMask) |
                (cacls->actionValid & freeActMask)) != 0)
        {
            err = fm10000NonDisruptAddIngAclHole(sw,
                                                 cacls,
                                                 compiledAcl,
                                                 apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        nextCompiledAcl = fmAlloc(sizeof(fm_fm10000CompiledAcl));
        if (nextCompiledAcl == NULL)
        {
            err = FM_ERR_NO_MEM;
            goto ABORT;
        }
        FM_CLEAR(*nextCompiledAcl);

        nextCompiledAcl->aclParts = compiledAcl->aclParts + 1;
        nextCompiledAcl->firstAclPart = TRUE;
        nextCompiledAcl->aclNum = compiledAcl->aclNum;
        nextCompiledAcl->aclKeepUnusedKeys = compiledAcl->aclKeepUnusedKeys;
        nextCompiledAcl->internal = compiledAcl->internal;
        nextCompiledAcl->aclInstance = compiledAcl->aclInstance;
        nextCompiledAcl->numRules = 0;
        fmTreeInit(&nextCompiledAcl->rules);
        nextCompiledAcl->portSetId = compiledAcl->portSetId;
        nextCompiledAcl->sliceInfo.keyStart  = firstCondSlice;
        nextCompiledAcl->sliceInfo.keyEnd    = firstActionSlice;
        nextCompiledAcl->sliceInfo.actionEnd = lastActionSlice;
        nextCompiledAcl->sliceInfo.validScenarios = compiledAcl->sliceInfo.validScenarios;
        nextCompiledAcl->sliceInfo.kase = compiledAcl->sliceInfo.kase;
        nextCompiledAcl->sliceInfo.validLow = compiledAcl->sliceInfo.validLow;
        nextCompiledAcl->sliceInfo.validHigh = compiledAcl->sliceInfo.validHigh;
        nextCompiledAcl->sliceInfo.selects = nextCompiledAcl->muxSelect;
        nextCompiledAcl->sliceInfo.caseLocation = nextCompiledAcl->caseLocation;
        FM_MEMCPY_S( nextCompiledAcl->muxSelect,
                     sizeof(nextCompiledAcl->muxSelect),
                     compiledAcl->muxSelect,
                     sizeof(compiledAcl->muxSelect));
        FM_MEMCPY_S( nextCompiledAcl->muxUsed,
                     sizeof(nextCompiledAcl->muxUsed),
                     compiledAcl->muxUsed,
                     sizeof(compiledAcl->muxUsed));
        FM_MEMCPY_S( nextCompiledAcl->caseLocation,
                     sizeof(nextCompiledAcl->caseLocation),
                     compiledAcl->caseLocation,
                     sizeof(compiledAcl->caseLocation));

        aclNumKey++;
        err = fmTreeInsert(&cacls->ingressAcl, aclNumKey, nextCompiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        fmUpdateValidSlice(&nextCompiledAcl->sliceInfo, cacls);

        compiledAcl->firstAclPart = FALSE;

        if (apply)
        {
            /* Translate the virtual mux selection to a concrete one. */
            fmInitializeMuxSelect(nextCompiledAcl->muxSelect, tmpMuxSelect);

            muxSelectsPtrs = nextCompiledAcl->sliceInfo.selects;
            nextCompiledAcl->sliceInfo.selects = tmpMuxSelect;

            err = fm10000ConfigureFFUSlice(sw,
                                           &nextCompiledAcl->sliceInfo,
                                           TRUE);
            nextCompiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Activate the new slices in hardware. */
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    found = FALSE;
    while ((err = fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl)) == FM_OK)
    {
        err = fmTreeFind(&cacls->ingressAcl,
                         aclNumKey - 1LL,
                         (void**) &nextCompiledAcl);
        if (err == FM_OK)
        {
            fmTreeIterInitBackwards(&itRule, &nextCompiledAcl->rules);
            err = fmTreeIterNext(&itRule,
                                 &nextRuleNumberKey,
                                 (void**) &nextCompiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (nextCompiledAclRule->ruleNumber < ruleNumber)
            {
                found = TRUE;
            }
        }
        else if (err == FM_ERR_NOT_FOUND)
        {
            found = TRUE;
        }
        else
        {
            goto ABORT;
        }

        if (found)
        {
            /* If the inserted rule is the less prioritize of all, its
             * position must be 0. */
            newPhysicalPos = 0;
            numRules = 0;

            /* Move rule down to free up the space needed for this new rule. */
            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                if (compiledAclRule->ruleNumber < ruleNumber)
                {
                    compiledAclRule->physicalPos++;
                    numRules++;
                }
                /* Found the proper space for this inserted rule */
                else
                {
                    newPhysicalPos = compiledAclRule->physicalPos + 1;
                    err = FM_ERR_NO_MORE;
                    break;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                goto ABORT;
            }

            if (apply && numRules)
            {
                err = fm10000MoveFFURules(sw,
                                          &compiledAcl->sliceInfo,
                                          newPhysicalPos,
                                          numRules,
                                          newPhysicalPos + 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            err = FM_OK;
            break;
        }
        else
        {
            /* Move the first rule of the next ACL (less prioritize one) to
               the most prioritize one of the current ACL. */
            newPhysicalPos = compiledAcl->numRules;

            /* Move this rule from "nextCompiledAcl" to the
             * "compiledAcl" position. The rule must be converted
             * to use the key position of this parts. */
            err = fmTreeFind(&info->acls,
                             compiledAcl->aclNum,
                             (void**) &aclEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmTreeFind(&aclEntry->rules,
                             nextCompiledAclRule->ruleNumber,
                             (void**) &aclRuleEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmTreeInsert(&compiledAcl->rules,
                               nextRuleNumberKey,
                               nextCompiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            compiledAcl->numRules++;

            err = fmTreeRemoveCertain(&nextCompiledAcl->rules,
                                      nextRuleNumberKey,
                                      NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            nextCompiledAcl->numRules--;

            err = fmConfigureConditionKey(sw,
                                          NULL,
                                          aclRuleEntry,
                                          nextCompiledAclRule->ruleNumber,
                                          compiledAcl,
                                          NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (apply)
            {
                err = fm10000SetFFURule(sw,
                                        &compiledAcl->sliceInfo,
                                        newPhysicalPos,
                                        nextCompiledAclRule->valid,
                                        nextCompiledAclRule->sliceKey,
                                        nextCompiledAclRule->actions,
                                        TRUE, /* atomically configure it */
                                        TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000SetFFURuleValid(sw,
                                             &nextCompiledAcl->sliceInfo,
                                             nextCompiledAclRule->physicalPos,
                                             FALSE,
                                             TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            nextCompiledAclRule->physicalPos = newPhysicalPos;

            numRules = 0;

            /* Move rule up on the next ACL to fill the gap created by the last
               rule movement. */
            for (fmTreeIterInit(&itRule, &nextCompiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumberKey, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
                compiledAclRule->physicalPos--;
                numRules++;
            }
            if (err == FM_ERR_NO_MORE)
            {
                if (apply && numRules)
                {
                    err = fm10000MoveFFURules(sw,
                                              &nextCompiledAcl->sliceInfo,
                                              compiledAclRule->physicalPos + 1,
                                              numRules,
                                              compiledAclRule->physicalPos);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

            }
            else
            {
                goto ABORT;
            }
        }
        aclNumKey--;
    }
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Configure the added rule */
    err = fmTreeInsert(&compiledAcl->rules,
                       ruleNumber,
                       newCompiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    compiledAcl->numRules++;

    newCompiledAclRule->physicalPos = newPhysicalPos;

    /* Initialize the indexes of the policer. */
    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        newCompiledAclRule->policerIndex[i] = 0;
    }

    /* Configure the condition key and key mask based on the entered
     * acl rule. */
    err = fmConfigureConditionKey(sw,
                                  NULL,
                                  rule,
                                  ruleNumber,
                                  compiledAcl,
                                  NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    strictCount = switchExt->aclStrictCount;

    err = fm10000ConfigurePolicerBank(sw,
                                      NULL,
                                      cacls,
                                      rule,
                                      newCompiledAclRule,
                                      strictCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Configure the action data based on the entered acl rule. */
    err = fmConfigureActionData(sw,
                                NULL,
                                cacls->policers,
                                &cacls->ecmpGroups,
                                rule,
                                compiledAcl,
                                newCompiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (apply)
    {
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            /* If the added rule needs a policer entry, configure it. */
            if (newCompiledAclRule->policerIndex[i] != 0)
            {
                policers = &cacls->policers[i];

                err = fmTreeFind(&policers->policerEntry,
                                 newCompiledAclRule->policerIndex[i],
                                 &nextValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;

                if (compiledPolEntry->countEntry)
                {
                    err = fm10000SetPolicerCounter(sw,
                                                   i,
                                                   newCompiledAclRule->policerIndex[i],
                                                   FM_LITERAL_64(0),
                                                   FM_LITERAL_64(0));
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    err = fm10000SetPolicer(sw,
                                            i,
                                            newCompiledAclRule->policerIndex[i],
                                            &compiledPolEntry->committed,
                                            &compiledPolEntry->excess);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fm10000SetPolicerConfig(sw,
                                                  i,
                                                  policers->indexLastPolicer,
                                                  policers->ingressColorSource,
                                                  policers->markDSCP,
                                                  policers->markSwitchPri,
                                                  TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
        }

        err = fm10000SetFFURule(sw,
                                &compiledAcl->sliceInfo,
                                newCompiledAclRule->physicalPos,
                                newCompiledAclRule->valid,
                                newCompiledAclRule->sliceKey,
                                newCompiledAclRule->actions,
                                TRUE, /* Live */
                                TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    /* Free the allocated memory structure of the compiled rule if this rule
     * was not inserted in any other compiled acl structure. */
    if ( newCompiledAclRule && (newCompiledAclRule->physicalPos < 0) )
    {
        fmFree(newCompiledAclRule);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddIngAclRule */


/*****************************************************************************/
/** fm10000NonDisruptAddEgressAcl
 * \ingroup intAcl
 *
 * \desc            This function try to add an ACL that is present into
 *                  the actual ACL configuration but not in the egress
 *                  compiled ACL structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       acl points to the defined acl that contain all the
 *                  configuration to define.
 *
 * \param[in]       aclNumber is the acl Id to add.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddEgressAcl(fm_int                  sw,
                                        fm_fm10000CompiledAcls *cacls,
                                        fm_acl *                acl,
                                        fm_int                  aclNumber,
                                        fm_bool                 apply)
{
    fm_switch * switchPtr;
    fm_status err = FM_OK;
    fm_treeIterator itAcl;
    fm_uint64 nextKey;
    fm_fm10000CompiledAcl *nextCompiledAcl;
    fm_fm10000CompiledAcl* compiledAcl = NULL;
    fm_int  chunkPos;
    fm_int  cpi;
    fm_int  port;
    fm_bool bitValue;
    fm_byte tmpMuxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;
    fm_uint32 validScenariosTmp;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "acl = %p, "
                 "aclNumber = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) acl,
                 aclNumber,
                 apply);

    switchPtr = GET_SWITCH_PTR(sw);

    chunkPos = FM10000_ACL_EGRESS_CHUNK_NUM - 1;
    for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl)) == FM_OK ; )
    {
        if (nextCompiledAcl->aclNum < aclNumber)
        {
            chunkPos = nextCompiledAcl->chunk - nextCompiledAcl->numChunk;
        }
        else
        {
            err = fm10000NonDisruptAddEgressAclHole(sw,
                                                    cacls,
                                                    nextCompiledAcl,
                                                    apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fm10000NonDisruptMoveEgressAcl(sw,
                                                 cacls,
                                                 nextCompiledAcl,
                                                 nextCompiledAcl->chunk - 1,
                                                 apply);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            break;
        }
    }

    /* lowest precedence position and no chunk available */
    if (chunkPos < 0)
    {
        err = FM_ERR_ACLS_TOO_BIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Create a blank ACL */
    compiledAcl = fmAlloc(sizeof(fm_fm10000CompiledAcl));
    if (compiledAcl == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_CLEAR(*compiledAcl);

    compiledAcl->aclKeepUnusedKeys = acl->aclKeepUnusedKeys;
    compiledAcl->internal = acl->internal;
    compiledAcl->aclParts = 0;
    compiledAcl->firstAclPart = TRUE;
    compiledAcl->aclNum = aclNumber;
    compiledAcl->numRules = 0;
    compiledAcl->chunk = chunkPos;
    compiledAcl->numChunk = 1;
    compiledAcl->aclInstance = FM_ACL_NO_INSTANCE;
    compiledAcl->sliceInfo.validLow = TRUE;
    compiledAcl->sliceInfo.validHigh = TRUE;
    compiledAcl->sliceInfo.selects = compiledAcl->muxSelect;
    compiledAcl->sliceInfo.caseLocation = compiledAcl->caseLocation;
    fmTreeInit(&compiledAcl->rules);

    compiledAcl->portSetId = fmAlloc(sizeof(fm_tree));
    if (compiledAcl->portSetId == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    fmTreeInit(compiledAcl->portSetId);

    fmTranslateAclScenario(sw,
                           acl->scenarios,
                           &compiledAcl->sliceInfo.validScenarios,
                           TRUE);

    /* Extract the egress port configured from this ACL and
     * configure the portSet accordingly. */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        err = fmGetAclAssociatedPort(sw,
                                     acl,
                                     port,
                                     FM_ACL_TYPE_EGRESS,
                                     &bitValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (bitValue)
        {
            /* Use the portSet tree to store the egress port configured. */
            err = fmTreeInsert(compiledAcl->portSetId,
                               port,
                               NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    fmTreeIterInit(&itAcl, &cacls->egressAcl);
    err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl);
    /* Other Egress ACL are already defined, use the actual slice allocation */
    if (err == FM_OK)
    {
        compiledAcl->sliceInfo.keyStart = nextCompiledAcl->sliceInfo.keyStart;
        compiledAcl->sliceInfo.keyEnd = nextCompiledAcl->sliceInfo.keyEnd;
        compiledAcl->sliceInfo.actionEnd = nextCompiledAcl->sliceInfo.actionEnd;
        FM_MEMCPY_S( compiledAcl->muxSelect,
                     sizeof(compiledAcl->muxSelect),
                     nextCompiledAcl->muxSelect,
                     sizeof(nextCompiledAcl->muxSelect));
    }
    /* First Egress ACL to be defined, need to allocate at least one condition
     * slice. */
    else
    {
        fmTreeIterInit(&itAcl, &cacls->ingressAcl);
        err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl);
        /* Ingress ACL use the needed slices? */
        if (err == FM_OK)
        {
            /* Find if this ingress ACL is located at the last position */
            if (nextCompiledAcl->sliceInfo.actionEnd == FM10000_ACL_EGRESS_SLICE_POS)
            {
                err = fm10000NonDisruptAddIngAclHole(sw, cacls, NULL, apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        fmInitializeConcreteKey(compiledAcl->muxSelect);

        compiledAcl->sliceInfo.keyStart  = FM10000_ACL_EGRESS_SLICE_POS;
        compiledAcl->sliceInfo.keyEnd    = FM10000_ACL_EGRESS_SLICE_POS;
        compiledAcl->sliceInfo.actionEnd = FM10000_ACL_EGRESS_SLICE_POS;

        fmUpdateValidSlice(&compiledAcl->sliceInfo, cacls);

        if (apply)
        {
            err = fmResetFFUSlice(sw, &compiledAcl->sliceInfo);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Translate the virtual mux selection to a concrete one. */
            fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelect);

            muxSelectsPtrs = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = tmpMuxSelect;
            validScenariosTmp = compiledAcl->sliceInfo.validScenarios;
            compiledAcl->sliceInfo.validScenarios = 0xffffffff;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.validScenarios = validScenariosTmp;
            compiledAcl->sliceInfo.selects = muxSelectsPtrs;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Activate the new slices in hardware. */
            err = fmUpdateMasterValid(sw, cacls);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    cacls->chunkValid |= 1 << (compiledAcl->chunk);

    if (apply)
    {
        err = fmSetEaclChunkCfg(sw,
                                compiledAcl->chunk,
                                compiledAcl->sliceInfo.validScenarios,
                                TRUE,
                                compiledAcl->portSetId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmUpdateMasterValid(sw, cacls);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeInsert(&cacls->egressAcl, aclNumber, compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

ABORT:

    if (compiledAcl != NULL)
    {
        fmFreeCompiledAcl(compiledAcl);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddEgressAcl */


/*****************************************************************************/
/** fm10000NonDisruptAddIngAcl
 * \ingroup intAcl
 *
 * \desc            This function try to add an ACL that is present into
 *                  the actual ACL configuration but not in the ingress
 *                  compiled ACL structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       acl points to the defined acl that contain all the
 *                  configuration to define.
 *
 * \param[in]       aclNumber is the acl Id to add.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddIngAcl(fm_int                  sw,
                                     fm_fm10000CompiledAcls *cacls,
                                     fm_acl *                acl,
                                     fm_int                  aclNumber,
                                     fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itAcl;
    fm_uint64 nextKey;
    fm_fm10000CompiledAcl *prevCompiledAcl;
    fm_fm10000CompiledAcl *nextCompiledAcl;
    fm_int slicePos;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_byte tmpMuxSelect[FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte *muxSelectsPtrs;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "acl = %p, "
                 "aclNumber = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 (void*) acl,
                 aclNumber,
                 apply);

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* The next condition limit can be an egress acl. */
    fmTreeIterInit(&itAcl, &cacls->egressAcl);
    err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl);
    if (err == FM_OK)
    {
        slicePos = nextCompiledAcl->sliceInfo.keyStart - 1;
    }
    /* No egress acl, the limit is definitively the configured FFU
     * slice allocation. */
    else
    {
        slicePos = lastAclSlice;
    }

    prevCompiledAcl = NULL;

    for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &nextKey, (void**) &nextCompiledAcl)) == FM_OK ; )
    {
        if (nextCompiledAcl->aclNum < aclNumber)
        {
            slicePos = nextCompiledAcl->sliceInfo.keyStart - 1;
        }
        else
        {
            /* Free one condition + action slice. */
            while (((cacls->sliceValid & (1 << slicePos)) |
                    (cacls->actionValid & (1 << slicePos))) != 0)
            {
                err = fm10000NonDisruptAddIngAclHole(sw,
                                                     cacls,
                                                     prevCompiledAcl,
                                                     apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            break;
        }

        prevCompiledAcl = nextCompiledAcl;
    }

    /* Non Disruptive ACL does not supports Instance for now */
    if (acl->instance != FM_ACL_NO_INSTANCE)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Create a blank ACL */
    compiledAcl = fmAlloc(sizeof(fm_fm10000CompiledAcl));
    if (compiledAcl == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_CLEAR(*compiledAcl);

    compiledAcl->aclKeepUnusedKeys = acl->aclKeepUnusedKeys;
    compiledAcl->internal = acl->internal;
    compiledAcl->aclParts = 0;
    compiledAcl->firstAclPart = TRUE;
    compiledAcl->aclNum = aclNumber;
    compiledAcl->numRules = 0;
    compiledAcl->numChunk = 1;
    compiledAcl->aclInstance = acl->instance;
    compiledAcl->sliceInfo.validLow = TRUE;
    compiledAcl->sliceInfo.validHigh = TRUE;
    compiledAcl->sliceInfo.selects = compiledAcl->muxSelect;
    compiledAcl->sliceInfo.caseLocation = compiledAcl->caseLocation;
    fmTreeInit(&compiledAcl->rules);

    compiledAcl->portSetId = fmAlloc(sizeof(fm_tree));
    if (compiledAcl->portSetId == NULL)
    {
        fmFreeCompiledAcl(compiledAcl);
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    fmTreeInit(compiledAcl->portSetId);

    err = fmTreeInsert(&cacls->ingressAcl,
                       FM_ACL_GET_MASTER_KEY(aclNumber),
                       compiledAcl);
    if (err != FM_OK)
    {
        fmFreeCompiledAcl(compiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    fmTranslateAclScenario(sw,
                           acl->scenarios,
                           &compiledAcl->sliceInfo.validScenarios,
                           FALSE);
    fmInitializeConcreteKey(compiledAcl->muxSelect);

    if (firstAclSlice > slicePos)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_ACLS_TOO_BIG);
    }

    compiledAcl->sliceInfo.keyStart  = slicePos;
    compiledAcl->sliceInfo.keyEnd    = slicePos;
    compiledAcl->sliceInfo.actionEnd = slicePos;

    fmUpdateValidSlice(&compiledAcl->sliceInfo, cacls);

    if (acl->numberOfPorts[FM_ACL_TYPE_INGRESS])
    {
        err = fmTreeInsert(compiledAcl->portSetId,
                           FM10000_SPECIAL_PORT_PER_ACL_KEY,
                           NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* This ACL is associated with a set of ports. */
        err = InsertGlobalPortSet(sw, cacls, compiledAcl, apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (apply)
    {
        /* Translate the virtual mux selection to a concrete one. */
        fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelect);

        muxSelectsPtrs = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = tmpMuxSelect;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.selects = muxSelectsPtrs;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Activate the new slices in hardware. */
        err = fmUpdateMasterValid(sw, cacls);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddIngAcl */


/*****************************************************************************/
/** fm10000NonDisruptAddAcls
 * \ingroup intAcl
 *
 * \desc            This function try to add any ACL or ACL rule that are
 *                  present into the actual ACL configuration but not in the
 *                  compiled ACL structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 * 
 * \param[in]       internalAcl is the internal ACL to compile/apply. Setting
 *                  this value to -1 will compile/apply all the non internal
 *                  ACL.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptAddAcls(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls,
                                   fm_int                  internalAcl,
                                   fm_bool                 apply)
{
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    fm_uint64 aclNumKey;
    fm_uint64 ruleNumber;
    void *nextValue;
    fm_aclInfo *info;
    fm_status err = FM_OK;
    fm_acl *acl;
    fm_aclRule *rule;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAcl* compiledAclTmp;
    fm_int lastRuleInserted;
    fm_int nextRuleToInsert;
    fm_int i;
    fm_int aclPartFreeSize;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "internalAcl = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 internalAcl,
                 apply);

    info = &switchPtr->aclInfo;

    /* Scan all the ACL from the actual configuration and try to match the ACL
     * Id into the proper compiled ACL structure. */
    for (fmTreeIterInit(&itAcl, &info->acls) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        acl = (fm_acl*) nextValue;

        /* Add this acl? */
        if (fmTreeSize(&acl->rules) == 0)
        {
            continue;
        }

        /* Internal ACL must only be processed if specified */
        if (acl->internal)
        {
            if ( (internalAcl == -1) || (internalAcl != (fm_int)aclNumber) )
            {
                continue;
            }
        }
        /* Non Internal ACL must only be processed if internalAcl == -1 */
        else
        {
            if (internalAcl != -1)
            {
                continue;
            }
        }

        /* ACL with assigned port will be skip if no ports are part of that
         * switch. This situation only make sense on SWAG. */
        if ( ((acl->aclPortType == FM_ACL_PORT_TYPE_INGRESS) &&
              (acl->numberOfPorts[FM_ACL_TYPE_INGRESS] == 0)) ||
             ((acl->aclPortType == FM_ACL_PORT_TYPE_EGRESS) &&
              (acl->numberOfPorts[FM_ACL_TYPE_EGRESS] == 0)) )
        {
            continue;
        }

        /* Non Disruptive ACL does not supports Instance for now */
        if (acl->instance != FM_ACL_NO_INSTANCE)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        if (acl->numberOfPorts[FM_ACL_TYPE_EGRESS])
        {
            err = fmTreeFind(&cacls->egressAcl, aclNumber, (void**) &compiledAcl);
            if (err == FM_OK)
            {
                for (fmTreeIterInit(&itRule, &acl->addedRules) ;
                     (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) == FM_OK ; )
                {
                    err = fmTreeFind(&acl->rules, ruleNumber, (void**) &rule);
                    if (err == FM_OK)
                    {
                        err = fmTreeFind(&compiledAcl->rules, ruleNumber, &nextValue);
                        if (err == FM_ERR_NOT_FOUND)
                        {
                            /* Add this egress acl-rule */
                            err = fm10000NonDisruptAddEgressAclRule(sw,
                                                                    cacls,
                                                                    compiledAcl,
                                                                    rule,
                                                                    ruleNumber,
                                                                    apply);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        else if (err != FM_OK)
                        {
                            goto ABORT;
                        }
                    }
                    else if (err != FM_ERR_NOT_FOUND)
                    {
                        goto ABORT;
                    }
                }
                if (err != FM_ERR_NO_MORE)
                {
                    goto ABORT;
                }

                if (apply && fmTreeSize(&acl->addedRules))
                {
                    fmTreeDestroy(&acl->addedRules, NULL);
                    fmTreeInit(&acl->addedRules);
                }
            }
            else if (err == FM_ERR_NOT_FOUND)
            {
                err = fm10000NonDisruptAddEgressAcl(sw,
                                                    cacls,
                                                    acl,
                                                    aclNumber,
                                                    apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Catch the added rule */
                fmTreeIterInitFromKey(&itAcl, &info->acls, aclNumber);
            }
            else
            {
                goto ABORT;
            }
        }
        else
        {
            /* Ingress ACLs */

            aclNumKey = FM_ACL_GET_MASTER_KEY(aclNumber);

            err = fmTreeFind(&cacls->ingressAcl, aclNumKey, (void**) &compiledAcl);
            if (err == FM_OK)
            {
                /* Single Part handling or single rule addition */
                if ((fmTreeSize(&acl->addedRules) <=
                     (FM10000_MAX_RULE_PER_ACL_PART - compiledAcl->numRules)) ||
                    (fmTreeSize(&acl->addedRules) <= 1))
                {
                    for (fmTreeIterInitBackwards(&itRule, &acl->addedRules) ;
                         (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) == FM_OK ; )
                    {
                        err = fmTreeFind(&acl->rules, ruleNumber, (void**) &rule);
                        if (err == FM_OK)
                        {
                            err = fmGetAclNumKey(&cacls->ingressAcl,
                                                 aclNumber,
                                                 ruleNumber,
                                                 &aclNumKey);
                            if (err == FM_ERR_NOT_FOUND)
                            {
                                /* Add this ingress acl-rule */
                                err = fm10000NonDisruptAddIngAclRule(sw,
                                                                     cacls,
                                                                     aclNumber,
                                                                     rule,
                                                                     ruleNumber,
                                                                     apply);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                            }
                            else if (err != FM_OK)
                            {
                                goto ABORT;
                            }
                        }
                        else if (err != FM_ERR_NOT_FOUND)
                        {
                            goto ABORT;
                        }
                    }
                    if (err != FM_ERR_NO_MORE)
                    {
                        goto ABORT;
                    }
                }
                /* Multi Part handling must use optimize insertion order for
                 * performance enhancement */
                else
                {
                    lastRuleInserted = -1;

                    fmTreeIterInit(&itRule, &acl->addedRules);
                    err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    nextRuleToInsert = ruleNumber;

                    while (nextRuleToInsert >= 0)
                    {
                        aclPartFreeSize = 0;
                        compiledAclTmp = compiledAcl;

                        for (i = 1 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
                        {
                            if (compiledAcl->numRules)
                            {
                                fmTreeIterInitBackwards(&itRule, &compiledAclTmp->rules);
                                err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                /* Rule must be added in subsequent part */
                                if (((fm_int)ruleNumber < nextRuleToInsert) &&
                                    (compiledAclTmp->numRules == FM10000_MAX_RULE_PER_ACL_PART))
                                {
                                    err = fmTreeFind(&cacls->ingressAcl,
                                                     FM_ACL_GET_MASTER_KEY(aclNumber) + i,
                                                     (void**) &compiledAclTmp);
                                    if (err == FM_ERR_NOT_FOUND)
                                    {
                                        /* Must be added to the last part */
                                        err = FM_OK;
                                        aclPartFreeSize = FM10000_MAX_RULE_PER_ACL_PART;
                                        break;
                                    }
                                    else if (err != FM_OK)
                                    {
                                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                    }
                                }
                                else
                                {
                                    /* Found the right part */
                                    aclPartFreeSize = FM10000_MAX_RULE_PER_ACL_PART -
                                                      compiledAclTmp->numRules;
                                    break;
                                }
                            }
                            /* Empty Part */
                            else
                            {
                                aclPartFreeSize = FM10000_MAX_RULE_PER_ACL_PART;
                                break;
                            }
                        }

                        /* Add only one rule if the part is full */
                        if (aclPartFreeSize == 0)
                        {
                            aclPartFreeSize = 1;
                        }

                        /* Reorder the insertion if multiple free location exist
                         * on this part */
                        err = fmTreeIterInitFromKey(&itRule,
                                                    &acl->addedRules,
                                                    nextRuleToInsert);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        for (i = 0 ; i < aclPartFreeSize ; i++)
                        {
                            err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue);
                            if (err == FM_OK)
                            {
                                nextRuleToInsert = ruleNumber;
                            }
                            else if (err == FM_ERR_NO_MORE)
                            {
                                err = FM_OK;
                                break;
                            }
                            else
                            {
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                            }
                        }

                        /* Do the insertion */
                        for (fmTreeIterInitFromKeyBackwards(&itRule, &acl->addedRules, nextRuleToInsert) ;
                             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) == FM_OK ; )
                        {
                            if ((fm_int)ruleNumber > lastRuleInserted)
                            {
                                err = fmTreeFind(&acl->rules, ruleNumber, (void**) &rule);
                                if (err == FM_OK)
                                {
                                    err = fmGetAclNumKey(&cacls->ingressAcl,
                                                         aclNumber,
                                                         ruleNumber,
                                                         &aclNumKey);
                                    if (err == FM_ERR_NOT_FOUND)
                                    {
                                        /* Add this ingress acl-rule */
                                        err = fm10000NonDisruptAddIngAclRule(sw,
                                                                             cacls,
                                                                             aclNumber,
                                                                             rule,
                                                                             ruleNumber,
                                                                             apply);
                                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                    }
                                    else if (err != FM_OK)
                                    {
                                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                    }
                                }
                                else if (err != FM_ERR_NOT_FOUND)
                                {
                                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                }
                            }
                            else
                            {
                                err = FM_ERR_NO_MORE;
                                break;
                            }
                        }
                        if (err != FM_ERR_NO_MORE)
                        {
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        lastRuleInserted = nextRuleToInsert;

                        err = fmTreeIterInitFromSuccessor(&itRule,
                                                          &acl->addedRules,
                                                          nextRuleToInsert);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        /* Are we done? */
                        err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue);
                        if (err == FM_OK)
                        {
                            nextRuleToInsert = ruleNumber;
                        }
                        else if (err == FM_ERR_NO_MORE)
                        {
                            err = FM_OK;
                            nextRuleToInsert = -1;
                        }
                        else
                        {
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                    }
                }

                if (apply && fmTreeSize(&acl->addedRules))
                {
                    fmTreeDestroy(&acl->addedRules, NULL);
                    fmTreeInit(&acl->addedRules);
                }
            }
            else if (err == FM_ERR_NOT_FOUND)
            {
                err = fm10000NonDisruptAddIngAcl(sw,
                                                 cacls,
                                                 acl,
                                                 aclNumber,
                                                 apply);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Catch the added rule */
                fmTreeIterInitFromKey(&itAcl, &info->acls, aclNumber);
            }
            else
            {
                goto ABORT;
            }
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }
    err = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptAddAcls */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000NonDisruptCompile
 * \ingroup intAcl
 *
 * \desc            This function try to modify the compiled ACL structure
 *                  to reflect the modification done in the unapplied ACL
 *                  configuration in a non disruptive way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 * 
 * \param[in]       internalAcl is the internal ACL to compile/apply. Setting
 *                  this value to -1 will compile/apply all the non internal
 *                  ACL.
 *
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptCompile(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls,
                                   fm_int                  internalAcl,
                                   fm_bool                 apply)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p, "
                 "internalAcl = %d, "
                 "apply = %d\n",
                 sw,
                 (void*) cacls,
                 internalAcl,
                 apply);


    /* First stage is to remove any ACL or ACL rule that are present into
     * the compiled ACL structure but not in the actual ACL configuration. */
    err = fm10000NonDisruptRemAcls(sw, cacls, internalAcl, apply);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Internal ACL compile/apply does not manage mappers */
    if (internalAcl == -1)
    {
        err = fm10000NonDisruptRemMappers(sw, cacls, apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000NonDisruptCleanRoutes(cacls, -1, -1, -1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Second stage is to add any ACL or ACL rule that are present into the
     * actual ACL configuration but not into the compiled ACL structure. */

    /* Internal ACL compile/apply does not manage mappers */
    if (internalAcl == -1)
    {
        err = fm10000NonDisruptAddMappers(sw, cacls, apply);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000NonDisruptAddAcls(sw, cacls, internalAcl, apply);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

ABORT:


    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptCompile */


/*****************************************************************************/
/** fm10000NonDisruptQuickUpdate
 * \ingroup intAcl
 *
 * \desc            This function try to quickly updates a rule if all the
 *                  required condition keys and action space required is
 *                  currently available.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number (0 to ''FM_MAX_ACLS'').
 *
 * \param[in]       rule is the rule number (0 to ''FM_MAX_ACL_RULES'').
 *
 * \return          FM_ERR_INVALID_ACL_RULE if full update needed
 *                  FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptQuickUpdate(fm_int sw, fm_int acl, fm_int rule)
{
    fm_status                  err = FM_OK;
    fm_aclInfo *               info;
    fm_switch *                switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *           switchExt = (fm10000_switch *) switchPtr->extension;
    fm_acl *                   aclEntry;
    fm_aclRule *               aclRuleEntry;
    fm_fm10000CompiledAcl *    compiledAcl;
    fm_uint64                  aclNumKey;
    fm_tree                    portSetKey;
    fm_tree                    abstractKey;
    fm_tree                    remainAbstractKey;
    fm_treeIterator            itPortSet;
    fm_uint64                  portSetNumber;
    void *                     nextValue;
    fm_int                     numActionsSlices;
    fm_fm10000CompiledAclRule *compiledAclRule;
    fm_int                     i;
    fm_int                     compiledAclRsrvdActSlice;
    fm_uint16                  tmpRulePolicerIndex[FM_FM10000_POLICER_BANK_MAX];
    fm_uintptr                 policerIndex;
    fm_individualPolicer *     policerEntry;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_ffuPolicerState         committed;
    fm_ffuPolicerState         excess;
    fm_uint32                  policersBankClean = 0;
    fm_bool                    InitPortSetsClean = FALSE;
    fm_int                     ecmpGrp;


    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "acl = %d, "
                 "rule = %d\n",
                 sw,
                 acl,
                 rule);


    info             = &switchPtr->aclInfo;
    numActionsSlices = 0;

    FM_CLEAR(abstractKey);
    FM_CLEAR(portSetKey);
    FM_CLEAR(remainAbstractKey);

    /* Grab the high level acl/rule entry */
    err = fmTreeFind(&info->acls, acl, (void**) &aclEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmTreeFind(&aclEntry->rules, rule, (void**) &aclRuleEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Non Disruptive ACL does not supports Instance for now */
    if (aclEntry->instance != FM_ACL_NO_INSTANCE)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Find its compiledAcl/rule counterpart */
    if (aclEntry->numberOfPorts[FM_ACL_TYPE_EGRESS])
    {
        err = fmTreeFind(&switchExt->appliedAcls->egressAcl,
                         acl,
                         (void**) &compiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    else
    {
        /* Ingress ACL */
        err = fmGetAclNumKey(&switchExt->appliedAcls->ingressAcl,
                             acl,
                             rule,
                             &aclNumKey);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmTreeFind(&switchExt->appliedAcls->ingressAcl,
                         aclNumKey,
                         (void**) &compiledAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        compiledAclRsrvdActSlice = compiledAcl->sliceInfo.actionEnd -
                                   compiledAcl->sliceInfo.keyEnd + 1;

        err = fmCountActionSlicesNeeded(sw,
                                        NULL,
                                        aclRuleEntry,
                                        &numActionsSlices);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Updated action needs more slice than currently allocated, call a
         * full recompile/apply sequence. */
        if (numActionsSlices > compiledAclRsrvdActSlice)
        {
            err = FM_ERR_INVALID_ACL_RULE;
            goto ABORT;
        }
    }

    fmTreeInit(&abstractKey);
    fmTreeInit(&portSetKey);

    /* Translate all the rule condition in abstract key. */
    err = fmFillAbstractKeyTree(sw,
                                NULL,
                                aclRuleEntry,
                                &abstractKey,
                                &portSetKey);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* PortSet has been defined as condition */
    if (fmTreeSize(&portSetKey))
    {
        fmTreeIterInit(&itPortSet, &portSetKey);
        err = fmTreeIterNext(&itPortSet, &portSetNumber, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Only allow portSet that are already defined for this ACL. If not,
         * call a full recompile/apply sequence. */
        err = fmTreeFind(compiledAcl->portSetId, portSetNumber, &nextValue);
        if (err != FM_OK)
        {
            err = FM_ERR_INVALID_ACL_RULE;
            goto ABORT;
        }
    }

    fmTreeInit(&remainAbstractKey);
    /* Extract all the abstract key that are already supported in this compiled
     * acl and return only the one that needs to be added. */
    err = FindUnconfiguredAbstract(compiledAcl,
                                   &abstractKey,
                                   &remainAbstractKey);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* This Quick Update function does not support new keys. If so, call a
     * full recompile/apply sequence. */
    if (fmTreeSize(&remainAbstractKey))
    {
        err = FM_ERR_INVALID_ACL_RULE;
        goto ABORT;
    }

    err = fmTreeFind(&compiledAcl->rules, rule, (void**) &compiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (aclEntry->numberOfPorts[FM_ACL_TYPE_EGRESS])
    {
        /* Configure the condition key and key mask based on the entered
         * acl rule. */
        err = fmConfigureConditionKey(sw,
                                      NULL,
                                      aclRuleEntry,
                                      rule,
                                      compiledAcl,
                                      NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Configure the action data based on the entered acl rule. */
        err = fmConfigureEgressActionData(aclRuleEntry, NULL, compiledAclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fm10000SetFFUEaclAction(sw,
                                      compiledAclRule->physicalPos,
                                      compiledAclRule->egressDropActions,
                                      TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        err = fm10000SetFFURule(sw,
                                &compiledAcl->sliceInfo,
                                compiledAclRule->physicalPos,
                                compiledAclRule->valid,
                                compiledAclRule->sliceKey,
                                compiledAclRule->actions,
                                TRUE, /* Live */
                                TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    else
    {

        /* Quick Update only support count/police action if the original rule
         * already used it. If new count/police action are entered, call a
         * full recompile/apply sequence. */
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            tmpRulePolicerIndex[i] = 0;
        }

        if (aclRuleEntry->action & FM_ACL_ACTIONEXT_POLICE)
        {
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                if (fmTreeFind(&switchExt->appliedAcls->policersId[i],
                               aclRuleEntry->param.policer,
                               (void **) &policerIndex) == FM_OK)
                {
                    if (policerIndex != compiledAclRule->policerIndex[i])
                    {
                        err = FM_ERR_INVALID_ACL_RULE;
                        goto ABORT;
                    }

                    /* Make sure the policer value entered into the rule
                     * parameter was defined into the policer API. */
                    err = fmTreeFind(&switchPtr->policerInfo.policers,
                                     aclRuleEntry->param.policer,
                                     (void*) &policerEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        
                    /* Retrieve policer configurationg from compiled ACL */
                    err = fmTreeFind(&switchExt->appliedAcls->policers[i].policerEntry,
                                     compiledAclRule->policerIndex[i],
                                     (void*) &compiledPolEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                     
                    /* Fill temporary policer config based
                     * on configuration from the policer API */
                    err = fm10000ConvertPolicerAttributeToState(&policerEntry->attributes,
                                                                &committed,
                                                                &excess);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Check if policer config has changed. */
                    if ( (compiledPolEntry->committed.action != committed.action) ||
                         (compiledPolEntry->committed.rateMantissa != committed.rateMantissa) ||
                         (compiledPolEntry->committed.rateExponent != committed.rateExponent) ||
                         (compiledPolEntry->committed.capacityMantissa != committed.capacityMantissa) ||
                         (compiledPolEntry->committed.capacityExponent != committed.capacityExponent) ||
                         (compiledPolEntry->excess.action != excess.action) ||
                         (compiledPolEntry->excess.rateMantissa != excess.rateMantissa) ||
                         (compiledPolEntry->excess.rateExponent != excess.rateExponent) ||
                         (compiledPolEntry->excess.capacityMantissa != excess.capacityMantissa) ||
                         (compiledPolEntry->excess.capacityExponent != excess.capacityExponent) )
                    {
                         err = FM_ERR_INVALID_ACL_RULE;
                         goto ABORT;
                    }

                    /* Original Police action found. */
                    tmpRulePolicerIndex[i] = policerIndex;
                    break;
                }
            }
            if (i == FM_FM10000_POLICER_BANK_MAX)
            {
                err = FM_ERR_INVALID_ACL_RULE;
                goto ABORT;
            }

        }
        else
        {
            /* Check if action FM_ACL_ACTIONEXT_POLICE has been removed. */
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                if (compiledAclRule->policerIndex[i])
                {
                    if (fmTreeFind(&switchExt->appliedAcls->policers[i].policerEntry,
                                   compiledAclRule->policerIndex[i],
                                   (void*) &compiledPolEntry) == FM_OK)
                    {
                        if (!compiledPolEntry->countEntry)
                        {
                            policersBankClean |= (1 << i);
                            break;
                        }
                    }
                }
            }
        }

        if (aclRuleEntry->action & FM_ACL_ACTIONEXT_COUNT)
        {
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                if (compiledAclRule->policerIndex[i])
                {
                    if (fmTreeFind(&switchExt->appliedAcls->policers[i].policerEntry,
                                   compiledAclRule->policerIndex[i],
                                   (void*) &compiledPolEntry) == FM_OK)
                    {
                        if (compiledPolEntry->countEntry)
                        {
                            /* Original Count action found. */
                            tmpRulePolicerIndex[i] = compiledAclRule->policerIndex[i];
                            break;
                        }
                    }
                }
            }
            if (i == FM_FM10000_POLICER_BANK_MAX)
            {
                err = FM_ERR_INVALID_ACL_RULE;
                goto ABORT;
            }
        }
        else
        {
            /* Check if action FM_ACL_ACTIONEXT_COUNT has been removed. */
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                if (compiledAclRule->policerIndex[i])
                {
                    if (fmTreeFind(&switchExt->appliedAcls->policers[i].policerEntry,
                                   compiledAclRule->policerIndex[i],
                                   (void*) &compiledPolEntry) == FM_OK)
                    {
                        if (compiledPolEntry->countEntry)
                        {
                            policersBankClean |= (1 << i);
                            break;
                        }
                    }
                }
            }
        }

        if (policersBankClean)
        {
            err = fm10000NonDisruptCleanPolicerRules(sw,
                                                     switchExt->appliedAcls,
                                                     compiledAclRule,
                                                     policersBankClean,
                                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        if (compiledAclRule->portSetId != FM_PORT_SET_ALL)
        {
            InitPortSetsClean = TRUE;
        }

        compiledAclRule->portSetId = FM_PORT_SET_ALL;
        /* Configure the condition key and key mask based on the entered
         * acl rule. */
        err = fmConfigureConditionKey(sw,
                                      NULL,
                                      aclRuleEntry,
                                      rule,
                                      compiledAcl,
                                      NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Reset Count/Policer counter of updated rule. */
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            compiledAclRule->policerIndex[i] = tmpRulePolicerIndex[i];

            if (tmpRulePolicerIndex[i])
            {
                err = fm10000SetPolicerCounter(sw,
                                               i,
                                               compiledAclRule->policerIndex[i],
                                               FM_LITERAL_64(0),
                                               FM_LITERAL_64(0));
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        for (i = 0 ; i < compiledAclRule->numActions ; i++)
        {
            compiledAclRule->actions[i].action = FM_FFU_ACTION_NOP;
            compiledAclRule->actions[i].precedence = 0;
            compiledAclRule->actions[i].counter = 0;
            compiledAclRule->actions[i].bank = 0;
        }

        compiledAclRule->numActions = numActionsSlices;
        /* Configure the action data based on the entered acl rule. */
        err = fmConfigureActionData(sw,
                                    NULL,
                                    switchExt->appliedAcls->policers,
                                    &switchExt->appliedAcls->ecmpGroups,
                                    aclRuleEntry,
                                    compiledAcl,
                                    compiledAclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fm10000SetFFURule(sw,
                                &compiledAcl->sliceInfo,
                                compiledAclRule->physicalPos,
                                compiledAclRule->valid,
                                compiledAclRule->sliceKey,
                                compiledAclRule->actions,
                                TRUE, /* Live */
                                TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (InitPortSetsClean)
        {
            err = CleanPortSet(sw, switchExt->appliedAcls, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        ecmpGrp = -1;
        if (aclRuleEntry->action & FM_ACL_ACTIONEXT_ROUTE)
        {
            ecmpGrp = aclRuleEntry->param.groupId;
        }

        err = fm10000NonDisruptCleanRoutes(switchExt->appliedAcls,
                                           acl,
                                           rule,
                                           ecmpGrp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    if (fmTreeIsInitialized(&abstractKey))
    {
        fmTreeDestroy(&abstractKey, NULL);
    }

    if (fmTreeIsInitialized(&portSetKey))
    {
        fmTreeDestroy(&portSetKey, NULL);
    }

    if (fmTreeIsInitialized(&remainAbstractKey))
    {
        fmTreeDestroy(&remainAbstractKey, NULL);
    }


    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptQuickUpdate */

