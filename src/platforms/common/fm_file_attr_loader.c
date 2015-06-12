/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_file_attr_loader.c
 * Creation Date:   September 10, 2007
 * Description:     A file based attribute loader for general use from
 *                  the platform layer.
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

/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmPlatformLoadAttributeFromLine
 * \ingroup intPlatform
 *
 * \desc            Loads attribute into the attribute subsystem by calling
 *                  the set and get methods on attribute read in from a text
 *                  line.
 *                                                                      \lb\lb
 *                  The expected line format for the database is as follows:
 *                                                                      \lb\lb
 *                  [key] [type] [value]
 *                                                                      \lb\lb
 *                  Where key is a dotted string, type is one of int, bool or
 *                  float, and value is the value to set. Space is the only
 *                  valid separator, but multiple spaces are allowed.
 *
 * \param[in]       line is thetext line to laod attribute from.
 *
 * \param[in]       lineNo is the number of line
 *
 * \param[out]      numAttr is the number of loaded attributes
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if line or numAttr is NULL.
 *
 *****************************************************************************/
fm_status fmPlatformLoadAttributeFromLine(fm_text line,
                                          fm_int  lineNo,
                                          fm_int *numAttr)
{
    fm_status err;
    char *    key;
    char *    type;
    char *    value;
    char *    endPtr;
    long int  cvt;
    char *    context;
    char *    str;
    uint      s1max;

    /* attribute values */
    fm_int    intValue;
    fm_bool   boolValue;
    fm_float  floatValue;
    fm_text   textValue;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "line=%p lineNo=%d\n", line, lineNo);

    if ( (line == NULL) || (numAttr == NULL) )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Null line\n");

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }
    
    context = NULL;
    s1max   = FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN;
    key     = FM_STRTOK_S(line, &s1max, " \r\n", &context);
    type    = FM_STRTOK_S(NULL, &s1max, " \r\n", &context);
    
    *numAttr = 0;

    /* check for comment line */
    if (line[0] == '#')
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }
    
    /* Look for a string starting with a " */
    if (type != NULL &&
        strcmp(type, "text") == 0 &&
        (str = strchr(context,'\"')) != NULL )
    {
        value = str;
    }
    else
    {
        value = FM_STRTOK_S(NULL, &s1max, " \r\n", &context);
    }
    
    if (key && type && value)
    {
        err = FM_OK;
        
        if (strcmp(type, "int") == 0)
        {
            cvt = strtol(value, &endPtr, 0);
            
            if (errno == ERANGE)
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Conversion error on line %d\n", lineNo);

                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
            }
            
            if (cvt > INT_MAX)
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Overflow error on line %d\n", lineNo);
                
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK); 
            }

            if (value == endPtr)
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Conversion error on line %d\n", lineNo);
                
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
            }

            intValue = (fm_int) cvt;
            
            err = fmSetApiProperty(key, FM_API_ATTR_INT, &intValue);
            
            (*numAttr)++;
            }
        else if (strcmp(type, "bool") == 0)
        {
            if ( (strcmp(value, "true") == 0) || (strcmp(value, "1") == 0) )
            {
                boolValue = TRUE;
            }
            else if ( (strcmp(value, "false") == 0) ||
                      (strcmp(value, "0") == 0) )
            {
                boolValue = FALSE;
            }
            else
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Conversion error on line %d. Type = [%s], value = [%s].\n", lineNo, type, value);
                
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK); 
            }

            err = fmSetApiProperty(key, FM_API_ATTR_BOOL, &boolValue);
            
            (*numAttr)++;
        }
        else if (strcmp(type, "float") == 0)
        {
            floatValue = strtod(value, NULL);
            
            if (floatValue == HUGE_VAL)
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Conversion error on line %d. Type = [%s], value = [%s].\n", lineNo, type, value);
                
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
            }

            err = fmSetApiProperty(key, FM_API_ATTR_FLOAT, &floatValue);
            
            (*numAttr)++;
        }
        else if (strcmp(type, "text") == 0)
        {
            textValue = value;
            
            err = fmSetApiProperty(key, FM_API_ATTR_TEXT, textValue); 
            
            (*numAttr)++;
        }
        
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "fmSetApiProperty returned '%s' on line %d\n",
                         fmErrorMsg(err), lineNo);
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Loaded %d attributes\n",
                 *numAttr);
    
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLoadAttributeFromLine */




/*****************************************************************************/
/** fmPlatformLoadAttributes
 * \ingroup intPlatform
 *
 * \desc            Loads attributes into the attribute subsystem by calling
 *                  the set and get methods on attributes read in from a text
 *                  file.
 *                                                                      \lb\lb
 *                  The expected file format for the database is as follows:
 *                                                                      \lb\lb
 *                  [key] [type] [value]
 *                                                                      \lb\lb
 *                  Where key is a dotted string, type is one of int, bool or
 *                  float, and value is the value to set. Space is the only
 *                  valid separator, but multiple spaces are allowed.
 *
 * \param[in]       fileName is the full path to a text file to load.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformLoadAttributes(fm_text fileName)
{
    fm_status err;
    FILE     *fp;
    int       lineNo = 0;
    char      line[FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN];
    int       numAttr = 0;
    int       lineNumAttr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "fileName=%s\n", fileName);

    fp = fopen(fileName, "rt");

    if (!fp)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                     "Unable to open attribute database %s\n",
                     fileName);

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    while ( fgets(line, FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN, fp) )
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

    }   /* end while ( fgets(line, FM_PLATFORM_API_ATTRIBUTE_CFG_LINE_MAX_LEN, fp) ) */

    fclose(fp);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Loaded %d attributes from %s\n",
                 numAttr, fileName);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLoadAttributes */
