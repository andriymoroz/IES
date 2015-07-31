/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_config_nvm.c
 * Creation Date:   April 1, 2015
 * Description:     Platform functions to handle loading configurations from
 *                  NVM image.
 *
 * Copyright (c) 2015, Intel Corporation
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
#include <platforms/util/fm_util_device_lock.h>

#include <platforms/util/fm_util.h>
#include <platforms/util/fm10000/fm10000_util_spi.h>

#ifdef FM_LT_WHITE_MODEL_SUPPORT
#include <platforms/common/model/fm10000/fm10000_model_types.h>
#endif


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define SPI_FREQ_KHZ                          50000

#define NVM_HEADER_LT_CFG                     0x4
#define NVM_HEADER_LT_CFG_l_Length            24
#define NVM_HEADER_LT_CFG_h_Length            31
#define NVM_HEADER_LT_CFG_l_Base              0
#define NVM_HEADER_LT_CFG_h_Base              23

#define NVM_LT_CFG                            0
#define NVM_LT_CFG_l_FileFmt                  24
#define NVM_LT_CFG_h_FileFmt                  31
#define NVM_LT_CFG_l_Version                  16
#define NVM_LT_CFG_h_Version                  23
/* Length in 32-bit words of Liberty Trail Config in NVM, including Liberty
 * Trail Config Header. */
#define NVM_LT_CFG_l_LengthInWords            0
#define NVM_LT_CFG_h_LengthInWords            15

#define NVM_LT_CFG_HEADER_LEN                 16

#define NVM_LT_CFG_FILE_FORMAT_NOT_PRESENT    0
#define NVM_LT_CFG_FILE_FORMAT_RAW            1

#define NVM_LT_CFG_VERSION_SUPPORTED          0


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
/** RegRead32
 * \ingroup intPlatform
 *
 * \desc            Read register value.
 *
 * \param[in]       switchMem is pointer to mapped memory
 *
 * \param[in]       addr is the address of the register
 *
 * \param[out]      value is a pointer to the register value
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchMem or value is NULL.
 *
 *****************************************************************************/
static fm_status RegRead32(fm_uintptr switchMem,
                           fm_uint    addr,
                           fm_uint32 *value)
{
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "switchMem %p addr 0x%x value %p\n",
                 (void*)switchMem,
                 addr,
                 (void*)value);

    if ( ((fm_uint32*)switchMem == NULL) || (value == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    *value = *(((fm_uint32*)switchMem) + addr);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end RegRead32 */




/*****************************************************************************/
/** RegWrite32
 * \ingroup intPlatform
 *
 * \desc            Write a value to register.
 *
 * \param[in]       switchMem is pointer to mapped memory
 *
 * \param[in]       addr is the address of the register
 *
 * \param[out]      value is the register value
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if switchMem is NULL.
 *
 *****************************************************************************/
static fm_status RegWrite32(fm_uintptr switchMem,
                            fm_uint    addr,
                            fm_uint32  value)
{
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "switchMem %p addr 0x%x value 0x%x\n",
                 (void*)switchMem,
                 addr,
                 value);

    if ((fm_uint32*)switchMem == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    *(((fm_uint32*)switchMem) + addr) = value;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end RegWrite32 */




/*****************************************************************************/
/** NvmRead32
 * \ingroup intPlatform
 *
 * \desc            Read 32-bit value from NVM image.
 *
 * \param[in]       switchMem is pointer to mapped memory
 *
 * \param[in]       addr is the word number in NMV image
 *
 * \param[out]      value is a pointer to the value
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchMem or value is NULL.
 *
 *****************************************************************************/
static fm_status NvmRead32(fm_uintptr switchMem,
                           fm_uint    addr,
                           fm_uint32 *value)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "switchMem %p addr 0x%x value %p\n",
                 (void*)switchMem,
                 addr,
                 (void*)value);

    if ( ((fm_uint32*)switchMem == NULL) || (value == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    err = fm10000UtilSpiReadFlash(switchMem,
                                  RegRead32,
                                  RegWrite32,
                                  addr << 2,
                                  (fm_byte *)value,
                                  sizeof(fm_uint32),
                                  SPI_FREQ_KHZ);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end NvmRead32 */




/*****************************************************************************/
/** ReadLineFromNvmLtCfg
 * \ingroup intPlatform
 *
 * \desc            Read a line from Liberty Trail Config in NVM image.
 *
 * \param[in]       switchMem is pointer to mapped memory
 *
 * \param[in]       headerLtCfgBase is the adress of Liberty Trail Config in NVM
 *                  image.
 *
 * \param[in]       lengthInWords length of Liberty Trail Config in words
 *                  including the Liberty Trail Config header
 *
 * \param[in,out]   dataAddr is the address of line relative to Liberty Trail
 *                  Config base
 *
 * \param[out]      line read from Liberty Trail Config in NVM image
 *
 * \param[in,out]   word is a pointer to the value of the first word provided
 *                  that line begins in the middle of the word (not at word
 *                  boundaries. At the end it points to the last word value.
 *
 * \return          TRUE if successfully read line from Liberty Trail Config in
 *                  NVM image.
 * \return          FALSE otherwise.
 *
 *****************************************************************************/
static fm_bool ReadLineFromNvmLtCfg(fm_uint32 *switchMem,
                                    fm_uint    headerLtCfgBase,
                                    fm_uint    lengthInWords,
                                    fm_uint *  dataAddr,
                                    fm_text    line,
                                    fm_uint32 *word)
{
    unsigned int i;
    int          currentLineIndex;
    fm_bool      endOfLine;
    fm_status    err;
    fm_char     *pwordChar;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "switchMem %p headerLtCfgBase %u lengthInWords %u dataAddr %p"
                 " line %p word %p\n",
                 (void*)switchMem,
                 headerLtCfgBase,
                 lengthInWords,
                 (void*)dataAddr,
                 line,
                 (void*)word);

    currentLineIndex = 0;
    endOfLine        = FALSE;

    pwordChar = (fm_char*)word;

    if ( (*dataAddr % sizeof(fm_uint32)) > 0 )
    {
        i = *dataAddr % sizeof(fm_uint32);
        while (i < sizeof(fm_uint32))
        {
            if (pwordChar[i] == '\n')
            {
                line[currentLineIndex++] = '\0';
                endOfLine                = TRUE;
                break;
            }
            else
            {
                line[currentLineIndex++] = pwordChar[i++];
            }
        }
    }
    
    (*dataAddr) += currentLineIndex;
    
    if (endOfLine == TRUE)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, TRUE, "TRUE\n");
    }

    while ( (endOfLine == FALSE) &&
            (*dataAddr < lengthInWords * sizeof(fm_uint32)) )
    {
        err = NvmRead32((fm_uintptr)switchMem,
                        (headerLtCfgBase + *dataAddr) / sizeof(fm_uint32),
                        (fm_uint32*)word);
        if (err != FM_OK)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "FAIL: %s\n",
                         fmErrorMsg(err));
            break;
        }

        for (i = 0; i < sizeof(fm_uint32); i++)
        {
            if (pwordChar[i] == '\n')
            {
                line[currentLineIndex++] = '\0';
                endOfLine                = TRUE;
                break;
            }
            else
            {
                line[currentLineIndex++] = pwordChar[i];
            }
        }

        if (i == sizeof(fm_uint32))
        {
            (*dataAddr) += sizeof(fm_uint32);
        }
        else
        {
            (*dataAddr) += (i + 1);
        }
        
    }

    if (endOfLine)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, TRUE, "TRUE\n");
    }
    
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, FALSE, "FALSE\n");

}   /* ReadLineFromNvmLtCfg */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

    

