/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug_regs.c
 * Creation Date:   July 17, 2009 (from fm6000_debug_regs.c)
 * Description:     Provide debugging functions for reading registers by name
 *                  instead of by address.
 *
 * Copyright (c) 2009 - 2014, Intel Corporation
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

#include <debug/fm10000/fm10000_debug_regs_int.h>

/* Use these to dummy out functions that aren't available yet */
#define HAVE_REGISTER_FIELDS    TRUE

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define IS_EPL_QUAD_INDEX(pReg)                                 \
        ( (pReg->flags & REG_FLAG_INDEX0_PER_EPL) &&            \
          (strcasestr(pReg->regname, "FM10000_EPL") != NULL) )

/* Virtual Function regs, not visible in global space */
#define IS_REG_PCIE_VF(regName) ((strncasecmp(regName, "FM10000_PCIE_VF", 15)  == 0) && \
                                 (regName[15] != '_'))
#define IS_REG_PCIE_INDEX(regName) ((strncasecmp(regName, "FM10000_PCIE_", 13)  == 0) && \
                                    !IS_REG_PCIE_VF(regName))
#define IS_REG_INDEX_BY_PCIE(regName) (strncasecmp(regName, "FM10000_INTERRUPT_MASK_PCIE", 27)  == 0)

#define INDEX_AS_PORT_MASK              0x7f
#define MAX_REGISTER_NAME_LENGTH        256
#define MIN_LOGICAL_PORT_INDEX          0
#define MAX_LOGICAL_PORT_INDEX          47  /* this is physical, not logical port! */
#define MIN_INTERNAL_EPL_PORT           3
#define CHANNEL_BIT_MASK                3
#define MAX_WORD_PER_REG                16

#define REG_NAME_PREFIX                 "FM10000_"
#define REG_NAME_PREFIX_LEN             8

#define NUM_REGISTER_FIELDS             80

#define FIELD_FORMAT_B  "    %-30s: %d\n"
#define FIELD_FORMAT_F  "    %-30s: %lld 0x%llx\n"
#define FIELD_FORMAT_W  "    %-30s: 0x%llx%016llx\n"
#define FIELD_FORMAT_S  "    %-30s: %s\n"

/* Prepend FM10000_ here to keep the preprocessor from substituting non-indexed
 * define before appending fields. */ 
