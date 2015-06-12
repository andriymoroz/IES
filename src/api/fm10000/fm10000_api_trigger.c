/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_trigger.c
 * Creation Date:   June 18th, 2013
 * Description:     Functions for manipulating the triggers of a switch
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#define FM_API_REQUIRE(expr, failCode)          \
    if ( !(expr) )                              \
    {                                           \
        err = failCode;                         \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, \
                            failCode);          \
    }

#define FM10000_TRIGGER_RL_CAPACITY_MAX       (1 << 12)
#define FM10000_TRIGGER_RL_MANTISSA_MAX       (1 << 12)
#define FM10000_TRIGGER_RL_EXPONENT_MAX       (1 << 2)

#define BITS_PER_BYTE                         8

#define FM10000_FRAME_HANDLER_FREQ            375000000 /* What is RRC's value for this ?*/

#define RATE_LIM_USAGE_TO_BYTES               16

/* The random value generator gives a 24-bit random value */
#define MAX_RANDOM_VALUE    (1 << 24)
#define MAX_RANDOM_EXP      0x18

#define APPEND_TEXT(dst, newText)                                   \
    if (newText != NULL)                                            \
    {                                                               \
        FM_STRCAT_S(dst,                                            \
                    sizeof(dst) - 1,                                \
                    newText);                                       \
    }


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* Local static variable that store the default/invalid condition state of a
 * trigger entry. */
static fm_triggerCondition invalidCond = 
{
    .cfg = { .matchSA =             FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchDA =             FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchHitSA =          FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchHitDA =          FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchHitSADA =        FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchVlan =           FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchFFU =            FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchSwitchPri =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchEtherType =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchDestGlort =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL,
             .matchFrameClassMask = 0,
             .matchRoutedMask =     0,
             .matchFtypeMask =      0,
             .matchRandomNumber =   FALSE,
             .matchTx =             FM_TRIGGER_TX_MASK_DOESNT_CONTAIN,
             .rxPortset =           FM_PORT_SET_NONE,
             .txPortset =           FM_PORT_SET_NONE,
             .HAMask =              0 },

    .param = { .saId =              0,
               .daId =              0,
               .vidId =             0,
               .switchPri =         0,
               .ffuId =             0,
               .ffuIdMask =         0,
               .etherType =         0,
               .etherTypeMask =     0,
               .destGlort =         0,
               .destGlortMask =     0,
               .randGenerator =     FM_TRIGGER_RAND_GEN_A,
               .randMatchThreshold = 0 }

};

/* Local static variables that store the default/invalid action state of a
 * trigger entry. */
static fm_triggerAction invalidAction = 
{
    .cfg = { .forwardingAction = FM_TRIGGER_FORWARDING_ACTION_ASIS,
             .trapAction =       FM_TRIGGER_TRAP_ACTION_ASIS,
             .mirrorAction =     FM_TRIGGER_MIRROR_ACTION_NONE,
             .switchPriAction =  FM_TRIGGER_SWPRI_ACTION_ASIS,
             .vlan1Action =      FM_TRIGGER_VLAN_ACTION_ASIS,
             .learningAction =   FM_TRIGGER_LEARN_ACTION_ASIS,
             .rateLimitAction =  FM_TRIGGER_RATELIMIT_ACTION_ASIS },

    .param = { .newDestGlort =     0,
               .newDestGlortMask = 0,
               .newDestPortset =   FM_PORT_SET_NONE,
               .filterDestMask =   FALSE,
               .dropPortset =      FM_PORT_SET_NONE,
               .newSwitchPri =     0,
               .newVlan1 =         0,
               .rateLimitNum =     0,
               .mirrorSelect =     0,
               .mirrorProfile =    0 }
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** RateToMantissaAndExponent
 * \ingroup triggerInt
 * 
 * \desc            Converts a rate in kbps to mantissa and exponent.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       rate is the desired rate-limiter rate in kbps.
 *
 * \param[out]      mantissa is a pointer to the caller allocated storage where
 *                  the mantissa should be stored,
 * 
 * \param[out]      exp is a pointer to the caller allocated storage where
 *                  the exponent should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RateToMantissaAndExponent(fm_int     sw,
                                           fm_uint32  rate, 
                                           fm_uint32 *mantissa,
                                           fm_uint32 *exp)
{
    fm_status err;
    fm_uint32 e;
    fm_uint32 m;
    fm_uint64 cf;
    fm_uint64 freq;
    fm_float  fhMhz;
    
    /************************************************ 
     * m:     mantissa
     * e:     exponent
     * f:     FH frequency (Hz)
     * T:     Clock period
     * cf:    conversion factor (1/16B per 16T to kpbs)
     * rate:  rate in kbps
     *
     * Refil rate of the rate limiter bucket:
     *
     * rate = ( (m * 32) >> (e + 2) ) * cf
     *
     * cf = (BITS_PER_BYTE * f) / (1000 * 16 * 16)
     *
     *************************************************/

    err = fm10000ComputeFHClockFreq(sw, &fhMhz);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    freq = fhMhz * 1e6;

    cf = (BITS_PER_BYTE * freq) / (1000 * 16 * 16);

    if (cf == 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_TRIGGER, 
                     "Failure to convert rate to mantissa/exponent, "
                     "division by 0\n");
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Start with e=3 for better granularity */
    e = 3;
    m = ( (rate / cf) << ( e + 2 ) ) / 32;

    while ( (m >= FM10000_TRIGGER_RL_MANTISSA_MAX) && 
            (e > 0) )
    {
        e--;
        m = ( (rate / cf) << ( e + 2 ) ) / 32;
    }
        
    if (m >= FM10000_TRIGGER_RL_MANTISSA_MAX)
    {
        m = FM10000_TRIGGER_RL_MANTISSA_MAX - 1;
    }
    
    *mantissa = m;
    *exp = e;

ABORT:
    return err;

}   /* end RateToMantissaAndExponent */




/*****************************************************************************/
/** MantissaAndExponentToRate
 * \ingroup triggerInt
 * 
 * \desc            Converts a mantissa and exponent representation to rate in
 *                  kbps.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       mantissa is the rate's mantissa.
 * 
 * \param[in]       exp is the rate's exponent.
 * 
 * \param[out]      rate is a pointer to the caller allocated storage
 *                  where the rate-limiter rate in kbps should be stored. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MantissaAndExponentToRate(fm_int     sw,
                                           fm_uint32  mantissa, 
                                           fm_uint32  exp, 
                                           fm_uint32 *rate)
{
    fm_status err;
    fm_uint64 cf;
    fm_uint64 freq;
    fm_float  fhMhz;

    err = fm10000ComputeFHClockFreq(sw, &fhMhz);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    freq = fhMhz * 1e6;

    cf = (BITS_PER_BYTE * freq) / (1000 * 16 * 16);
    *rate = ( (mantissa * 32) >> (exp + 2) ) * cf;

ABORT:
    return err;

}   /* end MantissaAndExponentToRate */




/*****************************************************************************/
/** PhysToLogPortMask
 * \ingroup triggerInt
 * 
 * \desc            Converts a physical port mask to a logical port mask.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       physMask is the port mask in physical form.
 *
 * \param[out]      logMask is a pointer to the caller allocated storage where
 *                  the port mask in logical form should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PhysToLogPortMask(fm_int     sw,
                                   fm_uint64  physMask, 
                                   fm_uint64 *logMask)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
    fm_portmask physPortMask;
    fm_portmask logPortMask;

    switchPtr = GET_SWITCH_PTR(sw);

    physPortMask.maskWord[0] = physMask & 0xFFFFFFFF;
    physPortMask.maskWord[1] = (physMask >> 32) & 0xFFFFFFFF;

    err = fmPortMaskPhysicalToLogical(switchPtr, &physPortMask, &logPortMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    *logMask = ( (fm_uint64)(logPortMask.maskWord[0]) ) | 
               ( (fm_uint64)(logPortMask.maskWord[1]) << 32);

ABORT:
    return err;

}   /* end PhysToLogPortMask */




/*****************************************************************************/
/** UpdateMatchByPrec
 * \ingroup triggerInt
 *
 * \desc            Updates the matchByPrec field of all entries within the
 *                  trigger tree. No changes are made to HW.
 *                  
 * \param[in]       sw is the switch to operate on
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateMatchByPrec(fm_int sw)
{
    fm_status             err = FM_OK;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm_treeIterator       triggerIt;
    fm_uint64             nextKey;
    fm10000_triggerEntry *nextTrigEntry;
    fm_int                curGroup;
    fm_int                lastGroup;

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    lastGroup = -1;

    /* Move the trigger entries down until we reach our new trigger */
    fmTreeIterInit(&triggerIt, &trigInfo->triggerTree);

    while ( (err = fmTreeIterNext(&triggerIt, 
                                  &nextKey, 
                                  (void**) &nextTrigEntry)) == FM_OK)
    {
        curGroup = FM10000_TRIGGER_KEY_TO_GROUP(nextKey);

        /* This is a new precedence group */
        if (curGroup != lastGroup)
        {
            nextTrigEntry->matchByPrec = 0;
        }
        else
        {
            nextTrigEntry->matchByPrec = 1;
        }

        lastGroup = curGroup;
    }

    if (err != FM_ERR_NO_MORE)
    {
        return err;
    }
    else
    {
        err = FM_OK;
    }

    return err;

}   /* end UpdateMatchByPrec */




/*****************************************************************************/
/** ConvertPublicMatchRandomToHW
 * \ingroup triggerInt
 *
 * \desc            Convert the public match random representation
 *                  to the nearest HW possible configuration
 * 
 * \param[in]       randEnable should be set to true if matching on
 *                  random values is enabled
 * 
 * \param[in]       randThresh is the random threshold below which
 *                  the trigger should hit.
 *                                                       
 *                  If (random value <= threshold) this trigger hits
 *                                                          
 *                  Note that the threshold will be rounded to the nearest
 *                  value supported by the hardware. When this value is read, it
 *                  may differ from the value set as the rounded value will be
 *                  returned.
 *                  
 *                  The random number generator generates a 24-bit value (0 to
 *                  16,777,215).
 * 
 * \param[out]      matchRandomIfLess is a pointer to the caller allocated
 *                  storage where the corresponding MatchRandomIfLess
 *                  bit from FM10000_TRIGGER_CONDITION_CFG should be stored.
 * 
 * \param[out]      matchRandomThreshExp is a pointer to the caller allocated
 *                  storate where the corresponding MatchRandomThreshold
 *                  field from FM10000_TRIGGER_CONDITION_CFG should be stored. 
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static void ConvertPublicMatchRandomToHW(fm_bool    randEnable, 
                                         fm_uint32  randThresh, 
                                         fm_bool *  matchRandomIfLess,
                                         fm_uint32 *matchRandomThreshExp)
{
    fm_uint32 exp;
    fm_uint32 hwThresh;

    if (randEnable)
    {
        /******************************
         * Treat special cases first
         ******************************/

        /* Never Match */
        if (randThresh == 0)
        {
            *matchRandomIfLess = FALSE;
            *matchRandomThreshExp = MAX_RANDOM_EXP;
        }
        /* Always Match */
        else if (randThresh >= (MAX_RANDOM_VALUE - 1))
        {
            *matchRandomIfLess = TRUE;
            *matchRandomThreshExp = MAX_RANDOM_EXP;
        }
        /* All other cases, compute values */
        else
        {

            if (randThresh > MAX_RANDOM_VALUE / 2)
            {
                *matchRandomIfLess = FALSE;
                exp = 0;

                hwThresh = MAX_RANDOM_VALUE - (1 << exp);

                while ( (hwThresh > randThresh) && 
                        (exp <= (MAX_RANDOM_EXP - 1) ) )
                {
                    exp++;
                    /* The hwThresh is decremented by 2^(exp-1)
                     * for rounding */
                    hwThresh = MAX_RANDOM_VALUE - 
                               ((1 << exp) + (1 << (exp - 1)));
                }
            }
            else
            {
                *matchRandomIfLess = TRUE;
                exp = 0;

                hwThresh = (1 << exp);

                while ( (hwThresh < randThresh) && 
                        (exp <= (MAX_RANDOM_EXP - 1) ) )
                {
                    exp++;

                    /* The hwThresh is incremented by 2^(exp-1)
                     * for rounding */
                    hwThresh     = (1 << exp) + (1 << (exp - 1));
                }
            }

            *matchRandomThreshExp = exp;
        }
    }
    else
    {
        /* Random matching is always enabled in HW, use values that guarantee
         * that the random generated values are below the threshold resulting
         * in an always match condition */
        *matchRandomIfLess = TRUE;
        *matchRandomThreshExp = MAX_RANDOM_EXP;
    }

}   /* end ConvertPublicMatchRandomToHW */




/*****************************************************************************/
/** ConvertHWMatchRandomToPublic
 * \ingroup triggerInt
 *
 * \desc            Convert the HW random match threshold to public API
 *                  representation
 *                  
 * \param[in]       matchRandomIfLess is the bit from
 *                  FM10000_TRIGGER_CONDITION_CFG.
 * 
 * \param[in]       matchRandomThreshExp is the random matching threshold
 *                  exponent from FM10000_TRIGGER_CONDITION_CFG.
 * 
 * \param[out]      randEnable is a pointer to the caller allocated
 *                  storage where the state of random matching should be stored.
 * 
 * \param[out]      randThresh is a pointer to the caller allocated
 *                  storage where the 24-bit random matching threshold should
 *                  be stored. 
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static void ConvertHWMatchRandomToPublic(fm_bool    matchRandomIfLess,
                                         fm_uint32  matchRandomThreshExp,
                                         fm_bool *  randEnable, 
                                         fm_uint32 *randThresh)
{
    *randEnable = TRUE;

    if (matchRandomIfLess)
    {
        *randThresh = (1 << matchRandomThreshExp);

        if (*randThresh >= (MAX_RANDOM_VALUE - 1))
        {
            *randEnable = FALSE;
        }
    }
    else
    {
        *randThresh = MAX_RANDOM_VALUE - (1 << matchRandomThreshExp);
    }

}   /* end ConvertHWMatchRandomToPublic */




/*****************************************************************************/
/** SetTriggerActionMirrorProfile
 * \ingroup intTriggerInt
 *
 * \desc            Converts a hardware mirror profile index to a mirror
 *                  profile handle, and stores it in the mirrorProfile field
 *                  of a trigger action structure.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       action points to the action parameters for the trigger.
 * 
 * \param[in]       mirrorIndex is the hardware mirror profile index.
 * 
 * \return          None.
 *
 *****************************************************************************/
static void SetTriggerActionMirrorProfile(fm_int            sw,
                                          fm_triggerAction *action,
                                          fm_uint32         mirrorIndex)
{

    action->param.mirrorProfile = 0;

    /* Trigger must request MIRROR action. */
    if (action->cfg.mirrorAction == FM_TRIGGER_MIRROR_ACTION_MIRROR)
    {
        fm10000ConvertMirrorIndexToProfile(sw,
                                           mirrorIndex,
                                           &action->param.mirrorProfile);
    }

}   /* end SetTriggerActionMirrorProfile */




/*****************************************************************************/
/** fm10000WriteTriggerCondition
 * \ingroup triggerInt
 *
 * \desc            Write a trigger condition to a HW trigger entry
 * 
 * \param[in]       sw is the switch to operate on
 * 
 * \param[in]       entryIndex is the HW entry index of the trigger
 * 
 * \param[in]       cond is a pointer to the condition parameters to write
 *                  in HW
 * 
 * \param[in]       matchByPrecedence is the HW bit that determines if this
 *                  trigger is part of a precedence group or not. 
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000WriteTriggerCondition(fm_int sw, 
                                              fm_uint32 entryIndex, 
                                              const fm_triggerCondition *cond,
                                              fm_bool matchByPrecedence)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
                
    fm_bool     matchRandomIfLess;
    fm_uint32   matchRandomThreshExp;
                
    fm_uint32   regCondCfg;
    fm_uint32   regCondParam;
    fm_uint32   regCondFfu;
    fm_uint32   regCondType;
    fm_uint32   regCondGlort;
    fm_uint64   regCondRx;
    fm_uint64   regCondTx;
    fm_uint32   regCondAmask1;
    fm_uint32   regCondAmask2;
    fm_portmask portmask;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, entryIndex = %d, "
                 "cond = %p, matchByPrecedence = %s\n",
                 sw, 
                 entryIndex,
                 (void *) cond,
                 matchByPrecedence ? "TRUE" : "FALSE");

    switchPtr = GET_SWITCH_PTR(sw);

    /*************************************************************** 
     * First convert the software structure to HW register values
     ***************************************************************/

    /* For FM10000_TRIGGER_CONDITION_RX and FM10000_TRIGGER_CONDITION_TX,
     * we must convert from portset representation to bit mask representation */
    err = fmPortSetToPortMask(sw, cond->cfg.rxPortset, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmPortMaskLogicalToPhysical(switchPtr, &portmask, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    regCondRx = ( (fm_uint64)(portmask.maskWord[0]) ) | 
                ( (fm_uint64)(portmask.maskWord[1]) << 32);

    err = fmPortSetToPortMask(sw, cond->cfg.txPortset, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmPortMaskLogicalToPhysical(switchPtr, &portmask, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    regCondTx = ( (fm_uint64)(portmask.maskWord[0]) ) | 
                ( (fm_uint64)(portmask.maskWord[1]) << 32);

    /* For FM10000_TRIGGER_CONDITION_CFG */
    regCondCfg = 0;
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchSA,        
                 cond->cfg.matchSA);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchDA,        
                 cond->cfg.matchDA);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchHitSA,     
                 cond->cfg.matchHitSA);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchHitDA,     
                 cond->cfg.matchHitDA);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchHitSADA,   
                 cond->cfg.matchHitSADA);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchVlan,      
                 cond->cfg.matchVlan);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchFFU,       
                 cond->cfg.matchFFU);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchSwitchPri, 
                 cond->cfg.matchSwitchPri);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchEtherType, 
                 cond->cfg.matchEtherType);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchDestGlort, 
                 cond->cfg.matchDestGlort);
    FM_SET_BIT  (regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchByPrecedence, 
                 matchByPrecedence);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchTx, 
                 cond->cfg.matchTx);

    ConvertPublicMatchRandomToHW(cond->cfg.matchRandomNumber, 
                                 cond->param.randMatchThreshold,
                                 &matchRandomIfLess,
                                 &matchRandomThreshExp);

    FM_SET_BIT  (regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchRandomIfLess, 
                 matchRandomIfLess);
    FM_SET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchRandomThreshold, 
                 matchRandomThreshExp);
    FM_SET_BIT  (regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchRandomNumber, 
                 cond->param.randGenerator );

    /* For FM10000_TRIGGER_CONDITION_PARAM */
    regCondParam = 0;
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, SA_ID, 
                 cond->param.saId);
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, DA_ID, 
                 cond->param.daId);
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, VID_ID, 
                 cond->param.vidId);
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, SwitchPri, 
                 cond->param.switchPri);
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, FrameClassMask, 
                 cond->cfg.matchFrameClassMask);
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, RoutedMask, 
                 cond->cfg.matchRoutedMask);
    FM_SET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, FtypeMask, 
                 cond->cfg.matchFtypeMask);
    
    /* For FM10000_TRIGGER_CONDITION_FFU */
    regCondFfu = 0;
    FM_SET_FIELD(regCondFfu, FM10000_TRIGGER_CONDITION_FFU, FFU_ID, 
                 cond->param.ffuId);
    FM_SET_FIELD(regCondFfu, FM10000_TRIGGER_CONDITION_FFU, FFU_Mask, 
                 cond->param.ffuIdMask);

    /* For FM10000_TRIGGER_CONDITION_TYPE */
    regCondType = 0;
    FM_SET_FIELD(regCondType, FM10000_TRIGGER_CONDITION_TYPE, EtherType, 
                 cond->param.etherType);
    FM_SET_FIELD(regCondType, FM10000_TRIGGER_CONDITION_TYPE, EtherTypeMask, 
                 cond->param.etherTypeMask);

    /* For FM10000_TRIGGER_CONDITION_GLORT */
    regCondGlort = 0;
    FM_SET_FIELD(regCondGlort, FM10000_TRIGGER_CONDITION_GLORT, DestGlort, 
                 cond->param.destGlort);
    FM_SET_FIELD(regCondGlort, FM10000_TRIGGER_CONDITION_GLORT, GlortMask, 
                 cond->param.destGlortMask);

    /* For FM10000_TRIGGER_CONDITION_AMASK_1 and
     * FM10000_TRIGGER_CONDITION_AMASK_2 */
    regCondAmask1 = cond->cfg.HAMask & 0xFFFFFFFF;
    regCondAmask2 = (cond->cfg.HAMask >> 32) & 0xFFFFFFFF;

    /*************************************************************** 
     * Now write to HW
     ***************************************************************/

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_PARAM(entryIndex), 
                                 regCondParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_CFG(entryIndex),
                                 regCondCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_FFU(entryIndex),
                                 regCondFfu);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_TYPE(entryIndex),
                                 regCondType);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_GLORT(entryIndex),
                                 regCondGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_AMASK_1(entryIndex),
                                 regCondAmask1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_AMASK_2(entryIndex),
                                 regCondAmask2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_CONDITION_TX(entryIndex, 0),
                                 regCondTx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Write the RX condition last so that it activates the trigger */
    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_CONDITION_RX(entryIndex, 0),
                                 regCondRx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    
ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000WriteTriggerCondition */




/*****************************************************************************/
/** fm10000ReadTriggerCondition
 * \ingroup triggerInt
 *
 * \desc            Read a trigger condition from a HW trigger entry.
 * 
 * \note            We cannot convert the HW portmasks to portsets, for this
 *                  reason portmasks are paramaters passed to this function.
 * 
 * \note            This function assumes that the caller has taken the
 *                  trigger lock.
 * 
 * \param[in]       sw is the switch to operate on
 * 
 * \param[in]       entryIndex is the HW entry index of the trigger
 * 
 * \param[out]      cond is a pointer to the caller allocated storage where
 *                  the condition parameters from HW should be stored. 
 * 
 * \param[out]      matchByPrecedence is a pointer to the caller allocated
 *                  storage where the HW bit that determines if this trigger
 *                  is part of a precedence group or not should be stored.
 * 
 * \param[out]      rxMask is a pointer the the caller allocated storage
 *                  where the rx port mask of this trigger should be stored.
 *                  
 * \param[out]      txMask is a pointer the the caller allocated storage
 *                  where the tx port mask of this trigger should be stored.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000ReadTriggerCondition(fm_int sw, 
                                             fm_uint32 entryIndex, 
                                             fm_triggerCondition *cond,
                                             fm_bool *matchByPrecedence,
                                             fm_uint64 *rxMask,
                                             fm_uint64 *txMask)
{
    fm_status   err;
    fm_switch * switchPtr;

    fm_uint32   regCondCfg;
    fm_uint32   regCondParam;
    fm_uint32   regCondFfu;
    fm_uint32   regCondType;
    fm_uint32   regCondGlort;
    fm_uint64   regCondRx;
    fm_uint64   regCondTx;
    fm_uint32   regCondAmask1;
    fm_uint32   regCondAmask2;

    fm_bool     matchRandomIfLess;
    fm_uint32   matchRandomThreshExp;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, entryIndex = %d, "
                 "cond = %p, matchByPrecedence = %p,"
                 "rxMask = %p, txMask = %p\n",
                 sw, 
                 entryIndex,
                 (void *) cond,
                 (void *) matchByPrecedence,
                 (void *) rxMask,
                 (void *) txMask);

    /*************************************************************** 
     * First, read HW values
     ***************************************************************/

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_PARAM(entryIndex), 
                                &regCondParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_CFG(entryIndex),
                                &regCondCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_FFU(entryIndex),
                                &regCondFfu);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_TYPE(entryIndex),
                                &regCondType);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_GLORT(entryIndex),
                                &regCondGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_AMASK_1(entryIndex),
                                &regCondAmask1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_AMASK_2(entryIndex),
                                &regCondAmask2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_CONDITION_TX(entryIndex, 0),
                                &regCondTx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_CONDITION_RX(entryIndex, 0),
                                &regCondRx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /*************************************************************** 
     * Update the condition structure
     ***************************************************************/

    *rxMask = regCondRx;
    *txMask = regCondTx;
    cond->cfg.HAMask = ((fm_triggerHaCondition)regCondAmask2 << 32) | 
                       regCondAmask1;

    /* For FM10000_TRIGGER_CONDITION_PARAM */
    cond->param.saId = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, SA_ID);
    cond->param.daId = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, DA_ID);
    cond->param.vidId = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, VID_ID);
    cond->param.switchPri = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, SwitchPri);
    cond->cfg.matchFrameClassMask = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, FrameClassMask);
    cond->cfg.matchRoutedMask = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, RoutedMask);
    cond->cfg.matchFtypeMask = 
        FM_GET_FIELD(regCondParam, FM10000_TRIGGER_CONDITION_PARAM, FtypeMask);
    
    /* For FM10000_TRIGGER_CONDITION_CFG */
    cond->cfg.matchSA =        
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchSA);
    cond->cfg.matchDA =        
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchDA);
    cond->cfg.matchHitSA =     
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchHitSA);
    cond->cfg.matchHitDA =     
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchHitDA);
    cond->cfg.matchHitSADA =   
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchHitSADA);
    cond->cfg.matchVlan =      
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchVlan);
    cond->cfg.matchFFU =       
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchFFU);
    cond->cfg.matchSwitchPri = 
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchSwitchPri);
    cond->cfg.matchEtherType = 
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchEtherType);
    cond->cfg.matchDestGlort = 
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchDestGlort);
    *matchByPrecedence =       
        FM_GET_BIT(regCondCfg,   FM10000_TRIGGER_CONDITION_CFG, MatchByPrecedence);
    cond->cfg.matchTx = 
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchTx);

    matchRandomIfLess = 
        FM_GET_BIT(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchRandomIfLess);
    matchRandomThreshExp = 
        FM_GET_FIELD(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchRandomThreshold);
    cond->param.randGenerator = 
        FM_GET_BIT(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchRandomNumber);

    ConvertHWMatchRandomToPublic(matchRandomIfLess, 
                                 matchRandomThreshExp,
                                 &cond->cfg.matchRandomNumber,
                                 &cond->param.randMatchThreshold);

    /* For FM10000_TRIGGER_CONDITION_FFU */
    cond->param.ffuId =         
        FM_GET_FIELD(regCondFfu, FM10000_TRIGGER_CONDITION_FFU, FFU_ID);
    cond->param.ffuIdMask =     
        FM_GET_FIELD(regCondFfu, FM10000_TRIGGER_CONDITION_FFU, FFU_Mask);

    /* For FM10000_TRIGGER_CONDITION_TYPE */
    cond->param.etherType =     
        FM_GET_FIELD(regCondType, FM10000_TRIGGER_CONDITION_TYPE, EtherType);
    cond->param.etherTypeMask = 
        FM_GET_FIELD(regCondType, FM10000_TRIGGER_CONDITION_TYPE, EtherTypeMask);

    /* For FM10000_TRIGGER_CONDITION_GLORT */
    cond->param.destGlort =     
        FM_GET_FIELD(regCondGlort, FM10000_TRIGGER_CONDITION_GLORT, DestGlort);
    cond->param.destGlortMask = 
        FM_GET_FIELD(regCondGlort, FM10000_TRIGGER_CONDITION_GLORT, GlortMask);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000ReadTriggerCondition */




/*****************************************************************************/
/** fm10000WriteTriggerAction
 * \ingroup triggerInt
 *
 * \desc            Write a trigger action to a HW trigger entry
 * 
 * \param[in]       sw is the switch to operate on
 * 
 * \param[in]       entryIndex is the HW entry index of the trigger
 * 
 * \param[in]       action is a pointer to the action parameters to write
 *                  in HW
 * 
 * \param[in]       mirrorIndex is the HW index of the mirror profile.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000WriteTriggerAction(fm_int            sw, 
                                           fm_uint32         entryIndex,
                                           fm_triggerAction *action,
                                           fm_uint32         mirrorIndex)
{
    fm_status   err;

    fm_switch * switchPtr;

    fm_uint32   regActionCfg1;
    fm_uint32   regActionCfg2;
    fm_uint32   regActionGlort;
    fm_uint64   regActionDmask;
    fm_uint32   regActionMirror;
    fm_uint64   regActionDrop;
    fm_portmask portmask; 

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, entryIndex = %d, action = %p, mirrorIndex = %u\n",
                 sw, 
                 entryIndex, 
                 (void *) action,
                 mirrorIndex);

    switchPtr = GET_SWITCH_PTR(sw);

    /*************************************************************** 
     * First convert the software structure to HW register values
     ***************************************************************/

    /* For FM10000_TRIGGER_ACTION_DMASK and FM10000_TRIGGER_ACTION_DROP,
     * we must convert from portset representation to bit mask representation */
    err = fmPortSetToPortMask(sw, action->param.newDestPortset, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmPortMaskLogicalToPhysical(switchPtr, &portmask, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    regActionDmask = ( (fm_uint64)(portmask.maskWord[0]) ) | 
                     ( (fm_uint64)(portmask.maskWord[1]) << 32);

    err = fmPortSetToPortMask(sw, action->param.dropPortset, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmPortMaskLogicalToPhysical(switchPtr, &portmask, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    regActionDrop = ( (fm_uint64)(portmask.maskWord[0]) ) | 
                    ( (fm_uint64)(portmask.maskWord[1]) << 32);

    /* For FM10000_TRIGGER_ACTION_CFG_1 */
    regActionCfg1 = 0;
    FM_SET_FIELD(regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, ForwardingAction, 
                 action->cfg.forwardingAction );
    FM_SET_FIELD(regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, TrapAction, 
                 action->cfg.trapAction );
    FM_SET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, MirroringAction, 
                 action->cfg.mirrorAction );
    FM_SET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, SwitchPriAction, 
                 action->cfg.switchPriAction );
    FM_SET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, VlanAction, 
                 action->cfg.vlan1Action );
    FM_SET_FIELD(regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, LearningAction, 
                 action->cfg.learningAction );
    FM_SET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, RateLimitAction, 
                 action->cfg.rateLimitAction );

    /* For FM10000_TRIGGER_ACTION_CFG_2 */
    regActionCfg2 = 0;
    FM_SET_FIELD(regActionCfg2, FM10000_TRIGGER_ACTION_CFG_2, NewSwitchPri,  
                 action->param.newSwitchPri );
    FM_SET_FIELD(regActionCfg2, FM10000_TRIGGER_ACTION_CFG_2, NewEVID,  
                 action->param.newVlan1 );
    FM_SET_FIELD(regActionCfg2, FM10000_TRIGGER_ACTION_CFG_2, RateLimitNum,  
                 action->param.rateLimitNum );

    /* For FM10000_TRIGGER_ACTION_CFG_GLORT */
    regActionGlort = 0;
    FM_SET_FIELD(regActionGlort, FM10000_TRIGGER_ACTION_GLORT, NewDestGlort,  
                 action->param.newDestGlort );
    FM_SET_FIELD(regActionGlort, FM10000_TRIGGER_ACTION_GLORT, NewDestGlortMask,  
                 action->param.newDestGlortMask );

    /* For FM10000_TRIGGER_ACTION_CFG_DMASK */
    FM_SET_BIT64(regActionDmask, FM10000_TRIGGER_ACTION_DMASK, FilterDestMask,  
                 action->param.filterDestMask );

    /* For FM10000_TRIGGER_ACTION_MIRROR */
    regActionMirror = 0;
    FM_SET_BIT  (regActionMirror, FM10000_TRIGGER_ACTION_MIRROR, MirrorSelect,  
                 action->param.mirrorSelect );
    FM_SET_FIELD(regActionMirror, FM10000_TRIGGER_ACTION_MIRROR, MirrorProfileIndex,  
                 mirrorIndex );
    
    /*************************************************************** 
     * Write registers
     ***************************************************************/

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_CFG_1(entryIndex),
                                 regActionCfg1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_CFG_2(entryIndex),
                                 regActionCfg2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_GLORT(entryIndex),
                                 regActionGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_MIRROR(entryIndex),
                                 regActionMirror);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_ACTION_DMASK(entryIndex, 0),
                                 regActionDmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_TRIGGER_ACTION_DROP(entryIndex, 0),
                                 regActionDrop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000WriteTriggerAction */




/*****************************************************************************/
/** fm10000ReadTriggerAction
 * \ingroup triggerInt
 *
 * \desc            Read trigger action from a HW trigger entry
 * 
 * \note            This function assumes that the caller has taken the
 *                  trigger lock.
 * 
 * \param[in]       sw is the switch to operate on
 * 
 * \param[in]       entryIndex is the HW entry index of the trigger
 * 
 * \param[out]      action is a pointer to the caller-allocated storage where
 *                  the action parameters should be stored.
 * 
 * \param[out]      destMask is a pointer to the caller-allocated storage where
 *                  the new destination mask should be stored.
 * 
 * \param[out]      dropMask is a pointer to the caller-allocated storage where
 *                  the drop mask should be stored.
 * 
 * \param[out]      mirrorIndex points to the location where the hardware
 *                  mirror profile index should be stored.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000ReadTriggerAction(fm_int            sw, 
                                          fm_uint32         entryIndex,
                                          fm_triggerAction *action,
                                          fm_uint64 *       destMask,
                                          fm_uint64 *       dropMask,
                                          fm_uint32 *       mirrorIndex)
{
    fm_switch * switchPtr;
    fm_status   err;

    fm_uint32   regActionCfg1;
    fm_uint32   regActionCfg2;
    fm_uint32   regActionGlort;
    fm_uint64   regActionDmask;
    fm_uint32   regActionMirror;
    fm_uint64   regActionDrop;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, entryIndex = %d, action = %p, "
                 "destMask = %p, dropMask = %p\n",
                 sw, 
                 entryIndex, 
                 (void *) action,
                 (void *) destMask,
                 (void *) dropMask);

    /*************************************************************** 
     * First, read HW values
     ***************************************************************/

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_CFG_1(entryIndex),
                                &regActionCfg1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_CFG_2(entryIndex),
                                &regActionCfg2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_GLORT(entryIndex),
                                &regActionGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_MIRROR(entryIndex),
                                &regActionMirror);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_ACTION_DMASK(entryIndex, 0),
                                &regActionDmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw,
                                FM10000_TRIGGER_ACTION_DROP(entryIndex, 0),
                                &regActionDrop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /*************************************************************** 
     * Update the condition structure
     ***************************************************************/

    *destMask = regActionDmask;
    *dropMask = regActionDrop;

    /* For FM10000_TRIGGER_ACTION_CFG_1 */
    action->cfg.forwardingAction = 
        FM_GET_FIELD(regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, ForwardingAction);
    action->cfg.trapAction = 
        FM_GET_FIELD(regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, TrapAction);
    action->cfg.mirrorAction = 
        FM_GET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, MirroringAction);
    action->cfg.switchPriAction =
        FM_GET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, SwitchPriAction);
    action->cfg.vlan1Action = 
        FM_GET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, VlanAction);
    action->cfg.learningAction = 
        FM_GET_FIELD(regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, LearningAction);
    action->cfg.rateLimitAction = 
        FM_GET_BIT  (regActionCfg1, FM10000_TRIGGER_ACTION_CFG_1, RateLimitAction);

    /* For FM10000_TRIGGER_ACTION_CFG_2 */
    action->param.newSwitchPri = 
        FM_GET_FIELD(regActionCfg2, FM10000_TRIGGER_ACTION_CFG_2, NewSwitchPri);
    action->param.newVlan1 = 
        FM_GET_FIELD(regActionCfg2, FM10000_TRIGGER_ACTION_CFG_2, NewEVID);
    action->param.rateLimitNum = 
        FM_GET_FIELD(regActionCfg2, FM10000_TRIGGER_ACTION_CFG_2, RateLimitNum);
    
    /* For FM10000_TRIGGER_ACTION_CFG_GLORT */
    action->param.newDestGlort = 
        FM_GET_FIELD(regActionGlort, FM10000_TRIGGER_ACTION_GLORT, NewDestGlort);
    action->param.newDestGlortMask = 
        FM_GET_FIELD(regActionGlort, FM10000_TRIGGER_ACTION_GLORT, NewDestGlortMask);
    
    /* For FM10000_TRIGGER_ACTION_CFG_DMASK */
    action->param.filterDestMask = 
        FM_GET_BIT64(regActionDmask, FM10000_TRIGGER_ACTION_DMASK, FilterDestMask);
    
    /* For FM10000_TRIGGER_ACTION_CFG_MIRROR */
    action->param.mirrorSelect = 
        FM_GET_BIT  (regActionMirror, FM10000_TRIGGER_ACTION_MIRROR, MirrorSelect);
    *mirrorIndex = 
        FM_GET_FIELD(regActionMirror, FM10000_TRIGGER_ACTION_MIRROR, MirrorProfileIndex);
    SetTriggerActionMirrorProfile(sw, action, *mirrorIndex);
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000ReadTriggerAction */




/*****************************************************************************/
/** CopyTrigger
 * \ingroup triggerInt
 *
 * \desc            Copy a trigger from an index to another
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       srcIndex is the HW entry index of the trigger to copy.
 * 
 * \param[in]       dstIndex is the HW entry index of the destination index
 *                  where the trigger should be copied.
 * 
 * \param[in]       matchByPrec is the new matchByPrecedence value to set
 *                  in the destination entry.
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CopyTrigger(fm_int  sw, 
                             fm_int  srcIndex, 
                             fm_int  dstIndex,
                             fm_bool matchByPrec)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    fm_uint32   regCondCfg;
    fm_uint32   regCondParam;
    fm_uint32   regCondFfu;
    fm_uint32   regCondType;
    fm_uint32   regCondGlort;
    fm_uint64   regCondRx;
    fm_uint64   regCondTx;
    fm_uint32   regCondAmask1;
    fm_uint32   regCondAmask2;

    fm_uint32   regActionCfg1;
    fm_uint32   regActionCfg2;
    fm_uint32   regActionGlort;
    fm_uint64   regActionDmask;
    fm_uint32   regActionMirror;
    fm_uint64   regActionDrop;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, srcIndex = %d, dstIndex = %d\n",
                 sw, 
                 srcIndex,
                 dstIndex);

    switchPtr = GET_SWITCH_PTR(sw);

    /* It is assumed that the destination index has an invalid trigger
     * condition and cannot hit, disable the trigger anyway for protection. */
    regCondRx = 0;

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_CONDITION_RX(dstIndex, 0),
                                 regCondRx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Read condition.
     **************************************************/

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_PARAM(srcIndex), 
                                &regCondParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_CFG(srcIndex),
                                &regCondCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_FFU(srcIndex),
                                &regCondFfu);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_TYPE(srcIndex),
                                &regCondType);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_GLORT(srcIndex),
                                &regCondGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_AMASK_1(srcIndex),
                                &regCondAmask1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_AMASK_2(srcIndex),
                                &regCondAmask2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_CONDITION_TX(srcIndex, 0),
                                &regCondTx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_CONDITION_RX(srcIndex, 0),
                                &regCondRx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Read action.
     **************************************************/

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_CFG_1(srcIndex),
                                &regActionCfg1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_CFG_2(srcIndex),
                                &regActionCfg2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_GLORT(srcIndex),
                                &regActionGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_ACTION_MIRROR(srcIndex),
                                &regActionMirror);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_ACTION_DMASK(srcIndex, 0),
                                &regActionDmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->ReadUINT64(sw,
                                FM10000_TRIGGER_ACTION_DROP(srcIndex, 0),
                                &regActionDrop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Write action.
     **************************************************/

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_CFG_1(dstIndex),
                                 regActionCfg1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_CFG_2(dstIndex),
                                 regActionCfg2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_GLORT(dstIndex),
                                 regActionGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_ACTION_MIRROR(dstIndex),
                                 regActionMirror);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_ACTION_DMASK(dstIndex, 0),
                                 regActionDmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_TRIGGER_ACTION_DROP(dstIndex, 0),
                                 regActionDrop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Write condition.
     **************************************************/

    FM_SET_BIT(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchByPrecedence,
               matchByPrec);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_PARAM(dstIndex), 
                                 regCondParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_CFG(dstIndex),
                                 regCondCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_FFU(dstIndex),
                                 regCondFfu);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_TYPE(dstIndex),
                                 regCondType);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_GLORT(dstIndex),
                                 regCondGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_AMASK_1(dstIndex),
                                 regCondAmask1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_CONDITION_AMASK_2(dstIndex),
                                 regCondAmask2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_CONDITION_TX(dstIndex, 0),
                                 regCondTx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /**************************************************
     * Write RX condition last, to activate trigger.
     **************************************************/

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_CONDITION_RX(dstIndex, 0),
                                 regCondRx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end CopyTrigger */




/*****************************************************************************/
/** FreeTriggerEntry
 * \ingroup triggerInt
 *
 * \desc            Frees the memory allocated for a
 *                  fm10000_triggerEntry structure and its members.
 * 
 * \param[in]       ptr is a pointer to a fm10000_triggerEntry structure. 
 * 
 * \return          None
 *                  
 *****************************************************************************/
static void FreeTriggerEntry(void *ptr)
{
    fm10000_triggerEntry *trigEntry;

    trigEntry = (fm10000_triggerEntry *)ptr;

    if ((trigEntry->cond != NULL) && 
        (trigEntry->cond != &invalidCond))
    {
        fmFree(trigEntry->cond);
    }

    if ((trigEntry->action != NULL) && 
        (trigEntry->action != &invalidAction))
    {
        fmFree(trigEntry->action);
    }

    if (trigEntry->name != NULL)
    {
        fmFree(trigEntry->name);
    }

    fmFree(trigEntry);

}   /* end FreeTriggerEntry */




/*****************************************************************************/
/** MoveTrigger
 * \ingroup triggerInt
 *
 * \desc            Move a trigger from its HW index up or down. The source
 *                  entry will get invalidated but will keep its
 *                  matchByPrecedence setting.
 * 
 * \note            This function assumes that the destination entry is
 *                  in the invalid state.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       trigEntry is a pointer to the trigger entry to work on.
 * 
 * \param[in]       direction indicates whether the trigger
 *                  should be moved up (0) or moved down (1) in HW. Note that
 *                  up means closer to index 0. 
 * 
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if the trigger cannot be moved in
 *                  the desired direction.
 *
 *****************************************************************************/
static fm_status MoveTrigger(fm_int                sw, 
                             fm10000_triggerEntry* trigEntry,
                             fm_bool               direction)
{
    fm_switch * switchPtr;
    fm_status   err;
    fm_uint32   srcIndex;
    fm_uint32   dstIndex;
    fm_uint32   regCondCfg;
    fm_bool     srcMatchByPrec;
    fm_uint64   srcCounter;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, trigEntry = %p, direction = %d\n",
                 sw, 
                 (void *) trigEntry,
                 direction);

    switchPtr = GET_SWITCH_PTR(sw);

    srcIndex = trigEntry->index;

    /* Retrieve the destination index and validate that we don't exceed
     * register index bounds */
    if (direction == 0)
    {
        if (srcIndex == 0)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
        }

        dstIndex = srcIndex - 1;
    }
    else
    {
        if (srcIndex >= (FM10000_MAX_HW_TRIGGERS - 1))
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
        }

        dstIndex = srcIndex + 1;
    }

    if (trigEntry->isBound)
    {
        /* Reserve the destination mirror profile index.
         * If the entry is in use, relocate it. */
        err = fm10000ReserveMirrorProfile(sw, dstIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        /* Copy the mirror profile to the new location. */
        err = fm10000CopyMirrorProfile(sw, srcIndex, dstIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Preserve the matchByPrecedence field of the src trigger. */
    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_CONDITION_CFG(srcIndex),
                                &regCondCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    srcMatchByPrec = 
        FM_GET_BIT(regCondCfg, FM10000_TRIGGER_CONDITION_CFG, MatchByPrecedence);

    /* Clear the trigger counter of destination entry. */
    err = switchPtr->WriteUINT64(sw, FM10000_TRIGGER_STATS(dstIndex, 0), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = CopyTrigger(sw, srcIndex, dstIndex, trigEntry->matchByPrec);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (trigEntry->isBound)
    {
        /* Update destination profile. */
        err = fm10000UpdateLogProfile(sw, trigEntry->logProfile, dstIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        /* Free source profile. */
        err = fm10000ReleaseMirrorProfile(sw, srcIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Let's invalidate the src trigger, without modifying the
     * matchByPrecedence field so that we don't disrupt grouping. */
    err = fm10000WriteTriggerCondition(sw, 
                                       srcIndex, 
                                       &invalidCond, 
                                       srcMatchByPrec);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fm10000WriteTriggerAction(sw, srcIndex, &invalidAction, 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Update the trigger stats in SW. */
    err = switchPtr->ReadUINT64(sw, 
                                FM10000_TRIGGER_STATS(srcIndex, 0), 
                                &srcCounter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    trigEntry->counterCache += srcCounter;

    trigEntry->index = dstIndex;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end MoveTrigger */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000InitTriggers
 * \ingroup triggerInt
 *
 * \desc            Initialize the trigger structures
 * 
 * \param[in]       switchPtr is the fm_switch pointer of the switch to
 *                  initialize.
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitTriggers(fm_switch *switchPtr)
{
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_status            err;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "switchPtr = %p\n", (void *) switchPtr);

    switchExt = GET_SWITCH_EXT(switchPtr->switchNumber);
    
    trigInfo = &switchExt->triggerInfo;
    trigInfo->numUsedTriggers = 0;

    fmTreeInit(&trigInfo->triggerTree);

    err = fmCreateBitArray(&trigInfo->usedMacTrigID, 
                           FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->macTrigIdInternal, 
                           FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->usedVlanTrigID, 
                           FM10000_TRIGGER_VLAN_TRIG_ID_MAX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->vlanTrigIdInternal, 
                           FM10000_TRIGGER_VLAN_TRIG_ID_MAX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->usedFFUTrigIDBits, 
                           FM10000_TRIGGER_FFU_TRIG_ID_BITS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->FFUTrigIdInternal, 
                           FM10000_TRIGGER_FFU_TRIG_ID_BITS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->usedRateLimiterID, 
                           FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmCreateBitArray(&trigInfo->rateLimiterIdInternal, 
                           FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Reserve (and mark as internal) entry 0 and entry 63 for the MAC
     * trigger ID. See bugzilla 20776 for the reason why both are reserved */
    err = fmSetBitArrayBit(&trigInfo->usedMacTrigID, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmSetBitArrayBit(&trigInfo->usedMacTrigID, 63, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmSetBitArrayBit(&trigInfo->macTrigIdInternal, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmSetBitArrayBit(&trigInfo->macTrigIdInternal, 63, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Reserve (and mark as internal) entry 0 for the Vlan trigger ID.
     * This is the default that should be used */
    err = fmSetBitArrayBit(&trigInfo->usedVlanTrigID, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmSetBitArrayBit(&trigInfo->vlanTrigIdInternal, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    FM_CLEAR(trigInfo->rateLimPortSet);
    
    /******************************************************************* 
     * Initialize mirror profiles.
     ******************************************************************/ 

    err = fm10000InitMirrorProfiles(switchPtr->switchNumber);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    
    /******************************************************************* 
     * Validation that the public enums match the FM10000 HW
     * definitions. If the public definitions change, this piece of 
     * code shall detect incorrect mappings on initialization. 
     ******************************************************************/ 

    FM_API_REQUIRE(FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL == 
                   FM10000_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL, 
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_MATCHCASE_MATCHIFEQUAL == 
                   FM10000_TRIGGER_MATCHCASE_MATCHIFEQUAL, 
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL == 
                   FM10000_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL, 
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_RAND_GEN_A == 
                   FM10000_TRIGGER_RAND_GEN_A, 
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_RAND_GEN_B == 
                   FM10000_TRIGGER_RAND_GEN_B, 
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FRAME_CLASS_UCAST ==
                   FM10000_TRIGGER_FRAME_CLASS_UCAST,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FRAME_CLASS_MCAST ==
                   FM10000_TRIGGER_FRAME_CLASS_MCAST,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FRAME_CLASS_BCAST ==
                   FM10000_TRIGGER_FRAME_CLASS_BCAST,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_SWITCHED_FRAMES ==
                   FM10000_TRIGGER_SWITCHED_FRAMES,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_ROUTED_FRAMES ==
                   FM10000_TRIGGER_ROUTED_FRAMES,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FTYPE_NORMAL ==
                   FM10000_TRIGGER_FTYPE_NORMAL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FTYPE_SPECIAL ==
                   FM10000_TRIGGER_FTYPE_SPECIAL,
                   FM_ERR_ASSERTION_FAILED);

    /* Validation of handler action codes */
    FM_API_REQUIRE(FM_TRIGGER_HA_FORWARD_SPECIAL ==
                   FM10000_TRIGGER_HA_FORWARD_SPECIAL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_PARSE_ERROR ==
                   FM10000_TRIGGER_HA_DROP_PARSE_ERROR,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_PARITY_ERROR ==
                   FM10000_TRIGGER_HA_DROP_PARITY_ERROR,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_RESERVED_MAC ==
                   FM10000_TRIGGER_HA_TRAP_RESERVED_MAC,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP ==
                   FM10000_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_LOG_RESERVED_MAC ==
                   FM10000_TRIGGER_HA_LOG_RESERVED_MAC,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_SWITCH_RESERVED_MAC ==
                   FM10000_TRIGGER_HA_SWITCH_RESERVED_MAC,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_RESERVED_MAC ==
                   FM10000_TRIGGER_HA_DROP_RESERVED_MAC,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_TAG ==
                   FM10000_TRIGGER_HA_DROP_TAG,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_CONTROL ==
                   FM10000_TRIGGER_HA_DROP_CONTROL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_INVALID_SMAC ==
                   FM10000_TRIGGER_HA_DROP_INVALID_SMAC,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_PORT_SV ==
                   FM10000_TRIGGER_HA_DROP_PORT_SV,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_CPU ==
                   FM10000_TRIGGER_HA_TRAP_CPU,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_STP_INL ==
                   FM10000_TRIGGER_HA_DROP_STP_INL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_VLAN_IV ==
                   FM10000_TRIGGER_HA_DROP_VLAN_IV,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_STP_IL ==
                   FM10000_TRIGGER_HA_DROP_STP_IL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_FFU ==
                   FM10000_TRIGGER_HA_DROP_FFU,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_FFU ==
                   FM10000_TRIGGER_HA_TRAP_FFU,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_ICMP_TTL ==
                   FM10000_TRIGGER_HA_TRAP_ICMP_TTL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_IP_OPTION ==
                   FM10000_TRIGGER_HA_TRAP_IP_OPTION,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_MTU ==
                   FM10000_TRIGGER_HA_TRAP_MTU,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_IGMP ==
                   FM10000_TRIGGER_HA_TRAP_IGMP,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_TRAP_TTL ==
                   FM10000_TRIGGER_HA_TRAP_TTL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_LOG_IP_MCST_TTL ==
                   FM10000_TRIGGER_HA_LOG_IP_MCST_TTL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_TTL ==
                   FM10000_TRIGGER_HA_DROP_TTL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_NULL_DEST ==
                   FM10000_TRIGGER_HA_DROP_NULL_DEST,
                   FM_ERR_ASSERTION_FAILED);
    
    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_VLAN_EV ==
                   FM10000_TRIGGER_HA_DROP_VLAN_EV,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_DLF ==
                   FM10000_TRIGGER_HA_DROP_DLF,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_GLORT_CAM_MISS ==
                   FM10000_TRIGGER_HA_DROP_GLORT_CAM_MISS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_POLICER ==
                   FM10000_TRIGGER_HA_DROP_POLICER,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_STP_E ==
                   FM10000_TRIGGER_HA_DROP_STP_E,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_DROP_LOOPBACK ==
                   FM10000_TRIGGER_HA_DROP_LOOPBACK,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_FORWARD_DGLORT == 
                   FM10000_TRIGGER_HA_FORWARD_DGLORT,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_FORWARD_FLOOD ==
                   FM10000_TRIGGER_HA_FORWARD_FLOOD,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_FORWARD_FID ==
                   FM10000_TRIGGER_HA_FORWARD_FID,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_PARSE_TIMEOUT ==
                   FM10000_TRIGGER_HA_PARSE_TIMEOUT,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_LOG_FFU_I ==
                   FM10000_TRIGGER_HA_LOG_FFU_I,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_LOG_ARP_REDIRECT ==
                   FM10000_TRIGGER_HA_LOG_ARP_REDIRECT,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_MIRROR_FFU ==
                   FM10000_TRIGGER_HA_MIRROR_FFU,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_HA_LOG_MCST_ICMP_TTL ==
                   FM10000_TRIGGER_HA_LOG_MCST_ICMP_TTL,
                   FM_ERR_ASSERTION_FAILED);
    
    /* Validation of action defines */

    FM_API_REQUIRE(FM_TRIGGER_FORWARDING_ACTION_ASIS ==
                   FM10000_TRIGGER_FORWARDING_ACTION_ASIS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FORWARDING_ACTION_FORWARD ==
                   FM10000_TRIGGER_FORWARDING_ACTION_FORWARD,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FORWARDING_ACTION_REDIRECT ==
                   FM10000_TRIGGER_FORWARDING_ACTION_REDIRECT,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_FORWARDING_ACTION_DROP ==
                   FM10000_TRIGGER_FORWARDING_ACTION_DROP,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_TRAP_ACTION_ASIS ==
                   FM10000_TRIGGER_TRAP_ACTION_ASIS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_TRAP_ACTION_TRAP ==
                   FM10000_TRIGGER_TRAP_ACTION_TRAP,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_TRAP_ACTION_LOG ==
                   FM10000_TRIGGER_TRAP_ACTION_LOG,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG ==
                   FM10000_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_MIRROR_ACTION_NONE ==
                   FM10000_TRIGGER_MIRROR_ACTION_NONE,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_MIRROR_ACTION_NONE ==
                   FM10000_TRIGGER_MIRROR_ACTION_NONE,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_SWPRI_ACTION_ASIS ==
                   FM10000_TRIGGER_SWPRI_ACTION_ASIS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_SWPRI_ACTION_REASSIGN ==
                   FM10000_TRIGGER_SWPRI_ACTION_REASSIGN,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_VLAN_ACTION_ASIS ==
                   FM10000_TRIGGER_VLAN_ACTION_ASIS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_VLAN_ACTION_REASSIGN ==
                   FM10000_TRIGGER_VLAN_ACTION_REASSIGN,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_LEARN_ACTION_ASIS ==
                   FM10000_TRIGGER_LEARN_ACTION_ASIS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_LEARN_ACTION_CANCEL ==
                   FM10000_TRIGGER_LEARN_ACTION_CANCEL,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_RATELIMIT_ACTION_ASIS ==
                   FM10000_TRIGGER_RATELIMIT_ACTION_ASIS,
                   FM_ERR_ASSERTION_FAILED);

    FM_API_REQUIRE(FM_TRIGGER_RATELIMIT_ACTION_RATELIMIT ==
                   FM10000_TRIGGER_RATELIMIT_ACTION_RATELIMIT,
                   FM_ERR_ASSERTION_FAILED);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}  /* end fm10000InitTriggers */




/*****************************************************************************/
/** fm10000DestroyTriggers
 * \ingroup triggerInt
 *
 * \desc            Destroy all trigger structures
 * 
 * \param[in]       switchPtr is the fm_switch pointer of the switch to destroy.
 *                  
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DestroyTriggers(fm_switch *switchPtr)
{
    fm_status            err = FM_OK;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, "switchPtr = %p\n", (void *) switchPtr);
    
    switchExt = GET_SWITCH_EXT(switchPtr->switchNumber);
    trigInfo  = &switchExt->triggerInfo;
    
    /******************************************************************* 
     * Free trigger resources.
     ******************************************************************/ 

    fmTreeDestroy(&trigInfo->triggerTree, FreeTriggerEntry);

    err = fmDeleteBitArray(&trigInfo->usedMacTrigID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->macTrigIdInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->usedVlanTrigID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->vlanTrigIdInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->usedFFUTrigIDBits);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->FFUTrigIdInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->usedRateLimiterID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->rateLimiterIdInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    
    /******************************************************************* 
     * Free mirror profile resources.
     ******************************************************************/ 

    err = fmDeleteBitArray(&trigInfo->usedProfileHandle);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->profileHandleInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmDeleteBitArray(&trigInfo->usedProfileIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000DestroyTriggers */




/*****************************************************************************/
/** fm10000CreateTrigger
 * \ingroup triggerInt
 *
 * \desc            Create a new trigger for exclusive use by the
 *                  application. It is up to the application to
 *                  ensure that such triggers do not modify any core
 *                  API functionality. 
 * 
 * \note            The created trigger will have an invalid condition and an
 *                  empty action until the ''fmSetTriggerCondition'' and
 *                  ''fmSetTriggerAction'' are called.
 *                  
 * \note            Triggers created by the API for internal use should
 *                  use group numbers below 10000 and provide sufficient
 *                  spacing (64 minimum) for a customer to insert his
 *                  own triggers in the case that a behavior needs to be
 *                  overridden. The suggested model is to use triggers group IDs
 *                  that are multiples of 100.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group in which the trigger should be created. 
 *                  The rules created within a group are mutually
 *                  exclusive, i.e. only one can hit. In the event that two
 *                  rules in two different groups have conflicting actions,
 *                  the group with the lowest precedence wins.
 * 
 * \param[in]       rule is the rule number within a group. Lowered numbered
 *                  rules have more precedence.
 * 
 * \param[in]       isInternal is should be set to true for the trigger
 *                  to be only modifiable/deletable by internal (fm10000_xxx)
 *                  calls. 
 * 
 * \param[in]       name is the name of the trigger, which is used for 
 *                  diagnostic purposes only and is displayed by
 *                  ''fmDbgDumpTriggers''.
 * 
 * \return          FM_OK if successful.
 *                  FM_ERR_ALREADY_EXISTS a trigger already exists for the
 *                  provided group/rule combination.
 *                  FM_ERR_TRIGGER_UNAVAILABLE if there are no more free
 *                  triggers.
 * 
 *****************************************************************************/
fm_status fm10000CreateTrigger(fm_int  sw, 
                               fm_int  group, 
                               fm_int  rule, 
                               fm_bool isInternal,
                               fm_text name)
{
    fm_status             err = FM_OK;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;
    fm_bool               trigEntryAllocated;
    fm_bool               trigEntryInserted;
    
    fm_treeIterator       triggerIt;

    fm_uint64             nextKey;
    fm10000_triggerEntry *nextTrigEntry;
    fm10000_triggerEntry *lastTrigEntry;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d\n",
                 sw,
                 group, 
                 rule);

    TAKE_TRIGGER_LOCK(sw);

    trigEntryAllocated = FALSE;
    trigEntryInserted  = FALSE;
    switchPtr          = GET_SWITCH_PTR(sw);
    switchExt          = GET_SWITCH_EXT(sw);
    trigInfo           = &switchExt->triggerInfo;

    if (trigInfo->numUsedTriggers >= FM10000_MAX_HW_TRIGGERS)
    {
        err = FM_ERR_TRIGGER_UNAVAILABLE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) == FM_OK)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    trigEntry = fmAlloc(sizeof(fm10000_triggerEntry));
    if (trigEntry == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }
    else
    {
        trigEntryAllocated = TRUE;
    }

    /* Initialize the trigger entry, the HW index is unknown at this time */
    trigEntry->index        = 0xFFFFFFFF;
    trigEntry->cond         = &invalidCond;
    trigEntry->action       = &invalidAction;
    trigEntry->counterCache = 0;
    trigEntry->isInternal   = isInternal;
    trigEntry->mirrorIndex  = 0;
    trigEntry->logProfile   = -1;
    trigEntry->isBound      = FALSE;

    if (name != NULL)
    {
        trigEntry->name = fmStringDuplicate(name);
    }
    else
    {
        trigEntry->name = NULL;
    }

    /* Insert the entry in the tree */
    err = fmTreeInsert(&trigInfo->triggerTree, 
                       FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule), 
                       (void*) trigEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    trigEntryInserted = TRUE;
    lastTrigEntry     = NULL;

    err = UpdateMatchByPrec(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Starting from the last trigger, move the trigger entries down
     * until we reach our new trigger */
    fmTreeIterInitBackwards(&triggerIt, &trigInfo->triggerTree);

    while ( (err = fmTreeIterNext(&triggerIt, 
                                  &nextKey, 
                                  (void**) &nextTrigEntry)) == FM_OK)
    {
        /* We're at the trigger we created, just update the index */
        if (trigEntry == nextTrigEntry)
        {
            if (lastTrigEntry != NULL)
            {
                trigEntry->index = (lastTrigEntry->index - 1);
            }
            else
            {
                trigEntry->index = trigInfo->numUsedTriggers;
            }

            /* Write the trigger as invalid, start with condition in case
             * there was an old trigger active and so that the
             * matchByPrecedence has the right value in HW. */
            err = fm10000WriteTriggerCondition(sw, 
                                               trigEntry->index, 
                                               trigEntry->cond, 
                                               trigEntry->matchByPrec);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

            err = fm10000WriteTriggerAction(sw, 
                                            trigEntry->index, 
                                            trigEntry->action,
                                            trigEntry->mirrorIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

            /* Clear the trigger counter. */
            err = switchPtr->WriteUINT64(sw, 
                                         FM10000_TRIGGER_STATS(trigEntry->index, 0), 
                                         0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

            trigInfo->numUsedTriggers++;
            break;
        }

        err = MoveTrigger(sw, nextTrigEntry, 1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        lastTrigEntry = nextTrigEntry;
    }

    /* End of iteration */
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

ABORT:
    if (err != FM_OK)
    {
        if (trigEntryInserted)
        {
            fmTreeRemoveCertain(&trigInfo->triggerTree, 
                                FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule), 
                                NULL);
            UpdateMatchByPrec(sw);
        }

        if (trigEntryAllocated)
        {
            if (trigEntry->name != NULL)
            {
                fmFree(trigEntry->name);
            }
            fmFree(trigEntry);
        }
    }

    DROP_TRIGGER_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000CreateTrigger */




/*****************************************************************************/
/** fm10000DeleteTrigger
 * \ingroup triggerInt
 *
 * \desc            Delete a trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group in which the trigger to delete is
 *                  located. 
 * 
 * \param[in]       rule is the rule number within a group to delete. If this
 *                  was the last rule in a group, this group simply doesn't
 *                  exist anymore.
 * 
 * \param[in]       isInternal should be set to true for an internal trigger
 *                  to be deletable.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND a trigger does not exist for the provided
 *                  group/rule combination.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is internal (
 *                  internal triggers can only be deleted by API internals).
 * 
 *****************************************************************************/
fm_status fm10000DeleteTrigger(fm_int  sw, 
                               fm_int  group, 
                               fm_int  rule, 
                               fm_bool isInternal)
{
    fm_status             err; 
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;
    fm_treeIterator       triggerIt;
    fm_uint64             nextKey;
    fm10000_triggerEntry *nextTrigEntry;
    fm_bool               isLastEntry = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d\n",
                 sw,
                 group, 
                 rule);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    if ( (trigEntry->isInternal == TRUE) && 
         (trigEntry->isInternal != isInternal) )
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Free up memory */
    if (trigEntry->cond != NULL && trigEntry->cond != &invalidCond)
    {
        fmFree(trigEntry->cond);
    }

    if (trigEntry->action != NULL && trigEntry->action != &invalidAction)
    {
        fmFree(trigEntry->action);
    }

    if (trigEntry->name != NULL)
    {
        fmFree(trigEntry->name);
    }

    /* We must first invalidate the trigger before moving the others */
    trigEntry->cond   = &invalidCond;
    trigEntry->action = &invalidAction;
    trigEntry->mirrorIndex = 0;

    /* Start with condition to disable it, keep the same matchByPrec value
     * as before */
    err = fm10000WriteTriggerCondition(sw, 
                                       trigEntry->index, 
                                       trigEntry->cond, 
                                       trigEntry->matchByPrec);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fm10000WriteTriggerAction(sw, 
                                    trigEntry->index, 
                                    trigEntry->action,
                                    trigEntry->mirrorIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Delete the log action mirror profile if we created one. */
    if (trigEntry->logProfile >= 0)
    {
        err = fm10000DeleteLogProfile(sw, trigEntry->logProfile);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        trigEntry->logProfile = -1;
        trigEntry->isBound = FALSE;
    }

    /* Before removing the trigger from the tree, get the successor which 
     * will be used as the starting point for moving the entries up
     * (to lower indexes). */
    err = fmTreeSuccessor(&trigInfo->triggerTree, 
                          FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule), 
                          &nextKey,
                          (void**) &nextTrigEntry);

    if (err == FM_ERR_NO_MORE)
    {
        isLastEntry = TRUE;
        err = FM_OK;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Remove the entry from the tree (and free memory) */
    err = fmTreeRemoveCertain(&trigInfo->triggerTree, 
                              FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                              fmFree);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    trigInfo->numUsedTriggers--;

    err = UpdateMatchByPrec(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (!isLastEntry)
    {
        /* Starting from the deleted trigger's next entry, move entries up
         * (overwriting the deleted trigger) */
        fmTreeIterInitFromKey(&triggerIt, &trigInfo->triggerTree, nextKey);

        while ( (err = fmTreeIterNext(&triggerIt, 
                                      &nextKey, 
                                      (void**) &nextTrigEntry)) == FM_OK)
        {
            err = MoveTrigger(sw, nextTrigEntry, 0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
        }

        /* End of iteration */        
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
    }

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000DeleteTrigger */




/*****************************************************************************/
/** fm10000SetTriggerCondition
 * \ingroup triggerInt
 *
 * \desc            Set the conditions for a given trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the trigger group ID of the trigger to config.
 * 
 * \param[in]       rule is the trigger rule ID (within group) of the trigger
 *                  to config.
 *
 * \param[in]       cond is a pointer to the condition vector to apply for
 *                  this trigger.
 * 
 * \param[in]       isInternal should be set to true for an internal trigger
 *                  to be modifiable. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is internal (
 *                  internal triggers can only be modified by API internals).
 * 
 *****************************************************************************/
fm_status fm10000SetTriggerCondition(fm_int                     sw, 
                                     fm_int                     group, 
                                     fm_int                     rule, 
                                     const fm_triggerCondition *cond,
                                     fm_bool                    isInternal)
{
    fm_status             err; 
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, cond = %p\n",
                 sw,
                 group, 
                 rule,
                 (void *) cond);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    if ( (trigEntry->isInternal == TRUE) && 
         (trigEntry->isInternal != isInternal) )
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Allocate a condition structure to keep a copy of the
     * condition being set. */
    if (trigEntry->cond == NULL || trigEntry->cond == &invalidCond)
    {
        trigEntry->cond = fmAlloc(sizeof(fm_triggerCondition));

        if (trigEntry->cond == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
        }
    }

    FM_MEMCPY_S(trigEntry->cond, 
                sizeof(fm_triggerCondition), 
                cond, 
                sizeof(*cond));

    /* Write to HW */
    err = fm10000WriteTriggerCondition(sw, 
                                       trigEntry->index, 
                                       trigEntry->cond,
                                       trigEntry->matchByPrec);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000SetTriggerCondition */




/*****************************************************************************/
/** fm10000SetTriggerAction
 * \ingroup triggerInt
 *
 * \desc            Set the actions for a given trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the trigger group ID of the trigger to configure.
 * 
 * \param[in]       rule is the trigger rule ID (within group) of the trigger
 *                  to configure.
 *
 * \param[in]       action is a pointer to the action vector to apply for
 *                  this trigger.
 * 
 * \param[in]       isInternal should be set to true for an internal trigger
 *                  to be modifiable. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is internal (
 *                  internal triggers can only be modified by API internals).
 * \return          FM_ERR_INVALID_MIRROR_PROFILE if the mirroring action
 *                  specifies an invalid mirrorProfile or the profile is
 *                  the wrong type.
 * 
 *****************************************************************************/
fm_status fm10000SetTriggerAction(fm_int                  sw, 
                                  fm_int                  group, 
                                  fm_int                  rule, 
                                  const fm_triggerAction *action,
                                  fm_bool                 isInternal)
{
    fm_status             err; 
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, action = %p\n",
                 sw,
                 group, 
                 rule,
                 (void *) action);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    if ( (trigEntry->isInternal == TRUE) && 
         (trigEntry->isInternal != isInternal) )
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Allocate an action structure to keep a copy of the
     * action being set. */
    if (trigEntry->action == NULL || trigEntry->action == &invalidAction)
    {
        trigEntry->action = fmAlloc(sizeof(fm_triggerAction));

        if (trigEntry->action == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
        }
    }

    FM_MEMCPY_S(trigEntry->action, 
                sizeof(fm_triggerAction), 
                action, 
                sizeof(*action));

    /* Update mirror profiles used by this trigger. */
    err = fm10000SetMirrorProfileAction(sw, group, rule, trigEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Write to HW */
    err = fm10000WriteTriggerAction(sw, 
                                    trigEntry->index, 
                                    trigEntry->action,
                                    trigEntry->mirrorIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000SetTriggerAction */




/*****************************************************************************/
/** fm10000GetTrigger
 * \ingroup triggerInt
 *
 * \desc            Get the condition and action configurations for a trigger. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the trigger group ID of the trigger to retrieve.
 * 
 * \param[in]       rule is the trigger rule ID (within group) of the trigger
 *                  to retrieve.
 * 
 * \param[out]      cond is a pointer to the caller allocated storage where
 *                  this trigger's condtion vector should be stored. 
 * 
 * \param[out]      action is a pointer to the caller allocated storage where
 *                  this trigger's action vector should be stored. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * 
 *****************************************************************************/
fm_status fm10000GetTrigger(fm_int               sw, 
                            fm_int               group, 
                            fm_int               rule, 
                            fm_triggerCondition *cond,
                            fm_triggerAction *   action)
{
    fm_status             err = FM_OK; 
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;
    fm_bool               matchRandomIfLess;
    fm_uint32             matchRandomThreshExp;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, cond = %p, action = %p\n",
                 sw,
                 group, 
                 rule,
                 (void *) cond,
                 (void *) action);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    FM_MEMCPY_S(cond, 
                sizeof(*cond), 
                trigEntry->cond, 
                sizeof(fm_triggerCondition));

    FM_MEMCPY_S(action, 
                sizeof(*action), 
                trigEntry->action, 
                sizeof(fm_triggerAction));

    /* Read back the effective value and adjust the trigger configuration */
    if (cond->cfg.matchRandomNumber)
    {
        ConvertPublicMatchRandomToHW(cond->cfg.matchRandomNumber, 
                                     cond->param.randMatchThreshold,
                                     &matchRandomIfLess,
                                     &matchRandomThreshExp);

        ConvertHWMatchRandomToPublic(matchRandomIfLess, 
                                     matchRandomThreshExp,
                                     &cond->cfg.matchRandomNumber,
                                     &cond->param.randMatchThreshold);
    }

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTrigger */




/*****************************************************************************/
/** fm10000GetTriggerFirst
 * \ingroup triggerInt
 *
 * \desc            Find the first trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      group is a pointer to the caller allocated storage where
 *                  the first trigger's group should be stored. 
 * 
 * \param[out]      rule is a pointer to the caller allocated storage where
 *                  the first trigger's rule should be stored. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there is no trigger.
 *
 *****************************************************************************/
fm_status fm10000GetTriggerFirst(fm_int sw, fm_int *group, fm_int *rule)
{
    fm_status             err = FM_OK;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm_treeIterator       triggerIt;
    fm_uint64             nextKey;
    fm10000_triggerEntry *nextTrigEntry;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %p, rule = %p\n",
                 sw,
                 (void *) group, 
                 (void *) rule );

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    fmTreeIterInit(&triggerIt, &trigInfo->triggerTree);

    err = fmTreeIterNext(&triggerIt, &nextKey, (void**) &nextTrigEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
   
    *group = FM10000_TRIGGER_KEY_TO_GROUP(nextKey);
    *rule  = FM10000_TRIGGER_KEY_TO_RULE(nextKey);

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerFirst */




/*****************************************************************************/
/** fm10000GetTriggerNext
 * \ingroup triggerInt
 *
 * \desc            Get the trigger after the specified trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       curGroup is the group of the current trigger. 
 * 
 * \param[in]       curRule is the rule of the current trigger 
 * 
 * \param[out]      nextGroup is a pointer to the caller allocated storage
 *                  where the next trigger's group should be stored. 
 * 
 * \param[out]      nextRule is a pointer to the caller allocated storage
 *                  where the next trigger's rule should be stored. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_NO_MORE if there is no more trigger.
 *
 *****************************************************************************/
fm_status fm10000GetTriggerNext(fm_int sw, 
                                fm_int curGroup, 
                                fm_int curRule,
                                fm_int *nextGroup, 
                                fm_int *nextRule)
{
    fm_status             err = FM_OK;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm_uint64             curKey;
    fm_uint64             nextKey;
    fm10000_triggerEntry *nextTrigEntry;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, curGroup = %d, curRule = %d, "
                 "nextGroup = %p, nextRule = %p\n",
                 sw,
                 curGroup, 
                 curRule,
                 (void *) nextGroup,
                 (void *) nextRule);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    curKey = FM10000_TRIGGER_GROUP_RULE_TO_KEY(curGroup, curRule);

    err = fmTreeSuccessor(&trigInfo->triggerTree, 
                          curKey, 
                          &nextKey, 
                          (void**) &nextTrigEntry);
    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }
   
    *nextGroup = FM10000_TRIGGER_KEY_TO_GROUP(nextKey);
    *nextRule  = FM10000_TRIGGER_KEY_TO_RULE(nextKey);

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerNext */




/*****************************************************************************/
/** fm10000AllocateTriggerResource
 * \ingroup triggerInt
 *
 * \desc            Allocate a resource for use with triggers. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to allocate.
 * 
 * \param[out]      value is a pointer to the the caller-allocated storage
 *                  where the value for the allocated resource should
 *                  be stored.
 * 
 * \param[in]       isInternal is should be set to true for the trigger
 *                  resource to be only deletable by internal (fm10000_xxx)
 *                  calls.   
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no more free
 *                  resources for the specified resource type. 
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type is not valid.
 *
 *****************************************************************************/
fm_status fm10000AllocateTriggerResource(fm_int                 sw, 
                                         fm_triggerResourceType res, 
                                         fm_uint32 *            value,
                                         fm_bool                isInternal)
{
    fm_status            err = FM_OK;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_bitArray *        resBitArrayPtr;
    fm_bitArray *        isIntBitArrayPtr;
    fm_int               bitNo;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, value = %p\n",
                 sw,
                 res, 
                 (void *) value);

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    switch (res)
    {
        case FM_TRIGGER_RES_MAC_ADDR:
            resBitArrayPtr =   &trigInfo->usedMacTrigID;
            isIntBitArrayPtr = &trigInfo->macTrigIdInternal;
            break;

        case FM_TRIGGER_RES_VLAN:
            resBitArrayPtr   = &trigInfo->usedVlanTrigID;
            isIntBitArrayPtr = &trigInfo->vlanTrigIdInternal;
            break;

        case FM_TRIGGER_RES_FFU:
            resBitArrayPtr   = &trigInfo->usedFFUTrigIDBits;
            isIntBitArrayPtr = &trigInfo->FFUTrigIdInternal;
            break;

        case FM_TRIGGER_RES_RATE_LIMITER:
            resBitArrayPtr   = &trigInfo->usedRateLimiterID;
            isIntBitArrayPtr = &trigInfo->rateLimiterIdInternal;
            break;

        case FM_TRIGGER_RES_MIRROR_PROFILE:
            err = fm10000CreateMirrorProfile(sw, value, isInternal);
            goto ABORT;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    }   /* end switch (res) */

    /* Find the first available bit in the bit array. */
    err = fmFindBitInBitArray(resBitArrayPtr, 
                              0, 
                              FM10000_TRIGGER_RES_FREE,
                              &bitNo);
    if (err == FM_OK && bitNo == -1 )
    {
        err = FM_ERR_NO_FREE_TRIG_RES;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmSetBitArrayBit(resBitArrayPtr, bitNo, FM10000_TRIGGER_RES_USED);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmSetBitArrayBit(isIntBitArrayPtr, bitNo, isInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    *value = bitNo;

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000AllocateTriggerResource */




/*****************************************************************************/
/** fm10000FreeTriggerResource
 * \ingroup triggerInt
 *
 * \desc            Free a trigger resource. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to free.
 * 
 * \param[in]       value is the the value of the resource type to free.
 * 
 * \param[in]       isInternal should be set to true for an internal trigger
 *                  resource to be freeable.
 *                  
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type
 *                  does is not valid or value is out of range.
 * \return          FM_ERR_INTERNAL_RESOURCE if the resource is internal (
 *                  internal resources can only be deleted by API internals).
 *
 *****************************************************************************/
fm_status fm10000FreeTriggerResource(fm_int sw, 
                                     fm_triggerResourceType res, 
                                     fm_uint32 value,
                                     fm_bool isInternal)
{
    fm_status            err = FM_OK;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_bitArray *        resBitArrayPtr;
    fm_bitArray *        isIntBitArrayPtr;
    fm_bool              resIsInternal;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, value = %d\n",
                 sw,
                 res, 
                 value);

    TAKE_TRIGGER_LOCK(sw);

    switchExt   = GET_SWITCH_EXT(sw);
    trigInfo    = &switchExt->triggerInfo;

    switch (res)
    {
        case FM_TRIGGER_RES_MAC_ADDR:
            resBitArrayPtr   = &trigInfo->usedMacTrigID;
            isIntBitArrayPtr = &trigInfo->macTrigIdInternal;

            if ( (value == 0) || 
                 (value == 63) || 
                 (value >= FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        case FM_TRIGGER_RES_VLAN:
            resBitArrayPtr   = &trigInfo->usedVlanTrigID;
            isIntBitArrayPtr = &trigInfo->vlanTrigIdInternal;

            if ( (value == 0) || 
                 (value >= FM10000_TRIGGER_VLAN_TRIG_ID_MAX) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        case FM_TRIGGER_RES_FFU:
            resBitArrayPtr   = &trigInfo->usedFFUTrigIDBits;
            isIntBitArrayPtr = &trigInfo->FFUTrigIdInternal;

            if (value >= FM10000_TRIGGER_FFU_TRIG_ID_BITS)
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        case FM_TRIGGER_RES_RATE_LIMITER:
            resBitArrayPtr   = &trigInfo->usedRateLimiterID;
            isIntBitArrayPtr = &trigInfo->rateLimiterIdInternal;

            if (value >= FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES)
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        case FM_TRIGGER_RES_MIRROR_PROFILE:
            err = fm10000DeleteMirrorProfile(sw, value, isInternal);
            goto ABORT;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;

    }   /* end switch (res) */

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    /* Check if this resource is internal and if the caller is allowed
     * to free it */
    err = fmGetBitArrayBit(isIntBitArrayPtr, value, &resIsInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (resIsInternal == TRUE &&
        resIsInternal != isInternal)
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Mark the specified ID as unused (no error returned if already
     * unused) */
    err = fmSetBitArrayBit(resBitArrayPtr, 
                           value, 
                           FM10000_TRIGGER_RES_FREE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000FreeTriggerResource */




/*****************************************************************************/
/** fm10000GetTriggerResourceFirst
 * \ingroup triggerInt
 *
 * \desc            Find the first resource allocated for a given type.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to get.
 * 
 * \param[out]      value is a pointer to the caller allocated storage
 *                  where the first trigger resource of type res should be
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there is no trigger resource.
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type is invalid
 *                  or value pointer is NULL. 
 *
 *****************************************************************************/
fm_status fm10000GetTriggerResourceFirst(fm_int sw, 
                                         fm_triggerResourceType res, 
                                         fm_uint32 *value)
{
    fm_status            err = FM_OK;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_bitArray *        resBitArrayPtr;
    fm_int               startBit;
    fm_int               endBit;
    fm_int               bitNo;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, value = %p\n",
                 sw,
                 res, 
                 (void *) value );

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    switch (res)
    {
        case FM_TRIGGER_RES_MAC_ADDR:
            resBitArrayPtr = &trigInfo->usedMacTrigID;
            /* IDs 0 and 63 are reserved */
            startBit = 1;
            endBit = 62;
            break;

        case FM_TRIGGER_RES_VLAN:
            resBitArrayPtr = &trigInfo->usedVlanTrigID;
            /* IDs 0 is reserved */
            startBit = 1;
            endBit = FM10000_TRIGGER_VLAN_TRIG_ID_MAX - 1;
            break;

        case FM_TRIGGER_RES_FFU:
            resBitArrayPtr = &trigInfo->usedFFUTrigIDBits;
            /* IDs 0 is reserved */
            startBit = 0;
            endBit = FM10000_TRIGGER_FFU_TRIG_ID_BITS - 1;
            break;

        case FM_TRIGGER_RES_RATE_LIMITER:
            resBitArrayPtr = &trigInfo->usedRateLimiterID;
            startBit = 0;
            endBit = FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES - 1;
            break;

        case FM_TRIGGER_RES_MIRROR_PROFILE:
            resBitArrayPtr = &trigInfo->usedProfileHandle;
            startBit = 0;
            endBit = FM10000_NUM_MIRROR_PROFILES - 1;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Find the first used bit in the bit array */
    err = fmFindBitInBitArray(resBitArrayPtr, 
                              startBit, 
                              FM10000_TRIGGER_RES_USED,
                              &bitNo);
    if ( err == FM_OK && 
         ( (bitNo == -1) || (bitNo > endBit) ) )
    {
        err = FM_ERR_NO_MORE;
        goto ABORT;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    *value = bitNo;

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerResourceFirst */




/*****************************************************************************/
/** fm10000GetTriggerResourceNext
 * \ingroup triggerInt
 *
 * \desc            Find the next resource allocated for a given type.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to get.
 * 
 * \param[in]       curValue is the current resource value of type res.
 * 
 * \param[out]      nextValue is a pointer to the caller allocated storage
 *                  where the next trigger resource of type res should be
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there is no trigger resource.
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type is invalid, or
 *                  if curValue does not exit, or if nextValue pointer is NULL. 
 *
 *****************************************************************************/
fm_status fm10000GetTriggerResourceNext(fm_int sw, 
                                        fm_triggerResourceType res, 
                                        fm_uint32 curValue,
                                        fm_uint32 *nextValue)
{
    fm_status            err = FM_OK;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_bitArray *        resBitArrayPtr;
    fm_int               startBit;
    fm_int               endBit;
    fm_int               bitNo;
    fm_bool              bitState;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, curValue = %d, nextValue = %p\n",
                 sw,
                 res,
                 curValue, 
                 (void *) nextValue );

    TAKE_TRIGGER_LOCK(sw);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    switch (res)
    {
        case FM_TRIGGER_RES_MAC_ADDR:
            resBitArrayPtr = &trigInfo->usedMacTrigID;
            /* IDs 0 and 63 are reserved */
            startBit = 1;
            endBit = 62;
            break;

        case FM_TRIGGER_RES_VLAN:
            resBitArrayPtr = &trigInfo->usedVlanTrigID;
            /* ID 0 is reserved */
            startBit = 1;
            endBit = FM10000_TRIGGER_VLAN_TRIG_ID_MAX - 1;
            break;

        case FM_TRIGGER_RES_FFU:
            resBitArrayPtr = &trigInfo->usedFFUTrigIDBits;
            startBit = 0;
            endBit = FM10000_TRIGGER_FFU_TRIG_ID_BITS - 1;
            break;

        case FM_TRIGGER_RES_RATE_LIMITER:
            resBitArrayPtr = &trigInfo->usedRateLimiterID;
            startBit = 0;
            endBit = FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES - 1;
            break;

        case FM_TRIGGER_RES_MIRROR_PROFILE:
            resBitArrayPtr = &trigInfo->usedProfileHandle;
            startBit = 0;
            endBit = FM10000_NUM_MIRROR_PROFILES - 1;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Validate that curValue is a valid allocated value */
    err = fmGetBitArrayBit(resBitArrayPtr, curValue, &bitState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (bitState != FM10000_TRIGGER_RES_USED)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    if (curValue >= (fm_uint32)endBit)
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Find the next used bit in the bit array */
    err = fmFindBitInBitArray(resBitArrayPtr, 
                              curValue + 1, 
                              FM10000_TRIGGER_RES_USED, &bitNo);
    if ( err == FM_OK && 
         ( (bitNo == -1) || (bitNo > endBit) ) )
    {
        err = FM_ERR_NO_MORE;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    *nextValue = bitNo;

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerResourceNext */




/*****************************************************************************/
/** fm10000SetTriggerRateLimiter
 * \ingroup triggerInt
 *
 * \desc            Set a rate limiter's configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rateLimiterId is the rate limiter ID (handle) to set.
 * 
 * \param[in]       cfg is the configuration to apply to the rate limiter.
 * 
 * \param[in]       isInternal should be set to true for an internal
 *                  rate limiter to be modifiable.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if this rate limiter handle does not exist.
 *
 *****************************************************************************/
fm_status fm10000SetTriggerRateLimiter(fm_int sw,
                                       fm_int rateLimiterId,
                                       fm_rateLimiterCfg *cfg,
                                       fm_bool isInternal)
{
    fm_status            err = FM_OK;
    fm_switch *          switchPtr;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_bool              bitState;
    fm_uint32            regVal;
    fm_uint32            mantissa;
    fm_uint32            exp;
    fm_uint64            dropMask;
    fm_bool              resIsInternal;
    fm_portmask          portmask;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, rateLimiterId = %d, capacity = %d, rate = %d\n",
                 sw,
                 rateLimiterId, 
                 cfg->capacity,
                 cfg->rate);

    TAKE_TRIGGER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    /* First validate that the rateLimiterID has been allocated */
    err = fmGetBitArrayBit(&trigInfo->usedRateLimiterID, 
                           rateLimiterId, 
                           &bitState);

    if (bitState != FM10000_TRIGGER_RES_USED)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Check if this resource is internal and if the caller is allowed
     * to free it */
    err = fmGetBitArrayBit(&trigInfo->rateLimiterIdInternal, 
                           rateLimiterId, 
                           &resIsInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    if (resIsInternal == TRUE &&
        resIsInternal != isInternal)
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    /* Validate that capacity is within the acceptable range. */
    if (cfg->capacity >= FM10000_TRIGGER_RL_CAPACITY_MAX)
    {
        err = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    err = fmPortSetToPortMask(sw, cfg->dropPortset, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = fmPortMaskLogicalToPhysical(switchPtr, &portmask, &portmask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    dropMask = ( (fm_uint64)(portmask.maskWord[0]) ) | 
               ( (fm_uint64)(portmask.maskWord[1]) << 32);

    /* Cache a copy of the portset number used as dropmask */
    trigInfo->rateLimPortSet[rateLimiterId] = cfg->dropPortset;

    err = RateToMantissaAndExponent(sw,
                                    cfg->rate, 
                                    &mantissa, 
                                    &exp);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    regVal = 0;

    FM_SET_FIELD(regVal, 
                 FM10000_TRIGGER_RATE_LIM_CFG_1, 
                 Capacity, 
                 cfg->capacity);

    FM_SET_FIELD(regVal, 
                 FM10000_TRIGGER_RATE_LIM_CFG_1, 
                 RateMantissa, 
                 mantissa);

    FM_SET_FIELD(regVal, 
                 FM10000_TRIGGER_RATE_LIM_CFG_1, 
                 RateExponent, 
                 exp);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_TRIGGER_RATE_LIM_CFG_1(rateLimiterId),
                                 regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    err = switchPtr->WriteUINT64(sw, 
                                 FM10000_TRIGGER_RATE_LIM_CFG_2(rateLimiterId, 0),
                                 dropMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    
ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000SetTriggerRateLimiter */




/*****************************************************************************/
/** fm10000GetTriggerRateLimiter
 * \ingroup triggerInt
 *
 * \desc            Get a rate limiter's configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rateLimiterId is the rate limiter ID (handle) to set.
 * 
 * \param[out]      cfg is a pointer to the caller-allocated storage where
 *                  the configuration of the rate limiter should be stored. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if this rate limiter handle does not exist.
 *
 *****************************************************************************/
fm_status fm10000GetTriggerRateLimiter(fm_int sw,
                                       fm_int rateLimiterId,
                                       fm_rateLimiterCfg *cfg)
{
    fm_status            err = FM_OK;
    fm_switch *          switchPtr;
    fm10000_switch *     switchExt;
    fm10000_triggerInfo *trigInfo;
    fm_uint32            regVal;
    fm_uint32            mantissa;
    fm_uint32            exp;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, rateLimiterId = %d, cfg = %p\n",
                 sw,
                 rateLimiterId,
                 (void *) cfg);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    TAKE_TRIGGER_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_RATE_LIM_CFG_1(rateLimiterId),
                                &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    cfg->capacity = FM_GET_FIELD(regVal, 
                                 FM10000_TRIGGER_RATE_LIM_CFG_1, 
                                 Capacity);

    mantissa = FM_GET_FIELD(regVal, 
                            FM10000_TRIGGER_RATE_LIM_CFG_1, 
                            RateMantissa);

    exp = FM_GET_FIELD(regVal, 
                       FM10000_TRIGGER_RATE_LIM_CFG_1, 
                       RateExponent);

    err = MantissaAndExponentToRate(sw, 
                                    mantissa, 
                                    exp, 
                                    &cfg->rate);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    cfg->dropPortset = trigInfo->rateLimPortSet[rateLimiterId];

    switchPtr->ReadUINT32(sw, 
                           FM10000_TRIGGER_RATE_LIM_USAGE(rateLimiterId),
                           &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

    cfg->usage = RATE_LIM_USAGE_TO_BYTES * (fm_int)regVal;

ABORT:
    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerRateLimiter */




/*****************************************************************************/
/** fm10000SetTriggerAttribute
 * \ingroup triggerInt
 * 
 * \desc            Set a trigger attribute.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group of the trigger. 
 * 
 * \param[in]       rule is the rule of the trigger. 
 * 
 * \param[in]       attr is the trigger (See ''Trigger Attributes'')
 *                  attribute to set. 
 * 
 * \param[in]       value points to the attribute value to set.
 * 
 * \param[in]       isInternal should be set to true for an internal trigger
 *                  attribute to be modifiable.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the attribute does not exist.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is internal (
 *                  internal triggers can only be modified by API internals).
 *
 *****************************************************************************/
fm_status fm10000SetTriggerAttribute(fm_int sw, 
                                     fm_int group, 
                                     fm_int rule,
                                     fm_int attr, 
                                     void *value,
                                     fm_bool isInternal)
{
    fm_status             err = FM_OK;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, "
                 "attr = %d, value = %p\n",
                 sw,
                 group, 
                 rule,
                 attr,
                 (void *) value);

    TAKE_TRIGGER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    if ( (trigEntry->isInternal == TRUE) && 
         (trigEntry->isInternal != isInternal) )
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switch (attr)
    {
        case FM_TRIGGER_ATTR_COUNTER:

            /* Reset the cached counter */
            trigEntry->counterCache = 0;

            /* Write counter value to HW */
            switchPtr->WriteUINT64(sw, 
                                   FM10000_TRIGGER_STATS(trigEntry->index, 0),
                                   *((fm_uint64*)(value)));
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000SetTriggerAttribute */




/*****************************************************************************/
/** fm10000GetTriggerAttribute
 * \ingroup triggerInt
 * 
 * \desc            Get a trigger attribute.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group of the trigger. 
 * 
 * \param[in]       rule is the rule of the trigger. 
 * 
 * \param[in]       attr is the trigger (See ''Trigger Attributes'')
 *                  attribute to get. 
 * 
 * \param[in]       value is a pointer to the caller allocated storage where
 *                  the value of the attribute should be stored. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the attribute does not exist. 
 *
 *****************************************************************************/
fm_status fm10000GetTriggerAttribute(fm_int sw, 
                                     fm_int group, 
                                     fm_int rule,
                                     fm_int attr, 
                                     void * value)
{
    fm_status             err = FM_OK;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm10000_triggerEntry *trigEntry;
    fm_uint64             cnt;
    
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, "
                 "attr = %d, value = %p\n",
                 sw,
                 group, 
                 rule,
                 attr,
                 (void *) value);

    TAKE_TRIGGER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    if (fmTreeFind(&trigInfo->triggerTree, 
                   FM10000_TRIGGER_GROUP_RULE_TO_KEY(group, rule),
                   (void**) &trigEntry) != FM_OK)
    {
        err = FM_ERR_INVALID_TRIG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switch (attr)
    {
        case FM_TRIGGER_ATTR_COUNTER:

            /* Read counter value from HW */
            switchPtr->ReadUINT64(sw, 
                                  FM10000_TRIGGER_STATS(trigEntry->index, 0),
                                  &cnt);
                                  
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

            /* Add the cache in the case that the trigger has been moved */
            *((fm_uint64*)(value)) = cnt + trigEntry->counterCache;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

ABORT:
    DROP_TRIGGER_LOCK(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerAttribute */




/*****************************************************************************/
/** fm10000GetTriggerFromTrapCode
 * \ingroup triggerInt
 *
 * \desc            Returns the high-level identifier of the trigger with
 *                  the specified trap code.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trapCode is the TRAP code to be identified.
 * 
 * \param[out]      group points to the location to receive the group number
 *                  of the trigger.
 * 
 * \param[out]      rule points to the location to receiver the rule number
 *                  of the trigger. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetTriggerFromTrapCode(fm_int  sw,
                                        fm_int  trapCode,
                                        fm_int *group,
                                        fm_int *rule)
{
    fm_status               err = FM_OK;
    fm10000_switch *        switchExt;
    fm10000_triggerInfo *   trigInfo;
    fm_uint32               trigID;
    fm_treeIterator         triggerIt;
    fm_uint64               trigKey;
    fm10000_triggerEntry *  trigEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER,
                 "sw=%d trapCode=0x%02x\n",
                 sw,
                 trapCode);

    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    trigID = trapCode & 0xff;

    if (trigID >= FM10000_MAX_HW_TRIGGERS)
    {
        /* No need to look up trap code if it's out of range for a
         * trigger index. */
        FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_ERR_NOT_FOUND);
    }

    TAKE_TRIGGER_LOCK(sw);

    fmTreeIterInit(&triggerIt, &trigInfo->triggerTree);

    for ( ; ; )
    {
        err = fmTreeIterNext(&triggerIt, &trigKey, (void **) &trigEntry);
        if (err != FM_OK)
        {
            break;
        }

        if (trigEntry->index == trigID)
        {
            *group = FM10000_TRIGGER_KEY_TO_GROUP(trigKey);
            *rule  = FM10000_TRIGGER_KEY_TO_RULE(trigKey);
            break;
        }

    }   /* end for ( ; ; ) */

    DROP_TRIGGER_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fm10000GetTriggerFromTrapCode */




/*****************************************************************************/
/** fm10000DbgDumpTriggers
 * \ingroup intDiag
 *
 * \desc            Dumps switch trigger information.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpTriggers(fm_int sw)
{
    fm_status             err = FM_OK;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm10000_triggerInfo * trigInfo;
    fm_treeIterator       triggerIt;
    fm_uint64             nextKey;
    fm10000_triggerEntry *nextTrigEntry;
    fm_uint32             trig;
    fm_text               name;
    fm_int                group;
    fm_int                rule;
    fm_int                counterCache;
    fm_triggerCondition   cond;
    fm_triggerAction      action;
    fm_bool               matchByPrec;

    fm_uint64             physRxMask;
    fm_uint64             physTxMask;
    fm_uint64             physDestMask;
    fm_uint64             physDropMask;
    fm_uint64             logRxMask;
    fm_uint64             logTxMask;
    fm_uint64             logDestMask;
    fm_uint64             logDropMask;

    fm_uint64             triggerCount;
    fm_text               activeState;
    fm_int                i;
    fm_int                cnt;
    fm_uint32             regVal;
    fm_uint32             capacity;
    fm_uint32             mantissa;
    fm_uint32             exponent;
    fm_uint32             rate;
    fm_uint32             mirrorIndex;
    fm_uint64             physDropMaskRL;
    fm_uint64             logDropMaskRL;
    fm_int                dropMaskRLPortSet;

    char                  pSetRxMask[12];
    char                  pSetTxMask[12];
    char                  pSetDestMask[12];
    char                  pSetDropMask[12];

    char                  textOutput[5000];
    char                  tempText[200];
    char                  invalidTriggerList[300];
    fm_int                invalidTriggerCnt;
    char *                curAction;

    TAKE_TRIGGER_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    trigInfo  = &switchExt->triggerInfo;

    printf("\n");
    printf("Note: Inactive triggers have been allocated but have \n"
           "      an invalid condition which prevents them from hitting\n");

    invalidTriggerCnt     = 0;
    invalidTriggerList[0] = 0;

    for (trig = 0 ; trig < FM10000_MAX_HW_TRIGGERS ; trig++)
    {
        name  = NULL;
        group = -1;
        rule  = -1;
        counterCache = 0;
        FM_SPRINTF_S(pSetRxMask, sizeof(pSetRxMask), "%s", "?");
        FM_SPRINTF_S(pSetTxMask, sizeof(pSetTxMask), "%s", "?");
        FM_SPRINTF_S(pSetDestMask, sizeof(pSetDestMask), "%s", "?");
        FM_SPRINTF_S(pSetDropMask, sizeof(pSetDropMask), "%s", "?");
        
        /* Get condition/action values for this trigger */
        err = fm10000ReadTriggerCondition(sw, 
                                          trig, 
                                          &cond, 
                                          &matchByPrec,
                                          &physRxMask, 
                                          &physTxMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = PhysToLogPortMask(sw, physRxMask, &logRxMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = PhysToLogPortMask(sw, physTxMask, &logTxMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = fm10000ReadTriggerAction(sw, 
                                       trig, 
                                       &action, 
                                       &physDestMask, 
                                       &physDropMask,
                                       &mirrorIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = PhysToLogPortMask(sw, physDestMask, &logDestMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = PhysToLogPortMask(sw, physDropMask, &logDropMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        /* Find the group / rule for this trigger */
        fmTreeIterInit(&triggerIt, &trigInfo->triggerTree); 

        while ( (err = fmTreeIterNext(&triggerIt, 
                                      &nextKey, 
                                      (void**) &nextTrigEntry)) == FM_OK)
        {
            if (nextTrigEntry->index == trig)
            {
                name =  nextTrigEntry->name;
                group = FM10000_TRIGGER_KEY_TO_GROUP(nextKey);
                rule  = FM10000_TRIGGER_KEY_TO_RULE(nextKey);
                counterCache = nextTrigEntry->counterCache;
                FM_SPRINTF_S(pSetRxMask, 
                             sizeof(pSetRxMask), 
                             "%d",
                             nextTrigEntry->cond->cfg.rxPortset);
                FM_SPRINTF_S(pSetTxMask, 
                             sizeof(pSetTxMask), 
                             "%d",
                             nextTrigEntry->cond->cfg.txPortset);
                FM_SPRINTF_S(pSetDestMask, 
                             sizeof(pSetDestMask), 
                             "%d",
                             nextTrigEntry->action->param.newDestPortset);
                FM_SPRINTF_S(pSetDropMask, 
                             sizeof(pSetDropMask), 
                             "%d",
                             nextTrigEntry->action->param.dropPortset);
                break;
            }
        }

        /* End of iteration */
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }

        /* Build action text */
        textOutput[0] = 0;
        tempText[0]   = 0;

        switch (action.cfg.forwardingAction)
        {
            case FM_TRIGGER_FORWARDING_ACTION_ASIS:
                curAction = NULL;
                break;
            case FM_TRIGGER_FORWARDING_ACTION_FORWARD:
                curAction = "  Forward:\n";
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "    New Destination Glort = 0x%04X, Mask = 0x%04X\n",
                             action.param.newDestGlort,
                             action.param.newDestGlortMask);
                break;
            case FM_TRIGGER_FORWARDING_ACTION_REDIRECT:
                curAction = "  Redirect:\n";
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "    New Destination Glort = 0x%04X, Mask = 0x%04X\n"
                             "    New Port Dest Mask (log/phys/portset) = "
                             "0x%012" FM_FORMAT_64 "X / "  
                             "0x%012" FM_FORMAT_64 "X / "  
                             "%s\n"
                             "    Filter Dest Mask = %s\n",
                             action.param.newDestGlort,
                             action.param.newDestGlortMask,
                             logDestMask,
                             physDestMask,
                             pSetDestMask,
                             action.param.filterDestMask == 1 ? "TRUE" : "FALSE");
                break;
            case FM_TRIGGER_FORWARDING_ACTION_DROP:
                curAction = "  Drop:\n";
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "    Drop Mask (log/phys/portset) = "
                             "0x%012" FM_FORMAT_64 "X / "
                             "0x%012" FM_FORMAT_64 "X / "
                             "%s\n",
                             logDropMask,
                             physDropMask,
                             pSetDropMask);
                break;
            default:
                curAction = "  Invalid Forwarding Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);
        APPEND_TEXT(textOutput, tempText);

        switch (action.cfg.trapAction)
        {
            case FM_TRIGGER_TRAP_ACTION_ASIS:
                curAction = NULL;
                break;
            case FM_TRIGGER_TRAP_ACTION_TRAP:
                curAction = "  Trap\n";
                break;
            case FM_TRIGGER_TRAP_ACTION_LOG:
                curAction = "  Log\n";
                break;
            case FM_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG:
                curAction = "  No Trap/Log\n";
                break;
            default:
                curAction = "  Invalid Trapping Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);

        tempText[0] = 0;
        switch (action.cfg.mirrorAction)
        {
            case FM_TRIGGER_MIRROR_ACTION_NONE:
                curAction = NULL;
                break;
            case FM_TRIGGER_MIRROR_ACTION_MIRROR:
                curAction = "  Mirror:\n";
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "    MirrorSelect  = %d\n" 
                             "    MirrorProfile = %d (%d)\n",
                             action.param.mirrorSelect,
                             action.param.mirrorProfile,
                             mirrorIndex);
                break;
            default:
                curAction = "  Invalid Mirroring Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);
        APPEND_TEXT(textOutput, tempText);

        tempText[0] = 0;
        switch (action.cfg.switchPriAction)
        {
            case FM_TRIGGER_SWPRI_ACTION_ASIS:
                curAction = NULL;
                break;
            case FM_TRIGGER_SWPRI_ACTION_REASSIGN:
                curAction = NULL;
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "  Reassign SwitchPriority to %d\n", 
                             action.param.newSwitchPri);
                break;
            default:
                curAction = "  Invalid Switch Priority Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);
        APPEND_TEXT(textOutput, tempText);

        tempText[0] = 0;
        switch (action.cfg.vlan1Action)
        {
            case FM_TRIGGER_VLAN_ACTION_ASIS:
                curAction = NULL;
                break;
            case FM_TRIGGER_VLAN_ACTION_REASSIGN:
                curAction = NULL;
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "  Reassign Vlan1 to %d\n",
                             action.param.newVlan1);
                break;
            default:
                curAction = "  Invalid Vlan Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);
        APPEND_TEXT(textOutput, tempText);

        tempText[0] = 0;
        switch (action.cfg.learningAction)
        {
            case FM_TRIGGER_LEARN_ACTION_ASIS:
                curAction = NULL;
                break;
            case FM_TRIGGER_LEARN_ACTION_CANCEL:
                curAction = "  Cancel Learning (No TCN Update)\n";
                break;
            default:
                curAction = "  Invalid Learning Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);

        tempText[0] = 0;
        switch (action.cfg.rateLimitAction)
        {
            case FM_TRIGGER_RATELIMIT_ACTION_ASIS:
                curAction = NULL;
                break;
            case FM_TRIGGER_RATELIMIT_ACTION_RATELIMIT:
                curAction = NULL;
                FM_SPRINTF_S(tempText, sizeof(tempText), 
                             "  Apply Rate Limiter %d\n",
                             action.param.rateLimitNum);
                break;
            default:
                curAction = "  Invalid Vlan Action\n";
                break;
        }

        APPEND_TEXT(textOutput, curAction);
        APPEND_TEXT(textOutput, tempText);
        
        if (physRxMask != 0)
        {
            activeState = "Active";
        }
        else
        {
            activeState = "Inactive";
        }

        if (textOutput[0] == 0 && 
            physRxMask == 0 &&
            group == -1)
        {
            /* Display 12 invalid triggers per line */
            if ((invalidTriggerCnt % 12) == 0)
            {
                /* Trigger is invalid, add it to invalid list */
                FM_SPRINTF_S(tempText, sizeof(tempText), "\n# %02d, ", trig);
                APPEND_TEXT(invalidTriggerList, tempText);
            }
            else
            {
                /* Trigger is invalid, add it to invalid list */
                FM_SPRINTF_S(tempText, sizeof(tempText), "%02d, ", trig);
                APPEND_TEXT(invalidTriggerList, tempText);
            }

            invalidTriggerCnt++;
        }
        else
        {
            FM_LOG_PRINT("\n");
            FM_LOG_PRINT("##################################################\n");
            if (name != NULL)
            {
                FM_LOG_PRINT("# Name:       %s\n", name);
            }
            FM_LOG_PRINT("# Index:      %d\n", trig);
            FM_LOG_PRINT("# Group/Rule: %d/%d\n", group, rule);
            FM_LOG_PRINT("# State:      %s\n", activeState);
            FM_LOG_PRINT("##################################################\n");
            FM_LOG_PRINT("\n");

            if (textOutput[0] == 0)
            {
                FM_SPRINTF_S(textOutput, sizeof(textOutput), "%s\n", "  None");
            }

            FM_LOG_PRINT("Conditions:\n");

            FM_LOG_PRINT("  Source Port Mask (log/phys/portset) = "
                         "0x%012" FM_FORMAT_64 "X / " 
                         "0x%012" FM_FORMAT_64 "X / " 
                         "%s\n"

                         "  Dest Port Mask (log/phys/portset)   = "
                         "0x%012" FM_FORMAT_64 "X / "
                         "0x%012" FM_FORMAT_64 "X / "
                         "%s\n",
                         logRxMask,
                         physRxMask,
                         pSetRxMask,
                         logTxMask, 
                         physTxMask, 
                         pSetTxMask);

        switch (cond.cfg.matchTx)
        {
            case FM_TRIGGER_TX_MASK_DOESNT_CONTAIN:
                /* If the trigger tx mask is not in always match condition */
                if (physTxMask != 0)
                {
                    FM_LOG_PRINT("  If DMASK doesn't contain ports in "
                                 "portset %s\n",
                                 pSetTxMask);
                }
                break;

            case FM_TRIGGER_TX_MASK_CONTAINS:
                FM_LOG_PRINT("  If DMASK contains one or more ports in "
                             "portset %s\n",
                             pSetTxMask);
                break;

            case FM_TRIGGER_TX_MASK_EQUALS:
                FM_LOG_PRINT("  If DMASK equals all ports in "
                             "portset %s\n",
                             pSetTxMask);
                break;

            case FM_TRIGGER_TX_MASK_DOESNT_EQUAL:
                FM_LOG_PRINT("  If DMASK doesn't equal all ports in "
                             "portset %s\n",
                             pSetTxMask);
                break;

            default:
                FM_LOG_PRINT("  MatchTx field invalid (0x%x)\n", cond.cfg.matchTx);
                break;

        }

        switch (cond.cfg.matchSA)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Source MAC Trig ID NOT EQUAL to %u\n",
                             cond.param.saId);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Source MAC Trig ID IS EQUAL to %u\n",
                             cond.param.saId);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH SA field invalid (0x%x)\n", cond.cfg.matchSA);
                break;
        }

        switch (cond.cfg.matchDA)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Dest MAC Trig ID NOT EQUAL to %u\n",
                             cond.param.daId);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Dest MAC Trig ID IS EQUAL to %u\n",
                             cond.param.daId);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH DA field invalid (0x%x)\n", cond.cfg.matchDA);
                break;
        }

        switch (cond.cfg.matchHitSA)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Source MAC NOT found in MAC Table\n");
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Source MAC found in MAC Table\n");
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH Hit SA field invalid (0x%x)\n",
                             cond.cfg.matchHitSA);
                break;
        }

        switch (cond.cfg.matchHitDA)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Dest MAC NOT found in MAC Table\n");
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Dest MAC found in MAC Table\n");
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH Hit DA field invalid (0x%x)\n",
                             cond.cfg.matchHitDA);
                break;
        }

        switch (cond.cfg.matchHitSADA)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Source or Dest MAC NOT found in MAC Table\n");
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Source or Dest MAC found in MAC Table\n");
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH Hit SADA field invalid (0x%x)\n",
                             cond.cfg.matchHitSADA);
                break;
        }

        switch (cond.cfg.matchVlan)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If VLAN Trig ID NOT EQUAL to %u\n", 
                             cond.param.vidId);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If VLAN Trig ID IS EQUAL to  %u\n", 
                             cond.param.vidId);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH VLAN field invalid (0x%x)\n", 
                             cond.cfg.matchVlan);
                break;
        }

        switch (cond.cfg.matchFFU)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If FFU Trig ID ANDed with 0x%02X "
                             "NOT EQUAL to 0x%02X\n",
                             cond.param.ffuIdMask,
                             cond.param.ffuId & cond.param.ffuIdMask);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If FFU Trig ID ANDed with 0x%02X "
                             "IS EQUAL to 0x%02X\n",
                             cond.param.ffuIdMask,
                             cond.param.ffuId & cond.param.ffuIdMask);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH FFU field invalid (0x%x)\n", 
                             cond.cfg.matchFFU);
                break;
        }

        switch (cond.cfg.matchSwitchPri)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Switch Pri NOT EQUAL to %u\n",
                             cond.param.switchPri);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Switch Pri IS EQUAL to  %u\n",
                             cond.param.switchPri);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH SWPRI field invalid (0x%x)\n",
                             cond.cfg.matchSwitchPri);
                break;
        }

        switch (cond.cfg.matchEtherType)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Ether Type ANDed with 0x%04X "
                             "NOT EQUAL to 0x%04X\n",
                             cond.param.etherTypeMask,
                             cond.param.etherType & cond.param.etherTypeMask);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Ether Type ANDed with 0x%04X "
                             "IS EQUAL to 0x%04X\n",
                             cond.param.etherTypeMask,
                             cond.param.etherType & cond.param.etherTypeMask);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH ETHERTYPE field invalid (0x%x)\n",
                             cond.cfg.matchEtherType);
                break;
        }

        switch (cond.cfg.matchDestGlort)
        {
            case FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL:
                FM_LOG_PRINT("  If Dest Glort ANDed with 0x%04X "
                             "NOT EQUAL to 0x%04X\n",
                             cond.param.destGlortMask,
                             cond.param.destGlort & cond.param.destGlortMask);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHIFEQUAL:
                FM_LOG_PRINT("  If Dest Glort ANDed with 0x%04X "
                             "IS EQUAL to 0x%04X\n",
                             cond.param.destGlortMask,
                             cond.param.destGlort & cond.param.destGlortMask);
                break;
            case FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL:
                break;
            default:
                FM_LOG_PRINT("  MATCH DEST GLORT field invalid (0x%x)\n",
                             cond.cfg.matchDestGlort);
                break;
        }

        if (cond.cfg.matchFrameClassMask)
        {
            if (cond.cfg.matchFrameClassMask & FM_TRIGGER_FRAME_CLASS_UCAST)
            {
                FM_LOG_PRINT("  Match Unicast Frames\n");
            }
            if (cond.cfg.matchFrameClassMask & FM_TRIGGER_FRAME_CLASS_BCAST)
            {
                FM_LOG_PRINT("  Match Broadcast Frames\n");
            }
            if (cond.cfg.matchFrameClassMask & FM_TRIGGER_FRAME_CLASS_MCAST)
            {
                FM_LOG_PRINT("  Match Multicast Frames\n");
            }
        }

        if (cond.cfg.matchRoutedMask)
        {
            if (cond.cfg.matchRoutedMask & FM_TRIGGER_SWITCHED_FRAMES)
            {
                FM_LOG_PRINT("  Match Switched Frames\n");
            }
            if (cond.cfg.matchRoutedMask & FM_TRIGGER_ROUTED_FRAMES)
            {
                FM_LOG_PRINT("  Match Routed Frames\n");
            }
        }

        if (cond.cfg.matchFtypeMask)
        {
            FM_LOG_PRINT("  Match Frame Type == 0x%02x\n", cond.cfg.matchFtypeMask);
        }

        if (!matchByPrec)
        {
            FM_LOG_PRINT("  Begin new Precedence Evaluation (matchByPrecedence == 0)\n");
        }

        if (cond.cfg.matchRandomNumber)
        {
            FM_LOG_PRINT("  Match if random number <= %d (~%d%%)\n",
                         cond.param.randMatchThreshold,
                         (cond.param.randMatchThreshold * 100) / 
                         (MAX_RANDOM_VALUE - 1));
            FM_LOG_PRINT("    Use random generator %d\n",
                         cond.param.randGenerator);
        }

        if (cond.cfg.HAMask)
        {
            FM_LOG_PRINT("  Match Using Frame Handler Action Codes:\n");
            if (cond.cfg.HAMask & FM_TRIGGER_HA_FORWARD_SPECIAL)
            {
                FM_LOG_PRINT("    Specially Handled Frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_PARSE_ERROR)
            {
                FM_LOG_PRINT("    Drop due to header parse error or discard eligible\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_PARITY_ERROR)
            {
                FM_LOG_PRINT("    Parity error detected during lookup\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_RESERVED_MAC)
            {
                FM_LOG_PRINT("    Trap MAC control frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP)
            {
                FM_LOG_PRINT("    Trap remapped MAC reserved frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_RESERVED_MAC)
            {
                FM_LOG_PRINT("    Drop MAC reserved frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_TAG)
            {
                FM_LOG_PRINT("    Drop due to tagging rules violation\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_CONTROL)
            {
                FM_LOG_PRINT("    Drop MAC Control Ethertype frame (including pause)\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_INVALID_SMAC)
            {
                FM_LOG_PRINT("    Drop due to invalid SMAC\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_PORT_SV)
            {
                FM_LOG_PRINT("    Drop due to MAC security violation "
                             "(moved address)\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_CPU)
            {
                FM_LOG_PRINT("    Trap CPU MAC Address\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_VLAN_IV)
            {
                FM_LOG_PRINT("    Drop due to VLAN ingress membership "
                             "violation\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_STP_INL)
            {
                FM_LOG_PRINT("    Drop due to ingress STP check "
                             "(non-learning state)\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_STP_IL)
            {
                FM_LOG_PRINT("    Drop due to ingress STP check "
                             "(learning state)\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_FFU)
            {
                FM_LOG_PRINT("    Drop due to FFU action\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_FFU)
            {
                FM_LOG_PRINT("    Trap due to FFU action\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_ICMP_TTL)
            {
                FM_LOG_PRINT("    Trap due to TTL <= 1 for ICMP frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_IP_OPTION)
            {
                FM_LOG_PRINT("    Trap IP frames with options\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_MTU)
            {
                FM_LOG_PRINT("    Trap due to MTU violation\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_IGMP)
            {
                FM_LOG_PRINT("    Trap due to IGMP\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_TRAP_TTL)
            {
                FM_LOG_PRINT("    Trap due to TTL <= 1 for non-ICMP frames\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_TTL)
            {
                FM_LOG_PRINT("    Drop due to TTL <= 1\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_NULL_DEST)
            {
                FM_LOG_PRINT("    Drop due to NULL destination mask\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_VLAN_EV)
            {
                FM_LOG_PRINT("    Drop due to VLAN egress violation\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_DLF)
            {
                FM_LOG_PRINT("    Drop due to flood control of DLF frames\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_GLORT_CAM_MISS)
            {
                FM_LOG_PRINT("    Drop due to GLORT_CAM miss\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_POLICER)
            {
                FM_LOG_PRINT("    Drop Policer\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_STP_E)
            {
                FM_LOG_PRINT("    Drop due to egress STP check\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_DROP_LOOPBACK)
            {
                FM_LOG_PRINT("    Drop due to loopback suppress\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_FORWARD_DGLORT)
            {
                FM_LOG_PRINT("    Forward due FFU/ARP DGLORT\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_FORWARD_FLOOD)
            {
                FM_LOG_PRINT("    Flood due to destination MAC miss in MA_TABLE\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_SWITCH_RESERVED_MAC)
            {
                FM_LOG_PRINT("    Switch MAC reserved frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_FORWARD_FID)
            {
                FM_LOG_PRINT("    Forward normally\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_LOG_FFU_I)
            {
                FM_LOG_PRINT("    Log Ingress FFU\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_LOG_RESERVED_MAC)
            {
                FM_LOG_PRINT("    Log MAC reserved frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_LOG_ARP_REDIRECT)
            {
                FM_LOG_PRINT("    Log ARP redirect frame\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_LOG_IP_MCST_TTL)
            {
                FM_LOG_PRINT("    Log IP multicast frames to CPU because "
                             "TTL <= 1\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_LOG_MCST_ICMP_TTL)
            {
                FM_LOG_PRINT("    Log ICMP Mcast TTL <= 1\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_PARSE_TIMEOUT)
            {
                FM_LOG_PRINT("    Header Timeout\n");
            }
            if (cond.cfg.HAMask & FM_TRIGGER_HA_MIRROR_FFU)
            {
                FM_LOG_PRINT("    Mirror FFU\n");
            }
        }
            FM_LOG_PRINT("\n");
            FM_LOG_PRINT("Actions:\n");
            FM_LOG_PRINT("%s", textOutput);

            err = switchPtr->ReadUINT64(sw, 
                                        FM10000_TRIGGER_STATS(trig, 0), 
                                        &triggerCount);

            FM_LOG_PRINT("\n");
            FM_LOG_PRINT("Counter:\n");
            FM_LOG_PRINT("  This trigger has hit %" FM_FORMAT_64 "d times\n", 
                         triggerCount + counterCache);
        }
    }

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("##################################################\n");
    FM_LOG_PRINT("# Invalid Triggers:\n");
    FM_LOG_PRINT("# %s\n", invalidTriggerList);
    FM_LOG_PRINT("##################################################\n");
    FM_LOG_PRINT("\n");

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Resources in use:\n");
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");
    FM_LOG_PRINT("| Resource      | Total    | Free   | Allocated ID's                    |\n");
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");

    /****************************** 
     * Print MAC Trig IDs
     *****************************/
    cnt = 0;
    textOutput[0] = 0;
    for (i = 0; i < FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX; i++)
    {
        err = fmFindBitInBitArray(&trigInfo->usedMacTrigID, i, 1, &i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (i < 0)
        {
            break;
        }
        else if ( (i == 0)  || (i == 63) )
        {
            /* Reserved trig ID's */
        }
        else
        {
            FM_SPRINTF_S(tempText, sizeof(tempText), "%02d, ", i);
            APPEND_TEXT(textOutput, tempText);

            cnt++;

            if (cnt % 8 == 0)
            {
                FM_SPRINTF_S(tempText, 
                             sizeof(tempText), 
                             "\n%s", 
                             "|               |          |        | ");
                APPEND_TEXT(textOutput, tempText);
            }
        }
    }

    FM_LOG_PRINT("| MAC Trig IDs  | %02d       | %02d     | %s\n",
                 FM10000_TRIGGER_MAC_ADDR_TRIG_ID_TOTAL,
                 FM10000_TRIGGER_MAC_ADDR_TRIG_ID_TOTAL - cnt,
                 textOutput);
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");

    /****************************** 
     * Print VLAN Trig IDs
     *****************************/
    cnt = 0;
    textOutput[0] = 0;
    for (i = 0; i < FM10000_TRIGGER_VLAN_TRIG_ID_MAX; i++)
    {
        err = fmFindBitInBitArray(&trigInfo->usedVlanTrigID, i, 1, &i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (i < 0)
        {
            break;
        }
        else if (i == 0)
        {
            /* Reserved trig ID's */
        }
        else
        {
            FM_SPRINTF_S(tempText, sizeof(tempText), "%02d, ", i);
            APPEND_TEXT(textOutput, tempText);

            cnt++;

            if (cnt % 8 == 0)
            {
                FM_SPRINTF_S(tempText, 
                             sizeof(tempText), 
                             "\n%s", 
                             "|               |          |        | ");
                APPEND_TEXT(textOutput, tempText);
            }
        }
    }

    FM_LOG_PRINT("| Vlan Trig IDs | %02d       | %02d     | %s\n",
                 FM10000_TRIGGER_VLAN_TRIG_ID_TOTAL,
                 FM10000_TRIGGER_VLAN_TRIG_ID_TOTAL - cnt,
                 textOutput);
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");

    /****************************** 
     * Print FFU action bits
     *****************************/
    cnt = 0;
    textOutput[0] = 0;
    for (i = 0; i < FM10000_TRIGGER_FFU_TRIG_ID_BITS; i++)
    {
        err = fmFindBitInBitArray(&trigInfo->usedFFUTrigIDBits, i, 1, &i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (i < 0)
        {
            break;
        }
        else
        {
            FM_SPRINTF_S(tempText, sizeof(tempText), "%02d, ", i);
            APPEND_TEXT(textOutput, tempText);

            cnt++;

            if (cnt % 8 == 0)
            {
                FM_SPRINTF_S(tempText, 
                             sizeof(tempText), 
                             "\n%s", 
                             "|               |          |        | ");
                APPEND_TEXT(textOutput, tempText);
            }
        }
    }

    FM_LOG_PRINT("| FFU act. bits | %02d       | %02d     | %s\n",
                 FM10000_TRIGGER_FFU_TRIG_ID_BITS,
                 FM10000_TRIGGER_FFU_TRIG_ID_BITS - cnt,
                 textOutput);
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");

    /****************************** 
     * Print Rate limiter IDs
     *****************************/
    cnt = 0;
    textOutput[0] = 0;
    for (i = 0; i < FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES; i++)
    {
        err = fmFindBitInBitArray(&trigInfo->usedRateLimiterID, i, 1, &i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (i < 0)
        {
            break;
        }
        else
        {
            FM_SPRINTF_S(tempText, sizeof(tempText), "%02d, ", i);
            APPEND_TEXT(textOutput, tempText);

            cnt++;

            if (cnt % 8 == 0)
            {
                FM_SPRINTF_S(tempText, 
                             sizeof(tempText), 
                             "\n%s", 
                             "|               |          |        | ");
                APPEND_TEXT(textOutput, tempText);
            }
        }
    }

    FM_LOG_PRINT("| RateLim IDs   | %02d       | %02d     | %s\n",
                 FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES,
                 FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES - cnt,
                 textOutput);
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");

    /****************************** 
     * Print Mirror Profile IDs
     *****************************/
    cnt = 0;
    textOutput[0] = 0;
    for (i = 0; i < FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES; i++)
    {
        err = fmFindBitInBitArray(&trigInfo->usedProfileIndex, i, 1, &i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        if (i < 0)
        {
            break;
        }
        else
        {
            FM_SPRINTF_S(tempText, sizeof(tempText), "%02d, ", i);
            APPEND_TEXT(textOutput, tempText);

            cnt++;

            if (cnt % 8 == 0)
            {
                FM_SPRINTF_S(tempText, 
                             sizeof(tempText), 
                             "\n%s", 
                             "|               |          |        | ");
                APPEND_TEXT(textOutput, tempText);
            }
        }
    }

    FM_LOG_PRINT("| MirProfileIDs | %02d       | %02d     | %s\n",
                 FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES,
                 FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES - cnt,
                 textOutput);
    FM_LOG_PRINT("+---------------+----------+--------+-----------------------------------+\n");



    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Dumping rate limiters\n");
    FM_LOG_PRINT("+----+---------+----------+----------+--------------+---------------------------------------+\n");
    FM_LOG_PRINT("| ID | Cap(KB) | Mantissa | Exponent | Rate(kbps)   | DropMask(log/phys/portset)            |\n");
    FM_LOG_PRINT("+----+---------+----------+----------+--------------+---------------------------------------+\n");

    for (i = 0; i < FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES; i++)
    {
        err = switchPtr->ReadUINT32(sw, 
                                FM10000_TRIGGER_RATE_LIM_CFG_1(i),
                                &regVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        capacity = FM_GET_FIELD(regVal, 
                                FM10000_TRIGGER_RATE_LIM_CFG_1, 
                                Capacity);

        mantissa = FM_GET_FIELD(regVal, 
                                FM10000_TRIGGER_RATE_LIM_CFG_1, 
                                RateMantissa);

        exponent = FM_GET_FIELD(regVal, 
                                FM10000_TRIGGER_RATE_LIM_CFG_1, 
                                RateExponent);

        MantissaAndExponentToRate(sw, 
                                  mantissa, 
                                  exponent, 
                                  &rate);

        dropMaskRLPortSet = trigInfo->rateLimPortSet[i];

        switchPtr->ReadUINT64(sw, 
                               FM10000_TRIGGER_RATE_LIM_CFG_2(i, 0),
                               &physDropMaskRL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        err = PhysToLogPortMask(sw, physDropMaskRL, &logDropMaskRL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);

        FM_LOG_PRINT("| %-2d | %-7d | %-8d | %-8d | %-12d | "
                     "0x%012" FM_FORMAT_64 "X / "
                     "0x%012" FM_FORMAT_64 "X / "
                     "(%d)\n",
                     i,
                     capacity, 
                     mantissa, 
                     exponent, 
                     rate,
                     logDropMaskRL,
                     physDropMaskRL,
                     dropMaskRLPortSet);
    }

    FM_LOG_PRINT("+----+---------+----------+----------+--------------+---------------------------------------+\n");
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\n");

ABORT:
    DROP_TRIGGER_LOCK(sw);

    return err;

}   /* end fm10000DbgDumpTriggers */

