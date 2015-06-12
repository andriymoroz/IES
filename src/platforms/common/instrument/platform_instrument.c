/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_instrument.c
 * Creation Date:   July 3, 2012
 * Description:     Platform Instrumentation function
 *
 * Copyright (c) 2006 - 2013, Intel Corporation
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
#include <platforms/common/instrument/platform_instrument.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
/* Must support a long path */
#define FM_INSTR_FILENAME_LENGTH                       1024

/**************************************************
 * A flag value that is written to the register
 * instrumentation log (if it is used at all)
 * to identify the log as big-endian or little-
 * endian.
 **************************************************/
#define FM_INSTR_ENDIAN_FLAG_ADDR                      0xfedcba98

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_char            instrumentationLogFilename[FM_INSTR_FILENAME_LENGTH];
static FILE *             instrumentationLog = NULL;



/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/** fmPlatformOpenInstrumentation
 * \ingroup intPlatformApp
 *
 * \desc            Open the instrumentation log file specified in the
 *                  environment variable INSTRUMENT_LOG_FILE.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmPlatformOpenInstrumentation()
{
#if INSTRUMENT_LOG_LEVEL > 0
    /* process environment variables for filename */
    fm_int  x = 0;
    fm_int  y;
    char    strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t strErrNum;

    if (getenv("INSTRUMENT_LOG_FILE"))
    {
        /* Copy the filename to local storage */
        strncpy(instrumentationLogFilename,
                 getenv("INSTRUMENT_LOG_FILE"),
                FM_INSTR_FILENAME_LENGTH - 2);

        /* The environment variable may have a trailing ':' separator,
         * strip it if that occurs */
        y = strlen(instrumentationLogFilename);

        if (instrumentationLogFilename[y - 1] == ':')
        {
            instrumentationLogFilename[y - 1] = '\0';
        }

        /* Make sure it's NULL terminated in case logFilename was too long. */
        instrumentationLogFilename[FM_INSTR_FILENAME_LENGTH - 1] = '\0';
    }
    else
    {
        return;
    }
#endif

#if INSTRUMENT_LOG_LEVEL == 2

    instrumentationLog = fopen(instrumentationLogFilename, "w");

    if (instrumentationLog == NULL)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                          "Unable to open instrumentation log file: %s (%s)!\n",
                          instrumentationLogFilename,
                          strErrBuf );
        }
        else
        {
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                          "Unable to open instrumentation log file: %s (%d)!\n",
                          instrumentationLogFilename,
                          errno );
        }
    }

#elif INSTRUMENT_LOG_LEVEL == 1

    instrumentationLog = fopen(instrumentationLogFilename, "wb");

    if (instrumentationLog != NULL)
    {
        /**************************************************
         * Write a flag value as the first entry that can
         * be used to identify the endianness of the log.
         **************************************************/

        INSTRUMENT_REG_WRITE(0, FM_INSTR_ENDIAN_FLAG_ADDR, 0);
    }
    else
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                          "Unable to open instrumentation log file: %s (%s)!\n",
                          instrumentationLogFilename,
                          strErrBuf );
        }
        else
        {
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                          "Unable to open instrumentation log file: %s (%d)!\n",
                          instrumentationLogFilename,
                          errno );
        }
    }

#endif

}   /* end fmPlatformOpenInstrumentation */




/*****************************************************************************/
/** fmPlatformCloseInstrumentation
 * \ingroup intPlatformApp
 *
 * \desc            Close the instrumentation log file.
 *
 * \param[in]       None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmPlatformCloseInstrumentation()
{
    /**************************************************
     * If log is open, close it.
     **************************************************/

    if (instrumentationLog != NULL)
    {
        fclose(instrumentationLog);
        instrumentationLog = NULL;
    }

}   /* end fmPlatformCloseInstrumentation */


#if INSTRUMENT_LOG_LEVEL
/*****************************************************************************/
/* fmPlatformInstrumentWriteCSR
 * \ingroup intPlatformApp
 *
 * \desc            Write a log file of every register word-wise write
 *                                                                      \lb\lb
 *                  When INSTRUMENT_LOG_LEVEL is defined as 1, output will be
 *                  binary, 4 bytes of address followed by 4 bytes of value.
 *                  The order of the bytes within each 4-byte word is machine
 *                  dependent. On a little-endian machine, a word value of
 *                  12345678 will appear in the file as 78 56 34 12.
 *                                                                      \lb\lb
 *                  When INSTRUMENT_LOG_LEVEL is defined as 2, output will be
 *                  ASCII, one write event per line, register address
 *                  followed by value.
 *
 * \param[in]       sw is the switch on which to operate (ignored).
 *
 * \param[in]       addr is the register address.
 *
 * \param[in]       newValue is the value to be written to the register.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
void fmPlatformInstrumentWriteCSR(fm_int sw, fm_uint32 addr, fm_uint32 newValue)
{
    char    strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t strErrNum;

    FM_NOT_USED(sw);

    if (instrumentationLog != NULL)
    {
#if INSTRUMENT_LOG_LEVEL == 2
        fprintf(instrumentationLog, "%08x %08x\n", addr, newValue);
#elif INSTRUMENT_LOG_LEVEL == 1

        if ( !fwrite(&addr, sizeof(fm_uint32), 1, instrumentationLog) )
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {            
                FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                              "Unable to write to file: %s (%s)!\n",
                              instrumentationLogFilename,
                              strErrBuf );
            }
            else
            {
                FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                              "Unable to write to file: %s (%d)!\n",
                              instrumentationLogFilename,
                              errno ); 
            }
        }

        if ( !fwrite(&newValue, sizeof(fm_uint32), 1, instrumentationLog) )
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            { 
                FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                              "Unable to write to file: %s (%s)!\n",
                              instrumentationLogFilename,
                              strErrBuf );
            }
            else
            { 
                FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                              "Unable to write to file: %s (%d)!\n",
                              instrumentationLogFilename,
                              errno );
            }
        }

#endif
    }

    return;

}   /* end fmPlatformInstrumentWriteCSR */
#endif  /* INSTRUMENT_LOG_LEVEL */