#define PRINT_BIT(value, reg, field)       \
    FM_LOG_PRINT( FIELD_FORMAT_B, # field, \
                 FM_ARRAY_GET_BIT(value, FM10000_ ## reg, field) )

#define PRINT_FIELD(value, reg, field)                                  \
    FM_LOG_PRINT( FIELD_FORMAT_F, # field,                              \
                 FM_ARRAY_GET_FIELD64(value, FM10000_ ## reg, field),   \
                 FM_ARRAY_GET_FIELD64(value, FM10000_ ## reg, field) )

#define MAX_STR_LEN                     80

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static const fm_text regNameSuffixStr    = "_P";
static const fm_text regNameSuffixStrAlt = "_M";

/* The dynamically allocated invalid index table. */
static fm_bool *invalidIndexEncountered  = NULL;

static fm_uint32 lastRegValue[MAX_WORD_PER_REG];
static fm_text lastRegName = NULL;

static fm_int regCount = 0; /* why is this global? */

#if defined(__KLOCWORK__)
static fm_bool isFalse = FALSE;
#endif

static fm_char invalidStr[MAX_STR_LEN]; /* Does not support multi thread */

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_status fm10000DbgDumpChipRegister(fm_int             sw,
                                            fm_int             indexA,
                                            fm_int             indexB,
                                            fm_int             indexC,
                                            fm_int             pepOffset,
                                            fm_int             regid,
                                            fm_bool            indexWithPort,
                                            fm_voidptr         callbackInfo,
                                            fm_regDumpCallback callback);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


static fm_status MapPhysicalPortToEplChannel(fm_int  sw,
                                             fm_int  physPort,
                                             fm_int *epl,
                                             fm_int *channel)
{
    return fm10000MapPhysicalPortToEplChannel(sw, physPort, epl, channel);
}



static fm_status MapEplChannelToPhysicalPort(fm_int  sw,
                                             fm_int  epl,
                                             fm_int  channel,
                                             fm_int *physPort)
{
    return fm10000MapEplChannelToPhysicalPort(sw, epl, channel, physPort);
}



static fm_status MapEplChannelToLane(fm_int  sw,
                                     fm_int  epl,
                                     fm_int  channel,
                                     fm_int *lane)
{
    return fm10000MapEplChannelToLane(sw, epl, channel, lane);
}



static fm_status MapEplLaneToChannel(fm_int  sw,
                                     fm_int  epl,
                                     fm_int  lane,
                                     fm_int *channel)
{
    return fm10000MapEplLaneToChannel(sw, epl, lane, channel);
}


/*****************************************************************************/
/** IsInvalidPepAddress
 * \ingroup intDiagReg
 *
 * \desc            Return whether the address is an invalid pep address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]        is the index into the register table.
 *
 * \param[in]       addr is the address of the register.
 *
 * \return          TRUE if address is an invalid pep address.
 *
 *****************************************************************************/
static fm_bool IsInvalidPepAddress(fm_int sw, fm_int addr)
{
    fm_switch      *switchPtr;
    fm_uint32       deviceCfg;
    fm_uint32       pcieMode;
    fm_int          pep;
    fm_bool         pepActive;
    fm_bool         x8;

    if (addr >= FM10000_PCIE_PF_BASE && addr < FM10000_TE_BASE)
    {
        pep = (addr - FM10000_PCIE_PF_BASE) / FM10000_PCIE_PF_SIZE;
        if ((fm10000GetPepResetState(sw, pep, &pepActive) != FM_OK) || !pepActive)
        {
            return TRUE;
        }
        /* Normallize address */
        addr = addr - pep * FM10000_PCIE_PF_SIZE;
        if (addr >= FM10000_PCIE_SERDES_CTRL(0,0) && addr <= FM10000_PCIE_SERDES_CTRL(7,1))
        {
            /* Only certain addresses are valid */
            if (pep == 8)
            {
                if (addr >= FM10000_PCIE_SERDES_CTRL(0,0) && addr <= FM10000_PCIE_SERDES_CTRL(0,1))
                {
                    /* Only first serdes is valid */
                    return FALSE;
                }
                return TRUE;
            }

            switchPtr = GET_SWITCH_PTR( sw );
            switchPtr->ReadUINT32( sw, FM10000_DEVICE_CFG(), &deviceCfg );
            pcieMode = FM_GET_FIELD(deviceCfg, FM10000_DEVICE_CFG, PCIeMode );
            x8 = ((pcieMode & ( 1 << (pep/2) ) ) == 0);
            if (x8)
            {
                /* Odd peps are invalid */
                if (pep & 1)
                {
                    return TRUE;
                }
            }
            else
            {
                if (pep & 1)
                {
                    if (addr >= FM10000_PCIE_SERDES_CTRL(0,0) && addr <= FM10000_PCIE_SERDES_CTRL(3,1))
                    {
                        /* first 4 serdes is invalid */
                        return TRUE;
                    }
                }
                else
                {
                    if (addr >= FM10000_PCIE_SERDES_CTRL(4,0) && addr <= FM10000_PCIE_SERDES_CTRL(7,1))
                    {
                        /* last 4 serdes is invalid */
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;

} /* IsInvalidPepAddress */




/*****************************************************************************/
/** fm10000DbgReadRegisterCallback
 * \ingroup intDiagReg
 *
 * \desc            Register dump callback function for
 *                  ''fm10000DbgReadRegister''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regId is the index into the register table.
 *
 * \param[in]       regAddress is the address of the register being read.
 *
 * \param[in]       regSize is the number of words in the register.
 *
 * \param[in]       isStatReg is TRUE if this is a status register.
 *
 * \param[in]       regValue1 is the first 64 bits of the register.
 *
 * \param[in]       regValue2 is the second 64 bits of the register.
 *
 * \param[out]      callbackinfo is a pointer to the memory where
 *                  the register value should be recorded.
 *
 * \return          TRUE unconditionally.
 *
 *****************************************************************************/
static fm_bool fm10000DbgReadRegisterCallback(fm_int     sw,
                                              fm_int     regId,
                                              fm_uint    regAddress,
                                              fm_int     regSize,
                                              fm_bool    isStatReg,
                                              fm_uint64  regValue1,
                                              fm_uint64  regValue2,
                                              fm_voidptr callbackinfo)
{
    fm_uint32 *valuePtr;

    FM_NOT_USED(sw);
    FM_NOT_USED(regId);
    FM_NOT_USED(regAddress);
    FM_NOT_USED(isStatReg);
    FM_NOT_USED(regValue2);


    switch (regSize)
    {
        case 1:
            {
                fm_uint32 value;

                value = (fm_uint32) regValue1;

                *( (fm_uint32 *) callbackinfo ) = value;
            }
            break;

        case 2:
            {
                *( (fm_uint64 *) callbackinfo ) = regValue1;
            }
            break;

        case 3:
            {
                valuePtr = (fm_uint32 *) callbackinfo;

                valuePtr[0] = (fm_uint32) regValue1;
                valuePtr[1] = (fm_uint32) (regValue1 >> FM_LITERAL_64(32));
                valuePtr[2] = (fm_uint32) regValue2;
            }
            break;

        case 4:
            {
                valuePtr = (fm_uint32 *) callbackinfo;

                valuePtr[0] = (fm_uint32) regValue1;
                valuePtr[1] = (fm_uint32) (regValue1 >> FM_LITERAL_64(32));
                valuePtr[2] = (fm_uint32) regValue2;
                valuePtr[3] = (fm_uint32) (regValue2 >> FM_LITERAL_64(32));
            }
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_DEBUG, "Unknown register size %d\n", regSize);
            return FALSE;

    }   /* end switch (regSize) */

    return TRUE;

}   /* end fm10000DbgReadRegisterCallback */




/*****************************************************************************/
/** ParseRegName
 * \ingroup intDiagReg
 *
 * \desc            Parse the input regName and optionally strip FM prefix.
 *
 * \param[in]       regName points to the register name.
 *
 * \param[in]       stripPrefix indicates whether to strip the FM10000_ prefix.
 *
 * \param[out]      registerName points to caller-allocated storage where the 
 *                  function place filtered register name. It must contain
 *                  at least MAX_REGISTER_NAME_LENGTH bytes of storage.
 *
 * \param[in]       indexByPort points to caller-supplied storage where the
 *  				function indicates whether the register is indexed by
 *  				logical port number.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void ParseRegName(fm_text   regName,
                         fm_bool   stripPrefix,
                         fm_text   registerName,
                         fm_bool * indexByPort)
{
    size_t    suffixOffset;
    size_t    nameLen;
    size_t    suffixLen;

    nameLen = strlen(regName);
    if (nameLen >= MAX_REGISTER_NAME_LENGTH)
    {
        nameLen = MAX_REGISTER_NAME_LENGTH - 1;
        regName[MAX_REGISTER_NAME_LENGTH - 1] = '\0';
    }

    if (strncmp(REG_NAME_PREFIX, regName, REG_NAME_PREFIX_LEN) == 0)
    {
        FM_STRNCPY_S(registerName,
                     MAX_REGISTER_NAME_LENGTH,
                     regName + REG_NAME_PREFIX_LEN,
                     nameLen - REG_NAME_PREFIX_LEN + 1);
    }
    else
    {
        FM_STRNCPY_S(registerName,
                     MAX_REGISTER_NAME_LENGTH,
                     regName,
                     nameLen + 1);
    }

    suffixLen = strlen(regNameSuffixStr);
    nameLen = FM_STRNLEN_S(registerName, MAX_REGISTER_NAME_LENGTH);
    suffixOffset = nameLen - suffixLen;

    /**************************************************
     * If the specified register name has a _P suffix,
     * the caller thinks this is a port-indexed register 
     * and wants to index by logical port. 
     **************************************************/

    if (strcasecmp(registerName+suffixOffset, regNameSuffixStr) == 0 ||
        strcasecmp(registerName+suffixOffset, regNameSuffixStrAlt) == 0)
    {
        /* Lop off the suffix */
        registerName[suffixOffset] = '\0';
        *indexByPort = TRUE;
    }

}   /* end ParseRegName */




/*****************************************************************************/
/** GetEplIndex
 * \ingroup intDiagReg
 *
 * \desc            Return the EPL index for a given logical port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logPort is the logical port.
 *
 * \param[in]       regName points to the register name.
 *
 * \param[out]      idx0 points to caller-allocated storage where the 
 *                  function place the epl number.
 *
 * \param[out]      idx1 points to caller-allocated storage where the 
 *                  function place the channel or lane number. idx1 can
 *                  be null if the register is only indexed by epl number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetEplIndex(fm_int  sw,
                             fm_int  logPort,
                             fm_text regName,
                             fm_int *idx0,
                             fm_int *idx1)
{
    fm_int    physPort;
    fm_int    epl;
    fm_int    channel;
    fm_status status;
    fm_int    temp;

    status = fmPlatformMapLogicalPortToPhysical(sw,
                                                logPort,
                                                &temp,
                                                &physPort);
    if (status != FM_OK)
    {
        return status;
    }

    status = MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
    if (status != FM_OK)
    {
        return status;
    }

    if (idx1 != NULL)
    {
        *idx0 = epl;
        *idx1 = channel;
    }
    else 
    {
        *idx0 = epl;
    }

    return FM_OK;

}   /* end GetEplIndex */




/*****************************************************************************/
/** IsEntrySelected
 * \ingroup intDiagReg
 *
 * \desc            Check whether entry is selected, and return appropriate
 *                  indices.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pReg points to fm10000DbgFulcrumRegister
 *
 * \param[in]       doOutOfRange indicates whether out of range is selected.
 *
 * \param[in]       reverseIdx indicates whether to reverse the return idx.
 *
 * \param[in]       indexA is the user-specified index.
 *
 * \param[in]       indexB is the user-specified index.
 *
 * \param[in]       indexC is the user-specified index.
 *
 * \param[in]       e2 is the index to be checked against user-specified
 *                  indexA. If it matches, then the entry is selected. If
 *                  they are out of range, the entry is selected if the
 *                  the user-specifed is negative, or if doOutOfRange is set.
 *
 * \param[in]       e1 is the index to be checked against user-specified
 *                  indexB. If it matches, then the entry is selected. If
 *                  they are out of range, the entry is selected if the
 *                  the user-specifed is negative, or if doOutOfRange is set.
 *
 * \param[in]       e0 is the index to be checked against user-specified
 *                  indexC. If it matches, then the entry is selected. If
 *                  they are out of range, the entry is selected if the
 *                  the user-specifed is negative, or if doOutOfRange is set.
 *
 * \param[out]      idx0 points to caller-allocated storage where this function
 *                  should place the appropriate index.
 *
 * \param[out]      idx1 points to caller-allocated storage where this function
 *                  should place the appropriate index.
 *
 * \param[out]      idx2 points to caller-allocated storage where this function
 *                  should place the appropriate index
 *
 * \return          TRUE if entry is selected.
 *
 *****************************************************************************/
static fm_bool IsEntrySelected(fm_int sw, 
                               const fm10000DbgFulcrumRegister *pReg, 
                               fm_bool doOutOfRange, 
                               fm_bool reverseIdx,
                               fm_int indexA, 
                               fm_int indexB, 
                               fm_int indexC,
                               fm_int e0, 
                               fm_int e1, 
                               fm_int e2,
                               fm_int *idx0, 
                               fm_int *idx1, 
                               fm_int *idx2)
{
    fm_bool inRange;
    fm_int  temp;

    switch (pReg->accessMethod)
    {
        case DBLINDEX:
        case MWDBLIDX:
            *idx0 = e0;
            *idx1 = e1;
            *idx2 = 0;

            inRange = indexA >= pReg->indexMin0 && indexA <= pReg->indexMax0;
            if ( (inRange && e0 != indexA) || 
                 (!inRange && indexA >= 0 && !doOutOfRange) )
            {
                return FALSE;
            }
            inRange = indexB >= pReg->indexMin1 && indexB <= pReg->indexMax1;
            if ( (inRange && e1 != indexB) ||
                 (!inRange && indexB >= 0 && !doOutOfRange) )
            {
                return FALSE;
            }

            if (reverseIdx)
            {
                temp = *idx0;
                *idx0 = *idx1;
                *idx1 = temp;
            }

            break;

        case TPLINDEX:
        case MWTPLIDX:
            *idx0 = reverseIdx ? e2 : e0;
            *idx1 = e1;
            *idx2 = reverseIdx ? e0 : e2;

            inRange = indexA >= pReg->indexMin0 && indexA <= pReg->indexMax0;
            if ( (inRange && e0 != indexA) || (!inRange && indexA >= 0 && !doOutOfRange) )
            {
                return FALSE;
            }

            inRange = indexB >= pReg->indexMin1 && indexB <= pReg->indexMax1;
            if ( (inRange && e1 != indexB) || (!inRange && indexB >= 0 && !doOutOfRange) )
            {
                return FALSE;
            }

            inRange = indexC >= pReg->indexMin2 && indexC <= pReg->indexMax2;
            if ( (inRange && e2 != indexC) || (!inRange && indexC >= 0 && !doOutOfRange) )
            {
                return FALSE;
            }

            break;

        default:
            *idx0 = e0;
            *idx1 = 0;
            *idx2 = 0;

            inRange = indexA >= pReg->indexMin0 && indexA <= pReg->indexMax0;
            if ( (inRange && e0 != indexA) || 
                 (!inRange && indexA >= 0 && !doOutOfRange) )
            {
                return FALSE;
            }

            break;

    }   /* end switch (pReg->accessMethod) */

    return TRUE;

}   /* end IsEntrySelected */



/*****************************************************************************/
/** GetRegOffsetIdx
 * \ingroup intDiagReg
 *
 * \desc            Return the register offset and its physical indices given
 *                  the user input indices.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regEntry points to ''fm10000DbgFulcrumRegister''.
 *
 * \param[in]       indexA is the user-specified index.
 *
 * \param[in]       indexB is the user-specified index.
 *
 * \param[in]       indexC is the user-specified index.
 *
 * \param[in]       indexByPort indicates whether to index by logical port.
 *
 * \param[in]       reverseIdx indicates whether to reverse the return idx.
 *
 * \param[out]      offset points to caller-allocated storage where this function
 *                  should place the offsets for the given register. The
 *                  caller can pass in a NULL pointer if no return value
 *                  is required.
 *
 * \param[out]      physIndex0 points to caller-allocated storage where 
 *                  this function should place the register index 0, if used.
 *
 * \param[out]      physIndex1 points to caller-allocated storage where 
 *                  this function should place the register index 1, if used.
 *
 * \param[out]      physIndex2 points to caller-allocated storage where 
 *                  this function should place the register index 2, if used.
 *
 * \return          TRUE if entry is selected.
 *
 *****************************************************************************/
static fm_status GetRegOffsetIdx(fm_int sw,
                                 const fm10000DbgFulcrumRegister *regEntry,
                                 fm_int indexA,
                                 fm_int indexB,
                                 fm_int indexC,
                                 fm_bool indexByPort,
                                 fm_int  pep,
                                 fm_bool reverseIdx,
                                 fm_uint *offset,
                                 fm_int *physIndex0,
                                 fm_int *physIndex1,
                                 fm_int *physIndex2,
                                 fm_int *pepIdx)
{
    fm_status err = FM_OK;
    fm_uint   i;
    fm_int    temp;
    fm_int    physIdx0 = 0;
    fm_int    physIdx1 = 0;
    fm_int    physIdx2 = 0;
    fm_int    pepOffset = 0;
    fm_bool   pepActive;

    if (IS_REG_PCIE_INDEX(regEntry->regname))
    {
        if (indexByPort)
        {
            err = fm10000MapLogicalPortToPep(sw, indexA, &pep);
            if (err != FM_OK)
            {
                pep = 0;
            }
            indexA = indexB;
            indexB = indexC;
            indexC = 0;
        }
        else
        {
            /* If not index by pep, then pep is passed in as an argument */
            if (pep < 0 || pep > FM10000_MAX_PEP)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
        }

        if ((fm10000GetPepResetState(sw, pep, &pepActive) != FM_OK) || !pepActive)
        {
            return FM_ERR_INVALID_ARGUMENT;
        }

        if (pepIdx)
        {
            *pepIdx = pep;
        }
        pepOffset = pep * FM10000_PCIE_PF_SIZE;

    }

    switch (regEntry->accessMethod)
    {
        case SCALAR:
        case MULTIWRD:

            for (i = 0 ;
                 (offset != NULL) && (i < (fm_uint32) regEntry->wordcount) ;
                 i++)
            {
                offset[i] = regEntry->regAddr + i + pepOffset;
            }

            break;

        case INDEXED:
        case MWINDEX:
            physIdx0 = indexA;

            if (indexByPort && regEntry->flags & REG_FLAG_INDEX0_PER_PORT)
            {
                err = fmDbgMapLogicalPortToPhysical(sw, indexA, &physIdx0);
                if (err != FM_OK)
                {
                    break;
                }
            }

            if (indexByPort && (regEntry->flags & REG_FLAG_INDEX0_PER_EPL) )
            {
                err = GetEplIndex(sw, indexA, NULL, &physIdx0, NULL);
                if (err != FM_OK)
                {
                    break;
                }
            }

            if (physIdx0 >= regEntry->indexMin0 && physIdx0 <= regEntry->indexMax0)
            {
                for (i = 0 ;
                     (offset != NULL) && (i < (fm_uint32) regEntry->wordcount) ;
                     i++)
                {
                    offset[i] = (physIdx0 * regEntry->indexStep0) + (regEntry->regAddr + i + pepOffset) ;
                }
            }
            else
            {
                err = FM_ERR_INVALID_INDEX;
            }

            break;

        case MWDBLIDX:
        case DBLINDEX:
            physIdx0 = indexA;
            physIdx1 = indexB;

            if (indexByPort && regEntry->flags & REG_FLAG_INDEX0_PER_PORT)
            {
                err = fmDbgMapLogicalPortToPhysical(sw, indexA, &physIdx0);
                if (err != FM_OK)
                {
                    break;
                }
            }

            if (indexByPort && regEntry->flags & REG_FLAG_INDEX1_PER_PORT)
            {
                err = fmDbgMapLogicalPortToPhysical(sw, indexB, &physIdx1);
                if (err != FM_OK)
                {
                    break;
                }
            }

            if (indexByPort && (regEntry->flags & REG_FLAG_INDEX0_CHAN_1_EPL) )
            {
                err = GetEplIndex(sw, indexA, regEntry->regname, &physIdx0, &physIdx1);
                if (err != FM_OK)
                {
                    break;
                }
            }

            if ( (physIdx0 >= regEntry->indexMin0 && physIdx0 <= regEntry->indexMax0) &&
                 (physIdx1 >= regEntry->indexMin1 && physIdx1 <= regEntry->indexMax1) )
            {

                for (i = 0 ;
                     (offset != NULL) && (i < (fm_uint32) regEntry->wordcount) ;
                     i++)
                {
                    offset[i] = (physIdx0 * regEntry->indexStep0)
                                + (physIdx1 * regEntry->indexStep1)
                                + (regEntry->regAddr + i + pepOffset);
                }
            }
            else
            {
                err = FM_ERR_INVALID_INDEX;
            }

            if (reverseIdx)
            {
                temp = physIdx0;
                physIdx0 = physIdx1;
                physIdx1 = temp;
            }

            break;

        case TPLINDEX:
        case MWTPLIDX:
            physIdx0 = indexA;
            physIdx1 = indexB;
            physIdx2 = indexC;

            /* These registers do not have index by port type
             * Just a sanity check here
             */
            if ( indexByPort && regEntry->flags & 
                 (REG_FLAG_INDEX0_PER_PORT |
                  REG_FLAG_INDEX1_PER_PORT |
                  REG_FLAG_INDEX0_CHAN_1_EPL) )
            {
                err = FM_ERR_UNSUPPORTED;
                break;
            }

            if ( (physIdx0 >= regEntry->indexMin0 && physIdx0 <= regEntry->indexMax0) &&
                 (physIdx1 >= regEntry->indexMin1 && physIdx1 <= regEntry->indexMax1) &&
                 (physIdx2 >= regEntry->indexMin2 && physIdx2 <= regEntry->indexMax2) )
            {
                for (i = 0 ;
                     (offset != NULL) && (i < (fm_uint32) regEntry->wordcount) ;
                     i++)
                {
                    offset[i] = (physIdx0 * regEntry->indexStep0) +
                                (physIdx1 * regEntry->indexStep1)+
                                (physIdx2 * regEntry->indexStep2) +
                                (regEntry->regAddr + i + pepOffset);
                }
            }
            else
            {
                err = FM_ERR_INVALID_INDEX;
            }

            if (reverseIdx)
            {
                temp = physIdx0;
                physIdx0 = physIdx2;
                physIdx2 = temp;
            }

            break;
    }

    *physIndex0 = physIdx0;
    *physIndex1 = physIdx1;
    *physIndex2 = physIdx2;

    return err;

}   /* end GetRegOffsetIdx */




/*****************************************************************************/
/** fm10000DbgDumpChipRegisterInt
 * \ingroup intDiagReg
 *
 * \desc            A wrapper function before calling fm10000DbgDumpChipRegister
 *                  that allows looping thru the full range of each indices, 
 *                  when argument is out of the register index range.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  or triply indexed, this argument specifies the left index 
 *                  as the register is described in the FocalPoint datasheet. 
 *                  For some indexed registers, this index may represent a 
 *                  port number. In those cases, this argument should specify 
 *                  the logical port number. If indexA is out of range, the
 *                  whole range is selected.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number. If indexB is out of range, the
 *                  whole range is selected.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers. If indexC is out of range, the
 *                  whole range is selected.
 *
 * \param[in]       pepIdx is used to index into PEP PCIE register space.
 *
 *
 * \param[in]       tblIndex is the index into the register table to be processed.
 *
 * \param[in]       indexByPort may be set to TRUE for registers that are
 *                  normally indexed by logical port number. In such a case,
 *                  indexA should be the logical port number.
 *                                                                      \lb\lb
 *                  If the specified register is not normally indexed by
 *                  logical port, this argument will be ignored.
 *
 * \param[in]       callbackInfo is a cookie to be passed to the callback
 *                  function.
 *
 * \param[in]       callback points to the callback function to be executed
 *                  for processing the output.
 *
 * \param[out]      regCount points to caller-allocated storage where 
 *                  this function increments by one for every entry found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regid is not recognized.
 * \return          FM_ERR_INVALID_INDEX if index is out of range for the 
 *                  specified register.
 * \return          FM_ERR_REG_SNAPSHOT_FULL when called to snapshot registers,
 *                  indicates that snapshot memory is full.
 *
 *****************************************************************************/
static fm_status fm10000DbgDumpRegisterInt(fm_int             sw,
                                           fm_int             indexA,
                                           fm_int             indexB,
                                           fm_int             indexC,
                                           fm_int             pepIdx,
                                           fm_int             tblIndex,
                                           fm_bool            indexByPort,
                                           fm_voidptr         callbackInfo,
                                           fm_regDumpCallback callback,
                                           fm_int            *regCount)

{
    const fm10000DbgFulcrumRegister *pReg;
    fm_status                       err = FM_ERR_UNKNOWN_REGISTER;
    fm_int                          m,k,j;
    fm_int                          index0, index1, index2;
    fm_int                          pep;
    fm_int                          pepStart, pepEnd;
    fm_bool                         pepActive;
    fm_switch                       *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);
    pReg = &fm10000RegisterTable[tblIndex];

    if (indexByPort)
    {
        if (indexA >= 0)
        {
            err = GetRegOffsetIdx(sw,
                                  pReg,
                                  indexA,
                                  indexB,
                                  indexC,
                                  indexByPort,
                                  0,
                                  FALSE,
                                  NULL,
                                  &index0,
                                  &index1,
                                  &index2,
                                  &pep);
            if (err == FM_OK)
            {
                indexA = index0;
                indexB = index1;
                indexC = index2;
                pepIdx = pep;
            }
            else
            {
                indexA = -1;
                indexB = -1;
                indexC = 0;
            }
        }
        else
        {
            indexA = -1;
            indexB = -1;
            indexC = 0;
        }
    }

    /* PCIE registers */
    if (IS_REG_PCIE_INDEX(pReg->regname))
    {
        if (pepIdx < 0 || pepIdx > FM10000_MAX_PEP)
        {
            pepStart = 0;
            pepEnd = FM10000_MAX_PEP;
        }
        else
        {
            pepStart = pepIdx;
            pepEnd = pepIdx;
        }
    }
    else
    {
        pepStart = 0;
        pepEnd = 0;
    }
  
    for (pep = pepStart ; pep <= pepEnd; pep++)
    {
        if (IS_REG_PCIE_INDEX(pReg->regname))
        {
            if ((fm10000GetPepResetState(sw, pep, &pepActive) != FM_OK) || !pepActive)
            {
                /* Skip invalid peps */
                continue;
            }
        }

        for (m = pReg->indexMin2 ; m <= pReg->indexMax2 ; m++)
        {
            for (k = pReg->indexMin1 ; k <= pReg->indexMax1 ; k++)
            {
                for (j = pReg->indexMin0 ; j <= pReg->indexMax0 ; j++)
                {
                    if (IsEntrySelected(sw, pReg, TRUE, FALSE,
                                        indexA, indexB, indexC,
                                        j, k, m,
                                        &index0, &index1, &index2))
                    {
                        err  = fm10000DbgDumpChipRegister(sw,
                                                          index0,
                                                          index1,
                                                          index2,
                                                          pep * FM10000_PCIE_PF_SIZE,
                                                          tblIndex,
                                                          FALSE,
                                                          0,
                                                          fmDbgPrintRegValue);
                         FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_DEBUG, err);
                         lastRegName = pReg->regname;
                         (*regCount)++;
                    }
                }   /* end for (j = pReg->indexMin0 ; j <= pReg->indexMax0 ; j++) */
            }   /* end for (k = pReg->indexMin1 ; k <= pReg->indexMax1 ; k++) */
        }   /* end for (m = pReg->indexMin2 ; m <= pReg->indexMax2 ; m++) */
    } /* end for (pep = pepStart ; pep < pepEnd; pep++) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);

}   /* end fm10000DbgDumpRegisterInt */




/*****************************************************************************/
/** UpdateRegisterField
 * \ingroup intDiagReg
 *
 * \desc            Update the register field given register offset and value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pReg points to fm10000DbgFulcrumRegister.
 *
 * \param[in]       fieldName is field name.
 *
 * \param[in]       offset is the address of the register.
 *
 * \param[in]       value is the value to set the register field to.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateRegisterField(fm_int     sw,
                                     const fm10000DbgFulcrumRegister *pReg,
                                     fm_text    fieldName,
                                     fm_uint    offset,
                                     fm_uint64  value)
{
#if HAVE_REGISTER_FIELDS
    fm_status  err;
    fm_switch *switchPtr;
    fm_int     i;
    fm_uint32  regVal[MAX_WORD_PER_REG];

    switchPtr = GET_SWITCH_PTR(sw);

    if (pReg->wordcount > MAX_WORD_PER_REG)
    {
         FM_LOG_PRINT("No support for reg size (%d) greater than %d words\n",
                      pReg->wordcount, MAX_WORD_PER_REG);
         FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_UNSUPPORTED);
    }

    if (IsInvalidPepAddress(sw, offset))
    {
         FM_LOG_PRINT("Accessing invalid pep register 0x%x\n", offset);
         FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < pReg->wordcount ; i++)
    {
        err = switchPtr->ReadUINT32(sw, offset + i, &regVal[i]);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);
    }

    err = fm10000DbgSetRegField(pReg->regname, fieldName, regVal, value);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);

    for (i = 0 ; i < pReg->wordcount ; i++)
    {
        err = switchPtr->WriteUINT32(sw, offset + i, regVal[i]);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);
#else
    FM_NOT_USED(sw);
    FM_NOT_USED(pReg);
    FM_NOT_USED(fieldName);
    FM_NOT_USED(offset);
    FM_NOT_USED(value);
    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_UNSUPPORTED);
#endif

}   /* end UpdateRegisterField */




/****************************************************************************/
/** StripFmPrefix
 * \ingroup intDiag
 *
 * \desc            Strip off the FM_ or FMxxxx_ prefix.
 *
 * \param[in]       registerName points to the register name string.
 *
 * \return          Pointer to register name without FMxxxx_ prefix.
 *
 *****************************************************************************/
static fm_text StripFmPrefix(fm_text registerName)
{

    if (registerName)
    {
        if (strncmp("FM_", registerName, 3) == 0)
        {
            return registerName + 3;
        }

        if (strncmp("FM10000_", registerName, 8) == 0)
        {
            return registerName + 8;
        }
    }

    return registerName;

}   /* end StripFmPrefix */




/*****************************************************************************/
/** CmpFieldStart
 * \ingroup intDiag
 *
 * \desc            Comparison function for register field sorting based on
 *                  start bit value.
 *
 * \param[in]       p1 points to the first register field.
 *
 * \param[in]       p2 points to the second register field.
 *
 * \return          negative if the first entry comes before the 2nd entry.
 * \return          positive if the first entry comes after the 2nd entry.
 *
 *****************************************************************************/
static fm_int CmpFieldStart(const void *p1, const void *p2)
{

   return( ( *(fmRegisterField * const *) p1)->start -
           ( *(fmRegisterField * const *) p2)->start );

}   /* end CmpFieldStart */




static fm_text TLinkFaultSignalStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "LFS_OK(0)";

        case 1: return "LFS_LOCAL_FAULT(1)";

        case 2: return "LFS_REMOTE_FAULT(2)";

        case 3: return "LFS_RESERVED(3)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "LFS_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end TLinkFaultSignalStr */


static fm_text OverrideStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "NORMAL(0)";

        case 1: return "FORCE_GODD(1)";

        case 2: return "FORCE_BAD(2)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "LFS_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end OverrideStr */



static fm_text TxMacFaultModeStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "FM_NORMAL(0)";

        case 1: return "FM_FORCE_IDLE(1)";

        case 2: return "FM_FORCE_LOCAL_FAULT(2)";

        case 3: return "FM_FORCE_REMOTE_FAULT(3)";

        case 4: return "FM_LINK_INTERRUPTION(4)";

        case 5: return "FM_FORCE_OK(5)";

        case 6: return "FM_FORCE_ERROR(6)";

        case 7: return "FM_FORCE_USER_VAL(7)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TX_FAULT_MODE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end TxMacFaultModeStr */


static fm_text TickTimeScaleStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "ON(0)";

        case 1: return "50NS(1)";

        case 2: return "1US(2)";

        case 3: return "10US(3)";

        case 4: return "100US(4)";

        case 5: return "1MS(5)";

        case 6: return "10MS(6)";

        case 7: return "100MS(7)";

        case 8: return "1S(8)";

        case 9: return "OFF(9)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TIMESCALE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end TickTimeScaleStr */


static fm_text EeeTimeScaleStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "50NS(0)";

        case 1: return "1US(1)";

        case 2: return "10US(2)";

        case 3: return "100US(3)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TIMESCALE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end EeeTimeScaleStr */



static fm_text TxMacDrainModeStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "DRAIN_DRAIN(0)";

        case 1: return "DRAIN_NORMAL(1)";

        case 2: return "HOLD_NORMAL(2)";

        case 3: return "HOLD_HOLD(3)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TX_DRAIN_MODE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end TxMacDrainModeStr */



static fm_text RxDrainModeStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "NORMAL(0)";
        case 1: return "DRAIN(1)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "RX_DRAIN_MODE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end RxDrainModeStr */



static fm_text TxMacFcsModeStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "TX_FCS_PASSTRHRU(0)";

        case 1: return "TX_FCS_PASSTRHRU_CHECK(1)";

        case 2: return "TX_FCS_INSERT_GOOD(2)";

        case 3: return "TX_FCS_INSERT_BAD(3)";

        case 4: return "TX_FCS_INSERT_NORMAL(4)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TX_FCS_MODE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end TxMacFcsModeStr */





static fm_text PreambleModeStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "PM_STRIP(0)";

        case 1: return "PM_PASSTHRU(1)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "PREAMBLE_MODE_INVALID_VALUE(%d)", value);
            return invalidStr;
    }
}   /* end PreambleModeStr */


static fm_text PcsSelStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "DISABLE(0)";

        case 1: return "AN_73(1)";

        case 2: return "SGMII_10(2)";

        case 3: return "SGMII_100(3)";

        case 4: return "SGMII_1000(4)";

        case 5: return "1000BASEX(5)";

        case 6: return "10GBASER(6)";

        case 7: return "40GBASER(7)";

        case 8: return "100GBASER(8)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "PCS_SEL_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end PcsSelStr */




static fm_text QplModeStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "XX_XX_XX_XX(0): No lane mode";

        case 1: return "L1_L1_L1_L1(1): 4 single lane ports";

        case 2: return "XX_XX_XX_L4(2): 4 lanes, single lane uses PORT3 and lane 3";

        case 3: return "XX_XX_L4_XX(3): 4 lanes, single lane uses PORT2 and lane 2";

        case 4: return "XX_L4_XX_XX(4): 4 lanes, single lane uses PORT1 and lane 1";

        case 5: return "L4_XX_XX_XX(5): 4 lanes, single lane uses PORT0 and lane 0";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "QPL_MODE_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end QplModeStr */



/**
 * Return the name of the AN_37_STATUS.State
 */
static fm_text An37StateStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "AN37_ENABLE(0)";

        case 1: return "AN37_RESTART(1)";

        case 2: return "AN37_DISABLE_LINK_OK(2)";

        case 3: return "AN37_ABILITY_DETECT(3)";

        case 4: return "AN37_ACKNOWLEDGE_DETECT(4)";

        case 5: return "AN37_COMPLETE_ACKNOWLEDGE(5)";

        case 6: return "AN37_IDLE_DETECT(6)";

        case 7: return "AN37_LINK_OK(7)";

        case 8: return "AN37_NEXT_PAGE_WAIT(8)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "AN37_INVALID_STATE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end An37StateStr */




/**
 * Return the name of the AN_73_STATUS.State
 */
static fm_text An73StateStr(fm_uint value)
{
    switch (value)
    {
        case 0: return "AN73_ENABLE(0)";

        case 1: return "AN73_TRANSMIT_DISABLE(1)";

        case 2: return "AN73_ABILITY_DETECT(2)";

        case 3: return "AN73_ACKNOWLEDGE_DETECT(3)";

        case 4: return "AN73_COMPLETE_ACKNOWLEDGE(4)";

        case 5: return "AN73_NEXT_PAGE_WAIT(5)";

        case 6: return "AN73_GOOD_CHECK(6)";

        case 7: return "AN73_GOOD(7)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "AN73_INVALID_STATE(%d)", value);
            return invalidStr;
    }   /* end switch (value) */
}   /* end An73StateStr */




static fm_text TxPcs10GBaserState(fm_uint value)
{
    switch (value)
    {
        case 0: return "INIT(0)";

        case 1: return "C(1)";

        case 2: return "D(2)";

        case 3: return "T(3)";

        case 4: return "E(4)";

        case 5: return "LI(4)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TGBR_TXSM_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end TxPcs10GBaserState */


static fm_text TxPcs10GBaserMode(fm_uint value)
{
    switch (value)
    {
        case 0: return "DATA(0)";

        case 1: return "QUIET(1)";

        case 2: return "ALERT(2)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TGBR_MODE_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end TxPcs10GBaserMode */



static fm_text TxPcs10GBaserLpiState(fm_uint value)
{
    switch (value)
    {
        case 0: return "TX_ACTIVE(0)";

        case 1: return "TX_SLEEP(1)";

        case 2: return "TX_QUIET(2)";

        case 3: return "TX_ALERT(3)";

        case 4: return "TX_WAKE(4)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TGBR_LPI_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end TxPcs10GBaserLpiState */


static fm_text RxPcs10GBaserMode(fm_uint value)
{
    switch (value)
    {
        case 0: return "DATA(0)";

        case 1: return "QUIET(1)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TGBR_MODE_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end RxPcs10GBaserMode */



static fm_text RxPcs10GBaserLpiState(fm_uint value)
{
    switch (value)
    {
        case 0: return "RX_ACTIVE(0)";

        case 1: return "RX_SLEEP(1)";

        case 2: return "NOISE(2)";

        case 3: return "RX_QUIET(3)";

        case 4: return "RX_WAKE(4)";

        case 5: return "RX_WTF(5)";

        case 6: return "RX_LINK_FAIL(6)";

        default: FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "TGBR_LPI_INVALID(%d)", value);
            return invalidStr;
    }   /* end switch (value) */

}   /* end RxPcs10GBaserLpiState */




/*****************************************************************************/
/** fm10000DbgDumpRegisterFieldInt
 * \ingroup intDiag
 *
 * \desc            Dump out each field of the given register.
 *
 * \param[in]       registerName is the register name string.
 *
 * \param[out]      value points to caller supplied storage where the values
 *                  are stored.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpRegisterFieldInt(fm_text registerName, fm_uint32 *value)
{
    fm_int                 i;
    fm_uint64              fieldVal;
    fm_uint64              fieldVal2;
    const fmRegisterField *field;
    const fmRegisterField *sortField[NUM_REGISTER_FIELDS];
    fm_int                 sortCnt;
    fm_int                 idx;
    const fmRegisterField *fields;

    /* Strip off the FM10000_ prefix */
    registerName = StripFmPrefix(registerName);
    fields = fm10000DbgGetRegisterFields(registerName);

    if (fields == NULL)
    {
        return FM_ERR_UNKNOWN_REGISTER;
    }

    sortCnt = 0;
    for (i = 0 ; ; i++)
    {
        if (fields[i].name == NULL)
        {
            break;
        }

        if (sortCnt >= NUM_REGISTER_FIELDS)
        {
            FM_LOG_PRINT("Array size (%d) is not large "
                         "enough to store all fields\n", 
                         NUM_REGISTER_FIELDS);
            break;
        }

        sortField[sortCnt++] = (const fmRegisterField *)&fields[i];
    }

    /* Sort the fields in bit field order */
    qsort(sortField, sortCnt, sizeof(fmRegisterField *), CmpFieldStart);

    for (idx = 0 ; idx < sortCnt ; idx++)
    {
        field = sortField[idx];

        if (field->size <= 64)
        {
            /* Handle all fields as 64-bit wide, and only support up to 64 bit field */
            fieldVal = fmMultiWordBitfieldGet64(value,
                                                field->size + field->start - 1,
                                                field->start);

            /* Print out field value name for some special fields */
            if ((strstr(registerName, "PORT_STATUS") != NULL) &&
                (strstr(field->name, "LinkFault") != NULL))
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TLinkFaultSignalStr(fieldVal));
            }
            else if (strstr(registerName, "PORT_STATUS") != NULL &&
                     strcmp(field->name, "Pcs") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, PcsSelStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     strcmp(field->name, "TxFaultMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TxMacFaultModeStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     strcmp(field->name, "TxPcActTimeScale") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TickTimeScaleStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     strcmp(field->name, "TxDrainMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TxMacDrainModeStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     strcmp(field->name, "RxDrain") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, RxDrainModeStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     (strcmp(field->name, "TxLpiTimescale") == 0 ||
                      strcmp(field->name, "TxLpiHoldTimescale") == 0))
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, EeeTimeScaleStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     strcmp(field->name, "PreambleMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, PreambleModeStr(fieldVal));
            }
            else if (strstr(registerName, "MAC_CFG") != NULL &&
                     strcmp(field->name, "TxFcsMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TxMacFcsModeStr(fieldVal));
            }
            else if (strstr(registerName, "EPL_CFG_B") != NULL &&
                     strstr(field->name, "PcsSel") != NULL)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, PcsSelStr(fieldVal));
            }
            else if (strstr(registerName, "EPL_CFG_B") != NULL &&
                     strcmp(field->name, "QplMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, QplModeStr(fieldVal));
            }
            else if (strstr(registerName, "AN_37_STATUS") != NULL &&
                     strcmp(field->name, "State") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, An37StateStr(fieldVal));
            }
            else if (strstr(registerName, "AN_37_TIMER_CFG") != NULL &&
                     strcmp(field->name, "TimeScale") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TickTimeScaleStr(fieldVal));
            }
            else if (strstr(registerName, "AN_73_TIMER_CFG") != NULL &&
                     strcmp(field->name, "TimeScale") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TickTimeScaleStr(fieldVal));
            }
            else if (strstr(registerName, "AN_73_STATUS") != NULL &&
                     strcmp(field->name, "State") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, An73StateStr(fieldVal));
            }
            else if (strstr(registerName, "PCS_10GBASER_TX_STATUS") != NULL &&
                     strcmp(field->name, "TxState") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TxPcs10GBaserState(fieldVal));
            }
            else if (strstr(registerName, "PCS_10GBASER_TX_STATUS") != NULL &&
                     strcmp(field->name, "TxMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TxPcs10GBaserMode(fieldVal));
            }
            else if (strstr(registerName, "PCS_10GBASER_TX_STATUS") != NULL &&
                     strcmp(field->name, "TxLpiState") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, TxPcs10GBaserLpiState(fieldVal));
            }
            else if (strstr(registerName, "PCS_10GBASER_RX_STATUS") != NULL &&
                     strcmp(field->name, "RxMode") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, RxPcs10GBaserMode(fieldVal));
            }
            else if (strstr(registerName, "PCS_10GBASER_RX_STATUS") != NULL &&
                     strcmp(field->name, "RxLpiState") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, RxPcs10GBaserLpiState(fieldVal));
            }
            else if (strstr(registerName, "LANE_ENERGY_DETECT_CFG") != NULL &&
                     strcmp(field->name, "EdOverride") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, OverrideStr(fieldVal));
            }
            else if (strstr(registerName, "LANE_SIGNAL_DETECT_CFG") != NULL &&
                     strcmp(field->name, "SdOverride") == 0)
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, field->name, OverrideStr(fieldVal));
            }
            else if (field->size == 1)
            {
                FM_LOG_PRINT(FIELD_FORMAT_B, field->name, (fm_int) fieldVal);
            }
            else
            {
                FM_LOG_PRINT(FIELD_FORMAT_F, field->name, fieldVal, fieldVal);
            }
        }
        else if (field->size <= 128)
        {
            fieldVal = fmMultiWordBitfieldGet64(value,
                                                field->start + 63,
                                                field->start);
            fieldVal2 = fmMultiWordBitfieldGet64(value,
                                                 field->size + field->start - 1,
                                                 field->start+64);

            FM_LOG_PRINT(FIELD_FORMAT_W, field->name, fieldVal2, fieldVal);
        }
        else
        {
            FM_LOG_PRINT(FIELD_FORMAT_S,
                         field->name,
                         "No support for fields over 128 bits");
        }
    }

    return FM_FAIL;

}   /* end fm10000DbgDumpRegisterFieldInt */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000DbgListRegisters
 * \ingroup intDiagReg
 *
 * \desc            Display a list of switch registers that may be displayed
 *                  by calling fmDbgDumpRegister.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       showGlobals should be TRUE to include all global registers
 *                  in the list.
 *
 * \param[in]       showPorts should be TRUE to include all port registers
 *                  in the list.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgListRegisters(fm_int sw, fm_bool showGlobals, fm_bool showPorts)
{
    const fm10000DbgFulcrumRegister *pReg;
    fm_char registerName[MAX_REGISTER_NAME_LENGTH];

    FM_NOT_USED(sw);

    if (showGlobals)
    {
        pReg = fm10000RegisterTable;

        FM_LOG_PRINT("\nGlobal Register List\n");
        FM_LOG_PRINT("Name                                     Length  "
                     "Min Index0   Max Index0   Min Index1   "
                     "Max Index1   Min Index2   Max Index2\n");

        while (pReg->regname != NULL)
        {
            if ( !(pReg->flags & REG_FLAG_PER_PORT) &&
                 !(pReg->flags & REG_FLAG_END_OF_REGS) )
            {
                FM_LOG_PRINT("%-41s  %-2d",
                             pReg->regname, pReg->wordcount);

                if ( (pReg->indexMax0 != 0) || 
                     (pReg->indexMax1 != 0) ||
                     (pReg->indexMax2 != 0) )
                {
                    FM_LOG_PRINT("      %-10d   %-10d   %-10d   %-10d   %-10d   %d", 
                                 pReg->indexMin0, 
                                 pReg->indexMax0, 
                                 pReg->indexMin1, 
                                 pReg->indexMax1,
                                 pReg->indexMin2, 
                                 pReg->indexMax2);
                }

                FM_LOG_PRINT("\n");
            }

            ++pReg;
        }
    }

    if (showPorts)
    {
        pReg = fm10000RegisterTable;

        FM_LOG_PRINT("\nPort Register List\n");
        FM_LOG_PRINT("Name                                     Length  "
                     "Min Index0   Max Index0   Min Index1   "
                     "Max Index1   Min Index2   Max Index2\n");

        while (pReg->regname != NULL)
        {
            if (pReg->flags & REG_FLAG_PER_PORT)
            {
                FM_LOG_PRINT("%-41s  %-2d      %-10d   %-10d   %-10d   "
                             "%-10d   %-10d   %d\n",
                             pReg->regname, 
                             pReg->wordcount, 
                             pReg->indexMin0, 
                             pReg->indexMax0, 
                             pReg->indexMin1, 
                             pReg->indexMax1,
                             pReg->indexMin2, 
                             pReg->indexMax2);

                /**************************************************
                 * If this is a register that is indexed by EPL and
                 * channel, list it again with the special suffix
                 * indicating it can be indexed by logical port
                 * as well.
                 *
                 * The min and max values won't be exactly right
                 * since they depend on the logical port map.
                 * Just use the internal port range.
                 **************************************************/

                if (pReg->flags & 
                    (REG_FLAG_INDEX0_CHAN_1_EPL | REG_FLAG_INDEX0_PER_EPL))
                {
                    /* Copy the register name locally so that we can ...*/
                    FM_STRCPY_S( registerName,
                                 sizeof(registerName),
                                 pReg->regname );

                    /* ...append the suffix to it. */
                    FM_STRCAT_S( registerName,
                                 sizeof(registerName),
                                 regNameSuffixStr );

                    FM_LOG_PRINT("%-41s  %d       %-10d   %-10d   %-10d   %-10d   %-10d   %d\n",
                                 registerName, 
                                 pReg->wordcount,
                                 MIN_LOGICAL_PORT_INDEX,
                                 MAX_LOGICAL_PORT_INDEX, 
                                 0,
                                 0,
                                 0,
                                 0);

                }   /* end if (pReg->flags & REG_FLAG_INDEX0_CHAN_1_EPL ...) */

            }   /* end if (pReg->flags & REG_FLAG_PER_PORT) */

            ++pReg;

        }   /* end while (pReg->regname != NULL) */

    }   /* end if (showPorts) */

}   /* end fm10000DbgListRegisters */




/*****************************************************************************/
/** fm10000DbgDumpRegisterV3
 * \ingroup intDiagReg
 *
 * \desc            Display the contents of a switch register, or a group
 *                  of related registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  or triply indexed, this argument specifies the left index 
 *                  as the register is described in the FocalPoint datasheet. 
 *                  For some indexed registers, this index may represent a 
 *                  port number. In those cases, this argument should specify 
 *                  the logical port number.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers.
 *
 * \param[in]       regName is the name of the register or group of registers 
 *                  to be displayed.
 *                                                                      \lb\lb
 *                  Some registers are indexed by EPL and channel number, but
 *                  an EPL-channel tuple represents a port. These registers
 *                  have an alias by the same name, but with a "_P" suffix.
 *                  When referred to by the alias name, indexA may be
 *                  specified using a logical port number and indexB and
 *                  indexC will be ignored. This function will automatically
 *                  convert the logical port number into the corresponding
 *                  EPL and channel indexes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regname is not recognized.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpRegisterV3(fm_int  sw,
                                   fm_int  indexA,
                                   fm_int  indexB,
                                   fm_int  indexC,
                                   fm_text regName)
{
    const fm10000DbgFulcrumRegister *pReg;
    fm_int                          pepIdx;
    fm_int                          tblIndex;
    fm_bool                         indexByPort = FALSE;
    fm_status                       err = FM_ERR_UNKNOWN_REGISTER;
    fm_char                         registerName[MAX_REGISTER_NAME_LENGTH];
    fm_bool                         exactMatch;
    fm_int                          idxA;
    fm_int                          idxB;
    fm_int                          idxC;



    ParseRegName(regName, TRUE, registerName, &indexByPort);

    pReg       = fm10000RegisterTable;
    exactMatch = FALSE;
    regCount = 0;
    tblIndex = 0;

    while (pReg->regname != NULL)
    {
        /* Do exact match first to avoid matching multiple registers
         * when one register name is a substring of another or
         * special FM_XXX.
         */
        if (strcasecmp(pReg->regname, registerName) == 0 ||
            strcasecmp(pReg->regname + REG_NAME_PREFIX_LEN, registerName) == 0)
        {
            if (IS_REG_PCIE_VF(pReg->regname))
            {
                FM_LOG_PRINT("%s not visible in global register space.\n",
                             regName);
                FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_UNKNOWN_REGISTER);
            }
            exactMatch = TRUE;
            if (IS_REG_PCIE_INDEX(pReg->regname))
            {
                if (!indexByPort)
                {
                    pepIdx = indexA;
                    idxA = indexB;
                    idxC = indexC / 1000;
                    if ((idxC < 1000) && (idxC > 0))
                    {
                        /* This function has only 3 arguments, 
                         * use indexC to specify both 3rd and 4th argument */
                        idxB = indexC % 1000;
                        idxC = indexC / 1000;
                    }
                    else
                    {
                        idxB = indexC;
                        idxC = -1;
                    }
                }
                else
                {
                    pepIdx = 0;
                    idxA = indexA;
                    idxB = indexB;
                    idxC = indexC;
                }
            }
            else
            {
                pepIdx = 0;
                idxA = indexA;
                idxB = indexB;
                idxC = indexC;
            }
            err = fm10000DbgDumpRegisterInt(sw,
                                            idxA,
                                            idxB,
                                            idxC,
                                            pepIdx,
                                            tblIndex,
                                            indexByPort,
                                            0,
                                            fmDbgPrintRegValue,
                                            &regCount);
            break;
        }

        /* Next table entry */
        ++pReg;
        ++tblIndex;

    }   /* end while (pReg->regname != NULL) */

    pReg     = fm10000RegisterTable;
    tblIndex = 0;

    /* Do partial match when no exact match is found */
    while (!exactMatch && pReg->regname != NULL)
    {
        if (strcasestr(pReg->regname, registerName) != NULL && !IS_REG_PCIE_VF(pReg->regname))
        {
            if (IS_REG_PCIE_INDEX(pReg->regname))
            {
                pepIdx = indexA;
                idxA = indexB;
                idxB = indexC;
                idxC = -1;
            }
            else
            {
                pepIdx = 0;
                idxA = indexA;
                idxB = indexB;
                idxC = indexC;
            }
            err = fm10000DbgDumpRegisterInt(sw,
                                            idxA,
                                            idxB,
                                            idxC,
                                            pepIdx,
                                            tblIndex,
                                            indexByPort,
                                            0,
                                            fmDbgPrintRegValue,
                                            &regCount);
        }

        /* Next table entry */
        ++pReg;
        ++tblIndex;

    }   /* end while (pReg->regname != NULL) */

    /* Dump out register fields if only one reg is shown */
    if (regCount == 1) 
    {
#if HAVE_REGISTER_FIELDS
        fm10000DbgDumpRegField(lastRegName, lastRegValue);
#else
        err = FM_ERR_UNSUPPORTED;
#endif
    }

    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);

}   /* end fm10000DbgDumpRegisterV3 */




/*****************************************************************************/
/** fm10000DbgDumpRegisterV2
 * \ingroup intDiagReg
 *
 * \desc            Display the contents of a switch register, or a group
 *                  of related registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  indexed, this argument specifies the left index as the
 *                  register is described in the FocalPoint datasheet. For
 *                  some indexed registers, this index may represent a port
 *                  number. In those cases, this argument should specify the
 *                  logical port number.
 *
 * \param[in]       indexB is used to index into doubly indexed registers
 *                  and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       regname is the name of the port or group of ports to
 *                  be displayed.
 *                                                                      \lb\lb
 *                  Some registers are indexed by EPL and channel number, but
 *                  an EPL-channel tuple represents a port. These registers
 *                  have an alias by the same name, but with a "_P" suffix.
 *                  When referred to by the alias name, indexA may be
 *                  specified using a logical port number and indexB 
 *                  will be ignored. This function will automatically
 *                  convert the logical port number into the corresponding
 *                  EPL and channel indexes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regname is not recognized.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpRegisterV2(fm_int  sw,
                                   fm_int  indexA,
                                   fm_int  indexB,
                                   fm_text regname)
{

    return fm10000DbgDumpRegisterV3(sw, indexA, indexB, 0, regname);

}   /* end fm10000DbgDumpRegisterV2 */




/*****************************************************************************/
/** fm10000DbgDumpRegister
 * \ingroup intDiagReg
 *
 * \desc            Display the contents of a switch register, or a group
 *                  of related registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number (for port registers) or
 *                  the index (for non-port indexed registers).
 *
 * \param[in]       regname is the name of the port or group of ports to
 *                  be displayed.
 *                                                                      \lb\lb
 *                  Some registers are indexed by EPL and channel number, but
 *                  an EPL-channel tuple represents a port. These registers
 *                  have an alias by the same name, but with a "_P" suffix.
 *                  When referred to by the alias name, port may be
 *                  specified as the logical port number. This function will 
 *                  automatically convert the logical port number into the 
 *                  corresponding EPL and channel indexes.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpRegister(fm_int sw, fm_int port, fm_text regname)
{

   fm10000DbgDumpRegisterV3(sw, port, 0, 0, regname);

}   /* end fm10000DbgDumpRegister */




/*****************************************************************************/
/** fm10000DbgDumpChipRegister
 * \ingroup intDiagReg
 *
 * \desc            Read a chip register or a set of registers and output
 *                  using a callback function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  or triply indexed, this argument specifies the left index 
 *                  as the register is described in the FocalPoint datasheet. 
 *                  For some indexed registers, this index may represent a 
 *                  port number. In those cases, this argument should specify 
 *                  the logical port number.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers.
 *
 * \param[in]       pepOffset is the offset to be added for non-zero pep and
 *                  is zero for all other registers.
 *
 * \param[in]       regid is the index into the register table to be processed.
 *
 * \param[in]       indexByPort may be set to TRUE for registers that are
 *                  normally indexed by logical port number. In such a case,
 *                  indexA should be the logical port number.
 *                                                                      \lb\lb
 *                  If the specified register is not normally indexed by
 *                  logical port, this argument will be ignored.
 *
 * \param[in]       callbackInfo is a cookie to be passed to the callback
 *                  function.
 *
 * \param[in]       callback points to the callback function to be executed
 *                  for processing the output.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regid is not recognized.
 * \return          FM_ERR_INVALID_INDEX if index is out of range for the 
 *                  specified register.
 * \return          FM_ERR_REG_SNAPSHOT_FULL when called to snapshot registers,
 *                  indicates that snapshot memory is full.
 *
 *****************************************************************************/
static fm_status fm10000DbgDumpChipRegister(fm_int             sw,
                                            fm_int             indexA,
                                            fm_int             indexB,
                                            fm_int             indexC,
                                            fm_int             pepOffset,
                                            fm_int             regid,
                                            fm_bool            indexByPort,
                                            fm_voidptr         callbackInfo,
                                            fm_regDumpCallback callback)
{
    fm_int                          wordcount;
    fm_int                          dumpWc;
    fm_uint                         offset[MAX_WORD_PER_REG];
    fm_uint                         regAddress;
    fm_uint                         baseAddress = 0xdeadbeef;
    fm_uint32                       logPortMask;
    fm_uint32                       val;
    fm_uint64                       val1 = FM_LITERAL_U64(0xdeadbeefdeadbeef);
    fm_uint64                       val2 = FM_LITERAL_U64(0xdeadbeefdeadbeef);
    const fm10000DbgFulcrumRegister *regEntry;
    const fm10000DbgFulcrumRegister *wkgRegEntry;
    fm_int                          physIndex0;
    fm_int                          physIndex1;
    fm_int                          physIndex2;
    fm_bool                         isStatReg;
    fm_int                          callbackGranularity;
    fm_int                          currentOffset;
    fm_int                          currentWord;
    fm_status                       err = FM_OK;
    fm_int                          tblIndex;
    fm_int                          regCount;

    /* fm10000RegisterTableSize - 1 because the last entry is a dummy */
    if (regid >= fm10000RegisterTableSize - 1)
    {
        /* Table index out of range. */
        return FM_ERR_UNKNOWN_REGISTER;
    }

    regEntry            = &fm10000RegisterTable[regid];
    wordcount           = regEntry->wordcount;
    callbackGranularity = wordcount;
    regCount            = 0; /* Not used */

    if (callbackGranularity > 4)
    {
        callbackGranularity = 4;
    }

    /**************************************************
     * Is this a statistics register?
     **************************************************/

    if (regEntry->flags & REG_FLAG_STATISTIC)
    {
        /* Yes. */
        isStatReg = TRUE;
    }
    else
    {
        /* No. */
        isStatReg = FALSE;
    }

    /**************************************************
     * Read the register per access type.
     **************************************************/

    switch (regEntry->accessMethod)
    {
        case SCALAR:
        case MULTIWRD:
        case INDEXED:
        case MWINDEX:
        case DBLINDEX:
        case MWDBLIDX:
        case TPLINDEX:
        case MWTPLIDX:
            err = GetRegOffsetIdx(sw,
                                  regEntry,
                                  indexA,
                                  indexB,
                                  indexC,
                                  indexByPort,
                                  pepOffset/FM10000_PCIE_PF_SIZE,
                                  FALSE,
                                  offset,
                                  &physIndex0,
                                  &physIndex1,
                                  &physIndex2,
                                  NULL);
            break;

        case ALL4PORT:

            /**************************************************
             * FM_ALL_REGS_FOR_PORT
             **************************************************/

            /* Validate the port number provided. */
            if (indexA >= regEntry->indexMin0 && indexA <= regEntry->indexMax0)
            {
                /**************************************************
                 * Walk through the table looking for all per-port
                 * registers.
                 **************************************************/

                wkgRegEntry = fm10000RegisterTable;

                while ( (wkgRegEntry->regname != NULL) &&
                        !(wkgRegEntry->flags & REG_FLAG_END_OF_REGS) )
                {
                    tblIndex = wkgRegEntry - fm10000RegisterTable;

                    if (wkgRegEntry->flags & REG_FLAG_PER_PORT)
                    {
                        switch (wkgRegEntry->accessMethod)
                        {
                            case DBLINDEX:
                            case MWDBLIDX:
                                if (wkgRegEntry->flags & REG_FLAG_INDEX0_PER_PORT)
                                {
                                    physIndex0 = -1;
                                    physIndex1 = indexA;
                                    physIndex2 = 0;
                                    break;
                                }
                                if (wkgRegEntry->flags & REG_FLAG_INDEX1_PER_PORT)
                                {
                                    physIndex0 = indexA;
                                    physIndex1 = -1;
                                    physIndex2 = 0;
                                    break;
                                }
                                /* Fall through */
                            default:
                                    physIndex0 = indexA;
                                    physIndex1 = 0;
                                    physIndex2 = 0;
                                break;

                        }   /* end switch (wkgRegEntry->accessMethod) */

                        /* Make the recursive call. */
                        fm10000DbgDumpRegisterInt(sw,
                                                  physIndex0,
                                                  physIndex1,
                                                  physIndex2,
                                                  0,
                                                  tblIndex,
                                                  TRUE,
                                                  callbackInfo,
                                                  callback,
                                                  &regCount);

                    }   /* end if (wkgRegEntry->flags & REG_FLAG_PER_PORT) */

                    ++wkgRegEntry;

                }   /* end while ( (wkgRegEntry->regname != NULL)... */

            }
            else
            {
                err = FM_ERR_INVALID_INDEX;

            }   /* end if (indexA >= regEntry->indexMin0 && indexA <= regEntry->indexMax0) */

            return err;

        case GROUPREG:

            /**************************************************
             * Statistics Groups
             *
             * Make the assumption that all stats group
             * registers are singly indexed. Therefore, the port
             * should be passed in IndexA to the recursed call
             * to fm10000DbgDumpChipRegister.
             **************************************************/

            /* Validate the port number provided. */
            if (indexA >= regEntry->indexMin0 && indexA <= regEntry->indexMax0)
            {
                /**************************************************
                 * Walk through the table looking for all registers
                 * in the matching group number.
                 **************************************************/

                wkgRegEntry = fm10000RegisterTable;

                while ( (wkgRegEntry->regname != NULL) &&
                        !(wkgRegEntry->flags & REG_FLAG_END_OF_REGS) )
                {
                    if (wkgRegEntry->statGroup == regEntry->statGroup)
                    {
                        tblIndex = wkgRegEntry - fm10000RegisterTable;

                        fm10000DbgDumpChipRegister(sw,
                                                   indexA,
                                                   0,
                                                   0,
                                                   0,
                                                   tblIndex,
                                                   FALSE,
                                                   callbackInfo,
                                                   callback);

                    }   /* end if (wkgRegEntry->statGroup == regEntry->statGroup) */

                    ++wkgRegEntry;

                }   /* end while ( (wkgRegEntry->regname != NULL)... */

            }
            else
            {
                err = FM_ERR_INVALID_INDEX;

            }   /* end if (indexA >= regEntry->indexMin0 && indexA <= regEntry->indexMax0) */

            return err;

        case ALLCONFG:

            /**************************************************
             * All configuration registers
             *
             * Walk through the table looking for all non-stats
             * registers.
             **************************************************/

            wkgRegEntry = fm10000RegisterTable;

            while ( (wkgRegEntry->regname != NULL) &&
                    !(wkgRegEntry->flags & REG_FLAG_END_OF_REGS) )
            {
                /* Make sure it's not a stat register. */
                if ( !(wkgRegEntry->flags & REG_FLAG_STATISTIC) )
                {
                    tblIndex = wkgRegEntry - fm10000RegisterTable;

                    err = fm10000DbgDumpRegisterInt(sw,
                                                    -1,
                                                    -1,
                                                    -1,
                                                    -1,
                                                    tblIndex,
                                                    FALSE,
                                                    callbackInfo,
                                                    callback,
                                                    &regCount);
                    if (err != FM_OK)
                    {
                        break;
                    }

                }   /* end if ( !(wkgRegEntry->flags & REG_FLAG_STATISTIC) ) */

                ++wkgRegEntry;

            }   /* end while ( (wkgRegEntry->regname != NULL)... */

            return err;

        case SPECIAL:

            /**************************************************
             * Special pseudo-registers
             *
             * These are special-case pseudo-register names
             * that don't map to hardware registers at all,
             * but rather to software-generated data, such as
             * software counters.
             **************************************************/

#if 0
            if (strcmp(regEntry->regname, "FM_DBG_DIAG_COUNT") == 0)
            {
                return fmDbgDiagCountDump(indexA);
            }
            else if (strcmp(regEntry->regname, "FM_DBG_GLOBAL_DIAG_COUNT") == 0)
            {
                return fmDbgGlobalDiagCountDump();
            }
            else if (strcmp(regEntry->regname, "FM_MEM_USAGE") == 0)
            {
                return fmDbgDumpMemoryUsage(sw);
            }
            else if (strcmp(regEntry->regname, "FM_WATERMARKS") == 0)
            {
                return fmDbgDumpWatermarks(sw);
            }
            else if (strcmp(regEntry->regname, "FM_ACLS_AS_C") == 0)
            {
                fmDbgDumpACLsAsC(sw, NULL);
                return FM_OK;
            }
#endif

        default:
            FM_LOG_PRINT("Unrecognized register table access method %d\n",
                         regEntry->accessMethod);
            err = FM_FAIL;
            break;

    }   /* end switch (regEntry->accessMethod) */

    /**************************************************
     * Now output register data.
     **************************************************/

    if (err == FM_OK)
    {
        currentWord   = 0;
        currentOffset = 0;

        do
        {
            regAddress = offset[currentWord++];

            FM_NOT_USED(logPortMask);

            if (IsInvalidPepAddress(sw, regAddress))
            {
                val = 0xdeaddead;
            }
            else
            {
                err = fmTestReadRegister(sw, regAddress, &val);
            }

            if (err != FM_OK)
            {
                FM_LOG_PRINT("Error reading register address %08X, "
                             "error code = %d\n", regAddress, err);
                break;
            }

            if (currentOffset == 0)
            {
                val1        = (fm_uint64) val;
                val2        = 0;
                baseAddress = regAddress;
            }
            else if (currentOffset == 1)
            {
                val1 |= (fm_uint64) val << 32;
            }
            else if (currentOffset == 2)
            {
                val2 = (fm_uint64) val;
            }
            else
            {
                val2 |= (fm_uint64) val << 32;
            }

            /* Save reg value for dumping out fields */
            if (currentWord <= MAX_WORD_PER_REG)
            {
                lastRegValue[currentWord-1] = val;
            }

            currentOffset++;

            if ( (callbackGranularity <= currentOffset) ||
                 (currentWord >= wordcount) )
            {
                dumpWc = currentOffset % callbackGranularity;

                if (dumpWc == 0)
                {
                    dumpWc = callbackGranularity;
                }

                if ( !callback(sw,
                               regid,
                               baseAddress,
                               dumpWc,
                               isStatReg,
                               val1,
                               val2,
                               callbackInfo) )
                {
                    err = FM_ERR_REG_SNAPSHOT_FULL;
                    break;
                }

                currentOffset = 0;

            }   /* end if (callbackGranularity <= currentOffset) */

        }
        while (currentWord < wordcount);

    }   /* end if (err == FM_OK) */

    return err;

}   /* end fm10000DbgDumpChipRegister */




/*****************************************************************************/
/** fmDbgWriteChipRegister
 * \ingroup intDiagReg
 *
 * \desc            Write to a chip register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regEntry points to the entry in the register table to
 *                  operate on.
 *
 * \param[in]       wordOffset is used for all multi-word registers and 
 *                  ignored for single-word (and single-word indexed) 
 *                  registers.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  or triply indexed, this argument specifies the left index 
 *                  as the register is described in the FocalPoint datasheet. 
 *                  For some indexed registers, this index may represent a 
 *                  port number. In those cases, this argument should specify 
 *                  the logical port number.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers.
 *
 * \param[in]       indexByPort may be set to TRUE for registers that are
 *                  normally indexed by logical port number. In such a case,
 *                  indexA should be the logical port number.
 *                                                                      \lb\lb
 *                  If the specified register is not normally indexed by
 *                  logical port, this argument will be ignored.
 *
 * \param[in]       value is the register value to be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if wordOffset is invalid for the
 *                  specified register.
 * \return          FM_ERR_INVALID_INDEX if indexA, indexB or indexC is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
static fm_status fmDbgWriteChipRegister(fm_int                          sw,
                                        const fm10000DbgFulcrumRegister *regEntry,
                                        fm_int                          wordOffset,
                                        fm_int                          indexA,
                                        fm_int                          indexB,
                                        fm_int                          indexC,
                                        fm_bool                         indexByPort,
                                        fm_uint                         value)
{
    fm_switch *                 switchPtr;
    fm_uint                     offset[MAX_WORD_PER_REG];
    fm_int                      physIndex0;
    fm_int                      physIndex1;
    fm_int                      physIndex2;
    fm_status                   err        = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG,
                 "sw=%d, regEntry=%p, wordOffset=%d, "
                 "indexA=%d, indexB=%d, value=%08X\n",
                 sw,
                 (void *) regEntry,
                 wordOffset,
                 indexA,
                 indexB,
                 value);

    if (wordOffset < 0 || wordOffset >= regEntry->wordcount)
    {
        FM_LOG_PRINT("Offset %d exceeds size (%d) of register.\n", wordOffset, regEntry->wordcount);
        FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Write the register per access type.
     **************************************************/

    switch (regEntry->accessMethod)
    {
        case SCALAR:
        case MULTIWRD:
        case INDEXED:
        case MWINDEX:
        case DBLINDEX:
        case MWDBLIDX:
        case TPLINDEX:
        case MWTPLIDX:
            err = GetRegOffsetIdx(sw,
                                  regEntry,
                                  indexA,
                                  indexB,
                                  indexC,
                                  indexByPort,
                                  0,
                                  FALSE,
                                  offset,
                                  &physIndex0,
                                  &physIndex1,
                                  &physIndex2,
                                  NULL);
            FM_LOG_DEBUG(FM_LOG_CAT_DEBUG,
                         "indexByPort=%d offset0=0x%x physIdx=%d,%d,%d\n",
                         indexByPort,
                         offset[0],
                         physIndex0,
                         physIndex1,
                         physIndex2);
            break;

        default:
            err = FM_ERR_INVALID_REGISTER;
            break;

    }   /* end switch (regEntry->accessMethod) */

    if (err == FM_OK)
    {
        if (IsInvalidPepAddress(sw, offset[wordOffset]))
        {
             FM_LOG_PRINT("Accessing invalid pep register 0x%x.\n", offset[wordOffset]);
             FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_INVALID_ARGUMENT);
        }
        err = switchPtr->WriteUINT32(sw, offset[wordOffset], value);
    }

    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);

}   /* end fmDbgWriteChipRegister */




/*****************************************************************************/
/** fm10000DbgWriteRegisterV3
 * \ingroup intDiagReg
 *
 * \desc            Write the contents of a switch register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       wordOffset is used for all multi-word registers and 
 *                  ignored for single-word (and single-word indexed) 
 *                  registers.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  indexed, this argument specifies the left index as the
 *                  register is described in the FocalPoint datasheet. For
 *                  some indexed registers, this index may represent a port
 *                  number. In those cases, this argument should specify the
 *                  logical port number.
 *
 * \param[in]       indexB is used to index into doubly indexed registers
 *                  and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed, and 
 *                  non-indexed registers.
 *
 * \param[in]       regName is the name of the register to write.
 *
 * \param[in]       value is the register value to be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regName is not recognized.
 * \return          FM_ERR_INVALID_ARGUMENT if wordOffset is invalid for the
 *                  specified register.
 * \return          FM_ERR_INVALID_INDEX if indexA, indexB or indexC is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
fm_status fm10000DbgWriteRegisterV3(fm_int    sw, 
                                    fm_int    wordOffset, 
                                    fm_int    indexA, 
                                    fm_int    indexB, 
                                    fm_int    indexC, 
                                    fm_text   regName, 
                                    fm_uint32 value)
{
    const fm10000DbgFulcrumRegister *regEntry;
    fm_status                       err = FM_ERR_UNKNOWN_REGISTER;
    fm_bool                         indexByPort = FALSE;
    fm_char                         registerName[MAX_REGISTER_NAME_LENGTH];

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG,
                 "sw=%d, wordOffset=%d, indexA=%d, "
                 "indexB=%d, indexC=%d, regName=%s, value=%08X\n",
                 sw,
                 wordOffset,
                 indexA,
                 indexB,
                 indexC,
                 regName,
                 value);

    ParseRegName(regName, TRUE, registerName, &indexByPort);

    /**************************************************
     * Search the table for the specified register name.
     **************************************************/

    regEntry = fm10000RegisterTable;

    while (regEntry->regname != NULL)
    {
        if (strcasecmp(registerName, regEntry->regname + REG_NAME_PREFIX_LEN) == 0)
        {
            err = fmDbgWriteChipRegister(sw,
                                         regEntry,
                                         wordOffset,
                                         indexA,
                                         indexB,
                                         indexC,
                                         indexByPort,
                                         value);
            break;
        }

        ++regEntry;
    }

    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);

}   /* end fm10000DbgWriteRegisterV3 */




/*****************************************************************************/
/** fm10000DbgWriteRegisterV2
 * \ingroup intDiagReg
 *
 * \desc            Write the contents of a switch register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       wordOffset is used for all multi-word registers and 
 *                  ignored for single-word (and single-word indexed) 
 *                  registers.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  indexed, this argument specifies the left index as the
 *                  register is described in the FocalPoint datasheet. For
 *                  some indexed registers, this index may represent a port
 *                  number. In those cases, this argument should specify the
 *                  logical port number.
 *
 * \param[in]       indexB is used to index into doubly indexed registers,
 *                  and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       regName is the name of the register to write.
 *
 * \param[in]       value is the register value to be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regName is not recognized.
 * \return          FM_ERR_INVALID_ARGUMENT if wordOffset is invalid for the
 *                  specified register.
 * \return          FM_ERR_INVALID_INDEX if indexA or indexB is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
fm_status fm10000DbgWriteRegisterV2(fm_int    sw, 
                                    fm_int    wordOffset, 
                                    fm_int    indexA, 
                                    fm_int    indexB, 
                                    fm_text   regName, 
                                    fm_uint32 value)
{

    return fm10000DbgWriteRegisterV3(sw, 
                                     wordOffset, 
                                     indexA, 
                                     indexB, 
                                     0,
                                     regName, 
                                     value);

}   /* end fm10000DbgWriteRegisterV2 */




/*****************************************************************************/
/** fm10000DbgWriteRegister
 * \ingroup intDiagReg
 *
 * \desc            Write the contents of a switch register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number (for port registers) or
 *                  the index (for non-port indexed registers).
 *
 * \param[in]       regName is the name of the port to be written.
 *
 * \param[in]       val is the contents of the register to be written.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgWriteRegister(fm_int sw, fm_int port, fm_text regName, fm_int val)
{
    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG,
                 "sw=%d, port=%d, regName=%s, val=%d\n",
                 sw,
                 port,
                 regName,
                 val);

    fm10000DbgWriteRegisterV3(sw, 
							  0, 
							  port, 
							  0, 
							  0, 
							  regName, 
							  val);

    FM_LOG_EXIT_VOID(FM_LOG_CAT_DEBUG);

}   /* end fm10000DbgWriteRegister */



/*****************************************************************************/
/** fm10000DbgWriteRegisterField
 * \ingroup intDiagReg
 *
 * \desc            Write the contents of a switch register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  indexed, this argument specifies the left index as the
 *                  register is described in the FocalPoint datasheet. For
 *                  some indexed registers, this index may represent a port
 *                  number. In those cases, this argument should specify the
 *                  logical port number.
 *
 * \param[in]       indexB is used to index into doubly indexed registers
 *                  and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers.
 *
 * \param[in]       regName is the name of the register to write.
 *
 * \param[in]       fieldName is the field name of the register to write.
 *
 * \param[in]       value is the field value to be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNKNOWN_REGISTER if regName is not recognized.
 * \return          FM_ERR_INVALID_ARGUMENT if wordOffset is invalid for the
 *                  specified register.
 * \return          FM_ERR_INVALID_INDEX if indexA, indexB or indexC is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
fm_status fm10000DbgWriteRegisterField(fm_int    sw, 
									   fm_int    indexA, 
									   fm_int    indexB, 
									   fm_int    indexC, 
									   fm_text   regName, 
									   fm_text   fieldName, 
									   fm_uint64 value)
{
    const fm10000DbgFulcrumRegister *pReg;
    fm_status                       err = FM_ERR_UNKNOWN_REGISTER;
    fm_bool                         indexByPort = FALSE;
    fm_char                         registerName[MAX_REGISTER_NAME_LENGTH];
    fm_uint                         offset;
    fm_uint                         offsetA[MAX_WORD_PER_REG];
    fm_int                          physIndex0;
    fm_int                          physIndex1;
    fm_int                          physIndex2;
    fm_int                          i, j, k, m;
    fm_int                          entryCnt;
    fm_int                          matchCnt;
    fm_int                          idxSel;
    fm_int                          pep;
    fm_int                          pepStart, pepEnd;

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG,
                 "sw=%d, indexA=%d, indexB=%d, indexC=%d, "
                 "regName=%s, fieldName=%s, value=0x%llX\n",
                 sw,
                 indexA,
                 indexB,
                 indexC,
                 regName,
                 fieldName,
                 value);

    ParseRegName(regName, TRUE, registerName, &indexByPort);

    /**************************************************
     * Search the table for the specified register name.
     **************************************************/

    pReg = fm10000RegisterTable;
    entryCnt = 0;
    matchCnt = 0;
    idxSel =  0;

    for (i = 0 ; pReg[i].regname != NULL ; i++)
    {
        if (IS_REG_PCIE_VF(pReg[i].regname))
        {
            continue;
        }
        if (strcasecmp(pReg[i].regname + REG_NAME_PREFIX_LEN, registerName) == 0)
        {
            /* Exact match */
            matchCnt = 1;
            idxSel = i;
            break;
        }
        if (strcasestr(pReg[i].regname + REG_NAME_PREFIX_LEN, registerName) != NULL)
        {
            matchCnt++;
            idxSel = i;
        }
    }

    if (matchCnt == 0)
    {
        FM_LOG_PRINT("No register matches to the given name: %s\n", regName);
        FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_UNKNOWN_REGISTER);
    }
    else if (matchCnt > 1)
    {
        FM_LOG_PRINT("Multiple (%d) registers matches to the given name: %s\n",
					 matchCnt,
					 regName);
        FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_UNKNOWN_REGISTER);
    }
    else if (IS_REG_PCIE_VF(regName))
    {
        FM_LOG_PRINT("%s not visible in global register space.\n",
                     regName);
        FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_UNKNOWN_REGISTER);
    }
    else
    {
        pReg = &fm10000RegisterTable[idxSel];

        if (pReg->wordcount > MAX_WORD_PER_REG)
        {
            FM_LOG_PRINT("Size (%d) of register is over buffer size of %d\n",
                         pReg->wordcount,
                         MAX_WORD_PER_REG);
            FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_FAIL);
        }

        /**************************************************
         * Loop through all the indexes for this register. 
         **************************************************/
        if (indexByPort)
        {
            err = GetRegOffsetIdx(sw,
                                  pReg,
                                  indexA,
                                  indexB,
                                  indexC,
                                  indexByPort,
                                  0,
                                  FALSE,
                                  offsetA,
                                  &physIndex0,
                                  &physIndex1,
                                  &physIndex2,
                                  &pep);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);

            FM_LOG_DEBUG(FM_LOG_CAT_DEBUG,
                         "UpdateField_P offset0=0x%x physIdx=%d,%d,%d\n",
                         offsetA[0],
                         physIndex0,
                         physIndex1,
                         physIndex2);

            err = UpdateRegisterField(sw,
                                      pReg,
                                      fieldName,
                                      offsetA[0],
                                      value);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);

            entryCnt++;
        }
        else
        {
            if (IS_REG_PCIE_INDEX(pReg->regname))
            {
                pep = indexA;
                if (pep < 0 || pep > FM10000_MAX_PEP)
                {
                    pepStart = 0;
                    pepEnd = FM10000_MAX_PEP;
                }
                else
                {
                    pepStart = pep;
                    pepEnd = pep;
                }
                indexA = indexB;
                indexB = indexC;
                indexC = -1;
            }
            else
            {
                pepStart = 0;
                pepEnd = 0;
            }

            for (pep = pepStart ; pep <= pepEnd; pep++)
            {
                for (m = pReg->indexMin2 ; m <= pReg->indexMax2 ; m++)
                {
                    for (k = pReg->indexMin1 ; k <= pReg->indexMax1 ; k++)
                    {
                        for (j = pReg->indexMin0 ; j <= pReg->indexMax0 ; j++)
                        {
                            /* FALSE: Don't do entry that is out of range, unless
                             * the user specifies it by passing in a negative value.
                             * So we don't modify the registers without the user
                             * knowingly specifies it.
                             */
                            if (IsEntrySelected(sw, pReg, FALSE, FALSE,
                                                indexA, indexB, indexC,
                                                j, k, m,
                                                &physIndex0, &physIndex1, &physIndex2))
                            {

                                offset = (physIndex0 * pReg->indexStep0)
                                         + (physIndex1 * pReg->indexStep1)
                                         + (physIndex2 * pReg->indexStep2)
                                         + pReg->regAddr
                                         + pep * FM10000_PCIE_PF_SIZE;

                                FM_LOG_DEBUG(FM_LOG_CAT_DEBUG,
                                             "UpdateField offset=0x%x physIdx=%d,%d,%d\n",
                                             offset,
                                             physIndex0,
                                             physIndex1,
                                             physIndex2);

                                err = UpdateRegisterField(sw,
                                                          pReg,
                                                          fieldName,
                                                          offset,
                                                          value);
                                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);

                                entryCnt++;
                            }
                        }   /* end for (j = pReg->indexMin0 ; j <= pReg->indexMax0 ; j++) */
                    }   /* end for (k = pReg->indexMin1 ; k <= pReg->indexMax1 ; k++) */
                }   /* end for (m = pReg->indexMin2 ; m <= pReg->indexMax2 ; m++) */
            } /* end for (pep = pepStart ; pep < pepEnd; pep++) */

        }

        if (entryCnt == 0)
        {
            FM_LOG_PRINT("No entry is modified. Index is out of range.\n");
        }
        else if (entryCnt > 1)
        {
            FM_LOG_PRINT("%d entries modified\n", entryCnt);
        }

        FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_OK);
    }

    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);

}   /* end fm10000DbgWriteRegisterField */


#if 0

