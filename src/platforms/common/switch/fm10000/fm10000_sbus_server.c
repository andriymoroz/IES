/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_sbus_server.c
 * Creation Date:   November 8, 2013
 * Description:     Server for accessing SBUS devices.
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fm_sdk_int.h>

#include <fm_sdk_fm10000_int.h>


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


#define CMD_BUF_LEN         32*10000 /* MAX_CMDS_BUFFERED * SBUS_CMD_BUF_SIZE */
#define REPLY_BUF_LEN       32*10000


#define EOS               '\0'
#define ISNUL(cp)         (*(cp) == EOS)
#define ISEOL(cp)         ((*(cp) == '#') || ISNUL(cp))  // comment or end.
#define ISTERM(cp)        (isspace(*(cp)) || ISEOL(cp) || (*(cp) == ';'))  // end of token.
#define SKIPSPACE(cp)     while (isspace(*(cp))) ++(cp)


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_thread sbusServerThread;
static fm_int    serverTcpPort;

fm_char          replyStr[REPLY_BUF_LEN+1];
fm_char          cmdStr[CMD_BUF_LEN];

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/





/*****************************************************************************/
/* GetNumFromHex
 *
 * \desc            Parse a hex number and return the equivalent binary number,
 *                  with *endp modified to the first char after the token.
 *
 * \param[in]       cp is a hex string.
 *
 * \param[in]       endp is the caller-allocated storage where this function
 *                  place pointer to the next token.
 *
 * \param[in]       min is the minimum token length.
 *
 * \param[in]       max is the maximum token length.
 *
 * \return          value of the hex string or 0 for error with endp set to
 *                  cp.
 *
 *****************************************************************************/
fm_uint32 GetNumFromHex(fm_text cp, fm_text *endp, fm_int min, fm_int max)
{
    fm_uint32 result;
    fm_text   start; 

    result = 0;
    start  = *endp = cp;
    while (isxdigit(*cp))
    {
        if (cp - start >= max)
        {
            return(0);
        }
        result = (result << 4) | (isdigit(*cp) ? (*cp - '0') : (10 + tolower(*cp) - 'a'));
        ++cp;
    }

    if ((cp - start < min) || (! ISTERM(cp)))
    {
        return(0);
    }

    *endp = cp;

    return(result);

}   /* end GetNumFromHex */




/*****************************************************************************/
/* GetNumFromBin
 *
 * \desc            Parse an ASCII binary number possibly containing 'x'/'X'
 *                  digits, and return the equivalent binary number, with *endp
 *                  modified to the first char after the token.
 *
 * \param[in]       cp is a binary string consisting of '0'/'1'/'x'/'X' digits.
 *
 * \param[in]       endp is the caller-allocated storage where this function
 *                  place pointer to the next token.
 *
 * \param[in]       min is the minimum token length.
 *
 * \param[in]       max is the maximum token length.
 *
 * \param[in]       maskp is the caller-allocated storage where this function
 *                  place the mask value, defaulted to all 0s, but
 *                  returns 0s for 'x'/'X' meaning bits not to be changed
 *                  (read/modify/write).
 *
 * \return          value of the hex string or 0 for error with endp set to
 *                  cp.
 *
 *****************************************************************************/
fm_uint32 GetNumFromBin(fm_text cp, fm_text *endp, fm_int min, fm_int max, fm_uint32 * maskp)
{
    fm_uint32  result;
    fm_uint32  mask;
    fm_text    start;

    result = 0;
    mask   = 0;
    start  = *endp = cp;
    while ((*cp == '0') || (*cp == '1') || (*cp == 'x') || (*cp == 'X'))
    {
        if (cp - start >= max)
        {
            return(0);
        }

        if ((*cp == '0') || (*cp == '1'))
        {
            result = (result << 1) | (*cp - '0');
            mask = (mask << 1) | 1;
        }
        else
        {
            result <<= 1;
            /* 0-bit, masked out */
            mask <<= 1;
        }

        ++cp;
    }

    *maskp = mask;

    if ((cp - start < min) || (! ISTERM(cp)))
    {
        return(0);
    }
    *endp = cp;

    return(result);

}   /* end GetNumFromBin */




