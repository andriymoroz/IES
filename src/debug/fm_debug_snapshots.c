/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_snapshots.c
 * Creation Date:   April 27, 2006
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#define MAX_REG_NAME_LENGTH         100
 
 
/* To allocate snapshot memory from shared-memory, set this #if to 1. This allows
 * any process to take and compare snapshots, but greatly limits the number of
 * snapshots that can be taken. For FM6000 chips, for instance, a maximum of 2
 * snapshots can be taken without increasing the shared-memory size.
 * Setting this #if to 0 causes snapshots to be allocated from process-local
 * storage. This relaxes the number of snapshot limitation but restricts
 * snapshot generation and comparisons to a single process. */
#if 0
#define ALLOC fmAlloc
#define FREE  fmFree
#else
#define ALLOC malloc
#define FREE  free
#endif


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


/*********************************************************************
 *
 * fmDbgSaveRegValueInSnapshot
 *
 * Description: callback function to save register contents into a snapshot
 *
 * Arguments:   sw                  switch number
 *              regId               Index into register table
 *              regAddress          address of register
 *              regSize             number of words in register
 *              isStatReg           TRUE if register is a statistics register
 *              regValue1           low-order 64 bits of register
 *              regValue2           high-order 64 bits of register
 *              callbackInfo        pointer to the snapshot
 *
 * Returns:     FALSE to cancel register scan, used if snapshot fills up
 *              TRUE to proceed to next register
 *
 *********************************************************************/
static fm_bool fmDbgSaveRegValueInSnapshot(fm_int    sw,
                                           fm_int    regId,
                                           fm_uint   regAddress,
                                           fm_int    regSize,
                                           fm_bool   isStatReg,
                                           fm_uint64 regValue1,
                                           fm_uint64 regValue2,
                                           fm_voidptr callbackInfo)
{
    fmDbgFulcrumSnapshot *        pSnapshot;
    fmDbgFulcrumRegisterSnapshot *pRegSnapshot;

    FM_NOT_USED(sw);

    pSnapshot = (fmDbgFulcrumSnapshot *) callbackInfo;

    if (pSnapshot->regCount >= FM_DBG_MAX_SNAPSHOT_REGS)
    {
        return FALSE;
    }

    pRegSnapshot = &pSnapshot->registers[pSnapshot->regCount];

    pRegSnapshot->regId      = regId;
    pRegSnapshot->regAddress = regAddress;
    pRegSnapshot->regSize    = regSize;
    pRegSnapshot->isStatReg  = isStatReg;
    pRegSnapshot->regValue1  = regValue1;
    pRegSnapshot->regValue2  = regValue2;

    pSnapshot->regCount++;
    return TRUE;

}   /* end fmDbgSaveRegValueInSnapshot */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*********************************************************************
 *
 * fmDbgInitSnapshots
 *
 * Description: internal function to initialize the snapshot facility
 *
 * Arguments:   none
 *
 * Returns:     nothing
 *
 *********************************************************************/
void fmDbgInitSnapshots(void)
{
    memset( fmRootDebug->fmDbgSnapshots, 0,
           sizeof(fmRootDebug->fmDbgSnapshots) );

}   /* end fmDbgInitSnapshots */




/*****************************************************************************/
/** fmDbgTakeChipSnapshot
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
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
void fmDbgTakeChipSnapshot(fm_int sw, fm_int snapshot)
{
    fmDbgFulcrumSnapshot *pSnapshot;
    fm_switch *           switchPtr;

    if (snapshot < 0 || snapshot >= FM_DBG_MAX_SNAPSHOTS)
    {
        FM_LOG_PRINT("snapshot number must be between 0 and %d inclusive\n",
                     FM_DBG_MAX_SNAPSHOTS - 1);
        return;
    }

    PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    if (switchPtr == NULL)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_PRINT("Invalid switch number %d, snapshot %d\n", sw, snapshot);
        return;
    }

    if (fmRootDebug->fmDbgSnapshots[snapshot] != NULL)
    {
        FREE(fmRootDebug->fmDbgSnapshots[snapshot]);
    }

    pSnapshot = (fmDbgFulcrumSnapshot *) ALLOC( sizeof(fmDbgFulcrumSnapshot) );
    fmRootDebug->fmDbgSnapshots[snapshot] = pSnapshot;

    if (pSnapshot == NULL)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_PRINT("can't allocate memory for snapshot %d\n", snapshot);
        return;
    }

    memset( pSnapshot, 0, sizeof(fmDbgFulcrumSnapshot) );

    pSnapshot->sw = sw;
    fmGetTime(&pSnapshot->timestamp);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgTakeChipSnapshot,
                            sw,
                            pSnapshot,
                            fmDbgSaveRegValueInSnapshot);

    UNPROTECT_SWITCH(sw);

}   /* end fmDbgTakeChipSnapshot */




