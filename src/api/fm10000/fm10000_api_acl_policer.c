/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_acl_policer.c
 * Creation Date:  September 9, 2013
 * Description:    ACL code related to the policer management for FM10000.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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


#define FM_POLICER_BANK_MAX_ENTRIES(bank, maxEntries)     \
    if ((bank) < FM10000_ACL_POLICER_4K_BANK_NUM)         \
    {                                                     \
        (maxEntries) = FM_FM10000_MAX_POLICER_4K_INDEX;   \
    }                                                     \
    else                                                  \
    {                                                     \
        (maxEntries) = FM_FM10000_MAX_POLICER_512_INDEX;  \
    }


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

extern void fmFreeCompiledPolicerEntry(void *value);

extern fm_status fmGetAclNumKey(fm_tree *  aclTree,
                                fm_int     acl,
                                fm_int     rule,
                                fm_uint64 *aclNumKey);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** PolicerColorSourceToFfuColorSource
 * \ingroup intAcl
 *
 * \desc            Converts a policer color source from the anonymous
 *                  enumeration defined in fm_api_policer.h to an
 *                  fm_ffuColorSource.
 *
 * \param[in]       policerColorSource is one of the values from
 *                  fm_api_policer.h.
 *
 * \param[out]      ffuColorSource is set to the equivalent value in the
 *                  fm_ffuColorSource enumeration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_POLICER if policerColorSource
 *                  is not valid.
 *
 *****************************************************************************/
static fm_status PolicerColorSourceToFfuColorSource(fm_int policerColorSource,
                                                    fm_ffuColorSource *ffuColorSource)
{
    switch (policerColorSource)
    {
        case FM_POLICER_COLOR_SRC_GREEN:
            *ffuColorSource = FM_COLOR_SOURCE_ASSUME_GREEN;
            return FM_OK;
#if 0
       case FM_POLICER_COLOR_SRC_DSCP:
           *ffuColorSource = FM_COLOR_SOURCE_DSCP;
           return FM_OK;
       case FM_POLICER_COLOR_SRC_SWPRI:
           *ffuColorSource = FM_COLOR_SOURCE_SWITCH_PRI;
           return FM_OK;
#endif
        default:
            return FM_ERR_INVALID_POLICER;

    }   /* end switch (policerColorSource) */

}   /* end PolicerColorSourceToFfuColorSource */


/*****************************************************************************/
/** PolicerBankMatch
 * \ingroup intAcl
 *
 * \desc            Analyze if the Policer Entry can be inserted into the
 *                  Policer Bank based on the current configuration.
 *
 * \param[in]       policerCfgBank refer to the policer bank configuration.
 *
 * \param[out]      policerCfgEntry refer to the policer entry configuration.
 *
 * \return          FM_OK if the entry can be inserted into the bank.
 * \return          FM_ERR_NO_POLICERS otherwise.
 *
 *****************************************************************************/
static fm_status PolicerBankMatch(fm_fm10000CompiledPolicers* policerCfgBank,
                                  fm_policerConfig *          policerCfgEntry)
{
    fm_ffuColorSource entryColorSource;
    fm_status         err;

    err = PolicerColorSourceToFfuColorSource(policerCfgEntry->colorSource,
                                             &entryColorSource);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* indexLastPolicer would be zero if this is the first policer configured */
    if ( (policerCfgBank->indexLastPolicer == 0) ||
         (policerCfgBank->ingressColorSource == entryColorSource) )
    {
        /* Entry doesn't need to initiate markdown */
        if ( (policerCfgEntry->mkdnDscp == FALSE) &&
             (policerCfgEntry->mkdnSwPri == FALSE) )
        {
            return FM_OK;
        }
        /* Policer bank still not configured for policing */
        else if ( (policerCfgBank->markDSCP == FALSE) &&
                  (policerCfgBank->markSwitchPri == FALSE) )
        {
            return FM_OK;
        }
        /* Entry and current policer bank configuration are identical */
        else if ( (policerCfgBank->markDSCP == policerCfgEntry->mkdnDscp) &&
                  (policerCfgBank->markSwitchPri == policerCfgEntry->mkdnSwPri) )
        {
            return FM_OK;
        }
    }

    return FM_ERR_NO_POLICERS;

}   /* end PolicerBankMatch */


/*****************************************************************************/
/** SelectPolicerBank
 * \ingroup intAcl
 *
 * \desc            This function select a policer bank that would be uses for
 *                  this specific compiledAclRule/entry tuple.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       entry points to the defined policer to configure or NULL
 *                  if the action is count.
 *
 * \param[in]       compiledAclRule points to the compiler ACL rule which
 *                  refer to this entered policer entry.
 * 
 * \param[out]      selectedBank will refer to the bank selected.
 * 
 * \param[in]       strictCount refer to the policer/counter mode configured.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SelectPolicerBank(fm_int                     sw,
                                   fm_fm10000CompiledAcls *   cacls,
                                   fm_individualPolicer *     entry,
                                   fm_fm10000CompiledAclRule *compiledAclRule,
                                   fm_int *                   selectedBank,
                                   fm_bool                    strictCount)
{
    fm_status              err = FM_OK;
    fm_int                 bank = 0;
    fm_int                 policerFree;
    fm_uint32              treeSize;
    fm_int                 i;
    void *                 value;
    fm_ffuOwnerType        owner;
    fm_uint32              maxPolicerEntries;
    fm_treeIterator        itAcl;
    fm_uint64              aclNumber;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAcl* compiledAclTmp;
    fm_uint32              validScenarios;
    fm_int*                aclNumElement;

    /* Fixed policer entry */
    if ( (entry != NULL) &&
         (entry->bank != FM_POLICER_BANK_AUTOMATIC) )
    {
        bank = entry->bank;

        err = fm10000SetPolicerOwnership(sw, FM_FFU_OWNER_ACL, bank);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = PolicerBankMatch(&cacls->policers[bank],
                               &entry->attributes);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmTreeFind(&cacls->policers[bank].acl,
                         compiledAclRule->aclNumber,
                         &value);
        if (err == FM_OK)
        {
            aclNumElement = (fm_int*) value;
            (*aclNumElement)++;
        }
        else if (err == FM_ERR_NOT_FOUND)
        {
            aclNumElement = (fm_int *) fmAlloc(sizeof(fm_int));
            if (aclNumElement == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            *aclNumElement = 1;
            err = fmTreeInsert(&cacls->policers[bank].acl,
                               compiledAclRule->aclNumber,
                               aclNumElement);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    else
    {
        if (strictCount)
        {
            /* Try to find a bank already reserved for this ACL */
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                FM_POLICER_BANK_MAX_ENTRIES(i, maxPolicerEntries);

                if ( (compiledAclRule->policerIndex[i] == 0) &&
                     (fmTreeFind(&cacls->policers[i].acl,
                                 compiledAclRule->aclNumber,
                                 &value) == FM_OK) &&
                     (fmTreeSize(&cacls->policers[i].policerEntry) < maxPolicerEntries) )
                {
                    /* Policer entry must match Policer Bank configuration */
                    if (entry)
                    {
                        err = PolicerBankMatch(&cacls->policers[i],
                                               &entry->attributes);
                    }
                    if (err == FM_OK)
                    {
                        /* Add new rules to this acl */
                        aclNumElement = (fm_int*) value;
                        (*aclNumElement)++;
                        break;
                    }
                    else if (err != FM_ERR_NO_POLICERS)
                    {
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    err = FM_OK;
                }
            }

            /* Try to merge this ACL with an existing one */
            err = fmTreeFind(&cacls->ingressAcl,
                             FM_ACL_GET_MASTER_KEY(compiledAclRule->aclNumber),
                             (void**) &compiledAcl);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (i == FM_FM10000_POLICER_BANK_MAX)
            {
                for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
                {
                    FM_POLICER_BANK_MAX_ENTRIES(i, maxPolicerEntries);
                    validScenarios = 0;

                    for (fmTreeIterInit(&itAcl, &cacls->policers[i].acl) ;
                         (err = fmTreeIterNext(&itAcl, &aclNumber, &value)) == FM_OK ; )
                    {
                        err = fmTreeFind(&cacls->ingressAcl,
                                         FM_ACL_GET_MASTER_KEY(aclNumber),
                                         (void**) &compiledAclTmp);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                        validScenarios |= compiledAclTmp->sliceInfo.validScenarios;

                    }
                    if (err != FM_ERR_NO_MORE)
                    {
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    err = FM_OK;

                    /* These ACLs are mutually exclusive */
                    if ( (validScenarios != 0) &&
                         ((validScenarios & compiledAcl->sliceInfo.validScenarios) == 0) &&
                         (compiledAclRule->policerIndex[i] == 0) &&
                         (fmTreeSize(&cacls->policers[i].policerEntry) < maxPolicerEntries) )
                    {
                        /* Policer entry must match Policer Bank configuration */
                        if (entry)
                        {
                            err = PolicerBankMatch(&cacls->policers[i],
                                                   &entry->attributes);
                        }
                        if (err == FM_OK)
                        {
                            aclNumElement = (fm_int *) fmAlloc(sizeof(fm_int));
                            if (aclNumElement == NULL)
                            {
                                err = FM_ERR_NO_MEM;
                                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                            }
                            *aclNumElement = 1;
                            err = fmTreeInsert(&cacls->policers[i].acl,
                                               compiledAclRule->aclNumber,
                                               aclNumElement);
                            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                            break;
                        }
                        else if (err != FM_ERR_NO_POLICERS)
                        {
                            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        err = FM_OK;
                    }
                }
            }

            /* Need to grab a new policer bank */
            if (i == FM_FM10000_POLICER_BANK_MAX)
            {
                for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
                {
                    err = fm10000GetPolicerOwnership(sw, &owner, i);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                    if ( (owner == FM_FFU_OWNER_NONE) ||
                         ((owner == FM_FFU_OWNER_ACL) &&
                          (fmTreeSize(&cacls->policers[i].acl) == 0)) )
                    {
                        err = fm10000SetPolicerOwnership(sw, FM_FFU_OWNER_ACL, i);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                        aclNumElement = (fm_int *) fmAlloc(sizeof(fm_int));
                        if (aclNumElement == NULL)
                        {
                            err = FM_ERR_NO_MEM;
                            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        *aclNumElement = 1;
                        err = fmTreeInsert(&cacls->policers[i].acl,
                                           compiledAclRule->aclNumber,
                                           aclNumElement);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                        break;
                    }
                }
            }

            /* All Banks already taken */
            if (i == FM_FM10000_POLICER_BANK_MAX)
            {
                err = FM_ERR_ACLS_TOO_BIG;
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            bank = i;
        }
        else
        {
            /* Find the policer bank with the most free space available.
             * Balancing the policer bank is prefered to make sure that every
             * bank have free entry in case of an acl rule with police and
             * count action. */
            policerFree = 0;
            bank = -1;
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                FM_POLICER_BANK_MAX_ENTRIES(i, maxPolicerEntries);

                err = fm10000GetPolicerOwnership(sw, &owner, i);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                treeSize = fmTreeSize(&cacls->policers[i].policerEntry);
                if ( (owner == FM_FFU_OWNER_ACL) &&
                     (compiledAclRule->policerIndex[i] == 0) &&
                     (treeSize < maxPolicerEntries) &&
                     (policerFree < (fm_int)(maxPolicerEntries - treeSize)) )
                {
                    /* Policer entry must match Policer Bank configuration */
                    if (entry)
                    {
                        err = PolicerBankMatch(&cacls->policers[i],
                                               &entry->attributes);
                    }
                    if (err == FM_OK)
                    {
                        policerFree = maxPolicerEntries - treeSize;
                        bank = i;
                    }
                    else if (err != FM_ERR_NO_POLICERS)
                    {
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    err = FM_OK;
                }
                else if ( (owner == FM_FFU_OWNER_NONE) &&
                          (policerFree == 0) )
                {
                    policerFree = -1;
                    bank = i;
                }
            }

            /* Not possible to insert this entry */
            if (bank == -1)
            {
                err = FM_ERR_ACLS_TOO_BIG;
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else if (policerFree == -1)
            {
                err = fm10000SetPolicerOwnership(sw, FM_FFU_OWNER_ACL, bank);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Link this ACL to the policer bank */
            err = fmTreeFind(&cacls->policers[bank].acl,
                             compiledAclRule->aclNumber,
                             &value);
            if (err == FM_OK)
            {
                aclNumElement = (fm_int*) value;
                (*aclNumElement)++;
            }
            else if (err == FM_ERR_NOT_FOUND)
            {
                aclNumElement = (fm_int *) fmAlloc(sizeof(fm_int));
                if (aclNumElement == NULL)
                {
                    err = FM_ERR_NO_MEM;
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                *aclNumElement = 1;
                err = fmTreeInsert(&cacls->policers[bank].acl,
                                   compiledAclRule->aclNumber,
                                   aclNumElement);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else
            {
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

    FM_POLICER_BANK_MAX_ENTRIES(bank, maxPolicerEntries);

    /* Indez 0 reserved */
    if (fmTreeSize(&cacls->policers[bank].policerEntry) >= maxPolicerEntries)
    {
        err = FM_ERR_ACLS_TOO_BIG;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Return bank selected */
    *selectedBank = bank;

    return err;

}   /* end SelectPolicerBank */


/*****************************************************************************/
/** AddPolicerEntry
 * \ingroup intAcl
 *
 * \desc            This function adds a policer entry into the selected bank
 *                  and properly updates policer and acls structure.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       entry points to the defined policer to configure or NULL
 *                  if the action is count.
 * 
 * \param[in]       policerId is the policer handler.
 *
 * \param[in,out]   compiledAclRule points to the compiler ACL rule which
 *                  refer to this entered policer entry.
 * 
 * \param[in]       selectedBank refer to the bank selected.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddPolicerEntry(fm_int                     sw,
                                 fm_fm10000CompiledAcls *   cacls,
                                 fm_individualPolicer *     entry,
                                 fm_int                     policerId,
                                 fm_fm10000CompiledAclRule *compiledAclRule,
                                 fm_int                     selectedBank)
{
    fm_status                       err = FM_OK;
    void *                          value;
    fm_uint32                       maxPolicerEntries;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_fm10000AclRule *             aclRule;
    fm_uintptr                      policerIndex;

    FM_POLICER_BANK_MAX_ENTRIES(selectedBank, maxPolicerEntries);

    /* Sanity check */
    if ( (compiledAclRule->policerIndex[selectedBank] != 0) ||
         (fmTreeFind(&cacls->policers[selectedBank].acl,
                     compiledAclRule->aclNumber,
                     &value) != FM_OK) ||
         (fmTreeSize(&cacls->policers[selectedBank].policerEntry) >= maxPolicerEntries) )
    {
        err = FM_FAIL;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    compiledPolEntry = (fm_fm10000CompiledPolicerEntry *) fmAlloc(sizeof(fm_fm10000CompiledPolicerEntry));
    if (compiledPolEntry == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    FM_CLEAR(*compiledPolEntry);
    fmDListInit(&compiledPolEntry->policerRules);

    /* Policer Entry */
    if (entry)
    {
        compiledPolEntry->countEntry = FALSE;
        compiledPolEntry->policerId = policerId;
        cacls->policers[selectedBank].indexLastPolicer++;

        /* Configure the Policer Bank based on the inserted attributes */
        err = PolicerColorSourceToFfuColorSource(entry->attributes.colorSource,
                                                 &cacls->policers[selectedBank].ingressColorSource);
        if (err != FM_OK)
        {
            fmFree(compiledPolEntry);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        if (entry->attributes.mkdnDscp)
        {
            cacls->policers[selectedBank].markDSCP = TRUE;
        }
        if (entry->attributes.mkdnSwPri)
        {
            cacls->policers[selectedBank].markSwitchPri = TRUE;
        }

        /* Translate Policer Attribute to CIR, EIR structure used at low level */
        err = fm10000ConvertPolicerAttributeToState(&entry->attributes,
                                                    &compiledPolEntry->committed,
                                                    &compiledPolEntry->excess);
        if (err != FM_OK)
        {
            fmFree(compiledPolEntry);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Fill PolicerId --> PolicerIndex mapping tree */
        policerIndex = cacls->policers[selectedBank].indexLastPolicer;
        err = fmTreeInsert(&cacls->policersId[selectedBank],
                           policerId,
                           (void*) policerIndex);
        if (err != FM_OK)
        {
            fmFree(compiledPolEntry);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    /* Counter Entry */
    else
    {
        /* Counter entries are filled by the bottom while policers are filled
         * starting with the top */
        compiledPolEntry->countEntry = TRUE;
        policerIndex = maxPolicerEntries -
                         (fmTreeSize(&cacls->policers[selectedBank].policerEntry) -
                          cacls->policers[selectedBank].indexLastPolicer);
    }

    /* Every Policer entry are mapped to at least one ACL/Rule tuple but can
     * be multiple if multiple rules police to a single ID. */
    aclRule = (fm_fm10000AclRule *) fmAlloc(sizeof(fm_fm10000AclRule));
    if (aclRule == NULL)
    {
        fmFree(compiledPolEntry);
        err = FM_ERR_NO_MEM;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    aclRule->aclNumber = compiledAclRule->aclNumber;
    aclRule->ruleNumber = compiledAclRule->ruleNumber;
    err = fmDListInsertEnd(&compiledPolEntry->policerRules,
                           (void*) aclRule);
    if (err != FM_OK)
    {
        fmFree(aclRule);
        fmFree(compiledPolEntry);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Fill the PolicerIndex --> CompiledPolicer structure mapping */
    err = fmTreeInsert(&cacls->policers[selectedBank].policerEntry,
                       policerIndex,
                       (void*) compiledPolEntry);
    if (err != FM_OK)
    {
        fmDListFreeWithDestructor(&compiledPolEntry->policerRules, fmFree);
        fmFree(compiledPolEntry);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    compiledAclRule->policerIndex[selectedBank] = policerIndex;


    return err;

}   /* end AddPolicerEntry */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000ConvertPolicerAttributeToState
 * \ingroup intAcl
 *
 * \desc            This function convert high level policer configuration
 *                  to low level structure.
 * 
 * \param[in]       attributes is the policer configuration to translate.
 * 
 * \param[out]      committed points to the commited low level structure to
 *                  fill.
 *
 * \param[out]      excess points to the excess low level structure to
 *                  fill.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConvertPolicerAttributeToState(fm_policerConfig *  attributes,
                                                fm_ffuPolicerState *committed,
                                                fm_ffuPolicerState *excess)
{
    fm_status err = FM_OK;

    switch(attributes->cirAction)
    {
        case FM_POLICER_ACTION_DROP:
            committed->action = FM_FFU_POLICER_ACTION_DROP;
            break;

        case FM_POLICER_ACTION_MKDN:
            committed->action = FM_FFU_POLICER_ACTION_MARK_DOWN;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000ConvertPolicerRate(attributes->cirRate,
                                    &committed->rateMantissa,
                                    &committed->rateExponent);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fm10000ConvertPolicerCapacity(attributes->cirCapacity,
                                        &committed->capacityMantissa,
                                        &committed->capacityExponent);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    switch(attributes->eirAction)
    {
        case FM_POLICER_ACTION_DROP:
            excess->action = FM_FFU_POLICER_ACTION_DROP;
            break;

        case FM_POLICER_ACTION_MKDN:
            excess->action = FM_FFU_POLICER_ACTION_MARK_DOWN;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fm10000ConvertPolicerRate(attributes->eirRate,
                                    &excess->rateMantissa,
                                    &excess->rateExponent);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fm10000ConvertPolicerCapacity(attributes->eirCapacity,
                                        &excess->capacityMantissa,
                                        &excess->capacityExponent);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    return err;

}   /* end fm10000ConvertPolicerAttributeToState */


/*****************************************************************************/
/** fm10000PreallocatePolicerBank
 * \ingroup intAcl
 *
 * \desc            This function preallocate policer entry that are not
 *                  flexible. Policer entry with fixed bank allocation and
 *                  acl rule with action police and count enter in this
 *                  category.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       rule points to the defined acl rule that contain the
 *                  policer or count action.
 *
 * \param[out]      compiledAclRule points to the compiler ACL rule which
 *                  refer to a possible entry that may need to be preallocate.
 * 
 * \param[in]       strictCount refer to the policer/counter mode configured.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000PreallocatePolicerBank(fm_int                     sw,
                                        fm_aclErrorReporter *      errReport,
                                        fm_fm10000CompiledAcls *   cacls,
                                        fm_aclRule *               rule,
                                        fm_fm10000CompiledAclRule *compiledAclRule,
                                        fm_bool                    strictCount)
{
    fm_status                       err = FM_OK;
    fm_policerInfo *                info;
    fm_individualPolicer *          entry;
    fm_uintptr                      policerIndex;
    fm_switch *                     switchPtr;
    fm_int                          i;
    fm_int                          selectedBank;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_fm10000AclRule *             aclRule;
    void *                          value;
    fm_int *                        aclNumElement;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    info  = &switchPtr->policerInfo;

    /* Prioritize unflexible allocation first. */
    if (rule->action & FM_ACL_ACTIONEXT_POLICE)
    {
        /* Make sure the policer value entered into the rule parameter was
         * defined into the policer API. */
        err = fmTreeFind(&info->policers, rule->param.policer, (void*) &entry);
        if (err != FM_OK)
        {
            fm10000FormatAclStatus(errReport, TRUE,
                                   "Policer Id %d not defined into the "
                                   "Policer API.\n", rule->param.policer);
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        /* Find if this policer Id was already been allocated. */
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            if (fmTreeFind(&cacls->policersId[i],
                           rule->param.policer,
                           (void **) &policerIndex) == FM_OK)
            {
                /* Refer this acl rule to this preallocated policer index. */
                compiledAclRule->policerIndex[i] = policerIndex;

                /* Link this bank to this ACL */
                err = fmTreeFind(&cacls->policers[i].acl,
                                 compiledAclRule->aclNumber,
                                 &value);
                if (err == FM_OK)
                {
                    aclNumElement = (fm_int*) value;
                    (*aclNumElement)++;
                }
                else if (err == FM_ERR_NOT_FOUND)
                {
                    aclNumElement = (fm_int *) fmAlloc(sizeof(fm_int));
                    if (aclNumElement == NULL)
                    {
                        err = FM_ERR_NO_MEM;
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    *aclNumElement = 1;
                    err = fmTreeInsert(&cacls->policers[i].acl,
                                       compiledAclRule->aclNumber,
                                       aclNumElement);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                /* Link this ACL/Rule to this policer entry */
                err = fmTreeFind(&cacls->policers[i].policerEntry,
                                 policerIndex,
                                 (void **) &compiledPolEntry);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                aclRule = (fm_fm10000AclRule *) fmAlloc(sizeof(fm_fm10000AclRule));
                if (aclRule == NULL)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_MEM);
                }
                aclRule->aclNumber = compiledAclRule->aclNumber;
                aclRule->ruleNumber = compiledAclRule->ruleNumber;
                err = fmDListInsertEnd(&compiledPolEntry->policerRules,
                                       (void*) aclRule);
                if (err != FM_OK)
                {
                    fmFree(aclRule);
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }

                break;
            }
        }
        /* Fixed policer entry are not flexible, make sure to find free space
         * for them first. */
        if ( (i == FM_FM10000_POLICER_BANK_MAX) &&
             (entry->bank != FM_POLICER_BANK_AUTOMATIC) )
        {
            if (entry->bank < FM_FM10000_POLICER_BANK_MAX)
            {
                err = SelectPolicerBank(sw,
                                        cacls,
                                        entry,
                                        compiledAclRule,
                                        &selectedBank,
                                        strictCount);
                if (err != FM_OK)
                {
                    if (err == FM_ERR_ACLS_TOO_BIG)
                    {
                        fm10000FormatAclStatus(errReport, TRUE,
                                               "Policer bank full.\n");
                    }
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    err = AddPolicerEntry(sw,
                                          cacls,
                                          entry,
                                          rule->param.policer,
                                          compiledAclRule,
                                          selectedBank);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                /* If this rule also have a counter action, this one must be
                 * inserted in one of the other banks to permit parallel hit. */
                if (!strictCount &&
                    (rule->action & FM_ACL_ACTIONEXT_COUNT))
                {
                    err = SelectPolicerBank(sw,
                                            cacls,
                                            NULL,
                                            compiledAclRule,
                                            &selectedBank,
                                            strictCount);
                    if (err != FM_OK)
                    {
                        if (err == FM_ERR_ACLS_TOO_BIG)
                        {
                            fm10000FormatAclStatus(errReport, TRUE,
                                                   "Policer bank full.\n");
                        }
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                    }
                    else
                    {
                        err = AddPolicerEntry(sw,
                                              cacls,
                                              NULL,
                                              0,
                                              compiledAclRule,
                                              selectedBank);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                }
            }
            else
            {
                fm10000FormatAclStatus(errReport, TRUE,
                                       "Invalid policer bank %d.\n",
                                       entry->bank);
                err = FM_ERR_INVALID_POLICER;
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
        }
        /* Acl rule with action police and count are not flexible since those
         * rule must use 2 banks. */
        else if (!strictCount &&
                 (rule->action & FM_ACL_ACTIONEXT_COUNT))
        {
            if (i == FM_FM10000_POLICER_BANK_MAX)
            {
                /* Preallocate Police Action */
                err = SelectPolicerBank(sw,
                                        cacls,
                                        entry,
                                        compiledAclRule,
                                        &selectedBank,
                                        strictCount);
                if (err != FM_OK)
                {
                    if (err == FM_ERR_ACLS_TOO_BIG)
                    {
                        fm10000FormatAclStatus(errReport, TRUE,
                                               "Policer bank full.\n");
                    }
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    err = AddPolicerEntry(sw,
                                          cacls,
                                          entry,
                                          rule->param.policer,
                                          compiledAclRule,
                                          selectedBank);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }

            /* Preallocate Count Action */
            err = SelectPolicerBank(sw,
                                    cacls,
                                    NULL,
                                    compiledAclRule,
                                    &selectedBank,
                                    strictCount);
            if (err != FM_OK)
            {
                if (err == FM_ERR_ACLS_TOO_BIG)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "Policer bank full.\n");
                }
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
            else
            {
                err = AddPolicerEntry(sw,
                                      cacls,
                                      NULL,
                                      0,
                                      compiledAclRule,
                                      selectedBank);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000PreallocatePolicerBank */


/*****************************************************************************/
/** fm10000ConfigurePolicerBank
 * \ingroup intAcl
 *
 * \desc            This function allocate the remaining policer entry that
 *                  can be configured in any bank.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       rule points to the defined acl rule that contain the
 *                  policer or count action.
 *
 * \param[out]      compiledAclRule points to the compiler ACL rule which
 *                  refer to a possible entry that may need to be handled.
 * 
 * \param[in]       strictCount refer to the policer/counter mode configured.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConfigurePolicerBank(fm_int                     sw,
                                      fm_aclErrorReporter *      errReport,
                                      fm_fm10000CompiledAcls *   cacls,
                                      fm_aclRule *               rule,
                                      fm_fm10000CompiledAclRule *compiledAclRule,
                                      fm_bool                    strictCount)
{
    fm_status                       err = FM_OK;
    fm_policerInfo *                info;
    fm_individualPolicer *          entry;
    fm_uintptr                      policerIndex;
    fm_switch *                     switchPtr;
    fm_int                          i;
    fm_int                          selectedBank;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_fm10000AclRule *             aclRule;
    void *                          value;
    fm_int *                        aclNumElement;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    info  = &switchPtr->policerInfo;

    /* Add Policer entry if needed */
    if (rule->action & FM_ACL_ACTIONEXT_POLICE)
    {
        /* Make sure the policer value entered into the rule parameter was
         * defined into the policer API. */
        err = fmTreeFind(&info->policers, rule->param.policer, (void*) &entry);
        if (err != FM_OK)
        {
            fm10000FormatAclStatus(errReport, TRUE,
                                   "Policer Id %d not defined into the "
                                   "Policer API.\n", rule->param.policer);
            FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
        }

        /* Scan currently allocated bank index */
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            if (compiledAclRule->policerIndex[i] != 0)
            {
                err = fmTreeFind(&cacls->policers[i].policerEntry,
                                 compiledAclRule->policerIndex[i],
                                 (void **) &compiledPolEntry);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Police entry already allocated */
                if (compiledPolEntry->countEntry == FALSE)
                {
                    break;
                }
            }
        }

        /* Need to allocate/link this policer */
        if (i == FM_FM10000_POLICER_BANK_MAX)
        {
            /* Find if this policer Id was already been allocated. */
            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                if (fmTreeFind(&cacls->policersId[i],
                               rule->param.policer,
                               (void **) &policerIndex) == FM_OK)
                {
                    /* Refer this acl rule to this preallocated policer index. */
                    compiledAclRule->policerIndex[i] = policerIndex;

                    /* Link this bank to this ACL */
                    err = fmTreeFind(&cacls->policers[i].acl,
                                     compiledAclRule->aclNumber,
                                     &value);
                    if (err == FM_OK)
                    {
                        aclNumElement = (fm_int*) value;
                        (*aclNumElement)++;
                    }
                    else if (err == FM_ERR_NOT_FOUND)
                    {
                        aclNumElement = (fm_int *) fmAlloc(sizeof(fm_int));
                        if (aclNumElement == NULL)
                        {
                            err = FM_ERR_NO_MEM;
                            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        *aclNumElement = 1;
                        err = fmTreeInsert(&cacls->policers[i].acl,
                                           compiledAclRule->aclNumber,
                                           aclNumElement);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    else
                    {
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Link this ACL/Rule to this policer entry */
                    err = fmTreeFind(&cacls->policers[i].policerEntry,
                                     policerIndex,
                                     (void **) &compiledPolEntry);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                    aclRule = (fm_fm10000AclRule *) fmAlloc(sizeof(fm_fm10000AclRule));
                    if (aclRule == NULL)
                    {
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_NO_MEM);
                    }
                    aclRule->aclNumber = compiledAclRule->aclNumber;
                    aclRule->ruleNumber = compiledAclRule->ruleNumber;
                    err = fmDListInsertEnd(&compiledPolEntry->policerRules,
                                           (void*) aclRule);
                    if (err != FM_OK)
                    {
                        fmFree(aclRule);
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                    }

                    break;
                }
            }
        }

        /* Entry not available, add it */
        if (i == FM_FM10000_POLICER_BANK_MAX)
        {
            if (entry->bank < FM_FM10000_POLICER_BANK_MAX)
            {
                err = SelectPolicerBank(sw,
                                        cacls,
                                        entry,
                                        compiledAclRule,
                                        &selectedBank,
                                        strictCount);
                if (err != FM_OK)
                {
                    if (err == FM_ERR_ACLS_TOO_BIG)
                    {
                        fm10000FormatAclStatus(errReport, TRUE,
                                               "Policer bank full.\n");
                    }
                    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
                }
                else
                {
                    err = AddPolicerEntry(sw,
                                          cacls,
                                          entry,
                                          rule->param.policer,
                                          compiledAclRule,
                                          selectedBank);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
            else
            {
                fm10000FormatAclStatus(errReport, TRUE,
                                       "Invalid policer bank %d.\n",
                                       entry->bank);
                err = FM_ERR_INVALID_POLICER;
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
        }
    }

    /* Add Counter entry if needed */
    if (rule->action & FM_ACL_ACTIONEXT_COUNT)
    {
        /* Scan currently allocated bank index */
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            if (compiledAclRule->policerIndex[i] != 0)
            {
                err = fmTreeFind(&cacls->policers[i].policerEntry,
                                 compiledAclRule->policerIndex[i],
                                 (void **) &compiledPolEntry);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

                /* Police entry already allocated */
                if (compiledPolEntry->countEntry == TRUE)
                {
                    break;
                }
            }
        }
        /* Entry not available, add it */
        if (i == FM_FM10000_POLICER_BANK_MAX)
        {
            err = SelectPolicerBank(sw,
                                    cacls,
                                    NULL,
                                    compiledAclRule,
                                    &selectedBank,
                                    strictCount);
            if (err != FM_OK)
            {
                if (err == FM_ERR_ACLS_TOO_BIG)
                {
                    fm10000FormatAclStatus(errReport, TRUE,
                                           "Policer bank full.\n");
                }
                FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
            }
            else
            {
                err = AddPolicerEntry(sw,
                                      cacls,
                                      NULL,
                                      0,
                                      compiledAclRule,
                                      selectedBank);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ConfigurePolicerBank */


/*****************************************************************************/
/** fm10000ApplyPolicerCfg
 * \ingroup intAcl
 *
 * \desc            This function apply the compiled policer configuration
 *                  into the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policers points to the array filled with compiled
 *                  policers and counters to apply.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ApplyPolicerCfg(fm_int                      sw,
                                 fm_fm10000CompiledPolicers *policers)
{
    fm_status                       err= FM_OK;
    fm_int                          i;
    fm_treeIterator                 itPolicer;
    fm_uint64                       policerIndex;
    void *                          nextValue;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        err = fm10000SetPolicerConfig(sw,
                                      i,
                                      policers[i].indexLastPolicer,
                                      policers[i].ingressColorSource,
                                      policers[i].markDSCP,
                                      policers[i].markSwitchPri,
                                      TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

        for (fmTreeIterInit(&itPolicer, &policers[i].policerEntry) ;
              (err = fmTreeIterNext(&itPolicer, &policerIndex, &nextValue)) ==
                FM_OK ; )
        {
            compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;
            if (compiledPolEntry->countEntry)
            {
                err = fm10000SetPolicerCounter(sw,
                                               i,
                                               policerIndex,
                                               FM_LITERAL_64(0),
                                               FM_LITERAL_64(0));
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else
            {
                err = fm10000SetPolicer(sw,
                                        i,
                                        policerIndex,
                                        &compiledPolEntry->committed,
                                        &compiledPolEntry->excess);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ApplyPolicerCfg */


/*****************************************************************************/
/** fm10000NonDisruptCleanPolicerRules
 * \ingroup intAcl
 *
 * \desc            This function remove any policers relation from the
 *                  compiled acl rules.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 * 
 * \param[in]       compiledAclRule points to the compiled ACL rule structure
 *                  to analyse.
 * 
 * \param[in]       bank is a bitmask uses to indicates which bank must be
 *                  cleared.
 * 
 * \param[in]       apply inform if the modification must be applied to the
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NonDisruptCleanPolicerRules(fm_int                     sw,
                                             fm_fm10000CompiledAcls *   cacls,
                                             fm_fm10000CompiledAclRule *compiledAclRule,
                                             fm_uint32                  bank,
                                             fm_bool                    apply)
{
    fm_status err = FM_OK;
    void *nextValue;
    fm_uint64 aclNumKey;
    fm_uint64 policerIndex;
    fm_uintptr policerIndexCast;
    fm_fm10000CompiledAcl* movedCompiledAcl;
    fm_fm10000CompiledAclRule* movedCompiledAclRule;
    fm_fm10000CompiledPolicerEntry* policerEntry;
    fm_fm10000AclRule* aclRule;
    fm_int i;
    fm_int j;
    fm_ffuOwnerType owner;
    fm_dlist_node* node;
    fm_uint32 maxPolicerEntries;
    fm_uint32 lastCounterIndex;
    fm_uint64 frameCount;
    fm_uint64 byteCount;
    fm_bool   bankCfgUpdate;
    fm_int    initialSize;
    fm_int*   aclNumElement;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, cacls = %p, compiledAclRule = %p, apply = %d\n",
                 sw, (void*) cacls, (void*) compiledAclRule, apply);

    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        /* Only clear the selected bank */
        if ( compiledAclRule->policerIndex[i] &&
             (bank & (1 << i)) )
        {
            FM_POLICER_BANK_MAX_ENTRIES(i, maxPolicerEntries);
            bankCfgUpdate = FALSE;
            initialSize = fmTreeSize(&cacls->policers[i].policerEntry);
            policerIndex = compiledAclRule->policerIndex[i];

            err = fmTreeFind(&cacls->policers[i].policerEntry,
                             policerIndex,
                             &nextValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            policerEntry = (fm_fm10000CompiledPolicerEntry *) nextValue;

            node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );
            while (node != NULL)
            {
                aclRule = (fm_fm10000AclRule *) node->data;

                /* Is this ACL/Rule tuple the one we are looking for? */
                if ( (aclRule->aclNumber == compiledAclRule->aclNumber) &&
                     (aclRule->ruleNumber == compiledAclRule->ruleNumber) )
                {
                    /* Yes, Remove this node */
                    err = fmTreeFind(&cacls->policers[i].acl,
                                     aclRule->aclNumber,
                                     &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    aclNumElement = (fm_int*) nextValue;
                    (*aclNumElement)--;

                    if (*aclNumElement == 0)
                    {
                        err = fmTreeRemoveCertain(&cacls->policers[i].acl,
                                                  aclRule->aclNumber,
                                                  fmFree);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    fmDListRemove(&policerEntry->policerRules, node);
                    fmFree(aclRule);
                    break;
                }

                node = FM_DLL_GET_NEXT(node, nextPtr);
            }

            /* This entry should be removed if all the nodes are removed */
            if (fmDListSize(&policerEntry->policerRules) == 0)
            {
                /* Processing Count entry type */
                if (policerEntry->countEntry)
                {
                    err = fmTreeRemove(&cacls->policers[i].policerEntry,
                                       policerIndex,
                                       fmFreeCompiledPolicerEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Compute the lastCounterIndex (starting from the bottom) */
                    lastCounterIndex = maxPolicerEntries -
                         (fmTreeSize(&cacls->policers[i].policerEntry) -
                          cacls->policers[i].indexLastPolicer);

                    /* Initialize the removed index  */
                    if (apply)
                    {
                        err = fm10000SetPolicerCounter(sw,
                                                       i,
                                                       policerIndex,
                                                       FM_LITERAL_64(0),
                                                       FM_LITERAL_64(0));
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Move the Counter currently located at the lastCounterIndex
                     * position to be now located at the removed location */
                    if (lastCounterIndex != policerIndex)
                    {
                        err = fmTreeFind(&cacls->policers[i].policerEntry,
                                         lastCounterIndex,
                                         &nextValue);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        policerEntry = (fm_fm10000CompiledPolicerEntry *) nextValue;

                        node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );
                        if (node == NULL)
                        {
                            err = FM_FAIL;
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        aclRule = (fm_fm10000AclRule *) node->data;

                        /* Move all the ACL/Rule tuple from the lastCounterIndex
                         * to the removed policer index */
                        err = fmGetAclNumKey(&cacls->ingressAcl,
                                             aclRule->aclNumber,
                                             aclRule->ruleNumber,
                                             &aclNumKey);
                        if (err == FM_OK)
                        {
                            err = fmTreeFind(&cacls->ingressAcl, aclNumKey, &nextValue);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            movedCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
                            err = fmTreeFind(&movedCompiledAcl->rules, aclRule->ruleNumber, &nextValue);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            movedCompiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                            movedCompiledAclRule->policerIndex[i] = policerIndex;

                            for (j = 0 ; j < FM10000_ACL_MAX_ACTIONS_PER_RULE ; j++)
                            {
                                if ( (movedCompiledAclRule->actions[j].bank == i) &&
                                     (movedCompiledAclRule->actions[j].counter == lastCounterIndex) )
                                {
                                    movedCompiledAclRule->actions[j].counter = policerIndex;
                                    break;
                                }
                            }
                            /* Bank/Index not found?! */
                            if (j == FM10000_ACL_MAX_ACTIONS_PER_RULE)
                            {
                                err = FM_FAIL;
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                            }

                            if (apply)
                            {
                                /* Applying the modification to the hardware */
                                err = fm10000SetFFURule(sw,
                                                        &movedCompiledAcl->sliceInfo,
                                                        movedCompiledAclRule->physicalPos,
                                                        movedCompiledAclRule->valid,
                                                        movedCompiledAclRule->sliceKey,
                                                        movedCompiledAclRule->actions,
                                                        FALSE, /* Live */
                                                        TRUE);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                /* Cache the current policer counter statistic
                                 * for this moved rule and add that cached
                                 * value to every future get counter call */
                                err = fm10000GetPolicerCounter(sw,
                                                               i,
                                                               lastCounterIndex,
                                                               &frameCount,
                                                               &byteCount);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                movedCompiledAclRule->cntAdjustPkts += frameCount;
                                movedCompiledAclRule->cntAdjustOctets += byteCount;
                            }

                            /* Update policer structure for the right mapping */
                            err = fmTreeRemoveCertain(&cacls->policers[i].policerEntry,
                                                      lastCounterIndex,
                                                      NULL);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            err = fmTreeInsert(&cacls->policers[i].policerEntry,
                                               policerIndex,
                                               policerEntry);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        else
                        {
                            err = FM_FAIL;
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                    }
                    /* The removed entry is currently located at the last
                     * position which mean that no movement is needed */
                    //else
                }
                /* Processing Policer entry type */
                else
                {
                    err = fmTreeRemove(&cacls->policersId[i],
                                       policerEntry->policerId,
                                       NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fmTreeRemove(&cacls->policers[i].policerEntry,
                                       policerIndex,
                                       fmFreeCompiledPolicerEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Move policer entry from last position to the removed
                     * one */
                    if (cacls->policers[i].indexLastPolicer != policerIndex)
                    {
                        err = fmTreeFind(&cacls->policers[i].policerEntry,
                                         cacls->policers[i].indexLastPolicer,
                                         &nextValue);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        policerEntry = (fm_fm10000CompiledPolicerEntry *) nextValue;

                        /* Configure the removed entry with the last position
                         * settings */
                        if (apply)
                        {
                            err = fm10000SetPolicer(sw,
                                                    i,
                                                    policerIndex,
                                                    &policerEntry->committed,
                                                    &policerEntry->excess);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }

                        node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );
                        while (node != NULL)
                        {
                            aclRule = (fm_fm10000AclRule *) node->data;

                            /* Move all the ACL/Rule tuple of this moved policer
                             * entry to now refer to the new position */
                            err = fmGetAclNumKey(&cacls->ingressAcl,
                                                 aclRule->aclNumber,
                                                 aclRule->ruleNumber,
                                                 &aclNumKey);
                            if (err == FM_OK)
                            {
                                err = fmTreeFind(&cacls->ingressAcl, aclNumKey, &nextValue);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                movedCompiledAcl = (fm_fm10000CompiledAcl*) nextValue;
                                err = fmTreeFind(&movedCompiledAcl->rules, aclRule->ruleNumber, &nextValue);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                movedCompiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                                movedCompiledAclRule->policerIndex[i] = policerIndex;

                                for (j = 0 ; j < FM10000_ACL_MAX_ACTIONS_PER_RULE ; j++)
                                {
                                    if ( (movedCompiledAclRule->actions[j].bank == i) &&
                                         (movedCompiledAclRule->actions[j].counter == cacls->policers[i].indexLastPolicer) )
                                    {
                                        movedCompiledAclRule->actions[j].counter = policerIndex;
                                        break;
                                    }
                                }
                                /* Bank/Index not found?! */
                                if (j == FM10000_ACL_MAX_ACTIONS_PER_RULE)
                                {
                                    err = FM_FAIL;
                                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                }

                                if (apply)
                                {
                                    err = fm10000SetFFURule(sw,
                                                            &movedCompiledAcl->sliceInfo,
                                                            movedCompiledAclRule->physicalPos,
                                                            movedCompiledAclRule->valid,
                                                            movedCompiledAclRule->sliceKey,
                                                            movedCompiledAclRule->actions,
                                                            FALSE, /* Live */
                                                            TRUE);
                                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                }
                            }

                            node = FM_DLL_GET_NEXT(node, nextPtr);
                        }
                        /* Update policer structure for the right mapping */
                        err = fmTreeRemoveCertain(&cacls->policers[i].policerEntry,
                                                  cacls->policers[i].indexLastPolicer,
                                                  NULL);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        err = fmTreeInsert(&cacls->policers[i].policerEntry,
                                           policerIndex,
                                           policerEntry);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        err = fmTreeRemove(&cacls->policersId[i],
                                           policerEntry->policerId,
                                           NULL);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        policerIndexCast = (fm_uintptr)policerIndex;
                        err = fmTreeInsert(&cacls->policersId[i],
                                           policerEntry->policerId,
                                           (void*) policerIndexCast);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    /* The removed entry is currently located at the last
                     * position which mean that no movement is needed */
                    //else

                    cacls->policers[i].indexLastPolicer--;
                    bankCfgUpdate = TRUE;
                }
            }

            /* This bank is now completely free */
            if ( initialSize &&
                 (fmTreeSize(&cacls->policers[i].policerEntry) == 0) )
            {
                /* Sanity check */
                if ( (cacls->policers[i].indexLastPolicer != 0) ||
                     (fmTreeSize(&cacls->policers[i].acl) != 0) )
                {
                    err = FM_FAIL;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                cacls->policers[i].markDSCP = FALSE;
                cacls->policers[i].markSwitchPri = FALSE;
                cacls->policers[i].ingressColorSource = 0;

                if (apply)
                {
                    err = fm10000SetPolicerConfig(sw,
                                                  i,
                                                  cacls->policers[i].indexLastPolicer,
                                                  cacls->policers[i].ingressColorSource,
                                                  cacls->policers[i].markDSCP,
                                                  cacls->policers[i].markSwitchPri,
                                                  TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fm10000GetPolicerOwnership(sw, &owner, i);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    if (owner == FM_FFU_OWNER_ACL)
                    {
                        /* Reseting the ownership of the bank */
                        err = fm10000SetPolicerOwnership(sw, FM_FFU_OWNER_NONE, i);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                }
            }
            else
            {
                /* Policer last index changed */
                if (apply && bankCfgUpdate)
                {
                    err = fm10000SetPolicerConfig(sw,
                                                  i,
                                                  cacls->policers[i].indexLastPolicer,
                                                  cacls->policers[i].ingressColorSource,
                                                  cacls->policers[i].markDSCP,
                                                  cacls->policers[i].markSwitchPri,
                                                  TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
        }
    }

ABORT:


    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptCleanPolicerRules */


/*****************************************************************************/
/** fm10000NonDisruptCleanPolicers
 * \ingroup intAcl
 *
 * \desc            This function remove any unused policers from the compiled
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
fm_status fm10000NonDisruptCleanPolicers(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_bool                 apply)
{
    fm_status err = FM_OK;
    fm_treeIterator itPolicers;
    void *nextValue;
    fm_uint64 aclNumKey;
    fm_uint64 policerIndex;
    fm_uintptr policerIndexCast;
    fm_uint64 nextPolicerIndex;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledPolicerEntry* policerEntry;
    fm_fm10000AclRule* aclRule;
    fm_int i;
    fm_int j;
    fm_ffuOwnerType owner;
    fm_dlist_node* node;
    fm_uint32 maxPolicerEntries;
    fm_uint32 lastCounterIndex;
    fm_uint64 frameCount;
    fm_uint64 byteCount;
    fm_bool   bankCfgUpdate;
    fm_int    initialSize;
    fm_int*   aclNumElement;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, cacls = %p, apply = %d\n",
                 sw, (void*) cacls, apply);

    /* Now validate that each policer configured have at least one rule that
     * refer to it. */
    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        FM_POLICER_BANK_MAX_ENTRIES(i, maxPolicerEntries);
        bankCfgUpdate = FALSE;
        initialSize = fmTreeSize(&cacls->policers[i].policerEntry);

        /* Removing unused Policers */
        for (fmTreeIterInit(&itPolicers, &cacls->policers[i].policerEntry) ;
             (err = fmTreeIterNext(&itPolicers, &policerIndex, &nextValue)) ==
                    FM_OK ; )
        {
            policerEntry = (fm_fm10000CompiledPolicerEntry *) nextValue;

            node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );

            while (node != NULL)
            {
                aclRule = (fm_fm10000AclRule *) node->data;

                /* Is this ACL/Rule tuple still in the system? */
                err = fmGetAclNumKey(&cacls->ingressAcl,
                                     aclRule->aclNumber,
                                     aclRule->ruleNumber,
                                     &aclNumKey);
                if (err != FM_OK)
                {
                    /* No, Remove this node */
                    err = fmTreeFind(&cacls->policers[i].acl,
                                     aclRule->aclNumber,
                                     &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    aclNumElement = (fm_int*) nextValue;
                    (*aclNumElement)--;

                    if (*aclNumElement == 0)
                    {
                        err = fmTreeRemoveCertain(&cacls->policers[i].acl,
                                                  aclRule->aclNumber,
                                                  fmFree);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    fmDListRemove(&policerEntry->policerRules, node);
                    fmFree(aclRule);
                    node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );
                    continue;
                }

                node = FM_DLL_GET_NEXT(node, nextPtr);
            }

            /* This entry should be removed if all the nodes are removed */
            if (fmDListSize(&policerEntry->policerRules) == 0)
            {
                /* Cache the next policerIndex */
                err = fmTreeIterNext(&itPolicers, &nextPolicerIndex, &nextValue);
                if (err != FM_OK)
                {
                    /* Last index */
                    nextPolicerIndex = 0;
                    err = FM_OK;
                }

                /* Processing Count entry type */
                if (policerEntry->countEntry)
                {
                    err = fmTreeRemove(&cacls->policers[i].policerEntry,
                                       policerIndex,
                                       fmFreeCompiledPolicerEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Compute the lastCounterIndex (starting from the bottom) */
                    lastCounterIndex = maxPolicerEntries -
                         (fmTreeSize(&cacls->policers[i].policerEntry) -
                          cacls->policers[i].indexLastPolicer);

                    /* Initialize the removed index  */
                    if (apply)
                    {
                        err = fm10000SetPolicerCounter(sw,
                                                       i,
                                                       policerIndex,
                                                       FM_LITERAL_64(0),
                                                       FM_LITERAL_64(0));
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Move the Counter currently located at the lastCounterIndex
                     * position to be now located at the removed location */
                    if (lastCounterIndex != policerIndex)
                    {
                        err = fmTreeFind(&cacls->policers[i].policerEntry,
                                         lastCounterIndex,
                                         &nextValue);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        policerEntry = (fm_fm10000CompiledPolicerEntry *) nextValue;

                        node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );
                        if (node == NULL)
                        {
                            err = FM_FAIL;
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        aclRule = (fm_fm10000AclRule *) node->data;

                        /* Move all the ACL/Rule tuple from the lastCounterIndex
                         * to the removed policer index */
                        err = fmGetAclNumKey(&cacls->ingressAcl,
                                             aclRule->aclNumber,
                                             aclRule->ruleNumber,
                                             &aclNumKey);
                        if (err == FM_OK)
                        {
                            err = fmTreeFind(&cacls->ingressAcl, aclNumKey, &nextValue);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
                            err = fmTreeFind(&compiledAcl->rules, aclRule->ruleNumber, &nextValue);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                            compiledAclRule->policerIndex[i] = policerIndex;

                            for (j = 0 ; j < FM10000_ACL_MAX_ACTIONS_PER_RULE ; j++)
                            {
                                if ( (compiledAclRule->actions[j].bank == i) &&
                                     (compiledAclRule->actions[j].counter == lastCounterIndex) )
                                {
                                    compiledAclRule->actions[j].counter = policerIndex;
                                    break;
                                }
                            }
                            /* Bank/Index not found?! */
                            if (j == FM10000_ACL_MAX_ACTIONS_PER_RULE)
                            {
                                err = FM_FAIL;
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                            }

                            if (apply)
                            {
                                /* Applying the modification to the hardware */
                                err = fm10000SetFFURule(sw,
                                                        &compiledAcl->sliceInfo,
                                                        compiledAclRule->physicalPos,
                                                        compiledAclRule->valid,
                                                        compiledAclRule->sliceKey,
                                                        compiledAclRule->actions,
                                                        FALSE, /* Live */
                                                        TRUE);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                /* Cache the current policer counter statistic
                                 * for this moved rule and add that cached
                                 * value to every future get counter call */
                                err = fm10000GetPolicerCounter(sw,
                                                               i,
                                                               lastCounterIndex,
                                                               &frameCount,
                                                               &byteCount);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                compiledAclRule->cntAdjustPkts += frameCount;
                                compiledAclRule->cntAdjustOctets += byteCount;
                            }

                            /* Update policer structure for the right mapping */
                            err = fmTreeRemoveCertain(&cacls->policers[i].policerEntry,
                                                      lastCounterIndex,
                                                      NULL);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            err = fmTreeInsert(&cacls->policers[i].policerEntry,
                                               policerIndex,
                                               policerEntry);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                            /* Continue with the next policer index */
                            err = fmTreeIterInitFromKey(&itPolicers,
                                                        &cacls->policers[i].policerEntry,
                                                        policerIndex);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        else
                        {
                            err = FM_FAIL;
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                    }
                    /* The removed entry is currently located at the last
                     * position which mean that no movement is needed */
                    else
                    {
                        /* Continue with the next policer index if one was found */
                        if (nextPolicerIndex != 0)
                        {
                            err = fmTreeIterInitFromKey(&itPolicers,
                                                        &cacls->policers[i].policerEntry,
                                                        nextPolicerIndex);
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
                /* Processing Policer entry type */
                else
                {
                    err = fmTreeRemove(&cacls->policersId[i],
                                       policerEntry->policerId,
                                       NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fmTreeRemove(&cacls->policers[i].policerEntry,
                                       policerIndex,
                                       fmFreeCompiledPolicerEntry);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Move policer entry from last position to the removed
                     * one */
                    if (cacls->policers[i].indexLastPolicer != policerIndex)
                    {
                        err = fmTreeFind(&cacls->policers[i].policerEntry,
                                         cacls->policers[i].indexLastPolicer,
                                         &nextValue);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        policerEntry = (fm_fm10000CompiledPolicerEntry *) nextValue;

                        /* Configure the removed entry with the last position
                         * settings */
                        if (apply)
                        {
                            err = fm10000SetPolicer(sw,
                                                    i,
                                                    policerIndex,
                                                    &policerEntry->committed,
                                                    &policerEntry->excess);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }

                        node = FM_DLL_GET_FIRST( (&policerEntry->policerRules), head );
                        while (node != NULL)
                        {
                            aclRule = (fm_fm10000AclRule *) node->data;

                            /* Move all the ACL/Rule tuple of this moved policer
                             * entry to now refer to the new position */
                            err = fmGetAclNumKey(&cacls->ingressAcl,
                                                 aclRule->aclNumber,
                                                 aclRule->ruleNumber,
                                                 &aclNumKey);
                            if (err == FM_OK)
                            {
                                err = fmTreeFind(&cacls->ingressAcl, aclNumKey, &nextValue);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
                                err = fmTreeFind(&compiledAcl->rules, aclRule->ruleNumber, &nextValue);
                                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                                compiledAclRule->policerIndex[i] = policerIndex;

                                for (j = 0 ; j < FM10000_ACL_MAX_ACTIONS_PER_RULE ; j++)
                                {
                                    if ( (compiledAclRule->actions[j].bank == i) &&
                                         (compiledAclRule->actions[j].counter == cacls->policers[i].indexLastPolicer) )
                                    {
                                        compiledAclRule->actions[j].counter = policerIndex;
                                        break;
                                    }
                                }
                                /* Bank/Index not found?! */
                                if (j == FM10000_ACL_MAX_ACTIONS_PER_RULE)
                                {
                                    err = FM_FAIL;
                                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                }

                                if (apply)
                                {
                                    err = fm10000SetFFURule(sw,
                                                            &compiledAcl->sliceInfo,
                                                            compiledAclRule->physicalPos,
                                                            compiledAclRule->valid,
                                                            compiledAclRule->sliceKey,
                                                            compiledAclRule->actions,
                                                            FALSE, /* Live */
                                                            TRUE);
                                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                                }
                            }

                            node = FM_DLL_GET_NEXT(node, nextPtr);
                        }
                        /* Update policer structure for the right mapping */
                        err = fmTreeRemoveCertain(&cacls->policers[i].policerEntry,
                                                  cacls->policers[i].indexLastPolicer,
                                                  NULL);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        err = fmTreeInsert(&cacls->policers[i].policerEntry,
                                           policerIndex,
                                           policerEntry);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        err = fmTreeRemove(&cacls->policersId[i],
                                           policerEntry->policerId,
                                           NULL);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        policerIndexCast = (fm_uintptr)policerIndex;
                        err = fmTreeInsert(&cacls->policersId[i],
                                           policerEntry->policerId,
                                           (void*) policerIndexCast);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        /* Continue with the next policer index */
                        err = fmTreeIterInitFromKey(&itPolicers,
                                                    &cacls->policers[i].policerEntry,
                                                    policerIndex);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    /* The removed entry is currently located at the last
                     * position which mean that no movement is needed */
                    else
                    {
                        /* Continue with the next policer index if one was found */
                        if (nextPolicerIndex != 0)
                        {
                            err = fmTreeIterInitFromKey(&itPolicers,
                                                        &cacls->policers[i].policerEntry,
                                                        nextPolicerIndex);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        }
                        else
                        {
                            /* Done */
                            cacls->policers[i].indexLastPolicer--;
                            bankCfgUpdate = TRUE;
                            err = FM_ERR_NO_MORE;
                            break;
                        }
                    }

                    cacls->policers[i].indexLastPolicer--;
                    bankCfgUpdate = TRUE;
                }
            }

        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }
        err = FM_OK;

        /* This bank is now completely free */
        if ( initialSize &&
             (fmTreeSize(&cacls->policers[i].policerEntry) == 0) )
        {
            /* Sanity check */
            if ( (cacls->policers[i].indexLastPolicer != 0) ||
                 (fmTreeSize(&cacls->policers[i].acl) != 0) )
            {
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            cacls->policers[i].markDSCP = FALSE;
            cacls->policers[i].markSwitchPri = FALSE;
            cacls->policers[i].ingressColorSource = 0;

            if (apply)
            {
                err = fm10000SetPolicerConfig(sw,
                                              i,
                                              cacls->policers[i].indexLastPolicer,
                                              cacls->policers[i].ingressColorSource,
                                              cacls->policers[i].markDSCP,
                                              cacls->policers[i].markSwitchPri,
                                              TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000GetPolicerOwnership(sw, &owner, i);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                if (owner == FM_FFU_OWNER_ACL)
                {
                    /* Reseting the ownership of the bank */
                    err = fm10000SetPolicerOwnership(sw, FM_FFU_OWNER_NONE, i);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
        }
        else
        {
            /* Policer last index changed */
            if (apply && bankCfgUpdate)
            {
                err = fm10000SetPolicerConfig(sw,
                                              i,
                                              cacls->policers[i].indexLastPolicer,
                                              cacls->policers[i].ingressColorSource,
                                              cacls->policers[i].markDSCP,
                                              cacls->policers[i].markSwitchPri,
                                              TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

ABORT:


    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NonDisruptCleanPolicers */


/*****************************************************************************/
/** fm10000SetPolicerAttribute
 * \ingroup intAcl
 *
 * \desc            Set a policer attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number whose attribute is to be
 *                  set. This argument is ignored for global attributes.
 *
 * \param[in]       attr is the policer attribute to set (see
 *                  ''Policer Attributes'').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetPolicerAttribute(fm_int      sw,
                                     fm_int      policer,
                                     fm_int      attr,
                                     const void *value)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_policerInfo *info;
    fm_int          i;
    fm_byte         dscpMkdnMap[FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES];
    fm_byte         swPriMkdnMap[FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES];
    
    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, policer = %d, attr = %d\n",
                 sw, policer, attr);

    switchPtr = GET_SWITCH_PTR(sw);
    info  = &switchPtr->policerInfo;

    switch (attr)
    {
       case FM_POLICER_SWPRI_MKDN_MAP:
           /* Convert fm_int[] to fm_byte[] */
            for (i = 0 ; i < FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES ; i++)
            {
                swPriMkdnMap[i] = info->swPriMkdnMap[i];
            }
            err = fm10000SetPolicerSwPriDownMap(sw, swPriMkdnMap, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
           break;

       case FM_POLICER_DSCP_MKDN_MAP:
           /* Convert fm_int[] to fm_byte[] */
            for (i = 0 ; i < FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES ; i++)
            {
                dscpMkdnMap[i] = info->dscpMkdnMap[i];
            }
            err = fm10000SetPolicerDSCPDownMap(sw, dscpMkdnMap, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
           break;

       default:
           break;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000SetPolicerAttribute */




/*****************************************************************************/
/** fm10000UpdatePolicer
 * \ingroup intAcl
 *
 * \desc            This function is used to update policer configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       policer is the policer number which has to be updated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdatePolicer(fm_int sw, fm_int policer)
{
    fm_status                       status ;
    fm_switch                      *switchPtr;
    fm10000_switch                 *switchExt;
    fm_fm10000CompiledAcls         *aacls;
    fm_policerInfo                 *info;
    fm_individualPolicer           *entry;
    fm_policerConfig               *policerCfgEntry;
    fm_fm10000CompiledPolicerEntry  localPolicerCfg;
    fm_fm10000CompiledPolicerEntry *policerCfg;
    void                           *value;
    fm_uintptr                      policerIndex;
    fm_int                          bank;
    fm_int                          i;

    bank = 0;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d, policer = %d\n", sw, policer);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = &switchPtr->policerInfo;

    status = fmTreeFind(&info->policers, policer, &value);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, status);

    entry           = (fm_individualPolicer *)value;
    policerCfgEntry = &entry->attributes;

    switchExt = (fm10000_switch *) switchPtr->extension;
    aacls     = switchExt->appliedAcls;
    if (aacls == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ACL_IMAGE);
    }

    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        status = fmTreeFind(&aacls->policersId[i], policer, (void **)&policerIndex);
        if (status == FM_ERR_NOT_FOUND)
        {
            /* Search in next bank */
            continue;
        }
        else if (status == FM_OK)
        {
            bank = i;
            break;
        }
        else
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, status);
        }
    }

    /* If not found */
    if (i == FM_FM10000_POLICER_BANK_MAX)
    {
        status = FM_ERR_INVALID_POLICER;
        FM_LOG_EXIT(FM_LOG_CAT_ACL, status);
    }
    //else

    /* Find the existing policerCfg and update. */
    status = fmTreeFind(&aacls->policers[bank].policerEntry, 
                        policerIndex,                    
                        (void **)&policerCfg);
    if ( (status == FM_ERR_NOT_FOUND) ||
         (status == FM_OK && policerCfg == NULL) )
    {
        status = FM_ERR_INVALID_POLICER;
        FM_LOG_EXIT(FM_LOG_CAT_ACL, status);
    }
    else if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, status);
    }

    status = PolicerBankMatch(&aacls->policers[bank],
                              policerCfgEntry);
    if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_CONFLICT_POLICER_PARAM);
    }
 
    /* Update policer bank, if bank doesn't have any markdown configured
     * and entry has markdown configured. */
    if ( (aacls->policers[bank].markDSCP == FALSE &&
          aacls->policers[bank].markSwitchPri == FALSE) &&
          (policerCfgEntry->mkdnDscp == TRUE ||
           policerCfgEntry->mkdnSwPri == TRUE) )
    {
        status = fm10000SetPolicerConfig(sw,
                                         i,
                                         aacls->policers[bank].indexLastPolicer,
                                         aacls->policers[bank].ingressColorSource,
                                         policerCfgEntry->mkdnDscp,
                                         policerCfgEntry->mkdnSwPri,
                                         TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, status);

        /* Update ACL image */
        aacls->policers[bank].markDSCP      = policerCfgEntry->mkdnDscp;
        aacls->policers[bank].markSwitchPri = policerCfgEntry->mkdnSwPri; 
    }
        
    FM_CLEAR(localPolicerCfg);
    status = fm10000ConvertPolicerAttributeToState(policerCfgEntry,
                                                   &localPolicerCfg.committed,
                                                   &localPolicerCfg.excess);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, status);
    
    status = fm10000SetPolicer(sw, 
                               bank, 
                               policerIndex, 
                               &localPolicerCfg.committed,
                               &localPolicerCfg.excess);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, status);

    /* After the update in hardware is successful, update the s/w acl image */
    policerCfg->committed = localPolicerCfg.committed;
    policerCfg->excess = localPolicerCfg.excess;
    
    status = fm10000SetPolicerCounter(sw,
                                      bank,
                                      policerIndex,
                                      FM_LITERAL_64(0),
                                      FM_LITERAL_64(0));
    FM_LOG_EXIT(FM_LOG_CAT_ACL, status);

}   /* end fm10000UpdatePolicer */