/*****************************************************************************/
/** fm10000DbgGetRegName
 * \ingroup intDiagReg
 *
 * \desc            Given an address return the name of the register.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       addr is the register address.
 *
 * \param[out]      name is pointer to the string buffer where the name will be
 *                  copied to. If not found, then "NotListed" is returned.
 *
 * \param[in]       len is the length of the buffer.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgGetRegName(fm_int sw, fm_uint32 addr, fm_char *name, fm_int len)
{
    const fm10000DbgFulcrumRegister *pReg;

    FM_NOT_USED(sw);

    pReg = fm10000RegisterTable;

    while (pReg->regname != NULL)
    {
        if (pReg->flags & REG_FLAG_END_OF_REGS)
        {
            break;
        }

        if (addr>=pReg->regAddr)
        {
            fm_int i;
            fm_int j;
            fm_int k;

            if (pReg->accessMethod == SCALAR) 
            {
                if (addr == pReg->regAddr)
                {
                    FM_SNPRINTF_S(name,len,"%s",pReg->regname);
                    return;
                }
            }
            else if (pReg->accessMethod == MULTIWRD) 
            {
                if (addr<pReg->regAddr+pReg->wordcount) 
                {
                    FM_SNPRINTF_S(name,
                                  len,
                                  "%s_w%u",
                                  pReg->regname,
                                  (addr-pReg->regAddr) % pReg->wordcount);
                   return;
                }
            }
            else if (pReg->accessMethod == INDEXED) 
            {
                for (i = pReg->indexMin0 ; i <= pReg->indexMax0 ; i++)
                {
                    if (addr == (pReg->regAddr+(pReg->indexStep0*i))) 
                    {
                        FM_SNPRINTF_S(name,len,"%s[%u]",pReg->regname,i);
                        return;
                    }
                }
            } 
            else if (pReg->accessMethod == MWINDEX) 
            {
                for (i = pReg->indexMin0 ; i <= pReg->indexMax0 ; i++)
                {
                    if ( (addr>=(pReg->regAddr+(pReg->indexStep0*i))) && 
                         (addr<(pReg->regAddr+(pReg->indexStep0*i)+pReg->wordcount)) ) 
                    {
                        FM_SNPRINTF_S( name,
                                       len,
                                       "%s_w%u[%u]",
                                       pReg->regname,
                                       ( addr-pReg->regAddr +(pReg->indexStep0 * i) )
                                       % pReg->wordcount,
                                       i);
                        return;
                    }
                }
            }
            else if (pReg->accessMethod == DBLINDEX) 
            {
                for (j = pReg->indexMin1 ; j <= pReg->indexMax1 ; j++)
                {
                    for (i = pReg->indexMin0 ; i <= pReg->indexMax0 ; i++)
                    {
                        if (addr == (pReg->regAddr+(pReg->indexStep0*i)+(pReg->indexStep1*j))) 
                        {
                            FM_SNPRINTF_S(name,len,"%s[%u][%u]",pReg->regname,j,i);
                            return;
                        }
                    }
                }
            } 
            else if (pReg->accessMethod == TPLINDEX) 
            {
                for (k = pReg->indexMin2 ; k <= pReg->indexMax2 ; k++)
                {
                    for (j = pReg->indexMin1 ; j <= pReg->indexMax1 ; j++)
                    {
                        for (i = pReg->indexMin0 ; i <= pReg->indexMax0 ; i++)
                        {
                            if (addr == ( pReg->regAddr +
                                          (pReg->indexStep0 * i) +
                                          (pReg->indexStep1 * j) +
                                          (pReg->indexStep2 * k) ) ) 
                            {
                                FM_SNPRINTF_S(name,
                                              len,
                                              "%s[%u][%u][%u]",
                                              pReg->regname,
                                              k,
                                              j,
                                              i);
                                return;
                            }
                        }
                    }
                }
            } 
        }
        ++pReg;
    }
    fmStringCopy(name,"NotListed",len);
    return;

}   /* end fm10000DbgGetRegName */
#endif




/*****************************************************************************/
/** fm10000DbgGetRegisterName
 * \ingroup intDiagReg
 *
 * \desc            Given a register address, return the register name and
 *                  information about the register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regId is the index into the register table.
 *
 * \param[in]       regAddress is the register address to look up.
 *
 * \param[out]      regName is a pointer to caller-allocated memory where
 *                  this function may write the register name.
 *
 * \param[in]       regNameLength is the length of regName.
 *
 * \param[in,out]   isPort points to caller-allocated storage where this
 *                  function will write TRUE if the register is a port
 *                  register. If NULL on entry, this argument will be
 *                  ignored by the function.
 *
 * \param[in,out]   index0Ptr points to caller-allocated storage where this
 *                  function will write the offset within the register array
 *                  for indexed registers.  If the register is a port register,
 *                  this will be set to the logical port number, not the
 *                  physical port. If NULL on entry, this argument will be
 *                  ignored by the function.
 *
 * \param[in,out]   index1Ptr points to caller-allocated storage where this
 *                  function will write the secondary offset within the
 *                  register array for doubly or triply indexed registers.
 *                  If the register is a port register, and the secondary index
 *                  represents the port, this will be set to the logical port
 *                  number, not the physical port. For non-doubly or triply 
 *                  indexed registers, this will be set to zero. If NULL on 
 *                  entry, this argument will be ignored by the function.
 *
 * \param[in,out]   index2Ptr points to caller-allocated storage where this
 *                  function will write the tertiary offset within the
 *                  register array for triply indexed registers.  For 
 *                  non-triply indexed registers, this will be set to zero. 
 *                  If NULL on entry, this argument will be ignored by the 
 *                  function.
 *
 * \param[in]       logicalPorts indicates whether port registers will have
 *                  the port number converted from physical to logical
 *                  before it is added to the register name.
 *
 * \param[in]       partialLongRegs indicates if the name should be built
 *                  with the expectation that registers longer than 1 word
 *                  will be dumped as separate words. This causes a subscript
 *                  for the word offset to be added to the register name.
 *                  FALSE causes the register name to be generated without
 *                  that subscript.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgGetRegisterName(fm_int   sw,
                               fm_int   regId,
                               fm_uint  regAddress,
                               fm_text  regName,
                               fm_uint  regNameLength,
                               fm_bool *isPort,
                               fm_int * index0Ptr,
                               fm_int * index1Ptr,
                               fm_int * index2Ptr,
                               fm_bool  logicalPorts,
                               fm_bool  partialLongRegs)
{
    const fm10000DbgFulcrumRegister *regEntry;

    fm_status   status;
    char        tempBuf[200];

    /* Difference between accessed address and register base address. */
    fm_int      offset;

    /* Index into array register, calculated from offset and typically a
     * port number. */
    fm_int      index0 = 0;

    /* The secondary index for doubly and triply indexed registers. */
    fm_int      index1 = 0;

    /* The tertiary index for triply indexed registers. */
    fm_int      index2 = 0;

    fm_int      pepIdx = -1;

    /* Identifies the 0-based word number in a multi-word register. */
    fm_int      word;

    /* Associated logical port if logicalPorts is set */
    fm_int      port;
    fm_int      physPort;
    fm_int      channel;

    regEntry = &fm10000RegisterTable[regId];

    /* Calculate the offset at which the target appears from the base address. */
    offset = regAddress - regEntry->regAddr;

    /**************************************************
     * Format the register name.
     **************************************************/

    /* Name of register. */
    fmStringCopy(regName, regEntry->regname, regNameLength);

    /* If indexed, add index(es). */

    port = -1;

    if (IS_REG_PCIE_INDEX(regEntry->regname))
    {
        pepIdx = offset / FM10000_PCIE_PF_SIZE;
        offset = offset - (pepIdx * FM10000_PCIE_PF_SIZE);
        status = fm10000MapPepToLogicalPort(sw, pepIdx, &port);
        if (status)
        {
            port = -1;
        }
    }

    switch (regEntry->accessMethod)
    {
        case INDEXED:
        case MWINDEX:
            index0 = offset / regEntry->indexStep0;
            word   = offset % regEntry->indexStep0;

            if (regEntry->flags & REG_FLAG_PER_PORT)
            {
                index0 &= INDEX_AS_PORT_MASK;

                if (logicalPorts)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               index0,
                                               &port);
                }
            }

            if (logicalPorts && regEntry->flags & REG_FLAG_INDEX0_PER_EPL)
            {
                if (MapEplChannelToPhysicalPort(sw,
                                                index0,
                                                0,
                                                &physPort) == FM_OK)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               physPort,
                                               &port);
                }
            }

            /* Special case */
            if (IS_REG_INDEX_BY_PCIE(regName))
            {
                status = fm10000MapPepToLogicalPort(sw, index0, &port);
                if (status)
                {
                    port = -1;
                }       
            }

            FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), "[%d]", index0);
            fmStringAppend(regName, tempBuf, regNameLength);
            break;

        case DBLINDEX:
        case MWDBLIDX:

            /**************************************************
             * Note: It cannot be said that index0 represents
             * the row and index1 the column or vice versa.
             * The mapping of index to row or column varies
             * from register to register. The index with the
             * greater step size must be taken as the row index
             * and the other as the column.
             **************************************************/

            if (regEntry->indexStep0 > regEntry->indexStep1)
            {
                index0 = offset / regEntry->indexStep0;
                index1 = (offset % regEntry->indexStep0) / regEntry->indexStep1;
                word   = (offset % regEntry->indexStep0) % regEntry->indexStep1;
            }
            else
            {
                index1 = offset / regEntry->indexStep1;
                index0 = (offset % regEntry->indexStep1) / regEntry->indexStep0;
                word   = (offset % regEntry->indexStep1) % regEntry->indexStep0;
            }

            /**************************************************
             * If either index0 or index1 is a port number,
             * convert it from physical to logical for
             * display.
             **************************************************/

            if (regEntry->flags & REG_FLAG_INDEX0_PER_PORT)
            {
                index0 &= INDEX_AS_PORT_MASK;

                if (logicalPorts)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               index0,
                                               &port);
                }
            }

            if (regEntry->flags & REG_FLAG_INDEX1_PER_PORT)
            {
                index1 &= INDEX_AS_PORT_MASK;

                if (logicalPorts)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               index1,
                                               &port);
                }
            }

            if (logicalPorts && regEntry->flags & REG_FLAG_INDEX0_CHAN_1_EPL)
            {
                channel = index1;

                status = MapEplChannelToPhysicalPort(sw,
                                                     index0,
                                                     channel,
                                                     &physPort);

                if (status == FM_OK)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               physPort,
                                               &port);
                }
            }

            /**************************************************
             * Display the two indexes in the order shown on
             * the datasheet.
             **************************************************/

            FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), "[%d][%d]", index0, index1);
            fmStringAppend(regName, tempBuf, regNameLength);
            break;

        case TPLINDEX:
        case MWTPLIDX:
            /**************************************************
             * Note: It cannot be taken for granted which
             * indexX corresponds to which dimension of the
             * array. The mapping of index to dimension could
             * vary from register to register. The index with the
             * greatest step size must be taken as the "outer"
             * dimension, the index with the smallest step size
             * is the "inner" dimension and the left over index
             * is the "center" dimension.
             **************************************************/

            if (regEntry->indexStep0 > regEntry->indexStep1)
            {
                if (regEntry->indexStep1 > regEntry->indexStep2)
                {
                    /* 0 > 1 > 2 */
                    index0 = offset / regEntry->indexStep0;
                    index1 = (offset % regEntry->indexStep0) / regEntry->indexStep1;
                    index2 = ( (offset % regEntry->indexStep0) % regEntry->indexStep1) / regEntry->indexStep2;
                    word   = ( (offset % regEntry->indexStep0) % regEntry->indexStep1) % regEntry->indexStep2;
                }
                else
                {
                    if (regEntry->indexStep0 > regEntry->indexStep2)
                    {
                        /* 0 > 2 > 1 */ 
                        index0 = offset / regEntry->indexStep0;
                        index2 = (offset % regEntry->indexStep0) / regEntry->indexStep2;
                        index1 = ( (offset % regEntry->indexStep0) % regEntry->indexStep2) / regEntry->indexStep1;
                        word   = ( (offset % regEntry->indexStep0) % regEntry->indexStep2) % regEntry->indexStep1;
                    }
                    else
                    {
                        /* 2 > 0 > 1 */
                        index2 = offset / regEntry->indexStep2;
                        index0 = (offset % regEntry->indexStep2) / regEntry->indexStep0;
                        index1 = ( (offset % regEntry->indexStep2) % regEntry->indexStep0) / regEntry->indexStep1;
                        word   = ( (offset % regEntry->indexStep2) % regEntry->indexStep0) % regEntry->indexStep1;
                    }

                }
            }
            else if (regEntry->indexStep0 > regEntry->indexStep2)
            {
                /* 1 > 0 > 2 */
                index1 = offset / regEntry->indexStep1;
                index0 = (offset % regEntry->indexStep1) / regEntry->indexStep0;
                index2 = ( (offset % regEntry->indexStep1) % regEntry->indexStep0) / regEntry->indexStep2;
                word   = ( (offset % regEntry->indexStep1) % regEntry->indexStep0) % regEntry->indexStep2;
            }
            else if (regEntry->indexStep1 > regEntry->indexStep2)
            {
                /* 1 > 2 > 0 */
                index1 = offset / regEntry->indexStep1;
                index2 = (offset % regEntry->indexStep1) / regEntry->indexStep2;
                index0 = ( (offset % regEntry->indexStep1) % regEntry->indexStep2) / regEntry->indexStep0;
                word   = ( (offset % regEntry->indexStep1) % regEntry->indexStep2) % regEntry->indexStep0;
            }
            else
            {
                /* 2 > 1 > 0 */
                index2 = offset / regEntry->indexStep2;
                index1 = (offset % regEntry->indexStep2) / regEntry->indexStep1;
                index0 = ( (offset % regEntry->indexStep2) % regEntry->indexStep1) / regEntry->indexStep0;
                word   = ( (offset % regEntry->indexStep2) % regEntry->indexStep1) % regEntry->indexStep0;
            }

            /**************************************************
             * If either index0 or index1 is a port number,
             * convert it from physical to logical for
             * display.
             **************************************************/

            if (regEntry->flags & REG_FLAG_INDEX0_PER_PORT)
            {
                index0 &= INDEX_AS_PORT_MASK;

                if (logicalPorts)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               index0,
                                               &port);
                }

            }

            if (regEntry->flags & REG_FLAG_INDEX1_PER_PORT)
            {
                index1 &= INDEX_AS_PORT_MASK;

                if (logicalPorts)
                {
                    fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                               index1,
                                               &port);
                }

            }

            /**************************************************
             * Display the three indexes in the order shown on
             * the datasheet.
             **************************************************/

            FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), "[%d][%d][%d]", index0, index1, index2);
            fmStringAppend(regName, tempBuf, regNameLength);
            break;

        default:
            word   = offset;
            break;

    }   /* end switch (regEntry->accessMethod) */

    /**************************************************
     * Finish up.
     **************************************************/

    if (partialLongRegs && regEntry->wordcount > 1)
    {
        FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), "[%d]", word);
        fmStringAppend(regName, tempBuf, regNameLength);
    }

    if (pepIdx >= 0)
    {
        FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), ".PEP(%d)", pepIdx);
        fmStringAppend(regName, tempBuf, regNameLength);
    }

    if (port >= 0)
    {
        if (IS_EPL_QUAD_INDEX(regEntry))
        {
            /* Assume logical port is mapped continous in one EPL block */
            FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), ".P(%d..%d)", port, port + 3);
        }
        else
        {
            FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), ".P(%d)", port);
        }

        fmStringAppend(regName, tempBuf, regNameLength);
    }

    if (isPort)
    {
        *isPort = (regEntry->flags & REG_FLAG_PER_PORT);
    }

    if (index0Ptr)
    {
        *index0Ptr = index0;
    }

    if (index1Ptr)
    {
        *index1Ptr = index1;
    }

    if (index2Ptr)
    {
        *index2Ptr = index2;
    }

}   /* end fm10000DbgGetRegisterName */




/*****************************************************************************/
/** fm10000DbgReadRegister
 * \ingroup intDiag
 *
 * \desc            Reads a register by name instead of by address.  The given
 *                  indices do not include the word index for > 32-bit registers.
 *                  These are handled internally and it is assumed that the
 *                  appropriately sized data is passed into the argument.  All
 *                  32-bit words are read back into 64-bit words for convenience.
 *
 * \param[in]       sw identifies the switch for which to read the register.
 *
 * \param[in]       firstIndex is the first index for the register.
 *
 * \param[in]       secondIndex is the second index for the register.
 *
 * \param[in]       registerName is the register name string.
 *
 * \param[out]      value points to caller allocated storage where the read values
 *                  will be written.   It is assumed that there are enough entries
 *                  in this storage space for the given register.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
void fm10000DbgReadRegister(fm_int  sw,
                            fm_int  firstIndex,
                            fm_int  secondIndex,
                            fm_text registerName,
                            void *  value)
{
    const fm10000DbgFulcrumRegister *pReg;
    fm_int                          tblIndex;

    pReg     = fm10000RegisterTable;
    tblIndex = 0;

    while (pReg->regname != NULL)
    {
        if (strcmp(pReg->regname, registerName) == 0)
        {
            break;
        }

        ++pReg;
        ++tblIndex;
    }

    if (pReg->regname == NULL)
    {
        return;
    }

    fm10000DbgDumpChipRegister(sw,
                               firstIndex,
                               secondIndex,
                               0,
                               0,
                               tblIndex,
                               FALSE,
                               value,
                               fm10000DbgReadRegisterCallback);


}   /* end fm10000DbgReadRegister */




/*****************************************************************************/
/* fm10000DbgTakeChipSnapshot
 * \ingroup intDiagReg
 *
 * \desc            Record a snapshot of the switch's configuration (the
 *                  register file).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       snapshot is an arbitrary snapshot number (0 - 31) by
 *                  which to recall the snapshot later.  The snapshot number
 *                  is global across all switches in the system.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgTakeChipSnapshot(fm_int                sw,
                                fmDbgFulcrumSnapshot *pSnapshot,
                                fm_regDumpCallback    callback)
{
    const fm10000DbgFulcrumRegister *pRegister;
    fm_int                          indexA;
    fm_int                          indexB = 0;
    fm_int                          indexC = 0;
    fm_int                          regId;
    fm_status                       err;

    if (invalidIndexEncountered == NULL)
    {
        fm_int regCount;

        regCount  = 0;
        pRegister = fm10000RegisterTable;

        while (pRegister->regname != NULL)
        {
            regCount++;
            pRegister++;
        }

        invalidIndexEncountered = fmAlloc( regCount * sizeof(fm_bool) );

        if (invalidIndexEncountered == NULL)
        {
            FM_LOG_ERROR(FM_LOG_CAT_DEBUG,
                         "Unable to allocate memory for invalid index table, "
                         "regCount=%d\n",
                         regCount);
            return;
        }
    }

    regId               = 0;
    pSnapshot->regCount = 0;
    pRegister           = fm10000RegisterTable;

    while (pRegister->regname != NULL)
    {
        if ( strcmp(pRegister->regname, "END_OF_REGISTERS") == 0 )
        {
            pRegister++;
            regId++;
            continue;
        }

        switch (pRegister->accessMethod)
        {
            case ALL4PORT:
            case GROUPREG:
            case ALLCONFG:
            case SPECIAL:
                break;

            case DBLINDEX:
            case MWDBLIDX:
                for (indexB = pRegister->indexMin1 ;
                     indexB <= pRegister->indexMax1 ;
                     indexB++)
                {
                    for (indexA = pRegister->indexMin0 ;
                         indexA <= pRegister->indexMax0 ;
                         indexA++)
                    {
                        err = fm10000DbgDumpChipRegister(sw,
                                                         indexB,
                                                         indexA,
                                                         0,
                                                         0,
                                                         regId,
                                                         FALSE,
                                                         pSnapshot,
                                                         callback);
                        if (err != FM_OK)
                        {
                            if ( (err == FM_ERR_INVALID_INDEX) &&
                                 !invalidIndexEncountered[regId] )
                            {
                                invalidIndexEncountered[regId] = TRUE;

                                FM_LOG_WARNING(FM_LOG_CAT_DEBUG,
                                               "Invalid index error for "
                                               "register %s, indexB=%d, "
                                               "indexA=%d\n",
                                               pRegister->regname,
                                               indexB,
                                               indexA);
                            }
                            else
                            {
                                FM_LOG_WARNING(FM_LOG_CAT_DEBUG,
                                               "Snapshot is incomplete: snapshot "
                                               "register table overflowed at %d "
                                               "registers, err=%d (%s)\n",
                                               pSnapshot->regCount,
                                               err,
                                               fmErrorMsg(err) );
                            }
                            break;
                        }
                    }
                }

                break;

            case TPLINDEX:
            case MWTPLIDX:

                for (indexC = pRegister->indexMin2 ;
                     indexB <= pRegister->indexMax2 ;
                     indexC++)
                {
                    for (indexB = pRegister->indexMin1 ;
                         indexB <= pRegister->indexMax1 ;
                         indexB++)
                    {
                        for (indexA = pRegister->indexMin0 ;
                             indexA <= pRegister->indexMax0 ;
                             indexA++)
                        {
                            err = fm10000DbgDumpChipRegister(sw,
                                                             indexC,
                                                             indexB,
                                                             indexA,
                                                             0,
                                                             regId,
                                                             FALSE,
                                                             pSnapshot,
                                                             callback);
                            if (err != FM_OK)
                            {
                                if ( (err == FM_ERR_INVALID_INDEX) &&
                                     !invalidIndexEncountered[regId] )
                                {
                                    invalidIndexEncountered[regId] = TRUE;

                                    FM_LOG_WARNING(FM_LOG_CAT_DEBUG,
                                                   "Invalid index error for "
                                                   "register %s, indexC=%d, "
                                                   "indexB=%d, indexA=%d\n",
                                                   pRegister->regname,
                                                   indexC,
                                                   indexB,
                                                   indexA);
                                }
                                else
                                {
                                    FM_LOG_WARNING(FM_LOG_CAT_DEBUG,
                                                   "Snapshot is incomplete: snapshot "
                                                   "register table overflowed at %d "
                                                   "registers, err=%d (%s)\n",
                                                   pSnapshot->regCount,
                                                   err,
                                                   fmErrorMsg(err) );
                                }
                                break;
                            }
                        }
                    }
                }

                break;

            default:

                for (indexA = pRegister->indexMin0 ;
                     indexA <= pRegister->indexMax0 ;
                     indexA++)
                {
                    err = fm10000DbgDumpChipRegister(sw,
                                                     indexA,
                                                     0,
                                                     0,
                                                     0,
                                                     regId,
                                                     FALSE,
                                                     pSnapshot,
                                                     callback);
                    if (err != FM_OK)
                    {
                        if ( (err == FM_ERR_INVALID_INDEX) &&
                             !invalidIndexEncountered[regId] )
                        {
                            invalidIndexEncountered[regId] = TRUE;

                            FM_LOG_WARNING(FM_LOG_CAT_DEBUG,
                                           "Invalid index error for "
                                           "register %s, indexA=%d, ",
                                           pRegister->regname,
                                           indexA);
                        }
                        else
                        {
                            FM_LOG_WARNING(FM_LOG_CAT_DEBUG,
                                           "Snapshot is incomplete: snapshot "
                                           "register table overflowed at %d "
                                           "registers, err=%d (%s)\n",
                                           pSnapshot->regCount,
                                           err,
                                           fmErrorMsg(err) );
                        }
                        break;
                    }
                }

                break;

        }   /* end switch (pRegister->accessMethod) */

        regId++;
        pRegister++;
    }

    FM_LOG_PRINT("Snapshot required %d entries\n", pSnapshot->regCount);

}   /* end fm10000DbgTakeChipSnapshot */




/*****************************************************************************/
/** fm10000DbgGetRegInfo
 * \ingroup intDiag
 *
 * \desc            Reads the register information by indexing the table using
 *                  the register name.
 *
 * \param[in]       registerName is the register name string.
 * 
 * \param[out]      registerAddr points to caller-allocated storage where this
 *                  function will write the register base address of the
 *                  register refered by registerName.
 * 
 * \param[out]      wordCnt points to caller-allocated storage where this
 *                  function will write the word count of this register.
 * 
 * \param[out]      idxMax0 points to caller-allocated storage where this
 *                  function will write the maximum value of the first index.
 * 
 * \param[out]      idxMax1 points to caller-allocated storage where this
 *                  function will write the maximum value of the second index.
 * 
 * \param[out]      idxMax2 points to caller-allocated storage where this
 *                  function will write the maximum value of the third index.
 * 
 * \param[out]      idxStep0 points to caller-allocated storage where this
 *                  function will write the steps between value of the first
 *                  index.
 * 
 * \param[out]      idxStep1 points to caller-allocated storage where this
 *                  function will write the steps between value of the second
 *                  index.
 * 
 * \param[out]      idxStep2 points to caller-allocated storage where this
 *                  function will write the steps between value of the third
 *                  index.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000DbgGetRegInfo(fm_text    registerName,
                               fm_uint32 *registerAddr,
                               fm_int *   wordCnt,
                               fm_int *   idxMax0,
                               fm_int *   idxMax1,
                               fm_int *   idxMax2,
                               fm_int *   idxStep0,
                               fm_int *   idxStep1,
                               fm_int *   idxStep2)
{
    const fm10000DbgFulcrumRegister *pReg;
    fm_int                          tblIndex;

    pReg     = fm10000RegisterTable;
    tblIndex = 0;

    while (pReg->regname != NULL)
    {
        if (strcmp(pReg->regname, registerName) == 0)
        {
            break;
        }

        ++pReg;
        ++tblIndex;
    }

    if (pReg->regname == NULL)
    {
        return FM_FAIL;
    }

    *registerAddr = pReg->regAddr;
    *wordCnt      = pReg->wordcount;
    *idxMax0      = pReg->indexMax0;
    *idxMax1      = pReg->indexMax1;
    *idxMax2      = pReg->indexMax2;
    *idxStep0     = pReg->indexStep0;
    *idxStep1     = pReg->indexStep1;
    *idxStep2     = pReg->indexStep2;

    return FM_OK;

}   /* end fm10000DbgGetRegInfo */





/*****************************************************************************/
/** fm10000DbgDumpRegField
 * \ingroup intDiag
 *
 * \desc            Dump out each field of the given register.
 *
 * \param[in]       registerName is the register name string.
 *
 * \param[out]      value points to caller-supplied storage where the values
 *                  are stored.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpRegField(fm_text registerName, fm_uint32 *value)
{
    {
        /* Dump using auto-generated field information. */
        fm10000DbgDumpRegisterFieldInt(registerName, value);
    }

    return FM_OK;

}   /* end fm10000DbgDumpRegField */




/*****************************************************************************/
/** fm10000DbgSetRegField
 * \ingroup intDiag
 *
 * \desc            Update the specified field in the given register.
 *
 * \param[in]       registerName is the register name string.
 *
 * \param[in]       fieldName is the fieldname of the register. The name can
 *                  be a unique substring in the register field name.
 *
 * \param[out]      regValue points to caller allocated storage where the
 *                  specified register value is stored and modified.
 *
 * \param[out]      fieldValue is the value to set the specified field.
 *                  Support max field width up to 64 bits.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000DbgSetRegField(fm_text    registerName,
                                fm_text    fieldName,
                                fm_uint32 *regValue,
                                fm_uint64  fieldValue)
{
    fm_int                 i;
    fm_int                 j;
    fm_uint64              val64;
    fm_uint64              mask;
    fm_int                 end;
    fm_int                 word_start;
    fm_int                 word_end;
    const fmRegisterField *fields;
    fm_int                 idxSel   = -1;
    fm_int                 matchCnt = 0;


    /* Strip off the FMxxxx_ prefix */
    registerName = StripFmPrefix(registerName);
    fields       = fm10000DbgGetRegisterFields(registerName);

    if (fields == NULL)
    {
        return FM_ERR_UNKNOWN_REGISTER;
    }

    for (i = 0 ; ; i++)
    {
        if (fields[i].name == NULL)
        {
            if (matchCnt == 1)
            {
                break;
            }
            else if (matchCnt > 1)
            {
                FM_LOG_PRINT("%s matches %d entries in register field name list:\n",
                             fieldName, matchCnt);

                for (j = 0 ; ; j++)
                {
                    if (fields[j].name == NULL)
                    {
                        return FM_ERR_INVALID_ARGUMENT;  /* end of list */
                    }

                    if (strcasestr(fields[j].name, fieldName) != NULL)
                    {
                        FM_LOG_PRINT("    %s\n", fields[j].name);
                    }
                }
            }
            else
            {
                FM_LOG_PRINT("%s is not found in register field name list:\n",
                             fieldName);

                for (j = 0 ; ; j++)
                {
                    if (fields[j].name == NULL)
                    {
                        return FM_ERR_NOT_FOUND;  /* end of list */
                    }

                    FM_LOG_PRINT("    %s\n", fields[j].name);
                }
            }
        }

        if (strcasestr(fields[i].name, fieldName) != NULL)
        {
            matchCnt++;
            idxSel = i;

            /* Exact match */
            if (strcasecmp(fields[i].name, fieldName) == 0)
            {
                break;
            }
        }
    }

    /* fieldName matches a single entry */
    if (idxSel >= 0)
    {
        i = idxSel;

        if (fields[i].size > 64)
        {
            FM_LOG_PRINT("%s: No support for field size (%d) greater than 64 bits.\n",
                         fields[i].name, fields[i].size);
            return FM_ERR_UNSUPPORTED;
        }

        end        = fields[i].start + fields[i].size;
        word_start = fields[i].start / 32;
        word_end   = (end - 1) / 32;
        if (fields[i].size == 64)
        {
            mask = FM_LITERAL_U64(0xFFFFFFFFFFFFFFFF);
        }
        else
        {
            mask = ( (FM_LITERAL_U64(1) << fields[i].size ) - FM_LITERAL_U64(1) )
                     << (fields[i].start - word_start * 32);
        }
        fieldValue = ( fieldValue << (fields[i].start - word_start * 32) ) & mask;

        if (word_start == word_end)
        {
            regValue[word_start] = (regValue[word_start] & ~mask) | fieldValue;
        }
        else if ( (word_end - word_start) == 1 )
        {
            val64                = regValue[word_end];
            val64                = (val64 << 32) | regValue[word_start];
            val64                = (val64 & ~mask) | fieldValue;
            regValue[word_start] = val64;
            regValue[word_end]   = (val64 >> 32);
        }
        else
        {
            FM_LOG_PRINT("%s: No support for field spanning more than 2 words.\n",
                         fields[i].name);
            return FM_ERR_UNSUPPORTED;
        }
        return FM_OK;
    }

    return FM_FAIL;

}   /* end fm10000DbgSetRegField */