/*****************************************************************************/
/** fmDbgDeleteChipSnapshot
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Discard a snapshot of the switch's configuration (the
 *                  register file) taken with a prior call to
 *                  fmDbgTakeChipSnapshot.
 *
 * \param[in]       snapshot is the snapshot number specified in a prior call
 *                  to fmDbgTakeChipSnapshot.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDeleteChipSnapshot(fm_int snapshot)
{
    if (snapshot < 0 || snapshot >= FM_DBG_MAX_SNAPSHOTS)
    {
        FM_LOG_PRINT("snapshot number must be between 0 and %d inclusive\n",
                     FM_DBG_MAX_SNAPSHOTS - 1);
        return;
    }

    if (fmRootDebug->fmDbgSnapshots[snapshot] != NULL)
    {
        FREE(fmRootDebug->fmDbgSnapshots[snapshot]);
        fmRootDebug->fmDbgSnapshots[snapshot] = NULL;
        FM_LOG_PRINT("Snapshot %d deleted\n", snapshot);
    }
    else
    {
        FM_LOG_PRINT("Snapshot %d was unused: no action taken\n", snapshot);
    }

}   /* end fmDbgDeleteChipSnapshot */




/*****************************************************************************/
/** fmDbgPrintChipSnapshot
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display a snapshot of the switch's configuration (the
 *                  register file) taken with a prior call to
 *                  fmDbgTakeChipSnapshot.
 *
 * \param[in]       snapshot is the snapshot number specified in a prior call
 *                  to fmDbgTakeChipSnapshot.
 *
 * \param[in]       showZeroValues should be TRUE to print registers with a
 *                  zero value or FALSE to only print registers with non-zero
 *                  values (this is useful to avoid printing thousands of
 *                  unused VID and FID table entries).
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgPrintChipSnapshot(fm_int snapshot, fm_bool showZeroValues)
{
    fmDbgFulcrumSnapshot *        pSnapshot;
    fm_int                        index;
    fmDbgFulcrumRegisterSnapshot *pRegister;

    if (snapshot < 0 || snapshot >= FM_DBG_MAX_SNAPSHOTS)
    {
        FM_LOG_PRINT("snapshot number must be between 0 and %d inclusive\n",
                     FM_DBG_MAX_SNAPSHOTS - 1);
        return;
    }

    pSnapshot = fmRootDebug->fmDbgSnapshots[snapshot];

    if (pSnapshot == NULL)
    {
        FM_LOG_PRINT("snapshot %d is unused\n", snapshot);
        return;
    }

    if (pSnapshot->regCount == 0)
    {
        FM_LOG_PRINT("snapshot %d is empty\n", snapshot);
        return;
    }

    pRegister = pSnapshot->registers;

    FM_LOG_PRINT("Snapshot %d was taken from switch %d at timestamp "
                 "%" FM_FORMAT_64 "u.%06" FM_FORMAT_64 "u with %d registers\n",
                 snapshot,
                 pSnapshot->sw,
                 pSnapshot->timestamp.sec,
                 pSnapshot->timestamp.usec,
                 pSnapshot->regCount);

    for (index = 0 ; index < pSnapshot->regCount ; index++, pRegister++)
    {
        if ( (pRegister->regValue1 != 0) || (pRegister->regValue2 != 0)
            || (showZeroValues == TRUE) )
        {
            fmDbgPrintRegValue(pSnapshot->sw,
                               pRegister->regId,
                               pRegister->regAddress,
                               pRegister->regSize,
                               pRegister->isStatReg,
                               pRegister->regValue1,
                               pRegister->regValue2, 0);
        }
    }

}   /* end fmDbgPrintChipSnapshot */