/*****************************************************************************/
/* GetBinaryStr
 *
 * \desc            Convert data to ASCII encoded binary string with optional 
 *                  underscores every 8 bits binary.
 *
 * \param[in]       data is the data to convert.
 *
 * \param[out]      str is the caller-allocated storage where this function
 *                  place binary string of the data.
 *
 * \param[in]       strlen is the size of str.
 *
 * \param[in]       bits is the number of bits to convert.
 *
 * \return          FM_OK if successful. 
 * \return          FM_ERR_INVALID_ARGUMENT if inputs are out of range. 
 *
 *****************************************************************************/
fm_status GetBinaryStr(fm_uint data, fm_text str, fm_int strLen, fm_uint bits)
{
    fm_int  kBitsPerByte = 8;
    fm_text pDest = str;
    fm_uint offset;
    fm_uint i;
    fm_int  len;

    /* Only convert if the number of bits requested to be equal to or less
       than the width of the data variable. */
    if (bits <= (sizeof(data) * kBitsPerByte) )
    {
        len = 0;
        offset = (kBitsPerByte*(bits%kBitsPerByte) - bits);
        for (i = 1; i <= bits; ++i)
        {
            if (len >= strLen)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }

            /* Check bit value MSB to LSB*/
            pDest[len++] = ((data & (1 << (bits-i))) ? '1' :'0');
        }

        /* Insert end of string marker. */
        pDest[len] = '\0';

        return FM_OK;
    }

    return FM_ERR_INVALID_ARGUMENT;

}   /* end GetBinaryStr */



/*****************************************************************************/
/* sbusParseDaOrCmd
 *
 * \desc            Parse an SBus command <da> or <cmd> field.
 *
 * \param[in]       cmd is the command string.
 *
 * \param[in,out]   cpp is the caller-allocated storage where the caller
 *                  places the pointer to the da or cmd field. This function
 *                  also returns the pointer to the next token.
 *
 * \param[out]      valp is the caller-allocated storage where the function
 *                  will place the value.
 *
 * \param[in]       argNum is the argument number of the field.
 *
 * \param[in]       valName is the name of the field.
 *
 * \param[out]      result is the caller-allocated buffer where the function
 *                  will place any error message.
 *
 * \param[in]       resultLen is the size of the result buffer.
 *
 * \return          FM_OK if successful. 
 * \return          FM_ERR_INVALID_ARGUMENT if inputs are out of range. 
 *
 *****************************************************************************/
fm_status sbusParseDaOrCmd(fm_text cmd,
                           fm_text *cpp,
                           fm_uint *valp,
                           fm_int argNum,
                           fm_text valName,
                           fm_text result,
                           fm_int resultLen)
{
    fm_text   cp2;
    fm_uint32 val;


    val = GetNumFromHex(*cpp, &cp2, 2, 2);

    if (cp2 == *cpp)
    {
        FM_SNPRINTF_S(result, resultLen,
                      "Invalid \"sbus\": Arg %d = <%s> must be 2 hex digits. Got: \"%s\".",
                      argNum, valName, cmd);
        return FM_ERR_INVALID_ARGUMENT;
    }

    SKIPSPACE(cp2);
    *cpp  = cp2;
    *valp = val;

    return FM_OK;

}   /* end sbusParseDaOrCmd */



/*****************************************************************************/
/* sbusParseData
 *
 * \desc            Parse an SBus command <data> field.
 *                  Address formats for <data>:
 *
 *                  C:  2 chars:  8-bit ASCII-encoded hex, such as "a5"
 *                  D:  8 chars:  8-bit ASCII-encoded binary, such as "10100101"
 *                  E: 10 chars:  32-bit ASCII-encoded hex, such as "0x012345a5"
 *                                (the leading "0x" or "0X" is required)
 *                  F: 32 chars:  32-bit ASCII-encoded binary, such as
 *                                "00000001001000110100010110100101"
 *                  G:  N <= 31 chars (variable length) beginning with 'z'/'Z', such as "z01011":
 *                                same as previous except the 'z'/'Z' is replaced with an
 *                                appropriate number of 'x's to make a 32-char <data> value (see
 *                                below)
 *                
 *                  For short formats (C and D), leading bits are prefilled as 0s.
 *                
 *                  For all ASCII-encoded binary formats (D, F, G), any character except
 *                  the 'z'/'Z' prefix for case G can be specified as 'x'/'X' instead of
 *                  '0'/'1'.  Special case:  D or F can start with "0x"|"0X".
 *
 * \param[in]       cmd is the command string.
 *
 * \param[in,out]   cpp is the caller-allocated storage where the caller
 *                  places the pointer to the da or cmd field. This function
 *                  also returns the pointer to the next token.
 *
 * \param[out]      datap is the caller-allocated storage where the function
 *                  will place the value.
 *
 * \param[in]       maskp is the caller-allocated storage where the function
 *                  will place the mask value.
 *
 * \param[in]       reslenp is the caller-allocated storage where the function
 *                  will place the reply length to send back.
 *
 * \param[out]      result is the caller-allocated buffer where the function
 *                  will place any error message.
 *
 * \param[in]       resultLen is the size of the result buffer.
 *
 * \return          FM_OK if successful. 
 * \return          FM_ERR_INVALID_ARGUMENT if inputs are out of range. 
 *
 *****************************************************************************/
fm_status sbusParseData(fm_text cmd,
                        fm_text *cpp,
                        fm_uint32 *datap,
                        fm_uint32 *maskp,
                        fm_uint *reslenp,
                        fm_text result,
                        fm_int resultLen)
{
    fm_text     cp = *cpp;
    fm_text     cp2;
    fm_uint     reslen;
    fm_uint32   data;
    fm_uint     len;

    if ((*cp == 'z') || (*cp == 'Z'))
    {
        data = GetNumFromBin(++cp, &cp2, 1, 31, maskp);

        if (cp == cp2)
        {
            if (! ISTERM(cp + 1))
            {
                FM_SNPRINTF_S(result, resultLen,
                              "Invalid \"sbus\": Arg 4 = <data> 'z' prefix must be followed "
                              "by 0-31 '0'/'1'/'x'/'X' chars, but got: \"%s\".", cmd);
                return FM_ERR_INVALID_ARGUMENT;
            }
        }
        reslen = 32;
    }
    else
    {
        *maskp = 0xffffffff;  /* default = no read/modify/write. */

        for (cp2 = cp; isxdigit(*cp2) || (*cp2 == 'x') || (*cp2 == 'X'); ++cp2)
        /* null */;

        len = cp2 - cp;

        if (len ==  2)
        {
            data = GetNumFromHex(cp, &cp2,  2,  2);          /* case C. */
        }
        else if (len ==  8)
        {
            data = GetNumFromBin(cp, &cp2,  8,  8, maskp);   /* case D. */
        }
        else if ((len == 10) && (0 == strncasecmp(cp, "0x", 2)))
        {
            cp += 2; data = GetNumFromHex(cp, &cp2,  8,  8); /* case E. */
        }
        else if (len == 32)
        {
            data = GetNumFromBin(cp, &cp2, 32, 32, maskp);   /* case F. */
        }
        else
        {
            cp2 = cp;  // marks an error.
        }

        if (cp == cp2)
        {
            FM_SNPRINTF_S(result, resultLen,
                          "Invalid \"sbus\": Arg 4 = <data> must be 2-hex, 8-bin, 0x-8-hex, 32-bin, "
                          "or \"z\" prefix variable-length, but got: \"%s\".", cmd);
            return(0);
        }
        reslen = ((len <= 8) ? 8 : 32);
    }

    *cpp     = cp2;
    *datap   = data;
    *reslenp = reslen;

    return FM_OK;

}   /* end sbusParseData */



/*****************************************************************************/
/* sbusParseDaOrCmd
 *
 * \desc            Handles an "sbus" command and writes into result an error
 *                  message or the Bus command response.
 *                  sbus <sa> <da> <cmd> <data> [# <comment>]  # in these formats:
 *                 
 *                    <sa>:    SBus (slice) address, optionally including chip + ring numbers
 *                    <da>:    data (register) address
 *                    <cmd>:   command, usually 01 = write or 02 = read
 *                    <data>:  data to send
 *                 
 *                  <sa>,<da>,<cmd> = 2-char hex only, such as "00" or "a5" (except <sa>, see
 *                    below); unlike Perl/HW servers, does not support ASCII binary for these
 *                    values.
 *                 
 *                    For <sa> only:  Optional 4/3-char format to set *chip_nump (0..f) (such as
 *                    "2" in "2081") and *ring_nump (0..f) (such as "3" in "37a").  Previous
 *                    chip and ring numbers are remembered and applied to new "sbus" commands
 *                    where they are not respecified, plus "chip_num" can revise chip_num
 *                    between SBus commands.
 *                 
 *                  <data> = 2-char hex, 8-char binary, 8-char hex (preceded by "0x"), or
 *                    32-char binary; with "z" and "x" allowed in binary values for leading
 *                    zeroes and read/modify/write, respectively.
 *                 
 * \param[in]       cmd is the command string.
 *
 * \param[in,out]   cp is the caller-allocated storage where the caller
 *                  places the pointer to the sbus command.
 *
 * \param[out]      replyStr is the caller-allocated buffer where the function
 *                  will place any error message or the return value.
 *
 * \param[in]       replyLen is the size of the replyStr buffer.
 *
 * \return          FM_OK if successful. 
 * \return          FM_ERR_INVALID_ARGUMENT if inputs are out of range. 
 *
 *****************************************************************************/
fm_status sbusCommand(fm_text cmd, fm_text cp, fm_text replyStr, fm_int replyLen)
{
    fm_text     cp2;
    fm_uint     chip;
    fm_uint     ring;
    fm_bool     eplRing;
    fm_uint     sbusAddr;
    fm_uint     regAddr;
    fm_uint32   scmd;
    fm_uint32   data;
    fm_uint32   mask;
    fm_uint     reslen;
    fm_status   status = FM_OK;
    fm_int      sw = 0;
    fm_uint32   value = 0;
    fm_int      cnt;

    SKIPSPACE(cp);

    if (ISEOL(cp))
    {
        FM_SNPRINTF_S(replyStr, replyLen, "%s",
            "Invalid \"sbus\" command ignored. Must be: \"sbus <sa> <da> <cmd> <data> [# <comment>]\".");
        return FM_ERR_INVALID_ARGUMENT;
    }

    sbusAddr = strtol(cp, &cp2, 16);

    if (cp2 == cp)
    {
        FM_SNPRINTF_S(replyStr, replyLen,
                      "Invalid \"sbus\" command ignored: Arg 1 = <sa> must be 2-4 hex digits. Got: \"%s\".", cmd);
        return FM_OK;
    }

    cp = cp2;
    SKIPSPACE(cp);

    if ( (sbusParseDaOrCmd(cmd, &cp, &regAddr, 2, "da",  replyStr, replyLen) ||
          sbusParseDaOrCmd(cmd, &cp, &scmd, 3, "cmd", replyStr, replyLen) ||
          sbusParseData(cmd, &cp, &data, &mask, &reslen, replyStr, replyLen) ) )
    {
        return FM_OK;
    }

    if (!ISTERM(cp))
    {
        FM_SNPRINTF_S(replyStr, replyLen, "Invalid Termination: \"%s\".", cmd);
        return FM_OK;
    }

#if 0
    if (new_chip < 0x10) *chip_nump = new_chip;
    if (new_ring < 0x10) *ring_nump = new_ring;

    sbusAddr |= (*chipNum << 12) | (*ringNum << 8);
#endif

    chip     = (sbusAddr & 0xf000) >> 12;
    ring     = (sbusAddr & 0x0f00) >>  8;
    sbusAddr = sbusAddr & 0xFF;
    eplRing  = (ring == 0);

    LOCK_SWITCH(sw);

    if (scmd == 0)
    {
        printf("SBUS Reset: %s\n", cmd);
    }
    else if (scmd == 1)
    {
        if (mask == 0xFFFFFFFF)
        {
            FM_LOG_DEBUG3(FM_LOG_CAT_PLATFORM, "SBUS write 0x%x reg 0x%x data 0x%x\n", sbusAddr, regAddr, data);
            status = fm10000SbusWrite(sw, eplRing, sbusAddr, regAddr, data);
        }
        else
        {
            status = fm10000SbusRead(sw, eplRing, sbusAddr, regAddr, &value);
            value = (data & mask) | (value & ~mask);
            FM_LOG_DEBUG3(FM_LOG_CAT_PLATFORM, "SBUS mask 0x%x reg 0x%x data 0x%x\n", sbusAddr, regAddr, value);
            status = fm10000SbusWrite(sw, eplRing, sbusAddr, regAddr, value);
        }
    }
    else if (scmd == 2)
    {
        status = fm10000SbusRead(sw, eplRing, sbusAddr, regAddr, &value);
        FM_LOG_DEBUG3(FM_LOG_CAT_PLATFORM, "SBUS read 0x%x reg 0x%x data 0x%x\n", sbusAddr, regAddr, value);
    }
    else
    {
        FM_SNPRINTF_S(replyStr, replyLen, "Unknown SBUS scmd(%d) => \"%s\".\n", scmd, cmd);
        return FM_OK;
    }

    UNLOCK_SWITCH(sw);

    if (status)
    {
        FM_SNPRINTF_S(replyStr, replyLen, "Command failed: \"%s\" => \"%s\".", cmd, fmErrorMsg(status));
        return FM_OK;
    }

    if (scmd != 1)
    {
        status = GetBinaryStr(value, replyStr, replyLen, 32);
        if (status)
        {
            FM_SNPRINTF_S(replyStr, replyLen,
                          "Convert to Binrary Failed: \"%s\" => \"%s\".",
                          cmd, fmErrorMsg(status));
            return FM_OK;
        }
        if (reslen == 8) 
        {
            /* FM_STRCPY_S(replyStr, replyLen, replyStr + 24); Give overlap string warning */
            /* last "byte" only */
            for (cnt = 0; cnt < 8; cnt++)
            {
                replyStr[cnt] = replyStr[cnt+24];
            }
            replyStr[cnt] = '\0';
        }
    }

    return FM_OK;

}   /* end sbusCommand */




/*****************************************************************************/
/* ProcessCmdStr
 *
 * \desc            Process command string.
 *
 * \param[in]       cmd is the command string.
 *
 * \param[out]      replyStr is the caller-allocated buffer where the function
 *                  will place any error message or the return value.
 *
 * \param[in]       replyLen is the size of the replyStr buffer.
 *
 * \return          FM_OK. 
 *
 ****************************************************************************/
fm_status ProcessCmdStr(fm_text cmdStr, fm_text reply, fm_int replyLen)
{
    fm_text cp = cmdStr;

    SKIPSPACE(cp);

    if (cp[0] == ';')
    {
        cp++;
    }

    if (cp[0] == '@')
    {
        cp++;
    }

    if (strncasecmp(cp, "sbus", 4) == 0)
    {
        sbusCommand(cmdStr, cp + 4, reply, replyLen);
    }
    else if (strncasecmp(cp, "jtag", 4) == 0)
    {
        FM_LOG_DEBUG3( FM_LOG_CAT_PLATFORM, "JTAG: %s\n", cp);
        if (strncasecmp(cmdStr, "jtag 32 02b6", 12) == 0)
        {
            FM_SNPRINTF_S(reply, replyLen, "%s", "01011000000000000000010101111111\n");
        }
        else
        {
            FM_SNPRINTF_S(reply, replyLen, "%s: No support\n", cmdStr);
        }
    }
    else if (strncasecmp(cp, "chipnum", 7) == 0)
    {
        FM_LOG_DEBUG3( FM_LOG_CAT_PLATFORM, "CHIPNUM: %s\n", cp);
        FM_SNPRINTF_S(reply, replyLen, "%s", "0\n");
    }
    else if (strncasecmp(cp, "chips", 5) == 0)
    {
        FM_LOG_DEBUG3( FM_LOG_CAT_PLATFORM, "CHIPS: %s\n", cp);
        FM_SNPRINTF_S(reply, replyLen, "%s: No support\n", cmdStr);
    }
    else if (strncasecmp(cp, "chip", 4) == 0)
    {
        FM_LOG_DEBUG3( FM_LOG_CAT_PLATFORM, "CHIP: %s\n", cp);
        FM_SNPRINTF_S(reply, replyLen, "%s: No support\n", cmdStr);
    }
    else if (strncasecmp(cp, "set_sbus", 8) == 0)
    {
        FM_LOG_DEBUG3( FM_LOG_CAT_PLATFORM, "set_sbus: %s\n", cp);
        FM_SNPRINTF_S(reply, replyLen, "%s: No support\n", cmdStr);
    }
    else if (strncasecmp(cp, "version", 7) == 0)
    {
        FM_SNPRINTF_S(reply, replyLen, "%s", "Version: FM10K API Sbus Server\n");
    }
    else if (strncasecmp(cp, "status", 6) == 0)
    {
        FM_LOG_DEBUG3( FM_LOG_CAT_PLATFORM, "STATUS: %s\n", cp);
        /* Fake the reply from the dev board */
        FM_SNPRINTF_S(reply, replyLen, "%s", 
            "-----------------------------------;Version:              FM10K API;"
            "Uptime:               1d 2:34:56;ASIC type:            IDCODE: 01011000000000000000010101111111;"
            "TAP generation:       3;SBus mode:            JTAG;Current chip:         0 (of 0..0);"
            "-----------------------------------\n");
    }
    else
    {
        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM, "UNKNOWN: %s => %s\n", cp, cmdStr);
        FM_SNPRINTF_S(reply, replyLen, "%s: Unknown command\n", cmdStr);
    }

    return FM_OK;

}   /* end ProcessCmdStr */




/*****************************************************************************/
/* platformSbusServer
 *
 * \desc            Start a SBUS server.
 *
 * \param[in]       tcpPort is the TCP port to listen on.
 *
 * \return          NONE. 
 *
 ****************************************************************************/
static void platformSbusServer(fm_int tcpPort)
{
    int                     rc;
    int                     fdSocket;
    int                     fdConn;
    struct sockaddr_in      sai;
    struct sockaddr_in      clientIp;
    socklen_t               addrLen; 
    fm_char                 strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t                 strErrNum;
    fm_text                 pCmd;
    int                     readLen;
    int                     replyLen;
    fm_int                  numCmd;
    fm_timestamp            t1;
    fm_timestamp            t2;
    fm_timestamp            tDiff;

    FM_LOG_PRINT("SBUS Server is now listening for TCP connections on port %d...\n", tcpPort);

    fdSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (fdSocket < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                     "Unable to open socket: '%s'\n", strErrBuf );
        return;
    }

    rc = 0;
    rc = setsockopt(fdSocket, SOL_SOCKET, SO_REUSEADDR, (void *) &rc, sizeof(rc));

    if (rc < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                     "Unable to set socket option: '%s'\n", strErrBuf );
        close(fdSocket);
        return;
    }

    sai.sin_family      = AF_INET;
    sai.sin_addr.s_addr = INADDR_ANY;
    sai.sin_port        = htons(tcpPort);

    /* SECURITY NOTE: Binding to INADDR_ANY is an insecure practice. We do so
     * here in order to simplify testing. Customers should consider this issue 
     * when implementing their platforms and adjust their code accordingly. 
     * Ignored in Klocwork. */ 
    if ((rc = bind(fdSocket, (struct sockaddr *) &sai, sizeof (sai))) < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                     "Unable to bind socket: '%s'\n", strErrBuf );
        close(fdSocket);
        return;
    }

    if ((rc = listen(fdSocket, /* backlog = */ 1)) < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }
        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                     "Unable to listen on socket: '%s'\n", strErrBuf );
        close(fdSocket);
        return;
    }

    addrLen = sizeof(clientIp);
    t1.sec = 0;
    t1.usec = 0;

    while (1)
    {
        /* Reduce output since AACS continously opening and closing socket */
        fmGetTime(&t2);
        fmSubTimestamps(&t2, &t1, &tDiff);
        if (tDiff.sec > 5)
        {
            FM_LOG_PRINT("Waiting for a connection on TCP port %d\n", tcpPort);
            fmGetTime(&t1);            
        }

        fdConn = accept(fdSocket, (struct sockaddr *) &clientIp, &addrLen);

        if (fdConn < 0)
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum)
            {
                FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
            }
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                         "Unable to accept connection from %s: '%s'\n", inet_ntoa(clientIp.sin_addr), strErrBuf );
        }
        else
        {
            if (tDiff.sec > 5)
            {
                FM_LOG_INFO( FM_LOG_CAT_PLATFORM,
                            "Got connection from %s:%d\n", inet_ntoa(clientIp.sin_addr), tcpPort);

                FM_LOG_PRINT("Got connection from %s:%d\n", inet_ntoa(clientIp.sin_addr), tcpPort);
            }
            while (1)
            {
                readLen = recv(fdConn, (void *) cmdStr, CMD_BUF_LEN - 1, 0);
                if (readLen == 0)
                {
                    close(fdConn);
                    break;
                }

                if (readLen < 0)
                {
                    strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
                    if (strErrNum)
                    {
                        FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
                    }
                    FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                                 "Unable to receive data: '%s'\n", strErrBuf );
                    close(fdConn);
                    close(fdSocket);
                    return;
                }

                if (strncasecmp(cmdStr, "close", 5) == 0)
                {
                    FM_LOG_DEBUG2(FM_LOG_CAT_PLATFORM, "Socket close request.\n");
                    close(fdConn);
                    break;
                }

                /* Remove trailing CR */
                if ((readLen >= 1) && (cmdStr[readLen - 1] == '\n'))
                {
                    readLen--;
                }
                if ((readLen >= 1) && (cmdStr[readLen - 1] == '\r'))
                {
                    readLen--;
                }
                /* Terminate with NULL */
                cmdStr[readLen] = '\0';
                FM_LOG_DEBUG3(FM_LOG_CAT_PLATFORM, "CMD: |%s|\n", cmdStr);

                pCmd = cmdStr;

                numCmd = 0;
                replyLen = 0;
                replyStr[0] = '\0';

                if (strlen(pCmd) < 3)
                {
                    FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                                 "Command [%s]is too short.\n", pCmd);
                }

                while (pCmd && (strlen(pCmd) >= 3) && ((pCmd - cmdStr) < readLen))
                {
                    if (numCmd)
                    {
                        replyLen = strlen(replyStr);
                        replyStr[replyLen] = ';';
                        replyLen++;
                        replyStr[replyLen] = '\0';
                    }
                    numCmd++;

                    if (replyLen < REPLY_BUF_LEN - 32)
                    {
                        ProcessCmdStr(pCmd, replyStr + replyLen, REPLY_BUF_LEN - replyLen);
                    }
                    else
                    {
                        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                                 "Reply buffer is too small (%d) for %d commands. ReadLen %d\n", replyLen, numCmd, readLen);   
                        printf(">>>|%s| => (%d) [%s]", cmdStr, replyLen, replyStr);
                    }

                    /* Get next batched command */
                    pCmd = strstr(pCmd + 2, ";");
                }

                replyLen = strlen(replyStr);
                if (replyLen && (replyLen < REPLY_BUF_LEN) && (replyStr[replyLen-1] != '\n'))
                {
                    replyLen++;
                    replyStr[replyLen-1] = '\n';
                    replyStr[replyLen] = '\0';
                }

                if (replyLen == 0)
                {
                    /* Send back something or the app will hang */
                    FM_SNPRINTF_S(replyStr, REPLY_BUF_LEN, "%s", ";\n");
                    replyLen = strlen(replyStr);
                }
                FM_LOG_DEBUG2(FM_LOG_CAT_PLATFORM, "REPLY: |%s| => (%d) [%s]", cmdStr, replyLen, replyStr);

                if ((replyLen = send(fdConn, (void *) replyStr, replyLen, 0)) < 0)
                {
                    strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
                    if (strErrNum)
                    {
                        FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
                    }
                    FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                                 "Unable to send %d bytes of data: '%s'\n", replyLen, strErrBuf );
                    close(fdConn);
                    close(fdSocket);
                    return;
                }
            }
        }
    }

    close(fdConn);
    close(fdSocket);

}   /* end platformSbusServer */




