/* vim:et:sw=4:ts=4:tw=79:
 */
/*****************************************************************************
 * File:            fm10000_api_an_int.h
 * Creation Date:   April 5, 2014
 * Description:     Autonegotiation support for SGMII, Clause 37 and Clause 73.
 *
 * Copyright (c) 2014, Intel Corporation
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

#ifndef __FM_FM10000_API_AN_INT_H
#define __FM_FM10000_API_AN_INT_H

#define BREAK_LINK_TIMER_MILLISEC           65 /* From Table 73-7 */
#define LINK_INHIBIT_TIMER_MILLISEC        500 /* From Table 73-7 */
#define LINK_INHIBIT_TIMER_MILLISEC_KX      50 /* KX or KX4 */

#define FM10000_AN37_INT_MASK (                           \
    (1U << FM10000_AN_IP_b_An37AnRestart)           |     \
    (1U << FM10000_AN_IP_b_An37AbilityDetect)       |     \
    (1U << FM10000_AN_IP_b_An37AcknowledgeDetect)   |     \
    (1U << FM10000_AN_IP_b_An37NextPageWait)        |     \
    (1U << FM10000_AN_IP_b_An37CompleteAcknowledge) |     \
    (1U << FM10000_AN_IP_b_An37IdleDetect)          |     \
    (1U << FM10000_AN_IP_b_An37LinkOk) )

#define FM10000_AN73_INT_MASK (                           \
    (1U << FM10000_AN_IP_b_An73TransmitDisable)     |     \
    (1U << FM10000_AN_IP_b_An73AbilityDetect)       |     \
    (1U << FM10000_AN_IP_b_An73AcknowledgeDetect)   |     \
    (1U << FM10000_AN_IP_b_An73CompleteAcknowledge) |     \
    (1U << FM10000_AN_IP_b_An73NextPageWait)        |     \
    (1U << FM10000_AN_IP_b_An73AnGoodCheck)         |     \
    (1U << FM10000_AN_IP_b_An73AnGood) )

#define FM10000_AN_INT_MASK (FM10000_AN37_INT_MASK | FM10000_AN73_INT_MASK)

/* Technology Ability Field */
#define FM10000_AN73_ABILITY_1000BASE_KX    (1 << 0)
#define FM10000_AN73_ABILITY_10GBASE_KX4    (1 << 1)
#define FM10000_AN73_ABILITY_10GBASE_KR     (1 << 2)
#define FM10000_AN73_ABILITY_40GBASE_KR4    (1 << 3)
#define FM10000_AN73_ABILITY_40GBASE_CR4    (1 << 4)
#define FM10000_AN73_ABILITY_100GBASE_CR10  (1 << 5)
#define FM10000_AN73_ABILITY_100GBASE_KP4   (1 << 6)
#define FM10000_AN73_ABILITY_100GBASE_KR4   (1 << 7)
#define FM10000_AN73_ABILITY_100GBASE_CR4   (1 << 8)
/* internal, subject to change */
#define FM10000_AN73_ABILITY_25GBASE_KR     (1 << 9)

/* supported abilities */
#define FM10000_AN73_SUPPORTED_ABILITIES      \
    ( FM10000_AN73_ABILITY_1000BASE_KX      | \
      FM10000_AN73_ABILITY_10GBASE_KR       | \
      FM10000_AN73_ABILITY_40GBASE_KR4      | \
      FM10000_AN73_ABILITY_40GBASE_CR4      | \
      FM10000_AN73_ABILITY_25GBASE_KR       | \
      FM10000_AN73_ABILITY_100GBASE_KR4     | \
      FM10000_AN73_ABILITY_100GBASE_CR4 )

/* unsupported abilities */
#define FM10000_AN73_UNSUPPORTED_ABILITIES    \
    ( FM10000_AN73_ABILITY_10GBASE_KX4      | \
      FM10000_AN73_ABILITY_100GBASE_CR10    | \
      FM10000_AN73_ABILITY_100GBASE_KP4 ) 
   
/* multi-lane abilities */
#define FM10000_AN73_ABILITY_MULTI_LANE       \
    ( FM10000_AN73_ABILITY_40GBASE_KR4      | \
      FM10000_AN73_ABILITY_40GBASE_CR4      | \
      FM10000_AN73_ABILITY_100GBASE_KR4     | \
      FM10000_AN73_ABILITY_100GBASE_CR4  )