/*****************************************************************************/
/** fmDbgCompareChipSnapshots
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display a comparison between two or more snapshots of the
 *                  switch's configuration (the register file) taken with prior
 *                  calls to fmDbgTakeChipSnapshot.
 *
 * \param[in]       snapshotMask contains the bit mask of snapshots to compare,
 *                  where snapshot 0 is the least-significant bit, snapshot 1
 *                  is the next bit, etc.  A -1 will cause all snapshots to be
 *                  compared, ignoring unused snapshot numbers.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgCompareChipSnapshots(fm_uint snapshotMask)
{
    fmDbgFulcrumSnapshot *        pSnaps[FM_DBG_MAX_SNAPSHOTS];
    fm_int                        snapshotNumbers[FM_DBG_MAX_SNAPSHOTS];
    fmDbgFulcrumRegisterSnapshot *pReg0 = NULL;
    fmDbgFulcrumRegisterSnapshot *pRegX;
    fm_int                        snap;
    fm_int                        index;
    fm_bool                       different;
    fm_int                        activeCount;
    fm_bool                       abort = FALSE;
    fm_char                       regName[MAX_REG_NAME_LENGTH];
    fm_char                       curReg1[20];
    fm_char                       curReg2[20];
    fm_char                       tempBuf[40];
    fm_char                       outputBuffer1[1000];
    fm_char                       outputBuffer2[1000];
    fm_bool                       isPort;
    fm_int                        index0Ptr;
    fm_int                        index1Ptr;
    fm_int                        index2Ptr;

    memset( pSnaps, 0, sizeof(pSnaps) );

    /* count the active snapshots */
    index = 0;

    for (snap = 0 ; snap < FM_DBG_MAX_SNAPSHOTS ; snap++)
    {
        if ( (fmRootDebug->fmDbgSnapshots[snap] != NULL)
            && (fmRootDebug->fmDbgSnapshots[snap]->regCount > 0)
            && ( snapshotMask & (1 << snap) ) )
        {
            pSnaps[index]          = fmRootDebug->fmDbgSnapshots[snap];
            snapshotNumbers[index] = snap;
            index++;
        }
    }

    activeCount = index;

    if (activeCount == 0)
    {
        FM_LOG_PRINT("No active snapshots were found using mask %08X\n",
                     snapshotMask);
        return;
    }

    if (activeCount == 1)
    {
        FM_LOG_PRINT("Only one active snapshot was found using mask %08X\n",
                     snapshotMask);
        return;
    }

    /* compare snapshot information and registers */
    for (index = -4 ; index < pSnaps[0]->regCount ; index++)
    {
        if (abort)
        {
            break;
        }

        different = FALSE;

        /* compare snapshot 0 against all other snapshots */
        for (snap = 1 ; snap < activeCount ; snap++)
        {
            if (index < 0)
            {
                switch (index)
                {
                    case - 4:            /* snapshot numbers */
                        different = TRUE;
                        break;

                    case - 3:

                        /* switch number */
                        if (pSnaps[0]->sw != pSnaps[snap]->sw)
                        {
                            different = TRUE;
                            break;
                        }

                        break;

                    case - 2:

                        /* timestamp */
                        if ( (pSnaps[0]->timestamp.sec != pSnaps[snap]->
                                  timestamp.sec)
                            || (pSnaps[0]->timestamp.usec != pSnaps[snap]->
                                    timestamp.usec) )
                        {
                            different = TRUE;
                            break;
                        }

                        break;

                    case - 1:

                        /* register count */
                        if (pSnaps[0]->regCount != pSnaps[snap]->regCount)
                        {
                            different = TRUE;
                            break;
                        }

                        break;

                }   /* end switch (index) */

            }
            else
            {
                pReg0 = &pSnaps[0]->registers[index];
                pRegX = &pSnaps[snap]->registers[index];

                if (pReg0->regAddress != pRegX->regAddress)
                {
                    FM_LOG_PRINT("ERROR!  Snapshot register tables do not match!\n"
                                 "  index = %d, address 0 = %08X, "
                                 "address %d = %08X\n",
                                 index,
                                 pReg0->regAddress,
                                 snap,
                                 pRegX->regAddress);
                    abort = TRUE;
                    break;
                }

                if (pReg0->regSize != pRegX->regSize)
                {
                    FM_LOG_PRINT("ERROR!  Snapshot register sizes do not match!\n"
                                 " index = %d, address = %08X, size 0 = %d, "
                                 "size %d = %d\n",
                                 index,
                                 pReg0->regAddress,
                                 pReg0->regSize,
                                 snap,
                                 pRegX->regSize);
                    abort = TRUE;
                    break;
                }

                if ( (pReg0->regValue1 != pRegX->regValue1)
                    || (pReg0->regValue2 != pRegX->regValue2) )
                {
                    different = TRUE;
                    break;
                }
            }
        }

        if (abort)
        {
            break;
        }

        if (different)
        {
            if (index < 0)
            {
                switch (index)
                {
                    case - 4:            /* snapshot numbers */
                        fmStringCopy(regName, "Snapshot Number",
                                     sizeof(regName));
                        break;

                    case - 3:            /* switch number */
                        fmStringCopy(regName, "Switch Number",
                                     sizeof(regName));
                        break;

                    case - 2:            /* timestamp */
                        fmStringCopy(regName, "Timestamp", sizeof(regName));
                        break;

                    case - 1:            /* register count */
                        fmStringCopy(regName, "Register Count",
                                     sizeof(regName));
                        break;

                }   /* end switch (index) */

            }
            else
            {
                fmDbgGetRegisterName(pSnaps[0]->sw,
                                     pReg0->regId,
                                     pReg0->regAddress,
                                     regName,
                                     MAX_REG_NAME_LENGTH,
                                     &isPort,
                                     &index0Ptr,
                                     &index1Ptr,
                                     &index2Ptr,
                                     TRUE,
                                     FALSE);
            }

            outputBuffer1[0] = 0;
            outputBuffer2[0] = 0;

            for (snap = 0 ; snap < activeCount ; snap++)
            {
                curReg1[0] = 0;
                curReg2[0] = 0;

                if (index < 0)
                {
                    switch (index)
                    {
                        case -4:             /* Snapshot # */
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%d", snapshotNumbers[snap]);
                            break;

                        case -3:            /* switch number */
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%d", pSnaps[snap]->sw);
                            break;

                        case -2:             /* timestamp */
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%" FM_FORMAT_64 "u.%06" FM_FORMAT_64 "u",
                                          pSnaps[snap]->timestamp.sec,
                                          pSnaps[snap]->timestamp.usec);
                            break;

                        case -1:             /* register count */
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%d", pSnaps[snap]->regCount);
                            break;

                    }   /* end switch (index) */

                }
                else
                {
                    pRegX = &pSnaps[snap]->registers[index];

                    switch (pRegX->regSize)
                    {
                        case 1:
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%08X",
                                          (fm_uint32) pRegX->regValue1);
                            break;

                        case 2:

                            if (pRegX->isStatReg)
                            {
                                FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                              "%" FM_FORMAT_64 "u",
                                              pRegX->regValue1);
                            }
                            else
                            {
                                FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                              "%016" FM_FORMAT_64 "X",
                                              pRegX->regValue1);
                            }

                            break;

                        case 3:
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%016" FM_FORMAT_64 "X",
                                          pRegX->regValue1);
                            FM_SNPRINTF_S(curReg2, sizeof(curReg2),
                                          "%08X",
                                          (fm_uint32) pRegX->regValue2);
                            break;

                        case 4:
                            FM_SNPRINTF_S(curReg1, sizeof(curReg1),
                                          "%016" FM_FORMAT_64 "X",
                                          pRegX->regValue1);
                            FM_SNPRINTF_S(curReg2, sizeof(curReg2),
                                          "%016" FM_FORMAT_64 "X",
                                          pRegX->regValue2);
                            break;

                    }    /* end switch (pRegX->regSize) */

                }

                FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), "%20s", curReg1);
                fmStringAppend(outputBuffer1, tempBuf, sizeof(outputBuffer1));

                if (curReg2[0] != 0)
                {
                    FM_SNPRINTF_S(tempBuf, sizeof(tempBuf), "%20s", curReg2);
                    fmStringAppend(outputBuffer2, tempBuf,
                                   sizeof(outputBuffer2));
                }
            }

            FM_LOG_PRINT("%-40s  %s\n", regName, outputBuffer1);

            if (outputBuffer2[0] != 0)
            {
                FM_LOG_PRINT("%40s  %s\n", " ", outputBuffer2);
            }
        }
    }

}   /* end fmDbgCompareChipSnapshots */
