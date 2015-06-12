/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_stats.c
 * Creation Date:   April 19, 2013
 * Description:     Structures and functions for dealing with counters
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#define FM10000_MAX_VLAN_COUNTER            63

#define FM10000_NB_RX_STATS_BANKS           FM10000_RX_STATS_BANK_ENTRIES_1
#define FM10000_BINS_PER_RX_STATS_BANK      16
#define FM10000_WORDS_PER_RX_STATS_COUNTER  FM10000_RX_STATS_BANK_WIDTH

/* Max expected entries in read stats scatter gather list */
#define MAX_STATS_SGLIST 128

/** Add a 32bit read of an EPL counter to the scatter gather
 *  list.
 *  
 * Depends on the existence of the variables: 
 * sgList [out] - scatter gather array to hold the reads 
 * sgListCnt [out] - number of entry in the sgList 
 */
#define FM10000_GET_EPL_PORT_STAT_32(reg, var)                         \
    sgList[sgListCnt].addr = reg(epl, lane);                        \
    sgList[sgListCnt].data = (fm_uint32*)&counters->var;               \
    sgList[sgListCnt].count = 1;                                       \
    sgListCnt++;

/** Clear a 32bit EPL counter by adding it to the scatter gather
 *  list.
 *  
 * Depends on the existence of the variables: 
 * sgList [out] - scatter gather array to hold the reads 
 * sgListCnt [out] - number of entry in the sgList 
 */
#define FM10000_SET_EPL_PORT_STAT_32(reg, varPtr)                      \
    sgList[sgListCnt].addr = reg(epl, lane);                        \
    sgList[sgListCnt].data = (fm_uint32*)varPtr;                       \
    sgList[sgListCnt].count = 1;                                       \
    sgListCnt++;

/** Add a 64bit read of a tx counter to the scatter gather list.
 *  
 * Depends on the existence of the variables: 
 * sgList [out] - scatter gather array to hold the reads 
 * sgListCnt [out] - number of entry in the sgList 
 */
#define FM10000_GET_TX_PORT_STAT_64(bank, bin, frameOffset, byteOffset)      \
    sgList[sgListCnt].addr =                                                 \
        FM10000_MOD_STATS_BANK_FRAME(bank, (physPort << 4 | bin), 0);        \
    sgList[sgListCnt].data = (fm_uint32*)(((fm_byte*)counters)+frameOffset); \
    sgList[sgListCnt].count = 2;                                             \
    sgListCnt++;                                                             \
    sgList[sgListCnt].addr =                                                 \
        FM10000_MOD_STATS_BANK_BYTE(bank, (physPort << 4 | bin), 0);         \
    sgList[sgListCnt].data = (fm_uint32*)(((fm_byte*)counters)+byteOffset);  \
    sgList[sgListCnt].count = 2;                                             \
    sgListCnt++;

/** Add a 128bit read of an rx counter to the scatter gather
 *  list.
 *  
 * NOTE: Read values will be stored in the temporary 
 *       array switchExt->cntRxPortStatsBank;
 *  
 * Depends on the existence of the variables: 
 * switchExt->cntRxPortStatsBank [out] - Array to store the 
 * 128bit values sgList 
 * [out] - scatter gather array to hold the reads sgListCnt 
 * [out] - number of entry in the sgList 
 */
#define FM10000_GET_RX_PORT_STAT_128(bank, bin)                         \
    sgList[sgListCnt].addr =                                            \
        FM10000_RX_STATS_BANK(bank, (physPort << 4 | bin), 0);          \
    sgList[sgListCnt].data = cntRxPortStatsBank[bank][bin];             \
    sgList[sgListCnt].count = 4;                                        \
    sgListCnt++;

/** Update the counter variable related to a 128bit HW counter
 *  
 * NOTE: Read values will be retrieved from the temporary 
 *       array switchExt->cntRxPortStatsBank;
 *  
 * Depends on the existence of the variables: 
 * switchExt->cntRxPortStatsBank [in] - Array to read the 
 * 128bit values from 
 */
#define FM10000_UPD_RX_PORT_STAT_128(bank, bin, frameOffset, byteOffset)    \
    *( (fm_uint64*) (((fm_byte*)counters)+frameOffset) ) = (fm_uint64) (    \
        ((fm_uint64)(cntRxPortStatsBank[bank][bin][1]) << 32) |             \
        ((fm_uint64)(cntRxPortStatsBank[bank][bin][0]) ) );                 \
    *( (fm_uint64*) (((fm_byte*)counters)+byteOffset) ) = (fm_uint64) (     \
        ((fm_uint64)(cntRxPortStatsBank[bank][bin][3]) << 32) |             \
        ((fm_uint64)(cntRxPortStatsBank[bank][bin][2]) ) );                 \
    
     


/* Rx Bank Definitions */
#define FM10000_RX_STAT_BANK_TYPE                       0
#define FM10000_RX_STAT_BANK_SIZE                       1
#define FM10000_RX_STAT_BANK_PRI                        2
#define FM10000_RX_STAT_BANK_FWD_1                      3
#define FM10000_RX_STAT_BANK_FWD_2                      4
#define FM10000_RX_STAT_BANK_VLAN                       5

/* FM10000_RX_STAT_BANK_TYPE Bin Definitions */
#define FM10000_RX_STAT_NONIP_L2UCAST                   0
#define FM10000_RX_STAT_NONIP_L2MCAST                   1
#define FM10000_RX_STAT_NONIP_L2BCAST                   2
#define FM10000_RX_STAT_IPV4_L2UCAST                    3
#define FM10000_RX_STAT_IPV4_L2MCAST                    4
#define FM10000_RX_STAT_IPV4_L2BCAST                    5
#define FM10000_RX_STAT_IPV6_L2UCAST                    6
#define FM10000_RX_STAT_IPV6_L2MCAST                    7
#define FM10000_RX_STAT_IPV6_L2BCAST                    8
#define FM10000_RX_STAT_IEEE802_3_PAUSE                 9
#define FM10000_RX_STAT_CLASS_BASED_PAUSE               10
#define FM10000_RX_STAT_FRAMING_ERR                     11
#define FM10000_RX_STAT_FCS_ERR                         12

/* FM10000_RX_STAT_BANK_SIZE Bin Definitions */
#define FM10000_RX_STAT_LEN_LT_64                       0 
#define FM10000_RX_STAT_LEN_EQ_64                       1 
#define FM10000_RX_STAT_LEN_65_127                      2 
#define FM10000_RX_STAT_LEN_128_255                     3 
#define FM10000_RX_STAT_LEN_256_511                     4 
#define FM10000_RX_STAT_LEN_512_1023                    5 
#define FM10000_RX_STAT_LEN_1024_1522                   6 
#define FM10000_RX_STAT_LEN_1523_2047                   7 
#define FM10000_RX_STAT_LEN_2048_4095                   8 
#define FM10000_RX_STAT_LEN_4096_8191                   9 
#define FM10000_RX_STAT_LEN_8192_10239                  10
#define FM10000_RX_STAT_LEN_GE_10240                    11

/* FM10000_RX_STAT_BANK_PRI Bin Definitions */
#define FM10000_RX_STAT_PRI_0                           0 
#define FM10000_RX_STAT_PRI_1                           1 
#define FM10000_RX_STAT_PRI_2                           2 
#define FM10000_RX_STAT_PRI_3                           3 
#define FM10000_RX_STAT_PRI_4                           4 
#define FM10000_RX_STAT_PRI_5                           5 
#define FM10000_RX_STAT_PRI_6                           6 
#define FM10000_RX_STAT_PRI_7                           7 
#define FM10000_RX_STAT_PRI_8                           8 
#define FM10000_RX_STAT_PRI_9                           9 
#define FM10000_RX_STAT_PRI_10                          10
#define FM10000_RX_STAT_PRI_11                          11
#define FM10000_RX_STAT_PRI_12                          12
#define FM10000_RX_STAT_PRI_13                          13
#define FM10000_RX_STAT_PRI_14                          14
#define FM10000_RX_STAT_PRI_15                          15

/* FM10000_RX_STAT_BANK_FWD_1 Bin Definitions */
#define FM10000_RX_STAT_FID_FORWARDED                   0 
#define FM10000_RX_STAT_FLOOD_FORWARDED                 1 
#define FM10000_RX_STAT_SPECIALLY_HANDLED               2 
#define FM10000_RX_STAT_PARSER_ERROR_DROP               3 
#define FM10000_RX_STAT_ECC_ERROR_DROP                  4 
#define FM10000_RX_STAT_TRAPPED                         5 
#define FM10000_RX_STAT_PAUSE_DROPS                     6 
#define FM10000_RX_STAT_STP_DROPS                       7 
#define FM10000_RX_STAT_SECURITY_VIOLATIONS             8 
#define FM10000_RX_STAT_VLAN_TAG_DROPS                  9 
#define FM10000_RX_STAT_VLAN_INGRESS_DROPS              10
#define FM10000_RX_STAT_VLAN_EGRESS_DROPS               11
#define FM10000_RX_STAT_GLORT_MISS_DROPS                12
#define FM10000_RX_STAT_FFU_DROPS                       13
#define FM10000_RX_STAT_TRIGGER_DROPS                   14
#define FM10000_RX_STAT_OVERFLOW4                       15

/* FM10000_RX_STAT_BANK_FWD_2 Bin Definitions */
#define FM10000_RX_STAT_POLICER_DROPS                   0 
#define FM10000_RX_STAT_TTL_DROPS                       1 
#define FM10000_RX_STAT_CM_PRIV_DROPS                   2 
#define FM10000_RX_STAT_CM_SMP0_DROPS                   3 
#define FM10000_RX_STAT_CM_SMP1_DROPS                   4 
#define FM10000_RX_STAT_CM_RX_HOG_0_DROPS               5 
#define FM10000_RX_STAT_CM_RX_HOG_1_DROPS               6 
#define FM10000_RX_STAT_CM_TX_HOG_0_DROPS               7 
#define FM10000_RX_STAT_CM_TX_HOG_1_DROPS               8 
/* Bin 9 reserved */
#define FM10000_RX_STAT_TRIGGER_REDIRECTS               10
#define FM10000_RX_STAT_FLOOD_CONTROL_DROPS             11
#define FM10000_RX_STAT_GLORT_FORWARDED                 12
#define FM10000_RX_STAT_LOOPBACK_SUPPRESS               13
#define FM10000_RX_STAT_OTHERS                          14
#define FM10000_RX_STAT_OVERFLOW5                       15

/* FM10000_RX_STAT_BANK_VLAN Bin definitions */
#define FM10000_RX_STAT_VLAN_UCAST                      0
#define FM10000_RX_STAT_VLAN_MCAST                      1
#define FM10000_RX_STAT_VLAN_BCAST                      2

/* Rx Bank Enable Bits */
#define FM10000_RX_STAT_BANK_EN_TYPE                    (1 << 0)
#define FM10000_RX_STAT_BANK_EN_SIZE                    (1 << 1)
#define FM10000_RX_STAT_BANK_EN_PRI                     (1 << 2)
#define FM10000_RX_STAT_BANK_EN_FORWARDING_1            (1 << 3)
#define FM10000_RX_STAT_BANK_EN_FORWARDING_2            (1 << 4)
#define FM10000_RX_STAT_BANK_EN_VLAN                    (1 << 5)

#define FM10000_RX_STAT_BANK_EN_ALL_PORT                  \
                ( FM10000_RX_STAT_BANK_EN_TYPE |          \
                  FM10000_RX_STAT_BANK_EN_SIZE |          \
                  FM10000_RX_STAT_BANK_EN_PRI  |          \
                  FM10000_RX_STAT_BANK_EN_FORWARDING_1 |  \
                  FM10000_RX_STAT_BANK_EN_FORWARDING_2)

#define FM10000_RX_STAT_BANK_EN_ALL                       \
                ( FM10000_RX_STAT_BANK_EN_ALL_PORT |      \
                  FM10000_RX_STAT_BANK_EN_VLAN)

/* Tx Bank Definitions */
#define FM10000_TX_STAT_BANK_TYPE                       0
#define FM10000_TX_STAT_BANK_SIZE                       1

/* FM10000_TX_STAT_BANK_TYPE Bin Definitions */
#define FM10000_TX_STAT_L2UCAST                         0 
#define FM10000_TX_STAT_L2MCAST                         1 
#define FM10000_TX_STAT_L2BCAST                         2
#define FM10000_TX_STAT_ERR_SENT                        3 
#define FM10000_TX_STAT_TIMEOUT_DROP                    4 
#define FM10000_TX_STAT_ERR_DROP                        5 
#define FM10000_TX_STAT_ECC_DROP                        6 
#define FM10000_TX_STAT_LOOPBACK_DROP                   7 
#define FM10000_TX_STAT_TTL1_DROP                       8 
#define FM10000_TX_STAT_IEEE802_3_PAUSE                 9
#define FM10000_TX_STAT_CLASS_BASED_PAUSE               10 

/* FM10000_TX_STAT_BANK_SIZE Bin Definitions */
#define FM10000_TX_STAT_LEN_LT_64                       0 
#define FM10000_TX_STAT_LEN_EQ_64                       1 
#define FM10000_TX_STAT_LEN_65_127                      2 
#define FM10000_TX_STAT_LEN_128_255                     3 
#define FM10000_TX_STAT_LEN_256_511                     4 
#define FM10000_TX_STAT_LEN_512_1023                    5 
#define FM10000_TX_STAT_LEN_1024_1522                   6 
#define FM10000_TX_STAT_LEN_1523_2047                   7 
#define FM10000_TX_STAT_LEN_2048_4095                   8 
#define FM10000_TX_STAT_LEN_4096_8191                   9 
#define FM10000_TX_STAT_LEN_8192_10239                  10
#define FM10000_TX_STAT_LEN_GE_10240                    11

/* Tx Bank Enable Bits */
#define FM10000_TX_STAT_BANK_EN_TYPE                    (1 << 0)
#define FM10000_TX_STAT_BANK_EN_SIZE                    (1 << 1)

#define FM10000_TX_STAT_BANK_EN_ALL                     \
                ( FM10000_TX_STAT_BANK_EN_TYPE |        \
                  FM10000_TX_STAT_BANK_EN_SIZE )

/* Priority selects */
#define FM10000_PRI_SEL_TC                              0
#define FM10000_PRI_SEL_SWITCH_PRI                      1

/* This structure is used to build a table of all rx / tx counters */
typedef struct _fm10000_portCounterMapping
{
   /* Bank in which the counter is located */
   fm_uint32 bank;          

   /* Bin  in which the counter is located */
   fm_uint32 bin;

   /* Offset (in bytes) of a the frame counter in
    * the fm_portCounters structure */
   fm_uint32 frameOffset;   

   /* Offset (in bytes) of a the byte counter in
    * the fm_portCounters structure */
   fm_uint32 byteOffset;    

} fm10000_portCounterMapping;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/* Table of all RX port counters in the RX banks
 * This table is used for retrieving counters as
 * well as reseting them */
static fm10000_portCounterMapping rxPortCntMapTable[] = 
{
    /* Bank                        Bin                                  frameOffset                                          byteOffset                                            */
    /* --------------------------  ------------------------------       ---------------------------------------------        ---------------------------------------------------   */

    /* Type Bank */
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_NONIP_L2UCAST,       offsetof(fm_portCounters, cntRxUcstPktsNonIP),       offsetof(fm_portCounters, cntRxUcstOctetsNonIP)    },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_NONIP_L2MCAST,       offsetof(fm_portCounters, cntRxMcstPktsNonIP),       offsetof(fm_portCounters, cntRxMcstOctetsNonIP)    },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_NONIP_L2BCAST,       offsetof(fm_portCounters, cntRxBcstPktsNonIP),       offsetof(fm_portCounters, cntRxBcstOctetsNonIP)    },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IPV4_L2UCAST,        offsetof(fm_portCounters, cntRxUcstPktsIPv4),        offsetof(fm_portCounters, cntRxUcstOctetsIPv4)     },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IPV4_L2MCAST,        offsetof(fm_portCounters, cntRxMcstPktsIPv4),        offsetof(fm_portCounters, cntRxMcstOctetsIPv4)     },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IPV4_L2BCAST,        offsetof(fm_portCounters, cntRxBcstPktsIPv4),        offsetof(fm_portCounters, cntRxBcstOctetsIPv4)     },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IPV6_L2UCAST,        offsetof(fm_portCounters, cntRxUcstPktsIPv6),        offsetof(fm_portCounters, cntRxUcstOctetsIPv6)     },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IPV6_L2MCAST,        offsetof(fm_portCounters, cntRxMcstPktsIPv6),        offsetof(fm_portCounters, cntRxMcstOctetsIPv6)     },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IPV6_L2BCAST,        offsetof(fm_portCounters, cntRxBcstPktsIPv6),        offsetof(fm_portCounters, cntRxBcstOctetsIPv6)     },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_IEEE802_3_PAUSE,     offsetof(fm_portCounters, cntRxPausePkts),           offsetof(fm_portCounters, cntRxPauseOctets)        },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_CLASS_BASED_PAUSE,   offsetof(fm_portCounters, cntRxCBPausePkts),         offsetof(fm_portCounters, cntRxCBPauseOctets)      },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_FRAMING_ERR,         offsetof(fm_portCounters, cntRxFramingErrorPkts),    offsetof(fm_portCounters, cntRxFramingErrorOctets) },
    {  FM10000_RX_STAT_BANK_TYPE,  FM10000_RX_STAT_FCS_ERR,             offsetof(fm_portCounters, cntRxFCSErrors),           offsetof(fm_portCounters, cntRxFCSErrorsOctets)    },

    /* Size Bank */
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_LT_64,           offsetof(fm_portCounters, cntRxMinTo63Pkts),         offsetof(fm_portCounters, cntRxMinTo63octets)     },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_EQ_64,           offsetof(fm_portCounters, cntRx64Pkts),              offsetof(fm_portCounters, cntRx64octets)          },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_65_127,          offsetof(fm_portCounters, cntRx65to127Pkts),         offsetof(fm_portCounters, cntRx65to127octets)     },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_128_255,         offsetof(fm_portCounters, cntRx128to255Pkts),        offsetof(fm_portCounters, cntRx128to255octets)    },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_256_511,         offsetof(fm_portCounters, cntRx256to511Pkts),        offsetof(fm_portCounters, cntRx256to511octets)    },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_512_1023,        offsetof(fm_portCounters, cntRx512to1023Pkts),       offsetof(fm_portCounters, cntRx512to1023octets)   },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_1024_1522,       offsetof(fm_portCounters, cntRx1024to1522Pkts),      offsetof(fm_portCounters, cntRx1024to1522octets)  },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_1523_2047,       offsetof(fm_portCounters, cntRx1523to2047Pkts),      offsetof(fm_portCounters, cntRx1523to2047octets)  },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_2048_4095,       offsetof(fm_portCounters, cntRx2048to4095Pkts),      offsetof(fm_portCounters, cntRx2048to4095octets)  },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_4096_8191,       offsetof(fm_portCounters, cntRx4096to8191Pkts),      offsetof(fm_portCounters, cntRx4096to8191octets)  },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_8192_10239,      offsetof(fm_portCounters, cntRx8192to10239Pkts),     offsetof(fm_portCounters, cntRx8192to10239octets) },
    {  FM10000_RX_STAT_BANK_SIZE,  FM10000_RX_STAT_LEN_GE_10240,        offsetof(fm_portCounters, cntRx10240toMaxPkts),      offsetof(fm_portCounters, cntRx10240toMaxOctets)  },
    
    /* Priority Bank */
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_0,               offsetof(fm_portCounters, cntRxPriorityPkts[0]),     offsetof(fm_portCounters, cntRxPriorityOctets[0])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_1,               offsetof(fm_portCounters, cntRxPriorityPkts[1]),     offsetof(fm_portCounters, cntRxPriorityOctets[1])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_2,               offsetof(fm_portCounters, cntRxPriorityPkts[2]),     offsetof(fm_portCounters, cntRxPriorityOctets[2])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_3,               offsetof(fm_portCounters, cntRxPriorityPkts[3]),     offsetof(fm_portCounters, cntRxPriorityOctets[3])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_4,               offsetof(fm_portCounters, cntRxPriorityPkts[4]),     offsetof(fm_portCounters, cntRxPriorityOctets[4])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_5,               offsetof(fm_portCounters, cntRxPriorityPkts[5]),     offsetof(fm_portCounters, cntRxPriorityOctets[5])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_6,               offsetof(fm_portCounters, cntRxPriorityPkts[6]),     offsetof(fm_portCounters, cntRxPriorityOctets[6])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_7,               offsetof(fm_portCounters, cntRxPriorityPkts[7]),     offsetof(fm_portCounters, cntRxPriorityOctets[7])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_8,               offsetof(fm_portCounters, cntRxPriorityPkts[8]),     offsetof(fm_portCounters, cntRxPriorityOctets[8])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_9,               offsetof(fm_portCounters, cntRxPriorityPkts[9]),     offsetof(fm_portCounters, cntRxPriorityOctets[9])  },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_10,              offsetof(fm_portCounters, cntRxPriorityPkts[10]),    offsetof(fm_portCounters, cntRxPriorityOctets[10]) },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_11,              offsetof(fm_portCounters, cntRxPriorityPkts[11]),    offsetof(fm_portCounters, cntRxPriorityOctets[11]) },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_12,              offsetof(fm_portCounters, cntRxPriorityPkts[12]),    offsetof(fm_portCounters, cntRxPriorityOctets[12]) },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_13,              offsetof(fm_portCounters, cntRxPriorityPkts[13]),    offsetof(fm_portCounters, cntRxPriorityOctets[13]) },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_14,              offsetof(fm_portCounters, cntRxPriorityPkts[14]),    offsetof(fm_portCounters, cntRxPriorityOctets[14]) },
    {  FM10000_RX_STAT_BANK_PRI,   FM10000_RX_STAT_PRI_15,              offsetof(fm_portCounters, cntRxPriorityPkts[15]),    offsetof(fm_portCounters, cntRxPriorityOctets[15]) },

    /* Forwarding Bank */
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_FID_FORWARDED,       offsetof(fm_portCounters, cntFIDForwardedPkts),      offsetof(fm_portCounters, cntFIDForwardedOctets)       },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_FLOOD_FORWARDED,     offsetof(fm_portCounters, cntFloodForwardedPkts),    offsetof(fm_portCounters, cntFloodForwardedOctets)     },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_SPECIALLY_HANDLED,   offsetof(fm_portCounters, cntSpeciallyHandledPkts),  offsetof(fm_portCounters, cntSpeciallyHandledOctets)   },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_PARSER_ERROR_DROP,   offsetof(fm_portCounters, cntParseErrDropPkts),      offsetof(fm_portCounters, cntParseErrDropOctets)       },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_ECC_ERROR_DROP,      offsetof(fm_portCounters, cntParityErrorPkts),       offsetof(fm_portCounters, cntParityErrorOctets)        },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_TRAPPED,             offsetof(fm_portCounters, cntTrappedPkts),           offsetof(fm_portCounters, cntTrappedOctets)            },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_PAUSE_DROPS,         offsetof(fm_portCounters, cntPauseDropPkts),         offsetof(fm_portCounters, cntPauseDropOctets)          },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_STP_DROPS,           offsetof(fm_portCounters, cntSTPDropPkts),           offsetof(fm_portCounters, cntSTPDropOctets)            },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_SECURITY_VIOLATIONS, offsetof(fm_portCounters, cntSecurityViolationPkts), offsetof(fm_portCounters, cntSecurityViolationOctets)  },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_VLAN_TAG_DROPS,      offsetof(fm_portCounters, cntVLANTagDropPkts),       offsetof(fm_portCounters, cntVLANTagDropOctets)        },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_VLAN_INGRESS_DROPS,  offsetof(fm_portCounters, cntVLANIngressBVPkts),     offsetof(fm_portCounters, cntVLANIngressBVOctets)      },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_VLAN_EGRESS_DROPS,   offsetof(fm_portCounters, cntVLANEgressBVPkts),      offsetof(fm_portCounters, cntVLANEgressBVOctets)       },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_GLORT_MISS_DROPS,    offsetof(fm_portCounters, cntGlortMissDropPkts),     offsetof(fm_portCounters, cntGlortMissDropOctets)      },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_FFU_DROPS,           offsetof(fm_portCounters, cntFFUDropPkts),           offsetof(fm_portCounters, cntFFUDropOctets)            },
    {  FM10000_RX_STAT_BANK_FWD_1, FM10000_RX_STAT_TRIGGER_DROPS,       offsetof(fm_portCounters, cntTriggerDropPkts),       offsetof(fm_portCounters, cntTriggerDropOctets)        },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_POLICER_DROPS,       offsetof(fm_portCounters, cntPolicerDropPkts),       offsetof(fm_portCounters, cntPolicerDropOctets)        },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_TTL_DROPS,           offsetof(fm_portCounters, cntTTLDropPkts),           offsetof(fm_portCounters, cntTTLDropOctets)            },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_PRIV_DROPS,       offsetof(fm_portCounters, cntCmPrivDropPkts),        offsetof(fm_portCounters, cntCmPrivDropOctets)         },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_SMP0_DROPS,       offsetof(fm_portCounters, cntSmp0DropPkts),          offsetof(fm_portCounters, cntSmp0DropOctets)           },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_SMP1_DROPS,       offsetof(fm_portCounters, cntSmp1DropPkts),          offsetof(fm_portCounters, cntSmp1DropOctets)           },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_RX_HOG_0_DROPS,   offsetof(fm_portCounters, cntRxHog0DropPkts),        offsetof(fm_portCounters, cntRxHog0DropOctets)         },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_RX_HOG_1_DROPS,   offsetof(fm_portCounters, cntRxHog1DropPkts),        offsetof(fm_portCounters, cntRxHog1DropOctets)         },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_TX_HOG_0_DROPS,   offsetof(fm_portCounters, cntTxHog0DropPkts),        offsetof(fm_portCounters, cntTxHog0DropOctets)         },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_CM_TX_HOG_1_DROPS,   offsetof(fm_portCounters, cntTxHog1DropPkts),        offsetof(fm_portCounters, cntTxHog1DropOctets)         },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_TRIGGER_REDIRECTS,   offsetof(fm_portCounters, cntTriggerRedirPkts),      offsetof(fm_portCounters, cntTriggerRedirOctets)       },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_FLOOD_CONTROL_DROPS, offsetof(fm_portCounters, cntFloodControlDropPkts),  offsetof(fm_portCounters, cntFloodControlDropOctets)   },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_GLORT_FORWARDED,     offsetof(fm_portCounters, cntGlortForwardedPkts),    offsetof(fm_portCounters, cntGlortForwardedOctets)     },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_OTHERS,              offsetof(fm_portCounters, cntOtherPkts),             offsetof(fm_portCounters, cntOtherOctets)              },
    {  FM10000_RX_STAT_BANK_FWD_2, FM10000_RX_STAT_LOOPBACK_SUPPRESS,   offsetof(fm_portCounters, cntLoopbackDropsPkts),     offsetof(fm_portCounters, cntLoopbackDropOctets)       },
     
 }; /* end rxPortCntMapTable */

    
/* Table of all TX port counters in the TX banks
 * This table is used for retrieving counters as
 * well as reseting them */    
static fm10000_portCounterMapping txPortCntMapTable[] = 
{
    /* Bank                        Bin                                  frameOffset                                          byteOffset                                            */
    /* --------------------------  ------------------------------       ---------------------------------------------        ---------------------------------------------------   */

    /* Type Bank */
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_L2UCAST,              offsetof(fm_portCounters, cntTxUcstPkts),            offsetof(fm_portCounters, cntTxUcstOctets)        },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_L2MCAST,              offsetof(fm_portCounters, cntTxMcstPkts),            offsetof(fm_portCounters, cntTxMcstOctets)        },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_L2BCAST,              offsetof(fm_portCounters, cntTxBcstPkts),            offsetof(fm_portCounters, cntTxBcstOctets)        },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_ERR_SENT,             offsetof(fm_portCounters, cntTxErrorSentPkts),       offsetof(fm_portCounters, cntTxErrorSentOctets)   },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_TIMEOUT_DROP,         offsetof(fm_portCounters, cntTxTimeOutPkts),         offsetof(fm_portCounters, cntTxTimeOutOctets)     },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_ERR_DROP,             offsetof(fm_portCounters, cntTxErrorDropPkts),       offsetof(fm_portCounters, cntTxErrorOctets)       },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_ECC_DROP,             offsetof(fm_portCounters, cntTxUnrepairEccPkts),     offsetof(fm_portCounters, cntTxUnrepairEccOctets) },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_LOOPBACK_DROP,        offsetof(fm_portCounters, cntTxLoopbackPkts),        offsetof(fm_portCounters, cntTxLoopbackOctets)    },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_TTL1_DROP,            offsetof(fm_portCounters, cntTxTTLDropPkts),         offsetof(fm_portCounters, cntTxTTLDropOctets)     },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_IEEE802_3_PAUSE,      offsetof(fm_portCounters, cntTxPausePkts),           offsetof(fm_portCounters, cntTxPauseOctets)       },
    {  FM10000_TX_STAT_BANK_TYPE, FM10000_TX_STAT_CLASS_BASED_PAUSE,    offsetof(fm_portCounters, cntTxCBPausePkts),         offsetof(fm_portCounters, cntTxCBPauseOctets)     },
                                                                        
    /* Size Bank */
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_LT_64,            offsetof(fm_portCounters, cntTxMinTo63Pkts),          offsetof(fm_portCounters, cntTxMinTo63octets)        },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_EQ_64,            offsetof(fm_portCounters, cntTx64Pkts),               offsetof(fm_portCounters, cntTx64octets)             },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_65_127,           offsetof(fm_portCounters, cntTx65to127Pkts),          offsetof(fm_portCounters, cntTx65to127octets)        },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_128_255,          offsetof(fm_portCounters, cntTx128to255Pkts),         offsetof(fm_portCounters, cntTx128to255octets)       },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_256_511,          offsetof(fm_portCounters, cntTx256to511Pkts),         offsetof(fm_portCounters, cntTx256to511octets)       },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_512_1023,         offsetof(fm_portCounters, cntTx512to1023Pkts),        offsetof(fm_portCounters, cntTx512to1023octets)      },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_1024_1522,        offsetof(fm_portCounters, cntTx1024to1522Pkts),       offsetof(fm_portCounters, cntTx1024to1522octets)     },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_1523_2047,        offsetof(fm_portCounters, cntTx1523to2047Pkts),       offsetof(fm_portCounters, cntTx1523to2047octets)     },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_2048_4095,        offsetof(fm_portCounters, cntTx2048to4095Pkts),       offsetof(fm_portCounters, cntTx2048to4095octets)     },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_4096_8191,        offsetof(fm_portCounters, cntTx4096to8191Pkts),       offsetof(fm_portCounters, cntTx4096to8191octets)     },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_8192_10239,       offsetof(fm_portCounters, cntTx8192to10239Pkts),      offsetof(fm_portCounters, cntTx8192to10239octets)    },
    {  FM10000_TX_STAT_BANK_SIZE, FM10000_TX_STAT_LEN_GE_10240,         offsetof(fm_portCounters, cntTx10240toMaxPkts),       offsetof(fm_portCounters, cntTx10240toMaxOctets)     },

};  /* end txPortCntMapTable */

/*****************************************************************************
 * Local Function Prototypes
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000SetPrioritySelect
 * \ingroup intStats
 *
 * \desc            Sets the RX priority select for bank 3.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is the physical port for which to retrieve
 *                  statistics.
 * 
 * \param[in]       prioritySelect should be set to FM10000_PRI_SEL_TC or
 *                  FM10000_PRI_SEL_SWITCH_PRI for priority indexing.
 * 
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status fm10000SetPrioritySelect(fm_int    sw,
                                          fm_int    physPort,
                                          fm_bool   prioritySelect)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_uint32  rxStatCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, 
                 "sw=%d physPort=%d, prioritySelect=%d\n", 
                 sw, 
                 physPort,
                 prioritySelect);
    
    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw)

    /* Retrieve existing values */
    err = switchPtr->ReadUINT32(sw, FM10000_RX_STATS_CFG(physPort), &rxStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Apply */
    FM_SET_BIT(rxStatCfg, FM10000_RX_STATS_CFG, PrioritySelect, prioritySelect);

    err = switchPtr->WriteUINT32(sw, FM10000_RX_STATS_CFG(physPort), rxStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    DROP_REG_LOCK(sw)

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* End fm10000SetPrioritySelect */




/*****************************************************************************/
/** fm10000SetBankEnable
 * \ingroup intStats
 *
 * \desc            Sets the RX/TX counter bank enable
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is the physical port for which to retrieve
 *                  statistics.
 * 
 * \param[in]       rxBankEnable is a bit mask for each RX bank that should be
 *                  enabled.
 * 
 * \param[in]       rxBankEnableUpdate is a bit mask for each RX bank that
 *                  should be updated.
 * 
 * \param[in]       txBankEnable is a bit mask for each TX bank that should
 *                  be enabled.
 * 
 * \param[in]       txBankEnableUpdate is a bit mask for each TX bank that
 *                  should be updated. 
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status fm10000SetBankEnable(fm_int    sw,
                                      fm_int    physPort,
                                      fm_uint32 rxBankEnable,
                                      fm_uint32 rxBankEnableUpdate,
                                      fm_uint32 txBankEnable,
                                      fm_uint32 txBankEnableUpdate)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_uint32  rxStatCfg;
    fm_uint64  txStatCfg;
    fm_uint32  rxEn;
    fm_bool    txEnGroup7;
    fm_bool    txEnGroup8;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, 
                 "sw=%d physPort=%d, "
                 "rxBankEnable=0x%08x, rxBankEnableUpdate=0x%08x, "
                 "txBankEnable=0x%08x, txBankEnableUpdate=0x%08x\n", 
                 sw, 
                 physPort,
                 rxBankEnable,
                 rxBankEnableUpdate,
                 txBankEnable,
                 txBankEnableUpdate);
    
    switchPtr = GET_SWITCH_PTR(sw);

    rxEn       = 0;       
    txEnGroup7 = 0; 
    txEnGroup8 = 0; 

    rxBankEnable        &= FM10000_RX_STAT_BANK_EN_ALL;
    rxBankEnableUpdate  &= FM10000_RX_STAT_BANK_EN_ALL;
    txBankEnable        &= FM10000_TX_STAT_BANK_EN_ALL;
    txBankEnableUpdate  &= FM10000_TX_STAT_BANK_EN_ALL;

    TAKE_REG_LOCK(sw)

    /* Retrieve existing values */
    err = switchPtr->ReadUINT32(sw, FM10000_RX_STATS_CFG(physPort), &rxStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = switchPtr->ReadUINT64(sw, FM10000_MOD_STATS_CFG(physPort, 0), &txStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    rxEn = FM_GET_FIELD(rxStatCfg, FM10000_RX_STATS_CFG, EnableMask);
    txEnGroup7 = FM_GET_BIT64(txStatCfg, FM10000_MOD_STATS_CFG, EnableGroup7);
    txEnGroup8 = FM_GET_BIT64(txStatCfg, FM10000_MOD_STATS_CFG, EnableGroup8);

    /* Update each enable mask */
    rxEn &= ~rxBankEnableUpdate;
    rxEn |=  rxBankEnable & rxBankEnableUpdate;

    if (txBankEnableUpdate & FM10000_TX_STAT_BANK_EN_TYPE)
    {
        txEnGroup7 = (txBankEnable & FM10000_TX_STAT_BANK_EN_TYPE) ? 1 : 0;
    }

    if (txBankEnableUpdate & FM10000_TX_STAT_BANK_EN_SIZE)
    {
        txEnGroup8 = (txBankEnable & FM10000_TX_STAT_BANK_EN_SIZE) ? 1 : 0;
    }
    
    /* Apply */
    FM_SET_FIELD(rxStatCfg, FM10000_RX_STATS_CFG, EnableMask, rxEn);
    FM_SET_BIT64(txStatCfg, FM10000_MOD_STATS_CFG, EnableGroup7, txEnGroup7);
    FM_SET_BIT64(txStatCfg, FM10000_MOD_STATS_CFG, EnableGroup8, txEnGroup8);

    err = switchPtr->WriteUINT32(sw, FM10000_RX_STATS_CFG(physPort), rxStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = switchPtr->WriteUINT64(sw, FM10000_MOD_STATS_CFG(physPort, 0), txStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    DROP_REG_LOCK(sw)

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* End fm10000SetBankEnable */


 /*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/** fm10000SetStatsFrameAdjustment
 * \ingroup intStats
 *
 * \desc            Sets the RX priority select for bank 3.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is the physical port for which to retrieve
 *                  statistics.
 * 
 * \param[in]       nbBytes is the number of bytes that should be substracted
 *                  from the received packet length for the specified port.
 * 
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
fm_status fm10000SetStatsFrameAdjustment(fm_int sw,
                                         fm_int physPort,
                                         fm_int nbBytes)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_uint32  rxStatCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, 
                 "sw=%d physPort=%d, nbBytes=%d\n", 
                 sw, 
                 physPort,
                 nbBytes);
    
    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw)

    /* Retrieve existing values */
    err = switchPtr->ReadUINT32(sw, FM10000_RX_STATS_CFG(physPort), &rxStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Apply */
    FM_SET_FIELD(rxStatCfg, FM10000_RX_STATS_CFG, PerFrameAdjustment, nbBytes);

    err = switchPtr->WriteUINT32(sw, FM10000_RX_STATS_CFG(physPort), rxStatCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    DROP_REG_LOCK(sw)

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* End fm10000SetStatsFrameAdjustment */


/*****************************************************************************/
/** fm10000GetPortCounters
 * \ingroup intStats
 *
 * \desc            Retrieve port statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port for which to retrieve statistics.
 *
 * \param[out]      counters is a pointer to an fm_portCounters structure to be
 *                  filled in by this function.
 *                  If the requested port is parsing L3 headers then version
 *                  will be FM10000_STATS_VERSION | FM_VALID_IP_STATS_VERSION.
 *                  Otherwise the version will be FM10000_STATS_VERSION.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if counters is NULL.
 *****************************************************************************/
fm_status fm10000GetPortCounters(fm_int           sw,
                                 fm_int           port,
                                 fm_portCounters *counters)
{
    fm_status                 err = FM_OK;
    fm_switch *               switchPtr;
    fm10000_switch *          switchExt;
    fm_int                    physPort;
    fm_int                    epl;
    fm_int                    lane;
    fm_bool                   hasEpl;
    fm_uint32                 parserCfg     = 0;
    fm_bool                   validIpStats = FALSE;
    fm_uint32                 i;
    fm_port *                 entry;
    fm_scatterGatherListEntry sgList[MAX_STATS_SGLIST];
    fm_int                    sgListCnt    = 0;
    fm_timestamp              ts;
    fm_bool                   stateLockTaken = FALSE;

    /* Temporary bin array to retrieve 128bit port counters (frame + bytes). */
    fm_uint32 cntRxPortStatsBank[FM10000_NB_RX_STATS_BANKS]
                                [FM10000_BINS_PER_RX_STATS_BANK]
                                [FM10000_WORDS_PER_RX_STATS_COUNTER];
    
    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "sw=%d port=%d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    
    entry     = GET_PORT_PTR(sw, port);
    physPort  = entry->physicalPort;

    err = fm10000MapPhysicalPortToEplLane(sw, physPort, &epl, &lane);
    hasEpl = (err == FM_OK) ? TRUE : FALSE;

    /* Some ports do not have an EPL */
    if (err == FM_ERR_INVALID_PORT)
    {
        err = FM_OK;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    
    /* Fill the counters structure with 0 to assure all fields not explicitly
     * set below, which will be the fields not supported by the FM10000,
     * will be 0 on return. */
    FM_MEMSET_S( (void *) counters, sizeof(*counters), 0, sizeof(*counters) );

    /**************************************************
     * Setting the counters version
     **************************************************/
    counters->cntVersion = FM10000_STATS_VERSION;

    /* Determine if L3 headers are being parsed and set the valid IP
     * stats version bit if so. */
    err = fm10000GetPortAttribute(sw,
                                  port,
                                  FM_PORT_ACTIVE_MAC,
                                  FM_PORT_LANE_NA,
                                  FM_PORT_PARSER,
                                  (void *) &parserCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    if (parserCfg >= FM_PORT_PARSER_STOP_AFTER_L3)
    {
        counters->cntVersion |= (fm_uint64)FM_VALID_IP_STATS_VERSION;
        validIpStats = TRUE;
    }

    /**************************************************
     * Reading counters for each RX bank
     *  
     * Because the RX_STATS_BANK register contains 
     * both, frame count and byte count in a 128bit
     * register, we first store the data in a temporary 
     * array. 
     *  
     * 1. Fill Scatter Gather to retrieve each bin 
     *    counter from the rx bank and store in a temp
     *    array.
     * 
     **************************************************/
    for (i = 0; i < FM_NENTRIES(rxPortCntMapTable); i++)
    {
        FM10000_GET_RX_PORT_STAT_128(rxPortCntMapTable[i].bank, 
                                     rxPortCntMapTable[i].bin);
    }

    /***************************************************
     * 2. Fill in the scatter gather for tx bank 
     *    counters. They will directly be stored
     *    in the fm_portCounters structure upon
     *    scatter gather read.
     **************************************************/
    for (i = 0; i < FM_NENTRIES(txPortCntMapTable); i++)
    {
        FM10000_GET_TX_PORT_STAT_64(txPortCntMapTable[i].bank,
                                    txPortCntMapTable[i].bin,
                                    txPortCntMapTable[i].frameOffset,
                                    txPortCntMapTable[i].byteOffset);
    }

    /***************************************************
     * 3. Fill in the scatter gather for TX CM DROP 
     *    and for EPL counters. They will directly be
     *    stored in the fm_portCounters structure upon
     *    scatter gather read.
     **************************************************/
    /* Read the TX CM drop counter CM_APPLY_DROP_COUNT*/
    sgList[sgListCnt].addr = FM10000_CM_APPLY_DROP_COUNT(physPort, 0);
    sgList[sgListCnt].data = (fm_uint32*) &counters->cntTxCMDropPkts;
    sgList[sgListCnt].count = 2;
    sgListCnt++;

    if (hasEpl)
    {
        /* EPL Counters */
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_OVERSIZE_COUNTER,   cntRxOversizedPkts);
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_JABBER_COUNTER,     cntRxJabberPkts);
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_UNDERSIZE_COUNTER,  cntRxUndersizedPkts);
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_RUNT_COUNTER,       cntRxFragmentPkts);
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_OVERRUN_COUNTER,    cntOverrunPkts);
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_UNDERRUN_COUNTER,   cntUnderrunPkts);
        FM10000_GET_EPL_PORT_STAT_32(FM10000_MAC_CODE_ERROR_COUNTER, cntCodeErrors);
    }
    
    /***************************************************
     * 4. Execute scatter gather read.
     **************************************************/
    if (sgListCnt >= MAX_STATS_SGLIST)
    {
        /* Pretty static. Mainly to warn if something new added, 
         * but the array size is not adjust accordingly */
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "fm4000GetPortCounters: Scatter list array %d overflow.\n", 
                     sgListCnt);
    }

    /* Taking lock to protect temporary structures used to
     * store 128b counters */
    FM_FLAG_TAKE_STATE_LOCK(sw);

    /* now get the stats in one shot, optimized for fibm */
    err = fmReadScatterGather(sw, sgListCnt, sgList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fmGetTime(&ts);
    counters->timestamp = ts.sec * 1000000 + ts.usec;   
    
    /***************************************************
     * 5. Retrieve the frame/byte counts from 128bit 
     *    registers stored in temporary array and set
     *    the proper fm_portCounter structure members.
     **************************************************/

    for (i = 0; i < FM_NENTRIES(rxPortCntMapTable); i++)
    {
        FM10000_UPD_RX_PORT_STAT_128(rxPortCntMapTable[i].bank,
                                     rxPortCntMapTable[i].bin,
                                     rxPortCntMapTable[i].frameOffset,
                                     rxPortCntMapTable[i].byteOffset);
    }
    
    FM_FLAG_DROP_STATE_LOCK(sw);

    /***************************************************
     * 6. Set some counters that are not available 
     *    in HW but can be computed from two or more
     *    HW counters.
     **************************************************/

    /* If IP parsing is enabled then the IP stats will be read and will be
     * valid. Else all traffic, whether IP or not, is counted as NonIP. */
    if (validIpStats)
    {
        /***************************************************
         * RX counters.
         **************************************************/
        counters->cntRxUcstPkts = counters->cntRxUcstPktsNonIP +
                                  counters->cntRxUcstPktsIPv4 +
                                  counters->cntRxUcstPktsIPv6;

        counters->cntRxMcstPkts = counters->cntRxMcstPktsNonIP +
                                  counters->cntRxMcstPktsIPv4 +
                                  counters->cntRxMcstPktsIPv6;

        counters->cntRxBcstPkts = counters->cntRxBcstPktsNonIP +
                                  counters->cntRxBcstPktsIPv4 +
                                  counters->cntRxBcstPktsIPv6;

        /***************************************************
         * Misc. counters.
         **************************************************/
        counters->cntRxOctetsNonIp  = counters->cntRxUcstOctetsNonIP +
                                      counters->cntRxMcstOctetsNonIP +
                                      counters->cntRxBcstOctetsNonIP;

        counters->cntRxOctetsIPv4  = counters->cntRxUcstOctetsIPv4 +
                                     counters->cntRxMcstOctetsIPv4 +
                                     counters->cntRxBcstOctetsIPv4;

        counters->cntRxOctetsIPv6  = counters->cntRxUcstOctetsIPv6 +
                                     counters->cntRxMcstOctetsIPv6 +
                                     counters->cntRxBcstOctetsIPv6;

        counters->cntRxGoodOctets = counters->cntRxOctetsNonIp +
                                    counters->cntRxOctetsIPv4 +
                                    counters->cntRxOctetsIPv6;
    }
    else
    {
        /***************************************************
         * RX counters.
         **************************************************/
        counters->cntRxUcstPkts     = counters->cntRxUcstPktsNonIP;
        counters->cntRxMcstPkts     = counters->cntRxMcstPktsNonIP;
        counters->cntRxBcstPkts     = counters->cntRxBcstPktsNonIP;

        /***************************************************
         * Misc. counters.
         **************************************************/
        counters->cntRxOctetsNonIp  = counters->cntRxUcstOctetsNonIP +
                                      counters->cntRxMcstOctetsNonIP +
                                      counters->cntRxBcstOctetsNonIP;

        counters->cntRxGoodOctets = counters->cntRxOctetsNonIp;
    }

    counters->cntTriggerDropRedirPkts = counters->cntTriggerRedirPkts +
                                        counters->cntTriggerDropPkts;

    /* Emulate Tx Octets counter using the sum of all size bins. */
    counters->cntTxOctets += counters->cntTxMinTo63octets;
    counters->cntTxOctets += counters->cntTx64octets;
    counters->cntTxOctets += counters->cntTx65to127octets;
    counters->cntTxOctets += counters->cntTx128to255octets;
    counters->cntTxOctets += counters->cntTx256to511octets;
    counters->cntTxOctets += counters->cntTx512to1023octets;
    counters->cntTxOctets += counters->cntTx1024to1522octets;
    counters->cntTxOctets += counters->cntTx1523to2047octets;
    counters->cntTxOctets += counters->cntTx2048to4095octets;
    counters->cntTxOctets += counters->cntTx4096to8191octets;
    counters->cntTxOctets += counters->cntTx8192to10239octets;
    counters->cntTxOctets += counters->cntTx10240toMaxOctets;

ABORT:
    if (stateLockTaken)
    {
        FM_FLAG_DROP_STATE_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000GetPortCounters */




/*****************************************************************************/
/** fm10000ResetPortCounters
 * \ingroup intStats
 *
 * \desc            Reset port statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port for which to reset statistics.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * 
 *****************************************************************************/
fm_status fm10000ResetPortCounters(fm_int sw,
                                   fm_int port)
{
    fm_status                 err = FM_OK;
    fm_status                 err2;
    fm_switch *               switchPtr;
    fm_port *                 entry;
    fm_uint64                 resetValue64 = 0;
    fm_uint32                 resetValue128[4] = { 0, 0, 0, 0 };
    fm_int                    physPort;
    fm_int                    epl;
    fm_int                    lane;
    fm_bool                   hasEpl;
    fm_bool                   stateLockTaken = FALSE;
    fm_uint32                 i;
    fm_scatterGatherListEntry sgList[MAX_STATS_SGLIST];
    fm_int                    sgListCnt    = 0;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "sw=%d port=%d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);
    
    entry     = switchPtr->portTable[port];
    physPort  = entry->physicalPort;

    err = fm10000MapPhysicalPortToEplLane(sw, physPort, &epl, &lane);
    hasEpl = (err == FM_OK) ? TRUE : FALSE;

    /* Some ports do not have an EPL */
    if (err == FM_ERR_INVALID_PORT)
    {
        err = FM_OK;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /************************************************ 
     * 1. Add all counters to be reset to the scatter
     *    gather list
     ***********************************************/

    /* Reset all RX counters for the given port */
    for (i = 0; i < FM_NENTRIES(rxPortCntMapTable); i++)
    {
        sgList[sgListCnt].addr = 
            FM10000_RX_STATS_BANK(rxPortCntMapTable[i].bank,
                                  (physPort << 4 | rxPortCntMapTable[i].bin),
                                  0);
        sgList[sgListCnt].count = 4;
        sgList[sgListCnt].data = resetValue128;
        sgListCnt++;
    }

    /* Reset all TX counters for the given port */
    for (i = 0; i < FM_NENTRIES(txPortCntMapTable); i++)
    {
        sgList[sgListCnt].addr = 
            FM10000_MOD_STATS_BANK_FRAME(txPortCntMapTable[i].bank,
                                  (physPort << 4 | txPortCntMapTable[i].bin),
                                  0);
        sgList[sgListCnt].count = 2;
        sgList[sgListCnt].data = (fm_uint32 *)&resetValue64;
        sgListCnt++;

        sgList[sgListCnt].addr = 
            FM10000_MOD_STATS_BANK_BYTE(txPortCntMapTable[i].bank,
                                  (physPort << 4 | txPortCntMapTable[i].bin),
                                  0);
        sgList[sgListCnt].count = 2;
        sgList[sgListCnt].data = (fm_uint32 *)&resetValue64;
        sgListCnt++;
    }

    if (sgListCnt >= MAX_STATS_SGLIST)
    {
        /* Pretty static. Mainly to warn if something new added, 
         * but the array size is not adjust accordingly */
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Scatter list array %d overflow.\n", 
                     sgListCnt);
    }

    /* Reset the TX CM drop counter CM_APPLY_DROP_COUNT*/
    sgList[sgListCnt].addr = FM10000_CM_APPLY_DROP_COUNT(physPort, 0);
    sgList[sgListCnt].data = (fm_uint32 *)&resetValue64;
    sgList[sgListCnt].count = 2;
    sgListCnt++;

    if (hasEpl)
    {
        /* Reset EPL Counters */
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_OVERSIZE_COUNTER,   &resetValue128[0]);
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_JABBER_COUNTER,     &resetValue128[0]);
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_UNDERSIZE_COUNTER,  &resetValue128[0]);
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_RUNT_COUNTER,       &resetValue128[0]);
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_OVERRUN_COUNTER,    &resetValue128[0]);
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_UNDERRUN_COUNTER,   &resetValue128[0]);
        FM10000_SET_EPL_PORT_STAT_32(FM10000_MAC_CODE_ERROR_COUNTER, &resetValue128[0]);
    }

    /************************************************ 
     * 2. Take the state lock during counter reset 
     *    to prevent reading the counters during
     *    this process. Execute Scatter Gather Write
     ***********************************************/

    FM_FLAG_TAKE_STATE_LOCK(sw);

    /* For atomic reset, we disable the banks before reseting */
    err = fm10000SetBankEnable(sw, 
                               physPort, 
                               ~FM10000_RX_STAT_BANK_EN_ALL_PORT,
                               FM10000_RX_STAT_BANK_EN_ALL_PORT,
                               ~FM10000_TX_STAT_BANK_EN_ALL,
                               FM10000_TX_STAT_BANK_EN_ALL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fmWriteScatterGather(sw, sgListCnt, sgList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    if (stateLockTaken == TRUE)
    {
        /* We must re-enable banks */
        err2 = fm10000SetBankEnable(sw, 
                                    physPort, 
                                    FM10000_RX_STAT_BANK_EN_ALL_PORT,
                                    FM10000_RX_STAT_BANK_EN_ALL_PORT,
                                    FM10000_TX_STAT_BANK_EN_ALL,
                                    FM10000_TX_STAT_BANK_EN_ALL);
        if (err2 != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PORT, 
                         "Could not set port statistic bank enable\n");

            /* err2 is the first error, make sure we return it */
            if (err == FM_OK)
            {
                err = err2;
            }
        }

        FM_FLAG_DROP_STATE_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000ResetPortCounters */




/*****************************************************************************/
/** fm10000GetVLANCounters
 * \ingroup intStats
 *
 * \desc            Retrieves the values of the specified VLAN counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vcid is the ID of the VLAN counter set to retrieve.
 *
 * \param[out]      counters is a pointer to an fm_vlanCounters structure to be
 *                  filled in by this function.
 *                  The version will be 1.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VCID if vcid is not valid.
 * \return          FM_ERR_INVALID_ARGUMENT if counters is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetVLANCounters(fm_int          sw,
                                fm_int           vcid,
                                fm_vlanCounters *counters)
{
    fm_status       err = FM_FAIL;
    fm_switch *     switchPtr = NULL;
    fm_bool         stateLockTaken = FALSE;
    fm_uint32       tmpUcstCnt128[4];
    fm_uint32       tmpMcstCnt128[4];
    fm_uint32       tmpBcstCnt128[4];

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vcid=%d counters=%p\n",
                 sw,
                 vcid,
                 (void *) counters);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (vcid < 0) || (vcid > FM10000_MAX_VLAN_COUNTER) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_VCID);
    }

    FM_MEMSET_S(counters, sizeof(fm_vlanCounters), 0, sizeof(fm_vlanCounters));

    counters->cntVersion = FM10000_STATS_VERSION;

    FM_FLAG_TAKE_STATE_LOCK(sw);

    err = switchPtr->ReadUINT32Mult(sw, 
                                    FM10000_RX_STATS_BANK(
                                         FM10000_RX_STAT_BANK_VLAN,
                                         (FM10000_RX_STAT_VLAN_UCAST << 6 | vcid),
                                         0),
                                    4,
                                    tmpUcstCnt128);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    err = switchPtr->ReadUINT32Mult(sw, 
                                    FM10000_RX_STATS_BANK(
                                         FM10000_RX_STAT_BANK_VLAN,
                                         (FM10000_RX_STAT_VLAN_MCAST << 6 | vcid),
                                         0),
                                    4,
                                    tmpMcstCnt128);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    err = switchPtr->ReadUINT32Mult(sw, 
                                    FM10000_RX_STATS_BANK(
                                         FM10000_RX_STAT_BANK_VLAN,
                                         (FM10000_RX_STAT_VLAN_BCAST << 6 | vcid),
                                         0),
                                    4,
                                    tmpBcstCnt128);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    counters->cntRxUcstPkts   = (((fm_uint64)(tmpUcstCnt128[1])) << 32) |
                                ((fm_uint64)  tmpUcstCnt128[0]);
    counters->cntRxUcstOctets = (((fm_uint64)(tmpUcstCnt128[3])) << 32) |
                                ((fm_uint64)  tmpUcstCnt128[2]);
    counters->cntRxMcstPkts   = (((fm_uint64)(tmpMcstCnt128[1])) << 32) |
                                ((fm_uint64)  tmpMcstCnt128[0]);
    counters->cntRxMcstOctets = (((fm_uint64)(tmpMcstCnt128[3])) << 32) |
                                ((fm_uint64)  tmpMcstCnt128[2]);
    counters->cntRxBcstPkts   = (((fm_uint64)(tmpBcstCnt128[1])) << 32) |
                                ((fm_uint64)  tmpBcstCnt128[0]);
    counters->cntRxBcstOctets = (((fm_uint64)(tmpBcstCnt128[3])) << 32) |
                                ((fm_uint64)  tmpBcstCnt128[2]);


ABORT:
    if (stateLockTaken == TRUE)
    {
        FM_FLAG_DROP_STATE_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fm10000GetVLANCounters */




/*****************************************************************************/
/** fm10000ResetVLANCounters
 * \ingroup intStats
 *
 * \desc            Reset the values of the specified VLAN counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vcid is the ID of the VLAN counter set to retrieve.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VCID if vcid is not valid.
 * \return          FM_ERR_INVALID_ARGUMENT if counters is NULL.
 *
 *****************************************************************************/
fm_status fm10000ResetVLANCounters(fm_int sw,
                                   fm_int vcid)
{
    fm_status       err = FM_FAIL;
    fm_switch *     switchPtr = NULL;
    fm_bool         stateLockTaken = FALSE;
    fm_uint32       zeros[4] = { 0, 0, 0, 0};
    
    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vcid=%d\n",
                 sw,
                 vcid);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (vcid < 0) || (vcid > FM10000_MAX_VLAN_COUNTER) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_VCID);
    }

    FM_FLAG_TAKE_STATE_LOCK(sw);

    err = switchPtr->WriteUINT32Mult(sw, 
                                     FM10000_RX_STATS_BANK(
                                         FM10000_RX_STAT_BANK_VLAN,
                                         (FM10000_RX_STAT_VLAN_UCAST << 6 | vcid),
                                         0),
                                     4,
                                     zeros);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    err = switchPtr->WriteUINT32Mult(sw, 
                                     FM10000_RX_STATS_BANK(
                                         FM10000_RX_STAT_BANK_VLAN,
                                         (FM10000_RX_STAT_VLAN_MCAST << 6 | vcid),
                                         0),
                                     4,
                                     zeros);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    err = switchPtr->WriteUINT32Mult(sw, 
                                     FM10000_RX_STATS_BANK(
                                         FM10000_RX_STAT_BANK_VLAN,
                                         (FM10000_RX_STAT_VLAN_BCAST << 6 | vcid),
                                         0),
                                     4,
                                     zeros);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

ABORT:
    if (stateLockTaken == TRUE)
    {
        FM_FLAG_DROP_STATE_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fm10000ResetVLANCounters */




/*****************************************************************************/
/** fm10000InitCounters
 * \ingroup intStats
 *
 * \desc            Initialize statistic counters
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitCounters(fm_int sw)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_int      logPort;
    fm_int      physPort;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* Enable each rx/tx counter banks on all ports */
        err = fm10000SetBankEnable(sw, 
                                   physPort, 
                                   FM10000_RX_STAT_BANK_EN_ALL,
                                   FM10000_RX_STAT_BANK_EN_ALL,
                                   FM10000_TX_STAT_BANK_EN_ALL,
                                   FM10000_TX_STAT_BANK_EN_ALL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* Set the priority source as SWITCH_PRI */
        err = fm10000SetPrioritySelect(sw, physPort, FM10000_PRI_SEL_SWITCH_PRI);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitCounters */




/*****************************************************************************/
/** fm10000GetCountersInitMode
 * \ingroup intStats
 *
 * \desc            Get the initialization mode
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      mode is used to return the type of initialization that
 *                  should be done.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetCountersInitMode(fm_int sw, fm_uint32 *mode)
{
    fm_status   err = FM_OK;
    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    *mode = FM_STAT_VLAN_ASSIGNMENT_INIT_GENERIC;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GetCountersInitMode */