#define FM10000_AN37_LINK_TIMER_TIMEOUT_MAX        0x7F
#define FM10000_AN73_BREAK_LINK_TIMEOUT_MAX        0x7F
#define FM10000_AN73_LINK_FAIL_INHIBIT_TIMEOUT_MAX 0x1FF
#define FM10000_AN73_LINK_FAIL_INHIBIT_TIMEOUT_DEBUG 0x3FF

enum
{
    AN73_HCD_INCOMPATIBLE_LINK = 0,
    AN73_HCD_10_KR,
    AN73_HCD_KX4,
    AN73_HCD_KX,
    AN73_HCD_40_KR4,
    AN73_HCD_40_CR4,
    AN73_HCD_100_CR10,
    AN73_HCD_100_KP4, 
    AN73_HCD_100_KR4, 
    AN73_HCD_100_CR4,
    AN73_HCD_25_KR
};

/* EEE technology message code (10)*/
#define FM10000_AN_NEXTPAGE_EEE_MSG_CODE    0xA
/*
 *      AN-73
 * Advertise that the 10GBASE-KR has EEE capability : U6 (0x4)
 * NEXT_PAGE NP = 0
 * NEXT_PAGE ACK = 0
 * NEXT_PAGE MP bit = 1
 * NEXT_PAGE ACK2 bit = 0
 * NEXT_PAGE T bit = 0
 * Next Page Message code 10 (0xA)
 */
#define FM10000_AN_73_NEXTPAGE_EEE               0x00000050200A
#define FM10000_AN_73_NEXTPAGE_EEE_10GBASE_KR    0x000000400000
#define FM10000_AN_73_NEXTPAGE_EEE_1000BASE_KX   0x000000100000

/* Max number of next pages to allocate */
#define MAX_NUM_NEXTPAGES  16

fm_status fm10000IsPortAutonegReady( fm_int     sw, 
                                     fm_int     port, 
                                     fm_ethMode ethMode,
                                     fm_uint32  anMode,
                                     fm_bool   *ready,
                                     fm_int    *smType );
fm_status fm10000An73SetLinkInhibitTimer( fm_int sw, 
                                          fm_int port, 
                                          fm_uint timeout );
fm_status fm10000An73SetLinkInhibitTimerKx( fm_int  sw,
                                            fm_int  port,
                                            fm_uint timeout );
fm_status fm10000An73SetIgnoreNonce( fm_int sw, 
                                     fm_int port,
                                     fm_bool ignoreNonce );

fm_uint   fm10000AnGetTimeScale( fm_uint  timeoutUsec,
                                 fm_uint  timeoutMax,
                                 fm_uint *timeScale,
                                 fm_uint *timeout );


fm_status fm10000AnSendConfigEvent( fm_int          sw,
                                    fm_int          port, 
                                    fm_int          eventId,
                                    fm_uint32       mode,
                                    fm_uint64       basepage,
                                    fm_anNextPages  nextPages );

fm_status fm10000AnRestartOnNewConfig( fm_int          sw,
                                       fm_int          port, 
                                       fm_ethMode      ethMode,
                                       fm_uint32       anMode,
                                       fm_uint64       basepage,
                                       fm_anNextPages  nextPages );

fm_status fm10000AnValidateBasePage( fm_int     sw, 
                                     fm_int     port, 
                                     fm_uint32  mode,
                                     fm_uint64  basepage,
                                     fm_uint64 *modBasePage );

fm_status fm10000AnEventHandler( fm_int    sw, 
                                 fm_int    epl, 
                                 fm_int    lane,
                                 fm_uint32 anIp );

fm_ethMode fm10000An73HcdToEthMode( fm_int hcd );

fm_text fm10000An73HCDStr(fm_uint value);

fm_status fm10000AnAddNextPage(fm_int sw, fm_int port, fm_uint64 nextPage);
fm_status fm10000AnVerifyEeeNegotiation(fm_int sw, fm_int port, fm_int ethMode);

#endif /* __FM_FM10000_API_AN_INT_H */

