/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_logging.c
 * Creation Date:   June 18, 2007
 * Description:     SDK logging facility implementation.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#define LOG_INITIALIZED(ls) \
    ( (ls) && (ls)->initMagicNumber == FM_LOG_MAGIC_NUMBER )

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_bool ApplyLoggingFilter(const char *haystack, const char *needle);
static fm_status LogMessage( fm_uint64   filteredCategoryMask,
                             fm_uint64   category,
                             fm_uint64   logLevel,
                             const char *srcFile,
                             const char *srcFunction,
                             fm_uint32   srcLine,
                             const char *format,
                             va_list     ap );

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** ApplyLoggingFilter
 * \ingroup intLogging
 *
 * \desc            Determine whether a (partial) match can be found for the
 *                  string needle in the set of (partial) filter expressions
 *                  haystack.
 *
 * \param[in]       haystack points to the set of (partial) filter expressions
 *
 * \param[in]       needle points to the string for a match is to be found
 *
 * \return          TRUE if a match is found
 * \return          FALSE otherwise
 *
 *****************************************************************************/
static fm_bool ApplyLoggingFilter(const char *haystack, const char *needle)
{
    fm_bool found = FALSE;
    char    buffer[FM_LOG_MAX_FILTER_LEN];
    char *  end;
    char *  token;

    /***************************************************
     * The strtok_r function CANNOT be used on constant
     * strings. To prevent depending on logic that is
     * outside the scope of this function, a local,
     * non-constant copy of haystack is made here.
     **************************************************/
    FM_STRNCPY_S(buffer, sizeof(buffer), haystack, sizeof(buffer) );

    buffer[sizeof(buffer) - 1] = '\0';

    token = strtok_r(buffer, " ,", &end);

    while (token)
    {
        if (strstr(needle, token))
        {
            found = TRUE;
            break;
        }

        token = strtok_r(NULL, " ,", &end);
    }

    return found;

}   /* end ApplyLoggingFilter */