/*****************************************************************************/
/** fmPlatformLoadPropertiesFromNVM
 * \ingroup intPlatform
 *
 * \desc            Loads properties into the properties subsystem by calling
 *                  the set and get methods on properties read in from NVM.
 *                                                                      \lb\lb
 *                  The expected format for the database is as follows:
 *                                                                      \lb\lb
 *                  [key] [type] [value]
 *                                                                      \lb\lb
 *                  Where key is a dotted string, type is one of int, bool or
 *                  float, and value is the value to set. Space is the only
 *                  valid separator, but multiple spaces are allowed.
 *
 * \param[in]       devName is the optional UIO device name.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if Liberty Trail Config Header is invalid.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformLoadPropertiesFromNVM(fm_text devName)
{
    fm_status    err;
    int          lineNo = 0;
    char         line[FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN];
    int          numAttr = 0;
    fm_status    status;
    fm_int       fd;
    void        *memmapAddr;
    fm_uint32   *switchMem;
    fm_timestamp start;
    fm_bool      isSpiPeripheralLockTaken;
    fm_uint32    bsmIntMask[2];
    fm_uint32    pollCnt;
    fm_uint32    value;
    fm_uint32    headerLtCfgBase;
    fm_uint32    headerLtCfgLen;
    fm_uint32    fileFmt;
    fm_uint32    version;
    fm_uint32    lengthInWords;
    fm_uint32    dataAddr;
    int          lineNumAttr;
    fm_bool      disabledBsmInterrupts;
    fm_int       size;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "%p\n", (void*)devName);

    isSpiPeripheralLockTaken = FALSE;
    disabledBsmInterrupts    = FALSE;

    err = fmPlatformMmapUioDevice(devName, &fd, &memmapAddr, &size);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    switchMem = (fm_uint32*) memmapAddr;

    err = fm10000UtilSpiPeripheralLock((fm_uintptr)switchMem,
                                       FM_SPI_PERIPHERAL_LOCK_OWNER_SWITCH_API,
                                       RegRead32,
                                       RegWrite32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    isSpiPeripheralLockTaken = TRUE;

    err = fm10000UtilsDisableBsmInterrupts((fm_uintptr)switchMem,
                                           bsmIntMask,
                                           RegRead32,
                                           RegWrite32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    disabledBsmInterrupts = TRUE;

    err = fmGetTime(&start);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    pollCnt = 1;

    err = NvmRead32((fm_uintptr)switchMem, NVM_HEADER_LT_CFG, &value);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    value = ntohl(value);  

    headerLtCfgLen  = FM_GET_FIELD(value, NVM_HEADER_LT_CFG, Length);
    headerLtCfgBase = FM_GET_FIELD(value, NVM_HEADER_LT_CFG, Base);

    if (headerLtCfgLen < NVM_LT_CFG_HEADER_LEN)
    {
        err = FM_FAIL;
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: LibertyTrail Config Length too small %d\n",
                     headerLtCfgLen);
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    } 

    if ( (headerLtCfgBase % sizeof(fm_uint32)) != 0 )
    {
        err = FM_FAIL;
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: LibertyTrail Config Length Base address 0x%x is not a mutiple of words\n",
                     headerLtCfgBase);
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }

    err = NvmRead32((fm_uintptr)switchMem,
                    headerLtCfgBase / sizeof(fm_uint32),
                    &value);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    value = ntohl(value);

    fileFmt       = FM_GET_FIELD(value, NVM_LT_CFG, FileFmt);
    version       = FM_GET_FIELD(value, NVM_LT_CFG, Version);
    lengthInWords = FM_GET_FIELD(value, NVM_LT_CFG, LengthInWords);

    if (fileFmt != NVM_LT_CFG_FILE_FORMAT_RAW)
    {
        err = FM_FAIL;
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: LibertyTrail Config file format %d is not supported\n",
                     fileFmt);
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }
   
    if (version > NVM_LT_CFG_VERSION_SUPPORTED)
    {
        err = FM_FAIL;
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: LibertyTrail Config version %d is not supported\n",
                     version);
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    } 

    if ( (lengthInWords * sizeof(fm_uint32)) < NVM_LT_CFG_HEADER_LEN )
    {
        err = FM_FAIL;
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "FAIL: LibertyTrail Config length %d is too small\n",
                     lengthInWords);
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }

    dataAddr = NVM_LT_CFG_HEADER_LEN;

    while (ReadLineFromNvmLtCfg((fm_uint32 *)switchMem,
                                headerLtCfgBase,
                                lengthInWords,
                                &dataAddr,
                                line,
                                &value))
    {
        lineNo++;
        lineNumAttr = 0;
        
        err = fmPlatformLoadAttributeFromLine(line, lineNo, &lineNumAttr);
        if (err == FM_OK)
        {
            numAttr += lineNumAttr;
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Error reading from line %d\n",
                         err);
        }
        
        err =
            fm10000UtilsPollBsmInterrupts((fm_uintptr)switchMem,
                                          RegRead32,
                                          RegWrite32,
                                          bsmIntMask,
                                          &start,
                                          &pollCnt,
                                          &disabledBsmInterrupts);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                "Loaded %d attributes from NVM\n",
                 numAttr);

ABORT:
    
    if (disabledBsmInterrupts == TRUE)
    {
        status =
            fm10000UtilsRestoreBsmInterrupts((fm_uintptr)switchMem,
                                             bsmIntMask,
                                             RegWrite32);
        if (status != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to restor BSM interrupts: error %d %s\n",
                         status,
                         fmErrorMsg(status));
            FM_ERR_COMBINE(err, status);
        }
    }

    if (isSpiPeripheralLockTaken == TRUE)
    {
        status = 
            fm10000UtilSpiPeripheralUnlock((fm_uintptr)switchMem,
                                           FM_SPI_PERIPHERAL_LOCK_OWNER_SWITCH_API,
                                           RegRead32,
                                           RegWrite32);
        if (status != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to release SPI peripheral lock: error %d %s\n",
                         status,
                         fmErrorMsg(status));
            FM_ERR_COMBINE(err, status);
        }
    }
    
    status = fmPlatformUnmapUioDevice(fd, memmapAddr, size);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Failed to unmap UIO device: error %d %s\n",
                     status,
                     fmErrorMsg(status));
        FM_ERR_COMBINE(err, status);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformLoadPropertiesFromNVM */
