/* vim:et:sw=4:ts=4:tw=79:
 */
/*****************************************************************************
 * File:            src/api/fm10000/fm10000_api_sbus.c
 * Creation Date:   November 6, 2013
 * Description:     SBus low-level API
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

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define SBUS_WAIT_LOOP_INITIAL_DELAY_NS     500

#define SBUS_MAX_WAIT_LOOPS                 1500

/* Min NMV version that supports PCIE SUBS locking */
#define MIN_NVM_VERSION                     0x113

enum
{
    FM10000_PCIE_SBUS_LOCK_FREE = 0,
    FM10000_PCIE_SBUS_LOCK_NVM  = 1,
    FM10000_PCIE_SBUS_LOCK_API  = 2,
};

/* SBus operation codes */
enum
{
    FM10000_SBUS_OP_RESET = 0x20,
    FM10000_SBUS_OP_WRITE = 0x21,
    FM10000_SBUS_OP_READ  = 0x22
};

/* SBus result codes */
enum
{
    FM10000_SBUS_RESULT_RESET = 0x0,
    FM10000_SBUS_RESULT_WRITE = 0x1,
    FM10000_SBUS_RESULT_READ  = 0x4
};

/* SBus request structure */
typedef struct
{
    fm_int      opCode;         /* Operation to perform */
    fm_uint32   resultCode;     /* Expected result code */
    fm_uint32   devAddr;        /* SBus device address */
    fm_uint32   regAddr;        /* SBus register address */
    fm_uint32   data;           /* Request or response value */

} fm10000_sbusReq;



/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_int sbusDebug = 0;

/*****************************************************************************
 * Local Function Prototypes
 *****************************************************************************/

static fm_status SBusRequest(fm_int           sw,
                             fm_bool          eplRing,
                             fm10000_sbusReq *pSbusReq);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** WaitForLockOwner
 * \ingroup intSBus
 *
 * \desc            Wait for lock owner value to be some specified value.
  *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       owner is the lock value to wait for.
 *
 * \param[in]       timeoutNsec is the timeout in nanoseconds to wait for.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status WaitForLockOwner(fm_int sw, fm_uint owner, fm_uint timeoutNsec)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint         waitLoopCnt;
    fm_uint32       busLock;
    fm_uint         prevOwner;
    fm_uint         lockOwner;
    fm_uint         delayNsec;
    fm_uint         totalDelayNsec;


    switchPtr = GET_SWITCH_PTR(sw);

    waitLoopCnt = 0;
    totalDelayNsec = 0;
    err = switchPtr->ReadUINT32( sw, 
                                 FM10000_BSM_SCRATCH(1),
                                 &busLock );
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    lockOwner = FM_GET_UNNAMED_FIELD( busLock, 0, 2);

    if (lockOwner == FM10000_PCIE_SBUS_LOCK_API)
    {
        if (owner != FM10000_PCIE_SBUS_LOCK_API)
        {
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                         "Lockowner is already held by API. Request %d\n", owner);
        }
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    while (lockOwner != owner)
    {
        if (totalDelayNsec >= timeoutNsec)
        {
            if (timeoutNsec > 0)
            {
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                             "Lock is still taken by %d != %d in %d usec. Interations %d\n",
                             lockOwner, owner, totalDelayNsec/1000, waitLoopCnt);
            }
            err = FM_FAIL;
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }

        waitLoopCnt++;
        delayNsec = 1000 * waitLoopCnt;
        fmDelay(0, delayNsec);
        totalDelayNsec += delayNsec;
        err = switchPtr->ReadUINT32( sw, 
                                     FM10000_BSM_SCRATCH(1),
                                     &busLock );
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        prevOwner = lockOwner;
        lockOwner = FM_GET_UNNAMED_FIELD( busLock, 0, 2);
    }

    if (waitLoopCnt)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                    "Wait for lock = %d (lastOwner = %d) in %d usec. Interations %d\n",
                     owner, prevOwner, totalDelayNsec/1000, waitLoopCnt);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

} /* end WaitForLockOwner */




/*****************************************************************************/
/** TakePcieSbusLock
 * \ingroup intSBus
 *
 * \desc            Take PCIE SBUS lock to avoid contention with NVM.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status TakePcieSbusLock(fm_int sw)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint         waitLoopCnt;
    fm_uint32       softReset;
    fm_uint32       PCIeActive;
    fm_uint         delayNsec;
    fm_uint         totalDelayNsec;
    fm_uint         nvmVer;
    fm_int          cnt;

    err = fm10000GetNvmImageVersion(sw, &nvmVer);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    switchPtr = GET_SWITCH_PTR(sw);

    if (nvmVer >= MIN_NVM_VERSION)
    {
        /* Has support for locking in NVM */
        for (cnt = 0; cnt < 3; cnt++)
        {
            /* Wait for lock is free */
            err = WaitForLockOwner(sw, FM10000_PCIE_SBUS_LOCK_FREE, 1000*1000*2000);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* Write to take lock 
             * 0: free
             * 2: API
             * x: others
             */
            err = switchPtr->WriteUINT32( sw, 
                                          FM10000_BSM_SCRATCH(1),
                                          FM10000_PCIE_SBUS_LOCK_API );
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            /* NVM is slower, so must delay before checking */
            fmDelay(0, 50*1000);

            /* Check to make sure lock is taken correctly, if not try again */
            err = WaitForLockOwner(sw, FM10000_PCIE_SBUS_LOCK_API, 0);
            if (err == FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
            }
            else
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                            "Unable to take lock. Try #%d\n", cnt);                
            }
        }
    }
    else
    {
        /* No locking support in NVM, NVM is accessing the PCIE SBUS */
        err = switchPtr->ReadUINT32( sw, 
                                     FM10000_SOFT_RESET(),
                                     &softReset );
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        PCIeActive = 
            FM_GET_UNNAMED_FIELD( softReset, 
                                  FM10000_SOFT_RESET_b_PCIeActive_0, 
                                  9 );
        waitLoopCnt = 0;
        totalDelayNsec = 0;
        while (PCIeActive)
        {
            if (totalDelayNsec > 1000*1000*500)
            {
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                             "PCIE is still active in %d usec. Interations %d\n",
                             totalDelayNsec/1000, waitLoopCnt);
                err = FM_FAIL;
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }

            waitLoopCnt++;
            delayNsec = 1000 * waitLoopCnt;
            fmDelay(0, delayNsec);

            switchPtr->ReadUINT32( sw, 
                                   FM10000_SOFT_RESET(),
                                   &softReset );
            PCIeActive = 
                FM_GET_UNNAMED_FIELD( softReset, 
                                      FM10000_SOFT_RESET_b_PCIeActive_0, 
                                      9 );
            totalDelayNsec += delayNsec;
        }
        if (waitLoopCnt)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "PCIeActive in %d usec. Interations %d\n",
                         totalDelayNsec/1000, waitLoopCnt);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

} /* TakePcieSbusLock */




/*****************************************************************************/
/** DropPcieSbusLock
 * \ingroup intSBus
 *
 * \desc            Drop PCIE SBUS lock, if needed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DropPcieSbusLock(fm_int sw)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint32       busLock;
    fm_uint         lockOwner;
    fm_uint32       nvmVer;

    switchPtr = GET_SWITCH_PTR(sw);

    err = fm10000GetNvmImageVersion(sw, &nvmVer);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (nvmVer >= MIN_NVM_VERSION)
    {
        err = switchPtr->ReadUINT32( sw, 
                                     FM10000_BSM_SCRATCH(1),
                                     &busLock );
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        lockOwner = FM_GET_UNNAMED_FIELD( busLock, 0, 2);

        switch (lockOwner)
        {
            case FM10000_PCIE_SBUS_LOCK_API:
                return (switchPtr->WriteUINT32( sw, 
                                                FM10000_BSM_SCRATCH(1),
                                                0 ));
                break;
            default:
                FM_LOG_WARNING(FM_LOG_CAT_SWITCH,
                    "Attemp to release API lock but lock is being used by %d\n",
                    lockOwner);
                break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    
} /* DropPcieSbusLock */



/*****************************************************************************/
/** SBusRequest
 * \ingroup intSBus
 *
 * \desc            Executes the specified SBus transaction. The kind of
 *                  transaction is specified by the opCode field of the
 *                  ''fm10000_sbusReq'' structure pointed by pSbusReq and it may
 *                  be write, read or reset. This function blocks until the
 *                  transaction is completed or an error has happend.
 *                  Note that execution of a single SBus request could take up
 *                  to 15us.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       eplRing is TRUE if the destination ring is EPL or FALSE
 *                  if it is the PCIE ring.
 *
 * \param[in,out]   pSbusReq points to the SBus request to execute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if pSbusReq is a NULL pointer.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status SBusRequest(fm_int           sw,
                      fm_bool          eplRing,
                      fm10000_sbusReq *pSbusReq)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint         sbusReqRegAddr;
    fm_uint         sbusCmdRegAddr;
    fm_uint         sbusRespRegAddr;
    fm_uint         waitLoopCnt;
    fm_uint32       regValue;
    fm_uint32       resultCode;
    fm_uint         delayNsec;
    fm_uint         totalDelayNsec;


    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d, eplRing=%d, pSbusReq=%p\n",
                         sw,
                         eplRing,
                         (void *) pSbusReq);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pSbusReq == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* Need to create a specific LOCK of very low priority for the SBUS access */
        TAKE_REG_LOCK(sw);

        if (!eplRing)
        {
            err = TakePcieSbusLock(sw);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }

        /* SBUS_EPL_XXX and SBUS_PCIE_XXX have the same field format, so SBUS_EPL_XXX
         * will be used for both */
        if (eplRing)
        {
            sbusReqRegAddr  = FM10000_SBUS_EPL_REQUEST();
            sbusCmdRegAddr  = FM10000_SBUS_EPL_COMMAND();
            sbusRespRegAddr = FM10000_SBUS_EPL_RESPONSE();
        }
        else
        {
            sbusReqRegAddr  = FM10000_SBUS_PCIE_REQUEST();
            sbusCmdRegAddr  = FM10000_SBUS_PCIE_COMMAND();
            sbusRespRegAddr = FM10000_SBUS_PCIE_RESPONSE();
        }

        /* verify that the SBUS is ready for a new command */
        err = switchPtr->ReadUINT32(sw, sbusCmdRegAddr, &regValue);

        if (err == FM_OK)
        {
            if (!FM_GET_BIT(regValue, FM10000_SBUS_EPL_COMMAND, Busy))
            {
                if (pSbusReq->opCode == FM10000_SBUS_OP_WRITE ||
                    pSbusReq->opCode == FM10000_SBUS_OP_RESET)
                {
                    /* Load the data for the SBus request that is to be executed. */
                    err = switchPtr->WriteUINT32(sw, sbusReqRegAddr, pSbusReq->data);
                }

                /* configure the SBUS command */
                if (err == FM_OK)
                {
                    regValue = 0;

                    /* Clear the SBUS_COMMAND register for new SBus request to be executed. */
                    err = switchPtr->WriteUINT32(sw, sbusCmdRegAddr, regValue);
                }

                if (err == FM_OK)
                {
                    /* Start execution of a new SBus request. */
                    FM_SET_FIELD(regValue,
                                 FM10000_SBUS_EPL_COMMAND,
                                 Register,
                                 pSbusReq->regAddr);
                    FM_SET_FIELD(regValue,
                                 FM10000_SBUS_EPL_COMMAND,
                                 Address,
                                 pSbusReq->devAddr);
                    FM_SET_FIELD(regValue,
                                 FM10000_SBUS_EPL_COMMAND,
                                 Op,
                                 pSbusReq->opCode);
                    FM_SET_BIT  (regValue,
                                 FM10000_SBUS_EPL_COMMAND,
                                 Execute,
                                 TRUE);
    
                    err = switchPtr->WriteUINT32(sw, sbusCmdRegAddr, regValue);
                }

                /* Wait until the SBus request has been executed. */

                /* timeout = SBUS_WAIT_LOOP_INITIAL_DELAY_NS * SBUS_MAX_WAIT_LOOPS * (SBUS_MAX_WAIT_LOOPS/2) */

                waitLoopCnt = 1;
                totalDelayNsec = 0;
                do
                {
                    /* check the busy flag */
                    err = switchPtr->ReadUINT32(sw, sbusCmdRegAddr, &regValue);

                    if (err == FM_OK && FM_GET_BIT(regValue, FM10000_SBUS_EPL_COMMAND, Busy) == FALSE)
                    {
                        break;
                    }

                    if (++waitLoopCnt >= SBUS_MAX_WAIT_LOOPS)
                    {
                        err = FM_FAIL;
                        FM_LOG_FATAL(FM_LOG_CAT_SWITCH, 
                                     "SBUS Command (dev=0x%x, reg=0x%x) timed out in %d ms\n", 
                                     pSbusReq->devAddr,
                                     pSbusReq->regAddr,
                                     totalDelayNsec );
                    }
                    else
                    {
                        delayNsec = SBUS_WAIT_LOOP_INITIAL_DELAY_NS * waitLoopCnt;
                        /* the delay is proportional to the number of wait loops */
                        fmDelay(0, delayNsec);  
                        totalDelayNsec += delayNsec;                    
                    }
                }
                while (err == FM_OK);


                if (err == FM_OK)
                {
                    /* Retrieve the SBus result code */
                    resultCode = FM_GET_FIELD(regValue, FM10000_SBUS_EPL_COMMAND, ResultCode);
    
                    if (resultCode != pSbusReq->resultCode)
                    {
                        err = FM_FAIL;
                        FM_LOG_FATAL(FM_LOG_CAT_SWITCH, 
                                     "SBUS Invalid result code %x. Expected 0x%x. Addr 0x%x Reg 0x%x.\n", 
                                     resultCode,
                                     pSbusReq->resultCode,
                                     sbusCmdRegAddr,
                                     regValue);
                    }
                    else if (pSbusReq->opCode == FM10000_SBUS_OP_READ)
                    {
                        /* a read command was executed, get the  SBus response. */
                        sbusRespRegAddr = eplRing ? FM10000_SBUS_EPL_RESPONSE() : FM10000_SBUS_PCIE_RESPONSE();
    
                        err = switchPtr->ReadUINT32(sw, sbusRespRegAddr, &regValue);
    
                        if (err == FM_OK)
                        {
                            pSbusReq->data = FM_GET_FIELD(regValue, FM10000_SBUS_EPL_RESPONSE, Data);
                        }
                    }
                }
            }   /* end if (!regValueFM_GET_BIT( ...) ) */
            else
            {
                err = FM_FAIL;
                if (fm10000VerifySwitchAliveStatus(sw) == FM_OK)
                {
                    /* No need to display this message if switch is dead 
                     * fm10000VerifySwitchAliveStatus will display one already */
                    FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "SBUS is busy, ring=%s \n", eplRing ? "EPL" : "PCIe");
                }
            }
        }

        if (!eplRing)
        {
            DropPcieSbusLock(sw);
        }

        DROP_REG_LOCK(sw);

    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end SBusRequest */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000SbusSetDebug
 * \ingroup intSBus
 *
 * \desc            Set debug flag.
 *
 * \param[in]       debug is the SBus debug level.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fm10000SbusSetDebug(fm_int debug)
{

    sbusDebug = debug;

} /* end fm10000SbusSetDebug */




/*****************************************************************************/
/** fm10000SbusInit
 * \ingroup intSBus
 *
 * \desc            Initializes the SBus ring if required.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       eplRing is TRUE if the destination ring is EPL or FALSE
 *                  if it is the PCIE ring.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbusInit(fm_int    sw,
                          fm_bool   eplRing)
{
    fm_switch      *switchPtr;
    fm10000_switch *switchExt;
    fm_uint32       divider;
    fm_status       err;
    fm_status       err2;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Skip SBus intialization if the API is running on the Test Bench */
    if (switchExt->serdesBypassSbus == FALSE)
    {
        TAKE_REG_LOCK(sw);

        /* send a reset command to the SBus */
        err = fm10000SbusSbmReset(sw, eplRing);
    
        if (err == FM_OK)
        {
            err = fm10000SbusReset(sw,eplRing);
            
            if (err == FM_OK)
            {
                /* set SBus clock to 1/2 REFCLK:
                 *  write FM10000_SBUS_EPL_RING_DIVIDER to reg 0x0a, sbusAddr 0xfe
                 *  (FM10000_SBUS_CONTROLLER_ADDR) */
               err = fm10000SbusWrite(sw, eplRing, FM10000_SBUS_CONTROLLER_ADDR, 0x0a,FM10000_SBUS_EPL_RING_DIVIDER);
            }
        }

        /* read back the divider and verify it
         * This code generates an error message but it does not return an error if it fails */
        if (err == FM_OK)
        {
            err2 = fm10000SbusRead(sw, eplRing, FM10000_SBUS_CONTROLLER_ADDR, 0x0a, &divider);

            if (err2 == FM_OK)
            {
                if (divider != FM10000_SBUS_EPL_RING_DIVIDER)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                                 "SBus ring:%s: Unexpected SBUS divider, read = %d, expected = %d\n",
                                 eplRing? "EPL" : "PCIe",
                                 divider,
                                 FM10000_SBUS_EPL_RING_DIVIDER);
                }
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                             "SBus ring:%s: Cannot read SBUS divider\n",
                             eplRing? "EPL" : "PCIe");
            }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                         "Cannot initialiaze SBus Ring %s\n",
                         eplRing? "EPL" : "PCIe");
        }
    
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SbusInit */




/*****************************************************************************/
/** fm10000SbusRead
 * \ingroup intSBus
 *
 * \desc            Performs a low-level SBus read access.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       eplRing specifies EPL ring (TRUE) or PCIE ring (FALSE).
 *
 * \param[in]       sbusAddr is the SBus address to read from.
 *
 * \param[in]       sbusReg is the SBus register to read from.
 *
 * \param[in]       pData points to user-allocated storage where this function
 *                  will place the register value.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbusRead(fm_int     sw,
                          fm_bool    eplRing,
                          fm_uint    sbusAddr,
                          fm_uint    sbusReg,
                          fm_uint32 *pData)
{
    fm_status       err;
    fm10000_sbusReq sbusReq;
    fm_timestamp    timeStamp;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d eplRing=%d sbusAddr=0x%x sbusReg=0x%x data=%p\n",
                         sw,
                         eplRing,
                         sbusAddr,
                         sbusReg,
                         (void *) pData);

    err = FM_OK;
    fmGetTime(&timeStamp);

    if (pData == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (sbusAddr > 0xFF)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                     "sbusAddr %04x out of range. reg 0x%x\n",
                      sbusAddr, sbusReg);
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *pData = 0;

        /* bypass mode:
         *   skip sbus access in bypass mode */
        if (!fmPlatformBypassEnabled(sw))
        {
            memset((void *) &sbusReq, 0, sizeof(fm10000_sbusReq));
        
            sbusReq.opCode      = FM10000_SBUS_OP_READ;
            sbusReq.resultCode  = FM10000_SBUS_RESULT_READ;
            sbusReq.devAddr     = sbusAddr & 0xFFU;
            sbusReq.regAddr     = sbusReg & 0xFFU;
        
            err = SBusRequest(sw, eplRing, &sbusReq);

            if (err == FM_OK)
            {
                *pData = sbusReq.data;

                if (sbusDebug)
                {
                    FM_LOG_PRINT("sw=%d ring=%d addr=0x%2.2x reg=0x%2.2x => 0x%8.8x  t=%4.4d.%3.3d\n",
                                 sw,
                                 eplRing,
                                 sbusAddr, 
                                 sbusReg,
                                 *pData,
                                 (fm_int)(timeStamp.sec%10000), 
                                 (fm_int)(timeStamp.usec/1000));
                }
            }
        }   /* end if (!fmPlatformBypassEnabled(sw)) */
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SbusRead */





/*****************************************************************************/
/** fm10000SbusWrite
 * \ingroup intSBus
 *
 * \desc            Performs a low-level SBus write access.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       eplRing specifies EPL ring (TRUE) or PCIE ring (FALSE).
 *
 * \param[in]       sbusAddr is the SBus address to read from.
 *
 * \param[in]       sbusReg is the SBus register to read from.
 *
 * \param[in]       data is the 32-bit data to be written.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbusWrite(fm_int    sw,
                           fm_bool   eplRing,
                           fm_uint   sbusAddr,
                           fm_uint   sbusReg,
                           fm_uint32 data)
{
    fm_status       err;
    fm10000_sbusReq sbusReq;
    fm_timestamp    timeStamp;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d eplRing=%d sbusAddr=0x%x sbusReg=0x%x data=0x%x\n",
                         sw,
                         eplRing, 
                         sbusAddr, 
                         sbusReg, 
                         data);

    err = FM_OK;
    fmGetTime(&timeStamp);

    if (sbusAddr > 0xFF)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                     "sbusAddr %04x our of range. reg 0x%x data %08x\n",
                      sbusAddr, sbusReg, data);
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (sbusAddr > 0)
    {
        /*  Do not do any SBUS request in bypass mode as the write
         *  accesses are not performed to the real hardware */
        if (!fmPlatformBypassEnabled(sw))
        {
            if (sbusDebug)
            {
                FM_LOG_PRINT("sw=%d ring=%d addr=0x%2.2x reg=0x%2.2x <= 0x%8.8x  t=%4.4d.%3.3d\n",
                             sw, 
                             eplRing, 
                             sbusAddr, 
                             sbusReg, 
                             data, 
                             (fm_int)(timeStamp.sec%10000), 
                             (fm_int)(timeStamp.usec/1000) );
            }
        
            memset((void *) &sbusReq, 0, sizeof(fm10000_sbusReq));
        
            sbusReq.opCode      = FM10000_SBUS_OP_WRITE;
            sbusReq.resultCode  = FM10000_SBUS_RESULT_WRITE;
            sbusReq.devAddr     = sbusAddr & 0xFFU;
            sbusReq.regAddr     = sbusReg & 0xFFU;
            sbusReq.data        = data;
        
            err = SBusRequest(sw, eplRing, &sbusReq);
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SbusWrite */




/*****************************************************************************/
/** fm10000SbusReceiverReset
 * \ingroup intSBus
 *
 * \desc            Sends a reset command to the specified SBUS receiver.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes identifes the receiver the Reset is directed to.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbusReceiverReset(fm_int    sw,
                                   fm_int    serDes)
{
    fm_status       err;
    fm10000_sbusReq sbusReq;
    fm_timestamp    timeStamp;
    fm_serdesRing   ring;
    fm_bool         eplRing;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw=%d serDes=%d\n", sw, serDes);

    err = FM_OK;
    fmGetTime(&timeStamp);

    sbusReq.opCode      = FM10000_SBUS_OP_RESET;
    sbusReq.resultCode  = FM10000_SBUS_RESULT_RESET;
    sbusReq.regAddr     = 0;
    sbusReq.data        = 0;

    /*  Do not do any SBUS request in bypass mode as the write
     *  accesses are not performed to the real hardware */
    if (!fmPlatformBypassEnabled(sw))
    {
        err = fm10000MapSerdesToSbus(sw, serDes, &sbusReq.devAddr, &ring);
        
        eplRing = ( ring == FM10000_SERDES_RING_EPL);
        
        if (sbusDebug)
        {
            FM_LOG_PRINT("sw=%d ring=%d addr=0x%2.2x reg=0x%2.2x <= 0x%8.8x  t=%4.4d.%3.3d\n",
                         sw, 
                         eplRing, 
                         sbusReq.devAddr, 
                         sbusReq.regAddr, 
                         sbusReq.data,
                         (fm_int)(timeStamp.sec%10000), 
                         (fm_int)(timeStamp.usec/1000));
        }
    
        err = SBusRequest(sw, eplRing, &sbusReq);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SbusReceiverReset */




/*****************************************************************************/
/** fm10000SbusReset
 * \ingroup intSBus
 *
 * \desc            Sends a reset command to the specified SBUS ring.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       eplRing specifies EPL ring (TRUE) or PCIE ring (FALSE).
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbusReset(fm_int    sw,
                           fm_bool   eplRing)
{
    fm_status       err;
    fm10000_sbusReq sbusReq;
    fm_timestamp    timeStamp;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw=%d eplRing=%d\n", sw, eplRing);

    err = FM_OK;
    fmGetTime(&timeStamp);

    /* use broadcast address to reset all SerDes */
    sbusReq.opCode      = FM10000_SBUS_OP_RESET;
    sbusReq.resultCode  = FM10000_SBUS_RESULT_RESET;
    sbusReq.devAddr     = 0xFFU;
    sbusReq.regAddr     = 0;
    sbusReq.data        = 0;


    /*  Do not do any SBUS request in bypass mode as the write
     *  accesses are not performed to the real hardware */
    if (!fmPlatformBypassEnabled(sw))
    {
        if (sbusDebug)
        {
            FM_LOG_PRINT("sw=%d ring=%d addr=0x%2.2x reg=0x%2.2x <= 0x%8.8x  t=%4.4d.%3.3d\n",
                         sw, 
                         eplRing, 
                         sbusReq.devAddr, 
                         sbusReq.regAddr, 
                         sbusReq.data,
                         (fm_int)(timeStamp.sec%10000), 
                         (fm_int)(timeStamp.usec/1000));
        }
    
        err = SBusRequest(sw, eplRing, &sbusReq);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SbusReset */




/*****************************************************************************/
/** fm10000SbusSbmReset
 * \ingroup intSBus
 *
 * \desc            Sends a SBM reset command to the specified SBUS ring.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       eplRing specifies EPL ring (TRUE) or PCIE ring (FALSE).
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbusSbmReset(fm_int    sw,
                              fm_bool   eplRing)
{
    fm_status       err;
    fm10000_sbusReq sbusReq;
    fm_timestamp    timeStamp;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH, "sw=%d eplRing=%d\n", sw, eplRing);

    err = FM_OK;
    fmGetTime(&timeStamp);


    sbusReq.opCode      = FM10000_SBUS_OP_RESET;
    sbusReq.resultCode  = FM10000_SBUS_RESULT_RESET;
    sbusReq.devAddr     = 0xFDU;
    sbusReq.regAddr     = 0;
    sbusReq.data        = 0;


    /*  Do not do any SBUS request in bypass mode as the write
     *  accesses are not performed to the real hardware */
    if (!fmPlatformBypassEnabled(sw))
    {
        if (sbusDebug)
        {
            FM_LOG_PRINT("sw=%d ring=%d addr=0x%2.2x reg=0x%2.2x <= 0x%8.8x  t=%4.4d.%3.3d\n",
                         sw, 
                         eplRing, 
                         sbusReq.devAddr, 
                         sbusReq.regAddr, 
                         sbusReq.data,
                         (fm_int)(timeStamp.sec%10000), 
                         (fm_int)(timeStamp.usec/1000));
        }
    
        err = SBusRequest(sw, eplRing, &sbusReq);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SbusSbmReset */