/*****************************************************************************/
/** LogMessage
 * \ingroup intLogging
 *
 * \desc            Internal version of ''fmLogMessage'' and ''fmLogMessageV2''
 * 
 * \param[in]       filteredCategoryMask is the bitmask representing enabled
 *                  logging categories when this function is invoked from
 *                  ''fmLogMessageV2'' or is the bitmask representing enabled 
 *                  categories for which legacy logging is enabled when invoked
 *                  from ''fmLogMessage''.
 * 
 * \param[in]       categories is a bitmask of category flags (see
 *                  ''Log Categories'') which indicate which categories this
 *                  log message belongs to.
 *
 * \param[in]       logLevel is a bitmask of level flags (see
 *                  ''Log Levels'') which indicate which log levels this
 *                  log message belongs to.
 *
 * \param[in]       srcFile is the name of the source code file generating
 *                  the log message. Usually invoked as __FILE__.
 *
 * \param[in]       srcFunction is the name of the source code function
 *                  generating the log message. Usually invoked as __func__.
 *
 * \param[in]       srcLine is the source code line number generating the
 *                  log message. Usually invoked as __LINE__.
 *
 * \param[in]       format is the printf-style format.
 *
 * \param[in]       ap is the printf var-args argument list.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status LogMessage( fm_uint64   filteredCategoryMask,
                             fm_uint64   categories,
                             fm_uint64   logLevel,
                             const char *srcFile,
                             const char *srcFunction,
                             fm_uint32   srcLine,
                             const char *format,
                             va_list     ap)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_loggingType   logType;
    fm_bool          enabled;
    fm_char          dateStr[64];
    fm_char          timeStampStr[64];
    fm_timestamp     ts;
    fm_text          threadName = NULL;
    fm_char          threadStr[64];
    FILE *           log;
    fm_char          logLevelStr[64];
    fm_char          callBackStr[1024];
    fm_uint64        levelMask;
    fm_text          functionFilter;
    fm_text          fileFilter;
    fm_uint32        verbosityMask;
    fm_bool          filter;
    fm_text          levelStr;
    fm_bool          bypassFilter = FALSE;
    int              posixErr;
    fm_bool          isMasterProcess = TRUE; 
    fm_status        status; 

    if ( LOG_INITIALIZED(ls) )
    {
        levelMask      = ls->levelMask;
        logType        = ls->logType;
        enabled        = ls->enabled;
        functionFilter = ls->functionFilter;
        fileFilter     = ls->fileFilter;
        verbosityMask  = ls->verbosityMask;
    }
    else
    {
        /* use defaults if the logging facility is not initialized */
        levelMask    = FM_LOG_LEVEL_FATAL |
                       FM_LOG_LEVEL_ERROR |
                       FM_LOG_LEVEL_WARNING;
        logType        = FM_LOG_TYPE_CONSOLE;
        enabled        = TRUE;
        functionFilter = NULL;
        fileFilter     = NULL;
        verbosityMask  = ~0;
    }

    if (logLevel & FM_LOG_LEVEL_DEFAULT )
    {
        /**************************************************
         * Make sure FM_LOG_PRINT shows up unconditionally
         * and unadorned and that error messages aren't 
         * inadvertently suppressed 
         **************************************************/
        
        if ( logLevel & FM_LOG_LEVEL_PRINT )
        {
            verbosityMask         = 0;
            categories            = FM_LOG_CAT_LOGGING;
        }

        enabled               = TRUE;
        levelMask             = logLevel;
        filteredCategoryMask  = FM_LOG_CAT_ALL;
        bypassFilter          = TRUE;
    }

    /**************************************************
     * Generate log output if the log level is set to
     * the print unconditionally, or if the logging
     * system is enabled and we have a category and
     * level match.
     **************************************************/

    if ( enabled &&
         (filteredCategoryMask & categories) != 0 &&
         ( (levelMask & logLevel) == logLevel ) )
    {

        if (verbosityMask & FM_LOG_VERBOSITY_THREAD)
        {
            /* Get thread name before taking logging lock, to avoid possible
             * deadly embrace between thread and logging code. */
            threadName = fmGetCurrentThreadName();
        }

        if ( LOG_INITIALIZED(ls) )
        {
            posixErr = pthread_mutex_lock( (pthread_mutex_t *) ls->accessLock );
            if (posixErr != 0)
            {
                FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNABLE_TO_LOCK);
            }
        }

        /* check secondary filters */
        filter = TRUE;

        if (!bypassFilter)
        {
            if (filter && functionFilter && *functionFilter != '\0')
            {
                filter = ApplyLoggingFilter(functionFilter, srcFunction);
            }
    
            if (filter && fileFilter && *fileFilter != '\0')
            {
                filter = ApplyLoggingFilter(fileFilter, srcFile);
            }
        }
        
        if (filter)
        {
            /**************************************************
             * Identify the log level at which this log message
             * was generated.
             **************************************************/
             
            if (verbosityMask & FM_LOG_VERBOSITY_LOG_LEVEL)
            {
                switch (logLevel)
                {
                    case FM_LOG_LEVEL_FUNC_ENTRY:
                    case FM_LOG_LEVEL_FUNC_ENTRY_API:
                    case FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE:
                    case FM_LOG_LEVEL_FUNC_ENTRY_API_VERBOSE:
                        levelStr = "ENTRY";
                        break;
                        
                    case FM_LOG_LEVEL_FUNC_EXIT:
                    case FM_LOG_LEVEL_FUNC_EXIT_API:
                    case FM_LOG_LEVEL_FUNC_EXIT_VERBOSE:
                    case FM_LOG_LEVEL_FUNC_EXIT_API_VERBOSE:
                        levelStr = "EXIT";
                        break;
                        
                    case FM_LOG_LEVEL_WARNING:
                        levelStr = "WARNING";
                        break;
                    
                    case FM_LOG_LEVEL_ERROR:
                        levelStr = "ERROR";
                        break;
                        
                    case FM_LOG_LEVEL_FATAL:
                        levelStr = "FATAL";
                        break;
                        
                    case FM_LOG_LEVEL_INFO:
                        levelStr = "INFO";
                        break;
                        
                    case FM_LOG_LEVEL_DEBUG:
                    case FM_LOG_LEVEL_DEBUG_VERBOSE:
                        levelStr = "DEBUG";
                        break;
                        
                    case FM_LOG_LEVEL_PRINT:
                        levelStr = "PRINT";
                        break;
                        
                    case FM_LOG_LEVEL_DEBUG2:
                        levelStr = "DEBUG2";
                        break;
                        
                    case FM_LOG_LEVEL_DEBUG3:
                        levelStr = "DEBUG3";
                        break;
                        
                    case FM_LOG_LEVEL_ASSERT:
                        levelStr = "ASSERT";
                        break;
                        
                    default:
                        levelStr = "UNKNOWN_LVL";
                        break;
                        
                }   /* end switch (logLevel) */
                
                FM_SNPRINTF_S(logLevelStr, sizeof(logLevelStr), "%s", levelStr);
            }

            if (verbosityMask & FM_LOG_VERBOSITY_DATE_TIME)
            {
                fmGetFormattedTime(dateStr);
            }

            if (verbosityMask & FM_LOG_VERBOSITY_TIMESTAMP)
            {
                fmGetTime(&ts);
                FM_SNPRINTF_S(timeStampStr, sizeof(logLevelStr),
                              "%u.%04u",
                              ts.sec,
                              ts.usec / 100);
            }


            if (verbosityMask & FM_LOG_VERBOSITY_THREAD)
            {
                if (threadName == NULL)
                {
                    FM_SNPRINTF_S( threadStr,
                                   sizeof(threadStr),
                                   "<%p>",
                                   fmGetCurrentThreadId() );
                    threadName = threadStr;
                }
            }
            else
            {
                threadStr[0] = '\0';
                threadName = threadStr;
            }

            switch (logType)
            {
                case FM_LOG_TYPE_CONSOLE:

                    if (verbosityMask & FM_LOG_VERBOSITY_DATE_TIME)
                    {
                        FM_PRINTF_S("%s:", dateStr);
                    }

                    if (verbosityMask & FM_LOG_VERBOSITY_TIMESTAMP)
                    {
                        FM_PRINTF_S("%s:", timeStampStr);
                    }

                    if (verbosityMask & FM_LOG_VERBOSITY_LOG_LEVEL)
                    {
                        FM_PRINTF_S("%s:", logLevelStr);
                    }

                    if (verbosityMask & FM_LOG_VERBOSITY_THREAD)
                    {
                        FM_PRINTF_S("%s:", threadName);
                    }

                    if (verbosityMask & FM_LOG_VERBOSITY_FILE)
                    {
                        FM_PRINTF_S("%s:", srcFile);
                    }

                    if (verbosityMask & FM_LOG_VERBOSITY_FUNC)
                    {
                        FM_PRINTF_S("%s:", srcFunction);
                    }

                    if (verbosityMask & FM_LOG_VERBOSITY_LINE)
                    {
                        FM_PRINTF_S("%d:", srcLine);
                    }

                    FM_VPRINTF_S(format, ap);
                    fflush(stdout);
                    break;

                case FM_LOG_TYPE_FILE:
                    log = fopen(ls->logFileName, "at");

                    if (log)
                    {
                        if (verbosityMask & FM_LOG_VERBOSITY_DATE_TIME)
                        {
                            FM_FPRINTF_S(log, "%s:", dateStr);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_TIMESTAMP)
                        {
                            FM_FPRINTF_S(log, "%s:", timeStampStr);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_LOG_LEVEL)
                        {
                            FM_FPRINTF_S(log, "%s:", logLevelStr);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_THREAD)
                        {
                            FM_FPRINTF_S(log, "%s:", threadName);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_FILE)
                        {
                            FM_FPRINTF_S(log, "%s:", srcFile);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_FUNC)
                        {
                            FM_FPRINTF_S(log, "%s:", srcFunction);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_LINE)
                        {
                            FM_FPRINTF_S(log, "%d:", srcLine);
                        }

                        FM_VFPRINTF_S(log, format, ap);
                        fclose(log);
                    }
                    else
                    {
                        /* Unable to open log file. */
                        FM_LOG_ERROR(FM_LOG_CAT_LOGGING,
                                     "Unable to open logfile %s for appending",
                                     ls->logFileName);

                    }   /* end if (log) */

                    break;

                case FM_LOG_TYPE_MEMBUF:
                    {
                        fm_int lineLen;
                        fm_int elemLen;
                        lineLen = 0;

                        if (verbosityMask & FM_LOG_VERBOSITY_DATE_TIME)
                        {
                            if ( (elemLen = FM_SNPRINTF_S(ls->logBuffer[ls->currentPos],
                                                          FM_LOG_MAX_LINE_SIZE,
                                                          "%s:",
                                                          dateStr) ) > 0 )
                            {
                                lineLen += elemLen;
                            }

                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_TIMESTAMP)
                        {
                            if ( (elemLen = FM_SNPRINTF_S(ls->logBuffer[ls->currentPos],
                                                          FM_LOG_MAX_LINE_SIZE,
                                                          "%s:",
                                                          timeStampStr) ) > 0 )
                            {
                                lineLen += elemLen;
                            }

                        }

                        if ( (verbosityMask & FM_LOG_VERBOSITY_LOG_LEVEL) &&
                             lineLen < FM_LOG_MAX_LINE_SIZE )
                        {
                            if ( (elemLen = FM_SNPRINTF_S(&ls->logBuffer[ls->currentPos][lineLen],
                                                          FM_LOG_MAX_LINE_SIZE - lineLen,
                                                          "%s:",
                                                          logLevelStr) ) > 0 )
                            {
                                lineLen += elemLen;
                            }

                        }

                        if ( (verbosityMask & FM_LOG_VERBOSITY_THREAD) &&
                             lineLen < FM_LOG_MAX_LINE_SIZE )
                        {
                            if ( (elemLen = FM_SNPRINTF_S(&ls->logBuffer[ls->currentPos][lineLen],
                                                          FM_LOG_MAX_LINE_SIZE - lineLen,
                                                          "%s:",
                                                          threadName) ) > 0 )
                            {
                                lineLen += elemLen;
                            }

                        }

                        if ( (verbosityMask & FM_LOG_VERBOSITY_FILE) &&
                             lineLen < FM_LOG_MAX_LINE_SIZE )
                        {
                            if ( (elemLen = FM_SNPRINTF_S(&ls->logBuffer[ls->currentPos][lineLen],
                                                          FM_LOG_MAX_LINE_SIZE - lineLen,
                                                          "%s:",
                                                          srcFile) ) > 0 )
                            {
                                lineLen += elemLen;
                            }

                        }

                        if ( (verbosityMask & FM_LOG_VERBOSITY_FUNC) &&
                             lineLen < FM_LOG_MAX_LINE_SIZE )
                        {
                            if ( (elemLen = FM_SNPRINTF_S(&ls->logBuffer[ls->currentPos][lineLen],
                                                          FM_LOG_MAX_LINE_SIZE - lineLen,
                                                          "%s:",
                                                          srcFunction) ) > 0 )
                            {
                                lineLen += elemLen;
                            }

                        }
 
                        if ( (verbosityMask & FM_LOG_VERBOSITY_LINE) &&
                             lineLen < FM_LOG_MAX_LINE_SIZE )
                        {
                            if ( (elemLen = FM_SNPRINTF_S(&ls->logBuffer[ls->currentPos][lineLen],
                                                          FM_LOG_MAX_LINE_SIZE - lineLen,
                                                          "%d:",
                                                          srcLine) ) > 0 )
                            {
                                lineLen += elemLen;
                            }
 
                        }
 
                        if (lineLen < FM_LOG_MAX_LINE_SIZE)
                        {
                            FM_VSNPRINTF_S(&ls->logBuffer[ls->currentPos][lineLen],
                                           FM_LOG_MAX_LINE_SIZE - lineLen,
                                           format,
                                           ap);
                        }

                        if (++ls->currentPos >= FM_LOG_MAX_LINES)
                        {
                            ls->currentPos = 0;

                            FM_LOG_INFO(FM_LOG_CAT_LOGGING,
                                        "-- Log Buffer Wrapped --\n");
                        }

                        break;
                    }

                case FM_LOG_TYPE_CALLBACK:

                    status = fmIsMasterProcess(&isMasterProcess);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_LOGGING,status);

                    if (ls->fmLogCallback && isMasterProcess)
                    {
                        if (verbosityMask & FM_LOG_VERBOSITY_DATE_TIME)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%s:",
                                          dateStr);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_TIMESTAMP)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%s:",
                                          timeStampStr);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_LOG_LEVEL)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%s:",
                                          logLevelStr);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_THREAD)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%s:",
                                          threadName);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_FILE)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%s:",
                                          srcFile);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_FUNC)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%s:",
                                          srcFunction);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        if (verbosityMask & FM_LOG_VERBOSITY_LINE)
                        {
                            FM_SNPRINTF_S(callBackStr,
                                          sizeof(callBackStr),
                                          "%d:",
                                          srcLine);
                            ls->fmLogCallback(callBackStr,
                                              ls->fmLogCookie1,
                                              ls->fmLogCookie2);
                        }

                        FM_VSNPRINTF_S(callBackStr, sizeof(callBackStr), format, ap);
                        ls->fmLogCallback(callBackStr,
                                          ls->fmLogCookie1,
                                          ls->fmLogCookie2);

                    }   /* end if (ls->fmLogCallback) */

                    break;

            }   /* end switch (logType) */

        }   /* end if (filter) */

        if ( LOG_INITIALIZED(ls) )
        {
            posixErr = pthread_mutex_unlock( (pthread_mutex_t *) ls->accessLock );
            if (posixErr != 0)
            {
                FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNABLE_TO_UNLOCK);
            }
        }


    }

    return FM_OK;

}   /* end fmLogMessage */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAlosLoggingInit
 * \ingroup intLogging
 *
 * \desc            Initializes the logging subsystem.
 *
 * \note            This is an internal function that should only be called
 *                  from fm_alos_init.c.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmAlosLoggingInit(void)
{
    fm_loggingState *   ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    pthread_mutexattr_t attr;
    fm_int              i;

    if (ls == NULL)
    {
        return FM_ERR_UNINITIALIZED;
    }

    /* clear state */
    FM_CLEAR(*ls);

    ls->logType       = FM_LOG_TYPE_CONSOLE;
    ls->enabled       = TRUE;
    ls->verbosityMask = FM_LOG_VERBOSITY_DEFAULT;

    /* initially dump anything fatal from anyone */
    ls->categoryMask = FM_LOG_CAT_DEFAULT;
    ls->levelMask    = FM_LOG_LEVEL_DEFAULT;

    /* make sure all object ID filters are pass-through */
    ls->legacyLoggingMask = FM_LOG_CAT_ALL;
    for (i = 0 ; i < 64  ; i++)
    {
        ls->range[i].rangeType = FM_LOG_RANGE_TYPE_INFINITE;
    }

    ls->functionFilter[0] = 0;
    ls->fileFilter[0] = 0;

    if ( pthread_mutexattr_init(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LOGGING, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) ||
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        pthread_mutexattr_destroy(&attr);
        FM_LOG_EXIT(FM_LOG_CAT_LOGGING, FM_ERR_LOCK_INIT);
    }

    ls->accessLock = fmAlloc( sizeof(pthread_mutex_t) );
    if (ls->accessLock == NULL)
    {
        pthread_mutexattr_destroy(&attr);
        FM_LOG_EXIT(FM_LOG_CAT_LOGGING, FM_ERR_NO_MEM);
    }
    if (pthread_mutex_init( (pthread_mutex_t *) ls->accessLock, &attr ) != 0)
    {
        pthread_mutexattr_destroy(&attr);
        FM_LOG_EXIT(FM_LOG_CAT_LOGGING, FM_ERR_LOCK_INIT);
    }

    pthread_mutexattr_destroy(&attr);

    /* set fault injection state */
#if defined(FM_ALOS_FAULT_INJECTION_POINTS) && (FM_ALOS_FAULT_INJECTION_POINTS==FM_ENABLED)
    ls->failFunction[0] = 0;
    ls->failError = FM_OK;
#endif

    /* indicate that we have finished initializing */
    ls->initMagicNumber = FM_LOG_MAGIC_NUMBER;

    return FM_OK;

}   /* end fmAlosLoggingInit */




/*****************************************************************************/
/** fmLoggingEnable
 * \ingroup alosLog
 *
 * \desc            Enable the logging subsystem (enabled by default).
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmLoggingEnable()
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "\n");

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->enabled = TRUE;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmLoggingEnable */




/*****************************************************************************/
/** fmLoggingDisable
 * \ingroup alosLog
 *
 * \desc            Disable the logging subsystem.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmLoggingDisable()
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING, "\n");

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->enabled = FALSE;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmLoggingDisable */




/*****************************************************************************/
/** fmSetLoggingVerbosity
 * \ingroup alosLog
 *
 * \desc            Each log message is output with a preamble comprising
 *                  a time stamp and the location in the source code from
 *                  which the log message was generated. The preamble can make
 *                  the total log message quite long. This function provides
 *                  control over the several components of the preamble.
 *
 * \param[in]       verbosityMask is a bit mask comprising a set of flag bits,
 *                  one for each component of the preamble, that may be ORed
 *                  together in any combination. See ''Log Verbosity Flags''
 *                  for a list of flag bits.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmSetLoggingVerbosity(fm_uint32 verbosityMask)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "verbosityMask=%08x\n",
                     verbosityMask);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->verbosityMask = verbosityMask;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmSetLoggingVerbosity */




/*****************************************************************************/
/** fmGetLoggingVerbosity
 * \ingroup alosLog
 *
 * \desc            Retrieve the current log message preamble verbosity mask.
 *
 * \param[in]       verbosityMask points to caller-allocated storage where 
 *                  this function should place the current verbosity bit 
 *                  mask. See ''Log Verbosity Flags'' for the meaning of
 *                  each bit.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmGetLoggingVerbosity(fm_uint32 *verbosityMask)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "verbosityMask=%p\n",
                     (void *) verbosityMask);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    *verbosityMask = ls->verbosityMask;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmGetLoggingVerbosity */




/*****************************************************************************/
/** fmSetLoggingType
 * \ingroup alosLog
 *
 * \desc            Set the logging destination to the given type.  The clear
 *                  argument allows logging to continue appending to a previously
 *                  created type.
 *
 * \param[in]       logType is the destination type (see ''fm_loggingType'').
 *
 * \param[in]       clear should be TRUE to reset the destination, otherwise
 *                  logging will be appended to the last position.
 *
 * \param[in]       arg is a parameter for the type (dependent on the type).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmSetLoggingType(fm_loggingType logType,
                           fm_bool        clear,
                           void *         arg)
{
    FILE *              log;
    fm_status           err;
    fm_logCallBackSpec *cbSpec;
    fm_loggingState *   ls;
    int                 posixErr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "logType=%d clear=%d arg=%p\n",
                     logType,
                     clear,
                     arg);

    err = FM_OK;
    ls  = (fmRootAlos != NULL) ? &fmRootAlos->fmLoggingState : NULL;

    if ( LOG_INITIALIZED(ls) )
    {
        posixErr = pthread_mutex_lock( (pthread_mutex_t *) ls->accessLock );
        if (posixErr != 0)
        {
            FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNABLE_TO_LOCK);
        }

        ls->logType = logType;

        switch (logType)
        {
            case FM_LOG_TYPE_CONSOLE:
                break;

            case FM_LOG_TYPE_FILE:

                if (clear)
                {
                    fmStringCopy(ls->logFileName, (fm_text) arg,
                                 FM_MAX_FILENAME_LENGTH);

                    /* Create the file to empty it */
                    log = fopen(ls->logFileName, "wt");

                    if (log)
                    {
                        fclose(log);
                    }
                    else
                    {
                        /* In the event of a failure, default to console */
                        ls->logType = FM_LOG_TYPE_CONSOLE;

                        FM_LOG_ERROR(FM_LOG_CAT_LOGGING,
                                     "Unable to create logfile %s",
                                     ls->logFileName);
                    }
                }

                break;

            case FM_LOG_TYPE_MEMBUF:

                if (clear)
                {
                    ls->maxLines = FM_LOG_MAX_LINES;

                    FM_CLEAR(ls->logBuffer);
                    ls->currentPos = 0;
                }

                break;

            case FM_LOG_TYPE_CALLBACK:
                cbSpec = (fm_logCallBackSpec *) arg;

                ls->fmLogCallback = cbSpec->callBack;
                ls->fmLogCookie1  = cbSpec->cookie1;
                ls->fmLogCookie2  = cbSpec->cookie2;

                break;

        }   /* end switch (logType) */

        posixErr = pthread_mutex_unlock( (pthread_mutex_t *) ls->accessLock );
        if (posixErr != 0)
        {
            FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNABLE_TO_UNLOCK);
        }
    }
    else
    {
        err = FM_ERR_UNINITIALIZED;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, err);

}   /* end fmSetLoggingType */




/*****************************************************************************/
/** fmEnableLoggingCategory
 * \ingroup alosLog
 *
 * \desc            Enables logging for specific log categories.
 *
 * \param[in]       categories is a bitmask of category flags (see
 *                  ''Log Categories'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmEnableLoggingCategory(fm_uint64 categories)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "categories=%15" FM_FORMAT_64 "u\n",
                     categories);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->categoryMask |= categories;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmEnableLoggingCategory */




/*****************************************************************************/
/** fmDisableLoggingCategory
 * \ingroup alosLog
 *
 * \desc            Disables logging for specific log categories.
 *
 * \param[in]       categories is a bitmask of category flags (see
 *                  ''Log Categories'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmDisableLoggingCategory(fm_uint64 categories)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "categories=%15" FM_FORMAT_64 "u\n",
                     categories);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->categoryMask &= ~categories;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmDisableLoggingCategory */




/*****************************************************************************/
/** fmEnableLoggingLevel
 * \ingroup alosLog
 *
 * \desc            Enables logging for specific log levels.
 *
 * \param[in]       levels is a bitmask of level flags (see
 *                  ''Log Levels'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmEnableLoggingLevel(fm_uint64 levels)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "levels=%15" FM_FORMAT_64 "u\n",
                     levels);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->levelMask |= levels;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmEnableLoggingLevel */




/*****************************************************************************/
/** fmDisableLoggingLevel
 * \ingroup alosLog
 *
 * \desc            Disables logging for specific log levels.
 *
 * \param[in]       levels is a bitmask of level flags (see
 *                  ''Log Levels'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmDisableLoggingLevel(fm_uint64 levels)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "levels=%15" FM_FORMAT_64 "u\n",
                     levels);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->levelMask &= ~levels;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmDisableLoggingLevel */




/*****************************************************************************/
/** fmSetLoggingFilter
 * \ingroup alosLog
 *
 * \desc            Set the logging filter by category, level, source code
 *                  function name and source code filename.
 *
 * \param[in]       categoryMask is a bitmask of category flags (see
 *                  ''Log Categories'').
 *
 * \param[in]       levelMask is a bitmask of level flags (see
 *                  ''Log Levels'').
 *
 * \param[in]       functionFilter is the name of the source code function
 *                  on which to filter, or NULL to not filter by function
 *                  name.
 *
 * \param[in]       fileFilter is the name of the source code file
 *                  on which to filter, or NULL to not filter by file name.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmSetLoggingFilter(fm_uint64   categoryMask,
                             fm_uint64   levelMask,
                             const char *functionFilter,
                             const char *fileFilter)
{
    int              posixErr;
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "categoryMask=%15" FM_FORMAT_64 "u, "
                     "levelMask=%15" FM_FORMAT_64 "u, "
                     "functionFilter=%s, "
                     "fileFilter=%s\n",
                     categoryMask,
                     levelMask,
                     functionFilter,
                     fileFilter);

    ls = (fmRootAlos != NULL) ? &fmRootAlos->fmLoggingState : NULL;

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    if ( LOG_INITIALIZED(ls) )
    {
        posixErr = pthread_mutex_lock( (pthread_mutex_t *) ls->accessLock );
        if (posixErr != 0)
        {
            FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNABLE_TO_LOCK);
        }
    }

    ls->categoryMask = categoryMask;
    ls->levelMask    = levelMask;

    if (functionFilter && (strcmp(functionFilter, "") != 0))
    {
        fmStringCopy(ls->functionFilter, functionFilter, FM_LOG_MAX_FILTER_LEN);
    }
    else
    {
        ls->functionFilter[0] = 0;
    }

    if (fileFilter && (strcmp(fileFilter, "") != 0))
    {
        fmStringCopy(ls->fileFilter, fileFilter, FM_LOG_MAX_FILTER_LEN);
    }
    else
    {
        ls->fileFilter[0] = 0;
    }

    if ( LOG_INITIALIZED(ls) )
    {
        posixErr = pthread_mutex_unlock( (pthread_mutex_t *) ls->accessLock );
        if (posixErr != 0)
        {
            FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNABLE_TO_UNLOCK);
        }
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmSetLoggingFilter */




/*****************************************************************************/
/** fmLogMessage
 * \ingroup alosLog
 *
 * \desc            Generate a log message if all filter criteria are met.
 *                  This function is normally invoked by helper macros.
 *
 * \note            This function may be called prior to the logging subsystem
 *                  being initialized. In that case, logging output will be to
 *                  the console only.
 *
 * \param[in]       categories is a bitmask of category flags (see
 *                  ''Log Categories'') which indicate which categories this
 *                  log message belongs to.
 *
 * \param[in]       logLevel is a bitmask of level flags (see
 *                  ''Log Levels'') which indicate which log levels this
 *                  log message belongs to.
 *
 * \param[in]       srcFile is the name of the source code file generating
 *                  the log message. Usually invoked as __FILE__.
 *
 * \param[in]       srcFunction is the name of the source code function
 *                  generating the log message. Usually invoked as __func__.
 *
 * \param[in]       srcLine is the source code line number generating the
 *                  log message. Usually invoked as __LINE__.
 *
 * \param[in]       format is the printf-style format.
 *
 * \param[in]       ... is the printf var-args argument list.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmLogMessage( fm_uint64   categories,
                        fm_uint64   logLevel,
                        const char *srcFile,
                        const char *srcFunction,
                        fm_uint32   srcLine,
                        const char *format,
                        ... )
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_uint64        filteredCategoryMask;
    fm_status        status;
    va_list          ap;

    /* get out if logging is disabled */
    if (LOG_INITIALIZED(ls) && !ls->enabled)
    {
        return FM_OK;
    }

    if ( LOG_INITIALIZED(ls) )
    {
        /* mask out enabled categories for which legacy logging is disabled*/
        filteredCategoryMask   = ls->categoryMask & ls->legacyLoggingMask;
    }
    else
    {
        /* use defaults if the logging facility is not initialized */
        filteredCategoryMask   = FM_LOG_CAT_ALL;
    }

    va_start(ap, format);
    status = LogMessage( filteredCategoryMask,
                         categories,           
                         logLevel,           
                         srcFile,            
                         srcFunction,        
                         srcLine,            
                         format,             
                         ap );              

    va_end(ap);
    return status;

}   /* end LogMessage()*/




/*****************************************************************************/
/** fmLogBufferDump
 * \ingroup alosLog
 *
 * \desc            Dump the in-memory log buffer, potentially filtering by
 *                  any of the given arguments.
 *
 * \note            The in-memory log buffer is enabled by calling
 *                  ''fmSetLoggingType'' with a ''fm_loggingType'' argument
 *                  of ''FM_LOG_TYPE_MEMBUF''.
 *
 * \param[in]       threadID is the thread ID to filter by, or NULL to
 *                  not filter by thread ID.
 *
 * \param[in]       srcFile is the source code filename to filter by,
 *                  or NULL to not filter by filename.
 *
 * \param[in]       srcFunction is the source code function to filter by, or
 *                  NULL not filter by function.
 *
 * \param[in]       srcLine is the source code line number to filter by,
 *                  or -1 to not filter by line number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmLogBufferDump(void *    threadID,
                          fm_text   srcFile,
                          fm_text   srcFunction,
                          fm_uint32 srcLine)
{
    fm_loggingState *ls    = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_bool          valid = TRUE;
    fm_char          idTmp[16], lineTmp[16];

    if ( !LOG_INITIALIZED(ls) )
    {
        return FM_ERR_UNINITIALIZED;
    }

    if (threadID)
    {
        FM_SNPRINTF_S(idTmp, sizeof(idTmp), "<%p", threadID);
    }

    if (srcLine > 0)
    {
        FM_SNPRINTF_S(lineTmp, sizeof(lineTmp), "%d", srcLine);
    }

    for (int i = 0 ; i < FM_LOG_MAX_LINES ; i++)
    {
        valid = TRUE;

        if (strlen(ls->logBuffer[i]) > 0)
        {
            if ( threadID && !strstr(ls->logBuffer[i], idTmp) )
            {
                valid = FALSE;
            }

            if ( srcFunction && !strstr(ls->logBuffer[i], srcFunction) )
            {
                valid = FALSE;
            }

            if ( srcFile && !strstr(ls->logBuffer[i], srcFile) )
            {
                valid = FALSE;
            }

            if ( (srcLine > 0) && !strstr(ls->logBuffer[i], lineTmp) )
            {
                valid = FALSE;
            }

            if (valid)
            {
                printf("%s", ls->logBuffer[i]);
            }
        }
    }

    return FM_OK;

}   /* end fmLogBufferDump */




/*****************************************************************************/
/** fmGetLogBuffer
 * \ingroup alosLog
 *
 * \desc            Copy the in-memory log buffer to a caller-supplied
 *                  character buffer
 *
 * \note            The in-memory log buffer is enabled by calling
 *                  ''fmSetLoggingType'' with a ''fm_loggingType'' argument
 *                  of ''FM_LOG_TYPE_MEMBUF''.
 *
 * \param[in]       buffer points to caller-allocated memory where the 
 *                  memory-based logging output may be copied.
 *
 * \param[in]       size is the size in bytes of buffer.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmGetLogBuffer(fm_text buffer, fm_int size)
{
    fm_loggingState *ls;
    fm_int           i;
    fm_int           length;
    fm_int           position;
    fm_char *        cursor;

    ls = fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL;

    if ( !LOG_INITIALIZED(ls) )
    {
        return FM_ERR_UNINITIALIZED;
    }

    position = ls->currentPos;

    cursor = buffer;

    for (i = 0 ; i < position ; i++)
    {
        length = (fm_int) strlen(ls->logBuffer[i]);

        length = size > length ? length : size;

        FM_STRNCPY_S(cursor,
                     size,
                     ls->logBuffer[i],
                     length);

        cursor += length;

        size -= length;

        if (size == 0)
        {
            break;
        }

    }   /* end for (i = 0 ; i < position ; i++) */

    buffer[size - 1] = '\0';

    return FM_OK;

}   /* end fmGetLogBuffer */




/*****************************************************************************/
/** fmGetLoggingAttribute
 * \ingroup alosLog
 *
 * \desc            Returns an attribute of the logging subsystem.
 *
 * \param[in]       attr is the attribute to be returned. See
 *                  ''Log Attributes'' for a list of available
 *                  attributes.
 *
 * \param[in]       size is the size in bytes of the buffer pointed to by
 *                  value. Used only for attributes whose value is of 
 *                  variable length.
 *
 * \param[out]      value points to a caller-allocated storage where this
 *                  function should place the attribute value. The data
 *                  type of value is indicated for each attribute (see
 *                  ''Log Attributes'').
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ATTRIB if attr is not recognized.
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmGetLoggingAttribute(fm_int  attr,
                                fm_int  size,
                                void *  value)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING,
                     "attr=%d, size=%d, value=%p\n",
                     attr,
                     size,
                     (void *) value);

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    switch (attr)
    {
        case FM_LOG_ATTR_ENABLED:
            *( (fm_bool *) value ) = ls->enabled;
            break;

        case FM_LOG_ATTR_LOG_TYPE:
            *( (fm_int *) value ) = ls->logType;
            break;

        case FM_LOG_ATTR_CATEGORY_MASK:
            *( (fm_uint64 *) value ) = ls->categoryMask;
            break;

        case FM_LOG_ATTR_LEVEL_MASK:
            *( (fm_uint64 *) value ) = ls->levelMask;
            break;

        case FM_LOG_ATTR_VERBOSITY_MASK:
            *( (fm_uint32 *) value ) = ls->verbosityMask;
            break;

        case FM_LOG_ATTR_FILE_FILTER:
            fmStringCopy( (char *) value, ls->fileFilter, size );
            break;

        case FM_LOG_ATTR_FUNCTION_FILTER:
            fmStringCopy( (char *) value, ls->functionFilter, size );
            break;

        case FM_LOG_ATTR_LOG_FILENAME:
            fmStringCopy( (char *) value, ls->logFileName, size );
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, err);

}   /* end fmGetLoggingAttribute */




/*****************************************************************************/
/** fmResetLogging
 * \ingroup alosLog
 *
 * \desc            Resets the logging subsystem to its default state.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmResetLogging(void)
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_int           i;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LOGGING, "\n");

    if ( !LOG_INITIALIZED(ls) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_ERR_UNINITIALIZED);
    }

    ls->logType       = FM_LOG_TYPE_CONSOLE;
    ls->enabled       = TRUE;
    ls->verbosityMask = FM_LOG_VERBOSITY_DEFAULT;

    ls->categoryMask  = FM_LOG_CAT_DEFAULT;
    ls->levelMask     = FM_LOG_LEVEL_DEFAULT;

    /* make sure all object ID filters are pass-through */
    ls->legacyLoggingMask = FM_LOG_CAT_ALL;
    for (i = 0 ; i < 64  ; i++)
    {
        ls->range[i].rangeType = FM_LOG_RANGE_TYPE_INFINITE;
    }

    ls->functionFilter[0] = 0;
    ls->fileFilter[0] = 0;

    FM_LOG_EXIT_API(FM_LOG_CAT_LOGGING, FM_OK);

}   /* end fmResetLogging */




/*****************************************************************************/
/** fmGetCallerName
 * \ingroup intLogging
 *
 * \desc            Generate a string containing the call stack. The stack
 *                  will be listed starting with the caller of this
 *                  function, followed by the caller of that function, etc.
 *                  Used for diagnostic purposes only.
 *
 * \note            The Fulcrum ALOS implementation relies on GNU extensions.
 *                  For non-GNU environments, this implementation will always
 *                  return only "!".
 *
 * \param[out]      buf points to a caller-allocated character array into
 *                  which this function should place the call stack string.
 *
 * \param[in]       bufSize is the length of buf in bytes.
 *
 * \param[in]       callerCount is the maximum number of stack frame levels
 *                  to return.
 *
 * \param[in]       delimiter is the string to insert between reported call
 *                  stack entries (e.g., "\n").
 *
 * \return          None.
 *
 *****************************************************************************/
void fmGetCallerName(fm_text buf,
                     fm_int  bufSize,
                     fm_int  callerCount,
                     fm_text delimiter)
{
#ifndef __gnu_linux__
    FM_NOT_USED(callerCount);
    
    if (bufSize >= 2)
    {
        buf[0] = '!';
        buf[1] = '\0';
    }
#else
    char **names;
    void *btBuffer[callerCount + 1];
    int btNum;
    int stackDepth;
    fm_int i;
    char *shortName;
    char *first;
    char *last;
    
    stackDepth = (int) callerCount + 1;
    
    /* Get the call stack as addresses */
    btNum = backtrace(btBuffer, stackDepth);
    
    /* Convert addresses to symbols */
    names = backtrace_symbols(btBuffer, btNum);

    if (names == NULL)
    {
        if (bufSize >= 2)
        {
            buf[0] = '!';
            buf[1] = '\0';
        }
    }
    else
    {
        fmStringCopy(buf, "", bufSize);
        
        /**************************************************
         * Call stack will be listed from latest to 
         * earliest. Skip the first entry since that is
         * for this function.
         **************************************************/
        
        for (i = 1 ; i < btNum - 1 ; i++)
        {
            shortName = names[i];

            first = strchr(shortName, '(');
            last  = strchr(shortName, ')');

            if (first == NULL || last == NULL)
            {
                first = strchr(shortName, '[');
                last  = strchr(shortName, ']');
            }

            if (first != NULL && last != NULL)
            {
                *last     = '\0';
                shortName = first + 1;
            }

            fmStringAppend(buf, shortName, bufSize);
            
            /* If not the second-to-last entry... */
            if (i < btNum - 2)
            {
                /* ...insert a delimiter string. */
                fmStringAppend(buf, delimiter, bufSize);
            }
            
        }   /* end for (i = 1 ; i < btNum - 1 ; i++) */
        
        /**************************************************
         * We use free instead of fm_free, because names
         * was allocated with malloc by backtrace_symbols.
         **************************************************/
        
        free(names);
        
    }   /* end if (names == NULL) */
    
#endif

}   /* end fmGetCallerName */




/*****************************************************************************/
/** fmLogMessageV2
 * \ingroup alosLog
 *
 * \desc            Generate a log message if all filter criteria are met.
 *                  This function is normally invoked by helper macros.
 * 
 * \note            As for ''fmLogMessage'' the category argument is a bitmask.
 *                  However, unlike ''fmLogMessage'' the behavior of this
 *                  function when more than one bit is set is undefined.
 *
 * \note            This function may be called prior to the logging subsystem
 *                  being initialized. In that case, logging output will be to
 *                  the console only.
 *
 * \param[in]       category is a bitmask of flags (see ''Log Categories'')
 *                  which indicates which category this log message belongs to.
 *
 * \param[in]       logLevel is a bitmask of level flags (see
 *                  ''Log Levels'') which indicate which log levels this
 *                  log message belongs to.
 *
 * \param[in]       srcFile is the name of the source code file generating
 *                  the log message. Usually invoked as __FILE__.
 *
 * \param[in]       srcFunction is the name of the source code function
 *                  generating the log message. Usually invoked as __func__.
 *
 * \param[in]       srcLine is the source code line number generating the
 *                  log message. Usually invoked as __LINE__.
 *
 * \param[in]       objectId is the ID of the object to filter on. The log
 *                  message will be processed if this ID is in the range of
 *                  object IDs configured for the log category
 * 
 * \param[in]       format is the printf-style format.
 *
 * \param[in]       ... is the printf var-args argument list.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmLogMessageV2( fm_uint64   category,
                          fm_uint64   logLevel,
                          fm_int      objectId,
                          const char *srcFile,
                          const char *srcFunction,
                          fm_uint32   srcLine,
                          const char *format,
                          ... )
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_status   status;
    fm_int      categoryIdx;
    fm_uint64   filteredCategoryMask;
    va_list     ap;

    if (LOG_INITIALIZED(ls) && !ls->enabled)
    {
        return FM_OK;
    }

    if ( LOG_INITIALIZED(ls) )
    {
        filteredCategoryMask   = ls->categoryMask;
    }
    else
    {
        /* use defaults if the logging facility is not initialized */
        filteredCategoryMask   = FM_LOG_CAT_ALL;
    }

    va_start(ap, format);
    if ( (category == 0)        ||
         !LOG_INITIALIZED(ls)   ||
         (logLevel & FM_LOG_LEVEL_DEFAULT) )
    {
        /* pass it along unconditionally if no category is specified or if
         * the logging subsystem hasn't been initialized yet */
        status = LogMessage( filteredCategoryMask,
                             category,           
                             logLevel,           
                             srcFile,            
                             srcFunction,        
                             srcLine,            
                             format,             
                             ap );              
    }
    else
    {
        /* find out the object ID range of this category */
        categoryIdx = 0;
        while ( ( categoryIdx < 64 ) && ((category & 0x1) == 0 ) )
        {
            category >>= 1;
            categoryIdx++;
        }

#if defined(__KLOCWORK__)
        /* Screening for (category == 0) ensures that the while loop
         * in the else clause always terminates with (categoryIdx < 64),
         * but Klocwork apparently can't infer this. Make it explicit. */
        if (categoryIdx >= 64)
        {
            return FM_ERR_ASSERTION_FAILED;
        }
#endif

        /* proceed if the object ID range for this category is infinite or
         * if the objectId range falls in the configured range */
        if ( ( ls->range[categoryIdx].rangeType == FM_LOG_RANGE_TYPE_INFINITE ) ||
             ( ls->range[categoryIdx].rangeType == FM_LOG_RANGE_TYPE_FINITE     &&
               ls->range[categoryIdx].minObjectId <= objectId         &&
               ls->range[categoryIdx].maxObjectId >= objectId         ) )
        {
            status = LogMessage( filteredCategoryMask,
                                 (FM_LITERAL_U64(1) << categoryIdx),           
                                 logLevel,           
                                 srcFile,            
                                 srcFunction,        
                                 srcLine,            
                                 format,             
                                 ap );              
        }
        else
        {
            status = FM_OK;
        }
    }


    va_end(ap);
    return status;

}   /* end fmLogMessageV2 */




/*****************************************************************************/
/** fmSetLoggingCategoryConfig
 * \ingroup alosLog
 *
 * \desc            Function to configure category-specific log filtering items
 * 
 * \note            This function is to be used in conjunction with
 *                  ''fmEnableLoggingCategory'' and
 *                  ''fmDisableLoggingCategory'' which can be used to
 *                  enable or disable respectively the logging category overall
 *
 * \note            Although the categoryMask argument is a bitmask this
 *                  function requires that only one bit is set, representing
 *                  the only logging category to be configured
 *
 * \param[in]       categoryMask is a bitmask of flags (see ''Log Categories'')
 *                  which indicates the category to be configured
 * 
 * \param[in]       rangeType is the object ID range type for this category,
 *                  see ''fm_loggingRangeType'' for valid values
 * 
 * \param[in]       minObjectId is the lowest valid value of the object ID when
 *                  rangeType is ''FM_LOG_RANGE_TYPE_FINITE''
 * 
 * \param[in]       maxObjectId is the highest valid value of the object ID 
 *                  when rangeType is ''FM_LOG_RANGE_TYPE_FINITE''
 * 
 * \param[in]       legacyLoggingOn is a boolean indicating whether legacy
 *                  logging (i.e. logging using ''fmLogMessage'') is enabled
 *                  for this category
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if this function is invoked before
 *                  the logging subsystem is initialized
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 *****************************************************************************/
fm_status fmSetLoggingCategoryConfig( fm_uint64           categoryMask,
                                      fm_loggingRangeType rangeType,
                                      fm_int              minObjectId,
                                      fm_int              maxObjectId,
                                      fm_bool             legacyLoggingOn )
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_int categories;
    fm_int idx;
    fm_int category;

    /* can't proceed if the log subsystem isn't initialized */
    if ( !LOG_INITIALIZED(ls) )
    {
        return FM_ERR_UNINITIALIZED;
    }

    /* make sure only only category is being configured */
    category    = 0;
    categories  = 0;
    idx         = 0;

    while ( idx < 64 )
    {
        if ( categoryMask & 1 )
        {
            category = idx;
            categories++;
        }
        categoryMask >>= 1;
        idx++;
    }

    /* sanity-check the arguments */
    if ( categories != 1 || rangeType > FM_LOG_RANGE_TYPE_MAX )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    ls->range[category].minObjectId = minObjectId;
    ls->range[category].maxObjectId = maxObjectId;
    ls->range[category].rangeType   = rangeType;
    if ( legacyLoggingOn )
    {
        ls->legacyLoggingMask |=  (FM_LITERAL_U64(1) << category);
    }
    else
    {
        ls->legacyLoggingMask &= ~(FM_LITERAL_U64(1) << category);
    }
     
    return FM_OK;

}   /* end fmSetLoggingCategoryConfig */




/*****************************************************************************/
/** fmGetLoggingCategoryConfig
 * \ingroup alosLog
 *
 * \desc            Function to retrieve category-specific log filtering items
 * 
 * \note            Although the categoryMask argument is a bitmask this
 *                  function requires that only one bit is set, representing
 *                  the only logging category to be configured
 * 
 * \param[in]       categoryMask is a bitmask of flags (see ''Log Categories'')
 *                  which indicates the category whose configuration is to be
 *                  retrieved
 *
 * \param[out]      rangeType is the pointer to a caller-allocated area where
 *                  this function will return the object ID range type for this
 *                  category, see ''fm_loggingRangeType'' for valid values
 * 
 * \param[out]      minObjectId is the pointer to a caller-allocated area where
 *                  this function will return the lowest valid value of the
 *                  object ID when rangeType is ''FM_LOG_RANGE_TYPE_FINITE''
 * 
 * \param[out]      maxObjectId is the pointer to a caller-allocated area where
 *                  this function will return the highest valid value of the
 *                  object ID when rangeType is ''FM_LOG_RANGE_TYPE_FINITE''
 * 
 * \param[out]      legacyLoggingOn is the pointer to a caller-allocated area
 *                  where this function will return a boolean indicating
 *                  whether legacy logging (i.e. logging using
 *                  ''fmLogMessage'') is enabled for this category
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if this function is invoked before
 *                  the logging subsystem is initialized
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 *****************************************************************************/
fm_status fmGetLoggingCategoryConfig( fm_uint64            categoryMask,
                                      fm_loggingRangeType *rangeType,
                                      fm_int              *minObjectId,
                                      fm_int              *maxObjectId,
                                      fm_bool             *legacyLoggingOn )
{
    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    fm_int categories;
    fm_int idx;
    fm_int category;

    /* can't proceed if the log subsystem isn't initialized */
    if ( !LOG_INITIALIZED(ls) )
    {
        return FM_ERR_UNINITIALIZED;
    }

    /* make sure only only category is being configured */
    category    = 0;
    categories  = 0;
    idx         = 0;

    while ( idx < 64 )
    {
        if ( categoryMask & 1 )
        {
            category = idx;
            categories++;
        }
        categoryMask >>= 1;
        idx++;
    }

    /* sanity-check the arguments */
    if ( categories      != 1    || 
         rangeType       == NULL ||
         minObjectId     == NULL ||
         maxObjectId     == NULL ||
         legacyLoggingOn == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }
     
    *minObjectId = ls->range[category].minObjectId;
    *maxObjectId = ls->range[category].maxObjectId;
    *rangeType   = ls->range[category].rangeType;

    if ( ls->legacyLoggingMask & (FM_LITERAL_U64(1) << category) )
    {
        *legacyLoggingOn = TRUE;
    }
    else
    {
        *legacyLoggingOn = FALSE;
    }

    return FM_OK;

}   /* end fmGetLoggingCategoryConfig */




#if defined(FM_ALOS_FAULT_INJECTION_POINTS) && (FM_ALOS_FAULT_INJECTION_POINTS==FM_ENABLED)
/*****************************************************************************/
/** UNPUBLISHED: fmActivateFaultInjectionPoint
 * \ingroup alosLog
 *
 * \desc            Activates fault injection point.
 *
 * \param[in]       functionName is the name of the source code function to
 *                  activate for fault incjection.
 *
 * \param[in]       error is the error code to return from function activated
 *                  for fault injection.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_UNINITIALIZED if the logging subsystem has not been
 *                  initialized.
 *
 *****************************************************************************/
fm_status fmActivateFaultInjectionPoint(const char *functionName, fm_status error)
{
    fm_status err = FM_OK;

    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    if ( LOG_INITIALIZED(ls) )
    {
        ls->failError = error;
        if (functionName && (strcmp(functionName, "") != 0))
        {
            fmStringCopy(ls->failFunction, functionName, FM_MAX_FUNC_NAME_LENGTH);
        }
        else
        {
            ls->failFunction[0] = 0;
        }
    }
    else
    {
        err = FM_ERR_UNINITIALIZED;
    }
    return err;

}   /* end fmActivateFaultInjectionPoint */




/*****************************************************************************/
/** UNPUBLISHED: fmFaultInjection
 * \ingroup alosLog
 *
 * \desc            Returns an error for fault injection point.
 *
 * \param[in]       functionName is the name of the source code function
 *                  with fault injection point. Invoked as __func__ in
 *                  ''FM_FAULT_INJECTION_POINT'' macro.
 *
 * \return          FM_OK if fault injection is not active for functionName.
 * \return          failError if fault injection is active for functionName.
 *
 *****************************************************************************/
fm_status fmFaultInjection(const char *functionName)
{
    fm_status err = FM_OK;

    fm_loggingState *ls = (fmRootAlos ? &(fmRootAlos->fmLoggingState) : NULL);
    if ( LOG_INITIALIZED(ls) )
    {
        if (functionName && (strcmp(functionName, ls->failFunction) == 0))
        {
            err = ls->failError;
            FM_LOG_WARNING(FM_LOG_CAT_LOGGING,
                           "function %s forced to return error %d.\n",
                           functionName,
                           err);
            ls->failFunction[0] = 0;
            ls->failError = FM_OK;
        }
    }
    return err;

}   /* end fmFaultInjection */
#endif
