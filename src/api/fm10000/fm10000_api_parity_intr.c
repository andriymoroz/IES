/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_parity_intr.c
 * Creation Date:   August 22, 2014.
 * Description:     Parity interrupt handling routines.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation.
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
#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Whether to enable/disable CRM interrupts. */
#define ENABLE_CRM_INTERRUPTS           FALSE

/* Bit mask for all the parity error bits in CORE_INTERRUPT_MASK. */
#define FM10000_INT_CORE_PARITY_ERR     \
    (FM10000_INT_CORE_MODIFY |          \
     FM10000_INT_CORE_SCHEDULER |       \
     FM10000_INT_CORE_SRAM_ERR |        \
     FM10000_INT_CORE_INGRESS_ERR |     \
     FM10000_INT_CORE_EGRESS_ERR)


#define FM_LOG_COMBINE_ON_ERR(cat, errcode, retStatus)                  \
    {                                                                   \
        fm_status localError = (errcode);                               \
        if ( localError != FM_OK )                                      \
        {                                                               \
            FM_LOG_DEBUG((cat),                                         \
                         "Failure occurred, continuing: %s\n",          \
                         fmErrorMsg(localError));                       \
            FM_ERR_COMBINE(retStatus, localError);                      \
        }                                                               \
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


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000CrossbarInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage CROSSBAR memory error interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       coreInt is the masked value of the CORE_INTERRUPT_DETECT
 *                  register.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000CrossbarInterruptHandler(fm_switch * switchPtr,
                                                 fm_uint32   coreInt)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d coreInt=%08x\n",
                 sw,
                 coreInt);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    parityInfo->core_int = coreInt;
    parityInfo->parityState = FM10000_PARITY_STATE_DECODE;

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);

}   /* end fm10000CrossbarInterruptHandler */




/*****************************************************************************/
/** fm10000ModifyInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage MODIFY memory error interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   coreMask points to the mask of CORE_INTERRUPT_MASK bits
 *                  to be cleared by the caller.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000ModifyInterruptHandler(fm_switch * switchPtr,
                                               fm_uint32 * coreMask)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           ipVal;
    fm_uint64           imVal;
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    err = switchPtr->ReadUINT64(sw, FM10000_MOD_IP(0), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT64(sw, FM10000_MOD_IM(0), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_MOD_IM(0), imVal | ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    ipVal &= ~imVal;

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "MOD_IP=0x%012llx\n", ipVal);

    if (ipVal)
    {
        /* Acknowledge MODIFY interrupts. */
        err = switchPtr->WriteUINT64(sw, FM10000_MOD_IP(0), ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

        parityInfo->mod_ip |= ipVal;
        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;
    }

    /* Tell caller to unmask MODIFY interrupts. */
    FM_SET_BIT(*coreMask, FM10000_CORE_INTERRUPT_MASK, MODIFY, 1);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000ModifyInterruptHandler */




/*****************************************************************************/
/** fm10000SchedulerInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage SCHEDULER memory error interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   coreMask points to the mask of CORE_INTERRUPT_MASK bits
 *                  to be cleared by the caller.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000SchedulerInterruptHandler(fm_switch * switchPtr,
                                                  fm_uint32 * coreMask)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint32           ipVal;
    fm_uint32           imVal;
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_IP(), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_IM(), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_SCHED_IM(), ipVal | imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    ipVal &= ~imVal;

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "SCHED_IP=0x%08x\n", ipVal);

    if (ipVal)
    {
        /* Acknowledge SCHEDULER interrupts. */
        err = switchPtr->WriteUINT32(sw, FM10000_SCHED_IP(), ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

        parityInfo->sched_ip |= ipVal;
        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;

    }   /* end if (ipVal) */

    /* Tell caller to unmask SCHEDULER interrupts. */
    FM_SET_BIT(*coreMask, FM10000_CORE_INTERRUPT_MASK, SCHEDULER, 1);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000SchedulerInterruptHandler */




/*****************************************************************************/
/** fm10000SramErrInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage SRAM memory error interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   coreMask points to the mask of CORE_INTERRUPT_MASK bits
 *                  to be cleared by the caller.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fm10000SramErrInterruptHandler(fm_switch * switchPtr,
                                                fm_uint32 * coreMask)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           ipVal;
    fm_uint64           imVal;
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    /* Fetch the bit mask indicating which SRAMS reported errors. */
    err = switchPtr->ReadUINT64(sw, FM10000_SRAM_ERR_IP(0), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Fetch the bit mask indicating which of the SRAMS are configured
     * to generate interrupts. */
    err = switchPtr->ReadUINT64(sw, FM10000_SRAM_ERR_IM(0), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Get the set of SRAMs to process. */
    ipVal &= ~imVal;

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "SRAM_ERR_IP=0x%012llx\n", ipVal);

    if (ipVal)
    {
        /* Acknowledge the interrupts we've accepted. */
        err = switchPtr->WriteUINT64(sw, FM10000_SRAM_ERR_IP(0), ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

        parityInfo->sram_ip |= ipVal;
        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;

    }   /* end if (ipVal) */

    /* Tell caller to unmask SRAM_ERR interrupts. */
    FM_SET_BIT(*coreMask, FM10000_CORE_INTERRUPT_MASK, SRAM_ERR, 1);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000SramErrInterruptHandler */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000CoreInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage CORE memory interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CoreInterruptHandler(fm_switch * switchPtr)
{
    fm_uint32   coreInt;
    fm_uint32   coreMask;
    fm_int      sw;
    fm_status   err;
    fm_status   retStatus;

    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    retStatus = FM_OK;

    /**************************************************
     * Read and acknowledge pending interrupts.
     **************************************************/

    err = switchPtr->ReadUINT32(sw,
                                FM10000_CORE_INTERRUPT_DETECT(),
                                &coreInt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT32(sw,
                                FM10000_CORE_INTERRUPT_MASK(),
                                &coreMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Mask the interrupts we've accepted. */
    err = switchPtr->WriteUINT32(sw,
                                 FM10000_CORE_INTERRUPT_MASK(),
                                 coreMask | coreInt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Interrupts to process. */
    coreInt &= ~coreMask;

    /* Interrupts to unmask. */
    coreMask = 0;

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "coreInt=0x%08x\n", coreInt);

    /**************************************************
     * Perform first-level interrupt processing.
     **************************************************/

    if ( coreInt & FM10000_INT_CORE_CROSSBAR_ERR )
    {
        err = fm10000CrossbarInterruptHandler(switchPtr, coreInt);
        FM_ERR_COMBINE(retStatus, err);
    }

    if ( FM_GET_BIT(coreInt, FM10000_CORE_INTERRUPT_DETECT, MODIFY))
    {
        err = fm10000ModifyInterruptHandler(switchPtr, &coreMask);
        FM_ERR_COMBINE(retStatus, err);
    }

    if ( FM_GET_BIT(coreInt, FM10000_CORE_INTERRUPT_DETECT, SCHEDULER) )
    {
        err = fm10000SchedulerInterruptHandler(switchPtr, &coreMask);
        FM_ERR_COMBINE(retStatus, err);
    }

    if ( FM_GET_BIT(coreInt, FM10000_CORE_INTERRUPT_DETECT, SRAM_ERR) )
    {
        err = fm10000SramErrInterruptHandler(switchPtr, &coreMask);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (coreMask)
    {
        /* Unmask specified CORE interrupts. */
        err = switchPtr->MaskUINT32(sw,
                                    FM10000_CORE_INTERRUPT_MASK(),
                                    coreMask,
                                    FALSE);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000CoreInterruptHandler */




/*****************************************************************************/
/** fm10000CrmInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage CRM interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CrmInterruptHandler(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint32           ipVal[FM10000_CRM_IP_WIDTH];
    fm_uint32           imVal[FM10000_CRM_IP_WIDTH];
    fm_uint32           maskVal[FM10000_CRM_IP_WIDTH];
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;
    fm_int              i;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IP(0),
                                    FM10000_CRM_IP_WIDTH,
                                    ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    for (i = 0 ; i < FM10000_CRM_IP_WIDTH ; i++)
    {
        maskVal[i] = imVal[i] | ipVal[i];
        ipVal[i] &= ~imVal[i];
    }

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     maskVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "CRM_IP=%08x %08x %08x\n",
                 ipVal[2],
                 ipVal[1],
                 ipVal[0]);

    if (ipVal[0] || ipVal[1] || ipVal[2])
    {
        /* Acknowledge CRM interrupts. Note that in the case of CRM checksum
         * mismatches, this will have no discernable effect, since the CRM will
         * continue to reassert the condition until the problem is corrected. */
        err = switchPtr->WriteUINT32Mult(sw,
                                         FM10000_CRM_IP(0),
                                         FM10000_CRM_IP_WIDTH,
                                         ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

        for (i = 0 ; i < FM10000_CRM_IP_WIDTH ; i++)
        {
            parityInfo->crm_ip[i] |= ipVal[i];
        }

        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;

    }   /* end if (ipVal[0] || ipVal[1] || ipVal[2]) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000CrmInterruptHandler */




/*****************************************************************************/
/** fm10000FHHeadInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage FH_HEAD interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FHHeadInterruptHandler(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           ipVal;
    fm_uint64           imVal;
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    err = switchPtr->ReadUINT64(sw, FM10000_FH_HEAD_IP(0), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT64(sw, FM10000_FH_HEAD_IM(0), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_FH_HEAD_IM(0), ipVal | imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    ipVal &= ~imVal;

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "FH_HEAD_IP=0x%012llx\n", ipVal);

    if (ipVal)
    {
        /* Acknowledge FH_HEAD interrupts. */
        err = switchPtr->WriteUINT64(sw, FM10000_FH_HEAD_IP(0), ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

        parityInfo->fh_head_ip |= ipVal;
        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;

    }   /* end if (ipVal) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000FHHeadInterruptHandler */




/*****************************************************************************/
/** fm10000FHTailInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage FH_TAIL interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[out]      fh_tail points to the location to receive the masked
 *                  value of the FH_TAIL_IP register.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FHTailInterruptHandler(fm_switch * switchPtr,
                                        fm_uint32 * fh_tail)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint32           ipVal;
    fm_uint32           imVal;
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    err = switchPtr->ReadUINT32(sw, FM10000_FH_TAIL_IP(), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT32(sw, FM10000_FH_TAIL_IM(), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    ipVal &= ~switchExt->fhTailImProp;

    err = switchPtr->WriteUINT32(sw, FM10000_FH_TAIL_IM(), ipVal | imVal);
    FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

    ipVal &= ~imVal;

    *fh_tail = ipVal;

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "FH_TAIL_IP=0x%08x\n", ipVal);

    if (ipVal)
    {
        /* Acknowledge FH_TAIL interrupts. */
        err = switchPtr->WriteUINT32(sw, FM10000_FH_TAIL_IP(), ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);
    }

    ipVal &= FM10000_INT_FH_TAIL_PARITY_ERR;

    if (ipVal)
    {
        parityInfo->fh_tail_ip |= ipVal;
        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000FHTailInterruptHandler */




/*****************************************************************************/
/** fm10000TEInterruptHandler
 * \ingroup intSwitch
 *
 * \desc            First-stage TE interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       index is the Tunneling Engine to operate on, in the
 *                  range 0..1.
 * 
 * \param[out]      intMask points to the location to receive the masked
 *                  value of the TE_IP register.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000TEInterruptHandler(fm_switch * switchPtr,
                                    fm_int      index,
                                    fm_uint64 * intMask)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           ipVal;
    fm_uint64           imVal;
    fm_int              sw;
    fm_status           err;
    fm_status           retStatus;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (index < 0 || index >= FM10000_NUM_TUNNEL_ENGINES)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    if (parityInfo->parityState > FM10000_PARITY_STATE_DECODE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);
    }

    retStatus = FM_OK;

    err = switchPtr->ReadUINT64(sw, FM10000_TE_IP(index, 0), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->ReadUINT64(sw, FM10000_TE_IM(index, 0), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    ipVal &= ~switchExt->teImProp;

    err = switchPtr->WriteUINT64(sw, FM10000_TE_IM(index, 0), imVal | ipVal);
    FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);

    ipVal &= ~imVal;

    if (intMask)
    {
        *intMask = ipVal;
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "TE_IP[%d]=0x%02llx\n", index, ipVal);

    if (ipVal)
    {
        /* Acknowledge TE interrupts. */
        err = switchPtr->WriteUINT64(sw, FM10000_TE_IP(index, 0), ipVal);
        FM_LOG_COMBINE_ON_ERR(FM_LOG_CAT_PARITY, err, retStatus);
    }

    if (ipVal & FM10000_INT_TE_PARITY_ERR)
    {
        parityInfo->te_ip[index] |= ipVal;
        parityInfo->parityState = FM10000_PARITY_STATE_DECODE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000TEInterruptHandler */




/*****************************************************************************/
/** fm10000EnableParityInterrupts
 * \ingroup intSwitch
 *
 * \desc            Enables parity error interrupts for the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000EnableParityInterrupts(fm_int sw)
{
    fm10000_parityInfo * parityInfo;
    fm_switch * switchPtr;
#if (ENABLE_CRM_INTERRUPTS)
    fm_uint32   crmVal[FM10000_CRM_IP_WIDTH];
#endif
    fm_uint64   rv64;
    fm_status   err;
    fm_int      i;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);

    TAKE_REG_LOCK(sw);

    /* Enable SRAM_ERR interrupts. */
    err = switchPtr->WriteUINT64(sw, FM10000_SRAM_ERR_IM(0), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Enable SCHEDULER parity interrupts. */
    err = switchPtr->MaskUINT32(sw,
                                FM10000_SCHED_IM(),
                                FM10000_INT_SCHED_PARITY_ERR,
                                FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Enable MODIFY parity interrupts. */
    err = switchPtr->WriteUINT64(sw, FM10000_MOD_IM(0), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Enable FH_HEAD parity interrupts. */
    err = switchPtr->ReadUINT64(sw, FM10000_FH_HEAD_IM(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    rv64 &= ~FM10000_INT_FH_HEAD_PARITY_ERR;

    err = switchPtr->WriteUINT64(sw, FM10000_FH_HEAD_IM(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Enable FH_TAIL parity interrupts. */
    err = switchPtr->MaskUINT32(sw,
                                FM10000_FH_TAIL_IM(),
                                FM10000_INT_FH_TAIL_PARITY_ERR,
                                FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

#if (ENABLE_CRM_INTERRUPTS)
    /* Enable CRM parity interrupts. */
    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    FM_ARRAY_SET_FIELD(crmVal, FM10000_CRM_IM, SramErr, 0);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);
#endif

    /* Enable TE parity interrupts. */
    for (i = 0 ; i < FM10000_NUM_TUNNEL_ENGINES ; i++)
    {
        err = switchPtr->ReadUINT64(sw, FM10000_TE_IM(i, 0), &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

        rv64 &= ~FM10000_INT_TE_PARITY_ERR;

        err = switchPtr->WriteUINT64(sw, FM10000_TE_IM(i, 0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);
    }

    /* Enable CORE parity interrupts. */
    err = switchPtr->MaskUINT32(sw,
                                FM10000_CORE_INTERRUPT_MASK(),
                                FM10000_INT_CORE_PARITY_ERR,
                                FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    parityInfo->interruptsEnabled = TRUE;

ABORT:
    DROP_REG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fm10000EnableParityInterrupts */




/*****************************************************************************/
/** fm10000DisableParityInterrupts
 * \ingroup intSwitch
 *
 * \desc            Disables parity error interrupts for the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DisableParityInterrupts(fm_int sw)
{
    fm10000_parityInfo * parityInfo;
    fm_switch * switchPtr;
#if (ENABLE_CRM_INTERRUPTS)
    fm_uint32   crmVal[FM10000_CRM_IP_WIDTH];
#endif
    fm_uint64   rv64;
    fm_status   err;
    fm_int      i;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);

    TAKE_REG_LOCK(sw);

    /* Disable SRAM_ERR interrupts. */
    err = switchPtr->WriteUINT64(sw, FM10000_SRAM_ERR_IM(0), ~FM_LITERAL_U64(0));
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Disable SCHEDULER parity interrupts. */
    err = switchPtr->MaskUINT32(sw,
                                FM10000_SCHED_IM(),
                                FM10000_INT_SCHED_PARITY_ERR,
                                TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Disable MODIFY parity interrupts. */
    err = switchPtr->WriteUINT64(sw, FM10000_MOD_IM(0), ~FM_LITERAL_U64(0));
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Disable FH_HEAD parity interrupts. */
    err = switchPtr->ReadUINT64(sw, FM10000_FH_HEAD_IM(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    rv64 |= FM10000_INT_FH_HEAD_PARITY_ERR;

    err = switchPtr->WriteUINT64(sw, FM10000_FH_HEAD_IM(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Disable FH_TAIL parity interrupts. */
    err = switchPtr->MaskUINT32(sw,
                                FM10000_FH_TAIL_IM(),
                                FM10000_INT_FH_TAIL_PARITY_ERR,
                                TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

#if (ENABLE_CRM_INTERRUPTS)
    /* Disable CRM parity interrupts. */
    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    FM_ARRAY_SET_FIELD(crmVal, FM10000_CRM_IM, SramErr, 3);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);
#endif

    /* Disable TE parity interrupts. */
    for (i = 0 ; i < FM10000_NUM_TUNNEL_ENGINES ; i++)
    {
        err = switchPtr->ReadUINT64(sw, FM10000_TE_IM(i, 0), &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

        rv64 |= FM10000_INT_TE_PARITY_ERR;

        err = switchPtr->WriteUINT64(sw, FM10000_TE_IM(i, 0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);
    }

    /* Disable CORE parity interrupts. */
    err = switchPtr->MaskUINT32(sw,
                                FM10000_CORE_INTERRUPT_MASK(),
                                FM10000_INT_CORE_PARITY_ERR,
                                TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    parityInfo->interruptsEnabled = FALSE;

ABORT:
    DROP_REG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fm10000DisableParityInterrupts */