/*****************************************************************************/
/* fmPlatformSbusServerThread
 * \ingroup intPlatform
 *
 * \desc            Thread to provide SBUS access via socket.
 *
 * \param[in]       args contains thread-initialization parameters
 *
 * \return          None.
 *
 *****************************************************************************/
static void *fmPlatformSbusServerThread(void *args)
{
    fm_int         cnt;

    fmDelay(5, 0);

    /* Try a few times and exit if failed */
    for (cnt = 0 ; cnt < 30 ; cnt ++)
    {
        platformSbusServer(serverTcpPort);

        /* Should not get to here */
        fmDelay(10,0);

    }

    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "SBUS Server Thread: Exiting...\n");

    return NULL;

}   /* end fmPlatformSbusServerThread */





/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/* fm10000SbusServerStart
 * \ingroup intPlatform
 *
 * \desc            Start SBUS Server.
 *
 * \param[in]       tcpPort is the tcp port to listen on.
 *
 * \return          None.
 *
 *****************************************************************************/
fm_status fm10000SbusServerStart(fm_int tcpPort)
{
    fm_status err;

    serverTcpPort = tcpPort;


    err = fmCreateThread("SBUS Server Thread",
                         FM_EVENT_QUEUE_SIZE_NONE,
                         &fmPlatformSbusServerThread,
                         NULL,
                         &sbusServerThread);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to start SBUS Server Thread: %s\n",
                     fmErrorMsg(err));
    }

    return err;

}   /* end fm10000SbusServerStart */


