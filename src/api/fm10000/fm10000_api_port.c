/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_port.c
 * Creation Date:   April 17, 2013 from the FM4000 version of this file.
 * Description:     Contains functions dealing with the state of individual
 *                  ports, and attributes thereof.
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define BIST_COUNTER_START_DELAY                250000000           /* nanosecs */
#define BIST_PATTERN_IDLECHAR                   0x9F27C9F27C
#define BIST_PATTERN_TESTCHAR                   0x1F07C1F07C
#define BIST_PATTERN_LOWFREQ                    0x1F07C1F07C
#define BIST_PATTERN_HIGHFREQ                   0x9565595655
#define BIST_PATTERN_SQUARE8_0                  0x00FF00FF00
#define BIST_PATTERN_SQUARE8_1                  0xFF00FF00FF
#define BIST_PATTERN_SQUARE10                   0xFFC00FFC00

#define FM10000_MIN_TRUNC_LEN       64
#define FM10000_MAX_TRUNC_LEN       192
#define F56_NB_BYTES                8

#define FM10000_PORT_EEE_TX_ACT_MAX_VAL         2550
#define FM10000_PORT_EEE_TX_LPI_MAX_VAL         255

/* attribute filters */
#define EXCLUDE_PCIE_RD    (1U << 0)
#define EXCLUDE_PCIE_WR    (1U << 1)
#define EXCLUDE_PCIE       ( EXCLUDE_PCIE_RD | EXCLUDE_PCIE_WR )
#define EXCLUDE_ETH_RD     (1U << 2)
#define EXCLUDE_ETH_WR     (1U << 3)
#define EXCLUDE_ETH        ( EXCLUDE_ETH_RD  | EXCLUDE_ETH_WR  )
#define EXCLUDE_WR         (0xAAAAAAAA)
#define READ_ONLY          (EXCLUDE_WR)
#define EXCLUDE_RD         (0x55555555)
#define WRITE_ONLY         (EXCLUDE_RD)
#define EXCLUDE_TE_RD      (1U << 4)
#define EXCLUDE_TE_WR      (1U << 5)
#define EXCLUDE_TE         ( EXCLUDE_TE_RD | EXCLUDE_TE_WR )
#define EXCLUDE_LPBK_RD    (1U << 6)
#define EXCLUDE_LPBK_WR    (1U << 7)
#define EXCLUDE_LPBK       ( EXCLUDE_LPBK_RD | EXCLUDE_LPBK_WR )

/***************************************************
 * In per-Lag management mode, per-lag attributes
 * cannot be set if the port is member of a lag.
 ***************************************************/

#define VALIDATE_PORT_ATTRIBUTE(attr)                                        \
    if (switchPtr->perLagMgmt && (attr)->perLag && !portExt->allowCfg &&     \
        FM_IS_PORT_IN_A_LAG(portPtr))                                        \
    {                                                                        \
        err = FM_ERR_PER_LAG_ATTRIBUTE;                                      \
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);                  \
    }


/**************************************************
 * Macros to validate read/write access rights for 
 * an attribute
 **************************************************/

/* macros to check write access */
#define IS_ATTRIBUTE_WRITABLE(attr)                                          \
    (( portExt->ring != FM10000_SERDES_RING_EPL  ||                          \
     ((attr)->excludedPhyPortTypes & EXCLUDE_ETH_WR)  == 0  ) &&             \
     ( portExt->ring != FM10000_SERDES_RING_PCIE ||                          \
     ((attr)->excludedPhyPortTypes & EXCLUDE_PCIE_WR) == 0  ) &&             \
     ( portPtr->portType != FM_PORT_TYPE_LOOPBACK ||                         \
     ((attr)->excludedPhyPortTypes & EXCLUDE_LPBK_WR)  == 0 ) &&             \
     ( portPtr->portType != FM_PORT_TYPE_TE ||                               \
     ((attr)->excludedPhyPortTypes & EXCLUDE_TE_WR)  == 0 ) )                


#define VALIDATE_ATTRIBUTE_WRITE_ACCESS(attr)                                \
    if( !IS_ATTRIBUTE_WRITABLE(attr) )                                       \
    {                                                                        \
        err = FM_ERR_UNSUPPORTED;                                            \
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);                  \
    } 



/* macros to check read access */
#define IS_ATTRIBUTE_READABLE(attr)                                          \
    (( portExt->ring != FM10000_SERDES_RING_EPL  ||                          \
     ((attr)->excludedPhyPortTypes & EXCLUDE_ETH_RD)  == 0  ) &&             \
     ( portExt->ring != FM10000_SERDES_RING_PCIE ||                          \
     ((attr)->excludedPhyPortTypes & EXCLUDE_PCIE_RD) == 0  ) &&             \
     ( portPtr->portType != FM_PORT_TYPE_LOOPBACK ||                         \
     ((attr)->excludedPhyPortTypes & EXCLUDE_LPBK_RD)  == 0 ) &&             \
     ( portPtr->portType != FM_PORT_TYPE_TE ||                               \
     ((attr)->excludedPhyPortTypes & EXCLUDE_TE_RD) == 0 ) )                 

#define IS_ATTRIBUTE_APPLICABLE(attr) IS_ATTRIBUTE_READABLE(attr)

#define VALIDATE_ATTRIBUTE_READ_ACCESS(attr)                                 \
    if( !IS_ATTRIBUTE_READABLE(attr) )                                       \
    {                                                                        \
        err = FM_ERR_UNSUPPORTED;                                            \
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);                  \
    }

#define VALIDATE_VALUE_IS_BOOL(value)                                          \
    if (!(*((fm_bool *) (value)) == FM_ENABLED ||                              \
        *((fm_bool *) (value)) == FM_DISABLED))                                \
    {                                                                          \
        err = FM_ERR_INVALID_VALUE;                                            \
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);                    \
    }

#define DUMP_FORMAT "%15s %-5s %-7s %-29s %-36s %-28s %s\n"

typedef struct
{
    fm_uint64     timestamp;
    fm_text       type;
    fm_text       currentState;
    fm_text       nextState;
    fm_text       event;
    fm_status     status;
    fm_int        port;
    fm_byte       lane;
    fm_byte       serdes;

} fm10000_portStateTDump;



/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


static fm10000_portAttrEntryTable portAttributeTable =
{
    .autoNegMode  =
    {
        .attr                 = FM_PORT_AUTONEG,
        .str                  = "FM_PORT_AUTONEG",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoNegMode),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegBasePage  =
    {
        .attr                 = FM_PORT_AUTONEG_BASEPAGE,
        .str                  = "FM_PORT_AUTONEG_BASEPAGE",
        .type                 = FM_TYPE_UINT64,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoNegBasePage),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegPartnerBasePage  =
    {
        .attr                 = FM_PORT_AUTONEG_PARTNER_BASEPAGE,
        .str                  = "FM_PORT_AUTONEG_PARTNER_BASEPAGE",
        .type                 = FM_TYPE_UINT64,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoNegPartnerBasePage),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegNextPages  =
    {
        .attr                 = FM_PORT_AUTONEG_NEXTPAGES,
        .str                  = "FM_PORT_AUTONEG_NEXTPAGES",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoNegNextPages),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegPartnerNextPages  =
    {
        .attr                 = FM_PORT_AUTONEG_PARTNER_NEXTPAGES,
        .str                  = "FM_PORT_AUTONEG_PARTNER_NEXTPAGES",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoNegPartnerNextPages),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNeg25GNxtPgOui =
    {
        .attr                 = FM_PORT_AUTONEG_25G_NEXTPAGE_OUI,
        .str                  = "FM_PORT_AUTONEG_25G_NEXTPAGE_OUI",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoNeg25GNxtPgOui),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .bcastFlooding =
    {
        .attr                 = FM_PORT_BCAST_FLOODING,
        .str                  = "FM_PORT_BCAST_FLOODING",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, bcastFlooding),
        .excludedPhyPortTypes = 0,
        .defValue             = FM_DISABLED,
    },

    .bcastPruning =
    {
        .attr                 = FM_PORT_BCAST_PRUNING,
        .str                  = "FM_PORT_BCAST_PRUNING",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, bcastPruning),
        .excludedPhyPortTypes = 0,
        .defValue             = FM_DISABLED,
    },

    .enableTxCutThrough  =
    {
        .attr                 = FM_PORT_TX_CUT_THROUGH,
        .str                  = "FM_PORT_TX_CUT_THROUGH",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, enableTxCutThrough),
        .excludedPhyPortTypes = 0,
    },

    .enableRxCutThrough  =
    {
        .attr                 = FM_PORT_RX_CUT_THROUGH,
        .str                  = "FM_PORT_RX_CUT_THROUGH",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, enableRxCutThrough),
        .excludedPhyPortTypes = 0,
    },

    .defCfi  =
    {
        .attr                 = FM_PORT_DEF_CFI,
        .str                  = "FM_PORT_DEF_CFI",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defCfi),
        .excludedPhyPortTypes = 0,
    },

    .defDscp  =
    {
        .attr                 = FM_PORT_DEF_DSCP,
        .str                  = "FM_PORT_DEF_DSCP",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defDscp),
        .excludedPhyPortTypes = 0,
    },

    .defVlanPri  =
    {
        .attr                 = FM_PORT_DEF_PRI,
        .str                  = "FM_PORT_DEF_PRI",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defVlanPri),
        .excludedPhyPortTypes = 0,
    },

    .defVlanPri2  =
    {
        .attr                 = FM_PORT_DEF_PRI2,
        .str                  = "FM_PORT_DEF_PRI2",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defVlanPri2),
        .excludedPhyPortTypes = 0,
    },

    .defSwpri  =
    {
        .attr                 = FM_PORT_DEF_SWPRI,
        .str                  = "FM_PORT_DEF_SWPRI",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defSwpri),
        .excludedPhyPortTypes = 0,
    },

    .defIslUser  =
    {
        .attr                 = FM_PORT_DEF_ISL_USER,
        .str                  = "FM_PORT_DEF_ISL_USER",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defIslUser),
        .excludedPhyPortTypes = 0,
    },

    .defVlan  =
    {
        .attr                 = FM_PORT_DEF_VLAN,
        .str                  = "FM_PORT_DEF_VLAN",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defVlan),
        .excludedPhyPortTypes = 0,
    },

    .defVlan2  =
    {
        .attr                 = FM_PORT_DEF_VLAN2,
        .str                  = "FM_PORT_DEF_VLAN2",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, defVlan2),
        .excludedPhyPortTypes = 0,
    },

    .dot1xState  =
    {
        .attr                 = FM_PORT_DOT1X_STATE,
        .str                  = "FM_PORT_DOT1X_STATE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, dot1xState),
        .excludedPhyPortTypes = 0,
    },

    .dropBv  =
    {
        .attr                 = FM_PORT_DROP_BV,
        .str                  = "FM_PORT_DROP_BV",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, dropBv),
        .excludedPhyPortTypes = 0,
    },

    .dropTagged  =
    {
        .attr                 = FM_PORT_DROP_TAGGED,
        .str                  = "FM_PORT_DROP_TAGGED",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, dropTagged),
        .excludedPhyPortTypes = 0,
    },

    .dropUntagged  =
    {
        .attr                 = FM_PORT_DROP_UNTAGGED,
        .str                  = "FM_PORT_DROP_UNTAGGED",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, dropUntagged),
        .excludedPhyPortTypes = 0,
    },

    .internal  =
    {
        .attr                 = FM_PORT_INTERNAL,
        .str                  = "FM_PORT_INTERNAL",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, internalPort),
        .excludedPhyPortTypes = 0,
    },

    .islTagFormat  =
    {
        .attr                 = FM_PORT_ISL_TAG_FORMAT,
        .str                  = "FM_PORT_ISL_TAG_FORMAT",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, islTagFormat),
        .excludedPhyPortTypes = 0,
    },

    .learning  =
    {
        .attr                 = FM_PORT_LEARNING,
        .str                  = "FM_PORT_LEARNING",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, learning),
        .excludedPhyPortTypes = 0,
    },

    .linkInterruptEnabled  =
    {
        .attr                 = FM_PORT_LINK_INTERRUPT,
        .str                  = "FM_PORT_LINK_INTERRUPT",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, linkInterruptEnabled),
        .excludedPhyPortTypes = ( EXCLUDE_ETH_WR | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .serdesLoopback  =
    {
        .attr                 = FM_PORT_LOOPBACK,
        .str                  = "FM_PORT_LOOPBACK",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, serdesLoopback),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE_WR | EXCLUDE_LPBK_WR ),
    },

    .fabricLoopback  =
    {
        .attr                 = FM_PORT_FABRIC_LOOPBACK,
        .str                  = "FM_PORT_FABRIC_LOOPBACK",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm10000_portAttr, fabricLoopback),
        .excludedPhyPortTypes = ( EXCLUDE_LPBK_WR ),
    },

    .loopbackSuppression  =
    {
        .attr                 = FM_PORT_LOOPBACK_SUPPRESSION,
        .str                  = "FM_PORT_LOOPBACK_SUPPRESSION",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, loopbackSuppression),
        .excludedPhyPortTypes = 0,
    },

    .maskWide  =
    {
        .attr                 = FM_PORT_MASK_WIDE,
        .str                  = "FM_PORT_MASK_WIDE",
        .type                 = FM_TYPE_PORTMASK,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, portMask),
        .excludedPhyPortTypes = 0,
    },

    .maxFrameSize  =
    {
        .attr                 = FM_PORT_MAX_FRAME_SIZE,
        .str                  = "FM_PORT_MAX_FRAME_SIZE",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, maxFrameSize),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE_WR ),
    },

    .mcastFlooding  =
    {
        .attr                 = FM_PORT_MCAST_FLOODING,
        .str                  = "FM_PORT_MCAST_FLOODING",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, mcastFlooding),
        .excludedPhyPortTypes = 0,
    },

    .mcastPruning  =
    {
        .attr                 = FM_PORT_MCAST_PRUNING,
        .str                  = "FM_PORT_MCAST_PRUNING",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, mcastPruning),
        .excludedPhyPortTypes = 0,
    },

    .minFrameSize  =
    {
        .attr                 = FM_PORT_MIN_FRAME_SIZE,
        .str                  = "FM_PORT_MIN_FRAME_SIZE",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, minFrameSize),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE_WR ),
    },

    .parser  =
    {
        .attr                 = FM_PORT_PARSER,
        .str                  = "FM_PORT_PARSER",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, parser),
        .excludedPhyPortTypes = 0,
    },

    .parserFlagOptions  =
    {
        .attr                 = FM_PORT_PARSER_FLAG_OPTIONS,
        .str                  = "FM_PORT_PARSER_FLAG_OPTIONS",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, parserFlagOptions),
        .excludedPhyPortTypes = 0,
    },

    .replaceDscp  =
    {
        .attr                 = FM_PORT_REPLACE_DSCP,
        .str                  = "FM_PORT_REPLACE_DSCP",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, replaceDscp),
        .excludedPhyPortTypes = 0,
    },

    .routable  =
    {
        .attr                 = FM_PORT_ROUTABLE,
        .str                  = "FM_PORT_ROUTABLE",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, routable),
        .excludedPhyPortTypes = 0,
    },

    .rxClassPause  =
    {
        .attr                 = FM_PORT_RX_CLASS_PAUSE,
        .str                  = "FM_PORT_RX_CLASS_PAUSE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, rxClassPause),
        .excludedPhyPortTypes = 0,
    },

    .txClassPause  =
    {
        .attr                 = FM_PORT_TX_CLASS_PAUSE,
        .str                  = "FM_PORT_TX_CLASS_PAUSE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txClassPause),
        .excludedPhyPortTypes = 0,
    },

    .rxLanePolarity  =
    {
        .attr                 = FM_PORT_RX_LANE_POLARITY,
        .str                  = "FM_PORT_RX_LANE_POLARITY",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, rxLanePolarity),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .rxPause  =
    {
        .attr                 = FM_PORT_RX_PAUSE,
        .str                  = "FM_PORT_RX_PAUSE",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, rxPause),
        .excludedPhyPortTypes = 0,
    },

    .speed  =
    {
        .attr                 = FM_PORT_SPEED,
        .str                  = "FM_PORT_SPEED",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, speed),
        .excludedPhyPortTypes = EXCLUDE_WR,
    },

    .swpriDscpPref  =
    {
        .attr                 = FM_PORT_SWPRI_DSCP_PREF,
        .str                  = "FM_PORT_SWPRI_DSCP_PREF",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, swpriDscpPref),
        .excludedPhyPortTypes = 0,
    },

    .swpriSource  =
    {
        .attr                 = FM_PORT_SWPRI_SOURCE,
        .str                  = "FM_PORT_SWPRI_SOURCE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, swpriSource),
        .excludedPhyPortTypes = 0,
    },

    .txFcsMode  =
    {
        .attr                 = FM_PORT_TX_FCS_MODE,
        .str                  = "FM_PORT_TX_FCS_MODE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, txFcsMode),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txLanePolarity  =
    {
        .attr                 = FM_PORT_TX_LANE_POLARITY,
        .str                  = "FM_PORT_TX_LANE_POLARITY",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txLanePolarity),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txPause  =
    {
        .attr                 = FM_PORT_TX_PAUSE,
        .str                  = "FM_PORT_TX_PAUSE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txPause),
        .excludedPhyPortTypes = 0,
    },

    .txPauseMode  =
    {
        .attr                 = FM_PORT_TX_PAUSE_MODE,
        .str                  = "FM_PORT_TX_PAUSE_MODE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txPauseMode),
        .excludedPhyPortTypes = 0,
    },

    .txPauseResendTime  =
    {
        .attr                 = FM_PORT_TX_PAUSE_RESEND_TIME,
        .str                  = "FM_PORT_TX_PAUSE_RESEND_TIME",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txPauseResendTime),
        .excludedPhyPortTypes = 0,
    },

    .txCfi  =
    {
        .attr                 = FM_PORT_TXCFI,
        .str                  = "FM_PORT_TXCFI",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txCfi),
        .excludedPhyPortTypes = 0,
    },

    .txCfi2  =
    {
        .attr                 = FM_PORT_TXCFI2,
        .str                  = "FM_PORT_TXCFI2",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txCfi2),
        .excludedPhyPortTypes = 0,
    },

    .txVpri  =
    {
        .attr                 = FM_PORT_TXVPRI,
        .str                  = "FM_PORT_TXVPRI",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txVpri),
        .excludedPhyPortTypes = 0,
    },

    .txVpri2  =
    {
        .attr                 = FM_PORT_TXVPRI2,
        .str                  = "FM_PORT_TXVPRI2",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, txVpri2),
        .excludedPhyPortTypes = 0,
    },

    .ucastFlooding  =
    {
        .attr                 = FM_PORT_UCAST_FLOODING,
        .str                  = "FM_PORT_UCAST_FLOODING",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, ucastFlooding),
        .excludedPhyPortTypes = 0,
    },

    .ucastPruning =
    {
        .attr                 = FM_PORT_UCAST_PRUNING,
        .str                  = "FM_PORT_UCAST_PRUNING",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, ucastPruning),
        .excludedPhyPortTypes = 0,
        .defValue             = FM_DISABLED,
    },

    .updateDscp  =
    {
        .attr                 = FM_PORT_UPDATE_DSCP,
        .str                  = "FM_PORT_UPDATE_DSCP",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, updateDscp),
        .excludedPhyPortTypes = 0,
    },

    .updateTtl  =
    {
        .attr                 = FM_PORT_UPDATE_TTL,
        .str                  = "FM_PORT_UPDATE_TTL",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, updateTtl),
        .excludedPhyPortTypes = 0,
    },

    .ignoreIfgErrors  =
    {
        .attr                 = FM_PORT_IGNORE_IFG_ERRORS,
        .str                  = "FM_PORT_IGNORE_IFG_ERRORS",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, ignoreIfgErrors),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .smpLosslessPause  =
    {
        .attr                 = FM_PORT_SMP_LOSSLESS_PAUSE,
        .str                  = "FM_PORT_SMP_LOSSLESS_PAUSE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, smpLosslessPause),
        .excludedPhyPortTypes = 0,
    },

    /*************************************************** 
     * FM10000-specific attributes.
     ***************************************************/

    .eyeScore  =
    {
        .attr                 = FM_PORT_EYE_SCORE,
        .str                  = "FM_PORT_EYE_SCORE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, eyeScore),
        .excludedPhyPortTypes = ( EXCLUDE_ETH_WR | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegLinkInhbTimer  =
    {
        .attr                 = FM_PORT_AUTONEG_LINK_INHB_TIMER,
        .str                  = "FM_PORT_AUTONEG_LINK_INHB_TIMER",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, autoNegLinkInhbTimer),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegLinkInhbTimerKx  =
    {
        .attr                 = FM_PORT_AUTONEG_LINK_INHB_TIMER_KX,
        .str                  = "FM_PORT_AUTONEG_LINK_INHB_TIMER_KX",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, autoNegLinkInhbTimerKx),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .autoNegIgnoreNonce  =
    {
        .attr                 = FM_PORT_AUTONEG_IGNORE_NONCE,
        .str                  = "FM_PORT_AUTONEG_IGNORE_NONCE",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, autoNegIgnoreNonce),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .taggingMode  =
    {
        .attr                 = FM_PORT_TAGGING_MODE,
        .str                  = "FM_PORT_TAGGING_MODE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, taggingMode),
        .excludedPhyPortTypes = 0,
    },

    .parserVlan1Tag  =
    {
        .attr                 = FM_PORT_PARSER_VLAN1_TAG,
        .str                  = "FM_PORT_PARSER_VLAN1_TAG",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, parserVlan1Tag),
        .excludedPhyPortTypes = 0,
    },

    .parserVlan2Tag  =
    {
        .attr                 = FM_PORT_PARSER_VLAN2_TAG,
        .str                  = "FM_PORT_PARSER_VLAN2_TAG",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, parserVlan2Tag),
        .excludedPhyPortTypes = 0,
    },

    .modifyVlan1Tag  =
    {
        .attr                 = FM_PORT_MODIFY_VLAN1_TAG,
        .str                  = "FM_PORT_MODIFY_VLAN1_TAG",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, modifyVlan1Tag),
        .excludedPhyPortTypes = 0,
    },

    .modifyVlan2Tag  =
    {
        .attr                 = FM_PORT_MODIFY_VLAN2_TAG,
        .str                  = "FM_PORT_MODIFY_VLAN2_TAG",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, modifyVlan2Tag),
        .excludedPhyPortTypes = 0,
    },

    .mirrorTruncSize  =
    {
        .attr                 = FM_PORT_MIRROR_TRUNC_SIZE,
        .str                  = "FM_PORT_MIRROR_TRUNC_SIZE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, mirrorTruncSize),
        .excludedPhyPortTypes = 0,
    },

    .parserFirstCustomTag  =
    {
        .attr                 = FM_PORT_PARSER_FIRST_CUSTOM_TAG,
        .str                  = "FM_PORT_PARSER_FIRST_CUSTOM_TAG",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, parserFirstCustomTag),
        .excludedPhyPortTypes = 0,
    },

    .parserSecondCustomTag  =
    {
        .attr                 = FM_PORT_PARSER_SECOND_CUSTOM_TAG,
        .str                  = "FM_PORT_PARSER_SECOND_CUSTOM_TAG",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, parserSecondCustomTag),
        .excludedPhyPortTypes = 0,
    },

    .parseMpls  =
    {
        .attr                 = FM_PORT_PARSE_MPLS,
        .str                  = "FM_PORT_PARSE_MPLS",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, parseMpls),
        .excludedPhyPortTypes = 0,
    },

    .routedFrameUpdateFields  =
    {
        .attr                 = FM_PORT_ROUTED_FRAME_UPDATE_FIELDS,
        .str                  = "FM_PORT_ROUTED_FRAME_UPDATE_FIELDS",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, routedFrameUpdateFields),
        .excludedPhyPortTypes = 0,
    },

    .securityAction  =
    {
        .attr                 = FM_PORT_SECURITY_ACTION,
        .str                  = "FM_PORT_SECURITY_ACTION",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, securityAction),
        .excludedPhyPortTypes = 0,
    },

    .storeMpls  =
    {
        .attr                 = FM_PORT_PARSER_STORE_MPLS,
        .str                  = "FM_PORT_PARSER_STORE_MPLS",
        .type                 = FM_TYPE_INT,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, storeMpls),
        .excludedPhyPortTypes = 0,
    },

    .tcnFifoWm  =
    {
        .attr                 = FM_PORT_TCN_FIFO_WM,
        .str                  = "FM_PORT_TCN_FIFO_WM",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, tcnFifoWm),
        .excludedPhyPortTypes = 0,
    },

    .pcieMode  =
    {
        .attr                 = FM_PORT_PCIE_MODE,
        .str                  = "FM_PORT_PCIE_MODE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, pcieMode),
        .excludedPhyPortTypes = ( EXCLUDE_ETH | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .pcieSpeed  =
    {
        .attr                 = FM_PORT_PCIE_SPEED,
        .str                  = "FM_PORT_PCIE_SPEED",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, speed),
        .excludedPhyPortTypes = ( EXCLUDE_ETH | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
        .defValue             = 0,
    },

    .pepMode  =
    {
        .attr                 = FM_PORT_PEP_MODE,
        .str                  = "FM_PORT_PEP_MODE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, pepMode),
        .excludedPhyPortTypes = ( EXCLUDE_ETH | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .ethMode  =
    {
        .attr                 = FM_PORT_ETHERNET_INTERFACE_MODE,
        .str                  = "FM_PORT_ETHERNET_INTERFACE_MODE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, ethMode),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .rxTermination  =
    {
        .attr                 = FM_PORT_RX_TERMINATION,
        .str                  = "FM_PORT_RX_TERMINATION",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .dfeMode  =
    {
        .attr                 = FM_PORT_DFE_MODE,
        .str                  = "FM_PORT_DFE_MODE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .fineTuning =
    {
        .attr                 = FM_PORT_FINE_DFE_STATE,
        .str                  = "FM_PORT_FINE_DFE_STATE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_ETH_WR | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .coarseTuning =
    {
        .attr                 = FM_PORT_COARSE_DFE_STATE,
        .str                  = "FM_PORT_COARSE_DFE_STATE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_ETH_WR | EXCLUDE_PCIE_WR | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txLaneCursor  =
    {
        .attr                 = FM_PORT_TX_LANE_CURSOR,
        .str                  = "FM_PORT_TX_LANE_CURSOR",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txLanePreCursor  =
    {
        .attr                 = FM_PORT_TX_LANE_PRECURSOR,
        .str                  = "FM_PORT_TX_LANE_PRECURSOR",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txLanePostCursor  =
    {
        .attr                 = FM_PORT_TX_LANE_POSTCURSOR,
        .str                  = "FM_PORT_TX_LANE_POSTCURSOR",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txLaneKrInitCursor  =
    {
        .attr                 = FM_PORT_TX_LANE_KR_INIT_CURSOR,
        .str                  = "FM_PORT_TX_LANE_KR_INIT_CURSOR",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
        .defValue             = 2,
    },

    .txLaneKrInitPreCursor  =
    {
        .attr                 = FM_PORT_TX_LANE_KR_INIT_PRECURSOR,
        .str                  = "FM_PORT_TX_LANE_KR_INIT_PRECURSOR",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
        .defValue             = 4,
    },

    .txLaneKrInitPostCursor  =
    {
        .attr                 = FM_PORT_TX_LANE_KR_INIT_POSTCURSOR,
        .str                  = "FM_PORT_TX_LANE_KR_INIT_POSTCURSOR",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
        .defValue             = 18,
    },

    .txLaneEnableConfigKrInit =
    {
        .attr                 = FM_PORT_TX_LANE_ENA_KR_INIT_CFG,
        .str                  = "FM_PORT_TX_LANE_ENA_KR_INIT_CFG",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = 0,
        .defValue             = FM_DISABLED,
    },

    .txLaneKrInitialPreDec  =
    {
        .attr                 = FM_PORT_TX_LANE_KR_INITIAL_PRE_DEC,
        .str                  = "FM_PORT_TX_LANE_KR_INITIAL_PRE_DEC",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
        .defValue             = 0,
    },

    .txLaneKrInitialPostDec   =
    {
        .attr                 = FM_PORT_TX_LANE_KR_INITIAL_POST_DEC,
        .str                  = "FM_PORT_TX_LANE_KR_INITIAL_POST_DEC",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = 0,
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
        .defValue             = 0,
    },

    .generateTimestamps  =
    {
        .attr                 = FM_PORT_TIMESTAMP_GENERATION,
        .str                  = "FM_PORT_TIMESTAMP_GENERATION",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, generateTimestamps),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .egressTimestampEvents  =
    {
        .attr                 = FM_PORT_EGRESS_TIMESTAMP_EVENTS,
        .str                  = "FM_PORT_EGRESS_TIMESTAMP_EVENTS",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, egressTimestampEvents),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .bistUserPatterLow40  =
    {
        .attr                 = FM_PORT_BIST_USER_PATTERN_LOW40,
        .str                  = "FM_PORT_BIST_USER_PATTERN_LOW40",
        .type                 = FM_TYPE_UINT64,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, bistUserPatterLow40),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .bistUserPatterUpp40  =
    {
        .attr                 = FM_PORT_BIST_USER_PATTERN_UPP40,
        .str                  = "FM_PORT_BIST_USER_PATTERN_UPP40",
        .type                 = FM_TYPE_UINT64,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, bistUserPatterUpp40),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .ifg  =
    {
        .attr                 = FM_PORT_IFG,
        .str                  = "FM_PORT_IFG",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, ifg),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .dicEnable  =
    {
        .attr                 = FM_PORT_DIC_ENABLE,
        .str                  = "FM_PORT_DIC_ENABLE",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, dicEnable),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txPadSize  =
    {
        .attr                 = FM_PORT_TX_PAD_SIZE,
        .str                  = "FM_PORT_TX_PAD_SIZE",
        .type                 = FM_TYPE_UINT32,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, txPadSize),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .eeeEnable  =
    {
        .attr                 = FM_PORT_EEE_MODE,
        .str                  = "FM_PORT_EEE_MODE",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, eeeEnable),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .eeeState  =
    {
        .attr                 = FM_PORT_EEE_STATE,
        .str                  = "FM_PORT_EEE_STATE",
        .type                 = FM_TYPE_INT,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, eeeState),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txPcActTimeout  =
    {
        .attr                 = FM_PORT_EEE_TX_ACTIVITY_TIMEOUT,
        .str                  = "FM_PORT_EEE_TX_ACTIVITY_TIMEOUT",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, txPcActTimeout),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .txLpiTimeout  =
    {
        .attr                 = FM_PORT_EEE_TX_LPI_TIMEOUT,
        .str                  = "FM_PORT_EEE_TX_LPI_TIMEOUT",
        .type                 = FM_TYPE_UINT32,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, txLpiTimeout),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },

    .parserVlan2First  =
    {
        .attr                 = FM_PORT_PARSER_VLAN2_FIRST,
        .str                  = "FM_PORT_PARSER_VLAN2_FIRST",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, parserVlan2First),
        .excludedPhyPortTypes = 0,
    },

    .modifyVid2First  =
    {
        .attr                 = FM_PORT_MODIFY_VID2_FIRST,
        .str                  = "FM_PORT_MODIFY_VID2_FIRST",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, modifyVid2First),
        .excludedPhyPortTypes = 0,
    },

    .replaceVlanFields =
    {
        .attr                 = FM_PORT_REPLACE_VLAN_FIELDS,
        .str                  = "FM_PORT_REPLACE_VLAN_FIELDS",
        .type                 = FM_TYPE_BOOL,
        .perLag               = TRUE,
        .attrType             = FM_PORT_ATTR_EXTENSION,
        .offset               = offsetof(fm10000_portAttr, replaceVlanFields),
        .excludedPhyPortTypes = 0,
    },
    .txClkCompensation =        
    { 
        .attr                 =    FM_PORT_TX_CLOCK_COMPENSATION,      
        .str                  =    "FM_PORT_TX_CLOCK_COMPENSATION",      
        .type                 =    FM_TYPE_UINT32,    
        .perLag               =    FALSE,  
        .attrType             =    FM_PORT_ATTR_GENERIC,    
        .offset               =    offsetof(fm_portAttr, txClkCompensation),
        .excludedPhyPortTypes =    ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),                    
    },

    .autoDetectModule =
    {
        .attr                 = FM_PORT_AUTODETECT_MODULE,
        .str                  = "FM_PORT_AUTODETECT_MODULE",
        .type                 = FM_TYPE_BOOL,
        .perLag               = FALSE,
        .attrType             = FM_PORT_ATTR_GENERIC,
        .offset               = offsetof(fm_portAttr, autoDetectModule),
        .excludedPhyPortTypes = ( EXCLUDE_PCIE | EXCLUDE_TE | EXCLUDE_LPBK ),
    },


};  /* end portAttributeTable */


typedef struct _uint32Map
{
    fm_uint32 a;
    fm_uint32 b;

} uint32Map;


/* FM10000_FCS_MODE_MAP */
static uint32Map txFcsModeMap[] = {

    /* fm_txFcsMode             fm10000_txFcsMode */
    { FM_FCS_PASSTHRU,          FM10000_FCS_PASSTHRU },
    { FM_FCS_REPLACE_NORMAL,    FM10000_FCS_REPLACE_NORMAL },
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_status ValidateEthMode( fm_int sw, fm_int port, fm_ethMode ethMode );
static fm_status ConfigureAnMode( fm_int     sw, 
                                  fm_int     port, 
                                  fm_uint32  newAnMode,
                                  fm_bool *  pRestoreAdminMode);
static fm_status updateAnMode( fm_int     sw, 
                               fm_int     port, 
                               fm_ethMode prevEthMode,
                               fm_ethMode newEthMode,
                               fm_bool *  restoreAdminMode);
static fm_status ConfigureRxTermination(fm_int     sw, 
                                        fm_int     port,
                                        fm_int     lane, 
                                        fm_rxTermination rxTermination);
static fm_status ConfigureDfeMode(fm_int     sw, 
                                  fm_int     port,
                                  fm_int     lane, 
                                  fm_dfeMode dfeMode);
static fm_status ConfigureLoopbackMode(fm_int     sw, 
                                       fm_int     port, 
                                       fm_int     loopbackMode);
static fm_status ConfigureFabricLoopback(fm_int     sw, 
                                         fm_int     port, 
                                         fm_int     loopbackMode);
fm_status fm10000SetPortAttributeInt(fm_int sw,
                                     fm_int port,
                                     fm_int lane,
                                     fm_int attr,
                                     void * value);

static fm_status SwapPortStateMachineType( fm_int sw,
                                           fm_int port,
                                           fm_int curType,
                                           fm_int newType );

static fm_status SetSerDesSmType( fm_int sw, fm_int port, fm_int *smType );

static fm_uint16 ConvertPpmToTxClkCompensationTimeout( fm_uint32  pcsType,
                                                       fm_uint32  speed,
                                                       fm_uint32  num_ppm );

static fm_int CountOnes( fm_uint64 value );

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** CmpTimestamp
 * \ingroup intPort
 *
 * \desc            Comparison function for fm10000_portStateTDump sorting based
 *                  on timestamp.
 *
 * \param[in]       p1 points to the first fm10000_portStateTDump .
 *
 * \param[in]       p2 points to the second fm10000_portStateTDump.
 *
 * \return          negative if the first entry comes before the 2nd entry.
 * \return          positive if the first entry comes after the 2nd entry.
 *
 *****************************************************************************/
static fm_int CmpTimestamp(const void *p1, const void *p2)
{

   return( ( *(fm10000_portStateTDump * const *) p1)->timestamp -
           ( *(fm10000_portStateTDump * const *) p2)->timestamp );

}   /* end CmpTimestamp */


/*****************************************************************************/
/** fm10000SmTypeStr
 * \ingroup intPort
 *
 * \desc            Return string description of the state machine type.
 *
 * \param[in]       type is the state machine type value.
 *
 * \return          String representation of the value
 *
 *****************************************************************************/
static fm_text fm10000SmTypeStr(fm_int type)
{
    switch (type)
    {
        case FM10000_AN_PORT_STATE_MACHINE:
            return "AN_PORT";
        case FM10000_BASIC_PORT_STATE_MACHINE:
            return "BASIC";
        case FM10000_PCIE_PORT_STATE_MACHINE:
            return "PCIE";
        case FM10000_CLAUSE73_AN_STATE_MACHINE:
            return "AN73";
        case FM10000_CLAUSE37_AN_STATE_MACHINE:
            return "AN37";
        case FM10000_BASIC_SERDES_STATE_MACHINE:
            return "SERDES";
        case FM10000_PCIE_SERDES_STATE_MACHINE:
            return "PCIE_S";
        case FM10000_STUB_SERDES_STATE_MACHINE:
            return "STUB";
        case FM10000_BASIC_SERDES_DFE_STATE_MACHINE:
            return "DFE";
    }

    return "UNKN";

} /* end fm10000SmTypeStr */



/*****************************************************************************/
/** ReadModPerPortCfg1
 * \ingroup intPort
 *
 * \desc            Reads an entry from the MOD_PER_PORT_CFG_1 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[out]      value points to the location to receive the contents
 *                  of the register table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadModPerPortCfg1(fm_int sw, fm_int index, fm_uint64 * value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheReadUINT64(sw, &fm10000CacheModPerPortCfg1, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->ReadUINT64(sw, FM10000_MOD_PER_PORT_CFG_1(index, 0), value);
#endif

}   /* end ReadModPerPortCfg1 */




/*****************************************************************************/
/** WriteModPerPortCfg1
 * \ingroup intPort
 *
 * \desc            Writes an entry to the MOD_PER_PORT_CFG_1 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[in]       value is the value to be written to the register entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteModPerPortCfg1(fm_int sw, fm_int index, fm_uint64 value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheWriteUINT64(sw, &fm10000CacheModPerPortCfg1, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->WriteUINT64(sw, FM10000_MOD_PER_PORT_CFG_1(index, 0), value);
#endif

}   /* end WriteModPerPortCfg1 */




/*****************************************************************************/
/** ReadModPerPortCfg2
 * \ingroup intPort
 *
 * \desc            Reads an entry from the MOD_PER_PORT_CFG_2 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[out]      value points to the location to receive the contents
 *                  of the register table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadModPerPortCfg2(fm_int sw, fm_int index, fm_uint64 * value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheReadUINT64(sw, &fm10000CacheModPerPortCfg2, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->ReadUINT64(sw, FM10000_MOD_PER_PORT_CFG_2(index, 0), value);
#endif

}   /* end ReadModPerPortCfg2 */




/*****************************************************************************/
/** WriteModPerPortCfg2
 * \ingroup intPort
 *
 * \desc            Writes an entry to the MOD_PER_PORT_CFG_2 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[in]       value is the value to be written to the register entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteModPerPortCfg2(fm_int sw, fm_int index, fm_uint64 value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheWriteUINT64(sw, &fm10000CacheModPerPortCfg2, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->WriteUINT64(sw, FM10000_MOD_PER_PORT_CFG_2(index, 0), value);
#endif

}   /* end WriteModPerPortCfg2 */




/*****************************************************************************/
/** ReadParserPortCfg2
 * \ingroup intPort
 *
 * \desc            Reads an entry from the PARSER_PORT_CFG_2 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[out]      value points to the location to receive the contents
 *                  of the register table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadParserPortCfg2(fm_int sw, fm_int index, fm_uint64 * value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheReadUINT64(sw, &fm10000CacheParserPortCfg2, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->ReadUINT64(sw, FM10000_PARSER_PORT_CFG_2(index, 0), value);
#endif

}   /* end ReadParserPortCfg2 */




/*****************************************************************************/
/** WriteParserPortCfg2
 * \ingroup intPort
 *
 * \desc            Writes an entry to the PARSER_PORT_CFG_2 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[in]       value is the value to be written to the register entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteParserPortCfg2(fm_int sw, fm_int index, fm_uint64 value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheWriteUINT64(sw, &fm10000CacheParserPortCfg2, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->WriteUINT64(sw, FM10000_PARSER_PORT_CFG_2(index, 0), value);
#endif

}   /* end WriteParserPortCfg2 */




#if 0

/*****************************************************************************/
/** ReadParserPortCfg3
 * \ingroup intPort
 *
 * \desc            Reads an entry from the PARSER_PORT_CFG_3 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[out]      value points to the location to receive the contents
 *                  of the register table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadParserPortCfg3(fm_int sw, fm_int index, fm_uint64 * value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheReadUINT64(sw, &fm10000CacheParserPortCfg3, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->ReadUINT64(sw, FM10000_PARSER_PORT_CFG_3(index, 0), value);
#endif

}   /* end ReadParserPortCfg3 */




/*****************************************************************************/
/** WriteParserPortCfg3
 * \ingroup intPort
 *
 * \desc            Writes an entry to the PARSER_PORT_CFG_3 register table.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       index is the index of the register table entry.
 * 
 * \param[in]       value is the value to be written to the register entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteParserPortCfg3(fm_int sw, fm_int index, fm_uint64 value)
{

#if (FM10000_USE_PORT_CFG_CACHE)
    return fmRegCacheWriteUINT64(sw, &fm10000CacheParserPortCfg3, index, value);
#else
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    return switchPtr->WriteUINT64(sw, FM10000_PARSER_PORT_CFG_3(index, 0), value);
#endif

}   /* end WriteParserPortCfg3 */

#endif




/*****************************************************************************/
/** GetPortAttrEntry
 * \ingroup intPort
 *
 * \desc            Returns a pointer to the entry corresponding to the given
 *                  attribute.
 *
 * \param[in]       attr is the attribute on which to operate.
 *
 * \return          Pointer to port attribute entry.
 * \return          NULL if attribute not found.
 *
 *****************************************************************************/
static fm_portAttrEntry *GetPortAttrEntry( fm_uint attr )
{
    fm_portAttrEntry *entry;
    fm_uint           i;

    entry = (fm_portAttrEntry *) &portAttributeTable;

    for (i = 0 ;
         i < sizeof(portAttributeTable)/sizeof(fm_portAttrEntry) ;
         i++)
    {
        if (entry->attr == (fm_int) attr)
        {
            return entry;
        }
        entry++;
    }

    return NULL;

}   /* end GetPortAttrEntry */




/*****************************************************************************/
/** CachePortAttribute
 * \ingroup intPort
 *
 * \desc            Cache the given attribute value.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have claimed the port attribute lock (PORT_ATTR_LOCK).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       attr is the attribute on which to operate.
 *
 * \param[in]       value points to the attribute value to cache.
 *
 * \param[in]       entry pointer to fm_portAttrEntry on which to operate.
 *                  Can be a NULL pointer. In that case the fm_portAttrEntry
 *                  pointer will be retreive using the 'attr' argument.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in 
 *                  this switch.
 * \return          FM_ERR_INVALID_ATTRIB if invalid port attribute definition 
 *                  in the port attribute table or unrecognised attribute.
 *
 *****************************************************************************/
static fm_status CachePortAttribute(fm_int            sw,
                                    fm_int            port,
                                    fm_int            attr,
                                    void *            value,
                                    fm_portAttrEntry *entry)
{
    fm_status         err;
    fm_switch *       switchPtr;
    fm_portAttrEntry *attrEntry;
    fm10000_portAttr *portAttrExt;
    fm_portAttr *     portAttr;
    void *            ptr;

    err       = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    attrEntry = (entry != NULL) ? entry : GetPortAttrEntry(attr);

    if (attrEntry)
    {
        portAttr    = GET_PORT_ATTR(sw, port);
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

        /* Compute the port attribute address. */
        if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
        {
            ptr = GET_PORT_ATTR_ADDRESS(portAttr, attrEntry);
        }
        else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
        {
            ptr = GET_PORT_ATTR_ADDRESS(portAttrExt, attrEntry);
        }
        else
        {
            err = FM_ERR_INVALID_ATTRIB;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        switch (attrEntry->type)
        {
            case FM_TYPE_INT:
                *( (fm_int *) ptr ) = *( (fm_int *) value );
                break;

            case FM_TYPE_UINT32:
                *( (fm_uint32 *) ptr ) = *( (fm_uint32 *) value );
                break;

            case FM_TYPE_UINT64:
                *( (fm_uint64 *) ptr ) = *( (fm_uint64 *) value );
                break;

            case FM_TYPE_BOOL:
                *( (fm_bool *) ptr ) = *( (fm_bool *) value );
               break;

            case FM_TYPE_PORTMASK:
                if (attrEntry->attr == FM_PORT_MASK_WIDE)
                {
                    /* Convert bitArray to portmask*/
                    err = fmBitArrayToPortMask((fm_bitArray *) value,
                                               (fm_portmask *) ptr,
                                               switchPtr->maxPhysicalPort + 1);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                }
                else
                {
                    *( (fm_portmask *) ptr ) = *( (fm_portmask *) value );
                }
                break;

            case FM_TYPE_MACADDR:
                *( (fm_macaddr *) ptr ) = *( (fm_macaddr *) value );
                break;

            default:
                err = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else
    {
        err = fmIsValidPortAttribute(attr) ? FM_ERR_UNSUPPORTED :
                                             FM_ERR_INVALID_ATTRIB;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

ABORT:
    return err;

}   /* end CachePortAttribute */




/*****************************************************************************/
/** GetCachedPortAttribute
 * \ingroup intPort
 *
 * \desc            Get port attribute from the cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       attr is the attribute on which to operate.
 *
 * \param[in]       entry pointer to fm_portAttrEntry on which to operate.
 *                  Can be a NULL pointer. In that case the fm_portAttrEntry
 *                  pointer will be retreive using the 'attr' argument.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in 
 *                  this switch.
 * \return          FM_ERR_INVALID_ATTRIB if invalid attribute definition in 
 *                  port attribute table or unrecognised attribute.
 *
 *****************************************************************************/
static fm_status GetCachedPortAttribute(fm_int            sw,
                                        fm_int            port,
                                        fm_int            attr,
                                        fm_portAttrEntry *entry,
                                        void *            value)
{
    fm_status         err;
    fm_switch *       switchPtr;
    fm_portAttrEntry *attrEntry;
    fm10000_portAttr *portAttrExt;
    fm_portAttr *     portAttr;
    void *            ptr;

    err       = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    attrEntry = (entry != NULL) ? entry : GetPortAttrEntry(attr);

    if (attrEntry)
    {
        portAttr    = GET_PORT_ATTR(sw, port);
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

        /* Compute the port attribute address. */
        if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
        {
            ptr = GET_PORT_ATTR_ADDRESS(portAttr, attrEntry);
        }
        else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
        {
            ptr = GET_PORT_ATTR_ADDRESS(portAttrExt, attrEntry);
        }
        else
        {
            err = FM_ERR_INVALID_ATTRIB;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        switch (attrEntry->type)
        {
            case FM_TYPE_INT:
                *( (fm_int *) value ) = *( (fm_int *) ptr );
                break;

            case FM_TYPE_UINT32:
                *( (fm_uint32 *) value ) = *( (fm_uint32 *) ptr );
                break;

            case FM_TYPE_UINT64:
                *( (fm_uint64 *) value ) = *( (fm_uint64 *) ptr );
                break;

            case FM_TYPE_BOOL:
                *( (fm_bool *) value ) = *( (fm_bool *) ptr );
               break;

            case FM_TYPE_PORTMASK:
                if (attrEntry->attr == FM_PORT_MASK_WIDE)
                {
                    /* Convert to bitArray */
                    err = fmPortMaskToBitArray((fm_portmask *) ptr,
                                               (fm_bitArray *) value,
                                               switchPtr->maxPhysicalPort + 1);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                }
                else
                {
                    *( (fm_portmask *) value ) = *( (fm_portmask *) ptr );
                }
                break;

            case FM_TYPE_MACADDR:
                *( (fm_macaddr *) value ) = *( (fm_macaddr *) ptr );
                break;

            default:
                err = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else
    {
        err = fmIsValidPortAttribute(attr) ? FM_ERR_UNSUPPORTED :
                                             FM_ERR_INVALID_ATTRIB;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

ABORT:
    return err;

}   /* end GetCachedPortAttribute */




/*****************************************************************************/
/** SetLAGPortAttribute
 * \ingroup intPort
 *
 * \desc            Set a port attribute on a LAG logical port.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have claimed the port attribute lock (PORT_ATTR_LOCK)
 *                  and the LAG lock.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate.
 *
 * \param[in]       attribute is the port attribute to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_PORT_MAC if mac is not valid for the
 *                  specified port, or if FM_PORT_ACTIVE_MAC was
 *                  specified and the port has no active MAC selected.
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not valid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in 
 *                  this switch.
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 * \return          FM_ERR_READONLY_ATTRIB if read-only attribute.
 *
 *****************************************************************************/
static fm_status SetLAGPortAttribute(fm_int sw,
                                     fm_int port,
                                     fm_int lane,
                                     fm_int attribute,
                                     void * value)
{
    fm_portAttrEntry *attrEntry;
    fm_portAttr *     portAttr;
    fm10000_port *    portExt;
    fm_switch *       switchPtr;
    fm_port *         portPtr;
    fm_lag *          lagPtr;
    fm_status         status = FM_OK;
    fm_int            members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int            numMembers;
    fm_int            memberPort;
    fm_int            i;
    fm_bool           isPciePort;
    fm_bitArray       tmpMaskBitArray;
    fm10000_switch   *switchExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                 port,
                 "sw=%d port=%d attribute=%d value=%p\n",
                 sw,
                 port,
                 attribute,
                 value);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    portPtr   = GET_PORT_PTR(sw, port);
    portAttr  = GET_PORT_ATTR(sw, port);

    if (portPtr->portType != FM_PORT_TYPE_LAG)
    {
        status = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }

    /* Obtain a list of all logical member ports of this LAG */
    status = fmGetLAGCardinalPortList(sw,
                                      port,
                                      &numMembers,
                                      members,
                                      FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    lagPtr = GET_LAG_PTR(sw, portPtr->lagIndex);

    /***************************************************
     * FM_PORT_INTERNAL may be applied to a lag only if
     * the lag doesn't have any member ports.
     ***************************************************/
    if (attribute == FM_PORT_INTERNAL)
    {
        if (numMembers > 0)
        {
            status = FM_ERR_LAG_IN_USE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
        }

        lagPtr->isInternalPort = *( (fm_bool *) value );
        portAttr->internalPort = *( (fm_bool *) value );

        /* Exit immediately since there is no member port */
        goto ABORT;
    }
    /* Pre-process port mask wide attribute to remove TE from internal port masks.
     * All encapsulation and decapsulation occurs on the ingress switch. No
     * frames that have been sent across internal links should ever be
     * encapsulated or decapsulated. */
    else if ( (attribute == FM_PORT_MASK_WIDE) &&
              lagPtr->isInternalPort &&
              fmGetBoolApiAttribute(FM_AAK_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS,
                                    FM_AAD_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS) )
    {
        tmpMaskBitArray = *( (fm_bitArray *) value );

        status = fmSetBitArrayBit(&tmpMaskBitArray,
                                  switchExt->tunnelCfg->tunnelPort[0],
                                  0);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
        
        status = fmSetBitArrayBit(&tmpMaskBitArray,
                                  switchExt->tunnelCfg->tunnelPort[1],
                                  0);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }


    /* Just cache attribute for lag port */
    attrEntry = GetPortAttrEntry(attribute);

    if (attrEntry)
    {
        /* In per-LAG management mode only per-lag attributes
         * can be set on a lag. */
        if (!switchPtr->perLagMgmt || attrEntry->perLag)
        {
            status = CachePortAttribute(sw,
                                        port,
                                        attribute,
                                        value,
                                        attrEntry);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
        }
        else
        {
            status = FM_ERR_NOT_PER_LAG_ATTRIBUTE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
        }
    }
    else
    {
        status = fmIsValidPortAttribute(attribute) ? FM_ERR_UNSUPPORTED :
                                                     FM_ERR_INVALID_ATTRIB;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }

    /* Apply attribute to each member ports */
    for (i = 0 ; i < numMembers ; i++)
    {
        memberPort = members[i];

        portExt = GET_PORT_EXT(sw, memberPort);
        portExt->allowCfg = TRUE;

        status = fm10000IsPciePort(sw, memberPort, &isPciePort);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        if ( (isPciePort) &&
             ( (attribute == FM_PORT_MIN_FRAME_SIZE) ||
               (attribute == FM_PORT_MAX_FRAME_SIZE) ) )
        {
            /* Attribute does not apply to PCIE ports */
            continue;
        }

        status = fm10000SetPortAttribute(sw,
                                         memberPort,
                                         0,
                                         lane,
                                         attribute,
                                         value);
        portExt->allowCfg = FALSE;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }

ABORT:
    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, status);

}   /* end SetLAGPortAttribute */




/*****************************************************************************/
/** UpdateTruncFrameSize
 * \ingroup intPort
 *
 * \desc            Updates the mirror truncation length register.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateTruncFrameSize(fm_int sw,
                                      fm_int port)
{
    fm_status         err;
    fm_switch *       switchPtr;
    fm_portAttr *     portAttr;
    fm10000_portAttr *portAttrExt;
    fm_int            physPort;
    fm_uint64         reg64;
    fm_int            tmpInt;
    fm_int            islTagSize;
    fm_bool           regLockTaken = FALSE;

    switchPtr   = GET_SWITCH_PTR(sw);
    portAttr    = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /* convert the logical port to physical port */
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    islTagSize = fmGetISLTagSize(portAttr->islTagFormat);

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Update truncation length */
    err = ReadModPerPortCfg2(sw, physPort, &reg64);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    tmpInt = (portAttrExt->mirrorTruncSize + islTagSize);
    if (tmpInt >= FM10000_MAX_TRUNC_LEN)
    {
        /* Saturate the value */
        tmpInt = FM10000_MAX_TRUNC_LEN;
    }

    FM_SET_FIELD64(reg64,
                   FM10000_MOD_PER_PORT_CFG_2,
                   MirrorTruncationLen,
                   (tmpInt >> 2));

    err = WriteModPerPortCfg2(sw, physPort, reg64);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    FM_FLAG_DROP_REG_LOCK(sw);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end UpdateTruncFrameSize */




/*****************************************************************************/
/** UpdateMinMaxFrameSize
 * \ingroup intPort
 *
 * \desc            Updates registers that depend on minimum and maximum frame
 *                  size. 
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateMinMaxFrameSize(fm_int sw,
                                       fm_int port)
{
    fm_status         err;
    fm_switch *       switchPtr;
    fm_portAttr *     portAttr;
    fm_int            physPort;
    fm_int            epl;
    fm_int            channel;
    fm_int            islTagSize;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_bool           regLockTaken = FALSE;

    switchPtr   = GET_SWITCH_PTR(sw);
    portAttr    = GET_PORT_ATTR(sw, port);

    /* convert the logical port to physical port */
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /* Retrieve the EPL and channel numbers. */
    err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
    if (err == FM_OK)
    {
        FM_FLAG_TAKE_REG_LOCK(sw);

        islTagSize = fmGetISLTagSize(portAttr->islTagFormat);

        err = switchPtr->ReadUINT32Mult(sw, 
                                        FM10000_MAC_CFG(epl, channel, 0), 
                                        FM10000_MAC_CFG_WIDTH,
                                        macCfg);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        FM_ARRAY_SET_FIELD(macCfg, 
                           FM10000_MAC_CFG, 
                           RxMinFrameLength, 
                           portAttr->minFrameSize + islTagSize);
        FM_ARRAY_SET_FIELD(macCfg, 
                           FM10000_MAC_CFG, 
                           RxMaxFrameLength, 
                           portAttr->maxFrameSize + islTagSize);

        err = switchPtr->WriteUINT32Mult(sw, 
                                         FM10000_MAC_CFG(epl, channel, 0), 
                                         FM10000_MAC_CFG_WIDTH,
                                         macCfg);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        FM_FLAG_DROP_REG_LOCK(sw);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end UpdateMinMaxFrameSize */




/*****************************************************************************/
/** UpdateInternalPort
 * \ingroup intPort
 *
 * \desc            Update the STP state of the internal port to
 *                  forwarding.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate. The port must
 *                  represent a physical port and not a LAG or Multicast
 *                  group.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_PORT if the port is not a physical port
 *
 *****************************************************************************/
static fm_status UpdateInternalPort(fm_int sw, fm_int port)
{

    fm_status  err;
    fm_switch *switchPtr;
    fm_stpMode stpMode;
    fm_bool    regLockTaken;
    fm_int     vlanId;
    fm_int     stp;
    fm_int     stpPortState;
    fm_bool    lastStpIter;
    fm_bool    isMember;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, port, "sw=%d port=%d \n", sw, port);

    switchPtr    = GET_SWITCH_PTR(sw);
    regLockTaken = FALSE;
    lastStpIter  = FALSE;

    /* Make sure, that the port is a physical port */
    if ( !fmIsCardinalPort(sw, port) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    FM_DROP_PORT_ATTR_LOCK(sw);
    FM_TAKE_L2_LOCK(sw);
    FM_TAKE_MTABLE_LOCK(sw);
    FM_TAKE_PORT_ATTR_LOCK(sw);
    FM_FLAG_TAKE_REG_LOCK(sw);

    err = fm10000GetSwitchAttribute(sw, FM_SPANNING_TREE_MODE, &stpMode);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /****************************************
     * Add the port to all existing vlans
     ***************************************/
    err = fmGetVlanFirst(sw, &vlanId);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    while (vlanId != -1)
    {
        fm_int nextVlanId;

        err = fmGetVlanMembership(sw,
                                  &switchPtr->vidTable[vlanId],
                                  port,
                                  &isMember);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        if (!isMember)
        {
            err = fmAddVlanPort(sw,
                                vlanId,
                                port,
                                FALSE);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        err = fmGetVlanNext(sw, vlanId, &nextVlanId);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        vlanId = nextVlanId;
    }

    /********************************************
     * Set the STP state depending on the mode
     *******************************************/
    switch (stpMode)
    {
        case FM_SPANNING_TREE_MULTIPLE:
            err = fmGetSpanningTreeFirst(sw, &stp);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            while ( (stp != -1) && !lastStpIter )
            {
                fm_int nextStp;

                err = fmGetSpanningTreePortState(sw, stp, port, &stpPortState);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                if (stpPortState != FM_STP_STATE_FORWARDING)
                {
                    err = fmSetSpanningTreePortState(sw,
                                                     stp,
                                                     port,
                                                     FM_STP_STATE_FORWARDING);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                }

                err = fmGetSpanningTreeNext(sw, stp, &nextStp);
                if (err != FM_ERR_NO_MORE)
                {
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                }
                else
                {
                    err         = FM_OK;
                    lastStpIter = TRUE;
                }

                stp = nextStp;
            }
            break;

        case FM_SPANNING_TREE_SHARED:
            err = fmGetSpanningTreePortState(sw, 0, port, &stpPortState);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            if (stpPortState != FM_STP_STATE_FORWARDING)
            {
                err = fmSetSpanningTreePortState(sw,
                                                 0,
                                                 port,
                                                 FM_STP_STATE_FORWARDING);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }
            break;

        default:
            break;
    }

ABORT:

    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
        FM_DROP_PORT_ATTR_LOCK(sw);
        FM_DROP_MTABLE_LOCK(sw);
        FM_DROP_L2_LOCK(sw);
        FM_TAKE_PORT_ATTR_LOCK(sw);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end UpdateInternalPort */


/*****************************************************************************/
/** ValidatePort
 * \ingroup intPort
 *
 * \desc            Given a logical port number, verify that it has a
 *                  physical port association
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number of the physical port
 *                  whose lane is to be validated.
 *
 * \return          FM_OK if the mac and lane are valid for the specified port.
 *
 * \return          FM_ERR_INVALID_PORT if port is not the logical port number
 *                  of a physical port.
 *****************************************************************************/
static fm_status ValidatePort( fm_int sw, fm_int port )
{
    fm_status     err;
    fm_int        physSw;
    fm_int        physPort;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, 
                     port,
                     "sw=%d port=%d\n",
                     sw,
                     port );

    if ( !fmIsCardinalPort( sw, port ) )
    {
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    err = fmPlatformMapLogicalPortToPhysical( sw, port, &physSw, &physPort );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    if ( sw != physSw )
    {
        /* Should never happen */
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ValidatePort */


/*****************************************************************************/
/** ValidateLane
 * \ingroup intPort
 *
 * \desc            Given a logical port number, determine if the specified
 *                  lane numbers are valid. Some ports have one lane or four
 *                  depends on the port's configuration and the configuration of
 *                  the other ports sharing the EPL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number of the physical port
 *                  whose lane is to be validated.
 *
 * \param[in]       lane is the port's zero-based lane number to validate,
 *                  or FM_PORT_LANE_NA if lane is to be ignored.
 *
 * \return          FM_OK if the mac and lane are valid for the specified port.
 *
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not a valid lane number
 *                  for the specified port. If this value is returned, the
 *                  mac argument is guaranteed to be valid.
 *****************************************************************************/
static fm_status ValidateLane( fm_int sw, fm_int port, fm_int lane )
{
    fm_status         err;
    fm_int            numLanes;
    fm_port          *portPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, 
                     port,
                     "sw=%d port=%d lane=%d\n",
                     sw,
                     port,
                     lane );

    err         = FM_OK;
    portPtr     = GET_PORT_PTR( sw, port );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    if ( lane == FM_PORT_LANE_NA )
    {
        /* Lane number is Not Applicable */
        ;
    }
    else if ( portExt->ring     == FM10000_SERDES_RING_EPL   &&
             portExt->ethMode  == FM_ETH_MODE_AN_73         && 
             portPtr->phyInfo.notifyEthModeChange == NULL )
    {
        /* assume 4 lanes for auto-negotiation. Do we still need this? */
        if ( (lane < 0) || (lane > 3) )
        {
            err = FM_ERR_INVALID_PORT_LANE;
        }
    }
    else
    {
        if ( portExt->ring == FM10000_SERDES_RING_EPL )
        {
            numLanes = 1;
            if ( (portPtr->capabilities & FM_PORT_CAPABILITY_SPEED_40G) ||
                 (portPtr->capabilities & FM_PORT_CAPABILITY_SPEED_100G) )
            {
                numLanes = 4;
            }
        }
        else
        {
            fm10000GetNumLanes( sw, port, &numLanes );
        }

        if ( lane < 0 || lane >= numLanes )
        {
            err = FM_ERR_INVALID_PORT_LANE;
        }

    }  

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end ValidateLane */



/*****************************************************************************/
/** ValidatePortLane
 * \ingroup intPort
 *
 * \desc            Given a logical port number, verify that it has a
 *                  physical port association and determine if the specified
 *                  lane numbers are valid. Some ports have one lane or four
 *                  depends on the port's configuration and the configuration of
 *                  the other ports sharing the EPL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number of the physical port
 *                  whose lane is to be validated.
 *
 * \param[in]       lane is the port's zero-based lane number to validate,
 *                  or FM_PORT_LANE_NA if lane is to be ignored.
 *
 * \return          FM_OK if the mac and lane are valid for the specified port.
 * \return          FM_ERR_INVALID_ARGUMENT if port is out of range.
 * \return          FM_ERR_INVALID_PORT if port is not the logical port number
 *                  of a physical port.
 * \return          FM_ERR_INVALID_PORT_MAC if mac is not a valid MAC number
 *                  for the specified port.
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not a valid lane number
 *                  for the specified port. If this value is returned, the
 *                  mac argument is guaranteed to be valid.
 *
 *****************************************************************************/
static fm_status ValidatePortLane( fm_int sw,
                                   fm_int port,
                                   fm_int lane )
{
    fm_status     err;

    err = ValidatePort( sw, port );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    err = ValidateLane( sw, port, lane );
ABORT:
    return err;

}   /* end ValidatePortLane */




/*****************************************************************************/
/** ValidateEthMode
 * \ingroup intPort
 *
 * \desc            Determine if the specified Ethernet interface mode is
 *                  valid in the face of how the other ports sharing this
 *                  port's EPL are configured.
 *                                                                      \lb\lb
 *                  The hardware constrains the choice of modes depending on
 *                  how the 4 ports sharing the EPL are configured:
 *                                                                      \lb\lb
 *                  (a) If any of the 4 ports are in 40G or 100G the other 3
 *                  ports must be disabled.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number whose Ethernet interface
 *                  mode is to be validated (the target port).
 *
 * \param[in]       ethMode is the Ethernet interface mode to be validated.
 *
 * \return          FM_OK if ethMode is valid for this port's MAC.
 * 
 * \return          FM_ERR_INVALID_PORT_STATE if ethMode cannot be selected for
 *                  this port's MAC because of conflicts with other ports
 *                  sharing the Ethernet port logic hardware resources.
 * 
 * \return          FM_ERR_INVALID_ETH_MODE if an attempt is made to configure
 *                  an ethMode that can only be auto-negotiated via Clause 73
 * 
 * \return          FM_ERR_INVALID_PORT if port is not a physical port.
 *
 *****************************************************************************/
static fm_status ValidateEthMode( fm_int sw, fm_int port, fm_ethMode ethMode )
{
    fm_status         err;
    fm_int            i;
    fm_int            fabricPort;
    fm_int            otherFabricPort;
    fm_int            otherPort;
    fm_port          *portPtr;
    fm10000_port     *portExt;
    fm10000_port     *otherPortExt;
    fm10000_portAttr *portAttrExt;
    fm10000_portAttr *otherPortAttrExt;
    fm_bool           isAnotherPortEnabled;
    fm_bool           isAnotherPort4Lanes; 
    fm_serdesRing     ring;
    fm_int            serDes;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, 
                     port,
                     "sw=%d port=%d ethMode=%d\n",
                     sw,
                     port,
                     ethMode );

    /* return with an error if trying to set a read-only mode */
    if ( ethMode == FM_ETH_MODE_10GBASE_KR  || 
         ethMode == FM_ETH_MODE_40GBASE_KR4 || 
         ethMode == FM_ETH_MODE_40GBASE_CR4 )
    {
        err = FM_ERR_INVALID_ETH_MODE;
        goto ABORT;
    }

    /* Make sure it's a physical port. */
    if ( !fmIsCardinalPort( sw, port ) )
    {
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    portPtr     = GET_PORT_PTR( sw, port );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    /* Convert logical port number to physical port number. */
    err = fm10000MapLogicalPortToFabricPort( sw, port, &fabricPort );
    if ( err == FM_ERR_INVALID_PORT )
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                         port, 
                         "Port %d isn't mapped to any switch fabric port\n",
                         port );
        goto ABORT;
    }
    else if( err != FM_OK )
    {
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
    }



    /* Now make sure it's an ethernet port */
    err = fm10000MapFabricPortToSerdes( sw, fabricPort, &serDes, &ring );
    if ( err != FM_OK || ring != FM10000_SERDES_RING_EPL )
    {
        /* it's not */
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    /****************************************************
     * First, if we want to configure for 40G/100G, make 
     * sure this port has the capability to do so.
     ****************************************************/

    if ( ( ethMode & FM_ETH_MODE_4_LANE_BIT_MASK ) &&
         ((portPtr->capabilities & FM_PORT_CAPABILITY_SPEED_40G) == 0) &&
         ((portPtr->capabilities & FM_PORT_CAPABILITY_SPEED_100G) == 0) )
    {
        err = FM_ERR_INVALID_PORT_STATE;
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
    }

    /**************************************************
     * Second catalog how all other ports sharing this
     * port's EPL are configured.
     **************************************************/

    isAnotherPortEnabled = FALSE;
    isAnotherPort4Lanes  = FALSE;

    /* For each port sharing the EPL ... */
    for ( i = 0 ; i < FM10000_PORTS_PER_EPL ; i++ )
    {
        otherFabricPort = ( fabricPort & EPL_PORT_GROUP_MASK ) | i;

        /* Skip the target port. */
        if ( otherFabricPort == fabricPort )
        {
            continue;
        }

        /* Get other port's logical port number. */
        err = fm10000MapFabricPortToLogicalPort( sw,
                                                 otherFabricPort,
                                                 &otherPort );
        if ( err == FM_ERR_INVALID_PORT )
        {
            /* This could happen if at least one port of an EPL isn't used,
               simply continue with next port. */
            err = FM_OK;
            continue;
        }
        else
        {
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
        }

        /* Get that port's configuration */
        otherPortExt     = GET_PORT_EXT( sw, otherPort );
        otherPortAttrExt = GET_FM10000_PORT_ATTR( sw, otherPort );

        /* Check for enabled port on any of this port's EPL */
        if ( otherPortAttrExt->ethMode & FM_ETH_MODE_ENABLED_BIT_MASK )
        {
            isAnotherPortEnabled = TRUE;

            /* Check if port is enabled as 4-lane on this EPL */
            if ( otherPortAttrExt->ethMode & FM_ETH_MODE_4_LANE_BIT_MASK )
            {
                isAnotherPort4Lanes = TRUE;
            }

        }   /* end if ( otherPortAttrExt->ethMode & FM_ETH_MODE_ENABLED_BIT_MASK ) */

    }   /* end for (i = 0 ; i < FM10000_PORTS_PER_EPL ; i++) */



    if ( ethMode & FM_ETH_MODE_4_LANE_BIT_MASK )
    {
        /****************************************************
         * If we want to configure for 40G/100G, make sure 
         * all other ports on the same EPL are disabled
         ****************************************************/

        if ( isAnotherPortEnabled )
        {
            err = FM_ERR_INVALID_PORT_STATE;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
        }
    }
    else if ( ethMode & FM_ETH_MODE_ENABLED_BIT_MASK )
    {
        /**************************************************
         * One lane mode: make sure no other ports are in
         * 4-lane on this EPL.
         **************************************************/

        if ( isAnotherPort4Lanes )
        {
            err = FM_ERR_INVALID_PORT_STATE;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
        }
    }
    else
    {
        /**************************************************
         * We're disabling this port on this EPL. Allow
         * unconditionally.
         **************************************************/

        ;

    }   

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ValidateEthMode */




/*****************************************************************************/
/** updateAnMode
 * \ingroup intPort
 *
 * \desc            Update the Autoneg mode according to the new Ethernet
 *                  Mode if required.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number whose Ethernet interface
 *                  mode is to be validated (the target port).
 *
 * \param[in]       prevEthMode is the previous Ethernet interface mode.
 *
 * \param[in]       newEthMode is the new Ethernet interface mode.
 *
 * \param[out]      restoreAdminMode is a pointer to a caller-allocated
 *                  variable where this function will return TRUE if it has
 *                  switched  port-level state transition table and therefore
 *                  the caller has to restore the current port admin mode. The
 *                  return value will be FALSE otherwise.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if autonegotiation mode is invalid
 *                  or restoreAdminMode is a NULL pointer.
 * \return          FM_ERR_UNSUPPORTED if autonegotiation mode cannot be set
 *                  for the specified port.
 *                  of a physical port.
 * \return          FM_ERR_PER_LAG_ATTRIBUTE attribute cannot be set on a lag
 *                  logical port.
 *
 *****************************************************************************/
static fm_status updateAnMode( fm_int     sw, 
                               fm_int     port, 
                               fm_ethMode prevEthMode,
                               fm_ethMode newEthMode,
                               fm_bool *  restoreAdminMode)
{
    fm_status    err=FM_OK;
    fm_uint32    anMode;
    fm_portAttr *portAttr;

    portAttr = GET_PORT_ATTR(sw, port);

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, 
                     port,
                     "sw=%d port=%d prevEthMode=%d newEthMode=%d\n",
                     sw,
                     port,
                     prevEthMode,
                     newEthMode );

    /* Adjust the Autoneg mode accordingly if required */
    if ( (newEthMode == FM_ETH_MODE_SGMII) || 
         (newEthMode == FM_ETH_MODE_AN_73) )
    {
        /* Enable AN */
        if (newEthMode == FM_ETH_MODE_SGMII)
        {
            anMode = FM_PORT_AUTONEG_SGMII;
        }
        else
        {
            anMode = FM_PORT_AUTONEG_CLAUSE_73;
        }

        err = ConfigureAnMode(sw, port, anMode, restoreAdminMode);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        portAttr->autoNegMode = anMode;
    }
    else if ( (prevEthMode == FM_ETH_MODE_SGMII) || 
              (prevEthMode == FM_ETH_MODE_AN_73) )
    {
        /* Disable AN */
        anMode = FM_PORT_AUTONEG_NONE;
        err = ConfigureAnMode(sw, port, anMode, restoreAdminMode);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        portAttr->autoNegMode = anMode;
    }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end updateAnMode */




/*****************************************************************************/
/** ConfigureLoopbackMode
 * \ingroup intPort
 *
 * \desc            Configure the (serdes) loopback Mode on a given port.
 *                  The supported serdes loopback modes are:
 *                  FM_PORT_LOOPBACK_TX2RX (nearLoopback),
 *                  FM_PORT_LOOPBACK_TX2RX (far loopback) and
 *                  FM_PORT_LOOPBAK_OFF (both loopbacks off).
 *                  Fabric loopback is managed separately.
 *                  Note well: fabric loopback takes precedence over serdes
 *                  loopbacks. If it set, this function will return an error.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID
 * 
 * \param[in]       loopbackMode is the desired serdes loopback mode.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureLoopbackMode(fm_int     sw, 
                                       fm_int     port, 
                                       fm_int     loopbackMode)
{
    fm_status         err;
    fm_int            currentLoopbackMode;
    fm10000_port *    portExt;
    fm_portAttr *     portAttr;
    fm_smEventInfo    eventInfo;
    fm10000_portAttr *portAttrExt;


    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d loopbackMode=%d\n",
                     sw, 
                     port, 
                     loopbackMode);

    err         = FM_OK;
    portAttr    = GET_PORT_ATTR( sw, port );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);


    /* validate that fabric loopback is off */

    if (portAttrExt->fabricLoopback != FM_PORT_LOOPBACK_OFF)
    {
        err = FM_ERR_INVALID_STATE;
    }
    else
    {
        /* save current loopback mode and init eventId with an invalid value */
        currentLoopbackMode = portAttr->serdesLoopback;
        eventInfo.eventId = -1;
    
        switch (loopbackMode)
        {
            case FM_PORT_LOOPBACK_RX2TX:
                if (portAttr->serdesLoopback != loopbackMode)
                {
                    eventInfo.eventId = FM10000_PORT_EVENT_CONFIG_REQ;
                    portExt->eventInfo.info.config.ethMode = portAttrExt->ethMode;
                    portExt->eventInfo.info.config.speed = portAttr->speed;
                }
                break;
            case FM_PORT_LOOPBACK_OFF:
                if (portAttr->serdesLoopback == FM_PORT_LOOPBACK_RX2TX)
                {
                    eventInfo.eventId = FM10000_PORT_EVENT_CONFIG_REQ;
                    portExt->eventInfo.info.config.ethMode = portAttrExt->ethMode;
                    portExt->eventInfo.info.config.speed = portAttr->speed;
                }
                else
                {
                    eventInfo.eventId = FM10000_PORT_EVENT_LOOPBACK_OFF_REQ;
                }
                break;
            case FM_PORT_LOOPBACK_TX2RX:
                eventInfo.eventId = FM10000_PORT_EVENT_LOOPBACK_ON_REQ;
                break;
            default:
                err = FM_ERR_INVALID_ARGUMENT;
        }
    
        if (err == FM_OK && eventInfo.eventId != -1)
        {
            /* save the new mode and, if required sent an event to the sm */
            portAttr->serdesLoopback = loopbackMode;
    
            eventInfo.smType         = portExt->smType;
            eventInfo.lock           = FM_GET_STATE_LOCK( sw );
            eventInfo.dontSaveRecord = FALSE;

            /* notify the event */
            portExt->eventInfo.regLockTaken = FALSE;
            err = fmNotifyStateMachineEvent( portExt->smHandle,
                                             &eventInfo,
                                             &portExt->eventInfo,
                                             &port );
        }
        if (err != FM_OK)
        {
            /* restore the previous value */
            portAttr->serdesLoopback = currentLoopbackMode;
        }
    }
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ConfigureLoopbackMode */




/*****************************************************************************/
/** ConfigureFabricLoopback
 * \ingroup intPort
 *
 * \desc            Configure the fabric loopback. The supported modes are:
 *                  FM_PORT_LOOPBACK_TX2RX (fabric loopback ON),
 *                  FM_PORT_LOOPBAK_OFF (fabric loopback off).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID
 * 
 * \param[in]       loopbackMode is the desired fabric loopback mode (ON/OFF)
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureFabricLoopback(fm_int     sw, 
                                         fm_int     port, 
                                         fm_int     loopbackMode)
{
    fm_status         err;
    fm_int            currentFabricLoopback;
    fm10000_port *    portExt;
    fm_portAttr *     portAttr;
    fm_smEventInfo    eventInfo;
    fm10000_portAttr *portAttrExt;
    fm_port          *portPtr;
    fm_int            te;
    fm_switch *       switchPtr;
    fm10000_switch *  switchExt;
    fm_uint64         reg64;
    fm_bool           regLockTaken = FALSE;
    fm_int            switchLoopbackDisable;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d loopbackMode=%d\n",
                     sw, 
                     port, 
                     loopbackMode);

    err         = FM_OK;
    portAttr    = GET_PORT_ATTR( sw, port );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);
    portPtr     = GET_PORT_PTR(sw, port);
    switchPtr   = GET_SWITCH_PTR(sw);
    switchExt   = (fm10000_switch *) switchPtr->extension;

    if (portPtr->portType == FM_PORT_TYPE_TE )
    {
        /* Move below code to fm10000_api_te.c in a function. */
        if (switchExt->tunnelCfg)
        {
            switch (loopbackMode)
            {
                case FM_PORT_LOOPBACK_OFF:
                    switchLoopbackDisable = 1;
                    break;
                case FM_PORT_LOOPBACK_TX2RX:
                    switchLoopbackDisable = 0;
                    break;
                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }
            te = (switchExt->tunnelCfg->tunnelPort[0] == port) ? 0 : 1 ;

            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_TE_CFG(te, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_TE_CFG,
                         SwitchLoopbackDisable,
                         switchLoopbackDisable);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_TE_CFG(te, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->fabricLoopback = loopbackMode;

            FM_FLAG_DROP_REG_LOCK(sw); 
        }
        else
        {
            /* Should'nt have happened */
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else
    {
        /* validate that serdes loopback is off */
        if (portAttrExt->fabricLoopback != loopbackMode)
        {
            /* save current loopback mode and init eventId with an invalid 
             * value */
            currentFabricLoopback = portAttrExt->fabricLoopback;
                                      
            switch (loopbackMode)
            {
                case FM_PORT_LOOPBACK_OFF:
                    eventInfo.eventId = 
                                    FM10000_PORT_EVENT_FABRIC_LOOPBACK_OFF_REQ;
                    break;
                case FM_PORT_LOOPBACK_TX2RX:
                    eventInfo.eventId = 
                                    FM10000_PORT_EVENT_FABRIC_LOOPBACK_ON_REQ;
                    break;
                default:
                    err = FM_ERR_INVALID_ARGUMENT;
            }
        
            if (err == FM_OK)
            {
                /* For PCIe port fabric loopback traffic may have to be 
                 * stopped */            

                /* save the new mode and, if required sent an event to the sm */
                portAttrExt->fabricLoopback = loopbackMode;
        
                eventInfo.smType         = portExt->smType;
                eventInfo.lock           = FM_GET_STATE_LOCK( sw );
                eventInfo.dontSaveRecord = FALSE;

                /* notify the event */
                portExt->eventInfo.regLockTaken = FALSE;
                err = fmNotifyStateMachineEvent( portExt->smHandle,
                                                 &eventInfo,
                                                 &portExt->eventInfo,
                                                 &port );
            }
        
            if (err != FM_OK)
            {
                /* restore the previous value */
                portAttrExt->fabricLoopback = currentFabricLoopback;
            }
        }
    }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ConfigureFabricLoopback */




/*****************************************************************************/
/** ConfigureAnMode
 * \ingroup intPort
 *
 * \desc            Configure the Autonegotiation Mode on a given port
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID
 * 
 * \param[in]       newAnMode is new autonegotiation mode to be set, see
 * 
 * \param[out]      pRestoreAdminMode is a pointer to a caller-allocated
 *                  variable where this function will return TRUE if it has
 *                  switched  port-level state transition table and therefore
 *                  the caller has to restore the current port admin mode. The
 *                  return value will be FALSE otherwise.
 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if autonegotiation mode is invalid
 *                  or pRestoreAdminMode is a NULL pointer.
 * \return          FM_ERR_UNSUPPORTED if autonegotiation mode cannot be set
 *                  for the specified port.
 *                  of a physical port.
 * \return          FM_ERR_PER_LAG_ATTRIBUTE attribute cannot be set on a lag
 *                  logical port.
 *
 *****************************************************************************/
static fm_status ConfigureAnMode( fm_int     sw, 
                                  fm_int     port, 
                                  fm_uint32  newAnMode,
                                  fm_bool *  pRestoreAdminMode)
{
    fm_status            err;
    fm_bool              anReady;
    fm_int               newAnSmType;
    fm_int               newPortSmType;
    fm_uint32            speed;
    fm_switch *          switchPtr;
    fm_port *            portPtr;
    fm_portAttr *        portAttr;
    fm10000_port *       portExt;
    fm10000_portAttr *   portAttrExt;
    fm_smEventInfo       eventInfo;
    fm_uint32            oldSpeed;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d, newAnMode=%d, pRestoreAdminMode=%p\n",
                     sw, 
                     port, 
                     newAnMode,
                     (void*)pRestoreAdminMode);

    if (pRestoreAdminMode == NULL              ||
        (newAnMode != FM_PORT_AUTONEG_NONE      &&
         newAnMode != FM_PORT_AUTONEG_SGMII     &&
         newAnMode != FM_PORT_AUTONEG_CLAUSE_37 &&
         newAnMode != FM_PORT_AUTONEG_CLAUSE_73) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
    
        err         = FM_OK;
        switchPtr   = GET_SWITCH_PTR(sw);
        portPtr     = GET_PORT_PTR(sw, port);
        portExt     = GET_PORT_EXT( sw, port );
        portAttr    = GET_PORT_ATTR( sw, port );
        portAttrExt = GET_FM10000_PORT_ATTR(sw,port);
        *pRestoreAdminMode = FALSE;
    
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegMode);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegMode);
    
    
        /* validate the current base page against the new autoneg mode */
        err = fm10000AnValidateBasePage( sw, 
                                         port,
                                         newAnMode,
                                         portAttr->autoNegBasePage,
                                         &portAttr->autoNegBasePage );
    
        if (err == FM_OK)
        {
            err = fm10000IsPortAutonegReady( sw,
                                             port,
                                             portAttrExt->ethMode,
                                             newAnMode,
                                             &anReady,
                                             &newAnSmType );
        }
    
        if (err == FM_OK)
        {
            /* do we need the port-level AN state machine? */
            if ( anReady == TRUE )
            {
                newPortSmType = FM10000_AN_PORT_STATE_MACHINE;
            }
            else
            {
                newPortSmType = FM10000_BASIC_PORT_STATE_MACHINE;
            }
    
            /* swap on the fly if currently we arean't using the AN state
               machine */
            if ( portExt->smType != newPortSmType )
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT, 
                                 port,
                                 "About to swap state machine: port=%d ethMode=0x%0x curSm=%d newSm=%d\n",
                                 port,
                                 portAttrExt->ethMode,
                                 portExt->smType,
                                 newPortSmType);
    
                err = SwapPortStateMachineType( sw, 
                                                port,
                                                portExt->smType, 
                                                newPortSmType );
                if ( ( err == FM_OK ) && 
                     ( (portAttrExt->ethMode == FM_ETH_MODE_SGMII) ||
                       (newAnMode != FM_PORT_AUTONEG_NONE) ) )
                {
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        
                    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT, 
                                     port,
                                     "After swap: port=%d ethMode=0x%0x\n",
                                     port,
                                     portAttrExt->ethMode );
        
                    /* we switched the port state machine, we need to restore
                       the current ethMode and admin mode */
                    eventInfo.smType = newPortSmType;
                    eventInfo.eventId = FM10000_PORT_EVENT_CONFIG_REQ;
                    eventInfo.lock    = FM_GET_STATE_LOCK( sw );
                    eventInfo.dontSaveRecord = FALSE;
                    portExt->eventInfo.info.config.ethMode = portAttrExt->ethMode;
                    portExt->eventInfo.info.anConfig.autoNegMode = newAnMode;
    
                    if (newAnMode == FM_PORT_AUTONEG_CLAUSE_73)
                    {
                        speed = 0;
                    }
                    else
                    {
                        /* clause 37 & SGMII */
                        speed = 1000;
                    }
    
                    portExt->eventInfo.info.config.speed   = speed;
        
                    /* notify the event */
                    portExt->eventInfo.regLockTaken = FALSE;
                    err = fmNotifyStateMachineEvent( portExt->smHandle,
                                                     &eventInfo,
                                                     &portExt->eventInfo,
                                                     &port );
                    if (err == FM_OK)
                    {
                        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            
                        portExt->ethMode = portAttrExt->ethMode;
                        oldSpeed = portExt->speed;
                        portExt->speed = speed;
                        portAttr->speed = speed;
            
                        if (oldSpeed != portExt->speed)
                        {
                            err = fm10000UpdateAllSAFValues(sw);
                            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
                        }

                        /* tell to restore the admin mode */
                        *pRestoreAdminMode = TRUE;
                    }
                }
    
            }   /* end if ( portExt->smType != newPortSmType ) */
        }
    
        if (err == FM_OK)
        {
            /* restart auto-negotiation if needed */
            err = fm10000AnRestartOnNewConfig( sw,
                                               port,
                                               portAttrExt->ethMode,
                                               newAnMode,
                                               portAttr->autoNegBasePage,
                                               portAttr->autoNegNextPages );
        }
    }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ConfigureAnMode */




/*****************************************************************************/
/** GetRxTermination
 * \ingroup intPort
 *
 * \desc            Get the SERDES termination on the specified port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES on which to operate.
 * 
 * \param[in]       rxTermination is the desired termination, see ''fm_rxTermination''
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetRxTermination(fm_int     sw, 
                                  fm_int     serdes,
                                  fm_rxTermination *rxTermination)
{
    fm_status         err;
    fm10000SerdesRxTerm serdesRxTerm;

    err = fm10000SerdesGetRxTerm(sw, serdes, &serdesRxTerm);

    if (err == FM_OK)
    {
         switch (serdesRxTerm)
         {
            case FM10000_SERDES_RX_TERM_AVDD:
                *rxTermination = FM_PORT_TERMINATION_HIGH;
            break;
            case FM10000_SERDES_RX_TERM_AGND:
                *rxTermination = FM_PORT_TERMINATION_LOW;
            break;
            case FM10000_SERDES_RX_TERM_FLOAT:
                *rxTermination = FM_PORT_TERMINATION_FLOAT;
            break;
        }
    }

    FM_LOG_EXIT_V2( FM_LOG_CAT_SERDES, serdes, err );

}   /* end GetRxTermination */




/*****************************************************************************/
/** ConfigureRxTermination
 * \ingroup intPort
 *
 * \desc            Configure the SERDES termination on the specified port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID on which to operate.
 * 
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate.
 * 
 * \param[in]       rxTermination is the desired termination, see ''fm_rxTermination''
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureRxTermination(fm_int     sw, 
                                        fm_int     port,
                                        fm_int     lane, 
                                        fm_rxTermination rxTermination)
{
    fm_status         err;
    fm10000_port *    portExt;
    fm_laneAttr *     laneAttr;
    fm10000_lane       *pLaneExt;
    fm10000SerdesRxTerm serdesRxTerm;
    fm_int            serdes;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d, lane=%d,rxTermination =%d\n",
                     sw, 
                     port,
                     lane, 
                     rxTermination);

    err         = FM_OK;
    laneAttr    = NULL;
    portExt     = GET_PORT_EXT( sw, port );

    err = fm10000MapPortLaneToSerdes(sw, port, lane, &serdes);

    if (err == FM_OK)
    {
        /* Index to lane attribute is serdes nuber */
        laneAttr    = GET_LANE_ATTR(sw, serdes);
    }

    /* only if new DFE is different from the current one */
    if (err == FM_OK &&
        laneAttr != NULL &&
        laneAttr->rxTermination != rxTermination)
    {
        pLaneExt = GET_LANE_EXT(sw, serdes);
        switch (rxTermination)
        {
            case FM_PORT_TERMINATION_HIGH:
                serdesRxTerm = FM10000_SERDES_RX_TERM_AVDD;
            break;
            case FM_PORT_TERMINATION_LOW:
                serdesRxTerm = FM10000_SERDES_RX_TERM_AGND;
            break;
            case FM_PORT_TERMINATION_FLOAT:
                serdesRxTerm = FM10000_SERDES_RX_TERM_FLOAT;
            break;
            default:
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_PORT, err );
            break;
        }
        err = fm10000SerdesSetRxTerm(sw, serdes, serdesRxTerm);

        if (err == FM_OK)
        {
            pLaneExt->rxTermination  = serdesRxTerm;
            laneAttr->rxTermination = rxTermination;
        }
    }

    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ConfigureRxTermination */





/*****************************************************************************/
/** ConfigureDfeMode
 * \ingroup intPort
 *
 * \desc            Configure the DFE mode on the specified port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID on which to operate.
 * 
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate.
 * 
 * \param[in]       dfeMode is the desired DFE mode, see ''fm_dfeMode''
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureDfeMode(fm_int     sw, 
                                  fm_int     port,
                                  fm_int     lane, 
                                  fm_dfeMode dfeMode)
{
    fm_status         err;
    fm10000_port *    portExt;
    fm_laneAttr *     laneAttr;
    fm_int            serdes;
    fm_smEventInfo    eventInfo;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d, lane=%d, dfeMode=%d\n",
                     sw, 
                     port,
                     lane, 
                     dfeMode);

    err         = FM_OK;
    laneAttr    = NULL;
    portExt     = GET_PORT_EXT( sw, port );

    err = fm10000MapPortLaneToSerdes(sw, port, lane, &serdes);

    if (err == FM_OK)
    {
        /* Index to lane attribute is serdes nuber */
        laneAttr    = GET_LANE_ATTR(sw, serdes);
    }

    /* only if new DFE is different from the current one */
    if (err == FM_OK &&
        laneAttr != NULL &&
        laneAttr->dfeMode != dfeMode)
    {
        /* Filter out unsupported modes
         * FM_DFE_MODE_KR shouldn't be set manually */
        if (dfeMode == FM_DFE_MODE_KR ||
            dfeMode >  FM_DFE_MODE_ICAL_ONLY)
        {
            err = FM_ERR_INVALID_VALUE;
        }
        else
        {
            /* send an event to configure DFE mode */
            eventInfo.eventId = FM10000_PORT_EVENT_CONFIGURE_DFE_REQ;
            eventInfo.smType  = portExt->smType;
            eventInfo.lock    = FM_GET_STATE_LOCK(sw);
            eventInfo.dontSaveRecord = FALSE;
    
            portExt->eventInfo.info.dfe.mode = dfeMode;
            portExt->eventInfo.info.dfe.lane = lane;
    
            portExt->eventInfo.regLockTaken = FALSE;
            err = fmNotifyStateMachineEvent( portExt->smHandle,
                                             &eventInfo,
                                             &portExt->eventInfo,
                                             &port );
            if (err == FM_OK)
            {
                laneAttr->dfeMode = dfeMode;
            }
        }
    }

    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end ConfigureDfeMode */




/*****************************************************************************/
/** ConvertBistSubmode
 * \ingroup intPort
 *
 * \desc            Helper function. Converts given BIST submode to 
 *                  rx-/tx- modes with any custom rx pattern bit sequence, 
 *                  if used.
 *
 * \param[in]       submode is the BIST submode to be converted
 *
 * \param[out]      txSubmode points to caller-allocated storage where this
 *                  function should place the converted tx BIST submode. 
 *
 * \param[out]      rxSubmode points to caller-allocated storage where this
 *                  function should place the converted rx BIST submode. 
 *
 * \param[out]      rxCustomPattern0 points to caller-allocated storage where
 *                  this function should place the lower 40 bits of the rx bit
 *                  sequence, if the rx submode is one of the modes requiring
 *                  custom pattern. In the case of 80 bit pattern, only bits
 *                  0..39 are relevant.
 * 
 * \param[out]      rxCustomPattern1 points to caller-allocated storage where
 *                  this function should place the upper 40 bits of the rx bit
 *                  sequence, if the rx submode is one of the modes requiring
 *                  custom pattern. This is only meanful in the case of 80 bit
 *                  test patterns. Only bits 0..39 are relevant.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if a pointer argument is NULL
 * \return          FM_ERR_INVALID_SUBMODE for invalid BIST submodes
 * \return          FM_ERR_UNSUPPORTED submode is valid but unsupported
 *
 *****************************************************************************/
static fm_status ConvertBistSubmode(fm_int      submode,
                                    fm_int     *txSubmode,
                                    fm_int     *rxSubmode,
                                    fm_uint64  *rxCustomPattern0,
                                    fm_uint64  *rxCustomPattern1)
{     
    fm_status   err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "submode=%d, txSubmode=%p, rxSubmode=%p, rxCustomPattern0=%p, rxCustomPattern1=%p\n",
                 submode,
                 (void *) txSubmode,
                 (void *) rxSubmode,
                 (void *) rxCustomPattern0,
                 (void *) rxCustomPattern1);

    if ( (txSubmode == NULL) || (rxSubmode == NULL) || (rxCustomPattern0 == NULL) || (rxCustomPattern1 == NULL))
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
    }

    *rxSubmode = FM_BIST_MAX;
    *txSubmode = FM_BIST_MAX;
    *rxCustomPattern0 = 0;
    *rxCustomPattern1 = 0;
    switch (submode)
    {
        /* tx only submodes */
        case FM_BIST_TX_PRBS_128:
        case FM_BIST_TX_PRBS_32K:
        case FM_BIST_TX_PRBS_8M:
        case FM_BIST_TX_PRBS_2G:
        case FM_BIST_TX_PRBS_2048:
        case FM_BIST_TX_PRBS_512B:
        case FM_BIST_TX_IDLECHAR:
        case FM_BIST_TX_TESTCHAR:
        case FM_BIST_TX_LOWFREQ:
        case FM_BIST_TX_HIGHFREQ:
            *txSubmode = submode;
            break;

        case FM_BIST_TX_SQUARE8:
            *txSubmode = FM_BIST_TX_CUSTOM80;
            *rxCustomPattern0 = BIST_PATTERN_SQUARE8_0;
            *rxCustomPattern1 = BIST_PATTERN_SQUARE8_1;
            break;

        case FM_BIST_TX_SQUARE10:
            *txSubmode = FM_BIST_TX_CUSTOM20;
            *rxCustomPattern0 = BIST_PATTERN_SQUARE10;
            break;

        case FM_BIST_TX_CUSTOM10:
        case FM_BIST_TX_CUSTOM20:
        case FM_BIST_TX_CUSTOM40:
        case FM_BIST_TX_CUSTOM80:
        case FM_BIST_TX_PRBS_1024:
        case FM_BIST_TX_PRBS_512A:
        case FM_BIST_TX_CJPAT:
            err = FM_ERR_UNSUPPORTED;
            break;

        /* rx only submodes */
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
            *rxSubmode = submode;
            if (submode == FM_BIST_RX_IDLECHAR)
            {
                *rxCustomPattern0 = BIST_PATTERN_IDLECHAR;
            }
            else if (submode == FM_BIST_RX_TESTCHAR)
            {
                *rxCustomPattern0 = BIST_PATTERN_TESTCHAR;
            }
            else if (submode == FM_BIST_RX_LOWFREQ)
            {
                *rxCustomPattern0 = BIST_PATTERN_LOWFREQ;
            }
            else if (submode == FM_BIST_RX_HIGHFREQ)
            {
                *rxCustomPattern0 = BIST_PATTERN_HIGHFREQ;
            }
            break;

        case FM_BIST_RX_SQUARE8:
            *rxSubmode = FM_BIST_RX_CUSTOM80;
            *rxCustomPattern0 = BIST_PATTERN_SQUARE8_0;
            *rxCustomPattern1 = BIST_PATTERN_SQUARE8_1;
            break;

        case FM_BIST_RX_SQUARE10:
            *rxSubmode = FM_BIST_RX_CUSTOM20;
            *rxCustomPattern0 = BIST_PATTERN_SQUARE10;
            break;

        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_RX_PRBS_512A:
        case FM_BIST_RX_CJPAT:
            err = FM_ERR_UNSUPPORTED;
            break;

        /* txrx submodes */
        case FM_BIST_TXRX_PRBS_128:
            *txSubmode = FM_BIST_TX_PRBS_128;
            *rxSubmode = FM_BIST_RX_PRBS_128;
            break;
        case FM_BIST_TXRX_PRBS_32K:
            *txSubmode = FM_BIST_TX_PRBS_32K;
            *rxSubmode = FM_BIST_RX_PRBS_32K;
            break;
        case FM_BIST_TXRX_PRBS_8M:
            *txSubmode = FM_BIST_TX_PRBS_8M;
            *rxSubmode = FM_BIST_RX_PRBS_8M;
            break;
        case FM_BIST_TXRX_PRBS_2G:
            *txSubmode = FM_BIST_TX_PRBS_2G;
            *rxSubmode = FM_BIST_RX_PRBS_2G;
            break;
        case FM_BIST_TXRX_PRBS_2048:
            *txSubmode = FM_BIST_TX_PRBS_2048;
            *rxSubmode = FM_BIST_RX_PRBS_2048;
            break;
        case FM_BIST_TXRX_PRBS_512B:
            *txSubmode = FM_BIST_TX_PRBS_512B;
            *rxSubmode = FM_BIST_RX_PRBS_512B;
            break;
        case FM_BIST_TXRX_IDLECHAR:
            *txSubmode = FM_BIST_TX_IDLECHAR;
            *rxSubmode = FM_BIST_RX_IDLECHAR;
            *rxCustomPattern0 = BIST_PATTERN_IDLECHAR;
            break;
        case FM_BIST_TXRX_TESTCHAR:
            *txSubmode = FM_BIST_TX_TESTCHAR;
            *rxSubmode = FM_BIST_RX_TESTCHAR;
            *rxCustomPattern0 = BIST_PATTERN_TESTCHAR;
            break;
        case FM_BIST_TXRX_LOWFREQ:
            *txSubmode = FM_BIST_TX_LOWFREQ;
            *rxSubmode = FM_BIST_RX_LOWFREQ;
            *rxCustomPattern0 = BIST_PATTERN_LOWFREQ;
            break;
        case FM_BIST_TXRX_HIGHFREQ:
            *txSubmode = FM_BIST_TX_HIGHFREQ;
            *rxSubmode = FM_BIST_RX_HIGHFREQ;
            *rxCustomPattern0 = BIST_PATTERN_HIGHFREQ;
            break;

        case FM_BIST_TXRX_SQUARE8:
            *txSubmode = FM_BIST_TX_CUSTOM80;
            *rxSubmode = FM_BIST_RX_CUSTOM80;
            *rxCustomPattern0 = BIST_PATTERN_SQUARE8_0;
            *rxCustomPattern1 = BIST_PATTERN_SQUARE8_1;
            break;

        case FM_BIST_TXRX_SQUARE10:
            *txSubmode = FM_BIST_TX_CUSTOM20;
            *rxSubmode = FM_BIST_RX_CUSTOM20;
            *rxCustomPattern0 = BIST_PATTERN_SQUARE10;
            break;

        /* currently unsupported */
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM80:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TXRX_CJPAT:
            err = FM_ERR_UNSUPPORTED;
            break;

        /* invalid submode */
        default:
            err = FM_ERR_INVALID_SUBMODE;
            break;

    }   /* end switch (submode) */

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end ConvertBistSubmode */




/*****************************************************************************/
/** IsBistChanging
 * \ingroup intPort
 *
 * \desc            Helper function. Checks if a new submode maps to any 
 *                  change on the ports BIST tx-/rx-configuration.
 *
 * \param[in]       sw is the switch on which to operate. 
 *
 * \param[in]       port is the logical port on which to operate. 
 *
 * \param[in]       submode is the BIST submode to be compared
 *
 * \return          TRUE if BIST configuration changes or port not in BIST mode
 * \return          FALSE otherwise
 *
 *****************************************************************************/
static fm_bool IsBistChanging(fm_int sw,
                              fm_int port,
                              fm_int submode)
{
    fm_status    err;
    fm_port     *portPtr;
    fm_int       rxPortSubmode, txPortSubmode;
    fm_int       rxSubmode, txSubmode;
    fm_uint64    customData0;
    fm_uint64    customData1;
    fm_bool      submodeChanging;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    port,
                    "sw=%d, port=%d, submode=%d\n",
                    sw,
                    port,
                    submode);

    portPtr = GET_PORT_PTR(sw, port);

    /* Port mode/state check */
    if (portPtr->mode != FM_PORT_MODE_BIST)
    {
        FM_LOG_EXIT_CUSTOM_V2(FM_LOG_CAT_PORT,
                              port,
                              TRUE,
                              "Mode changing\n");
    }

    /* Submode check */
    err = ConvertBistSubmode(submode, &txSubmode, &rxSubmode, &customData0, &customData1);
    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                        port,
                        "Unable to convert submode: %s\n",
                        fmErrorMsg(err) );
        return FALSE;
    }

    err = ConvertBistSubmode(portPtr->submode, &txPortSubmode, &rxPortSubmode, &customData0, &customData1);

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                        port,
                        "Unable to convert portPtr->submode: %s\n",
                        fmErrorMsg(err) );
        return FALSE;
    }

    submodeChanging = !( (txPortSubmode == txSubmode) && (rxPortSubmode == rxSubmode) );

    FM_LOG_EXIT_CUSTOM_V2(FM_LOG_CAT_PORT, 
                          port,
                          submodeChanging,
                          "Submode %s\n",
                          (submodeChanging) ? "changing" : "not changing");

}   /* end IsBistChanging */




/*****************************************************************************/
/** IsBistRxSubmode
 * \ingroup intPort
 *
 * \desc            Helper function. Checks if the BIST submode is one of the
 *                  rx-enabled submodes
 *
 * \param[in]       submode is the BIST submode to be checked
 *
 * \return          TRUE if BIST rx comparator enabled for the submode
 * \return          FALSE otherwise
 *
 *****************************************************************************/
static fm_bool IsBistRxSubmode(fm_int submode)
{
    switch (submode)
    {
        /* rx only submodes */
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
        case FM_BIST_RX_SQUARE8:
        case FM_BIST_RX_SQUARE10:
        case FM_BIST_RX_MIXEDFREQ:
        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_RX_PRBS_512A:
        /* txrx submodes */
        case FM_BIST_TXRX_PRBS_128:
        case FM_BIST_TXRX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_2G:
        case FM_BIST_TXRX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_512B:
        case FM_BIST_TXRX_IDLECHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_TXRX_LOWFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
        case FM_BIST_TXRX_SQUARE8:
        case FM_BIST_TXRX_SQUARE10:
        case FM_BIST_TXRX_MIXEDFREQ:
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM80:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TXRX_CJPAT:
            return TRUE;
        /* invalid submode */
        default:
            return FALSE;
    }   /* end switch (submode) */

}   /* end IsBistRxSubmode */




/*****************************************************************************/
/** UpdateSAFValuesForCardinalPort
 * \ingroup intPort
 *
 * \desc            Called whenever port speeds change or the user requests a
 *                  change to the SAF values for a port, this compares port
 *                  speeds to determine which port pairs must be set to
 *                  store-and-forward.  All of these port pairs, plus those
 *                  requested by the user, are set to store-and-forward mode.
 *                  Other port pairs are set to cut-through mode.  Although
 *                  'entry' could be looked up by this function using the 'sw'
 *                  and 'port' arguments, it is passed in as an optimization.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       srcPortCpi is the cardinal port number for the source port.
 *
 * \param[in]       schedPortSpeeds is the scheduler port speeds of all ports.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateSAFValuesForCardinalPort(fm_int    sw,
                                                fm_int    srcPortCpi,
                                                fm_int  * schedPortSpeeds)
{
    fm_int             dstPortCpi;
    fm_status          err;
    fm_uint64          rv;
    fm_uint64          reg64;
    fm_int             srcPort;
    fm_int             destPort;
    fm_int             srcPhysPort;
    fm_int             destPhysPort;
    fm_portAttr *      destPortAttr;
    fm_portAttr *      portAttr;
    fm_switch *        switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d srcPortCpi=%d schedPortSpeeds=%p\n",
                 sw,
                 srcPortCpi,
                 (void *)schedPortSpeeds);

    switchPtr = GET_SWITCH_PTR(sw);
    srcPort   = GET_LOGICAL_PORT(sw, srcPortCpi);
    portAttr  = GET_PORT_ATTR(sw, srcPort);
    err = fmMapLogicalPortToPhysical(switchPtr, srcPort, &srcPhysPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, err);

    if (portAttr->enableRxCutThrough)
    {
        rv = 0;

        for (dstPortCpi = 0 ;
             dstPortCpi < switchPtr->numCardinalPorts ;
             dstPortCpi++)
        {
            err = fmMapCardinalPortInternal(switchPtr, 
                                            dstPortCpi, 
                                            &destPort, 
                                            &destPhysPort);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, err);

            if (srcPortCpi != dstPortCpi)
            {
                /* Turn off cut-through if speed requirement is not satisfied */
                if (schedPortSpeeds[srcPortCpi] < schedPortSpeeds[dstPortCpi])
                {
                    FM_SET_UNNAMED_FIELD64(rv, destPhysPort, 1, 1);
                }
            }

            /* Turn off tx cut-through if disabled  */
            destPortAttr  = GET_PORT_ATTR(sw, destPort);
            if (!destPortAttr->enableTxCutThrough)
            {
                FM_SET_UNNAMED_FIELD64(rv, destPhysPort, 1, 1);
            }
        }
    }
    else
    {
        rv = FM10000_PORT_ALL_SAF_ENABLED;
    }

    err = fmRegCacheReadUINT64(sw, &fm10000CacheSafMatrix, srcPhysPort, &reg64);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_SET_FIELD64(reg64, 
                   FM10000_SAF_MATRIX, 
                   EnableSNF, 
                   rv);

    /* Set the CutThruMode to One Segment SnF because Full CutThruModfe (0) is
     * a debug mode only. */
    FM_SET_FIELD64(reg64, 
                   FM10000_SAF_MATRIX, 
                   CutThruMode, 
                   1);

    err = fmRegCacheWriteUINT64(sw, &fm10000CacheSafMatrix, srcPhysPort, reg64);

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end UpdateSAFValuesForCardinalPort */




/*****************************************************************************/
/** SwapPortStateMachineType
 * \ingroup intPort
 *
 * \desc            Helper function to swap port-level state machine type
 *
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       port is the port on which to operate
 * 
 * \param[in]       curType is the current state machine type
 * 
 * \param[in]       newType is the new state machine type
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SwapPortStateMachineType( fm_int sw,
                                           fm_int port,
                                           fm_int curType,
                                           fm_int newType )
{
    fm_status       status;
    fm10000_port   *portExt;
    fm_smEventInfo  eventInfo;

    status = FM_OK;
    portExt = GET_PORT_EXT( sw, port );

    /* Proceeed if the new port-level state transition table is now known */
    if ( newType != FM_SMTYPE_UNSPECIFIED )
    {
        /* do we need to change the state transition table? */
        if ( newType != curType )
        {
            /* yes, then... */
            if ( curType != FM_SMTYPE_UNSPECIFIED )
            {
                /* ...notify the DISABLE event to the port state machine... */
                eventInfo.smType = curType;
                eventInfo.eventId = FM10000_PORT_EVENT_DISABLE_REQ;
                eventInfo.lock    = FM_GET_STATE_LOCK( sw );
                eventInfo.dontSaveRecord = FALSE;
                portExt->eventInfo.info.config.ethMode = FM_ETH_MODE_DISABLED;
                portExt->eventInfo.info.config.speed   = 0;
                portExt->eventInfo.regLockTaken = FALSE;
                status = fmNotifyStateMachineEvent( portExt->smHandle,
                                                    &eventInfo,
                                                    &portExt->eventInfo,
                                                    &port );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

                portExt->ethMode = FM_ETH_MODE_DISABLED;
                portExt->speed   = 0;

                /* ...then stop it, then... */
                status = fmStopStateMachine( portExt->smHandle );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
            }

            /* ...start it with the new type */
            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "Binding Port State Transition Table Type %d "
                             "to Port %d's State Machine\n",
                             newType,
                             port );

            /* then start it with the new type */
            portExt->smType = FM_SMTYPE_UNSPECIFIED;
            status = fmStartStateMachine( portExt->smHandle,
                                          newType,
                                          FM10000_PORT_STATE_DISABLED );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            /* finally restart it and reconfigure it */
            portExt->smType   = newType;

        } /* end if ( newType != curType ) */

    }   /* end if ( newType != FM_SMTYPE_UNSPECIFIED ) */

ABORT:
    return status;

}   /* end SwapPortStateMachineType */




/*****************************************************************************
 * HandleEgressTimeStamp
 * \ingroup intPort
 *
 * \desc            Processes an egress timestamp interrupt.
 *
 * \note            This is an internal function. The caller is assumed to have
 *                  claimed the state lock (STATE_LOCK) prior to calling this
 *                  function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in[       port is the logical port on which to operate.
 *
 * \param[in]       epl is the index of the Ethernet Port Logic block.
 *
 * \param[in]       channel is the index of the PCS channel.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandleEgressTimeStamp(fm_int   sw,
                                       fm_int   port,
                                       fm_int   epl,
                                       fm_int   channel)
{
    fm_switch *         switchPtr;
    fm_uint64           globalSystime;
    fm_uint32           globalSystimeLow32;
    fm_uint32           globalSystimeHigh32;
    fm_uint64           mac_1588_status;
    fm_uint32           rawIngressTime;
    fm_uint32           rawEgressTime;
    fm_uint64           rawIngressTime64;
    fm_uint64           rawEgressTime64;
    fm_event *          eventPtr = NULL;
    fm_status           status;
    fm_int              lagPort;
    fm_uint32           dglort;
    fm_uint32           sglort;
    fm10000_switch *    switchExt;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    
    status = switchPtr->ReadUINT64(sw,
                                   FM10000_SYSTIME(0),
                                   &globalSystime);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

    /* Retrieve the ingress and egress timestamps. */
    status = switchPtr->ReadUINT64(sw,
                                   FM10000_MAC_1588_STATUS(epl, channel, 0),
                                   &mac_1588_status);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

    rawIngressTime = FM_GET_FIELD64(mac_1588_status,
                                    FM10000_MAC_1588_STATUS,
                                    IngressTimeStamp);

    rawEgressTime = FM_GET_FIELD64(mac_1588_status,
                                   FM10000_MAC_1588_STATUS,
                                   EgressTimeStamp);

    /* Clear the latched values. */
    mac_1588_status = 0;
    status = switchPtr->WriteUINT64(sw,
                                    FM10000_MAC_1588_STATUS(epl, channel, 0),
                                    mac_1588_status);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

    /* Calculate 64-bit time */
    globalSystimeLow32  = globalSystime;
    globalSystimeHigh32 = (globalSystime >> 32);

    if (globalSystimeLow32 < rawEgressTime)
    {
        globalSystimeHigh32--;        
    }
    rawEgressTime64  = globalSystimeHigh32;
    rawEgressTime64  = ( (rawEgressTime64 << 32) | (rawEgressTime) );

    if (rawEgressTime < rawIngressTime)
    {
        /* In this case last 32 bit of egress time is overlapped 
           after ingress timestamp. */
        globalSystimeHigh32--;
    }
    rawIngressTime64 = globalSystimeHigh32;
    rawIngressTime64 = ( (rawIngressTime64 << 32) | (rawIngressTime) );

    if (switchExt->ethTimestampsOwnerPort != -1)
    {
        lagPort = fmGetLAGForPort(sw, port); 
        if (lagPort != -1)
        {
            port = lagPort;
        }

        status = fmGetLogicalPortGlort(sw,
                                       port,
                                       &dglort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

        status = fmGetLogicalPortGlort(sw,
                                       switchExt->ethTimestampsOwnerPort,
                                       &sglort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

        status = fmDeliverPacketTimestamp(sw,
                                          sglort,
                                          dglort,
                                          rawEgressTime64,
                                          rawIngressTime64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

#if 0
        /* For debugging */
        FM_LOG_PRINT("To mailbox: destination logical port=%d rawIngress=0x%016llx "
                     "rawEgress=0x%016llx dglort %d\n", 
                     switchExt->ethTimestampsOwnerPort,
                     rawIngressTime64, 
                     rawEgressTime64, 
                     dglort);
#endif
    }

ABORT:

    /* Release the event if sending it to the application failed */
    if ( (status != FM_OK) && (eventPtr != NULL) )
    {
        fmReleaseEvent(eventPtr);
    }

    return status;

}   /* end HandleEgressTimeStamp */




/*****************************************************************************
 * HandleEeePcSilent
 * \ingroup intPort
 *
 * \desc            Processes an EEE Port Channel Silent interrupt.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in[       port is the logical port on which to operate.
 *
 * \param[in]       epl is the index of the Ethernet Port Logic block.
 *
 * \param[in]       lane is the relative ID of the lane on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandleEeePcSilent(fm_int   sw,
                                  fm_int   port,
                                  fm_int   epl,
                                  fm_int   lane)
{
    fm_switch *       switchPtr;
    fm10000_portAttr *portAttrExt;
    fm10000_port *    portExt;
    fm_smEventInfo    eventInfo;
    fm_uint32         addr;
    fm_uint32         portStatus;
    fm_bool           eeePortSilentStatus;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_status         status=FM_OK;

    switchPtr   = GET_SWITCH_PTR(sw);
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    if ( (( portAttrExt->negotiatedEeeModeEnabled == TRUE ) &&
         !( portAttrExt->dbgEeeMode & FM10000_EEE_DBG_DISABLE_TX )) ||
         ( portAttrExt->dbgEeeMode & FM10000_EEE_DBG_ENABLE_TX ) )
    {
        /* EEE Enabled */

        addr = FM10000_PORT_STATUS( epl, lane );

        /* read the port status and extract the relevant fields */
        status = switchPtr->ReadUINT32( sw, addr, &portStatus );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        eeePortSilentStatus = FM_GET_BIT( portStatus, 
                                          FM10000_PORT_STATUS, 
                                          EeeSlPcSilent );

        if ( eeePortSilentStatus )
        {
            /* The port is Idle */
            eventInfo.smType = portExt->smType;
            eventInfo.lock   = FM_GET_STATE_LOCK( sw );
            eventInfo.dontSaveRecord = TRUE;

            eventInfo.eventId = FM10000_PORT_EVENT_EEE_SILENT_IND;
            portExt->eventInfo.regLockTaken = FALSE;
            status = fmNotifyStateMachineEvent( portExt->smHandle,
                                                &eventInfo,
                                                &portExt->eventInfo,
                                                &port );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

            /* Increment diagnostic counter */
            portAttrExt->eeePcSilentCnt++;
        }
        else
        {
            /* Traffic has resumed on the port; i.e. transmiter is
               no longer in LPI mode */
            addr = FM10000_MAC_CFG(epl, lane, 0);

            status = switchPtr->ReadUINT32Mult(sw,
                                               addr,
                                               FM10000_MAC_CFG_WIDTH,
                                               macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

            /* Clear the LPI Request bit */
            FM_ARRAY_SET_BIT( macCfg, 
                              FM10000_MAC_CFG, 
                              TxLpIdleRequest, 
                              FALSE );

            status = switchPtr->WriteUINT32Mult( sw,
                                                 addr,
                                                 FM10000_MAC_CFG_WIDTH,
                                                 macCfg );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            /* Increment diagnostic counter */
            portAttrExt->eeePcActiveCnt++;
        }
    }
    else 
    {
        /* Increment diag counter: received interrupt while EEE is disabled */
        portAttrExt->eeePcSilentDisabledCnt++;
    }

ABORT:
    return status;

}   /* end HandleEeePcSilent */




/*****************************************************************************
 * SetSerDesSmType
 * \ingroup intPort
 *
 * \desc            Set the SerDes State Machine type based on the current
 *                  SerDes operational mode
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in[       port is the logical port on which to operate.
 *
 * \param[out]      smType it the pointer to a caller allocated variable where
 *                  this function will return the State Machine Type
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetSerDesSmType( fm_int sw, fm_int port, fm_int *smType )
{
    fm_status            err;
    fm_int               serDesLane0;
    fm10000_serDesSmMode serdesSmMode;

    err = fm10000MapPortLaneToSerdes(sw, port, 0, &serDesLane0);
    if( err == FM_OK )
    {
        err = fm10000SerdesGetOpMode( sw, 
                                      serDesLane0, 
                                      NULL, 
                                      &serdesSmMode, 
                                      NULL );
        if( err == FM_OK )
        {
            if( serdesSmMode == FM10000_SERDES_USE_STUB_STATE_MACHINE )
            {
                *smType = FM10000_STUB_SERDES_STATE_MACHINE;
            }
            else
            {
                *smType = FM10000_BASIC_SERDES_STATE_MACHINE;
            }
        }
    }

    return err;

}   /* end SetSerDesSmType() */



/*****************************************************************************/
/** ConvertPpmToTxClkCompensationTimeout
 * \ingroup intPort
 *
 * \desc            This function takes a value expressed in ppm and converts
 *                  it to MAC_CFG.TxClkCompensationTimeout unit.
 *
 * \param[in]       pcsType is the PCS type.
 *
 * \param[in]       speed is the line speed, to be used with PCS modes that
 *                  could be run at different speeds
 * 
 * \param[in]       num_ppm is the number of ppm to be converted.
 *
 * \return          the timeout value to be programmed in the
 *                  MAC_CFG.TxClkCompensationTimeout
 *
 *****************************************************************************/
static fm_uint16 ConvertPpmToTxClkCompensationTimeout( fm_uint32  pcsType,
                                                       fm_uint32  speed,
                                                       fm_uint32  num_ppm )
{
    fm_float  f;             /* SerDes data rate in baud/sec.              */
    fm_float  ppm = 1E-6;    /* clock deviation in parts per million (ppm) */
    fm_float  e;             /* encoding compression factor                */
    fm_float  c;             /* number of MII columns processed per tick   */
    fm_float  T;             /* tick period in sec.                        */
    fm_float  d;             /* clock deviation                            */
    fm_float  timeout;       /* TxClockCompensationTimeout value           */
    fm_float  default40Gppm;
    fm_float  default100Gppm;
    fm_uint16 txClockCompensationTimeout;

    /**************************************************
     * 40GBASE-R and 100GBASE-R PCS types need clock
     * compensation enabled by default to compensate
     * for alignment marker insertion by the TX PCS.
     * If num_ppm is 0 then a value of 8192 needs to be
     * programmed into MAC_CFG.txClockCompensationTimeout.
     *
     * For other PCS types if num_ppm is 0 then the
     * Tx clock compensation circuit must be disabled.
     **************************************************/

    if (num_ppm == 0 || pcsType == FM10000_PCS_SEL_DISABLE)
    {
        if ( pcsType == FM10000_PCS_SEL_40GBASER )
        {
            return 4*2048;
        }
        else if ( pcsType == FM10000_PCS_SEL_100GBASER )
        {
            /* Same value for 40G and 100G? */
            return 20*2048;
        }
        else
        {
            /* Will disable the tx clock compensation circuit */
            return 0;
        }
    }

    /**************************************************
     * Select the SerDes data rate in baud/sec based
     * on PCS type. It is the data rate at the
     * differential pair (analog) side of the SerDes.
     *
     * The SerDes operates at
     *    1.25E9 baud/s for 1000BASE-X and SGMII
     *  10.312E9 baud/s for 10GBASE-R and 40GBASE-R
     *  25.78125 baud/s for 100GBASE-R
     *
     * Select tick period in sec according to the
     * FM10000 datasheet.
     * See MAC_CFG.txClockCompensationTimeout:
     *
     * 100GBASER   count unit is 1.28 ns
     * 40GBASER    count unit is 3.2 ns
     * 10GBASER    count unit is 3.2 ns
     * 1000BASEX   count unit is 8 ns
     * SGMII 1000  count unit is 8 ns
     * SGMII 100   count unit is 80 ns
     * SGMII 10    count unit is 800 ns
     **************************************************/

    switch (pcsType)
    {
        case FM10000_PCS_SEL_SGMII_10:
            f = 1.25E9 / 100;
            T = 800.0E-9;
            break;

        case FM10000_PCS_SEL_SGMII_100:
            f = 1.25E9 / 10;
            T = 80.0E-9;
            break;

        case FM10000_PCS_SEL_SGMII_1000:
        case FM10000_PCS_SEL_1000BASEX:
            if ( speed == 1000 )
            {
                /* 1G */
                f = 1.25E9;
                T = 8.0E-9;
            }
            else
            {
                /* 2.5G */
                f = 3.125E9;
                T = 3.2E-9;
            }
            break;

        case FM10000_PCS_SEL_100GBASER:
            f = 25.78125E9;
            T = 1.28E-9;
            break;

        case FM10000_PCS_SEL_10GBASER:
        case FM10000_PCS_SEL_40GBASER:
        default:
            f = 10.3125E9;
            T = 3.2E-9;
            break;
    }

    /**************************************************
     * Select the encoding compression factor based on
     * the PCS type.
     *
     *  64/66 for <S>BASE-R
     *   8/10 for <S>BASE-X
     *
     *  SGMII runs on top of 1000BASE-X
     **************************************************/
    if ( pcsType == FM10000_PCS_SEL_10GBASER ||
         pcsType == FM10000_PCS_SEL_40GBASER ||
         pcsType == FM10000_PCS_SEL_100GBASER )
    {
        /* Is this correct for 100G? */
        e = 64.0 / 66.0;
    }
    else
    {
        e = 8.0 / 10.0;
    }

    /* Number of MII columns processed per tick (also for 100G) */
    if ( pcsType == FM10000_PCS_SEL_40GBASER ||
         pcsType == FM10000_PCS_SEL_100GBASER )
    {
        c = 2;
    }
    else
    {
        c = 1;
    }

    /**************************************************
     * For 40GBASE-R and 20GBASE-R PCS types the
     * num_ppm must be set on top of the minimal ppm
     * value required.
     **************************************************/

    if (pcsType == FM10000_PCS_SEL_40GBASER)
    {
        /* Convert the default timeout in pmm */
        default40Gppm = c / (T * f * 4*2048 * (e / 8.0));
        ppm = (num_ppm * ppm) + default40Gppm;
    }
    else if ( pcsType == FM10000_PCS_SEL_100GBASER )
    {
        /* Convert the default timeout in pmm */
        default100Gppm = c / (T * f * 20 * 2048 * (e / 8.0));
        ppm = (num_ppm * ppm) + default100Gppm;
    }
    else
    {
        ppm = ppm * num_ppm;
    }

    /* Compute the clock deviation in baud/s. */
    d = ppm * f;

    /* Convert the clock deviation to bytes/s.
       Note: The PCS encoding scheme needs to be taken into account here as the
             clock compensation operates on decoded bytes.
    */
    d = d * (e / 8.0);

    /* Compute the periodic rate at which a single Idle byte should be deleted. */
    timeout = (c / T) / d;

    /* MAC_CFG.txClockCompensationTimeout is 16-bit long */
    if (timeout > 0xFFFF)
    {
        txClockCompensationTimeout = 0xFFFF;
    }
    else
    {
        txClockCompensationTimeout = timeout;
    }

    FM_LOG_DEBUG2(FM_LOG_CAT_PORT,
                 "f %f ppm %f c %f e %f T %f ns timeout %d\n",
                 f,
                 ppm/1E-6,
                 c,
                 e,
                 T/1e-9,
                 txClockCompensationTimeout);

    return txClockCompensationTimeout;

}   /* end ConvertPpmToTxClkCompensationTimeout */



/*****************************************************************************/
/** CountOnes
 *
 * \desc            Helper function to count the number of 1's
 *                  in a given fm_uint64 variable.
 *
 * \param[in]       value is the variable whose bits are to be counted.
 *
 * \return          The number of 1's in the argument.
 *
 *****************************************************************************/
static fm_int CountOnes(fm_uint64 value)
{
    fm_int      i;
    fm_int      count;

    count = 0;

    for (i = 0 ; i < 64 ; i++)
    {
        if ( value & 0x1 )
        {
            count++;
        }
        value = value >> 1;
    }

    return count;

}   /* end CountOnes */



/*****************************************************************************/
/** GetPortLpiStatus
 * \ingroup intPort
 *
 * \desc            NOT DOCUMENTED.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[out]      lpiStatus is NOT DOCUMENTED.
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status GetPortLpiStatus(fm_int sw, fm_int  port, fm_int *lpiStatus)
{
    fm_status     status;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_int        epl;
    fm_int        physLane;
    fm_uint32     portStatus;

    switchPtr = GET_SWITCH_PTR( sw );
    portExt   = GET_PORT_EXT( sw, port );

    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

    /* Read the port status and extract the relevant fields */
    status = switchPtr->ReadUINT32( sw, 
                                    FM10000_PORT_STATUS( epl, physLane ), 
                                    &portStatus );

    *lpiStatus = FM_PORT_EEE_NORMAL;

    if ( FM_GET_BIT( portStatus, FM10000_PORT_STATUS, RxLpIdle ) == 1)
    {
        *lpiStatus |= FM_PORT_EEE_RX_LPI;
    }

    if ( FM_GET_BIT( portStatus, FM10000_PORT_STATUS, TxLpIdle ) == 1 )
    {
        *lpiStatus |= FM_PORT_EEE_TX_LPI;
    }

    return status;

}   /* end GetPortLpiStatus */



/*****************************************************************************/
/** GetPcieSpeedFromReg
 * \ingroup intPort
 *
 * \desc            Read base PCIE clock speed directly from 
 *                  PCIE_CFG_PCIE_LINK_CTRL register
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[out]      speed points to caller-allocated storage where this
 *                  function should place the PCIE speed value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid (not pcie).
 * \return          FM_ERR_INVALID_STATE if the PEP is in reset or is
 *                  not enabled.
 * \return          FM_ERR_UNSUPPORTED if unrecognized clock speed.
 *
 *****************************************************************************/
fm_status GetPcieSpeedFromReg(fm_int sw, fm_int port, fm_uint32 *speed)
{
    fm_status      status;
    fm_uint32      addr;
    fm_uint32      reg;
    fm_uint32      regField;
    fm_int         pep;
    fm_bool        isPciePort;
    fm_port      * portPtr;
    fm10000_port * portExt;

    portPtr = GET_PORT_PTR(sw, port);
    status  = fm10000IsPciePort(sw, port, &isPciePort);
    FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    /* Allow only PCIE physical port belonging to PEP or CPU port */    
    if ( ! ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL ||
              portPtr->portType == FM_PORT_TYPE_CPU) && isPciePort ) )
    {
        status = FM_ERR_INVALID_PORT;
        FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }

    portExt = GET_PORT_EXT(sw, port);
    pep     = portExt->endpoint.pep;
    addr    = FM10000_PCIE_CFG_PCIE_LINK_CTRL();


    /* Read the link status and control register */
    status = fm10000ReadPep( sw, addr, pep, &reg );
    FM_LOG_EXIT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* determine the lane speed */
    regField = FM_GET_FIELD( reg, 
                             FM10000_PCIE_CFG_PCIE_LINK_CTRL, 
                             CurrentLinkSpeed );
    switch ( regField )
    {
        case 1:
            *speed = FM_PORT_PCIE_SPEED_2500;
            break;

        case 2:
            *speed = FM_PORT_PCIE_SPEED_5000;
            break;

        case 3:
            *speed = FM_PORT_PCIE_SPEED_8000;
            break;

        default:
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                             port, 
                             "Invalid CurrentLinkSpeed value %d on port %d. LINK_CTRL 0x%x\n",
                             regField,
                             port,
                             reg );
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_EXIT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
            break;
    }

    return status;
}




/*****************************************************************************/
/** GetPcieModeFromReg
 * \ingroup intPort
 *
 * \desc            Read PCIE link width directly from PCIE_CFG_PCIE_LINK_CTRL
 *                  register
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[out]      mode points to caller-allocated storage where this
 *                  function should place the PCIE link width.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid (not pcie).
 * \return          FM_ERR_INVALID_STATE if the PEP is in reset or is
 *                  not enabled.
 * \return          FM_ERR_UNSUPPORTED if unrecognized link width.
 *
 *****************************************************************************/
fm_status GetPcieModeFromReg(fm_int sw, fm_int port, fm_uint32 *mode)
{
    fm_status      status;
    fm_uint32      addr;
    fm_uint32      reg;
    fm_uint32      regField;
    fm_int         pep;
    fm_bool        isPciePort;
    fm_port      * portPtr;
    fm10000_port * portExt;

    portPtr = GET_PORT_PTR(sw, port);
    status  = fm10000IsPciePort(sw, port, &isPciePort);
    FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    /* Allow only PCIE physical port belonging to PEP or CPU port*/    
    if ( ! ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL ||
              portPtr->portType == FM_PORT_TYPE_CPU) && isPciePort ) )
    {
        status = FM_ERR_INVALID_PORT;
        FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }

    portExt = GET_PORT_EXT(sw, port);
    pep     = portExt->endpoint.pep;
    addr    = FM10000_PCIE_CFG_PCIE_LINK_CTRL();


    /* Read the link status and control register */
    status = fm10000ReadPep( sw, addr, pep, &reg );
    FM_LOG_EXIT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* determine the link width */
    regField = FM_GET_FIELD( reg, 
                             FM10000_PCIE_CFG_PCIE_LINK_CTRL, 
                             CurrentLinkWidth );
    switch ( regField )
    {
        case 1:
            *mode = FM_PORT_PCIE_MODE_X1;
            break;

        case 2:
            *mode = FM_PORT_PCIE_MODE_X2;
            break;

        case 4:
            *mode = FM_PORT_PCIE_MODE_X4;
            break;

        case 8:
            *mode = FM_PORT_PCIE_MODE_X8;
            break;

        default:
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                             port, 
                             "Invalid CurrentLinkWidth value %d on port %d. LINK_CTRL 0x%x\n",
                             regField,
                             port,
                             reg );
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_EXIT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
            break;
    }

    return status;
}




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


#if 0
/*****************************************************************************/
 * NOTE:
 * This function is not needed.When a new logical port is created for LAG
 * it is initialized default port attributes.
 *****************************************************************************/
/*****************************************************************************/
/** fm10000InitLAGPortAttributes
 * \ingroup intPort
 *
 * \desc            Initialize the LAG port attributes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the lag logical port on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000InitLAGPortAttributes(fm_int sw, fm_int port)
{
    fm_portAttrEntry *attrEntry;
    fm10000_portAttr *portAttrExt;
    fm_portAttr *     portAttr;
    fm_switch *       switchPtr;
    void *            ptr;
    fm_status         err = FM_OK;
    fm_uint           i;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, port, "sw=%d port=%d\n", sw, port);

    switchPtr   = GET_SWITCH_PTR(sw);
    portAttr    = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    FM_TAKE_PORT_ATTR_LOCK(sw);

    attrEntry = (fm_portAttrEntry *) &portAttributeTable;

    for (i = 0 ;
         i < sizeof(portAttributeTable)/sizeof(fm_portAttrEntry) ;
         i++, attrEntry++)
    {
        /* Compute the port attribute address. */
        if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
        {
            ptr = GET_PORT_ATTR_ADDRESS(portAttr, attrEntry);
        }
        else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
        {
            ptr = GET_PORT_ATTR_ADDRESS(portAttrExt, attrEntry);
        }
        else
        {
            continue;
        }

        switch (attrEntry->type)
        {
            case FM_TYPE_INT:
                *( (fm_int *) ptr ) = (fm_int) attrEntry->defValue;
                break;

            case FM_TYPE_UINT32:
                *( (fm_uint32 *) ptr ) = (fm_uint32) attrEntry->defValue;
                break;

            case FM_TYPE_UINT64:
                *( (fm_uint64 *) ptr ) = (fm_uint64) 0;
                break;

            case FM_TYPE_BOOL:
                *( (fm_bool *) ptr ) = (fm_bool) attrEntry->defValue;
               break;

            case FM_TYPE_PORTMASK:
                if (attrEntry->attr == FM_PORT_MASK_WIDE)
                {
                    FM_PORTMASK_ENABLE_ALL((fm_portmask *) ptr,
                                           switchPtr->maxPhysicalPort + 1);
                }
                break;

            case FM_TYPE_MACADDR:
                *( (fm_macaddr *) ptr ) = (fm_macaddr) 0;
                break;

            default:
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT,
                                       port,
                                       FM_ERR_INVALID_ATTRIB);
                break;
        }
    }

ABORT:
    FM_DROP_PORT_ATTR_LOCK(sw);

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000InitLAGPortAttributes */
#endif




/*****************************************************************************/
/** fm10000ApplyLagMemberPortAttr
 * \ingroup intPort
 *
 * \desc            Configure the port with the common LAG settings.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000ApplyLagMemberPortAttr(fm_int sw, fm_int port, fm_int lagIndex)
{
    fm_portAttrEntry *attrEntry;
    fm_portAttr *     portAttr;
    fm10000_portAttr *portAttrExt;
    fm_portAttr *     lagAttr;
    fm10000_portAttr *lagAttrExt;
    fm10000_port *    portExt;
    void *            attrPort;
    void *            attrLag;
    fm_status         err = FM_OK;
    fm_switch *       switchPtr;
    fm_int            lagLogicalPort;
    fm_uint           i;
    fm_bitArray       bitArray;
    fm_bool           bitArrayCreated;
    fm_bool           isPciePort;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, port, "sw=%d port=%d\n", sw, port);

    bitArrayCreated = FALSE;

    if (!fmIsCardinalPort(sw, port))
    {
       FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, FM_ERR_INVALID_PORT);
    }

    switchPtr   = GET_SWITCH_PTR(sw);
    portExt     = GET_PORT_EXT(sw, port);

    FM_TAKE_PORT_ATTR_LOCK(sw);

    /* Get pointers to the port attribute structures */
    portAttr    = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /* Save current settings */
    FM_MEMCPY_S( &portExt->originalAttr.genAttr,
                 sizeof(portExt->originalAttr.genAttr),
                 portAttr,
                 sizeof(fm_portAttr) );
    FM_MEMCPY_S( &portExt->originalAttr.extAttr,
                 sizeof(portExt->originalAttr.extAttr),
                 portAttrExt,
                 sizeof(fm10000_portAttr) );
    portExt->originalAttr.attrSaved = TRUE;

    err = fmLagIndexToLogicalPort(sw, lagIndex, &lagLogicalPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /* Get pointer to the lag port attribute structures */
    lagAttr    = GET_PORT_ATTR(sw, lagLogicalPort);
    lagAttrExt = GET_FM10000_PORT_ATTR(sw, lagLogicalPort);

    portExt->allowCfg = TRUE;

    attrEntry = (fm_portAttrEntry *) &portAttributeTable;

    for (i = 0 ;
         i < sizeof(portAttributeTable)/sizeof(fm_portAttrEntry) ;
         i++, attrEntry++)
    {
        /* Only set the perLag attributes */
        if (attrEntry->perLag)
        {
            /* Get the lag and port attribute addresses. */
            if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
            {
                attrLag  = GET_PORT_ATTR_ADDRESS(lagAttr, attrEntry);
                attrPort = GET_PORT_ATTR_ADDRESS(portAttr, attrEntry);
            }
            else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
            {
                attrLag  = GET_PORT_ATTR_ADDRESS(lagAttrExt, attrEntry);
                attrPort = GET_PORT_ATTR_ADDRESS(portAttrExt, attrEntry);
            }
            else
            {
                continue;
            }

            /* Set the attribute only if their values are different. */
            if (fmComparePortAttributes(attrEntry->type, attrPort, attrLag) > 0)
            {
                if (attrEntry->attr == FM_PORT_MASK_WIDE)
                {
                    err = fmCreateBitArray(&bitArray, switchPtr->maxPhysicalPort + 1);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                    bitArrayCreated = TRUE;

                    /* convert the port mask to a bitarray */
                    err = fmPortMaskToBitArray((fm_portmask *) attrLag,
                                               &bitArray,
                                               switchPtr->maxPhysicalPort + 1);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                    attrLag = &bitArray;
                }
                else if ( (attrEntry->attr == FM_PORT_MIN_FRAME_SIZE) ||
                          (attrEntry->attr == FM_PORT_MAX_FRAME_SIZE) ||
                          (attrEntry->attr == FM_PORT_ISL_TAG_FORMAT) )
                            
                {
                    err = fm10000IsPciePort(sw, port, &isPciePort);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                    if (isPciePort)
                    {
                        /* Those attributes don't apply to PCIE ports */
                        continue;
                    }
                }

                err = fm10000SetPortAttribute(sw,
                                              port,
                                              FM_PORT_ACTIVE_MAC,
                                              FM_PORT_LANE_NA,
                                              attrEntry->attr,
                                              attrLag);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }
        }
    }

ABORT:
    portExt->allowCfg = FALSE;

    if (bitArrayCreated)
    {
        fmDeleteBitArray(&bitArray);
    }

    FM_DROP_PORT_ATTR_LOCK(sw);

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000ApplyLagMemberPortAttr */




/*****************************************************************************/
/** fm10000RestoreLagMemberPortAttr
 * \ingroup intPort
 *
 * \desc            Restore original port settings like they were before the
 *                  port was added to a LAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000RestoreLagMemberPortAttr(fm_int sw, fm_int port)
{
    fm_portAttrEntry *attrEntry;
    fm_portAttr *     portAttr;
    fm10000_portAttr *portAttrExt;
    fm_portAttr *     origAttr;
    fm10000_portAttr *origAttrExt;
    fm10000_port *    portExt;
    void *            original;
    void *            current;
    fm_status         err = FM_OK;
    fm_switch *       switchPtr;
    fm_uint           i;
    fm_bitArray       bitArray;
    fm_bool           bitArrayCreated;
    fm_bool           isPciePort;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, port, "sw=%d port=%d\n", sw, port);

    bitArrayCreated = FALSE;

    if (!fmIsCardinalPort(sw, port))
    {
       FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, FM_ERR_INVALID_PORT);
    }

    switchPtr   = GET_SWITCH_PTR(sw);
    portExt     = GET_PORT_EXT(sw, port);

    if (!portExt->originalAttr.attrSaved)
    {
        /* The original port attributes have not been previoulsy saved in
         * fm10000ApplyLagMemberPortAttr, so return immediately */
        FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, FM_ERR_UNINITIALIZED);
    }

    FM_TAKE_PORT_ATTR_LOCK(sw);

    /* Get pointer to the original and current port portAttr structures */
    origAttr    = &portExt->originalAttr.genAttr;
    origAttrExt = &portExt->originalAttr.extAttr;

    portAttr    = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /***************************************************
     * Iterrate through the port attributes table and
     * restore the per-lag attributes to their original
     * values.
     **************************************************/

    attrEntry = (fm_portAttrEntry *) &portAttributeTable;

    for (i = 0 ;
         i < sizeof(portAttributeTable)/sizeof(fm_portAttrEntry) ;
         i++, attrEntry++)
    {
        /* Only restore the perLag attributes */
        if (attrEntry->perLag)
        {
            /* Get the original and current attribute addresses. */
            if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
            {
                original = GET_PORT_ATTR_ADDRESS(origAttr, attrEntry);
                current  = GET_PORT_ATTR_ADDRESS(portAttr, attrEntry);
            }
            else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
            {
                original = GET_PORT_ATTR_ADDRESS(origAttrExt, attrEntry);
                current  = GET_PORT_ATTR_ADDRESS(portAttrExt, attrEntry);
            }
            else
            {
                continue;
            }

            /* Set the attribute only if their values are different. */
            if (fmComparePortAttributes(attrEntry->type, current, original) > 0)
            {
                if (attrEntry->attr == FM_PORT_MASK_WIDE)
                {
                    err = fmCreateBitArray(&bitArray, switchPtr->maxPhysicalPort + 1);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                    bitArrayCreated = TRUE;

                    /* convert the port mask to a bitarray */
                    err = fmPortMaskToBitArray((fm_portmask *) original,
                                               &bitArray,
                                               switchPtr->maxPhysicalPort + 1);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                    original = &bitArray;
                }
                else if ( (attrEntry->attr == FM_PORT_MIN_FRAME_SIZE) ||
                          (attrEntry->attr == FM_PORT_MAX_FRAME_SIZE) )
                            
                {
                    err = fm10000IsPciePort(sw, port, &isPciePort);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                    if (isPciePort)
                    {
                        /* Those attributes don't apply to PCIE ports */
                        continue;
                    }
                }
                
                err = fm10000SetPortAttribute(sw,
                                              port,
                                              FM_PORT_ACTIVE_MAC,
                                              FM_PORT_LANE_NA,
                                              attrEntry->attr,
                                              original);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }
        }
    }

ABORT:

    /* Indicate the port attribute are not saved anymore. */
    portExt->originalAttr.attrSaved = FALSE;

    if (bitArrayCreated)
    {
        fmDeleteBitArray(&bitArray);
    }

    FM_DROP_PORT_ATTR_LOCK(sw);

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000RestoreLagMemberPortAttr */




/*****************************************************************************/
/** fm10000IsPerLagPortAttribute
 * \ingroup intPort
 *
 * \desc            Indicate whether the given attribute is a per-lag port
 *                  attribute (i.e. can be set on a LAG logical port)
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       attr is the attribute on which to operate.
 *
 * \return          TRUE if per-lag attribute.
 * \return          FALSE if not per-lag attribute.
 *
 *****************************************************************************/
fm_bool fm10000IsPerLagPortAttribute(fm_int sw, fm_uint attr)
{
    fm_portAttrEntry *attrEntry;

    FM_NOT_USED(sw);

    attrEntry = GetPortAttrEntry(attr);

    return (attrEntry) ? attrEntry->perLag : FALSE;

}   /* end fm10000IsPerLagPortAttribute */




/*****************************************************************************/
/** fm10000DbgDumpPortAttributes
 * \ingroup intPort
 *
 * \desc            Dumps the port attributes settings.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port for which to retrieve attribute settings.
 *                  Can be a LAG logical port.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpPortAttributes(fm_int sw, fm_int port)
{
    fm_status         err = FM_OK;
    fm_int            lagLogicalPort;
    fm_uint           i;
    fm_port *         portPtr;
    fm10000_port *     portExt;
    fm_portAttr *     lagAttr;
    fm10000_portAttr *lagAttrExt = NULL;
    fm_portAttr *     origAttr = NULL;
    fm10000_portAttr *origAttrExt = NULL;
    fm_portAttrEntry *attrEntry;
    void *            origPtr;
    void *            valuePtr;
    void *            lagValue;
    fm_uint32         value32;
    fm_uint64         value64;
    fm_int            intVal;
    fm_bool           boolVal;
    fm_macaddr        macAddr;

    if (!fmIsValidPort(sw, port, ALLOW_CPU | ALLOW_LAG))
    {
        FM_LOG_PRINT("ERROR: invalid port %d\n", port);
        return;
    }

    portPtr = GET_PORT_PTR(sw, port);
    portExt = GET_PORT_EXT(sw, port);

    if (FM_IS_PORT_IN_A_LAG(portPtr) || portPtr->portType == FM_PORT_TYPE_LAG)
    {
        err = fmLagIndexToLogicalPort(sw, portPtr->lagIndex, &lagLogicalPort);

        if (err != FM_OK)
        {
            FM_LOG_PRINT("ERROR(%d): invalid lag for port %d\n", err, port);
            return;
        }

        /* Get pointer to the lag port attribute structures */
        lagAttr    = GET_PORT_ATTR(sw, lagLogicalPort);
        lagAttrExt = GET_FM10000_PORT_ATTR(sw, lagLogicalPort);
    }
    else
    {
        /* Setting it to NULL indicates below that the port isn't in lag. */
        lagAttr = NULL;
    }

    if (portExt->originalAttr.attrSaved)
    {
        /* Get pointer to the original port portAttr structures */
        origAttr    = &portExt->originalAttr.genAttr;
        origAttrExt = &portExt->originalAttr.extAttr;
    }

    /* Print the table header */
    FM_LOG_PRINT("%-34s: %-24s %-24s %-24s\n",
                 "Attributes",
                 "Original value",
                 "Current value",
                 "LAG value");
    FM_LOG_PRINT("\n-------------------------------------------------------\n");

    attrEntry = (fm_portAttrEntry *) &portAttributeTable;

    for (i = 0 ;
         i < sizeof(portAttributeTable)/sizeof(fm_portAttrEntry) ;
         i++, attrEntry++)
    {
        /* Skip this attribute if it's not applicable to the current port */
        if ( !IS_ATTRIBUTE_APPLICABLE(attrEntry) )
        {
            continue;
        }

        /* Get pointer to original port attribute */
        origPtr = NULL;
        if (portExt->originalAttr.attrSaved)
        {
            if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
            {
                origPtr = GET_PORT_ATTR_ADDRESS(origAttr, attrEntry);
            }
            else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
            {
                origPtr = GET_PORT_ATTR_ADDRESS(origAttrExt, attrEntry);
            }
        }

        /* Get pointer to lag attribute address */
        lagValue = NULL;
        if (lagAttr != NULL)
        {
            if (attrEntry->attrType == FM_PORT_ATTR_GENERIC)
            {
                lagValue = GET_PORT_ATTR_ADDRESS(lagAttr, attrEntry);
            }
            else if (attrEntry->attrType == FM_PORT_ATTR_EXTENSION)
            {
                lagValue = GET_PORT_ATTR_ADDRESS(lagAttrExt, attrEntry);
            }
        }

        /* Get current attribute value */
        switch (attrEntry->type)
        {
            case FM_TYPE_INT:
                valuePtr = &intVal;
                break;

            case FM_TYPE_UINT32:
                valuePtr = &value32;
                break;

            case FM_TYPE_UINT64:
                valuePtr = &value64;
                break;

            case FM_TYPE_BOOL:
                valuePtr = &boolVal;
                break;

            case FM_TYPE_PORTMASK:
                continue;
                break;

            case FM_TYPE_MACADDR:
                valuePtr = &macAddr;
                break;

            default:
                /* Unknown type, ignore this attribute and go to next one */
                continue;
        }

        err = fm10000GetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      attrEntry->attr,
                                      valuePtr);
        if ( err != FM_OK )
        {
            FM_LOG_PRINT("ERROR: reading attr %s port %d\n",
                         attrEntry->str,
                         port);
        }
        else
        {

            /* Print the attribute values. */
            fmPrintPortAttributeValues(attrEntry->type,
                                       attrEntry->str,
                                       attrEntry->perLag,
                                       (origPtr) ? origPtr : valuePtr,
                                       valuePtr,
                                       lagValue);
        }
    }

}   /* end fm10000DbgDumpPortAttributes */




/*****************************************************************************/
/** fm10000SetPortAttributeOnLAGs
 * \ingroup intPort
 *
 * \desc            Set a port attribute on all valid LAGs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the port attribute (see 'Port Attributes') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_PORT_MAC if mac is not valid for the
 *                  specified port, or if FM_PORT_SELECT_ACTIVE_MAC was
 *                  specified and the port has no active MAC selected.
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not valid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in
 *                  this switch.
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 * \return          FM_ERR_READONLY_ATTRIB if read-only attribute.
 * \return          FM_ERR_INVALID_VALUE if value points to an invalid value
 *                  for the specified attribute.
 * \return          FM_ERR_NOT_PER_LAG_ATTRIBUTE if an attempt is made to
 *                  set a per-port attribute on a LAG logical port.
 * \return          FM_ERR_PER_LAG_ATTRIBUTE if an attempt is made to set
 *                  a per-LAG attribute on a member port.
 *
 *****************************************************************************/
fm_status fm10000SetPortAttributeOnLAGs(fm_int sw, fm_int attr, void * value)
{
    fm_status status;
    fm_int    lagList[FM_MAX_NUM_LAGS];
    fm_int    numLags;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "sw=%d attr=%d value=%p\n", sw, attr, value);

    /* Get the list of existing LAGs. */
    status = fmGetLAGListExt(sw, &numLags, lagList, FM_MAX_NUM_LAGS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);

    /* Apply the attribute on each LAG logical port. */
    for (i = 0 ; i < numLags ; i++)
    {
        /* lagList[] contains the LAG logical port numbers. */
        status = fm10000SetPortAttribute(sw,
                                        lagList[i],
                                        FM_PORT_ACTIVE_MAC,
                                        FM_PORT_LANE_NA,
                                        attr,
                                        value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, status);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PORT, status);

}   /* end fm10000SetPortAttributeOnLAGs */




/*****************************************************************************/
/** fm10000SetPortAttribute
 * \ingroup intPort
 *
 * \desc            Set a port attribute.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       mac is the port's zero-based MAC number on which to operate.
 *                  Not applicable on this chip family.
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. May be specified as FM_PORT_LANE_NA for non-lane
 *                  oriented attributes.
 *
 * \param[in]       attr is the port attribute (see 'Port Attributes')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not valid.
 * 
 * \return          FM_ERR_INVALID_PORT_STATE if ethMode cannot be selected for
 *                  this port's MAC because of conflicts with other ports
 *                  sharing the Ethernet port logic hardware resources.
 * 
 * \return          FM_ERR_INVALID_ETH_MODE if an attempt is made to configure
 *                  an ethMode that can only be auto-negotiated via Clause 73
 * 
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in
 *                  this switch.
 * 
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 * 
 * \return          FM_ERR_READONLY_ATTRIB if read-only attribute.
 * 
 *****************************************************************************/
fm_status fm10000SetPortAttribute(fm_int sw,
                                  fm_int port,
                                  fm_int mac,
                                  fm_int lane,
                                  fm_int attr,
                                  void * value)
{
    fm_status         status;
    fm10000_portAttr *portAttrExt;
    fm_bool           doLaneIterate;
    fm_int            startLane;
    fm_int            endLane;

    FM_NOT_USED(mac);

    status      = FM_OK;
    startLane   = lane;
    endLane     = lane;

    doLaneIterate = FALSE;

    if (lane == FM_PORT_LANE_ALL)
    {
        switch (attr)
        {
            case FM_PORT_TX_LANE_CURSOR:
            case FM_PORT_TX_LANE_PRECURSOR:
            case FM_PORT_TX_LANE_POSTCURSOR:
            case FM_PORT_TX_LANE_KR_INIT_CURSOR:
            case FM_PORT_TX_LANE_KR_INIT_PRECURSOR:
            case FM_PORT_TX_LANE_KR_INIT_POSTCURSOR:
            case FM_PORT_TX_LANE_KR_INITIAL_PRE_DEC:
            case FM_PORT_TX_LANE_KR_INITIAL_POST_DEC:
            case FM_PORT_TX_LANE_ENA_KR_INIT_CFG:
            case FM_PORT_SIGNAL_THRESHOLD:
            case FM_PORT_DFE_MODE:
            case FM_PORT_RX_TERMINATION:
#if 0
            case FM_PORT_DFE_PARAMETERS:
#endif
            case FM_PORT_SLEW_RATE:
            case FM_PORT_LOW_EYE_SCORE_RECOVERY:
            case FM_PORT_LOW_EYE_SCORE_THRESHOLD:
            case FM_PORT_LOW_EYE_SCORE_TIMEOUT:
            case FM_PORT_SIGNAL_DETECT_DEBOUNCE_TIME:
            case FM_PORT_RX_OFFSET_A_COMPENSATION:
            case FM_PORT_RX_OFFSET_B_COMPENSATION:
            case FM_PORT_BIST_USER_PATTERN_LOW40:
            case FM_PORT_BIST_USER_PATTERN_UPP40:
            case FM_PORT_IFG:
            case FM_PORT_DIC_ENABLE:
            case FM_PORT_RX_LANE_POLARITY:
            case FM_PORT_TX_LANE_POLARITY:
                doLaneIterate = (lane == FM_PORT_LANE_ALL);
                break;
        }
        lane = FM_PORT_LANE_NA;
    }

    if (doLaneIterate)
    {
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

        if (doLaneIterate)
        {
            startLane = 0;
            status = fm10000GetNumLanes( sw, port, &endLane );
            FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
            endLane--;
        }
        else if (startLane == FM_PORT_LANE_ALL)
        {
            /* FM_PORT_LANE_ALL is not valid for this attribute */
            startLane = FM_PORT_LANE_NA;
            endLane = FM_PORT_LANE_NA;
        }

        for (lane = startLane ; lane <= endLane ; lane++)
        {
            status = fm10000SetPortAttributeInt(sw,
                                                port,
                                                lane,
                                                attr,
                                                value);
            FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
        }
        return status;
    }
    else
    {
        if (lane == FM_PORT_LANE_ALL)
        {
            lane = FM_PORT_LANE_NA;
        }
        return fm10000SetPortAttributeInt(sw,
                                         port,
                                         lane,
                                         attr,
                                         value);
    }

}   /* end fm10000SetPortAttribute */




/*****************************************************************************/
/** fm10000SetPortAttributeInt
 * \ingroup intPort
 *
 * \desc            Set a port attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. May be specified as FM_PORT_LANE_NA for non-lane
 *                  oriented attributes.
 *
 * \param[in]       attr is the port attribute (see 'Port Attributes') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not valid.
 * 
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in this 
 *                  switch.
 * 
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 * 
 * \return          FM_ERR_READONLY_ATTRIB if read-only attribute.
 * 
 * \return          FM_ERR_INVALID_VALUE if value points to an invalid value
 *                  for the specified attribute
 * 
 * \return          FM_ERR_INVALID_PORT_STATE if ethMode cannot be selected for
 *                  this port's MAC because of conflicts with other ports
 *                  sharing the Ethernet port logic hardware resources.
 * 
 * \return          FM_ERR_INVALID_ETH_MODE if an attempt is made to configure
 *                  an ethMode that can only be auto-negotiated via Clause 73
 *
 *****************************************************************************/
fm_status fm10000SetPortAttributeInt(fm_int sw,
                                     fm_int port,
                                     fm_int lane,
                                     fm_int attr,
                                     void * value)
{
    fm_status               err;
    fm_switch              *switchPtr;
    fm10000_switch         *switchExt;
    fm_port                *portPtr;
    fm10000_port           *portExt;
    fm_portAttr            *portAttr;
    fm_laneAttr            *laneAttr;
    fm10000_portAttr       *portAttrExt;
    fm_bool                 regLockTaken;
    fm_bool                 portAttrLockTaken;
    fm_bool                 lagLockTaken;
    fm_int                  physPort;
    fm_uint64               reg64;
    fm_uint32               reg32;
    fm_bool                 tmpBool;
    fm_int                  tmpInt;
    fm_int                  tmp2Int;
    fm_uint32               tmpUint32;
    fm_uint64               tmpUint64;
    fm_dot1xState           tmpDot1xState;
    fm_islTagFormat         tmpIslTagFormat;
    fm_bitArray             tmpMaskBitArray;
    fm_int                  intValue;
    fm_uint32               macCfg[FM10000_MAC_CFG_WIDTH];
    fm_int                  channel;
    fm_int                  epl;
    fm_int                  serdes;
    fm_ethMode              ethMode;
    fm_ethMode              prevEthMode;
    fm_portTaggingMode      tagMode;
    fm_fm10000MapSrcPortCfg mapSrcPortCfg;
    fm_smEventInfo          eventInfo;
    fm_bool                 restoreAdminMode = FALSE;
    fm_bool                 isEplPort;
    fm_bool                 isInternal;
    fm_bool                 isLagAttr;
    fm_bitArray             portMaskBitArray;
    fm_bool                 allowTeAccess;
    fm_uint32               i;
    fm_bool                 dicEnable;
    fm10000_pcsTypes        pcsType;
    fm_bool                 isPciePort;
    fm_uint32               allValidMask;
    fm_uint32               oldSpeed;
    fm_anNextPages         *nextPages;
    fm_int                  currentVlan;
    fm_int                  nextVlan;
    fm_vlanEntry           *ventry;
    fm_bool                 isMember;
    fm_bool                 toDelete;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, 
                     port,
                     "sw=%d port=%d lane=%d attr=%d value=%p\n",
                     sw,
                     port,
                     lane,
                     attr,
                     value );

    err               = FM_OK;
    regLockTaken      = FALSE;
    portAttrLockTaken = FALSE;
    portPtr           = NULL;
    lagLockTaken      = FALSE;
    isLagAttr         = FALSE;
    switchPtr         = GET_SWITCH_PTR(sw);
    switchExt         = GET_SWITCH_EXT(sw);
    toDelete          = FALSE;

    if ( (lane != FM_PORT_LANE_NA) && (lane != FM_PORT_LANE_ALL) )
    {
        err = ValidatePortLane( sw, port, lane );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    portPtr     = GET_PORT_PTR(sw, port);
    portExt     = GET_PORT_EXT(sw, port);
    portAttr    = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw,port);

    /* Pre-process internal port attribute to remove TE from internal port masks.
     * All encapsulation and decapsulation occurs on the ingress switch. No
     * frames that have been sent across internal links should ever be
     * encapsulated or decapsulated. Internal ports must remain in TE port
     * masks in order for encapsulated and decapsulated frames to route/switch
     * onto internal ports. */
    if ( (attr == FM_PORT_INTERNAL)
         && fmGetBoolApiAttribute(FM_AAK_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS,
                                  FM_AAD_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS)
         && (port != switchExt->tunnelCfg->tunnelPort[0])
         && (port != switchExt->tunnelCfg->tunnelPort[1]) )
    {
        isInternal    = *( (fm_bool *) value );
        allowTeAccess = (isInternal) ? FALSE : TRUE;

        err = fmCreateBitArray(&portMaskBitArray, switchPtr->numCardinalPorts);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        toDelete = TRUE;

        err = fmPortMaskToBitArray(&portAttr->portMask,
                                   &portMaskBitArray,
                                   switchPtr->numCardinalPorts);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        err = fmSetBitArrayBit(&portMaskBitArray,
                               switchExt->tunnelCfg->tunnelPort[0],
                               allowTeAccess);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        err = fmSetBitArrayBit(&portMaskBitArray,
                               switchExt->tunnelCfg->tunnelPort[1],
                               allowTeAccess);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        err = fm10000SetPortAttributeInt(sw,
                                         port,
                                         lane,
                                         FM_PORT_MASK_WIDE,
                                         &portMaskBitArray);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        err = fmDeleteBitArray(&portMaskBitArray);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        toDelete = FALSE;
    }

    if ( fmIsCardinalPort(sw, port) )
    {
        /* convert the logical port to physical port */
        err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else if (portPtr->portType == FM_PORT_TYPE_LAG)
    {
        if ( fm10000IsPerLagPortAttribute(sw, attr) )
        {
            /* will be TRUE if port type is LAG AND the attribute is per-lag */
            isLagAttr = TRUE;
        }
        else
        {
            err = FM_ERR_NOT_PER_LAG_ATTRIBUTE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if ( (portPtr->portType == FM_PORT_TYPE_VIRTUAL) &&
              (attr == FM_PORT_DEF_VLAN) )
    {
        /* Do nothing */
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    if ( ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
           || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (port != 0) ) )
          && lane != FM_PORT_LANE_NA )
    {
        err = fm10000MapPortLaneToSerdes(sw, port, lane, &serdes);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        /* Index to lane attribute is serdes nuber */
        laneAttr    = GET_LANE_ATTR(sw, serdes);
    }
    else
    {
        laneAttr = NULL;
    }

    /* LAG lock must be taken before PORT_ATTR lock */
    if (isLagAttr)
    {
        FM_FLAG_TAKE_LAG_LOCK(sw);
    }
    FM_FLAG_TAKE_PORT_ATTR_LOCK(sw);

    if (attr == FM_PORT_TYPE)
    {
        /* read-only property */
        err = FM_ERR_READONLY_ATTRIB;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    }
    else if (attr == FM_PORT_MIN_FRAME_SIZE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.minFrameSize);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.minFrameSize);

        tmpUint32 = (fm_uint32) ( *( (fm_uint32 *) value ) );

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_MAC_CFG,
                                              RxMinFrameLength) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        /* round down to the closest multiple of 4 */
        tmpUint32 = (tmpUint32 >> 2) << 2;

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->minFrameSize = tmpUint32;

            err = UpdateMinMaxFrameSize(sw, port);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_MAX_FRAME_SIZE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.maxFrameSize);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.maxFrameSize);

        tmpInt = *((fm_int *) value);

        /* round the word count up.  (if max frame size is to be 65 bytes, we
         * can't cut max frame size off at 64) */
        tmpInt = (fm_uint32) ((tmpInt + 3) >> 2) << 2;

        if ( (tmpInt < 0) ||
             (tmpInt > (FM10000_MAX_FRAME_SIZE - F56_NB_BYTES) ) )
        {
            FM_LOG_DEBUG_V2(FM_LOG_CAT_PORT,
                            port,
                            "Switch %d Port %d Requested max frame size (%d) "
                            "is out of the valid range (from 0 to 15360)\n",
                            sw,
                            port,
                            tmpInt);
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpInt);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->maxFrameSize = tmpInt;

            err = UpdateMinMaxFrameSize(sw, port);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_SECURITY_ACTION)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.securityAction);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.securityAction);

        tmpUint32 = *( (fm_uint32 *) value );

        if (tmpUint32 >= FM_PORT_SECURITY_ACTION_MAX)
        {
            err = FM_ERR_INVALID_VALUE;
            goto ABORT;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttrExt->securityAction = tmpUint32;

            err = fm10000SetPortSecurityAction(sw, port, tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_LEARNING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.learning);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.learning);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = switchPtr->ReadUINT32(sw, FM10000_PORT_CFG_3(physPort), &reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT(reg32, FM10000_PORT_CFG_3, LearningEnable, (tmpBool) ? 1 : 0);

            err = switchPtr->WriteUINT32(sw, FM10000_PORT_CFG_3(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_FLAG_DROP_REG_LOCK(sw);

            portAttr->learning = tmpBool;
        }
    }
    else if (attr == FM_PORT_TAGGING_MODE)
    {
        fm_int ptag;

        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.taggingMode);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.taggingMode);

        tagMode = *( (fm_portTaggingMode *) value );

        switch (tagMode)
        {
            case FM_PORT_TAGGING_8021Q:
                ptag = 0;
                break;

            case FM_PORT_TAGGING_8021AD_CUST:
                ptag = 1;
                break;

            case FM_PORT_TAGGING_8021AD_PROV:
                ptag = 2;
                break;

            case FM_PORT_TAGGING_PSEUDO1:
                ptag = 3;
                break;

            case FM_PORT_TAGGING_PSEUDO2:
                ptag = 4;
                break;

            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                break;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tagMode);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64, FM10000_MOD_PER_PORT_CFG_2, VlanTagging, ptag);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->taggingMode = tagMode;
        }
    }
    else if (attr == FM_PORT_TAGGING)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else if (attr == FM_PORT_TAGGING2)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else if (attr == FM_PORT_DEF_VLAN)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defVlan);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defVlan);

        tmpUint32 = *( (fm_uint32 *) value );
        
        if ( (tmpUint32 < 1 ) || 
             (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_1,
                                              defaultVID) ) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }   

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* for virtual ports we just update the defVlan field */
            if (portPtr->portType != FM_PORT_TYPE_VIRTUAL)
            {
                FM_FLAG_TAKE_REG_LOCK(sw);
                err = switchPtr->ReadUINT64(sw,
                                            FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                            &reg64);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                FM_SET_FIELD64(reg64,
                               FM10000_PARSER_PORT_CFG_1,
                               defaultVID,
                               tmpUint32);

                err = switchPtr->WriteUINT64(sw,
                                             FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                             reg64);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }

            portAttr->defVlan = tmpUint32;
        }

        if (portPtr->portType == FM_PORT_TYPE_VIRTUAL)
        {
            if (portAttrLockTaken)
            {
                FM_FLAG_DROP_PORT_ATTR_LOCK(sw);
            }

            /* Indicate to the host interface that we are changing state */
            err = fmNotifyPvidUpdate(sw,
                                     port,
                                     portAttr->defVlan);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
        else if (portPtr->portType == FM_PORT_TYPE_LAG)
        {
            /* Do nothing */
        }
        else
        {
            /* Indicate to the platform code that we are changing state */
            fmPlatformSetPortDefaultVlan(sw, physPort, (fm_int) tmpUint32);
        }
    }
    else if (attr == FM_PORT_DEF_VLAN2)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defVlan2);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defVlan2);

        tmpUint32 = *( (fm_uint32 *) value );

        if ( (tmpUint32 < 1) || 
             (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_1,
                                              defaultVID2) ) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_1,
                           defaultVID2,
                           tmpUint32);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->defVlan2 = tmpUint32;
        }
    }
    else if (attr == FM_PORT_DEF_PRI)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defVlanPri);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defVlanPri);

        tmpUint32 = *( (fm_uint32 *) value );

        if (tmpUint32 > 0x07)
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* Higher 3 bits of defaultVPRI field is the actual vpri.*/
            FM_SET_UNNAMED_FIELD64(reg64,
                                   FM10000_PARSER_PORT_CFG_1_l_defaultVPRI + 1,
                                   3,
                                   tmpUint32);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->defVlanPri = tmpUint32;
        }
    }
    else if (attr == FM_PORT_DEF_PRI2)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defVlanPri2);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defVlanPri2);

        tmpUint32 = *( (fm_uint32 *) value ) ;

        if (tmpUint32 > 0x07)
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* Higher 3 bits of defaultVPRI field is the actual vpri.*/
            FM_SET_UNNAMED_FIELD64(reg64,
                                   FM10000_PARSER_PORT_CFG_1_l_defaultVPRI2 + 1,
                                   3,
                                   tmpUint32);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->defVlanPri2 = tmpUint32;
        }
    }
    else if (attr == FM_PORT_DEF_DSCP)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defDscp);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defDscp);
    
        tmpUint32 = *( (fm_uint32 *) value);

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_2,
                                              defaultDSCP) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_2,
                           defaultDSCP,
                           tmpUint32);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->defDscp = tmpUint32;
        }
    }
    else if (attr == FM_PORT_DEF_SWPRI)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defSwpri);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defSwpri);
    
        tmpUint32 = *( (fm_uint32 *) value);

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PORT_CFG_ISL,
                                              defaultPriority) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, FM10000_PORT_CFG_ISL(physPort), &reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD(reg32,
                         FM10000_PORT_CFG_ISL,
                         defaultPriority,
                         tmpUint32);

            err = switchPtr->WriteUINT32(sw, FM10000_PORT_CFG_ISL(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->defSwpri = tmpUint32;
        }
    }
    else if (attr == FM_PORT_DEF_ISL_USER)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defIslUser);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defIslUser);
    
        tmpUint32 = *( (fm_uint32 *) value);

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PORT_CFG_ISL,
                                              USR) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if ( isLagAttr )
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* Retrieve the EPL and channel numbers. */
            isEplPort = FALSE;
            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            if (err == FM_OK)
            {
                isEplPort = TRUE;
            }

            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, FM10000_PORT_CFG_ISL(physPort), &reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD(reg32,
                         FM10000_PORT_CFG_ISL,
                         USR,
                         tmpUint32);

            err = switchPtr->WriteUINT32(sw, FM10000_PORT_CFG_ISL(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            if ( isEplPort )
            {
                /* The physical port has EPL associated to it, we must
                 * set the passthrough mode of the EPL. */
                err = switchPtr->ReadUINT32Mult(sw,
                                                FM10000_MAC_CFG(epl, channel, 0),
                                                FM10000_MAC_CFG_WIDTH,
                                                 macCfg);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                FM_ARRAY_SET_FIELD(macCfg, FM10000_MAC_CFG, StartCharD, tmpUint32);

                err = switchPtr->WriteUINT32Mult(sw,
                                                 FM10000_MAC_CFG(epl, channel, 0),
                                                 FM10000_MAC_CFG_WIDTH,
                                                 macCfg);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }

            portAttr->defIslUser = tmpUint32;
        }
    }
    else if (attr == FM_PORT_DROP_BV)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.dropBv);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.dropBv);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, FM10000_PORT_CFG_3(physPort), &reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT(reg32,
                       FM10000_PORT_CFG_3,
                       filterVLANIngress,
                       (tmpBool) ? 1 : 0);

            err = switchPtr->WriteUINT32(sw, FM10000_PORT_CFG_3(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->dropBv = tmpBool;
        }
    }
    else if (attr == FM_PORT_DROP_UNTAGGED)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.dropUntagged);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.dropUntagged);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         dropUntagged,
                         (tmpBool) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->dropUntagged = tmpBool;
        }
    }
    else if (attr == FM_PORT_DROP_TAGGED)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.dropTagged);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.dropTagged);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         dropTagged,
                         (tmpBool) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->dropTagged = tmpBool;
        }
    }
    else if (attr == FM_PORT_SPEED)
    {
        err = FM_ERR_READONLY_ATTRIB;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err); 
    }
    else if (attr == FM_PORT_MASK_WIDE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.maskWide);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.maskWide);

        tmpMaskBitArray = *( (fm_bitArray *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpMaskBitArray);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            /* Save bitarray as a portmask. */
            err = fmBitArrayToPortMask( (fm_bitArray *) value,
                                        &portAttr->portMask,
                                        switchPtr->numCardinalPorts );
            if (err == FM_OK)
            {
                /* Apply the updated configuration. */
                err = fm10000UpdatePortMask(sw, port);
            }
        }
    }
    else if (attr == FM_PORT_DOT1X_STATE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.dot1xState);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.dot1xState);

        switch(*( (fm_dot1xState *) value) )
        {
            case FM_DOT1X_STATE_NOT_ACTIVE:
            case FM_DOT1X_STATE_NOT_AUTH:
            case FM_DOT1X_STATE_AUTH:
                    /* Valid values */
                    break;
            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        tmpDot1xState = *( (fm_dot1xState *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpDot1xState);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            portAttr->dot1xState = tmpDot1xState;

            /* Apply the updated configuration. */
            err = fm10000UpdatePortMask(sw, port);
        }
    }
    else if (attr == FM_PORT_TX_PAUSE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txPause);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txPause);

        tmpUint32 =  *( (fm_uint32 *) value );

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_MOD_PER_PORT_CFG_2, 
                                              TxPauseValue))
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64, FM10000_MOD_PER_PORT_CFG_2, TxPauseValue, tmpUint32);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txPause = tmpUint32;
        }
    }
    else if (attr == FM_PORT_RX_PAUSE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.rxPause);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.rxPause);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, FM10000_CM_PAUSE_CFG(physPort), &reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /*****************************************************
             * Here we ensure that all traffic classes obey/ignore
             * pause.  The per traffic class property is
             * FM_PORT_RX_CLASS_PAUSE.
             *****************************************************/
            tmpUint32 = tmpBool ?
                        FM_FIELD_UNSIGNED_MAX(FM10000_CM_PAUSE_CFG, PauseMask) :
                        0;

            FM_SET_FIELD(reg32, FM10000_CM_PAUSE_CFG, PauseMask, tmpUint32);

            err = switchPtr->WriteUINT32(sw, FM10000_CM_PAUSE_CFG(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->rxPause      = tmpBool;
            portAttr->rxClassPause = 
                        tmpBool ?
                        FM_FIELD_UNSIGNED_MAX(FM10000_CM_PAUSE_CFG, PauseMask) :
                        0; 
        }
    }
    else if (attr == FM_PORT_ETHERNET_INTERFACE_MODE)
    {
        /* filter out PCIE ports */
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.ethMode);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* proceed only if there was an actual change of interface mode */
            if ( *(fm_ethMode *)value != portAttrExt->ethMode )
            {
                prevEthMode = portAttrExt->ethMode;
                ethMode = *( (fm_ethMode *) value );

                /* Make sure this ethernet mode is compatible with the
                   current EPL arrangement */
                err = ValidateEthMode( sw, port, ethMode );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                /* Prior to change the Ethernet Mode, adjust the Autoneg mode */
                /* accordingly if required */
                err = updateAnMode(sw, port, prevEthMode, ethMode, &restoreAdminMode);
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                err = fm10000ConfigureEthMode( sw,
                                               port,
                                               ethMode,
                                               &restoreAdminMode );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                portAttrExt->ethMode = ethMode;
                portExt->ethMode     = ethMode;
                portAttr->speed      = fm10000GetPortSpeed( ethMode );
                oldSpeed             = portExt->speed;
                portExt->speed       = portAttr->speed;

                err = fm10000SetPauseQuantaCoefficients(sw, port);
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                if (oldSpeed != portExt->speed)
                {
                    err = fm10000UpdateAllSAFValues(sw);
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
                }
            }

            err = FM_OK;
        }
    }
    else if (attr == FM_PORT_RX_TERMINATION)
    {
        /* add code to filter out PCIE ports */
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.rxTermination);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.rxTermination);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA ||
                 laneAttr == NULL)
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }
            else
            {
                err = ConfigureRxTermination(sw,port,lane,*(fm_rxTermination *)value);
            }
        }
    }
    else if (attr == FM_PORT_DFE_MODE)
    {
        /* add code to filter out PCIE ports */
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.dfeMode);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.dfeMode);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA ||
                 laneAttr == NULL)
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }
            else
            {
                err = ConfigureDfeMode(sw,port,lane,*(fm_dfeMode *)value);
            }
        }
    }
    else if (attr == FM_PORT_LOOPBACK)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.serdesLoopback);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.serdesLoopback);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = ConfigureLoopbackMode(sw, port, *(fm_bool *)value );

            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
        }
    }
    else if (attr == FM_PORT_FABRIC_LOOPBACK)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.fabricLoopback);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.fabricLoopback);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = ConfigureFabricLoopback(sw, port, *(fm_bool *)value);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
        }
    }
    else if (attr == FM_PORT_RX_LANE_POLARITY)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.rxLanePolarity);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.rxLanePolarity);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if (lane == FM_PORT_LANE_NA)
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            else
            {
                tmpUint32 = *( (fm_uint32 *) value );

                if (portAttr->serdesLoopback != FM_PORT_LOOPBACK_TX2RX &&
                    portAttrExt->ethMode != FM_ETH_MODE_DISABLED)
                {
                    /* only set the lane polarity if near loopback is not enabled */
                    err = fm10000SetSerdesLanePolarity(sw,
                                                       serdes,
                                                       (laneAttr->txPolarity!=0),
                                                       (tmpUint32 != 0));
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                }

                laneAttr->rxPolarity = tmpUint32;
            }
        }
    }
    else if (attr == FM_PORT_TX_FCS_MODE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txFcsMode);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txFcsMode);

        tmpUint32 = *( (fm_uint32 *) value );

        /* convert from generic values to FM10000 values */
        for (i = 0; i < FM_NENTRIES(txFcsModeMap); i++)
        {
            if (txFcsModeMap[i].a == tmpUint32)
            {
                tmpUint32 = txFcsModeMap[i].b;
                break;
            }
        }

        /* If we reached end of iteration loop, we did not find
         * a match */
        if (i == FM_NENTRIES(txFcsModeMap))
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* Retrieve the EPL and channel numbers. */
            isEplPort = FALSE;
            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            if (err == FM_OK)
            {
                isEplPort = TRUE;
            }

            if (isEplPort)
            {
                FM_FLAG_TAKE_REG_LOCK(sw);

                /* The physical port has EPL associated to it, we must
                 * set the passthrough mode of the EPL. */
                err = switchPtr->ReadUINT32Mult(sw,
                                                FM10000_MAC_CFG(epl, channel, 0),
                                                FM10000_MAC_CFG_WIDTH,
                                                 macCfg);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                FM_ARRAY_SET_FIELD(macCfg, FM10000_MAC_CFG, TxFcsMode, tmpUint32);

                err = switchPtr->WriteUINT32Mult(sw,
                                                 FM10000_MAC_CFG(epl, channel, 0),
                                                 FM10000_MAC_CFG_WIDTH,
                                                 macCfg);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                portAttrExt->txFcsMode = *((fm_int *) value);
            }
        }
    }
    else if (attr == FM_PORT_TX_LANE_POLARITY)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLanePolarity);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLanePolarity);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if (lane == FM_PORT_LANE_NA)
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            else
            {
                tmpUint32 = *( (fm_uint32 *) value );

                if (portAttr->serdesLoopback != FM_PORT_LOOPBACK_TX2RX &&
                    portAttrExt->ethMode != FM_ETH_MODE_DISABLED)
                {
                    /* only set the lane polarity if near loopback is not enabled */
                    err = fm10000SetSerdesLanePolarity(sw,
                                                       serdes,
                                                       (tmpUint32 != 0),
                                                       (laneAttr->rxPolarity != 0));
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                }

                laneAttr->txPolarity = tmpUint32;
            }
        }
    }
    else if (attr == FM_PORT_BIST_USER_PATTERN_LOW40)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.bistUserPatterLow40);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.bistUserPatterLow40);

        tmpUint64 = *( (fm_uint64 *) value );

        /* count the number of 1's in the given value.*/
        tmpInt = CountOnes(tmpUint64);

        FM_LOG_DEBUG(FM_LOG_CAT_PORT,
                        "The number of 1's in the given value is %d\n",
                        tmpInt);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* set the low part of the bist user pattern */
            err = fm10000SetBistUserPattern(sw, port, lane, (fm_uint64 *) value, NULL);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_BIST_USER_PATTERN_UPP40)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.bistUserPatterUpp40);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.bistUserPatterUpp40);

        tmpUint64 = *( (fm_uint64 *) value );

        /* count the number of 1's in the given value.*/
        tmpInt = CountOnes(tmpUint64);

        FM_LOG_DEBUG(FM_LOG_CAT_PORT,
                        "The number of 1's in the given value is %d\n",
                        tmpInt);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* set the high part of the bist user pattern */
            err = fm10000SetBistUserPattern(sw, port, lane, NULL, (fm_uint64 *) value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_TX_PAUSE_MODE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txPauseMode);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txPauseMode);

        tmpUint32 = *( (fm_uint32 *) value);

        switch ( tmpUint32 )
        {
            case FM_PORT_TX_PAUSE_NORMAL:
            case FM_PORT_TX_PAUSE_CLASS_BASED:
                /* Valid values */
                break;
            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                break;

        }   /* end switch ( tmpUint32 ) */

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            if (tmpUint32 == FM_PORT_TX_PAUSE_CLASS_BASED)
            {
                FM_SET_BIT64(reg64, FM10000_MOD_PER_PORT_CFG_2, TxPauseType, 1);

                /* Make sure the PriEnVec is set as FM_PORT_TX_CLASS_PAUSE */
                FM_SET_FIELD64(reg64,
                               FM10000_MOD_PER_PORT_CFG_2,
                               TxPausePriEnVec,
                               portAttr->txClassPause);
            }
            else if (tmpUint32 == FM_PORT_TX_PAUSE_NORMAL)
            {
                FM_SET_BIT64(reg64, FM10000_MOD_PER_PORT_CFG_2, TxPauseType, 0);
            }
            else
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txPauseMode = tmpUint32;
        }
    }
    else if (attr == FM_PORT_TX_PAUSE_RESEND_TIME)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txPauseResendTime);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txPauseResendTime);

        tmpUint32 = *( (fm_uint32 *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000SetPauseResendInterval(sw, port, tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txPauseResendTime = tmpUint32;
        }
    }
    else if (attr == FM_PORT_RX_CLASS_PAUSE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.rxClassPause);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.rxClassPause);

        tmpUint32 = *( (fm_uint32 *) value);
        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_CM_PAUSE_CFG,
                                              PauseMask) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32(sw, FM10000_CM_PAUSE_CFG(physPort), &reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD(reg32, FM10000_CM_PAUSE_CFG, PauseMask, tmpUint32);

            err = switchPtr->WriteUINT32(sw, FM10000_CM_PAUSE_CFG(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->rxClassPause = tmpUint32;
            if (portAttr->rxClassPause == 
                        FM_FIELD_UNSIGNED_MAX(FM10000_CM_PAUSE_CFG, PauseMask) )
            {
                portAttr->rxPause = TRUE; 
            }
            else
            {
                portAttr->rxPause = FALSE;
            }
        }
    }
    else if (attr == FM_PORT_TX_CLASS_PAUSE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txClassPause);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txClassPause);

        tmpUint32 = *( (fm_uint32 *) value);
        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_MOD_PER_PORT_CFG_2,
                                              TxPausePriEnVec) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_MOD_PER_PORT_CFG_2,
                           TxPausePriEnVec,
                           tmpUint32);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txClassPause = tmpUint32;
        }
    }

    else if (attr == FM_PORT_TXCFI)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txCfi);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txCfi);

        tmpUint32 = *( (fm_uint32 *) value );

        switch (tmpUint32)
        {
            case FM_PORT_TXCFI_ASIS:
                tmpBool = FALSE;
                break;

            case FM_PORT_TXCFI_ISPRIBIT:
                tmpBool = TRUE;
                break;

            default:
                err = FM_ERR_INVALID_VALUE;
                goto ABORT;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnableDei1Update,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txCfi = tmpUint32;
        }
    }
    else if (attr == FM_PORT_TXCFI2)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txCfi2);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txCfi2);

        tmpUint32 = *( (fm_uint32 *) value );

        switch (tmpUint32)
        {
            case FM_PORT_TXCFI_ASIS:
                tmpBool = FALSE;
                break;

            case FM_PORT_TXCFI_ISPRIBIT:
                tmpBool = TRUE;
                break;

            default:
                err = FM_ERR_INVALID_VALUE;
                goto ABORT;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnableDei2Update,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txCfi2 = tmpUint32;
        }
    }
    else if (attr == FM_PORT_SWPRI_SOURCE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.swpriSource);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.swpriSource);

        tmpUint32 = *( (fm_uint32 *) value );

        allValidMask = FM_PORT_SWPRI_VPRI1 |
                       FM_PORT_SWPRI_DSCP  |
                       FM_PORT_SWPRI_ISL_TAG;

        /* If 0 or any bit set apart from valid mask, then its invalid */
        if ( (tmpUint32 == 0) || ( (tmpUint32 & ~allValidMask) != 0 ) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         SwitchPriorityFromVLAN,
                         ( (tmpUint32 & FM_PORT_SWPRI_VPRI) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         SwitchPriorityFromDSCP,
                         ( (tmpUint32 & FM_PORT_SWPRI_DSCP) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         SwitchPriorityFromISL,
                         ( (tmpUint32 & FM_PORT_SWPRI_ISL_TAG) != 0) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->swpriSource = tmpUint32;
        }
    }
    else if (attr == FM_PORT_SWPRI_DSCP_PREF)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.swpriDscpPref);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.swpriDscpPref);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *((fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);

            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         SwitchPriorityPrefersDSCP,
                         (tmpBool) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->swpriDscpPref = tmpBool;
        }
    }
    else if (attr == FM_PORT_PARSER)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parser);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parser);

        tmpUint32 = *( (fm_uint32 *) value );

        switch ( tmpUint32 )
        {
            case FM_PORT_PARSER_STOP_AFTER_L2:
            case FM_PORT_PARSER_STOP_AFTER_L3:
            case FM_PORT_PARSER_STOP_AFTER_L4:
                /* Valid values */
                break;
            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                break;

        }   /* end switch ( tmpUint32 ) */

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);

            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            switch ( tmpUint32 )
            {
                case FM_PORT_PARSER_STOP_AFTER_L2:
                    FM_SET_BIT64(reg64,
                                 FM10000_PARSER_PORT_CFG_2,
                                 ParseL3,
                                 0);
                    FM_SET_BIT64(reg64,
                                 FM10000_PARSER_PORT_CFG_2,
                                 ParseL4,
                                 0);
                    break;

                case FM_PORT_PARSER_STOP_AFTER_L3:
                    FM_SET_BIT64(reg64,
                                 FM10000_PARSER_PORT_CFG_2,
                                 ParseL3,
                                 1);
                    FM_SET_BIT64(reg64,
                                 FM10000_PARSER_PORT_CFG_2,
                                 ParseL4,
                                 0);
                    break;

                case FM_PORT_PARSER_STOP_AFTER_L4:
                    FM_SET_BIT64(reg64,
                                 FM10000_PARSER_PORT_CFG_2,
                                 ParseL3,
                                 1);
                    FM_SET_BIT64(reg64,
                                 FM10000_PARSER_PORT_CFG_2,
                                 ParseL4,
                                 1);
                    break;

                default:
                    err = FM_ERR_INVALID_VALUE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            }   /* end switch ( tmpUint32 ) */

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->parser = tmpUint32;
        }
    }
    else if (attr == FM_PORT_PARSER_FLAG_OPTIONS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parserFlagOptions);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parserFlagOptions);

        tmpUint32 = *( (fm_uint32 *) value );

        allValidMask = FM_PORT_PARSER_FLAG_IPV4_OPTIONS |
                       FM_PORT_PARSER_FLAG_IPV6_HOPBYHOP |
                       FM_PORT_PARSER_FLAG_IPV6_ROUTING |
                       FM_PORT_PARSER_FLAG_IPV6_FRAGMENT |
                       FM_PORT_PARSER_FLAG_IPV6_DEST |
                       FM_PORT_PARSER_FLAG_IPV6_AUTH;

        if ( (tmpUint32 & ~allValidMask) != 0)
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         FlagIPv4Options,
                         ( (tmpUint32 & FM_PORT_PARSER_FLAG_IPV4_OPTIONS) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         FlagIPv6HopByHop,
                         ( (tmpUint32 & FM_PORT_PARSER_FLAG_IPV6_HOPBYHOP) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         FlagIPv6Routing,
                         ( (tmpUint32 & FM_PORT_PARSER_FLAG_IPV6_ROUTING) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         FlagIPv6Frag,
                         ( (tmpUint32 & FM_PORT_PARSER_FLAG_IPV6_FRAGMENT) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         FlagIPv6Dest,
                         ( (tmpUint32 & FM_PORT_PARSER_FLAG_IPV6_DEST) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         FlagIPv6Auth,
                         ( (tmpUint32 & FM_PORT_PARSER_FLAG_IPV6_AUTH) != 0) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->parserFlagOptions = tmpUint32;
        }
    }
    else if (attr == FM_PORT_AUTONEG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegMode);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegMode);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = ConfigureAnMode(sw, port, *( (fm_uint32 *) value), &restoreAdminMode);

            if ( err == FM_OK )
            {
                portAttr->autoNegMode = *((fm_uint32 *) value);
            }
        }
    }
    else if (attr == FM_PORT_AUTONEG_BASEPAGE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegBasePage);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegBasePage);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* validate the current autoneg mode against the new base page */
            err = fm10000AnValidateBasePage( sw,
                                             port,
                                             portAttr->autoNegMode,
                                             *(fm_uint64 *)value,
                                             (fm_uint64  *)value );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* if there is already an AN state machine notify of the config
                change */
            if ( portExt->smType == FM10000_AN_PORT_STATE_MACHINE )
            {
                err = fm10000AnSendConfigEvent( sw,
                                                port,
                                                FM10000_PORT_EVENT_AN_CONFIG_REQ,
                                                portAttr->autoNegMode,
                                                *( (fm_uint64 *) value ),
                                                portAttr->autoNegNextPages );
            }
            else
            {
                /* nothing to do if AN state machine wasn't started yet */
                err = FM_OK;
            }

            /* update the attribute if successful */
            if ( err == FM_OK )
            {
                portAttr->autoNegBasePage = *( (fm_uint64 *) value );
            }
        }
    }
    else if (attr == FM_PORT_AUTONEG_NEXTPAGES)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegNextPages);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegNextPages);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* if there is already an AN state machine notify of the config
                change */
            nextPages = (fm_anNextPages *) value;

            /* Validate extended tech ability next page */
            if (nextPages->numPages > 0)
            {
                if (fm10000AnGetNextPageExtTechAbilityIndex(sw,
                        port,
                        nextPages->nextPages,
                        nextPages->numPages,
                        &i,
                        "Tx") == FM_OK)
                {
                    if (FM_GET_UNNAMED_BIT64(nextPages->nextPages[i], 24))
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_PORT_AUTONEG,
                                       "Port does not support 50GBase-KR2\n");
                    }
                    if (FM_GET_UNNAMED_BIT64(nextPages->nextPages[i], 25))
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_PORT_AUTONEG,
                                       "Port does not support 50GBase-CR2\n");
                    }
                    if (FM_GET_UNNAMED_BIT64(nextPages->nextPages[i], 40))
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_PORT_AUTONEG,
                                       "Port cannot advertise Clause 91 FEC Ability\n");
                    }
                    if (FM_GET_UNNAMED_BIT64(nextPages->nextPages[i], 41))
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_PORT_AUTONEG,
                                       "Port cannot advertise Clause 74 FEC Ability\n");
                    }
                    if (FM_GET_UNNAMED_BIT64(nextPages->nextPages[i], 42))
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_PORT_AUTONEG,
                                       "Port cannot request Clause 91 FEC\n");
                    }
                    if (FM_GET_UNNAMED_BIT64(nextPages->nextPages[i], 43))
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_PORT_AUTONEG,
                                       "Port cannot request Clause 74 FEC\n");
                    }
                }
            }            
            
            if ( portExt->smType == FM10000_AN_PORT_STATE_MACHINE )
            {
                err = fm10000AnSendConfigEvent( sw,
                                                port,
                                                FM10000_PORT_EVENT_AN_CONFIG_REQ,
                                                portAttr->autoNegMode,
                                                portAttr->autoNegBasePage,
                                                *nextPages );
            }
            else
            {
                /* nothing to do if AN state machine wasn't started yet */
                err = FM_OK;
            }

            /* update the attribute if successful */
            if ( err == FM_OK )
            {
                /* Reuse the old memory if new pages are smaller than previous */
                if (portAttr->autoNegNextPages.numPages < nextPages->numPages)
                {
                    if (portAttr->autoNegNextPages.nextPages)
                    {
                        fmFree(portAttr->autoNegNextPages.nextPages);
                        portAttr->autoNegNextPages.nextPages = NULL;
                    }
                    portAttr->autoNegNextPages.nextPages = fmAlloc( sizeof(fm_uint64) *
                                                                    nextPages->numPages );
                    if (!portAttr->autoNegNextPages.nextPages)
                    {
                        err = FM_ERR_NO_MEM;
                        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                    }
                }
                portAttr->autoNegNextPages.numPages = nextPages->numPages;
                if (nextPages->numPages)
                {
                    FM_MEMCPY_S( portAttr->autoNegNextPages.nextPages,
                                 sizeof(fm_uint64) * nextPages->numPages,
                                 nextPages->nextPages,
                                 sizeof(fm_uint64) * nextPages->numPages );
                }
            }
        }
    }
    else if (attr == FM_PORT_AUTONEG_25G_NEXTPAGE_OUI)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNeg25GNxtPgOui);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNeg25GNxtPgOui);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* OUI has only 24 bits, so mask out unused bits */
            portAttr->autoNeg25GNxtPgOui = (*( (fm_uint32 *) value )) & 0x00ffffff;
            err = FM_OK;
        }
    }
    else if ( attr == FM_PORT_AUTONEG_LINK_INHB_TIMER )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegLinkInhbTimer);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegLinkInhbTimer);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000An73SetLinkInhibitTimer( sw, port, *(fm_uint *)value );
        }
    }
    else if ( attr == FM_PORT_AUTONEG_LINK_INHB_TIMER_KX )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegLinkInhbTimerKx);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegLinkInhbTimerKx);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000An73SetLinkInhibitTimerKx( sw, port, *(fm_uint *)value );
        }
    }
    else if ( attr == FM_PORT_AUTONEG_IGNORE_NONCE )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoNegIgnoreNonce);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoNegIgnoreNonce);

        VALIDATE_VALUE_IS_BOOL(value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000An73SetIgnoreNonce( sw, port, *(fm_uint *)value );
        }
    }
    else if (attr == FM_PORT_AUTONEG_PARTNER_BASEPAGE   ||
             attr == FM_PORT_AUTONEG_PARTNER_NEXTPAGES )
    {
        /* valid only for get */
        err = FM_ERR_READONLY_ATTRIB;
    }
    else if (attr == FM_PORT_ISL_TAG_FORMAT)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.islTagFormat);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.islTagFormat);
        
        tmpIslTagFormat = *((fm_islTagFormat *) value);

        if ( (tmpIslTagFormat != FM_ISL_TAG_NONE) &&
             (tmpIslTagFormat != FM_ISL_TAG_F56) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpIslTagFormat);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            /* Retrieve the EPL and channel numbers. */
            isEplPort = FALSE;
            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            if (err == FM_OK)
            {
                isEplPort = TRUE;
            }

            /* Instead of non-Ethernet port check for Pcie and TE ports */
            /* For non-Ethernet ports only FM_ISL_TAG_F56 tag is allowed.
             * Ideally one time initialization of non-Ethernet ports is enough,
             * but SWAG architecture rely on setting these attributes. Hence
             * only FM_ISL_TAG_F56 is allowed for non-Ethernet ports.   */
            if (!isEplPort && ( *((fm_islTagFormat *) value) != FM_ISL_TAG_F56))
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }


            /* Keep old ISL tag format in case of error */
            tmpUint32 = portAttr->islTagFormat;
            portAttr->islTagFormat = *( (fm_uint32 *) value );
            err = UpdateMinMaxFrameSize(sw, port);

            /* Non EPL ports do not have a minium/maximum frame size configuration */
            if (err == FM_ERR_INVALID_ARGUMENT && !isEplPort)
            {
                err = FM_OK;
            }
            else if (err != FM_OK)
            {
                /* Restore old ISL tag mode */
                portAttr->islTagFormat = tmpUint32;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }

            /* Untag port for all VLAN Membership */
            if (!fmGetBoolApiProperty(FM_AAK_API_PORT_ALLOW_FTAG_VLAN_TAGGING,
                                      FM_AAD_API_PORT_ALLOW_FTAG_VLAN_TAGGING))
            {
                err = fmGetVlanFirst(sw, &currentVlan);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                while (currentVlan != -1)
                {
                    ventry = GET_VLAN_PTR(sw, currentVlan);
                    err = fmGetVlanMembership(sw,
                                              ventry,
                                              port,
                                              &isMember);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_VLAN, port, err);

                    if (isMember)
                    {
                        err = fmSetVlanTag(sw,
                                           FM_VLAN_SELECT_VLAN1,
                                           ventry,
                                           port,
                                           FALSE);
                        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_VLAN, port, err);
                    }

                    fmGetVlanNext(sw, currentVlan, &nextVlan);
                    currentVlan = nextVlan;
                }
            }

            err = UpdateTruncFrameSize(sw, port);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* Update HW with the new mode */
            tmpBool = (*((fm_islTagFormat *) value) == FM_ISL_TAG_F56) ? 1 : 0;

            err = fm10000SetStatsFrameAdjustment(sw,
                                                 physPort,
                                                 tmpBool ? F56_NB_BYTES : 0);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64, FM10000_MOD_PER_PORT_CFG_2, FTAG,  tmpBool);

            err = WriteModPerPortCfg2(sw, physPort, reg64);

            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64, FM10000_PARSER_PORT_CFG_1, FTAG,  tmpBool);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);

            if ( isEplPort )
            {
                /* The physical port has EPL associated to it, we must
                 * set the passthrough mode of the EPL. */
                err = switchPtr->ReadUINT32Mult(sw,
                                                FM10000_MAC_CFG(epl, channel, 0),
                                                FM10000_MAC_CFG_WIDTH,
                                                 macCfg);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                FM_ARRAY_SET_BIT(macCfg, FM10000_MAC_CFG, PreambleMode, tmpBool);

                err = switchPtr->WriteUINT32Mult(sw,
                                                 FM10000_MAC_CFG(epl, channel, 0),
                                                 FM10000_MAC_CFG_WIDTH,
                                                 macCfg);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }

            FM_FLAG_DROP_REG_LOCK(sw);
        }
    }
    else if (attr == FM_PORT_ROUTABLE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.routable);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.routable);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            mapSrcPortCfg.routable = tmpBool;

            err = fm10000SetMapSourcePort(sw,
                                          physPort,
                                          &mapSrcPortCfg,
                                          FM_FM10000_MAP_SRC_ROUTABLE,
                                          TRUE);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->routable = mapSrcPortCfg.routable;
        }
    }
    else if (attr == FM_PORT_UPDATE_TTL)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.updateTtl);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.updateTtl);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnableTTLDecrement,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->updateTtl = tmpBool;
        }
    }
    else if (attr == FM_PORT_IGNORE_IFG_ERRORS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.ignoreIfgErrors);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.ignoreIfgErrors);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_FLAG_TAKE_REG_LOCK(sw);

            err = switchPtr->ReadUINT32Mult(sw,
                                            FM10000_MAC_CFG(epl, channel, 0),
                                            FM10000_MAC_CFG_WIDTH,
                                             macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_ARRAY_SET_BIT(macCfg, FM10000_MAC_CFG, RxIgnoreIfgErrors, tmpBool);

            err = switchPtr->WriteUINT32Mult(sw,
                                             FM10000_MAC_CFG(epl, channel, 0),
                                             FM10000_MAC_CFG_WIDTH,
                                             macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->ignoreIfgErrors = tmpBool;
        }
    }
    else if (attr == FM_PORT_UPDATE_DSCP)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.updateDscp);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.updateDscp);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnableDSCPModification,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->updateDscp = tmpBool;
        }
    }
    else if (attr == FM_PORT_TXVPRI)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txVpri);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txVpri);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnablePcp1Update,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txVpri = tmpBool;
        }
    }
    else if (attr == FM_PORT_TXVPRI2)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txVpri2);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txVpri2);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnablePcp2Update,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txVpri2 = tmpBool;
        }
    }
    else if (attr == FM_PORT_DEF_CFI)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.defCfi);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.defCfi);

        tmpUint32 = *( (fm_uint32 *) value );

        if ( !( (tmpUint32 == 0) || (tmpUint32 == 1) ) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* Low bit of defaultVPRI corresponds to the CFI/DEI bit */
            FM_SET_UNNAMED_FIELD64(reg64,
                                   FM10000_PARSER_PORT_CFG_1_l_defaultVPRI,
                                   1,
                                   tmpUint32);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->defCfi = tmpUint32;
        }
    }
    else if (attr == FM_PORT_REPLACE_DSCP)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.replaceDscp);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.replaceDscp);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         useDefaultDSCP,
                         (tmpBool) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->replaceDscp = tmpBool;
        }
    }
    else if (attr == FM_PORT_UCAST_FLOODING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.ucastFlooding);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.ucastFlooding);

        intValue = *( (fm_int *) value);

        switch ( intValue )
        {
            case FM_PORT_UCAST_FWD_EXCPU:
            case FM_PORT_UCAST_FWD:
            case FM_PORT_UCAST_DISCARD:
            case FM_PORT_UCAST_TRAP:
                /* Valid values */
                break;
            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                break;

        }   /* end switch ( intValue ) */

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &intValue);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000SetPortUcastFlooding(sw, port, intValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            portAttr->ucastFlooding = intValue;

            /* fm10000SetSwitchAttribute will override this. */
            switchExt->ucastFlooding = FM_UCAST_FLOODING_PER_PORT;
        }
    }
    else if (attr == FM_PORT_MCAST_FLOODING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.mcastFlooding);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.mcastFlooding);

        intValue = *( (fm_int *) value);

        switch ( intValue )
        {
            case FM_PORT_MCAST_FWD_EXCPU:
            case FM_PORT_MCAST_FWD:
            case FM_PORT_MCAST_DISCARD:
            case FM_PORT_MCAST_TRAP:
                /* Valid values */
                break;
            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                break;

        }   /* end switch ( intValue ) */

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &intValue);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000SetPortMcastFlooding(sw, port, intValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            portAttr->mcastFlooding = intValue;

            /* fm10000SetSwitchAttribute will override this. */
            switchExt->mcastFlooding = FM_MCAST_FLOODING_PER_PORT;
        }
    }
    else if (attr == FM_PORT_BCAST_FLOODING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.bcastFlooding);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.bcastFlooding);

        intValue = *( (fm_int *) value);

        switch ( intValue )
        {
            case FM_PORT_BCAST_FWD:
            case FM_PORT_BCAST_FWD_EXCPU:
            case FM_PORT_BCAST_DISCARD:
            case FM_PORT_BCAST_TRAP:
                /* Valid values */
                break;
            default:
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
                break;

        }   /* end switch ( intValue ) */

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &intValue);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000SetPortBcastFlooding(sw, port, intValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            portAttr->bcastFlooding = intValue;

            /* fm10000SetSwitchAttribute will override this. */
            switchExt->bcastFlooding = FM_BCAST_FLOODING_PER_PORT;
        }
    }
    else if (attr == FM_PORT_BCAST_PRUNING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.bcastPruning);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.bcastPruning);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->bcastPruning = tmpBool;

            err = fm10000SetFloodDestPort(sw,
                                          port,
                                          !portAttr->bcastPruning,
                                          FM_PORT_BCAST);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
    }
    else if (attr == FM_PORT_MCAST_PRUNING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.mcastPruning);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.mcastPruning);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->mcastPruning = tmpBool;

            err = fm10000SetFloodDestPort(sw,
                                          port,
                                          !portAttr->mcastPruning,
                                          FM_PORT_MCAST);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
    }
    else if (attr == FM_PORT_UCAST_PRUNING)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.ucastPruning);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.ucastPruning);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->ucastPruning = tmpBool;

            err = fm10000SetFloodDestPort(sw,
                                          port,
                                          !portAttr->ucastPruning,
                                          FM_PORT_FLOOD);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
    }
    else if ( (attr == FM_PORT_CUT_THROUGH) ||
              (attr == FM_PORT_TX_CUT_THROUGH) )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.enableTxCutThrough);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.enableTxCutThrough);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->enableTxCutThrough = tmpBool;

            err = fm10000UpdateAllSAFValues(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
    }
    else if (attr == FM_PORT_RX_CUT_THROUGH)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.enableRxCutThrough);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.enableRxCutThrough);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->enableRxCutThrough = tmpBool;

            err = fm10000UpdateAllSAFValues(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
    }
    else if (attr == FM_PORT_LOOPBACK_SUPPRESSION)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.loopbackSuppression);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.loopbackSuppression);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttr->loopbackSuppression = tmpBool;

            err = fm10000UpdateLoopbackSuppress(sw, port);
        }
    }
    else if (attr == FM_PORT_TX_LANE_PRECURSOR)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLanePreCursor);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLanePreCursor);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            tmp2Int = *( (fm_int *) value);

            if ( tmp2Int  != laneAttr->preCursor)
            {
                err = fm10000SetSerdesPreCursor(sw, serdes, tmp2Int );
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                laneAttr->preCursor = tmp2Int;

                /* Send the new attribute value to the platform layer. */
                fmPlatformNotifyPortAttribute(sw,
                                              port,
                                              0,
                                              lane,
                                              attr,
                                              value);
            }
        }
    }
    else if (attr == FM_PORT_TX_LANE_POSTCURSOR)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLanePostCursor);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLanePostCursor);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            tmp2Int = *( (fm_int *) value);

            if ( tmp2Int  != laneAttr->postCursor)
            {
                err = fm10000SetSerdesPostCursor(sw, serdes, tmp2Int );
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                laneAttr->postCursor = tmp2Int;

                /* Send the new attribute value to the platform layer. */
                fmPlatformNotifyPortAttribute(sw,
                                              port,
                                              0,
                                              lane,
                                              attr,
                                              value);
            }
        }
    }
    else if (attr == FM_PORT_TX_LANE_CURSOR)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneCursor);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneCursor);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA  || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            tmp2Int = *( (fm_int *) value);

            if (tmp2Int != laneAttr->cursor)
            {
                err = fm10000SetSerdesCursor(sw, serdes, tmp2Int);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                laneAttr->cursor = tmp2Int;

                /* Send the new attribute value to the platform layer. */
                fmPlatformNotifyPortAttribute(sw,
                                              port,
                                              0,
                                              lane,
                                              attr,
                                              value);
            }
        }
    }
    else if (attr == FM_PORT_TX_LANE_KR_INIT_PRECURSOR)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneKrInitPreCursor);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneKrInitPreCursor);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            laneAttr->initializePreCursor = *( (fm_int *) value);
        }
    }
    else if (attr == FM_PORT_TX_LANE_KR_INIT_POSTCURSOR)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneKrInitPostCursor);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneKrInitPostCursor);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            laneAttr->initializePostCursor = *( (fm_int *) value);
        }
    }
    else if (attr == FM_PORT_TX_LANE_KR_INIT_CURSOR)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneKrInitCursor);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneKrInitCursor);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA  || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            laneAttr->initializeCursor = *( (fm_int *) value);
        }
    }
    else if (attr == FM_PORT_TX_LANE_KR_INITIAL_PRE_DEC)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneKrInitialPreDec);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneKrInitialPreDec);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA  || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            laneAttr->preCursorDecOnPreset = *( (fm_int *) value);
        }
    }
    else if (attr == FM_PORT_TX_LANE_KR_INITIAL_POST_DEC)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneKrInitialPostDec);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneKrInitialPostDec);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA  || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            laneAttr->postCursorDecOnPreset = *( (fm_int *) value);
        }
    }
    else if (attr == FM_PORT_TX_LANE_ENA_KR_INIT_CFG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLaneEnableConfigKrInit);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLaneEnableConfigKrInit);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( lane == FM_PORT_LANE_NA || laneAttr == NULL )
            {
                err = FM_ERR_INVALID_PORT_LANE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            laneAttr->txLaneEnableConfigKrInit = *( (fm_int *) value);
        }
    }
    else if (attr == FM_PORT_INTERNAL)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.internal);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.internal);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000IsPciePort( sw, port, &isPciePort );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            if ( ( isPciePort && (tmpBool != FM_DISABLED) ) ||
                 (portPtr->portType == FM_PORT_TYPE_TE &&
                 (tmpBool == FM_DISABLED) ) )
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            /* Check for TE port as well. For TE ports only it is always
             * internal port. Only FM_ENABLED is valid for TE port. */
            portAttr->internalPort = tmpBool;

            if ( fmIsCardinalPort(sw, port) && portAttr->internalPort )
            {
                /* Change the STP state of the port to forwarding */
                err = UpdateInternalPort(sw, port);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            }
        }
    }
    else if (attr == FM_PORT_LINK_INTERRUPT)
    {
        err = FM_ERR_READONLY_ATTRIB;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err); 
    }
    else if (attr == FM_PORT_PARSER_VLAN1_TAG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parserVlan1Tag);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parserVlan1Tag);

        tmpInt = *( (fm_int *) value );
        if ( (tmpInt < 0) || 
             (tmpInt > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_1,
                                           Vlan1Tag) ) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpInt);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_1,
                           Vlan1Tag,
                           tmpInt);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->parserVlan1Tag = tmpInt;
        }
    }
    else if (attr == FM_PORT_PARSER_VLAN2_TAG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parserVlan2Tag);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parserVlan2Tag);

        tmpInt = *( (fm_int *) value );
        if ( (tmpInt < 0) || 
             (tmpInt > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_1,
                                             Vlan2Tag) ) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpInt);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_1,
                           Vlan2Tag,
                           tmpInt);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->parserVlan2Tag = tmpInt;
        }
    }
    else if (attr == FM_PORT_MODIFY_VLAN1_TAG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.modifyVlan1Tag);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.modifyVlan1Tag);

        tmpInt = *( (fm_int *) value );
        if ( (tmpInt < 0) || 
             (tmpInt > FM_MAX_MODIFY_VLAN_ETYPE_INDEX) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpInt);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_MOD_PER_PORT_CFG_2,
                           VLAN1_EType,
                           tmpInt);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->modifyVlan1Tag = tmpInt;
        }
    }
    else if (attr == FM_PORT_MODIFY_VLAN2_TAG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.modifyVlan2Tag);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.modifyVlan2Tag);

        tmpInt = *( (fm_int *) value );
        if ( (tmpInt < 0) || 
             (tmpInt > FM_MAX_MODIFY_VLAN_ETYPE_INDEX) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpInt);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_MOD_PER_PORT_CFG_2,
                           VLAN2_EType,
                           tmpInt);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->modifyVlan2Tag = tmpInt;
        }
    }
    else if (attr == FM_PORT_MIRROR_TRUNC_SIZE)
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "port=%d exclude=0x%08x type=%d\n",
                         port,
                         portAttributeTable.mirrorTruncSize.excludedPhyPortTypes,
                         portExt->ring );
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.mirrorTruncSize);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.mirrorTruncSize);

        tmpUint32 = *( (fm_uint32 *) value );
        if ( ((tmpUint32 < FM10000_MIN_TRUNC_LEN) || 
              (tmpUint32 > FM10000_MAX_TRUNC_LEN)) &&
             (tmpUint32 != FM_PORT_DISABLE_TRUNCATION) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttrExt->mirrorTruncSize = tmpUint32 & ~0x3;

            err = UpdateTruncFrameSize(sw, port);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_PARSER_FIRST_CUSTOM_TAG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parserFirstCustomTag);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parserFirstCustomTag);

        tmpUint32 = *( (fm_uint32 *) value);
        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_2,
                                              CustomTag1) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_2,
                           CustomTag1,
                           tmpUint32);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->parserFirstCustomTag = tmpUint32;
        }
    }
    else if (attr == FM_PORT_PARSER_SECOND_CUSTOM_TAG)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parserSecondCustomTag);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parserSecondCustomTag);

        tmpUint32 = *( (fm_uint32 *) value);
        if ( tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_PARSER_PORT_CFG_2,
                                               CustomTag2) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_2,
                           CustomTag2,
                           tmpUint32);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->parserSecondCustomTag = tmpUint32;
        }
    }
    else if (attr == FM_PORT_PARSE_MPLS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parseMpls);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parseMpls);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_2,
                         ParseMPLS,
                         (tmpBool) ? 1 : 0);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->parseMpls = tmpBool;
        }
    }
    else if (attr == FM_PORT_PARSER_STORE_MPLS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.storeMpls);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.storeMpls);

        tmpInt = *( (fm_int *) value);
        if ( (tmpInt < 0) || (tmpInt > FM_MAX_NUM_HALF_WORDS_MPLS_TAG) )
        {
            err = FM_ERR_INVALID_VALUE;
            goto ABORT;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpInt);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadParserPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_FIELD64(reg64,
                           FM10000_PARSER_PORT_CFG_2,
                           StoreMPLS,
                           tmpInt);

            err = WriteParserPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->storeMpls = tmpInt;
        }
    }
    else if (attr == FM_PORT_ROUTED_FRAME_UPDATE_FIELDS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.routedFrameUpdateFields);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.routedFrameUpdateFields);

        tmpUint32 = *( (fm_uint32 *) value );

        allValidMask = FM_PORT_ROUTED_FRAME_UPDATE_DMAC |
                       FM_PORT_ROUTED_FRAME_UPDATE_SMAC |
                       FM_PORT_ROUTED_FRAME_UPDATE_VLAN;

        /* If any bits other than valid bits is set, then it is invalid */
        if ( (tmpUint32 & ~allValidMask) != 0)
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnableDMACRouting,
                         ( (tmpUint32 & FM_PORT_ROUTED_FRAME_UPDATE_DMAC) != 0) ? 1 : 0);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         EnableSMACRouting,
                         ( (tmpUint32 & FM_PORT_ROUTED_FRAME_UPDATE_SMAC) != 0) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            err = ReadModPerPortCfg1(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_1,
                         EnableVLANUpdate,
                         ( (tmpUint32 & FM_PORT_ROUTED_FRAME_UPDATE_VLAN) != 0) ? 1 : 0);

            err = WriteModPerPortCfg1(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->routedFrameUpdateFields = tmpUint32;
        }
    }
    else if (attr == FM_PORT_TCN_FIFO_WM)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.tcnFifoWm);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.tcnFifoWm);

        tmpUint32 = *( (fm_uint32 *) value );

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_MA_TCN_WM, Wm))
        {
            err = FM_ERR_INVALID_VALUE;
            goto ABORT;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            reg32 = 0;
            FM_SET_FIELD(reg32, FM10000_MA_TCN_WM, Wm, tmpUint32);

            FM_FLAG_TAKE_REG_LOCK(sw);

            err = switchPtr->WriteUINT32(sw, FM10000_MA_TCN_WM(physPort), reg32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->tcnFifoWm = tmpUint32;
        }
    }
    else if ( attr == FM_PORT_PCIE_MODE )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.pcieMode);
    }
    else if ( attr == FM_PORT_PCIE_SPEED )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.pcieSpeed);
    }
    else if ( attr == FM_PORT_PEP_MODE )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.pepMode);
    }
    else if (attr == FM_PORT_EYE_SCORE )
    {
        err = FM_ERR_READONLY_ATTRIB;
    }
    else if (attr == FM_PORT_TIMESTAMP_GENERATION)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.generateTimestamps);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.generateTimestamps);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_FLAG_TAKE_REG_LOCK(sw);

            err = switchPtr->ReadUINT32Mult(sw,
                                            FM10000_MAC_CFG(epl, channel, 0),
                                            FM10000_MAC_CFG_WIDTH,
                                             macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_ARRAY_SET_BIT(macCfg, FM10000_MAC_CFG, Ieee1588Enable, tmpBool);

            err = switchPtr->WriteUINT32Mult(sw,
                                             FM10000_MAC_CFG(epl, channel, 0),
                                             FM10000_MAC_CFG_WIDTH,
                                             macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->generateTimestamps = tmpBool;
        }
    }
    else if (attr == FM_PORT_EGRESS_TIMESTAMP_EVENTS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.egressTimestampEvents);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.egressTimestampEvents);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_FLAG_TAKE_REG_LOCK(sw);

            err = switchPtr->MaskUINT32(sw,
                                        FM10000_LINK_IM(epl, channel),
                                        FM10000_TIMESTAMP_INT_MASK,
                                        !tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* Update cached linkInterruptMask. */
            FM_SET_BIT(portExt->linkInterruptMask, FM10000_LINK_IM, EgressTimeStamp, tmpBool);

            portAttrExt->egressTimestampEvents = tmpBool;
        }
    }
    else if (attr == FM_PORT_EEE_MODE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.eeeEnable);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.eeeEnable);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( portAttrExt->eeeEnable != tmpBool )
            {
                /* Prepare the event notification for the port state machine */
                eventInfo.smType = portExt->smType;
                eventInfo.eventId = FM10000_PORT_EVENT_EEE_CONFIG_REQ;

                eventInfo.lock = FM_GET_STATE_LOCK( sw );
                eventInfo.dontSaveRecord = FALSE;

                /* notify the event */
                portExt->eventInfo.regLockTaken = FALSE;
                err = fmNotifyStateMachineEvent( portExt->smHandle,
                                                 &eventInfo,
                                                 &portExt->eventInfo,
                                                 &port );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                /* record the new value */
                portAttrExt->eeeEnable = *(fm_bool *) value;

                if (!portAttrExt->eeeEnable)
                {
                    /* EEE is disabled; clear the related NextPage context */
                    FM_SET_BIT64(portAttr->autoNegBasePage,
                                 FM10000_AN_73_BASE_PAGE_TX, NP, 0);
                    portAttrExt->eeeNextPageAdded = FALSE;
                    portAttrExt->negotiatedEeeModeEnabled = FALSE;
                    portAttr->autoNegNextPages.numPages = 0;
                    portAttr->autoNegNextPages.nextPages = NULL;
                }
            }
        }
    }
    else if (attr == FM_PORT_EEE_TX_ACTIVITY_TIMEOUT)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txPcActTimeout);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txPcActTimeout);

        tmpUint32 = *( (fm_uint32 *) value);
        if ( tmpUint32 > FM10000_PORT_EEE_TX_ACT_MAX_VAL)
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttrExt->txPcActTimescale = FM10000_PORT_EEE_TX_ACT_TIME_SCALE;
            portAttrExt->txPcActTimeout = tmpUint32 /
                                             FM10000_PORT_EEE_TX_ACT_TOUT_DIV;

            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32Mult(sw,
                                            FM10000_MAC_CFG(epl, channel, 0),
                                            FM10000_MAC_CFG_WIDTH,
                                            macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxPcActTimeout,
                                portAttrExt->txPcActTimeout );
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxPcActTimeScale,
                                portAttrExt->txPcActTimescale );

            err = switchPtr->WriteUINT32Mult(sw,
                                             FM10000_MAC_CFG(epl, channel, 0),
                                             FM10000_MAC_CFG_WIDTH,
                                             macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if (attr == FM_PORT_EEE_TX_LPI_TIMEOUT)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txLpiTimeout);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txLpiTimeout);

        tmpUint32 = *( (fm_uint32 *) value);
        if ( tmpUint32 > FM10000_PORT_EEE_TX_LPI_MAX_VAL)
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            portAttrExt->txLpiTimescale = FM10000_PORT_EEE_TX_LPI_TIME_SCALE;
            portAttrExt->txLpiTimeout = tmpUint32 /
                                           FM10000_PORT_EEE_TX_LPI_TOUT_DIV;

            err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT32Mult(sw,
                                            FM10000_MAC_CFG(epl, channel, 0),
                                            FM10000_MAC_CFG_WIDTH,
                                            macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxLpiTimeout,
                                portAttrExt->txLpiTimeout );
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxLpiTimescale,
                                portAttrExt->txLpiTimescale );

            err = switchPtr->WriteUINT32Mult(sw,
                                             FM10000_MAC_CFG(epl, channel, 0),
                                             FM10000_MAC_CFG_WIDTH,
                                             macCfg);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
    }
    else if ( attr == FM_PORT_IFG )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.ifg);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.ifg);

        tmpUint32 = *( (fm_uint32 *) value );

        if (tmpUint32 > FM_FIELD_UNSIGNED_MAX(FM10000_MAC_CFG, TxIdleMinIfgBytes))
        {
            err = FM_ERR_INVALID_VALUE;
            goto ABORT;
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000MapPhysicalPortToEplChannel( sw,
                                                      physPort,
                                                      &epl,
                                                      &channel );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);


            err = fm10000WriteMacIfg( sw,
                                      epl,
                                      channel,
                                      NULL,
                                      tmpUint32 );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->ifg = tmpUint32;
        }
    }
    else if ( attr == FM_PORT_DIC_ENABLE )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.dicEnable);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.dicEnable )

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = FM_OK;

            if ( portAttr->dicEnable != tmpBool )
            {
                pcsType = fm10000GetPcsType( portAttrExt->ethMode, portAttr->speed );
                switch (pcsType)
                {
                    case FM10000_PCS_SEL_10GBASER:
                    case FM10000_PCS_SEL_40GBASER:
                    case FM10000_PCS_SEL_100GBASER:
                        dicEnable = tmpBool;
                        break;

                    default:
                        dicEnable = FALSE;
                        break;

                }   /* end switch (pcsType) */

                err = fm10000MapPhysicalPortToEplChannel( sw,
                                                          physPort,
                                                          &epl,
                                                          &channel );
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "ethMode=0x%08x "
                                 "port=%d epl=%d, channel=%d, pcsType=%d dic=%d\n",
                                 portAttrExt->ethMode,
                                 port,
                                 epl,
                                 channel,
                                 pcsType,
                                 dicEnable );
                err = fm10000WriteMacDicEnable( sw,
                                                epl,
                                                channel,
                                                NULL,
                                                dicEnable );
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

                portAttr->dicEnable = tmpBool;
            }
        }
    }
    else if ( attr == FM_PORT_TX_PAD_SIZE )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txPadSize);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txPadSize );

        tmpUint32 = *( ( fm_uint32 *)value );

        if ( tmpUint32 < 40 ||
            tmpUint32 > 116 )
        {
            err = FM_ERR_INVALID_VALUE;
            goto ABORT;
        }

        /* The value must be a multiple of 4. Truncation to the closest lower 4-byte boundary */
        tmpUint32 = tmpUint32 & 0xFFFFFFFC;

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000MapPhysicalPortToEplChannel( sw, 
                                                      physPort, 
                                                      &epl,
                                                      &channel );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);


            err = fm10000WriteMacMinColumns( sw, 
                                             epl, 
                                             channel, 
                                             NULL, 
                                             *( ( fm_uint32 *) value ) );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->txPadSize = tmpUint32;
            err = FM_OK;
        }

    }
    else if (attr == FM_PORT_PARSER_VLAN2_FIRST)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.parserVlan2First);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.parserVlan2First);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_1,
                         Vlan2First,
                         (tmpBool) ? 1 : 0);
    
            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->parserVlan2First = tmpBool;
        }
    }
    else if (attr == FM_PORT_MODIFY_VID2_FIRST)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.modifyVid2First);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.modifyVid2First);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = ReadModPerPortCfg2(sw, physPort, &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_MOD_PER_PORT_CFG_2,
                         VID2First,
                         (tmpBool) ? 1 : 0);

            err = WriteModPerPortCfg2(sw, physPort, reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->modifyVid2First = tmpBool;
        }
    }
    else if (attr == FM_PORT_REPLACE_VLAN_FIELDS)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.replaceVlanFields);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.replaceVlanFields);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            FM_FLAG_TAKE_REG_LOCK(sw);

            err = switchPtr->ReadUINT64(sw,
                                        FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                        &reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            FM_SET_BIT64(reg64,
                         FM10000_PARSER_PORT_CFG_1,
                         useDefaultVLAN,
                         (tmpBool) ? 1 : 0);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_PARSER_PORT_CFG_1(physPort, 0),
                                         reg64);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttrExt->replaceVlanFields = tmpBool;
        }
    }
    else if ( attr == FM_PORT_TX_CLOCK_COMPENSATION )
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.txClkCompensation);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.txClkCompensation );

        tmpUint32 = *( ( fm_uint32 *)value );

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            err = fm10000MapPhysicalPortToEplChannel( sw,
                                                      physPort,
                                                      &epl,
                                                      &channel );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            pcsType = fm10000GetPcsType( portExt->ethMode, portExt->speed );
            err = fm10000WriteMacTxClockCompensation( sw,
                                                      epl,
                                                      channel,
                                                      NULL,
                                                      tmpUint32,
                                                      pcsType,
                                                      portExt->speed );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            portAttr->txClkCompensation = *((fm_uint32 *) value);
            err = FM_OK;
        }
    }
    else if (attr == FM_PORT_SMP_LOSSLESS_PAUSE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.smpLosslessPause);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.smpLosslessPause);

        tmpUint32 = *( (fm_uint32 *) value );

        if ( tmpUint32 > FM_PORT_SMP_ALL )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpUint32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( portAttr->smpLosslessPause != tmpUint32 )
            {
                portAttr->smpLosslessPause = tmpUint32;

                err = fm10000GetSwitchQOS(sw,
                                          FM_AUTO_PAUSE_MODE,
                                          0,
                                          &tmpBool);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

                if (tmpBool)
                {
                    /* recalculate the watermarks */
                    err = fm10000SetSwitchQOS(sw,
                                              FM_AUTO_PAUSE_MODE,
                                              0,
                                              &tmpBool);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
                }
            }
        }
    }
    else if (attr == FM_PORT_AUTODETECT_MODULE)
    {
        VALIDATE_ATTRIBUTE_WRITE_ACCESS(&portAttributeTable.autoDetectModule);
        VALIDATE_PORT_ATTRIBUTE(&portAttributeTable.autoDetectModule);

        VALIDATE_VALUE_IS_BOOL(value);

        tmpBool = *( (fm_bool *) value);

        if (isLagAttr)
        {
            err = SetLAGPortAttribute(sw, port, lane, attr, &tmpBool);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        }
        else
        {
            if ( portAttr->autoDetectModule != tmpBool )
            {
                portAttr->autoDetectModule = *(fm_bool *) value;

                /* Send the new attribute value to the platform layer. */
                err = fmPlatformNotifyPortAttribute(sw,
                                                    port,
                                                    0,
                                                    lane,
                                                    attr,
                                                    value);
            }
        }
    }
    else 
    {
       err = fmIsValidPortAttribute(attr) ? FM_ERR_UNSUPPORTED :
                                            FM_ERR_INVALID_ATTRIB;
    }

ABORT:
    if (toDelete)
    {
        err = fmDeleteBitArray(&portMaskBitArray);
        if (err != FM_OK)
        {
            FM_LOG_WARNING( FM_LOG_CAT_PORT,
                            "%s", fmErrorMsg(err) );
        }
    }

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    if (portAttrLockTaken)
    {
        FM_DROP_PORT_ATTR_LOCK(sw);
    }

    if (lagLockTaken)
    {
        FM_FLAG_DROP_LAG_LOCK(sw);
    }

    if ( err == FM_OK && restoreAdminMode )
    {
        err = fm10000SetPortState( sw, 
                                   port, 
                                   0, 
                                   portPtr->mode, 
                                   portPtr->submode );
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000SetPortAttributeInt */




/*****************************************************************************/
/** fm10000GetPortAttribute
 * \ingroup intPort
 *
 * \desc            Get a port attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       mac is the port's zero-based MAC number on which to operate.
 *                  May be specified as FM_PORT_ACTIVE_MAC to operate on the
 *                  currently selected active MAC (see the
 *                  ''FM_PORT_SELECT_ACTIVE_MAC'' port attribute). Must be
 *                  specified as either FM_PORT_ACTIVE_MAC or zero for ports
 *                  that have only one MAC (physical link connection).
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate.
 *
 * \param[in]       attribute is the port attribute (see 'Port Attributes')
 *                  to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_PORT_MAC if mac is not valid for the
 *                  specified port, or if FM_PORT_ACTIVE_MAC was
 *                  specified and the port has no active MAC selected.
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not valid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported in 
 *                  this switch.
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fm10000GetPortAttribute(fm_int sw,
                                  fm_int port,
                                  fm_int mac,
                                  fm_int lane,
                                  fm_int attribute,
                                  void * value)
{
    fm_status         err;
    fm_switch *       switchPtr;
    fm10000_switch *  switchExt;
    fm_port *         portPtr;
    fm10000_port *    portExt;
    fm_portAttr *     portAttr;
    fm_laneAttr *     laneAttr;
    fm10000_portAttr *portAttrExt;
    fm_portAttrEntry *attrEntry;
    fm_int            serdes;
#if 0
    fm_uint           uval;
#endif
    fm_uint32         uval32;
    fm_anNextPages   *nextPages;
    fm_uint32         i;
    fm10000_schedSpeedInfo speedInfo;
    fm_bool           isPciePort;

    FM_NOT_USED(mac);

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                 port,
                 "sw=%d port=%d lane=%d attribute=%d value=%p\n",
                 sw,
                 port,
                 lane,
                 attribute,
                 value);

    switchPtr   = GET_SWITCH_PTR(sw);
    switchExt   = GET_SWITCH_EXT(sw);
    portPtr     = GET_PORT_PTR(sw, port);
    portExt     = GET_PORT_EXT(sw, port);
    portAttr    = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);
    err         = FM_OK;

    if (fmIsCardinalPort(sw, port))
    {
        /**************************************************
         * Validate the lane numbers for
         * this port. Note that a lane value of
         * FM_PORT_LANE_NA will be accepted here, so per-lane
         * attributes must individually verify that lane
         * is _not_ set to FM_PORT_LANE_NA.
         **************************************************/

        err = ValidatePortLane(sw, port, lane);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else if (portPtr->portType == FM_PORT_TYPE_LAG)
    {
        /* For LAG port, return the cache value. */
        attrEntry = GetPortAttrEntry(attribute);

        if (attrEntry)
        {
            /* In per-LAG management mode only per-lag attributes
             * can be read on a lag. */
            if (!switchPtr->perLagMgmt || attrEntry->perLag)
            {
                err = GetCachedPortAttribute( sw,
                                              port,
                                              attribute,
                                              attrEntry,
                                              value );
            }
            else
            {
                err = FM_ERR_NOT_PER_LAG_ATTRIBUTE;
            }
        }
        else
        {
            err = fmIsValidPortAttribute(attribute) ? FM_ERR_UNSUPPORTED :
                                                      FM_ERR_INVALID_ATTRIB;
        }

        FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);
    }
    else if ( (portPtr->portType == FM_PORT_TYPE_VIRTUAL) &&
              (attribute == FM_PORT_DEF_VLAN) )
    {
        /* Do nothing */
    }
    else
    {
        FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                 port,
                 "sw=%d port=%d lane=%d attribute=%d value=%p\n",
                 sw,
                 port,
                 lane,
                 attribute,
                 value);

    if (lane == FM_PORT_LANE_NA)
    {
        lane = 0;
    }

    /* init serdes with an invalid value, to force an error if
     * a lane attribute is called using an invalid port type */
    serdes = -1;

    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
         || ( (portPtr->portType == FM_PORT_TYPE_CPU) ) )
    {
        err = fm10000MapPortLaneToSerdes(sw, port, lane, &serdes);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
        laneAttr = GET_LANE_ATTR(sw, serdes);

        err = fm10000IsPciePort(sw, port, &isPciePort);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else
    {
        laneAttr = NULL;
        isPciePort = FALSE;
    }

    switch (attribute)
    {
        case FM_PORT_MIN_FRAME_SIZE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.minFrameSize);
            *( (fm_int *) value ) = portAttr->minFrameSize;
            break;

        case FM_PORT_MAX_FRAME_SIZE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.maxFrameSize);
            *( (fm_int *) value ) = portAttr->maxFrameSize;
            break;

        case FM_PORT_SECURITY_ACTION:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.securityAction);
            *( (fm_uint32 *) value ) = portAttrExt->securityAction;
            break;

        case FM_PORT_LEARNING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.learning);
            *( (fm_bool *) value ) = portAttr->learning;
            break;

        case FM_PORT_INTERNAL:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.internal);
            *( (fm_bool *) value ) = portAttr->internalPort;
            break;

        case FM_PORT_TAGGING_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.taggingMode);
            *( (fm_portTaggingMode *) value ) = portAttrExt->taggingMode;
            break;

        case FM_PORT_TAGGING:
            err = FM_ERR_UNSUPPORTED;
            break;

        case FM_PORT_TAGGING2:
            err = FM_ERR_UNSUPPORTED;
            break;

        case FM_PORT_PARSER:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parser);
            *( (fm_int *) value ) = portAttr->parser;
            break;

        case FM_PORT_DROP_BV:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.dropBv);
            *( (fm_bool *) value ) = portAttr->dropBv;
            break;

        case FM_PORT_DEF_VLAN:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defVlan);
            *( (fm_uint32 *) value ) = portAttr->defVlan;
            break;

        case FM_PORT_DEF_VLAN2:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defVlan2);
            *( (fm_uint32 *) value ) = portAttr->defVlan2;
            break;

        case FM_PORT_DEF_PRI:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defVlanPri);
            *( (fm_uint32 *) value ) = portAttr->defVlanPri;
            break;

        case FM_PORT_DEF_PRI2:                                        
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defVlanPri2);
            *( (fm_uint32 *) value ) = portAttr->defVlanPri2;
            break;

        case FM_PORT_DEF_DSCP:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defDscp);
            *( (fm_uint32 *) value ) = portAttr->defDscp;
            break;

        case FM_PORT_DEF_SWPRI:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defSwpri);
            *( (fm_uint32 *) value ) = portAttr->defSwpri;
            break;

        case FM_PORT_DEF_ISL_USER:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defIslUser);
            *( (fm_uint32 *) value ) = portAttr->defIslUser;
            break;

        case FM_PORT_DROP_TAGGED:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.dropTagged);
            *( (fm_bool *) value ) = portAttr->dropTagged;
            break;

        case FM_PORT_DROP_UNTAGGED:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.dropUntagged);
            *( (fm_bool *) value ) = portAttr->dropUntagged;
            break;

        case FM_PORT_PARSER_VLAN1_TAG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parserVlan1Tag);
            *( (fm_int *) value ) = portAttrExt->parserVlan1Tag;
            break;

        case FM_PORT_PARSER_VLAN2_TAG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parserVlan2Tag);
            *( (fm_int *) value ) = portAttrExt->parserVlan2Tag;
            break;

        case FM_PORT_MODIFY_VLAN1_TAG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.modifyVlan1Tag);
            *( (fm_int *) value ) = portAttrExt->modifyVlan1Tag;
            break;

        case FM_PORT_MODIFY_VLAN2_TAG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.modifyVlan2Tag);
            *( (fm_int *) value ) = portAttrExt->modifyVlan2Tag;
            break;

        case FM_PORT_MIRROR_TRUNC_SIZE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.mirrorTruncSize);
            *( (fm_int *) value ) = portAttrExt->mirrorTruncSize;
            break;

        case FM_PORT_TXCFI:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txCfi);
            *( (fm_uint32 *) value ) = portAttr->txCfi;
            break;

        case FM_PORT_TXCFI2:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txCfi2);
            *( (fm_uint32 *) value ) = portAttr->txCfi2;
            break;

        case FM_PORT_SWPRI_SOURCE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.swpriSource);
            *( (fm_uint32 *) value ) = portAttr->swpriSource;
            break;

        case FM_PORT_SWPRI_DSCP_PREF:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.swpriDscpPref);
            *( (fm_bool *) value ) = portAttr->swpriDscpPref;
            break;

        case FM_PORT_PARSER_FLAG_OPTIONS:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parserFlagOptions);
            *( (fm_uint32 *) value ) = portAttr->parserFlagOptions;
            break;
    
        case FM_PORT_UPDATE_TTL:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.updateTtl);
            *( (fm_bool *) value ) = portAttr->updateTtl;
            break;

        case FM_PORT_UPDATE_DSCP:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.updateDscp);
            *( (fm_bool *) value ) = portAttr->updateDscp;
            break;

        case FM_PORT_TXVPRI:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txVpri);
            *( (fm_bool *) value ) = portAttr->txVpri;
            break;

        case FM_PORT_TXVPRI2:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txVpri2);
            *( (fm_bool *) value ) = portAttr->txVpri2;
            break;

        case FM_PORT_DEF_CFI:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.defCfi);
            *( (fm_uint32 *) value ) = portAttr->defCfi;
            break;

        case FM_PORT_REPLACE_DSCP:                                         
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.replaceDscp);
            *( (fm_bool *) value ) = portAttr->replaceDscp;
            break;
 
        case FM_PORT_ISL_TAG_FORMAT:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.islTagFormat);
            *( (fm_islTagFormat *) value ) = portAttr->islTagFormat;
            break;

        case FM_PORT_BCAST_FLOODING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.bcastFlooding);
            *( (fm_int *) value) = portAttr->bcastFlooding;
            break;

        case FM_PORT_BCAST_PRUNING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.bcastPruning);
            *( (fm_int *) value) = portAttr->bcastPruning;
            break;

        case FM_PORT_MCAST_FLOODING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.mcastFlooding);
            *( (fm_int *) value) = portAttr->mcastFlooding;
            break;

        case FM_PORT_MCAST_PRUNING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.mcastPruning);
            *( (fm_int *) value) = portAttr->mcastPruning;
            break;

        case FM_PORT_UCAST_FLOODING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.ucastFlooding);
            *( (fm_int *) value) = portAttr->ucastFlooding;
            break;

        case FM_PORT_UCAST_PRUNING:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.ucastPruning);
            *( (fm_int *) value) = portAttr->ucastPruning;
            break;

        case FM_PORT_ROUTABLE:                                         
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.routable);
            *( (fm_bool *) value ) = portAttr->routable;
            break;

        case FM_PORT_MASK_WIDE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.maskWide);
            err = fmPortMaskToBitArray(&portAttr->portMask,
                                          (fm_bitArray *) value,
                                          switchPtr->numCardinalPorts);
            break;
    
        case FM_PORT_DOT1X_STATE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.dot1xState);
            *( (fm_dot1xState *) value ) = portAttr->dot1xState;
            break;

        case FM_PORT_TX_PAUSE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txPause);
            *( (fm_uint32 *) value ) = portAttr->txPause;
            break;

        case FM_PORT_RX_PAUSE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.rxPause);
            *( (fm_bool *) value ) = portAttr->rxPause;
            break;

        case FM_PORT_TX_PAUSE_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txPauseMode);
            *( (fm_uint32 *) value ) = portAttr->txPauseMode;
            break;

        case FM_PORT_TX_PAUSE_RESEND_TIME:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txPauseResendTime);
            err = fm10000GetPauseResendInterval(sw, port, &uval32);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            *( (fm_uint32 *) value ) = uval32;
            break;

        case FM_PORT_RX_CLASS_PAUSE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.rxClassPause);
            *( (fm_uint32 *) value ) = portAttr->rxClassPause;
            break;

        case FM_PORT_TX_CLASS_PAUSE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txClassPause);
            *( (fm_uint32 *) value ) = portAttr->txClassPause;
            break;

        case FM_PORT_LOOPBACK_SUPPRESSION:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.loopbackSuppression);
            *( (fm_bool *) value ) = portAttr->loopbackSuppression;
            break;    

        case FM_PORT_ROUTED_FRAME_UPDATE_FIELDS:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.routedFrameUpdateFields);
            *( (fm_uint32 *) value ) = portAttrExt->routedFrameUpdateFields;
            break;

        case FM_PORT_PARSER_FIRST_CUSTOM_TAG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parserFirstCustomTag);
           *( (fm_int *) value ) = portAttrExt->parserFirstCustomTag;
            break;
 
        case FM_PORT_PARSER_SECOND_CUSTOM_TAG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parserSecondCustomTag);
           *( (fm_int *) value ) = portAttrExt->parserSecondCustomTag;
            break;

        case FM_PORT_PARSE_MPLS:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.parseMpls);
            *( (fm_bool *) value ) = portAttrExt->parseMpls;
            break;

        case FM_PORT_PARSER_STORE_MPLS:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.storeMpls);
            *( (fm_int *) value ) = portAttrExt->storeMpls;
            break; 

        case FM_PORT_TCN_FIFO_WM:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.tcnFifoWm);
            *( (fm_uint32 *) value ) = portAttrExt->tcnFifoWm;
            break;
 
        case FM_PORT_TX_LANE_CURSOR:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneCursor);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->cursor;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_PRECURSOR:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLanePreCursor);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->preCursor;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_POSTCURSOR:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLanePostCursor);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->postCursor;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_KR_INIT_CURSOR:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneKrInitCursor);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->initializeCursor;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_KR_INIT_PRECURSOR:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneKrInitPreCursor);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->initializePreCursor;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_KR_INIT_POSTCURSOR:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneKrInitPostCursor);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->initializePostCursor;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_KR_INITIAL_PRE_DEC:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneKrInitialPreDec);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->preCursorDecOnPreset;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_KR_INITIAL_POST_DEC:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneKrInitialPostDec);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->postCursorDecOnPreset;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_TX_LANE_ENA_KR_INIT_CFG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLaneEnableConfigKrInit);
            if ( laneAttr )
            {
                *( (fm_int *) value ) = laneAttr->txLaneEnableConfigKrInit;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }

            break;

        case FM_PORT_RX_LANE_POLARITY:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.rxLanePolarity);
            if ( laneAttr )
            {
                /* 0: disabled; 1: enabled */
                *( (fm_uint32 *) value ) = (laneAttr->rxPolarity ? 1 : 0);
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

        case FM_PORT_TX_FCS_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txFcsMode);

            /* convert from FM10000 values to generic values */
            for (i = 0 ; i < FM_NENTRIES(txFcsModeMap) ; i++)
            {
                if (portAttrExt->txFcsMode == txFcsModeMap[i].b)
                {
                    *( (fm_uint32 *) value ) = txFcsModeMap[i].a;
                    break;
                }
            }

            /* If we reached end of iteration loop, we did not find
             * a match */
            if (i == FM_NENTRIES(txFcsModeMap))
            {
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            break;

        case FM_PORT_TX_LANE_POLARITY:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLanePolarity);
            if ( laneAttr )
            {
                /* 0: disabled; 1: enabled */
                *( (fm_uint32 *) value ) = (laneAttr->txPolarity ? 1 : 0);
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

        case FM_PORT_LOOPBACK:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.serdesLoopback);
            *((fm_int *) value) = portAttr->serdesLoopback;
            break;

        case FM_PORT_FABRIC_LOOPBACK:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.fabricLoopback);
            *((fm_int *) value) = portAttrExt->fabricLoopback;
            break;

        case FM_PORT_ETHERNET_INTERFACE_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.ethMode);
             *( (fm_ethMode *) value ) = portAttrExt->ethMode;
            break;

        case FM_PORT_SPEED:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.speed);
            if ( ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
                   || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (port != 0) ) )
                 && !isPciePort )
            {
                *( (fm_uint32 *) value ) = portExt->speed;
            }
            else
            {
                err =  fm10000GetSchedPortSpeed(sw, 
                                                port, 
                                                &speedInfo);

                *( (fm_int *) value) = speedInfo.assignedSpeed;
            }
            break;

        case FM_PORT_RX_TERMINATION:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.rxTermination);
            if ( laneAttr )
            {
                if (isPciePort)
                {
                    /* Get directly from HW at request time */
                    err = GetRxTermination(sw, serdes, &laneAttr->rxTermination);
                    
                }
                *( (fm_int *) value) = laneAttr->rxTermination;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

        case FM_PORT_DFE_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.dfeMode);
            if ( laneAttr )
            {
                *( (fm_int *) value) = laneAttr->dfeMode;
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

        case FM_PORT_COARSE_DFE_STATE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.coarseTuning);
            /*Fall through to next case */
        case FM_PORT_FINE_DFE_STATE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.fineTuning);

            *( (fm_uint32 *) value) = 0;

            if ( laneAttr )
            {
                err = fm10000SerdesGetTuningState(sw, serdes, &uval32);
                
                if (err == FM_OK)
                {
                    if (attribute == FM_PORT_FINE_DFE_STATE)
                    {
                        /* fine tuning status is given by bits 3::2 */
                        uval32 >>= 2;
                    }

                    *( (fm_uint32 *) value) = uval32 & 0x03;
                }
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

#if 0
        case FM_PORT_DFE_PARAMETERS:
            /* Probably need new attribute instead */
            if ( laneAttr )
            {
                *( (fm_uint32 *) value) = 0;

                /* Do not access serdes internal values, use
                 * the values from the dfe structure instead */
                uval = 0;
                /* DC */
                *( (fm_uint32 *) value) |= ((uval & 0xFF) << 24);

                /* LF */
                *( (fm_uint32 *) value) |= ((uval & 0xFF) << 16);

                /* HF */
                *( (fm_uint32 *) value) |= ((uval & 0xFF) << 8);

                /* BW */
                *( (fm_uint32 *) value) |= ((uval & 0xFF) << 0);
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
                /* Remove when TestPoint show port is fixed */
                if ( portPtr->portType == FM_PORT_TYPE_TE || portPtr->portType == FM_PORT_TYPE_LOOPBACK )
                {
                    *( (fm_uint32 *) value) = 0;
                    err = FM_OK;
                }
            }
            break;
#endif

        case FM_PORT_BIST_USER_PATTERN_LOW40:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.bistUserPatterLow40);
            if ( laneAttr )
            {
                err = fm10000GetBistUserPattern(sw,port,lane,(fm_uint64 *)value, NULL);
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

        case FM_PORT_BIST_USER_PATTERN_UPP40:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.bistUserPatterUpp40);
            if ( laneAttr )
            {
                err = fm10000GetBistUserPattern(sw,port,lane,NULL, (fm_uint64 *)value);
            }
            else
            {
                err = FM_ERR_INVALID_PORT_LANE;
            }
            break;

        case FM_PORT_TX_CUT_THROUGH:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.enableTxCutThrough);
            *( (fm_bool *) value ) = portAttr->enableTxCutThrough;
            break;

        case FM_PORT_RX_CUT_THROUGH:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.enableRxCutThrough);
            *( (fm_bool *) value ) = portAttr->enableRxCutThrough;
            break;

        case FM_PORT_PCIE_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.pcieMode);
            if ( !portPtr->linkUp )
            {
                *(fm_uint32 *) value = FM_PORT_PCIE_MODE_DISABLED;
            }
            else
            {
                err = GetPcieModeFromReg(sw, port,(fm_uint32 *) value);
            }
            break;

        case FM_PORT_PCIE_SPEED:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.pcieSpeed);
            if ( !portPtr->linkUp )
            {
                *(fm_uint32 *) value = FM_PORT_PCIE_SPEED_UNKNOWN;
            }
            else
            {
                err = GetPcieSpeedFromReg(sw, port, (fm_uint32 *) value);
            }
            break;

        case FM_PORT_PEP_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.pepMode);
            *( (fm_pepMode *) value ) = portAttrExt->pepMode;
            break;

        case FM_PORT_AUTONEG:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegMode);
            *( (fm_uint32 *) value ) = portAttr->autoNegMode;
            break;

        case FM_PORT_AUTONEG_PARTNER_BASEPAGE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegPartnerBasePage); 
            *( (fm_uint64 *) value ) = portAttr->autoNegPartnerBasePage;
            break;

        case FM_PORT_AUTONEG_BASEPAGE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegBasePage);
            *( (fm_uint64 *) value ) = portAttr->autoNegBasePage;
            break;

       case FM_PORT_AUTONEG_PARTNER_NEXTPAGES:
           VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegPartnerNextPages);
            nextPages = (fm_anNextPages *) value;
            nextPages->numPages = portAttr->autoNegPartnerNextPages.numPages;
            if (nextPages->numPages > 0)
            {
                nextPages->nextPages = fmAlloc( sizeof(fm_uint64) * 
                                                nextPages->numPages );
                if (!nextPages->nextPages)
                {
                    err = FM_ERR_NO_MEM;
                    break;
                }
                FM_MEMCPY_S( nextPages->nextPages,
                             sizeof(fm_uint64) * nextPages->numPages,
                             portAttr->autoNegPartnerNextPages.nextPages,
                             sizeof(fm_uint64) * nextPages->numPages );
            }
            break;

        case FM_PORT_AUTONEG_NEXTPAGES:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegNextPages);
            nextPages = (fm_anNextPages *) value;
            nextPages->numPages = portAttr->autoNegNextPages.numPages;
            if (nextPages->numPages > 0)
            {
                nextPages->nextPages = fmAlloc( sizeof(fm_uint64) * 
                                                nextPages->numPages );
                if (!nextPages->nextPages)
                {
                    err = FM_ERR_NO_MEM;
                    break;
                }
                FM_MEMCPY_S( nextPages->nextPages,
                             sizeof(fm_uint64) * nextPages->numPages,
                             portAttr->autoNegNextPages.nextPages,
                             sizeof(fm_uint64) * nextPages->numPages );
            }
            break;

        case FM_PORT_AUTONEG_25G_NEXTPAGE_OUI:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNeg25GNxtPgOui);
            *( (fm_uint32 *) value ) = portAttr->autoNeg25GNxtPgOui;
            break;

        case FM_PORT_AUTONEG_LINK_INHB_TIMER:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegLinkInhbTimer);
            *( (fm_uint32 *) value ) = portAttrExt->autoNegLinkInhbTimer;
            break;

        case FM_PORT_AUTONEG_LINK_INHB_TIMER_KX:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegLinkInhbTimerKx);
            *( (fm_uint32 *) value ) = portAttrExt->autoNegLinkInhbTimerKx;
            break;

        case FM_PORT_AUTONEG_IGNORE_NONCE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoNegIgnoreNonce);
            *( (fm_uint32 *) value ) = portAttrExt->autoNegIgnoreNonce;
            break;

        case FM_PORT_EYE_SCORE:
            err = fm10000GetPortLaneEyeScore(sw, port, lane, (fm_uint32 *) value);
            break;

        case FM_PORT_TIMESTAMP_GENERATION:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.generateTimestamps);
            *( (fm_bool *) value ) = portAttrExt->generateTimestamps;
            break;

        case FM_PORT_EGRESS_TIMESTAMP_EVENTS:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.egressTimestampEvents);
            *( (fm_bool *) value ) = portAttrExt->egressTimestampEvents;
            break;
            
        case FM_PORT_EEE_MODE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.eeeEnable);
            *( (fm_bool *) value ) = portAttrExt->eeeEnable;
            break;

        case FM_PORT_EEE_STATE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.eeeState);
            err = GetPortLpiStatus(sw, port, (fm_int *) value);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
            portAttrExt->eeeState = *((fm_int *) value);
            break;

        case FM_PORT_EEE_TX_ACTIVITY_TIMEOUT:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txPcActTimeout);
            *( (fm_uint *) value ) = portAttrExt->txPcActTimeout * 
                                     FM10000_PORT_EEE_TX_ACT_TOUT_DIV;
            break;

        case FM_PORT_EEE_TX_LPI_TIMEOUT:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.txLpiTimeout);
            *( (fm_uint *) value ) = portAttrExt->txLpiTimeout * 
                                     FM10000_PORT_EEE_TX_LPI_TOUT_DIV;
            break;
            
        case FM_PORT_IFG:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.ifg );
            *( (fm_uint32 *) value ) = portAttr->ifg;
            break;

        case FM_PORT_DIC_ENABLE:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.dicEnable );
            *( (fm_bool *) value ) = portAttr->dicEnable;
            break;

        case FM_PORT_TX_PAD_SIZE:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.txPadSize );
            *( (fm_uint32 *) value ) = portAttrExt->txPadSize;
            break;

        case FM_PORT_PARSER_VLAN2_FIRST:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.parserVlan2First );
            *( (fm_bool *) value ) = portAttrExt->parserVlan2First;
            break;

        case FM_PORT_MODIFY_VID2_FIRST:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.modifyVid2First );
            *( (fm_bool *) value ) = portAttrExt->modifyVid2First;
            break;
        
        case FM_PORT_REPLACE_VLAN_FIELDS:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.replaceVlanFields);
            *( (fm_bool *) value ) = portAttrExt->replaceVlanFields;
            break;
        
        case FM_PORT_LINK_INTERRUPT:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.linkInterruptEnabled);
            *( (fm_bool *) value ) = portAttr->linkInterruptEnabled;
            break;
        
        case FM_PORT_IGNORE_IFG_ERRORS:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.ignoreIfgErrors);
            *( (fm_bool *) value ) = portAttr->ignoreIfgErrors;
            break;

        case FM_PORT_TX_CLOCK_COMPENSATION:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.txClkCompensation );
            *( (fm_uint32  *) value ) = portAttr->txClkCompensation;
            break;

        case FM_PORT_SMP_LOSSLESS_PAUSE:
            VALIDATE_ATTRIBUTE_READ_ACCESS( &portAttributeTable.smpLosslessPause );
            *( (fm_uint32  *) value ) = portAttr->smpLosslessPause;
            break;

        case FM_PORT_AUTODETECT_MODULE:
            VALIDATE_ATTRIBUTE_READ_ACCESS(&portAttributeTable.autoDetectModule);
            *( (fm_bool *) value ) = portAttr->autoDetectModule;
            break;

        default:
            err = fmIsValidPortAttribute(attribute) ? FM_ERR_UNSUPPORTED :
                                                      FM_ERR_INVALID_ATTRIB;
    }

ABORT:

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000GetPortAttribute */


/*****************************************************************************/
/** fm10000GetPortState
 * \ingroup intPort
 *
 * \desc            Retrieve a port's state information.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port for which to retrieve state information.
 *
 * \param[in]       mac identifies which of the port's MACs to operate on.
 *
 * \param[in]       numBuffers is the size of caller-provided info array where
 *                  this function will return per-lane information
 * 
 * \param[out]      numLanes is the pointer to a caller-provided area where
 *                  this function will return the number of lanes for which
 *                  the info buffer was provided. If the function had information
 *                  about more lanes than buffers provided by the caller, it
 *                  will fill out all available info entries and return
 *                  ''FM_ERR_BUFFER_FULL''
 *                
 * \param[out]      mode points to caller-allocated storage where this
 *                  function should place the port's mode (see Port State) as
 *                  set in a prior call to fmSetPortState.
 *
 * \param[out]      state points to caller-allocated storage where this
 *                  function should place the port's state (see 'Port States').
 *
 * \param[out]      info points to a caller-allocated array where this
 *                  function will place mode-specific information, indexed
 *                  by lane number:
 *                      - For mode = FM_PORT_MODE_BIST:
 *                          info holds the contents of FM10000_SERDES_BIST_ERR_CNT,
 *                          being the number of errors, indexed by lane. A
 *                          value of zero indicates that the corresponding
 *                          lane is working. The port is in the BIST mode, only if
 *                          the port's state is reported as UP. Otherwise, the
 *                          port is waiting for the BIST to be enabled and info 
 *                          hold the port lane status as below.
 *                      - For all other modes, info holds the port lane status
 *                          (see 'Port Lane Status').
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * 
 * \return          FM_ERR_BUFFER_FULL if the function had more lanes to report
 *                  on than info entries provided by the caller
 * 
 *****************************************************************************/
fm_status fm10000GetPortState( fm_int   sw,
                               fm_int   port,
                               fm_int   mac,
                               fm_int   numBuffers,
                               fm_int  *numLanes,
                               fm_int  *mode,
                               fm_int  *state,
                               fm_int  *info )
{
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_port *        portPtr;
    fm10000_port *   portExt;
    fm10000_portAttr *portAttrExt;
    fm_int           totalLanes;
    fm_uint32        pcsRxStatus;
    fm_bool          rxPllRdy;
    fm_bool          txPllRdy;
    fm_bool          rxSignalOk;
    fm_bool          rxSigStrengthEn;
    fm_bool          align40G;
    fm_int           i;
    fm_int           serdes = 0;
    fm_bool          isBistActive;
    fm_bool          truncated = FALSE;
    fm_bool          isDisabled = TRUE;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, 
                     port,
                     "sw=%d port=%d mac=%d numBuffers=%d "
                     "numLanes=%p mode=%p state=%p info=%p\n",
                     sw,
                     port,
                     mac,
                     numBuffers,
                     (void *)numLanes,
                     (void *) mode,
                     (void *) state,
                     (void *) info );

    FM_NOT_USED(mac);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);
    portPtr   = GET_PORT_PTR(sw, port);

    err = fm10000IsPortDisabled( sw, port, 0, &isDisabled );
    if ( err != FM_OK || isDisabled == TRUE )
    {
        /* WM send link up on disabled port */
        *mode  = portPtr->mode;
        *state = FM_PORT_STATE_DOWN;
        goto ABORT;
    }

    portExt   = GET_PORT_EXT(sw, port);
    if ( portExt->ring == FM10000_SERDES_RING_NONE )
    {
        /* Special internal ports */
        *state = FM_PORT_STATE_UP;
        info[0] = 0;
        goto ABORT;
    }

    err = fm10000GetNumLanes( sw, port, &totalLanes ); 
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    switchPtr = GET_SWITCH_PTR(sw);
    *mode     = portPtr->mode;

    if ( numBuffers < totalLanes )
    {
        *numLanes = numBuffers;
        truncated = TRUE;
    }
    else
    {
        *numLanes = totalLanes;
    }

    if (*mode == FM_PORT_MODE_BIST)
    {
        err = fm10000IsPortBistActive(sw, port,&isBistActive);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else
    {
        /* Could be called on CPU port */
        isBistActive = FALSE;
    }

    /* Read the number of errors for BIST per lane if in rx or txrx submode */
#if 0
/* isBistActive is not managed by the states machines, review this
 * also fm10000IsPortBistActive accesses lane structures from port level:
 * it should call a function at serdes level instead */
    if (*mode == FM_PORT_MODE_BIST && isBistActive == TRUE)
#endif
    if (*mode == FM_PORT_MODE_BIST)
    {
        if ( IsBistRxSubmode(portPtr->submode) )
        {
            for ( i = 0 ; i < *numLanes  ; i++ )
            {
                err = fm10000MapPortLaneToSerdes(sw, port, i, &serdes);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err );
                err = fm10000GetSerdesErrorCounter(sw, serdes, (fm_uint32*) &info[i]);
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err );
            }
        }
    }
    else
    {
        for ( i = 0 ; i < *numLanes  ; i++ )
        {
            info[i] = 0;

            err = fm10000MapPortLaneToSerdes(sw, port, i, &serdes);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err );

            /* Serdes PLL status */
            err = fm10000SerdesGetTxRxReadyStatus(sw, serdes, &txPllRdy, &rxPllRdy);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            /* report rxSigStrengthEn when receiver is enabled */
            rxSigStrengthEn = (rxPllRdy != 0);

            /* Serdes signal strength status */
            err = fm10000SerdesGetSignalOk(sw, serdes, &rxSignalOk);
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

            info[i] |= ((rxPllRdy ? 1 : 0) << 0);
            info[i] |= ((txPllRdy ? 1 : 0) << 1);
            /* Similar to FM6000 convention */
            info[i] |= ((rxSigStrengthEn ? 1 : 0) << 2);
            info[i] |= ((rxSignalOk ? 3 : 0) << 3); 
        }

        /* This needs to be implemented */
        align40G = TRUE;
        pcsRxStatus = 0;

        if ( align40G )
        {
            {
                if ( numBuffers > 0 )
                {
                    info[0] |= (1 << 5);
                }
            }
        }
    }

    if (portPtr->linkUp)
    {
        *state = FM_PORT_STATE_UP;
    }
    else
    {
        *state = FM_PORT_STATE_DOWN;
    }

    if (*mode == FM_PORT_MODE_BIST)
    {
        *state = (isBistActive == TRUE) ? FM_PORT_STATE_UP : FM_PORT_STATE_DOWN;
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    if ( err == FM_OK && truncated == TRUE )
    {
        err = FM_ERR_BUFFER_FULL;
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000GetPortState */


/*****************************************************************************/
/** fm10000SetPortState
 * \ingroup intPort
 *
 * \desc            Set the state of a port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       mac identifies which of the port's MACs to operate on
 *                  (does not apply to FM10000 devices).
 *
 * \param[in]       mode indicates the port state (see 'Port States') to set.
 *
 * \param[in]       submode is the port submode (see 'Port State Submodes')
 *                  and is used only if mode is FM_PORT_STATE_BIST.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetPortState( fm_int sw,
                               fm_int port,
                               fm_int mac,
                               fm_int mode,
                               fm_int submode )
{
    fm_port         *portPtr;
    fm_portAttr     *portAttr;
    fm10000_port    *portExt;
    fm10000_portAttr *portAttrExt;
    fm_status        err = FM_OK;
    fm_smEventInfo   eventInfo;
    fm_int           serDes;
    fm_serdesRing    ring;
    fm_bool          sendUpdate;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw=%d, port=%d, mode=%d, submode=%d\n",
                     sw,
                     port,
                     mode,
                     submode);

    FM_NOT_USED(mac);

    portPtr = GET_PORT_PTR( sw, port );


    /* Update LAGs depending on if port is going UP or not. */
    if (mode != FM_PORT_STATE_UP)
    {
        /* Setting the port not UP - remove from all LAGs. */
        err = fmInformLAGPortDown(sw, port);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    if (!fmPlatformBypassEnabled(sw))
    {
        portPtr = GET_PORT_PTR( sw, port );
        portAttr = GET_PORT_ATTR( sw, port );
        portExt = GET_PORT_EXT( sw, port );
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

        if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
             || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (port != 0) ) )
        {
            err = fm10000MapPhysicalPortToSerdes( sw,
                                                  portPtr->physicalPort,
                                                  &serDes,
                                                  &ring );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
            if( ring == FM10000_SERDES_RING_EPL )
            {

                /* Unconditionally configure PHY interface as UP or not. */
                if (mode == FM_PORT_MODE_UP || mode == FM_PORT_MODE_BIST)
                {
                    if ( (portPtr->phyInfo.phyEnable != NULL) &&
                         (portAttrExt->ethMode != FM_ETH_MODE_DISABLED) )
                    {
                        err = portPtr->phyInfo.phyEnable(sw, portPtr->physicalPort, 0, portPtr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
                    }
                }
                else
                {
                    if (portPtr->phyInfo.phyDisable != NULL)
                    {
                        err = portPtr->phyInfo.phyDisable(sw, portPtr->physicalPort, 0, portPtr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
                    }
                }

                portExt->eventInfo.info.admin.mode    = mode;
                portExt->eventInfo.info.admin.submode = submode;

                eventInfo.smType = portExt->smType;

                /* map the desired mode to the port-level state machine event ID */
                switch ( mode )
                {
                    case FM_PORT_MODE_ADMIN_DOWN:
                        eventInfo.eventId = FM10000_PORT_EVENT_ADMIN_DOWN_REQ;
                        break;

                    case FM_PORT_MODE_REMOTE_FAULT:
                        eventInfo.eventId = FM10000_PORT_EVENT_REMOTE_FAULT_REQ;
                        break;

                    case FM_PORT_MODE_LOCAL_FAULT:
                        eventInfo.eventId = FM10000_PORT_EVENT_LOCAL_FAULT_REQ;
                        break;

                    case FM_PORT_MODE_ADMIN_PWRDOWN:
                        eventInfo.eventId = FM10000_PORT_EVENT_ADMIN_PWRDOWN_REQ;
                        break;

                    case FM_PORT_MODE_UP:
                        eventInfo.eventId = FM10000_PORT_EVENT_ADMIN_UP_REQ;
                        break;

                    case FM_PORT_MODE_BIST:
                        eventInfo.eventId = FM10000_PORT_EVENT_BIST_REQ;
                        break;

                    default:
                        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                                         port, 
                                         "Invalid admin mode: %d\n",
                                         mode );
                        err = FM_ERR_INVALID_ARGUMENT;
                        goto ABORT;

                }       /* end switch ( mode ) */

                /* notify the admin mode change event */
                eventInfo.lock = FM_GET_STATE_LOCK( sw );
                eventInfo.dontSaveRecord = FALSE;
                portExt->eventInfo.regLockTaken = FALSE;
                err = fmNotifyStateMachineEvent( portExt->smHandle,
                                                 &eventInfo,
                                                 &portExt->eventInfo,
                                                 &port );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                portPtr->mode    = mode;
                portPtr->submode = submode;
            }
            else
            {
                /*********************************************************
                 * Dynamic management of PCIE ports isn't supported 
                 *  
                 * physical ports to send a link up event as soon as there
                 * is a change in admin mode. To be removed once we have 
                 * fully functional port/serdes state machines  
                 *********************************************************/

                if ( mode == FM_PORT_MODE_UP )
                {
                    portPtr->mode = mode;
                    portPtr->submode = submode;
                    err = FM_OK;
                }
                else
                {
                    FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                                     port,
                                     "Mode %d isn't supported for PCIE ports\n",
                                     mode );
                    err = FM_ERR_UNSUPPORTED;
                }

            }

        }
        else if ( portPtr->portType == FM_PORT_TYPE_TE || 
                  portPtr->portType == FM_PORT_TYPE_LOOPBACK ||
                  portPtr->portType == FM_PORT_TYPE_PTI )
        {
            /* send a link status message for TE and Loopback ports
               depending on the admin mode */
            portPtr->mode = mode;
            portPtr->submode = submode;

            sendUpdate = FALSE;
            if ( mode == FM_PORT_MODE_UP && !portPtr->linkUp )
            {
                portPtr->linkUp = TRUE;
                sendUpdate = TRUE;
            }

            if ( mode != FM_PORT_MODE_UP && portPtr->linkUp )
            {
                portPtr->linkUp = FALSE;
                sendUpdate = TRUE;
            }

            if ( sendUpdate == TRUE )
            {
                err = fm10000SendLinkUpDownEvent( sw, 
                                                  portPtr->physicalPort, 
                                                  mac,
                                                  portPtr->linkUp,
                                                  FM_EVENT_PRIORITY_LOW );
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err)
            }
            
        }

    }   /* end if (!fmPlatformBypassEnabled(sw)) */

ABORT:
    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000SetPortState */




/*****************************************************************************/
/** fm10000GetPortLowLevelState
 * \ingroup intPort
 * 
 * \desc            Returns the low level port state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[out]      portState returned low level port state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if portState pointer is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetPortLowLevelState(fm_int  sw,
                                      fm_int  port,
                                      fm_int *portState)
{
    fm_status     err;
    fm_int        serDes;
    fm10000_lane *pLaneExt;
    fm_portAttr  *portAttr;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, port, "sw=%d port=%d\n", sw, port);

    if (portState == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }
    
    err = FM_OK;
    
    PROTECT_SWITCH(sw);
    err = fm10000MapPortLaneToSerdes(sw, port, 0, &serDes);
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    
    pLaneExt = GET_LANE_EXT(sw, serDes);
    portAttr = GET_PORT_ATTR(sw, port);
                
    err = fmGetStateMachineCurrentState(pLaneExt->smHandle, portState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, 
                    port,
                    "port=%d portState=%s farLoopback=%s\n",
                    port,
                    fm10000PortStatesMap[*portState],
                    (portAttr->serdesLoopback == FM_PORT_LOOPBACK_RX2TX) ? 
                    "On" : "Off");

ABORT:
    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000GetPortLowLevelState */




/*****************************************************************************/
/** fm10000GetCpuPort
 * \ingroup intPort
 *
 * \desc            Returns the logical port number for the CPU port for
 *                  the specified switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      cpuPort is the logical port number for the CPU port.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
fm_status fm10000GetCpuPort(fm_int sw, fm_int *cpuPort)
{
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    *cpuPort = switchPtr->cpuPort;

    return FM_OK;

}   /* end fm10000GetCpuPort */




/*****************************************************************************/
/** fm10000SetCpuPort
 * \ingroup intPort
 *
 * \desc            Set the logical port number for the CPU port for
 *                  the specified switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       cpuPort is the logical port number to set for
 *                  the CPU port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetCpuPort(fm_int sw, fm_int cpuPort)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_int          cpuPhysPort;
    fm_bool         regLockTaken;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    cpuPort,
                    "sw=%d cpuPort=%d\n",
                    sw,
                    cpuPort);

    switchPtr    = GET_SWITCH_PTR(sw);
    regLockTaken = FALSE;

    err = fmMapLogicalPortToPhysical(switchPtr, cpuPort, &cpuPhysPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, cpuPort, err);

    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_CPU_TRAP_MASK_FH(0),
                                 (FM_LITERAL_U64(1) << cpuPhysPort) );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, cpuPort, err);

    switchPtr->cpuPort = cpuPort;
    switchPtr->defaultSourcePort = cpuPort;

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, cpuPort, err);

}   /* end fm10000SetCpuPort */




/*****************************************************************************/
/** fm10000GetNumEthLanes
 * \ingroup intPort
 *
 * \desc            Given the ethMode, return the number of lanes.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetNumEthLanes(fm_ethMode ethMode, fm_int *numLanes)
{
    switch (ethMode)
    {
        /* 1G mode */
        case FM_ETH_MODE_SGMII:
        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_1000BASE_KX:

        /* 2.5G mode */
        case FM_ETH_MODE_2500BASE_X:

        /* 10G single channel */
        case FM_ETH_MODE_6GBASE_KR:
        case FM_ETH_MODE_6GBASE_CR:
        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_25GBASE_KR:
        case FM_ETH_MODE_25GBASE_CR:
        case FM_ETH_MODE_AN_73:
            *numLanes    =  1;
            break;

        /* 10G four lanes */
        case FM_ETH_MODE_10GBASE_KX4:
        case FM_ETH_MODE_10GBASE_CX4:
        case FM_ETH_MODE_XAUI:
            *numLanes    = 4;
            break;

        /* 24G or 40G four lanes */
        case FM_ETH_MODE_XLAUI:
        case FM_ETH_MODE_24GBASE_CR4:
        case FM_ETH_MODE_24GBASE_KR4:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_40GBASE_SR4:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_KR4:
        case FM_ETH_MODE_100GBASE_SR4:
            *numLanes    = 4;
            break;

        /* disabled */
        case FM_ETH_MODE_DISABLED:
            *numLanes = 1;
            break;

        /* Unsupported mode */
        default:
            *numLanes    = 1;
            FM_LOG_ERROR(FM_LOG_CAT_PORT, "Invalid ETHMODE 0x%x\n", ethMode);
            return FM_ERR_INVALID_ETH_MODE;
    }

    return FM_OK;

}   /* end fm10000GetNumEthLanes */




/*****************************************************************************/
/** fm10000GetNumPepLanes
 * \ingroup intPort
 *
 * \desc            Given the PEP mode, return the number of lanes.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetNumPepLanes(fm_pepMode pepMode, fm_int *numLanes)
{
    switch (pepMode)
    {
        
        case FM_PORT_PEP_MODE_DISABLED:
        case FM_PORT_PEP_MODE_1X1:
        default:
            *numLanes = 1;
            break;
            
        case FM_PORT_PEP_MODE_2X4:
            *numLanes = 4;
            break;

        case FM_PORT_PEP_MODE_1X8:
            *numLanes = 8;
            break;
    }

    return FM_OK;

}   /* end fm10000GetNumPepLanes */




/*****************************************************************************/
/** fm10000GetNumLanes
 * \ingroup intPort
 *
 * \desc            Returns the number of lanes associated to a given port
 *                  (ethernet or PCIE)
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetNumLanes( fm_int sw, fm_int port, fm_int *numLanes )
{
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;

    portExt = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    if ( portExt->ring == FM10000_SERDES_RING_EPL )
    {
        fm10000GetNumEthLanes( portExt->ethMode, numLanes );
    }
    else if ( portExt->ring == FM10000_SERDES_RING_PCIE )
    {
        fm10000GetNumPepLanes( portAttrExt->pepMode, numLanes );
    }
    else
    {
        *numLanes = 1;
    }

    return FM_OK;

}   /* end fm10000GetNumLanes */



/*****************************************************************************/
/** fm10000GetPortSpeed
 * \ingroup intPort
 *
 * \desc            Given the ethMode, return the port speed.
 *
 * \return          Port speed.
 *
 *****************************************************************************/
fm_int fm10000GetPortSpeed(fm_ethMode ethMode)
{
    switch (ethMode)
    {
        case FM_ETH_MODE_SGMII:
            return 1000; /* This will be changed once speed is negotiated */

        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_1000BASE_KX:
            return 1000;

        case FM_ETH_MODE_2500BASE_X:
            return 2500;

        case FM_ETH_MODE_6GBASE_KR:
        case FM_ETH_MODE_6GBASE_CR:
            return 6000;

        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_10GBASE_KX4:
        case FM_ETH_MODE_10GBASE_CX4:
            return 10000;

        case FM_ETH_MODE_24GBASE_CR4:
        case FM_ETH_MODE_24GBASE_KR4:
            return 24000;
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_40GBASE_SR4:
        case FM_ETH_MODE_40GBASE_KR4:
            return 40000;

        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_25GBASE_KR:
        case FM_ETH_MODE_25GBASE_CR:
            return 25000;

        case FM_ETH_MODE_100GBASE_KR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_SR4:
            return 100000;

        /* Unsupported mode */
        default:
            return 0;
    }

}   /* end fm10000GetPortSpeed */




/*****************************************************************************/
/** fm10000GetEthModeStr
 * \ingroup intPort
 *
 * \desc            Given the ethMode, return the equivalent string.
 *
 * \return          Ethernet mode string.
 *
 *****************************************************************************/
fm_text fm10000GetEthModeStr(fm_ethMode ethMode)
{
    switch (ethMode)
    {
        case FM_ETH_MODE_DISABLED:
            return "DISABLED";
        case FM_ETH_MODE_AN_73:
            return "AN-73";
        case FM_ETH_MODE_SGMII:
            return "SGMII";
        case FM_ETH_MODE_1000BASE_X:
            return "1000BASE_X";
        case FM_ETH_MODE_1000BASE_KX:
            return "1000BASE_KX";
        case FM_ETH_MODE_2500BASE_X:
            return "2500BASE_X";
        case FM_ETH_MODE_6GBASE_KR:
            return "6GBASE_KR";
        case FM_ETH_MODE_6GBASE_CR:
            return "6GBASE_CR";
        case FM_ETH_MODE_10GBASE_KR:
            return "10GBASE_KR";
        case FM_ETH_MODE_10GBASE_CR:
            return "10GBASE_CR";
        case FM_ETH_MODE_10GBASE_SR:
            return "10GBASE_SR";
        case FM_ETH_MODE_10GBASE_KX4:
            return "10GBASE_KX4";
        case FM_ETH_MODE_10GBASE_CX4:
            return "10GBASE_CX4";
        case FM_ETH_MODE_24GBASE_CR4:
            return "24GBASE_CR4";
        case FM_ETH_MODE_24GBASE_KR4:
            return "24GBASE_KR4";;
        case FM_ETH_MODE_40GBASE_CR4:
            return "40GBASE_CR4";
        case FM_ETH_MODE_40GBASE_SR4:
            return "40GBASE_SR4";
        case FM_ETH_MODE_40GBASE_KR4:
            return "40GBASE_KR4";
        case FM_ETH_MODE_25GBASE_SR:
            return "25GBASE_SR";
        case FM_ETH_MODE_25GBASE_KR:
            return "25GBASE_KR";
        case FM_ETH_MODE_25GBASE_CR:
            return "25GBASE_CR";
        case FM_ETH_MODE_100GBASE_KR4:
            return "100GBASE_KR4";
        case FM_ETH_MODE_100GBASE_CR4:
            return "100GBASE_CR4";
        case FM_ETH_MODE_100GBASE_SR4:
            return "100GBASE_SR4";

        /* Unsupported mode */
        default:
            return "UNKNOWN";
    }

}   /* end fm10000GetEthModeStr */




/*****************************************************************************/
/** fm10000GetPortModeName
 * \ingroup intPort
 *
 * \desc            Get the eye height and width for a given port-lane. For
 *                  the time being only the height of the port is available.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate
 * 
 * \param[out]      name pointer to a caller-allocated area where this
 *                  function will return the associated name.
 * 
 * \param[in]       nameLen is the size of the name string.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000GetPortModeName(fm_int sw, fm_int port, fm_text name, fm_int nameLen )
{
    fm_status         status;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_int            physPort;
    fm_int            fabricPort;

    portExt = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    if ( portExt->ring == FM10000_SERDES_RING_EPL )
    {
        FM_SNPRINTF_S(name, nameLen, "%s", fm10000GetEthModeStr(portExt->ethMode));
    }
    else if ( portExt->ring == FM10000_SERDES_RING_PCIE )
    {
        switch (portAttrExt->pepMode)
        {
            
            case FM_PORT_PEP_MODE_DISABLED:
                FM_SNPRINTF_S(name, nameLen, "%s", "DISABLED");
                break;
            case FM_PORT_PEP_MODE_1X1:
                FM_SNPRINTF_S(name, nameLen, "%s", "PCIEx1");
                break;
            case FM_PORT_PEP_MODE_2X4:
                FM_SNPRINTF_S(name, nameLen, "%s", "PCIEx4");
                break;
            case FM_PORT_PEP_MODE_1X8:
                FM_SNPRINTF_S(name, nameLen, "%s", "PCIEx8");
                break;
            default:
                FM_SNPRINTF_S(name, nameLen, "PCIE_MODE(%d)", portAttrExt->pepMode);
                break;
        }
    }
    else
    {
        status = fmPlatformMapLogicalPortToPhysical(sw, port, &sw, &physPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, status);

        status = fm10000MapPhysicalPortToFabricPort(sw, physPort, &fabricPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, status);

        /* Per Table 11 of the spec, it refers to physPort, but our API refers to fabric port */
        if ((fabricPort >= 52) && (fabricPort <= 55))
        {
            FM_SNPRINTF_S(name, nameLen, "%s", "TE0");
        }
        else if ((fabricPort >= 56) && (fabricPort <= 59))
        {
            FM_SNPRINTF_S(name, nameLen, "%s", "TE1");
        }
        else if (fabricPort == 60)
        {
            FM_SNPRINTF_S(name, nameLen, "%s", "FIBM");
        }
        else if (fabricPort == 61)
        {
            FM_SNPRINTF_S(name, nameLen, "%s", "Loopback0");
        }
        else if (fabricPort == 62)
        {
            FM_SNPRINTF_S(name, nameLen, "%s", "Loopback1");
        }
        else
        {
            FM_SNPRINTF_S(name, nameLen, "PHYSPORT(%d)", fabricPort);
        }
    }

    return FM_OK;

}   /* end fm10000GetPortModeName */




/*****************************************************************************/
/** fm10000GetPortLaneEyeScore
 * \ingroup intPort
 *
 * \desc            Get the eye height and width for a given port-lane. For
 *                  the time being only the height of the port is available.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate
 * 
 * \param[in]       lane is the relative ID of the lane on which to operate.
 *                  It cannot be specified as FM_PORT_LANE_NA.
 * 
 * \param[out]      pEyeScore pointer to a caller-allocated area where this
 *                  function will return the measured eye height (bit 0 - 7)
 *                  and width (bit 8 - 15). If a value is not available,
 *                  0xff will be returned.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer arguments
 * \return          FM_ERR_INVALID_PORT_LANE invalid lane ID
 *
 *****************************************************************************/
fm_status fm10000GetPortLaneEyeScore(fm_int      sw,
                                     fm_int      port,
                                     fm_int      lane,
                                     fm_uint32 * pEyeScore )
{
    fm_status err;
    fm_int    serDes;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    port,
                    "sw=%d, port=%d lane=%d pEyeScore=%p\n",
                    sw,
                    port,
                    lane,
                    (void *)pEyeScore);

    if (pEyeScore == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (lane == FM_PORT_LANE_NA)
    {
        err = FM_ERR_INVALID_PORT_LANE;
    }
    else
    {
        *pEyeScore = 0xffff;

        err = fm10000MapPortLaneToSerdes(sw, port, lane, &serDes);

        if (err == FM_OK)
        {
            err = fm10000SerdesGetEyeScore(sw, serDes, pEyeScore);
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000GetPortLaneEyeScore */




/*****************************************************************************/
/** fm10000GetBistUserPattern
 * \ingroup intPort
 *
 * \desc            Get the the BIST user pattern for the given port-lane.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate
 * 
 * \param[in]       lane is the relative ID of the lane on which to operate.
 *                  It cannot be specified as FM_PORT_LANE_NA.
 * 
 * \param[out]      pBistUserPatternLow pointer to a caller-allocated area where
 *                  this function will return the lower part of the BIST user
 *                  pattern. It may be NULL.
 * 
 * \param[out]      pBistUserPatternHigh pointer to a caller-allocated area where
 *                  this function will return the upper part of the BIST user
 *                  pattern. It may be NULL.
 * 
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid
 * \return          FM_ERR_INVALID_PORT_LANE invalid lane ID
 *
 *****************************************************************************/
fm_status fm10000GetBistUserPattern(fm_int      sw,
                                    fm_int      port,
                                    fm_int      lane,
                                    fm_uint64 * pBistUserPatternLow,
                                    fm_uint64 * pBistUserPatternHigh )
{
    fm_status err;
    fm_int    serDes;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    port,
                    "sw=%d, port=%d lane=%d pBistUserPatternLow=%p, pBistUserPatternHigh=%p\n",
                    sw,
                    port,
                    lane,
                    (void *)pBistUserPatternLow,
                    (void *)pBistUserPatternHigh);

    if (lane == FM_PORT_LANE_NA)
    {
        err = FM_ERR_INVALID_PORT_LANE;
    }
    else
    {
        err = fm10000MapPortLaneToSerdes(sw, port, lane, &serDes);

        if (err == FM_OK)
        {
            err = fm10000SerdesGetBistUserPattern(sw, serDes, pBistUserPatternLow, pBistUserPatternHigh);
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000GetBistUserPattern */




/*****************************************************************************/
/** fm10000SetBistUserPattern
 * \ingroup intPort
 *
 * \desc            Set the the BIST user pattern for the given port-lane.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate
 * 
 * \param[in]       lane is the relative ID of the lane on which to operate.
 *                  It cannot be specified as FM_PORT_LANE_NA.
 * 
 * \param[out]      pBistUserPatternLow pointer to the lower part of the BIST
 *                  user pattern. It may be NULL, in which case the low current
 *                  value is kept.
 * 
 * \param[out]      pBistUserPatternHigh pointer to the upper part of the BIST
 *                  user pattern. It may be NULL, in which case the low current
 *                  value is kept.
 * 
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid
 * \return          FM_ERR_INVALID_PORT_LANE invalid lane ID
 *
 *****************************************************************************/
fm_status fm10000SetBistUserPattern(fm_int      sw,
                                    fm_int      port,
                                    fm_int      lane,
                                    fm_uint64 * pBistUserPatternLow,
                                    fm_uint64 * pBistUserPatternHigh )
{
    fm_status err;
    fm_int    serDes;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    port,
                    "sw=%d, port=%d lane=%d pBistUserPatternLow=%p, pBistUserPatternHigh=%p\n",
                    sw,
                    port,
                    lane,
                    (void *)pBistUserPatternLow,
                    (void *)pBistUserPatternHigh);

    if (lane == FM_PORT_LANE_NA)
    {
        err = FM_ERR_INVALID_PORT_LANE;
    }
    else
    {
        err = fm10000MapPortLaneToSerdes(sw, port, lane, &serDes);

        if (err == FM_OK)
        {
            err = fm10000SerdesSetBistUserPattern(sw, serDes, pBistUserPatternLow, pBistUserPatternHigh);
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000SetBistUserPattern */




/*****************************************************************************/
/** fm10000MapPhysicalPortToEplChannel
 * \ingroup intPort
 *
 * \desc            Maps a physical port number to an EPL/Channel tuple.
 *                                                                      \lb\lb
 *                  The EPL/Channel tuple identifies the physical EPL and
 *                  channel number of a particular physical port.
 *
 * \param[in]       sw is the switch number (not used for the moment).
 *
 * \param[in]       physPort is the physical port number.
 *
 * \param[out]      epl points to storage where the epl number of the
 *                  (epl, channel) tuple should be stored. For paired EPLs the
 *                  odd epl number is returned.
 *
 * \param[out]      channel points to storage where the channel number
 *                  of the (epl, channel) tuple should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MapPhysicalPortToEplChannel(fm_int  sw,
                                             fm_int  physPort,
                                             fm_int *epl,
                                             fm_int *channel)
{
    fm_status err = FM_OK;
    fm_int          fabricPort;
    fm_int          serDes;
    fm_serdesRing   ring;
    
    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d physPort=%d epl=%p channel=%p\n",
                 sw,
                 physPort,
                 (void *) epl,
                 (void *) channel);

    err = fm10000MapPhysicalPortToFabricPort( sw, physPort, &fabricPort );
    if ( err == FM_OK )
    {
        err = fm10000MapFabricPortToSerdes( sw, fabricPort, &serDes, &ring );
        if ( err == FM_OK )
        {
            if ( ring == FM10000_SERDES_RING_EPL )
            {
                *epl     = fabricPort / FM10000_PORTS_PER_EPL;
                *channel = fabricPort % FM10000_PORTS_PER_EPL;
            }
            else
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000MapPhysicalPortToEplChannel */



/*****************************************************************************/
/** fm10000MapEplChannelToPhysicalPort
 * \ingroup intPort
 *
 * \desc            Maps an (EPL, channel) tuple to a physical port number.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       epl is the epl number.
 *
 * \param[in]       channel is the channel number.
 *
 * \param[out]      physPort points the location where the physical port number
 *                  should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MapEplChannelToPhysicalPort(fm_int  sw,
                                             fm_int  epl,
                                             fm_int  channel,
                                             fm_int *physPort)
{
    fm_status err = FM_OK;
    fm_int    fabricPort;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, 
                 "sw=%d epl=%d channel=%d physPort=%p\n",
                 sw,
                 epl,
                 channel,
                 (void *) physPort);

    if ( epl > FM10000_MAX_EPL || channel >= FM10000_PORTS_PER_EPL ) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT,  err);
    }

    fabricPort = (epl * FM10000_PORTS_PER_EPL) + channel;

    err = fm10000MapFabricPortToPhysicalPort( sw, fabricPort, physPort );

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000MapEplChannelToPhysicalPort */





/*****************************************************************************/
/** fm10000UpdatePortMask
 * \ingroup intPort
 *
 * \desc            Updates the hardware source mask based on the port's
 *                  link state.
 *                                                                      \lb\lb
 *                  This function is called directly by fm10000SetPortAttribute
 *                  (for the FM_PORT_MASK_WIDE attribute),
 *                  and indirectly through the UpdatePortMask pointer in the
 *                  port structure. It is valid for physical ports only.
 *
 * \note            The caller is assumed to have taken the register lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdatePortMask(fm_int sw, fm_int port)
{
    fm_portAttr * portAttr;
    fm_switch *   switchPtr;
    fm_port *     portPtr;
    fm10000_port *portExt;
    fm_int        physPort;
    fm_portmask   mask;
    fm_portmask   upMask;
    fm_portmask   portmask;
    fm_uint32     maskbit;
    fm_status     err;
    fm_int        cpi;
    fm_int        mp;
    fm_uint64     regVal64;
    fm10000_port *txPortExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT, port, "sw=%d port=%d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);
    portAttr  = GET_PORT_ATTR(sw, port);
    portExt   = GET_PORT_EXT(sw, port);

    /* convert the logical port to physical port */
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
 
    /* if the port is not authorized, then the only output is the CPU */
    if (portAttr->dot1xState == FM_DOT1X_STATE_NOT_AUTH)
    {
        FM_PORTMASK_ASSIGN_BIT(&mask, 0);
    }
    else
    {
        mask  = portAttr->portMask;
    }

    /***************************************************
     * Ports that are up have their port masks filtered
     * by link state. Ports that are down are explicitly 
     * forced to have a port mask of 0.
     **************************************************/
    if (portPtr->linkUp)
    {
        /* remove the ports that are administratively down */
        for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
        {
            if ( FM_PORTMASK_IS_BIT_SET(&mask, cpi) )
            {
                mp          = GET_LOGICAL_PORT(sw, cpi);
                txPortExt   = GET_PORT_EXT(sw, mp);
                maskbit = 1;
                if (switchPtr->portTable[mp]->mode == FM_PORT_STATE_ADMIN_DOWN || 
                    txPortExt->isDraining)
                {
                    if (switchPtr->portTable[mp]->isPortForceUp || 
                        txPortExt->isDraining)
                    {
                        maskbit = 0;
                    }
                    FM_PORTMASK_SET_BIT(&mask, cpi, maskbit);
                }
            }
        }

        /* Remove LAG member ports from the portmask */
        FM_AND_PORTMASKS(&mask, &mask, &portExt->internalPortMask);

        err = fmPortMaskLogicalToLinkUpMask(switchPtr, &mask, &upMask);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        err = fmPortMaskLogicalToPhysical(switchPtr, &upMask, &portmask);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }
    else
    {
        /* a downed port cannot send to anyone */
        FM_PORTMASK_DISABLE_ALL(&portmask);
    }

    regVal64 = ( (fm_uint64)(portmask.maskWord[0]) ) | 
               ( (fm_uint64)(portmask.maskWord[1]) << 32);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_PORT_CFG_2(physPort, 0),
                                 regVal64);

ABORT:
    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000UpdatePortMask */




/*****************************************************************************/
/** fm10000UpdateLoopbackSuppress
 * \ingroup intPort
 *
 * \desc            Updates the loopback suppress registers
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateLoopbackSuppress(fm_int sw, fm_int port)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_port *    portPtr;
    fm_portAttr *portAttr;
    fm_int       physPort;
    fm_uint32    rv;
    fm_uint64    rvMod;
    fm_bool      regLockTaken;
    fm_uint32    glort;
    fm_uint32    mask;
    fm10000_lag *lagExt;
    
    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                 port,
                 "sw = %d, port = %d\n",
                 sw,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);
    portAttr  = GET_PORT_ATTR(sw, port);
    regLockTaken = FALSE;
    
    /***************************************************
     * We require loopback supression to be on when a
     * port is a member of a LAG to avoid flooding back
     * to other LAG members.  
     **************************************************/
    if (portPtr->lagIndex != -1)
    {
        lagExt = GET_LAG_EXT(sw, portPtr->lagIndex);

        FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_PORT, 
                               lagExt != NULL, 
                               err = FM_FAIL, 
                               "Unexpected lagExt==NULL\n");

        /* The Glort and Mask should be set to the canonical source
         * glort of the lag */
        glort = lagExt->lagGlort;
        mask = ~( (1 << FM10000_LAG_MASK_SIZE) - 1 );
    }
    else
    {
        /* Use the port's glort if loopback suppression is enabled */
        if (portAttr->loopbackSuppression)
        {
            glort = portPtr->glort;
            mask = 0xffff;
        }
        else
        {
            /* Will never match (disables loopback suppression) */
            glort = 0xFFFF;
            mask = 0;
        }
    }
    
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /* There are two loopback suppression mechanisms.
     * The first one is for non-L3 multicast frames and is 
     * applied by FM10000_FH_LOOPBACK_SUPPRESS. */
    rv = 0;
    FM_SET_FIELD(rv,
                 FM10000_FH_LOOPBACK_SUPPRESS,
                 glort,
                 glort);
    FM_SET_FIELD(rv,
                 FM10000_FH_LOOPBACK_SUPPRESS,
                 mask,
                 mask);

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_FH_LOOPBACK_SUPPRESS(physPort), 
                                 rv);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /* The second one is for L3 multicast frames and is 
     * applied by FM10000_MOD_PER_PORT_CFG_1 after the 
     * replication stage. */
    FM_FLAG_TAKE_REG_LOCK(sw);

    err = ReadModPerPortCfg1(sw, physPort, &rvMod);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    FM_SET_FIELD64(rvMod,
                   FM10000_MOD_PER_PORT_CFG_1,
                   LoopbackSuppressGlort,
                   glort);
    FM_SET_FIELD64(rvMod,
                   FM10000_MOD_PER_PORT_CFG_1,
                   LoopbackSuppressMask,
                   mask);

    err = WriteModPerPortCfg1(sw, physPort, rvMod);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000UpdateLoopbackSuppress */




/*****************************************************************************/
/** fm10000StartPortLaneBistCounter
 * \ingroup intPort
 *
 * \desc            Restart the BIST rx comparator counter
 *
 * \note            This is an internal function.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[in]       lane is the relative ID of the lane on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000StartPortLaneBistCounter(fm_int sw, 
                                          fm_int port,
                                          fm_int lane)
{
    fm10000_portAttr *portAttrExt;
    fm_status         err;
    fm_int            serdes;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                 port,
                 "sw=%d, port=%d laneNum=%d\n",
                 sw,
                 port,
                 lane);

    /* Validate logical port ID */
    if ( !fmIsValidPort(sw, port, DISALLOW_CPU) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    /* Get all port-related structure pointers we need */
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /* Retrieve the Serdes number */
    err = fm10000MapPortLaneToSerdes(sw, port, lane, &serdes);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    
    /* Reset the counter */
    err = fm10000ResetSerdesErrorCounter(sw, serdes);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

ABORT:
    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}  /* end fm10000StartPortLaneBistCounter */




/*****************************************************************************/
/** fm10000DisablePortLaneBistMode
 * \ingroup intPort
  *
 * \desc            Disable the BIST mode
 *
 * \note            This is an internal function.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[in]       lane is the relative ID of the lane on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DisablePortLaneBistMode( fm_int sw, fm_int port, fm_int lane )
{
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_status        err;
    fm_int           serdes = 0;
    fm10000_lane     *laneExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    port,
                    "sw=%d, port=%d, lane=%d",
                    sw,
                    port,
                    lane);

    /* Validate logical port ID */
    if ( !fmIsValidPort(sw, port, DISALLOW_CPU) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    /* Get all port-related structure pointers we need */
    portExt     = GET_PORT_EXT(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /* retrieve the Serdes ID */
    err = fm10000MapPortLaneToSerdes(sw, port, lane, &serdes);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err );

    laneExt = GET_LANE_EXT( sw, serdes );

    /* Unconditionally disable both the comparator and the generator */
    err = fm10000ClearSerdesRxPattern(sw, serdes);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    err = fm10000ClearSerdesTxPattern(sw, serdes);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    laneExt->bistActive = FALSE;
ABORT:
    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err );

}  /* end fm10000DisablePortLaneBistMode */




/*****************************************************************************/
/** fm10000IsPortBistActive
 * \ingroup intPort
 *
 * \desc            This function checks, if the BIST is active on all of the 
 *                  port's lanes
 *
 * \note            This is an internal function.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[out]      isBistActive points to caller-supplied storage where this
 *                  function will return a flag indicating whether BIST is
 *                  active (TRUE) or not (FALSE)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000IsPortBistActive(fm_int   sw,
                                  fm_int   port,
                                  fm_bool *isBistActive)
{
    fm10000_port *portExt;
    fm_status     err;
    fm10000_lane *laneExt;

    /* Validate logical port ID */
    if ( !fmIsValidPort(sw, port, DISALLOW_CPU) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    if (isBistActive == NULL)
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, port, "Invalid data pointer\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    *isBistActive = TRUE;
    err = FM_OK;
    portExt = GET_PORT_EXT(sw, port);
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        if ( laneExt->bistActive == FALSE )
        {
            *isBistActive = FALSE;
            break;
        }

        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

ABORT:
    return err;

}   /* end fm10000IsPortBistActive */




/*****************************************************************************/
/** fm10000DrainPhysPort
 * \ingroup intSwitch
 *
 * \desc            Drain the specified physical ports.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       physPort is the physical port to operate on.
 *
 * \param[in]       numPorts is the number of physical ports to drain.
 * 
 * \param[in]       drain indicates whether to drain or un-drain.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DrainPhysPort( fm_int  sw,
                                fm_int  physPort,
                                fm_int  numPorts,
                                fm_bool drain )
{
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_status         err;
    fm_timestamp      end;
    fm_timestamp      delta;
    fm_timestamp      start;
    fm_uint32         usage1;
    fm_uint32         value[FM10000_MAC_CFG_WIDTH];
    fm_uint32         totalUsage;
    fm_uint32         usage2;
    fm_uint           addr;
    fm_int            epl;
    fm_int            channel;
    fm_int            i;
    fm_int            mp;
    fm_int            logPort;

   /******************************************************
    * This function can be made state machine action 
    * compliant if executed only from a state machine 
    * context. In that case it'd have to be moved to 
    * fm10000_api_port_actions.c 
    ******************************************************/
   
    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw=%d physPort=%d numPorts=%d drain=%d\n",
                 sw,
                 physPort,
                 numPorts,
                 drain);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmMapPhysicalPortToLogical(switchPtr, physPort, &logPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    portExt     = GET_PORT_EXT(sw, logPort);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, logPort);

    if (portExt->ring == FM10000_SERDES_RING_EPL) 
    {
        err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

        /* Is this check still valid on FM10000? */
        if ( (channel != 3) && ( channel != 0 ) && ( numPorts > 1 ) )
        {
            FM_LOG_FATAL( FM_LOG_CAT_PORT,
                          "Draining multi-lane port, but physPort "
                          "%d (np=%d) maps to epl=%d channel=%d\n",
                          physPort,
                          numPorts,
                          epl,
                          channel );
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
        }
    }

    portExt->isDraining = drain;

    err = fmUpdateSwitchPortMasks(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* For Ethernet port Drain mode is configured in MAC. */
    if (portExt->ring == FM10000_SERDES_RING_EPL) 
    {
        for (i = 0 ; i < numPorts ; i++)
        {
            if (channel + i > 3)
            {
                /* A sanity check */
                FM_LOG_FATAL( FM_LOG_CAT_PORT,
                              "Overflow: DrainPort physPort %d "
                              "(np=%d) epl=%d channel=%d i=%d\n",
                              physPort,
                              numPorts,
                              epl,
                              channel,
                              i );
                continue;
            }

            /* read modify write the MAC_CFG register */
            addr = FM10000_MAC_CFG( epl, channel + i, 0 );

            err = switchPtr->ReadUINT32Mult(sw, addr, FM10000_MAC_CFG_WIDTH, value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            FM_ARRAY_SET_FIELD( value,
                                FM10000_MAC_CFG,
                                TxDrainMode,
                                (drain ?
                                 FM10000_PORT_TX_DRAIN_ALWAYS :
                                 FM10000_PORT_TX_DRAIN_ON_LINK_DOWN) );
            FM_ARRAY_SET_BIT(value, FM10000_MAC_CFG, RxDrain, (drain ? 1 : 0));

            err = switchPtr->WriteUINT32Mult(sw,
                                             addr,
                                             FM10000_MAC_CFG_WIDTH,
                                             value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

        }   /* end for ( i = 0 ; i < numPorts ; i++) */
    }

    /* If draining make sure there are no more in-flight frames.*/ 
    if (drain)
     {
        fmGetTime(&start);

        do
        {
            totalUsage = 0;

            for ( i = physPort ; i < (numPorts + physPort) ; i++ )
            {
                /* Do we really need to check for in-flight frames in the
                   Rx Direction? */
                for ( mp = 0 ; mp < FM10000_MAX_SHMEM_PARTITION ; mp++ )
                {
                    err = switchPtr->ReadUINT32( sw,
                                                 FM10000_CM_RX_SMP_USAGE(i, mp),
                                                 &usage1 );
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

                    totalUsage += FM_GET_FIELD( usage1,
                                                FM10000_CM_RX_SMP_USAGE,
                                                count );
                }

                for ( mp = 0 ; mp < FM10000_MAX_TRAFFIC_CLASS ; mp++ )
                {
                    err = switchPtr->ReadUINT32( sw,
                                                 FM10000_CM_TX_TC_USAGE(i,mp),
                                                 &usage2 );
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

                    totalUsage += FM_GET_FIELD( usage2,
                                                FM10000_CM_TX_TC_USAGE,
                                                count );
                }
            }

            /* This should be based on ticks */
            fmDelay(0, 68);

            fmGetTime(&end);
            fmSubTimestamps(&end, &start, &delta);

        }
        while ( ( totalUsage > 0 ) && ( delta.sec < FM10000_DRAIN_DELAY ) );

        if ( delta.sec >= FM10000_DRAIN_DELAY )
        {
            FM_LOG_FATAL(FM_LOG_CAT_PORT,
                         "Drain of ports %d..%d did not finish in %dsec. totalUsage=%d\n",
                         physPort,
                         physPort + numPorts - 1,
                         FM10000_DRAIN_DELAY,
                         totalUsage);
        }
    }


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000DrainPhysPort */


/*****************************************************************************/
/** fm10000SetPortModuleState
 * \ingroup intPort
 *
 * \desc            Internal function to set the state of specified port
 *                  module. The port module state consists of an array of
 *                  status flags that must be managed in a monolithic way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number on which to operate. May 
 *                  not  be a LAG logical port number nor a CPU interface 
 *
 * \param[in]       lane is the port's zero-based lane number on which to 
 *                  operate. May not be specified as FM_PORT_LANE_NA nor
 *                  FM_PORT_LANE_ALL
 * 
 * \param[in]       xcvrSignals a bitmask where each bit represents the status
 *                  of a given signal (see the ''Transceiver Signals'' 
 *                  definitions)
 * 
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT_LANE if lane is not valid.
 *****************************************************************************/
fm_status fm10000SetPortModuleState( fm_int     sw, 
                                     fm_int     port, 
                                     fm_int     lane, 
                                     fm_uint32  xcvrSignals )
{
    fm_status         status;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;


    FM_LOG_ENTRY_API_V2( FM_LOG_CAT_PORT,
                         port,
                         "sw=%d port=%d lane=%d signals=0x%x\n",
                         sw,
                         port,
                         lane,
                         xcvrSignals );

    /* perform a minimal argument validation */
    status = FM_OK;
    if ( fmIsCardinalPort(sw, port) )
    {
        if ( lane < 0 )
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_PORT, port, "Invalid lane ID\n");
            status = FM_ERR_INVALID_PORT_LANE;
        }

        portExt     = GET_PORT_EXT(sw, port);
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);


        if (status == FM_OK)
        {
            /* validate only the port number if the port
               is being disabled */
            if ( portExt->ethMode == FM_ETH_MODE_DISABLED )
            {
                status = ValidatePort( sw, port );
            }
            else
            {
                status = ValidatePortLane(sw, port, lane);
            }
        }
        
        if (status == FM_OK)
        {
            /* set port module status */
            portExt->portModStatus[lane] = xcvrSignals;
        }
    }
    else
    {
        /* Invalid port for this command */
        status = FM_ERR_INVALID_PORT;
    }

    FM_LOG_EXIT_API_V2(FM_LOG_CAT_PORT, port, status);

}   /* end fm10000SetPortModuleState */



/*****************************************************************************/
/** fm10000DbgDumpPortStateTransitionsV2
 * \ingroup intPort
 *
 * \desc            Function to dump a Port's State Transition history.
 *
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       portList is an array of ports for which state transitions
 *                  are displayed..
 *
 * \param[in]       portCnt is the size of portList array.
 *
 * \param[in]       maxEntries is maximun number of entries to display.
 *                  Set to 0 to display all.
 *
 * \param[in]       optionStr is comma-delimited options string.
 *                  Supported options:
 *                      help        - List available options
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM in case of memory allocation failures.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpPortStateTransitionsV2( fm_int  sw,
                                                fm_int  *portList,
                                                fm_int  portCnt,
                                                fm_int  maxEntries,
                                                fm_text optionStr)
{
    fm_status               status;
    fm10000_port           *portExt;
    fm10000_lane           *laneExt;
    fm10000_laneDfe        *pLaneDfe;
    fm_smHandle             smHandle;
    fm_int                  numTransitions;
    fm_int                  numEntry;
    fm_int                  maxEntry;
    fm10000_portStateTDump *pstd;
    fm_text                 naStr = "N/A";
    fm_char                 tempStr[32];
    fm_char                 tempStr2[32];
    fm_text                 pStr;
    fm_bool                 showPortST;
    fm_bool                 showAnST;
    fm_bool                 showSerdesST;
    fm_bool                 showSerdesDfeST;
    fm_bool                 showAbsTime;
    fm_bool                 showUsecTime;
    fm_int                  cnt;
    fm_int                  i;
    fm_int                  port;
    fm_int                  historySize;
    fm_smTransitionRecord  *history;
    fm10000_portStateTDump **sortPstd;

    status          = FM_OK;
    history         = NULL;
    portExt         = NULL;
    pstd            = NULL;
    showPortST      = FALSE;
    showAnST        = FALSE;
    showSerdesST    = FALSE;
    showSerdesDfeST = FALSE;
    showAbsTime     = FALSE;
    showUsecTime    = FALSE;

    if (optionStr)
    {
        if (strstr(optionStr, "help") != NULL)
        {
            FM_LOG_PRINT("Available options:\n");
            FM_LOG_PRINT("    help        - List available comma-delimited options\n"
                         "    all         - All transition types\n"
                         "    port        - Include port state transitions\n"
                         "    autoneg     - Include autoneg state transitions\n"
                         "    serdes      - Include SERDES state transitions\n"
                         "    serdesDfe   - Include SERDES DFE state transitions\n"
                         "    absTime     - Absolute instead of relative timestamp from first entry\n"
                         "    usecTime    - Show timestamp in usec instead of msec resolution\n"
                         );
            FM_LOG_EXIT( FM_LOG_CAT_PORT, FM_OK );
        }
        if (strcasestr(optionStr, "absTime") != NULL)
        {
            showAbsTime = TRUE;
        }
        if (strcasestr(optionStr, "usecTime") != NULL)
        {
            showUsecTime = TRUE;
        }
        if (strcasestr(optionStr, "port") != NULL)
        {
            showPortST = TRUE;
        }
        if (strcasestr(optionStr, "autoneg") != NULL)
        {
            showAnST = TRUE;
        }
        if ((pStr = strcasestr(optionStr, "serdes")) != NULL)
        {
            /* Don't match serdesDfe */
            if (!(pStr[6] == 'D' || pStr[6] == 'd'))
            {
                showSerdesST = TRUE;
            }
        }
        if (strcasestr(optionStr, "serdesDfe") != NULL)
        {
            showSerdesDfeST = TRUE;
        }
    }

    if (!showPortST && !showAnST && !showSerdesST && !showSerdesDfeST)
    {
        showPortST      = TRUE;
        showAnST        = TRUE;
        showSerdesST    = TRUE;
        showSerdesDfeST = TRUE;
    }

    numEntry = 0;
    for (cnt = 0 ; cnt < portCnt ; cnt++)
    {
        port = portList[cnt];
        portExt = GET_PORT_EXT( sw, port );

        numEntry += portExt->transitionHistorySize;
        numEntry += portExt->anTransitionHistorySize;

        laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
        while ( laneExt != NULL )
        {
            numEntry += laneExt->transitionHistorySize;

            if (laneExt->dfeExt.smType != FM_SMTYPE_UNSPECIFIED)
            {
                pLaneDfe = &laneExt->dfeExt;

                numEntry += pLaneDfe->transitionHistorySize;
            }

            /* next lane, if any */
            laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
        }
    }
    maxEntry = numEntry;
    pstd = fmAlloc( maxEntry * sizeof(fm10000_portStateTDump) );
    if ( pstd == NULL )
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, status );
    }


    numEntry = 0;
    for (cnt = 0 ; cnt < portCnt ; cnt++)
    {
        port = portList[cnt];
        if (showPortST)
        {
            portExt = GET_PORT_EXT( sw, port );

            /**************************************************
             * Port-level state machine
             **************************************************/
            
            numTransitions = portExt->transitionHistorySize;
            smHandle       = portExt->smHandle;

            historySize = numTransitions * sizeof(fm_smTransitionRecord );
            history = fmAlloc( historySize );
            if ( history == NULL )
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
            }

            FM_MEMSET_S( history, historySize, 0, historySize );

            status = fmGetStateTransitionHistory( smHandle,
                                                  &numTransitions,
                                                  history );
            if ( status == FM_OK )
            {
                for ( i = 0 ; i < numTransitions ; i++ )
                {
                    if (numEntry >= maxEntry)
                    {
                        break;
                    }

                    pstd[numEntry].timestamp = history[i].eventTime.sec*1000000 + 
                                               history[i].eventTime.usec;
                    if ( history[i].currentState != FM_STATE_UNSPECIFIED )
                    {
                        pstd[numEntry].currentState= fm10000PortStatesMap[history[i].currentState];
                    }
                    else
                    {
                        pstd[numEntry].currentState = naStr;
                    }
                    if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
                    {
                        pstd[numEntry].event = fm10000PortEventsMap[history[i].eventInfo.eventId];
                    }
                    else
                    {
                        pstd[numEntry].event = naStr;
                    }
                    pstd[numEntry].type = fm10000SmTypeStr(history[i].eventInfo.smType);
                    pstd[numEntry].nextState = fm10000PortStatesMap[history[i].nextState];
                    pstd[numEntry].status = history[i].status;
                    pstd[numEntry].port = port;
                    pstd[numEntry].serdes = 0xFF;
                    numEntry++;
                }
            }

            fmFree( history );
        }

        if (showAnST)
        {
            portExt = GET_PORT_EXT( sw, port );

            /**************************************************
             * Auto-negotiation state machine if AN is enabled
             **************************************************/
            if ( portExt->smType   == FM10000_AN_PORT_STATE_MACHINE &&
                 portExt->anSmType != FM_SMTYPE_UNSPECIFIED )
            {

                numTransitions = portExt->anTransitionHistorySize;
                smHandle       = portExt->anSmHandle;

                historySize = numTransitions * sizeof(fm_smTransitionRecord );
                history = fmAlloc( historySize );
                if ( history == NULL )
                {
                    status = FM_ERR_NO_MEM;
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                }

                FM_MEMSET_S( history, historySize, 0, historySize );

                status = fmGetStateTransitionHistory( smHandle,
                                                      &numTransitions,
                                                      history );

                status = fmGetStateTransitionHistory( smHandle,
                                                      &numTransitions,
                                                      history );
                if ( status == FM_OK )
                {
                    for ( i = 0 ; i < numTransitions ; i++ )
                    {
                        if (numEntry >= maxEntry)
                        {
                            break;
                        }

                        pstd[numEntry].timestamp = history[i].eventTime.sec*1000000 + 
                                                   history[i].eventTime.usec;

                        if ( history[i].currentState != FM_STATE_UNSPECIFIED )
                        {
                            pstd[numEntry].currentState = fm10000AnStatesMap[history[i].currentState];
                        }
                        else
                        {
                            pstd[numEntry].currentState = naStr;
                        }
                        if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
                        {
                            pstd[numEntry].event = fm10000AnEventsMap[history[i].eventInfo.eventId];
                        }
                        else
                        {
                            pstd[numEntry].event = naStr;
                        }
                        pstd[numEntry].type = fm10000SmTypeStr(history[i].eventInfo.smType);
                        pstd[numEntry].nextState = fm10000AnStatesMap[history[i].nextState];
                        pstd[numEntry].status = history[i].status;
                        pstd[numEntry].port = port;
                        pstd[numEntry].serdes = 0xFF;
                        numEntry++;
                    }
                }
                fmFree( history );
            }
        }

        /**************************************************
         * SerDes-level state machine(s)
         **************************************************/

        laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
        while ( laneExt != NULL )
        {
            if (showSerdesST)
            {
                numTransitions = laneExt->transitionHistorySize;
                smHandle       = laneExt->smHandle;

                historySize = numTransitions * sizeof(fm_smTransitionRecord );
                history = fmAlloc( historySize );
                if ( history == NULL )
                {
                    status = FM_ERR_NO_MEM;
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                }

                FM_MEMSET_S( history, historySize, 0, historySize );

                status = fmGetStateTransitionHistory( smHandle,
                                                      &numTransitions,
                                                      history );

                if ( status == FM_OK )
                {
                    for ( i = 0 ; i < numTransitions ; i++ )
                    {
                        if (numEntry >= maxEntry)
                        {
                            break;
                        }

                        pstd[numEntry].timestamp = history[i].eventTime.sec*1000000 + 
                                                   history[i].eventTime.usec;

                        if ( history[i].currentState != FM_STATE_UNSPECIFIED )
                        {
                            pstd[numEntry].currentState = fm10000SerDesStatesMap[history[i].currentState];
                        }
                        else
                        {
                            pstd[numEntry].currentState = naStr;
                        }
                        if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
                        {
                            pstd[numEntry].event = fm10000SerDesEventsMap[history[i].eventInfo.eventId];
                        }
                        else
                        {
                            pstd[numEntry].event = naStr;
                        }
                        pstd[numEntry].type = fm10000SmTypeStr(history[i].eventInfo.smType);
                        pstd[numEntry].nextState = fm10000SerDesStatesMap[history[i].nextState];
                        pstd[numEntry].status = history[i].status;
                        pstd[numEntry].port = port;
                        pstd[numEntry].lane = laneExt->lane;
                        pstd[numEntry].serdes = laneExt->serDes;
                        numEntry++;

                    }
                }

                fmFree( history );
            }

            if (showSerdesDfeST && laneExt->dfeExt.smType != FM_SMTYPE_UNSPECIFIED)
            {
                pLaneDfe = &laneExt->dfeExt;

                numTransitions = pLaneDfe->transitionHistorySize;
                smHandle       = pLaneDfe->smHandle;

                historySize = numTransitions * sizeof(fm_smTransitionRecord );
                history = fmAlloc( historySize );
                if ( history == NULL )
                {
                    status = FM_ERR_NO_MEM;
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                }

                FM_MEMSET_S( history, historySize, 0, historySize );

                status = fmGetStateTransitionHistory( smHandle,
                                                      &numTransitions,
                                                      history );

                if ( status == FM_OK )
                {
                    for ( i = 0 ; i < numTransitions ; i++ )
                    {
                        if (numEntry >= maxEntry)
                        {
                            break;
                        }

                        pstd[numEntry].timestamp = history[i].eventTime.sec*1000000 + 
                                                   history[i].eventTime.usec;

                        if ( history[i].currentState != FM_STATE_UNSPECIFIED )
                        {
                            pstd[numEntry].currentState = fm10000SerDesDfeStatesMap[history[i].currentState];
                        }
                        else
                        {
                            pstd[numEntry].currentState = naStr;
                        }
                        if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
                        {
                            pstd[numEntry].event = fm10000SerDesDfeEventsMap[history[i].eventInfo.eventId];
                        }
                        else
                        {
                            pstd[numEntry].event = naStr;
                        }
                        pstd[numEntry].type = fm10000SmTypeStr(history[i].eventInfo.smType);
                        pstd[numEntry].nextState = fm10000SerDesDfeStatesMap[history[i].nextState];
                        pstd[numEntry].status = history[i].status;
                        pstd[numEntry].port = port;
                        pstd[numEntry].lane = laneExt->lane;
                        pstd[numEntry].serdes = laneExt->serDes;
                        numEntry++;
                    }
                }

                fmFree( history );
            }

            /* next lane, if any */
            laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );

        }   /* end while (laneExt != NULL ) */
    } /* end for (cnt = 0; cnt < portCnt; cnt++) */

    if (numEntry)
    {

        sortPstd = fmAlloc( numEntry*sizeof(fm10000_portStateTDump *) );
        if (sortPstd  == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        for (cnt = 0 ; cnt < numEntry ; cnt++)
        {
            sortPstd[cnt] = (fm10000_portStateTDump *)&pstd[cnt];
        }

        qsort(sortPstd, numEntry, sizeof(fm10000_portStateTDump *), CmpTimestamp);


        FM_LOG_PRINT(DUMP_FORMAT,
                                  "---------------",
                                  "-----",
                                  "-------",
                                  "-----------------------------",
                                  "------------------------------------",
                                  "----------------------------",
                                  "------");
        FM_LOG_PRINT(DUMP_FORMAT, "TIME(sec)", "PORT", "TYPE", "INITIAL STATE", "EVENT", "FINAL STATE", "STATUS");
        FM_LOG_PRINT(DUMP_FORMAT,
                                  "---------------",
                                  "-----",
                                  "-------",
                                  "-----------------------------",
                                  "------------------------------------",
                                  "----------------------------",
                                  "------");

        if (maxEntries > 0 && numEntry > maxEntries)
        {
            numEntry = maxEntries;
        }

        for (cnt = 0 ; cnt < numEntry ; cnt++)
        {
            if (showAbsTime)
            {
                FM_SNPRINTF_S(tempStr, 32, "%15.3f", (sortPstd[cnt]->timestamp/1000000.0));
            }
            else
            {
                if (showUsecTime)
                {
                    FM_SNPRINTF_S(tempStr,
                                  32,
                                  "%15.6f",
                                  ((sortPstd[cnt]->timestamp - sortPstd[0]->timestamp)/1000000.0));
                }
                else
                {
                    FM_SNPRINTF_S(tempStr,
                                  32,
                                  "%15.3f",
                                  ((sortPstd[cnt]->timestamp - sortPstd[0]->timestamp)/1000000.0));
                }
            }
            if (sortPstd[cnt]->serdes == 0xFF)
            {
                FM_SNPRINTF_S(tempStr2, 32, "%d", sortPstd[cnt]->port);
            }
            else
            {
                FM_SNPRINTF_S(tempStr2, 32, "%d.%d", sortPstd[cnt]->port, sortPstd[cnt]->lane);
            }
            FM_LOG_PRINT(DUMP_FORMAT,
                         tempStr,
                         tempStr2, 
                         sortPstd[cnt]->type, 
                         sortPstd[cnt]->currentState, 
                         sortPstd[cnt]->event, 
                         sortPstd[cnt]->nextState,
                         sortPstd[cnt]->status ? fmErrorMsg(sortPstd[cnt]->status):"OK");
        }

        fmFree(sortPstd);
    }
    else
    {
        FM_LOG_PRINT("No matching entry found.\n");
    }

ABORT:
    if (pstd)
    {
        fmFree(pstd);
    }

    FM_LOG_EXIT( FM_LOG_CAT_PORT, status );

} /* end fm10000DbgDumpPortStateTransitionsV2  */


/*****************************************************************************/
/** fm10000DbgDumpPortStateTransitions
 * \ingroup intPort
 *
 * \desc            Function to dump a Port's State Transition history.
 *
 * \param[in]       sw the ID of the switch the port belongs to.
 * 
 * \param[in]       port is the logical port ID.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM in case of memory allocation failures.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpPortStateTransitions( fm_int sw, fm_int port )
{
    fm_status              status;
    fm_int                 numTransitions;
    fm_int                 historySize;
    fm_int                 i;
    fm_smTransitionRecord *history;
    fm_text                currentState;
    fm_text                event;
    fm10000_port          *portExt;
    fm10000_lane          *laneExt;
    fm10000_laneDfe       *pLaneDfe;
    fm_smHandle            smHandle;

    portExt = GET_PORT_EXT( sw, port );


    /**************************************************
     * Port-level state machine
     **************************************************/
    
    numTransitions = portExt->transitionHistorySize;
    smHandle       = portExt->smHandle;

    historySize = numTransitions * sizeof(fm_smTransitionRecord );
    history = fmAlloc( historySize );
    if ( history == NULL )
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }

    FM_MEMSET_S( history, historySize, 0, historySize );

    status = fmGetStateTransitionHistory( smHandle,
                                          &numTransitions,
                                          history );
    if ( status == FM_OK )
    {
        FM_LOG_PRINT("\n");

       /* print the header's divider */
        FM_LOG_PRINT("Port-level state machine\n");
        FM_LOG_PRINT( "-------------------"
                      "---------"
                      "-------------------------------"
                      "---------------------------------------"
                      "-------------------------------"
                      "----------");
        FM_LOG_PRINT("\n");

        /* print the formatted header */
        FM_LOG_PRINT("%17s   ", "ELAPSED TIME");
        FM_LOG_PRINT("%-6s   ", "TYPE");
        FM_LOG_PRINT("%-28s   ", "INITIAL STATE");
        FM_LOG_PRINT("%-36s   ", "EVENT");
        FM_LOG_PRINT("%-28s   ", "FINAL STATE");
        FM_LOG_PRINT("%-7s   ", "STATUS");
        FM_LOG_PRINT("\n");

        /* print the header's divider */
        FM_LOG_PRINT( "-------------------"
                      "---------"
                      "-------------------------------"
                      "---------------------------------------"
                      "-------------------------------"
                      "----------");
        FM_LOG_PRINT("\n");

        for ( i = 0 ; i < numTransitions ; i++ )
        {
           if ( history[i].currentState != FM_STATE_UNSPECIFIED )
           {
               currentState = fm10000PortStatesMap[history[i].currentState];
           }
           else
           {
               currentState = "N/A";
           }
           if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
           {
               event = fm10000PortEventsMap[history[i].eventInfo.eventId];
           }
           else
           {
               event = "N/A";
           }
           FM_LOG_PRINT( "%8lld.%03lld secs   ", 
                   history[i].eventTime.sec, 
                   history[i].eventTime.usec/1000 );

           FM_LOG_PRINT("%-6d   ",  history[i].eventInfo.smType );
           FM_LOG_PRINT("%-28s   ", currentState );
           FM_LOG_PRINT("%-36s   ", event );
           FM_LOG_PRINT("%-28s   ", fm10000PortStatesMap[history[i].nextState]);
           FM_LOG_PRINT("%-7d   ",  history[i].status);
           FM_LOG_PRINT("\n");
        }
    }

    fmFree( history );


    /**************************************************
     * Auto-negotiation state machine if AN is enabled
     **************************************************/

    if ( portExt->smType   == FM10000_AN_PORT_STATE_MACHINE &&
         portExt->anSmType != FM_SMTYPE_UNSPECIFIED )
    {
        numTransitions = portExt->anTransitionHistorySize;
        smHandle       = portExt->anSmHandle;

        historySize = numTransitions * sizeof(fm_smTransitionRecord );
        history = fmAlloc( historySize );
        if ( history == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        FM_MEMSET_S( history, historySize, 0, historySize );

        status = fmGetStateTransitionHistory( smHandle,
                                              &numTransitions,
                                              history );
        if ( status == FM_OK )
        {
            FM_LOG_PRINT("\n");

           /* print the header's divider */
            FM_LOG_PRINT("Auto-negotiation state machine\n");
            FM_LOG_PRINT( "-------------------"
                          "---------"
                          "-------------------------------"
                          "---------------------------------------"
                          "-------------------------------"
                          "----------");
            FM_LOG_PRINT("\n");

            /* print the formatted header */
            FM_LOG_PRINT("%17s   ", "ELAPSED TIME");
            FM_LOG_PRINT("%-6s   ", "TYPE");
            FM_LOG_PRINT("%-28s   ", "INITIAL STATE");
            FM_LOG_PRINT("%-36s   ", "EVENT");
            FM_LOG_PRINT("%-28s   ", "FINAL STATE");
            FM_LOG_PRINT("%-7s   ", "STATUS");
            FM_LOG_PRINT("\n");

            /* print the header's divider */
            FM_LOG_PRINT( "-------------------"
                          "---------"
                          "-------------------------------"
                          "---------------------------------------"
                          "-------------------------------"
                          "----------");
            FM_LOG_PRINT("\n");

            for ( i = 0 ; i < numTransitions ; i++ )
            {
               if ( history[i].currentState != FM_STATE_UNSPECIFIED )
               {
                   currentState = fm10000AnStatesMap[history[i].currentState];
               }
               else
               {
                   currentState = "N/A";
               }
               if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
               {
                   event = fm10000AnEventsMap[history[i].eventInfo.eventId];
               }
               else
               {
                   event = "N/A";
               }
               FM_LOG_PRINT( "%8lld.%03lld secs   ", 
                       history[i].eventTime.sec, 
                       history[i].eventTime.usec/1000 );

               FM_LOG_PRINT("%-6d   ",  history[i].eventInfo.smType );
               FM_LOG_PRINT("%-28s   ", currentState );
               FM_LOG_PRINT("%-36s   ", event );
               FM_LOG_PRINT("%-28s   ", fm10000AnStatesMap[history[i].nextState]);
               FM_LOG_PRINT("%-7d   ",  history[i].status);
               FM_LOG_PRINT("\n");

            }

        }

        fmFree( history );

    }   /* end if ( portExt->smType == FM10000_AN_PORT_STATE_MACHINE && ... ) */


    /**************************************************
     * SerDes-level state machine(s)
     **************************************************/

    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        numTransitions = laneExt->transitionHistorySize;
        smHandle       = laneExt->smHandle;

        historySize = numTransitions * sizeof(fm_smTransitionRecord );
        history = fmAlloc( historySize );
        if ( history == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        FM_MEMSET_S( history, historySize, 0, historySize );

        status = fmGetStateTransitionHistory( smHandle,
                                              &numTransitions,
                                              history );

        if ( status == FM_OK )
        {
            FM_LOG_PRINT("\n");

           /* print the header's divider */
            FM_LOG_PRINT( "Serdes-level state machine for serdes %2d\n", 
                          laneExt->serDes );
            FM_LOG_PRINT( "-------------------"
                          "---------"
                          "-------------------------------"
                          "---------------------------------------"
                          "-------------------------------"
                          "----------");
            FM_LOG_PRINT("\n");

            /* print the formatted header */
            FM_LOG_PRINT("%17s   ",  "ELAPSED TIME");
            FM_LOG_PRINT("%-6s   ",  "TYPE");
            FM_LOG_PRINT("%-28s   ", "INITIAL STATE");
            FM_LOG_PRINT("%-36s   ", "EVENT");
            FM_LOG_PRINT("%-28s   ", "FINAL STATE");
            FM_LOG_PRINT("%-7s   ",  "STATUS");
            FM_LOG_PRINT("\n");

            /* print the header's divider */
            FM_LOG_PRINT( "-------------------"
                          "---------"
                          "-------------------------------"
                          "---------------------------------------"
                          "-------------------------------"
                          "----------");
            FM_LOG_PRINT("\n");

            for ( i = 0 ; i < numTransitions ; i++ )
            {
               if ( history[i].currentState != FM_STATE_UNSPECIFIED )
               {
                   currentState = fm10000SerDesStatesMap[history[i].currentState];
               }
               else
               {
                   currentState = "N/A";
               }
               if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
               {
                   event = fm10000SerDesEventsMap[history[i].eventInfo.eventId];
               }
               else
               {
                   event = "N/A";
               }
               FM_LOG_PRINT( "%8lld.%03lld secs   ", 
                       history[i].eventTime.sec, 
                       history[i].eventTime.usec/1000 );

               FM_LOG_PRINT("%-6d   ",  history[i].eventInfo.smType );
               FM_LOG_PRINT("%-28s   ", currentState );
               FM_LOG_PRINT("%-36s   ", event );
               FM_LOG_PRINT("%-28s   ", fm10000SerDesStatesMap[history[i].nextState]);
               FM_LOG_PRINT("%-7d   ",  history[i].status);
               FM_LOG_PRINT("\n");
            }
        }


        fmFree( history );

        if (laneExt->dfeExt.smType != FM_SMTYPE_UNSPECIFIED)
        {
            pLaneDfe = &laneExt->dfeExt;

            numTransitions = pLaneDfe->transitionHistorySize;
            smHandle       = pLaneDfe->smHandle;
    
            historySize = numTransitions * sizeof(fm_smTransitionRecord );
            history = fmAlloc( historySize );
            if ( history == NULL )
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
            }
    
            FM_MEMSET_S( history, historySize, 0, historySize );
    
            status = fmGetStateTransitionHistory( smHandle,
                                                  &numTransitions,
                                                  history );
    
            if ( status == FM_OK )
            {
                FM_LOG_PRINT("\n");
    
               /* print the header's divider */
                FM_LOG_PRINT( "Serdes-DFE level state machine for serdes %2d \n", 
                              laneExt->serDes );
                FM_LOG_PRINT( "-------------------"
                              "---------"
                              "-------------------------------"
                              "---------------------------------------"
                              "-------------------------------"
                              "----------");
                FM_LOG_PRINT("\n");
    
                /* print the formatted header */
                FM_LOG_PRINT("%17s   ", "ELAPSED TIME");
                FM_LOG_PRINT("%-6s   ", "TYPE");
                FM_LOG_PRINT("%-28s   ", "INITIAL STATE");
                FM_LOG_PRINT("%-36s   ", "EVENT");
                FM_LOG_PRINT("%-28s   ", "FINAL STATE");
                FM_LOG_PRINT("%-7s   ", "STATUS");
                FM_LOG_PRINT("\n");
    
                /* print the header's divider */
                FM_LOG_PRINT( "-------------------"
                              "---------"
                              "-------------------------------"
                              "---------------------------------------"
                              "-------------------------------"
                              "----------");
                FM_LOG_PRINT("\n");
    
                for ( i = 0 ; i < numTransitions ; i++ )
                {
                   if ( history[i].currentState != FM_STATE_UNSPECIFIED )
                   {
                       currentState = fm10000SerDesDfeStatesMap[history[i].currentState];
                   }
                   else
                   {
                       currentState = "N/A";
                   }
                   if ( history[i].eventInfo.eventId != FM_EVENT_UNSPECIFIED )
                   {
                       event = fm10000SerDesDfeEventsMap[history[i].eventInfo.eventId];
                   }
                   else
                   {
                       event = "N/A";
                   }
                   FM_LOG_PRINT( "%8lld.%03lld secs   ", 
                           history[i].eventTime.sec, 
                           history[i].eventTime.usec/1000 );
    
                   FM_LOG_PRINT("%-6d   ",  history[i].eventInfo.smType );
                   FM_LOG_PRINT("%-28s   ", currentState );
                   FM_LOG_PRINT("%-36s   ", event );
                   FM_LOG_PRINT("%-28s   ", fm10000SerDesDfeStatesMap[history[i].nextState]);
                   FM_LOG_PRINT("%-7d   ",  history[i].status);
                   FM_LOG_PRINT("\n");
                }
            }
    
            fmFree( history );
        }

        /* next lane, if any */
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );

    }   /* end while (laneExt != NULL ) */


    FM_LOG_PRINT("\nAll history buffers can contain up to %d transitions "
                 "at this time\n", portExt->transitionHistorySize );
ABORT:

    return status;

}   /* end fm10000DbgDumpPortStateTransitions */


/*****************************************************************************/
/** fm10000DbgClearPortStateTransitions
 * \ingroup intPort
 *
 * \desc            Function to clear a Port's State Transition history.
 *
 * \param[in]       sw the ID of the switch the port belongs to.
 * 
 * \param[in]       port is the logical port ID.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgClearPortStateTransitions( fm_int sw, fm_int port )
{
    fm10000_port *portExt;
    fm10000_lane *laneExt;

    portExt = GET_PORT_EXT( sw, port );

    /* clear link-level and AN-level state transition history */
    fmClearStateTransitionHistory( portExt->smHandle );
    fmClearStateTransitionHistory( portExt->anSmHandle );

    /* Go through all lanes associated to this port */
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {

        /* Clear SerDes-level and DFE-level state transition history */
        fmClearStateTransitionHistory( laneExt->smHandle );
        fmClearStateTransitionHistory( laneExt->dfeExt.smHandle );

        /* next lane, if any */
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    return FM_OK;

}   /* end fm10000DbgClearPortStateTransitions */


/*****************************************************************************/
/** fm10000DbgSetPortStateTransitionHistorySize
 * \ingroup intDiag
 *
 * \desc            Change the size of the state transition history buffer
 *                  for a given logical port
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port for which to retrieve attribute settings.
 *                  Can't be a LAG logical port.
 * 
 * \param[in]       size is the new size of the history buffer.
 *
 * \return          FM_OK if successful
 * 
 * \return          FM_ERR_NO_MEM if there was a memory allocation problem
 * 
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 *
 * \return          FM_ERR_INVALID_PORT if the port ID is invalid
 * 
 *****************************************************************************/
fm_status fm10000DbgSetPortStateTransitionHistorySize( fm_int sw, 
                                                       fm_int port, 
                                                       fm_int size )
{
    fm_status     status;
    fm_status     lastErr;
    fm10000_port *portExt;
    fm10000_lane *laneExt;

    portExt = GET_PORT_EXT( sw, port );

    lastErr = FM_OK;

    /* Set the transitions  history size fo the link-level state machine */
    status = fmChangeStateTransitionHistorySize( portExt->smHandle, size ); 
    if ( status != FM_OK )
    {
        lastErr = status;
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                          port,
                          "Couldn't change history size for port %d's "
                          "link-level state machine\n", 
                          port);
    }
    else
    {
        portExt->transitionHistorySize = size;
    }

    /* Set the transitions  history size fo the AN-level state machine */
    status = fmChangeStateTransitionHistorySize( portExt->anSmHandle, size );
    if ( status != FM_OK )
    {
        lastErr = status;
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                         port,
                         "Couldn't change history size for port %d's "
                         "AN-level state machine\n", 
                         port);
    }
    else
    {
        portExt->anTransitionHistorySize = size;
    }

    /* Go through all lanes associated to this port */
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {

        /* Clear SerDes-level and DFE-level state transition history */
        status = fmChangeStateTransitionHistorySize( laneExt->smHandle, size );
        if ( status != FM_OK )
        {
            lastErr = status;
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Couldn't change history size for port %d's "
                             "SerDes-level state machine (lane %d)\n", 
                             port,
                             laneExt->physLane );
        }
        else
        {
            laneExt->transitionHistorySize = size;
        }

        status = fmChangeStateTransitionHistorySize( laneExt->dfeExt.smHandle,
                                                     size );
        if ( status != FM_OK )
        {
            lastErr = status;
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Couldn't change history size for port %d's "
                             "DFE-level state machine (lane %d)\n", 
                             port,
                             laneExt->physLane );
        }
        else
        {
            laneExt->dfeExt.transitionHistorySize = size;
        }

        /* next lane, if any */
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    return lastErr;


}   /* end fm10000DbgSetPortStateTransitionHistorySize */


/*****************************************************************************/
/** fm10000IsPciePort
 * \ingroup intlport
 *
 * \desc            Helper function to check whether a logical port belongs to
 *                  a PCIE Express Endpoint
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[out]      isPciePort is the pointer to a caller-provided boolean
 *                  variable where this function will return TRUE if the
 *                  indicated logical port belongs to a PCIE endpoint
 *
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000IsPciePort( fm_int sw, fm_int port, fm_bool *isPciePort )
{
    fm10000_port *portExt;

    portExt = GET_PORT_EXT( sw, port );

    *isPciePort = ( portExt->ring == FM10000_SERDES_RING_PCIE );

    return FM_OK;

}   /* end fm10000IsPciePort */




/*****************************************************************************/
/** fm10000IsSpecialPort
 * \ingroup intlport
 *
 * \desc            Helper function to check whether a logical port is belongs
 *                  to the special port group (TE, FIBM and Loopback ports).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[out]      isSpecialPort is the pointer to a caller-provided boolean
 *                  variable where this function will return TRUE if the
 *                  indicated logical port belongs to the special port group.
 *
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000IsSpecialPort( fm_int sw, fm_int port, fm_bool *isSpecialPort )
{
    fm10000_port *portExt;

    portExt = GET_PORT_EXT( sw, port );

    *isSpecialPort = ( portExt->ring == FM10000_SERDES_RING_NONE );

    return FM_OK;

}   /* end fm10000IsSpecialPort */




/*****************************************************************************/
/** fm10000ConfigurePepMode
 * \ingroup intPort
 *
 * \desc            Configure the PCI Express Endpoint mode
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ConfigurePepMode( fm_int sw, fm_int port )
{
    fm_status            status;
    fm_port             *portPtr;
    fm_lane             *lanePtr;
    fm_laneAttr         *laneAttr;
    fm10000_port        *portExt;
    fm10000_lane        *laneExt;
    fm_int               pep;
    fm_int               localPep;
    fm_int               serDes;
    fm_int               numSerDes;
    fm_int               physPort;
    fm_switch           *switchPtr;
    fm_uint32            deviceCfg;
    fm_int               i;
    fm10000_serDesSmMode serdesSmMode;
    fm_bool              configureIt;
    fm_uint32            pcieMode;
    fm_uint32            pcieEnable;
    fm_smEventInfo       eventInfo;
    fm10000_portAttr    *portAttrExt;
    fm_pepMode           newPepMode;


    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT, port, "sw%d, port=%d\n", sw,  port );

    switchPtr   = GET_SWITCH_PTR( sw );
    portPtr     = GET_PORT_PTR( sw, port );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );
    physPort    = portPtr->physicalPort;

    /* start the port state machine */
    status = fmStartStateMachine( portExt->smHandle, 
                                  portExt->smType,
                                  FM10000_PORT_STATE_DISABLED );
    if ( status != FM_OK )
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                         port, 
                         "Start failed on port %d "
                         "handle=%p type=%d\n",
                         port,
                         portExt->smHandle,
                         portExt->smType );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }

    /* determine the endpoint this port belongs to and its first serDes ID */
    status = fm10000MapPhysicalPortToPepSerDes( sw, 
                                                physPort,
                                                &pep, 
                                                &serDes );
    if ( status != FM_OK )
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                         port,
                         "Failed mapping physical port %d to pep and serDes "
                         "pep=%d serDes=%d\n",
                         physPort,
                         pep,
                         serDes );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }

    portExt->endpoint.pep  = pep;
    portExt->speed         = 0;
    portAttrExt->pcieMode  = FM_PORT_PCIE_MODE_DISABLED;
    portAttrExt->pepMode   = FM_PORT_PEP_MODE_DISABLED;
    newPepMode             = FM_PORT_PEP_MODE_DISABLED;
    localPep               = fm10000SerdesGetPepFromMap(serDes);

    /* initialize the lane extension structures */
    configureIt = TRUE;
    if ( pep <= FM10000_MAX_DUAL_PEP )
    {
        status = switchPtr->ReadUINT32( sw, FM10000_DEVICE_CFG(), &deviceCfg );
        if ( status != FM_OK )
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Failed reading device configuration "
                             "deviceCfg=%d\n",
                             deviceCfg );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        pcieMode = FM_GET_FIELD( deviceCfg, FM10000_DEVICE_CFG, PCIeMode );

        /* is this host interface in 2x4 mode? */
        if ( ( pcieMode & ( 1 << (pep/2) ) ) != 0 )
        {
            /* yes, four serdes to be intialized */
            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "Setting 4 serDes to be initialized for port %d\n",
                             port );
            numSerDes = FM10000_SERDES_PER_DUAL_PEP;
            newPepMode = FM_PORT_PEP_MODE_2X4;
        }
        else
        {
            /* no, is this an even numbered PEP? */
            if ( ( localPep % 2 ) == 0 )
            {
                /* yes, eight serdes to be initialized */
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Setting 8 serDes to be initialized for port %d\n",
                                 port );
                numSerDes = 2*FM10000_SERDES_PER_DUAL_PEP;
                newPepMode = FM_PORT_PEP_MODE_1X8;
            }
            else
            {
                /* odd numbered PEP in eight lane modes, no serdes to be
                   initialized. Plus we'll skip sending the CONFIGURE event
                   to this port's state machine */
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Odd PEP number %d, no serDes to be initialized "
                                 "for port %d\n",
                                 localPep,
                                 port );
                numSerDes = 0;
                configureIt = FALSE;
            }
        }

        /* skip sending the CONFIGURE event if this PEP is disabled */
        pcieEnable = FM_GET_FIELD( deviceCfg, FM10000_DEVICE_CFG, PCIeEnable );
        if ( ( pcieEnable & ( 1 << pep ) ) == 0 )
        {
            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "PEP %d disabled, skipping configuration "
                             "for port %d\n",
                             pep,
                             port );
            configureIt = FALSE;
        }

    }
    else
    {
        /* single lane PEP (CPU port) */
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Setting serDes to be initialized for CPU port\n");
        numSerDes = FM10000_SERDES_PER_SINGLE_PEP;
        newPepMode = FM_PORT_PEP_MODE_1X1;
    }

    /**************************************************
     * initialize the lane extension structures for 
     * this port
     **************************************************/
    
    /* the following block will be executed only for unused (odd numbered)
       PEPs for 1x8 Host Interfaces */
    if ( numSerDes == 0 )
    {
        /**************************************************
         * Create the linkage between port and native lane
         **************************************************/

        for (i = 0 ; i < FM10000_SERDES_PER_DUAL_PEP  ; i++)
        {
            laneExt  = GET_LANE_EXT( sw, serDes + i );

            if ( i == 0 )
            {
                /* for a given port there isn't just one native lane,
                   just take lane 0 as the native lane. */
                portExt->nativeLaneExt = laneExt;
            }
            laneExt->nativePortExt = portExt;
        }
    }


    /* this block will be executed for all active PEPs (1x1, 2x4 or 1x8 */
    for ( i = 0 ; i < numSerDes  ; i++ )
    {
        /* fill the serdes-level event info structure */
        lanePtr  = GET_LANE_PTR( sw, serDes + i );
        laneAttr = GET_LANE_ATTR( sw, serDes + i );
        laneExt  = GET_LANE_EXT( sw, serDes + i );
        FM_DLL_INIT_NODE( laneExt, nextLane, prevLane );

        /**************************************************
         * Create the linkage between port and native lane
         **************************************************/

        if ( i == 0 )
        {
            /* for a given port there isn't just one native lane,
               just take lane 0 as the native lane. */
            portExt->nativeLaneExt = laneExt;
        }
        if ( i < FM10000_SERDES_PER_DUAL_PEP )
        {
            /* skip it for SerDes belonging to an unused PEP in 1x8 mode */
            laneExt->nativePortExt = portExt;
        }


        /**************************************************
         * The API-level lane ID is initialized only when 
         * the ethMode is configured. 
         **************************************************/

        laneExt->physLane = i;
        laneExt->channel  = i;

        /* determine the type of state machine to be used at serdes-level */
        status = fm10000SerdesGetOpMode(sw, serDes, NULL, &serdesSmMode, NULL);
        if (status == FM_OK)
        {
            if (serdesSmMode == FM10000_SERDES_USE_STUB_STATE_MACHINE)
            {
                /* we are in test mode */
                laneExt->smType = FM10000_STUB_SERDES_STATE_MACHINE;
            }
            else
            {
                /* we aren't in test mode, default to the basic */
                laneExt->smType = FM10000_PCIE_SERDES_STATE_MACHINE;
            }
            status = fmStartStateMachine( laneExt->smHandle,
                                          laneExt->smType,
                                          FM10000_SERDES_STATE_DISABLED );
        }
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    }   /* end for ( i = 0 ; i < numSerDes  ; i++ ) */


    /* Do we need to send the CONFIGURE event to this port's state machine? */
    if ( configureIt == TRUE )
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Sending FM10000_PORT_EVENT_CONFIG_REQ event for port %d\n",
                         port );
        /* yes, fill out the event info */
        eventInfo.eventId        = FM10000_PORT_EVENT_CONFIG_REQ;
        eventInfo.smType         = portExt->smType;
        eventInfo.lock           = FM_GET_STATE_LOCK( sw );
        eventInfo.dontSaveRecord = FALSE;
        portExt->eventInfo.info.config.pepMode = newPepMode;

        /* notify the event */
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->smHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        portAttrExt->pepMode = newPepMode;
     }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, status );

}   /* end fm10000ConfigurePepMode */




/*****************************************************************************/
/** fm10000PepRecoveryHandler
 * \ingroup intPort
 *
 * \desc            Sends the link down event to the port state machine in case
 *                  of a stuck interrupt for a given pep
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port of the pep on
 *                  which we need to send link down event
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000PepRecoveryHandler( fm_int sw, 
                                     fm_int port )
{
    fm_status      status;
    fm10000_port * portExt;
    fm_smEventInfo eventInfo;

    FM_CLEAR(eventInfo);

    /* Notify the port state machine of a PCIE port going down */
    portExt = GET_PORT_EXT(sw, port);
    eventInfo.smType         = portExt->smType;
    eventInfo.lock           = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    eventInfo.eventId        = FM10000_PORT_EVENT_LINK_DOWN_IND;

    portExt->eventInfo.regLockTaken = FALSE;
    FM_TAKE_MAILBOX_LOCK( sw );
    status = fmNotifyStateMachineEvent( portExt->smHandle,
                                        &eventInfo,
                                        &portExt->eventInfo,
                                        &port );
    FM_DROP_MAILBOX_LOCK( sw );

    return status;

}    /* end fm10000PepRecoveryHandler */




/*****************************************************************************/
/** fm10000PepEventHandler
 * \ingroup intPort
 *
 * \desc            Process interrupt events for a given PEP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number.
 * 
 * \param[in]       pepIp is a snapshot of the current Interrupt Pending
 *                  register for this PEP.
 * 
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
fm_status fm10000PepEventHandler( fm_int sw, 
                                  fm_int port, 
                                  fm_uint32 pepIp )
{
    fm_switch      *switchPtr;
    fm_status       status;
    fm_uint32       addr;
    fm_uint32       reg;
    fm_smEventInfo  eventInfo;
    fm10000_port   *portExt;
    fm_int          pep;

    status = FM_OK;

    /* Map the logical port to its pep */
    status = fm10000MapLogicalPortToPep(sw, port, &pep);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    if ( FM_GET_BIT( pepIp, FM10000_PCIE_IP, DeviceStateChange ) )
    {
        switchPtr = GET_SWITCH_PTR( sw );

        addr = FM10000_PCIE_FACTPS();
        status = fm10000ReadPep( sw, addr, pep, &reg );
        
        /*  send the link event */
        if ( (status == FM_OK) && 
             (FM_GET_FIELD( reg, FM10000_PCIE_FACTPS, Func0PowerState) == 2 ) )
        {
            /* device is in state D0a, link is UP */
            eventInfo.eventId = FM10000_PORT_EVENT_LINK_UP_IND;
        }
        else
        {
            /* otherwise link is DOWN */
            eventInfo.eventId = FM10000_PORT_EVENT_LINK_DOWN_IND;
        }

        portExt = GET_PORT_EXT(sw, port);
        eventInfo.smType         = portExt->smType;
        eventInfo.lock           = FM_GET_STATE_LOCK( sw );
        eventInfo.dontSaveRecord = FALSE;

        portExt->eventInfo.regLockTaken = FALSE;
        FM_TAKE_MAILBOX_LOCK( sw );
        status = fmNotifyStateMachineEvent( portExt->smHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_DROP_MAILBOX_LOCK( sw );
    }
ABORT:
    return status;

}   /* end fm10000PepEventHandler */




/*****************************************************************************/
/** fm10000GetNumPortLanes
 * \ingroup intPort
 *
 * \desc            This function returns the number of lanes associated to a
 *                  given port
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number on which to operate. May 
 *                  not be a LAG logical. It can be a CPU interface.
 *
 * \param[in]       mac is the port's zero-based MAC number on which to operate. 
 *                  May be specified as zero or FM_PORT_ACTIVE_MAC
 *
 * \param[out]      numLanes is the pointer to a caller allocated area where
 *                  this function will return the number of lanes associated
 *                  to the given port 
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_PORT_MAC if mac is not valid 
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 * 
 *****************************************************************************/
fm_status fm10000GetNumPortLanes( fm_int sw, 
                                  fm_int port, 
                                  fm_int mac, 
                                  fm_int *numLanes )
{
    /* check some of the arguments */
    if ( mac != FM_PORT_ACTIVE_MAC && mac != 0 )
    {
        return FM_ERR_INVALID_PORT_MAC;
    }

    return fm10000GetNumLanes( sw, port, numLanes );

}   /* end fm10000GetNumPortLanes */



/*****************************************************************************/
/** fm10000LinkEventHandler
 * \ingroup intPort
 *
 * \desc            Function that processes link-level interrupts
 *
 * \param[in]       sw is the switch on which to operate 
 * 
 * \param[in]       epl is the ID of the EPL on which the event occurred
 *
 * \param[in]       lane is the ID of the lane on which the event occurred
 * 
 * \param[in]       linkIp is the interrupt pending mask for this EPL lane
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000LinkEventHandler( fm_int    sw, 
                                   fm_int    epl, 
                                   fm_int    lane,
                                   fm_uint32 linkIp )
{
    fm_status         status;
    fm_int            serDes;
    fm_int            port;
    fm10000_lane     *laneExt;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_smEventInfo    eventInfo;
    fm_switch        *switchPtr;
    fm_uint32         addr;
    fm_uint32         portStatus;
    fm_int            linkFaultState;
    fm_bool           sendIt;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];

    switchPtr = GET_SWITCH_PTR( sw );

    status = fm10000MapEplLaneToSerdes( sw, epl, lane, &serDes );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT_AUTONEG, status );

    /* only process it if a lane is associated to this serdes */
    laneExt = GET_LANE_EXT( sw, serDes );
    if ( laneExt != NULL )
    {
        /* only process it if the lane is currently mapped to an active port */
        portExt = laneExt->parentPortExt;
        if ( portExt != NULL )
        {
            port = portExt->base->portNumber;

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "Link Interrupt on port %d (type %d): 0x%08x\n", 
                             port, 
                             portExt->smType,
                             linkIp );


            /* get the port status register */
            if ( FM_GET_BIT( linkIp, FM10000_LINK_IP, LinkFaultDebounced ) )
            {
                addr = FM10000_PORT_STATUS( epl, lane );

                /* read the port status and extract the relevant fields */
                status = switchPtr->ReadUINT32( sw, addr, &portStatus );

                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

                linkFaultState = FM_GET_FIELD( portStatus, 
                                               FM10000_PORT_STATUS, 
                                               LinkFaultDebounced );

                eventInfo.smType = portExt->smType;
                eventInfo.lock   = FM_GET_STATE_LOCK( sw );
                eventInfo.dontSaveRecord = FALSE;

                /* Determine the event to be sent based on the link fault */
                sendIt = TRUE;
                switch ( linkFaultState )
                {
                    case 0:
                        /* link ok */
                        eventInfo.eventId = FM10000_PORT_EVENT_LINK_UP_IND;
                        break;

                    case 1:
                        /* local fault */
                        eventInfo.eventId = FM10000_PORT_EVENT_LOCAL_FAULT_IND;
                        break;

                    case 2:
                        /* remote fault */
                        eventInfo.eventId = FM10000_PORT_EVENT_REMOTE_FAULT_IND;
                        break;
                
                    default:
                        sendIt = FALSE;
                        break;
                }

                if ( sendIt )
                {
                    portExt->eventInfo.regLockTaken = FALSE;
                    status = fmNotifyStateMachineEvent( portExt->smHandle,
                                                        &eventInfo,
                                                        &portExt->eventInfo,
                                                        &port );
                }

            }   

            if ( FM_GET_BIT( linkIp, FM10000_LINK_IP, EeePcSilent ) )
            {
                port = portExt->base->portNumber;

                status = HandleEeePcSilent(sw, port, epl, lane);
                if ( status != FM_OK )
                {
                    FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                                    port,
                                    "HandleEeePcSilent invalid status (%d)\n",
                                    status);
                }
            }   

            if ( FM_GET_BIT( linkIp, FM10000_LINK_IP, LpIdleIndicate ) )
            {
                port = portExt->base->portNumber;
                portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

                /* Increment diagnostic counter */
                portAttrExt->lpiRxIndicateCnt++;

                status = FM_OK;
            }

            if ( FM_GET_BIT( linkIp, FM10000_LINK_IP, LpiWakeError ) )
            {
                port = portExt->base->portNumber;
                portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

                /* Increment diagnostic counter */
                portAttrExt->lpiWakeErrorCnt++;

                status = FM_OK;
            }

            if ( FM_GET_BIT( linkIp, FM10000_LINK_IP, EgressTimeStamp ) )
            {
                port = portExt->base->portNumber;

                status = HandleEgressTimeStamp(sw, port, epl, lane);
                if ( status != FM_OK )
                {
                    FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                                    port,
                                    "HandleEgressTimeStamp invalid status (%d)\n",
                                    status);
                }

            }   /* end if ( FM_GET_BIT( ... ) ) */
                
        }   /* end if ( portExt != NULL ) */

    }   /* end if ( laneExt != NULL ) */


ABORT:
    status = switchPtr->MaskUINT32( sw,
                                    FM10000_LINK_IM(epl,lane),
                                    linkIp,
                                    FALSE );
    return status;

}   /* end fm10000LinkEventHandler */


/*****************************************************************************/
/** fm10000MapEthModeToDfeMode
 * \ingroup intPort
 *
 * \desc            Function that maps the desired ethMode on a given port to
 *                  the DFE mode on any of its associated lanes.
 *
 * \param[in]       sw is the switch on which to operate 
 * 
 * \param[in]       port is the ID of the port on which to operate
 *
 * \param[in]       lane is the relative ID of the lane on which to operate
 * 
 * \param[in]       ethMode is the desired ethMode on this port
 * 
 * \param[out]      dfeMode is a pointer to a caller-allocated variable where
 *                  this function will return the DFE mode for the given lane
 *
 * \return          FM_OK if successfull
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if the pointer argument is invalid
 * 
 * \return          FM_ERR_INVALID_ETH if the Ethernet mode currently
 *                  configured on the port is invalid
 * 
 * \return          FM_ERR_UNSUPPORTED if the Ethernet mode currently
 *                  configured on the port is not supported for this chip
 *****************************************************************************/
fm_status fm10000MapEthModeToDfeMode( fm_int      sw, 
                                      fm_int      port, 
                                      fm_int      lane,
                                      fm_ethMode  ethMode,
                                      fm_dfeMode *dfeMode )
{
    fm_status      status;
    fm_laneAttr   *laneAttr;
    fm_int         serDes;

    switch ( ethMode )
    {
        /* Ethernet modes for which DFE can only be static */
        case FM_ETH_MODE_DISABLED:
        case FM_ETH_MODE_SGMII:
        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_1000BASE_KX:
        case FM_ETH_MODE_2500BASE_X:
        case FM_ETH_MODE_AN_73:

            *dfeMode = FM_DFE_MODE_STATIC;
            status   = FM_OK;
            break;

        /* Negotiated Ethernet Modes for which DFE can only be KR-based */
        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_25GBASE_KR:
        case FM_ETH_MODE_25GBASE_CR:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_KR4:

            *dfeMode = FM_DFE_MODE_KR;
            status   = FM_OK;
            break;

        /* Ethernet modes for which DFE can be whatever was administratively
           configured */
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_40GBASE_SR4:
        case FM_ETH_MODE_100GBASE_SR4:

            status = fm10000MapPortLaneToSerdes( sw, port, lane, &serDes );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            laneAttr = GET_LANE_ATTR( sw, serDes );
            *dfeMode = laneAttr->dfeMode;

            status = FM_OK;
            break;

        /* Unsupported Ethernet Modes */
        case FM_ETH_MODE_6GBASE_KR:
        case FM_ETH_MODE_6GBASE_CR:
        case FM_ETH_MODE_XAUI:
        case FM_ETH_MODE_10GBASE_KX4:
        case FM_ETH_MODE_10GBASE_CX4:
        case FM_ETH_MODE_24GBASE_KR4:
        case FM_ETH_MODE_24GBASE_CR4:
        case FM_ETH_MODE_XLAUI:
            status = FM_ERR_UNSUPPORTED;
            break;

        /* Invalid Ethernet Modes */
        default:
            status = FM_ERR_INVALID_ETH_MODE;
            break;
    }
    
ABORT:
    return status;

}   /* end fm10000MapEthModeToDfeMode */



/*****************************************************************************/
/** fm10000IsPortDisabled
 * \ingroup intPort
 *
 * \desc            This function indicates if a port is disabled
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number on which to operate. May 
 *                  not be a LAG logical. It can be a CPU interface.
 *
 * \param[in]       mac is the port's zero-based MAC number on which to operate. 
 *                  May be specified as zero or FM_PORT_ACTIVE_MAC
 *
 * \param[out]      isDisabled is the pointer to a caller allocated area where
 *                  this function will return TRUE is the port is disabled,
 *                  FALSE otherwise
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_PORT_MAC if mac is not valid 
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 * 
 *****************************************************************************/
fm_status fm10000IsPortDisabled( fm_int   sw, 
                                 fm_int   port, 
                                 fm_int   mac, 
                                 fm_bool *isDisabled )
{
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;

    /* check some of the arguments */
    if ( mac != FM_PORT_ACTIVE_MAC && mac != 0 )
    {
        return FM_ERR_INVALID_PORT_MAC;
    }

    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    *isDisabled = FALSE;
    if ( ( portExt->ring        == FM10000_SERDES_RING_EPL   && 
          portExt->ethMode     == FM_ETH_MODE_DISABLED )    ||
        ( portExt->ring        == FM10000_SERDES_RING_PCIE  &&
          portAttrExt->pepMode == FM_PORT_PEP_MODE_DISABLED ) )
    {
        *isDisabled = TRUE;
    }

    return FM_OK;


}   /* end fm10000IsPortDisabled */




/*****************************************************************************/
/** fm10000GetPortEyeDiagram 
 * \ingroup intPort 
 *
 * \desc            This function collects FM10000_PORT_EYE_SAMPLES eye
 *                  diagram sample points from a given Ethernet logical port
 *                  by computing the bit error rate for every combination of
 *                  phase and offset level.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate.
 * 
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. It may not be specified as FM_PORT_LANE_NA or
 *                  FM_PORT_LANE_ALL.
 * 
 * \param[out]      pSampleTable is a pointer to a caller-allocated area where
 *                  this function will return FM10000_PORT_EYE_SAMPLES
 *                  eye sample points.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not UP yet.
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid.
 * \return          FM_ERR_INVALID_PORT_MAC if the MAC ID is invalid.
 * \return          FM_ERR_INVALID_PORT_LANE if the lane ID is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if sample table is NULL.
 *
 *****************************************************************************/
fm_status fm10000GetPortEyeDiagram(fm_int               sw,
                                   fm_int               port,
                                   fm_int               lane,
                                   fm_eyeDiagramSample *pSampleTable)
{
    fm_status        err;
    fm_int           serDes;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    port,
                    "sw=%d port=%d lane=%d pSampleTable=%p\n",
                    sw,
                    port,
                    lane,
                    (void*) pSampleTable);

    /* validate switch ID */
    VALIDATE_SWITCH_INDEX(sw);


    /* argument validation */
    if ( pSampleTable == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    /*  do not accept FM_PORT_LANE_ALL  or FM_PORT_LANE_NA */
    else if ( lane == FM_PORT_LANE_ALL || lane == FM_PORT_LANE_NA )
    {
        err = FM_ERR_INVALID_PORT_LANE;
    }
    /* Validate logical port ID */
    /* This validation is forced for the time being */
    else if (ValidatePort(sw, port) == FALSE && 0)
    {
        err = FM_ERR_INVALID_PORT;
    }
    else
    {
        /* Get the serDes ID */
        err = fm10000MapPortLaneToSerdes(sw, port, lane, &serDes);
    
        if (err == FM_OK)
        {
            /* get the SerDes eye diagram */
            err = fm10000SerDesGetEyeDiagram(sw, serDes, pSampleTable);
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                            port,
                            "Cannot determine the serDes for port=%d\n",
                            port);
        }

    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, port, err);

}   /* end fm10000GetPortEyeDiagram */




/*****************************************************************************/
/** fm10000DbgMapLogicalPort 
 * \ingroup intPort 
 *
 * \desc            Generic logical port mapping function for FM10000. It Maps
 *                  the given logical port according to the mappingType
 *                  specification.
 *                  The goal of this function is mostly to be used for low
 *                  level mappings, such as logical ports to serdes address,
 *                  which are required by some debug functions.
 *                  In some cases that the mapped value is returned as a
 *                  tuple: one element is encoded in the lower 16 bits and the
 *                  other one in the upper 16 bits, as follows:
 *                   -FM_MAP_LOGICAL_PORT_TO_PHYSICAL_PORT:
 *                      *pMapped[0..15]:  physical port
 *                      *pMapped[16..31]: physical switch
 *                   -FM_MAP_LOGICAL_PORT_TO_SERDES_ADDRESS:
 *                      *pMapped[0..15]:  serdes sbus address
 *                      *pMapped[16..31]: ring (1:EPL ring; 0:PCIe ring)
 *                   -FM_MAP_LOGICAL_PORT_TO_EPL_ABS_LANE:
 *                      *pMapped[0..15]:  epl
 *                      *pMapped[0..15]:  absolute lane   
 *                   -FM_MAP_LOGICAL_PORT_TO_EPL_CHANNEL:
 *                      *pMapped[0..15]:  epl
 *                      *pMapped[0..15]:  channel   
 * 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logPort is the logical port number to be mapped
 * 
 * \param[in]       lane is the relative number of the lane for multi-lane
 *                  ports. It must be specified as 0 for single lane ports.
 *
 * \param[in]       mappingType specifies the required mapping. See
 *                  ''fmLogPortMappingType''.
 * 
 * \param[out]      pMapped points to caller-allocated storage where this
 *                  function should place the mapped value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if pMapped table is NULL.
 *
 *****************************************************************************/
fm_status fm10000DbgMapLogicalPort(fm_int                 sw,
                                   fm_int                 logPort,
                                   fm_int                 lane,
                                   fm_logPortMappingType  mappingType,
                                   fm_int                *pMapped)
{
    fm_status       err;
    fm_int          physSwitch;
    fm_int          physPort;
    fm_uint         sbusAddr;
    fm_int          serdesId;
    fm_serdesRing   ring;
    fm_int          epl;
    fm_int          absLane;
    fm_int          channel;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_PORT,
                    logPort,
                    "sw=%d, logPort=%d, lane=%d, mappingType=%d, pSampleTable=%p\n",
                    sw,
                    logPort,
                    lane,
                    mappingType,
                    (void*) pMapped);

    /* validate switch ID */
    VALIDATE_SWITCH_INDEX(sw);


    /* argument validation */
    if ( pMapped == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* set a default returned value */
        *pMapped = 0;

        switch (mappingType)
        {
            case FM_MAP_LOGICAL_PORT_TO_PHYSICAL_PORT:
            {
                err = fmPlatformMapLogicalPortToPhysical(sw, logPort, &physSwitch, &physPort);
                if (err == FM_OK)
                {
                    *pMapped = physSwitch << 16 || physPort;
                }
                break;
            }
            case FM_MAP_LOGICAL_PORT_TO_FABRIC_PORT:
            {
                err = fm10000MapLogicalPortToFabricPort(sw,logPort,pMapped);
                break;
            }
            case    FM_MAP_LOGICAL_PORT_TO_SERDES_ID:
            {
                err = fm10000MapPortLaneToSerdes(sw, logPort, lane, pMapped);
                break;
            }
            case FM_MAP_LOGICAL_PORT_TO_SERDES_ADDRESS:
            {
                err = fm10000MapPortLaneToSerdes(sw, logPort, lane, &serdesId);
                if (err == FM_OK)
                {
                    err = fm10000MapSerdesToSbus(sw, serdesId, &sbusAddr, &ring);

                    if (err == FM_OK)
                    {
                        *pMapped = ring<<16 || sbusAddr;
                    }
                }
                break;
            }
            case FM_MAP_LOGICAL_PORT_TO_EPL_ABS_LANE:
            {
                err = fm10000MapLogicalPortToEplLane(sw, logPort, &epl, &absLane);
                if (err == FM_OK)
                {
                    *pMapped = absLane<<16 || epl;
                }
                break;
            }
            case FM_MAP_LOGICAL_PORT_TO_EPL_CHANNEL:
            {
                err = fmPlatformMapLogicalPortToPhysical(sw, logPort, &physSwitch, &physPort);
                if (err == FM_OK)
                {
                    err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
                    if (err == FM_OK)
                    {
                        *pMapped = channel<<16 || epl;
                    }

                }
                break;
            }
            default:
                err = FM_ERR_INVALID_ARGUMENT;
        }

    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT, logPort, err);

}   /* end fm10000DbgMapLogicalPort */





/*****************************************************************************/
/** fm10000UpdateAllSAFValues
 * \ingroup intPort
 *
 * \desc            Called whenever port speeds change, this compares port
 *                  speeds to determine which port pairs must be set to
 *                  store-and-forward.  All of these port pairs, plus those
 *                  requested by the user, are set to store-and-forward mode.
 *                  Other port pairs are set to cut-through mode.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateAllSAFValues(fm_int sw)
{
    fm_status               err;
    fm_int                  cpi;
    fm_int                  port;
    fm_int                  physPort;
    fm_switch *             switchPtr;
    fm_int *                portSpeeds;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    portSpeeds = fmAlloc(sizeof(fm_int) *  switchPtr->numCardinalPorts);
    if (!portSpeeds)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_NO_MEM);
    }
    
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

        err = fm10000GetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_SPEED,
                                      &portSpeeds[cpi]);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);
    }

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = UpdateSAFValuesForCardinalPort(sw, cpi, portSpeeds);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

ABORT:
    fmFree(portSpeeds);
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fm10000UpdateAllSAFValues */




/*****************************************************************************/
/** fm10000DbgDumpPortEeeStatus
 * \ingroup intPort
 *
 * \desc            Dumps the port's EEE state and counters.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       port is the logical port number.
 * 
 * \param[in]       clear is TRUE if the counters should be reset.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM in case of memory allocation failures.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpPortEeeStatus( fm_int sw, fm_int port, fm_bool clear )
{
    fm_status         status=FM_OK;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_int            epl;
    fm_int            physLane;
    fm_uint32         pcs1000BaseXAddr=0;
    fm_uint32         pcs1000BaseXReg;
    fm_uint32         pcs10GBaserAddr=0;
    fm_uint32         pcs10GBaserReg;
    fm_uint32         addr=0;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_uint32         portStatus;
    fm_uint32         laneStatus;
    fm_bool           enEee;
    fm_bool           eeeSlPcSilent;
    fm_bool           txLpIdle;
    fm_bool           rxLpIdle;
    fm_bool           signalDetect;
    fm_bool           energyDetect;
    fm_bool           txLpiAutomatic;
    fm_bool           txLpIdleRequest;
    fm_int            txPcActTimeScale;
    fm_int            txPcActTimeout;  
    fm_int            txLpiTimescale;  
    fm_int            txLpiTimeout;    
    fm_int            txLpiHoldTimescale;  
    fm_int            txLpiHoldTimeout;    
    fm_int            txDrainMode;


    switchPtr   = GET_SWITCH_PTR( sw );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

/*    TAKE_REG_LOCK( sw ); */

    FM_LOG_PRINT("\nPort=%d, epl=%d, Lane=%d, EthMode=0x%x   ", 
                 port,
                 epl,
                 physLane,
                 portAttrExt->ethMode);

    /* Read the FM10000_PCS_1000BASEX_CFG register */
    pcs1000BaseXAddr = FM10000_PCS_1000BASEX_CFG( epl, physLane );
    status = switchPtr->ReadUINT32( sw, pcs1000BaseXAddr, &pcs1000BaseXReg );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    enEee = FM_GET_BIT( pcs1000BaseXReg, FM10000_PCS_1000BASEX_CFG, EnEee );

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nPCS_1000BASEX_CFG : 0x%x   ", pcs1000BaseXReg);
    FM_LOG_PRINT("\n  EnEee : %d   ", enEee);


    /* Read the FM10000_PCS_10GBASER_CFG register */
    pcs10GBaserAddr = FM10000_PCS_10GBASER_CFG( epl, physLane );
    status = switchPtr->ReadUINT32( sw, pcs10GBaserAddr, &pcs10GBaserReg );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    enEee = FM_GET_BIT( pcs10GBaserReg, FM10000_PCS_10GBASER_CFG, EnEee );

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nPCS_10GBASER_CFG  : 0x%x   ", pcs10GBaserReg);
    FM_LOG_PRINT("\n  EnEee : %d   ", enEee);


    /* Read the FM10000_MAC_CFG register */
    addr = FM10000_MAC_CFG(epl, physLane, 0);
    status = switchPtr->ReadUINT32Mult(sw,
                                       addr,
                                       FM10000_MAC_CFG_WIDTH,
                                       macCfg);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    txLpiAutomatic     = FM_ARRAY_GET_BIT( macCfg,
                                           FM10000_MAC_CFG,
                                           TxLpiAutomatic );
    txLpIdleRequest    = FM_ARRAY_GET_BIT( macCfg,
                                           FM10000_MAC_CFG,
                                           TxLpIdleRequest );
    txPcActTimeScale   = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxPcActTimeScale );
    txPcActTimeout     = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxPcActTimeout );
    txLpiTimescale     = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxLpiTimescale );
    txLpiTimeout       = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxLpiTimeout );
    txLpiHoldTimescale = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxLpiHoldTimescale );
    txLpiHoldTimeout   = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxLpiHoldTimeout );
    txDrainMode        = FM_ARRAY_GET_FIELD( macCfg,
                                             FM10000_MAC_CFG,
                                             TxDrainMode );

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nMAC_CFG");
    FM_LOG_PRINT("\n  TxLpiAutomatic     : %d   ", txLpiAutomatic);
    FM_LOG_PRINT("\n  TxLpIdleRequest    : %d   ", txLpIdleRequest);
    FM_LOG_PRINT("\n  TxPcActTimeScale   : %d   ", txPcActTimeScale);
    FM_LOG_PRINT("\n  TxPcActTimeout     : %d   ", txPcActTimeout);
    FM_LOG_PRINT("\n  TxLpiTimescale     : %d   ", txLpiTimescale);
    FM_LOG_PRINT("\n  TxLpiTimeout       : %d   ", txLpiTimeout);
    FM_LOG_PRINT("\n  TxLpiHoldTimescale : %d   ", txLpiHoldTimescale);
    FM_LOG_PRINT("\n  TxLpiHoldTimeout   : %d   ", txLpiHoldTimeout);
    FM_LOG_PRINT("\n  TxDrainMode        : %d   ", txDrainMode);

    /* Read the port status and extract the relevant fields */
    addr = FM10000_PORT_STATUS( epl, physLane );
    status = switchPtr->ReadUINT32( sw, addr, &portStatus );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    eeeSlPcSilent = FM_GET_BIT( portStatus, 
                                      FM10000_PORT_STATUS, 
                                      EeeSlPcSilent );
    txLpIdle = FM_GET_BIT( portStatus, FM10000_PORT_STATUS, TxLpIdle );
    rxLpIdle = FM_GET_BIT( portStatus, FM10000_PORT_STATUS, RxLpIdle );

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nPORT_STATUS       : 0x%x   ", portStatus);
    FM_LOG_PRINT("\n  EeeSlPcSilent : %d   ", eeeSlPcSilent);
    FM_LOG_PRINT("\n  TxLpIdle : %d   ", txLpIdle);
    FM_LOG_PRINT("\n  RxLpIdle : %d   ", rxLpIdle);

    /* Read the port status and extract the relevant fields */
    addr = FM10000_LANE_STATUS( epl, physLane );
    status = switchPtr->ReadUINT32( sw, addr, &laneStatus );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    signalDetect = FM_GET_BIT( laneStatus, FM10000_LANE_STATUS, RxSignalDetect );
    energyDetect = FM_GET_BIT( laneStatus, FM10000_LANE_STATUS, RxEnergyDetect );

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nLANE_STATUS       : 0x%x   ", laneStatus);
    FM_LOG_PRINT("\n  RxSignalDetect : %d   ", signalDetect);
    FM_LOG_PRINT("\n  RxEnergyDetect : %d   ", energyDetect);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nEEE Diagnostic interrupt counters");
    FM_LOG_PRINT( "\n  eeePcSilentCnt   = %lld", portAttrExt->eeePcSilentCnt);
    FM_LOG_PRINT( "\n  eeePcActiveCnt   = %lld", portAttrExt->eeePcActiveCnt);
    FM_LOG_PRINT( "\n  lpiRxIndicateCnt = %lld", portAttrExt->lpiRxIndicateCnt);
    FM_LOG_PRINT( "\n  lpiWakeErrorCnt  = %lld", portAttrExt->lpiWakeErrorCnt);
    FM_LOG_PRINT( "\n  eeePcSilentDisabledCnt = %lld", 
                  portAttrExt->eeePcSilentDisabledCnt);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\n");

    if (clear)
    {
        /* Clear interrupt diagnostic counters */
        portAttrExt->eeePcSilentCnt=0;
        portAttrExt->eeePcActiveCnt=0;
        portAttrExt->lpiRxIndicateCnt=0;
        portAttrExt->lpiWakeErrorCnt=0;
        portAttrExt->eeePcSilentDisabledCnt=0;
    }

ABORT:

/*    DROP_REG_LOCK( sw ); */
    return status;

}   /* end fm10000DbgDumpPortEeeStatus */




/*****************************************************************************/
/** fm10000DbgEnablePortEee
 * \ingroup intPort
 *
 * \desc            Function to enable EEE on the port.
 *
 * \param[in]       sw the ID of the switch the port belongs to.
 * 
 * \param[in]       port is the logical port ID.
 * 
 * \param[in]       mode is the EEE mode to configure.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM in case of memory allocation failures.
 *
 *****************************************************************************/
fm_status fm10000DbgEnablePortEee( fm_int sw, fm_int port, fm_int mode )
{
    fm_status         status=FM_OK;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_int            epl;
    fm_int            physLane;
    fm_uint32         pcs1000BaseXAddr=0;
    fm_uint32         pcs1000BaseXReg;
    fm_uint32         pcs10GBaserAddr=0;
    fm_uint32         pcs10GBaserReg;
    fm_uint32         macCfgAddr=0;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_bool           eeeRxEnable=FALSE;
    fm_bool           eeeTxEnable=FALSE;
    fm_bool           regLockTaken = FALSE;

    switchPtr   = GET_SWITCH_PTR( sw );
    portExt     = GET_PORT_EXT( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );

    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

    /* Notify the serdes SM about enabling/disabling EEE */
    if ( mode )
    {
        status = fm10000NotifyEeeModeChange( sw, port, TRUE );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }
    else if ( !portAttrExt->eeeEnable )
    {
        status = fm10000NotifyEeeModeChange( sw, port, FALSE );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
    }

    if ( ( mode & FM10000_EEE_DBG_ENABLE_RX ) &&
        !( mode & FM10000_EEE_DBG_DISABLE_RX ) )
    {
        eeeRxEnable = TRUE;
    }

    if ( ( mode & FM10000_EEE_DBG_ENABLE_TX ) &&
        !( mode & FM10000_EEE_DBG_DISABLE_TX ) )

    {
        eeeTxEnable = TRUE;
    }

    FM_FLAG_TAKE_REG_LOCK(sw);

    macCfgAddr = FM10000_MAC_CFG(epl, physLane, 0);
    status = switchPtr->ReadUINT32Mult(sw,
                                       macCfgAddr,
                                       FM10000_MAC_CFG_WIDTH,
                                       macCfg);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    FM_ARRAY_SET_BIT( macCfg, FM10000_MAC_CFG, TxLpIdleRequest, eeeTxEnable );

    /* write back MAC_CFG */
    status = switchPtr->WriteUINT32Mult( sw,
                                         macCfgAddr,
                                         FM10000_MAC_CFG_WIDTH,
                                         macCfg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    portAttrExt->dbgEeeMode = mode;

    if ( (portExt->ethMode == FM_ETH_MODE_1000BASE_KX) ||
         (portExt->ethMode == FM_ETH_MODE_1000BASE_X) )
    {
        /* Read the FM10000_PCS_1000BASEX_CFG register */
        pcs1000BaseXAddr = FM10000_PCS_1000BASEX_CFG( epl, physLane );
        status = switchPtr->ReadUINT32( sw, pcs1000BaseXAddr, &pcs1000BaseXReg );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        FM_SET_BIT( pcs1000BaseXReg,
                    FM10000_PCS_1000BASEX_CFG,
                    EnEee,
                    eeeRxEnable );

        /* Write the FM10000_PCS_1000BASEX_CFG register */
        status = switchPtr->WriteUINT32( sw, pcs1000BaseXAddr, pcs1000BaseXReg );
    }
    else
    {
        /* Read the FM10000_PCS_10GBASER_CFG register */
        pcs10GBaserAddr = FM10000_PCS_10GBASER_CFG( epl, physLane );
        status = switchPtr->ReadUINT32( sw, pcs10GBaserAddr, &pcs10GBaserReg );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        FM_SET_BIT( pcs10GBaserReg,
                    FM10000_PCS_10GBASER_CFG,
                    EnEee,
                    eeeRxEnable );

        /* Write the FM10000_PCS_1000BASEX_CFG register */
        status = switchPtr->WriteUINT32( sw, pcs10GBaserAddr, pcs10GBaserReg );
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    return status;

}   /* end fm10000DbgEnablePortEee */




/*****************************************************************************/
/** fm10000WriteMacIfg
 * \ingroup intPort
 *
 * \desc            Configure the TxIdleMinIfgBytes field in the MAC_CFG
 *                  register
 * 
 * \note            If macCfg is NULL, then this function will take the,
 *                  register lock, read-modify-write MAC_CFG and release the
 *                  register lock. Otherwise the caller has the option to pass
 *                  the pointer of its local copy of the MAC_CFG register. In
 *                  that case the function will simply update the IFG field.
 * 
 * \param[in]       sw is the ID of the switch the port belongs to.
 * 
 * \param[in]       epl is the ID of the EPL
 * 
 * \param[in]       physLane is the ID of the physical lane
 * 
 * \param[in]       macCfg is the pointer to a caller allocated area containing
 *                  the current local content of the MAC_CFG register which
 *                  this function will update.
 * 
 * \param[in]       ifg is the desired value for the TxIdleMinIfgBytes field
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000WriteMacIfg( fm_int     sw, 
                              fm_int     epl, 
                              fm_uint    physLane, 
                              fm_uint32 *macCfg, 
                              fm_uint32  ifg )
{
    fm_status status;
    fm_uint32 addr;
    fm_uint32 localMacCfg[FM10000_MAC_CFG_WIDTH];
    fm_switch *switchPtr;

    if ( macCfg == NULL )
    {
        /* we need to get the current MAC_CFG */
        addr      = FM10000_MAC_CFG( epl, physLane, 0 );
        switchPtr = GET_SWITCH_PTR( sw );

        /* read-modify-write while holding the register lock */
        TAKE_REG_LOCK(sw);
        status = switchPtr->ReadUINT32Mult(sw,
                                           addr,
                                           FM10000_MAC_CFG_WIDTH,
                                           localMacCfg);
        if ( status == FM_OK )
        {
            /* write it back */
            FM_ARRAY_SET_FIELD( localMacCfg, 
                                FM10000_MAC_CFG, 
                                TxIdleMinIfgBytes, 
                                ifg );

            status = switchPtr->WriteUINT32Mult( sw,
                                                 addr,
                                                 FM10000_MAC_CFG_WIDTH,
                                                 localMacCfg );
        }
        DROP_REG_LOCK( sw );

    }
    else
    {
        /* In this case the user just wants us to update its IFG field */
        FM_ARRAY_SET_FIELD( macCfg, FM10000_MAC_CFG, TxIdleMinIfgBytes, ifg );
        status = FM_OK;
    }

    return status;
    
}   /* end fm10000WriteMacIfg */




/*****************************************************************************/
/** fm10000WriteMacDicEnable
 * \ingroup intPort
 *
 * \desc            Configure the TxIdleEnableDic field in the MAC_CFG
 *                  register
 * 
 * \note            If macCfg is NULL, then this function will take the,
 *                  register lock, read-modify-write MAC_CFG and release the
 *                  register lock. Otherwise the caller has the option to pass
 *                  the pointer of its local copy of the MAC_CFG register. In
 *                  that case the function will simply update the IFG field.
 * 
 * \param[in]       sw is the ID of the switch the port belongs to.
 * 
 * \param[in]       epl is the ID of the EPL
 * 
 * \param[in]       physLane is the ID of the physical lane
 * 
 * \param[in]       macCfg is the pointer to a caller allocated area containing
 *                  the current local content of the MAC_CFG register which
 *                  this function will update.
 * 
 * \param[in]       dicEnable is the desired value for the TxIdleEnableDic
 *                  field
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000WriteMacDicEnable( fm_int     sw, 
                                    fm_int     epl, 
                                    fm_uint    physLane, 
                                    fm_uint32 *macCfg, 
                                    fm_bool    dicEnable )
{                      
    fm_status           status;
    fm_uint32           addr;
    fm_uint32           localMacCfg[FM10000_MAC_CFG_WIDTH];
    fm_switch          *switchPtr;

    if ( macCfg == NULL )
    {
        /* we need to get the current MAC_CFG */
        addr      = FM10000_MAC_CFG( epl, physLane, 0 );
        switchPtr = GET_SWITCH_PTR( sw );

        /* read-modify-write while holding the register lock */
        TAKE_REG_LOCK(sw);
        status = switchPtr->ReadUINT32Mult(sw,
                                           addr,
                                           FM10000_MAC_CFG_WIDTH,
                                           localMacCfg);
        if ( status == FM_OK )
        {
            /* write it back */
            FM_ARRAY_SET_BIT( localMacCfg, 
                              FM10000_MAC_CFG, 
                              TxIdleEnableDic,
                              dicEnable );

            status = switchPtr->WriteUINT32Mult( sw,
                                                 addr,
                                                 FM10000_MAC_CFG_WIDTH,
                                                 localMacCfg );
        }
        DROP_REG_LOCK( sw );

    }
    else
    {
        /* In this case the user just wants us to update its dic field */
        FM_ARRAY_SET_BIT( macCfg, 
                          FM10000_MAC_CFG, 
                          TxIdleEnableDic, 
                          dicEnable );
        status = FM_OK;
    }

    return status;
    
}   /* end fm10000WriteMacDicEnable */




/*****************************************************************************/
/** fm10000GetPcsType
 * \ingroup intPort
 *
 * \desc            This function returns the PCS Type associated to a given
 *                  combination of ethernet interface mode and speed 
 *
 * \param[in]       ethMode is the ethernet interface mode
 *
 * \param[in]       speed is the line speed, to be used with PCS modes that
 *                  could be run at different speeds
 *
 * \return          The PCS type (see ''fm10000_pcsType'')
 * 
 *****************************************************************************/
fm10000_pcsTypes fm10000GetPcsType( fm_ethMode ethMode, fm_uint32 speed )
{
    fm10000_pcsTypes pcsType;

    /* invoke the appropriate PCS Initialization depending on the ethMode */
    switch ( ethMode )
    {
        case FM_ETH_MODE_SGMII:
            if ( speed == 10 )
            {
                pcsType = FM10000_PCS_SEL_SGMII_10;
            }
            else if ( speed == 100 )
            {
                pcsType = FM10000_PCS_SEL_SGMII_100;
            }
            else
            {
                pcsType = FM10000_PCS_SEL_SGMII_1000;
            }
            break;

        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_1000BASE_KX:
        case FM_ETH_MODE_2500BASE_X:
            pcsType = FM10000_PCS_SEL_1000BASEX;
            break;

        case FM_ETH_MODE_6GBASE_KR:
        case FM_ETH_MODE_6GBASE_CR:
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_25GBASE_KR:
        case FM_ETH_MODE_25GBASE_CR:
            pcsType = FM10000_PCS_SEL_10GBASER;
            break;

        case FM_ETH_MODE_XLAUI:
        case FM_ETH_MODE_24GBASE_KR4:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_24GBASE_CR4:
        case FM_ETH_MODE_40GBASE_SR4:
            pcsType = FM10000_PCS_SEL_40GBASER;
            break;

        case FM_ETH_MODE_100GBASE_KR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_SR4:
            pcsType = FM10000_PCS_SEL_100GBASER;
            break;

        case FM_ETH_MODE_AN_73:
            pcsType = FM10000_PCS_SEL_AN_73;
            break;

        default:
            /* Add support for 20GBASE-R */
            pcsType = FM10000_PCS_SEL_DISABLE;
            break;

    }   /* end switch ( ethMode ) */

    return pcsType;

}   /* end fm10000GetPcsType */




/*****************************************************************************/
/** fm10000WriteMacMinColumns
 * \ingroup intPort
 *
 * \desc            Configure the TxMinColumns field in the MAC_CFG  register
 * 
 * \note            If macCfg is NULL, then this function will take the,
 *                  register lock, read-modify-write MAC_CFG and release the
 *                  register lock. Otherwise the caller has the option to pass
 *                  the pointer of its local copy of the MAC_CFG register. In
 *                  that case the function will simply update the IFG field.
 * 
 * \param[in]       sw is the ID of the switch the port belongs to.
 * 
 * \param[in]       epl is the ID of the EPL
 * 
 * \param[in]       physLane is the ID of the physical lane
 * 
 * \param[in]       macCfg is the pointer to a caller allocated area containing
 *                  the current local content of the MAC_CFG register which
 *                  this function will update.
 * 
 * \param[in]       txPadSize is the current or desired value of the
 *                  TX_PAD_SIZE attribute
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000WriteMacMinColumns( fm_int     sw, 
                                     fm_int     epl, 
                                     fm_uint    physLane, 
                                     fm_uint32 *macCfg, 
                                     fm_uint32  txPadSize )
{
    fm_status status;
    fm_uint32 addr;
    fm_uint32 localMacCfg[FM10000_MAC_CFG_WIDTH];
    fm_switch *switchPtr;

    /* Configure frame padding.  Valid range is 40..116, i.e. 12 to 31 columns
       including preamble or FTAG */
    if ( txPadSize < 40 )
    {
        txPadSize = 40;
    }
    if ( txPadSize > 116 )
    {
        txPadSize = 116;
    }

    /* add the size of preamble or FTAG */
    txPadSize += 8;

    /* convert from bytes to columns */
    txPadSize >>= 2;


    if ( macCfg == NULL )
    {
        /* we need to get the current MAC_CFG */
        addr      = FM10000_MAC_CFG( epl, physLane, 0 );
        switchPtr = GET_SWITCH_PTR( sw );

        /* read-modify-write while holding the register lock */
        TAKE_REG_LOCK(sw);
        status = switchPtr->ReadUINT32Mult(sw,
                                           addr,
                                           FM10000_MAC_CFG_WIDTH,
                                           localMacCfg);
        if ( status == FM_OK )
        {
            /* write it back */
            FM_ARRAY_SET_FIELD( localMacCfg, 
                                FM10000_MAC_CFG, 
                                TxMinColumns,
                                txPadSize );

            status = switchPtr->WriteUINT32Mult( sw,
                                                 addr,
                                                 FM10000_MAC_CFG_WIDTH,
                                                 localMacCfg );
        }
        DROP_REG_LOCK( sw );

    }
    else
    {
        /* In this case the user wants us to update its MinColumns field */
        FM_ARRAY_SET_FIELD( macCfg, FM10000_MAC_CFG, TxMinColumns, txPadSize );
        status = FM_OK;
    }

    return status;
    
}   /* end fm10000WriteMacMinColumns */



/*****************************************************************************/
/** fm10000WriteMacTxClockCompensation
 * \ingroup intPort
 *
 * \desc            Configure the Tx Clock Compensation in the MAC_CFG register
 * 
 * \note            If macCfg is NULL, then this function will take the,
 *                  register lock, read-modify-write MAC_CFG and release the
 *                  register lock. Otherwise the caller has the option to pass
 *                  the pointer of its local copy of the MAC_CFG register. In
 *                  that case the function will simply update the register
 *                  fields that are related to clock compensation
 * 
 * \param[in]       sw is the ID of the switch the port belongs to.
 * 
 * \param[in]       epl is the ID of the EPL
 * 
 * \param[in]       physLane is the ID of the physical lane
 * 
 * \param[in]       macCfg is the pointer to a caller allocated area containing
 *                  the current local content of the MAC_CFG register which
 *                  this function will update.
 * 
 * \param[in]       txClkComp is the current or desired value of the
 *                  TX_CLOCK_COMPENSATION attribute
 *
 * \param[in]       pcsType is the current PCS type on the port
 * 
 * \param[in]       speed is the current port speed
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000WriteMacTxClockCompensation( fm_int            sw, 
                                              fm_int            epl, 
                                              fm_uint           physLane, 
                                              fm_uint32        *macCfg, 
                                              fm_uint32         txClkComp,
                                              fm10000_pcsTypes  pcsType,
                                              fm_uint32         speed  )
{
    fm_status   status;
    fm_uint32   addr;
    fm_uint32   localMacCfg[FM10000_MAC_CFG_WIDTH];
    fm_switch  *switchPtr;
    fm_uint16   timeout;

    timeout = ConvertPpmToTxClkCompensationTimeout( pcsType, speed, txClkComp );

    if ( macCfg == NULL )
    {
        /* we need to get the current MAC_CFG */
        addr      = FM10000_MAC_CFG( epl, physLane, 0 );
        switchPtr = GET_SWITCH_PTR( sw );


        /* read-modify-write while holding the register lock */
        TAKE_REG_LOCK(sw);
        status = switchPtr->ReadUINT32Mult( sw,
                                            addr,
                                            FM10000_MAC_CFG_WIDTH,
                                            localMacCfg );
        if ( status == FM_OK )
        {

            FM_ARRAY_SET_BIT( localMacCfg,
                              FM10000_MAC_CFG,
                              TxClockCompensationEnable,
                              ( timeout ? TRUE : FALSE ) );

            FM_ARRAY_SET_FIELD( localMacCfg,
                                FM10000_MAC_CFG,
                                TxClockCompensationTimeout,
                                timeout );

            status = switchPtr->WriteUINT32Mult( sw,
                                                 addr,
                                                 FM10000_MAC_CFG_WIDTH,
                                                 localMacCfg );
        }
        DROP_REG_LOCK( sw );

    }
    else
    {
        /* In this case the user wants us to update its MinColumns field */
        FM_ARRAY_SET_BIT( macCfg,
                          FM10000_MAC_CFG,
                          TxClockCompensationEnable,
                          ( timeout ? TRUE : FALSE ) );

        FM_ARRAY_SET_FIELD( macCfg,
                            FM10000_MAC_CFG,
                            TxClockCompensationTimeout,
                            timeout );
        status = FM_OK;
    }

    return status;
    
}   /* end fm10000WriteMacTxClockCompensation */




/*****************************************************************************/
/** fm10000ConfigureEthMode
 * \ingroup intPort
 *
 * \desc            Configure the Ethernet Mode on a given port
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port ID
 * 
 * \param[in]       ethMode is the desired Ethernet interface mode
 * 
 * \param[out]      restoreMode is a pointer to a caller-allocated variable
 *                  where this function will return TRUE if it has switched 
 *                  port-level state transition table and therefore the caller
 *                  has to restore the current port admin mode. The return
 *                  value will be FALSE otherwise.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConfigureEthMode( fm_int      sw, 
                                   fm_int      port, 
                                   fm_ethMode  ethMode,
                                   fm_bool    *restoreMode )
{
    fm_status            err;
    fm_int               newPortSmType;
    fm_int               newSerdesSmType;
    fm_int               curPortSmType;
    fm_int               curSerdesSmType;
    fm_port             *portPtr;
    fm10000_port        *portExt;
    fm_portAttr         *portAttr;
    fm10000_lane        *laneExt[FM10000_PORTS_PER_EPL];
    fm_smEventInfo       eventInfo;
    fm_uint32            speed;
    fm_int               numLanes;
    fm_int               serDes;
    fm_int               baseFabricPort;
    fm_int               i;
    fm_serdesRing        ring;
    fm_bool              anReady;
    fm_int               newAnSmType;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d ethMode=0x%x\n",
                     sw, 
                     port, 
                     ethMode );

    err      = FM_OK;
    portPtr  = GET_PORT_PTR( sw, port );
    portAttr = GET_PORT_ATTR( sw, port );
    portExt  = GET_PORT_EXT( sw, port );
    speed    = fm10000GetPortSpeed( ethMode );

    /**************************************************
     * Next we're going to determine if there's a need 
     * to change the state transition tables to which 
     * the port and the serdes state machines instance(s) 
     * are currently bound. That'll depend on the old 
     * and new ethMode. 
     **************************************************/
    
    /* we'll use the native lane  to determine the current SerDes State
       Transition Type. That is because the port may or may not be linked
       to any lane at this time. The native lane will always represent the
       current state transition table of any lane, if any, associated to
       this port */
    curPortSmType   = portExt->smType;
    curSerdesSmType = portExt->nativeLaneExt->smType;

    /* we don't know yet the new state transition table types */
    newPortSmType   = FM_SMTYPE_UNSPECIFIED;
    newSerdesSmType = FM_SMTYPE_UNSPECIFIED;

    /* we'll use the desired ethMode to determine them */
    switch ( ethMode )
    {
        case FM_ETH_MODE_DISABLED:
            /* if we're disabling the port we'll keep the
               current state transition tables, because all
               of them will process this event and allow
               the DISABLED state */
            newPortSmType   = curPortSmType;
            newSerdesSmType = curSerdesSmType;

            if ( newPortSmType == FM_SMTYPE_UNSPECIFIED )
            {
                newPortSmType = FM10000_BASIC_PORT_STATE_MACHINE;
            }

            if ( newSerdesSmType == FM_SMTYPE_UNSPECIFIED )
            {
                /* serdes state machine type */
                err = SetSerDesSmType( sw, port, &newSerdesSmType );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
            }
            break;

        case FM_ETH_MODE_SGMII:
        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_AN_73:

            /* if we are in one of these modes we may need to use the
               port-level AN state transition table */
            err = fm10000IsPortAutonegReady( sw, 
                                             port, 
                                             ethMode,
                                             portAttr->autoNegMode,
                                             &anReady, 
                                             &newAnSmType );
            if ( err == FM_OK && anReady == TRUE )
            {
                newPortSmType = FM10000_AN_PORT_STATE_MACHINE;
                break;
            }

            /******************************************************
             * NOTE: LACK OF BREAK STATEMENT IS INTENTIONAL HERE: 
             * if we're in one of the AN-capable ethModes but 
             * not ready for AN then we fallback to the basic 
             * state machine 
             ******************************************************/
            
        case FM_ETH_MODE_2500BASE_X:
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_40GBASE_SR4:
        case FM_ETH_MODE_100GBASE_SR4:
            /* port state machine type */
            newPortSmType   = FM10000_BASIC_PORT_STATE_MACHINE;

            /* serdes state machine type */
            err = SetSerDesSmType( sw, port, &newSerdesSmType );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

            break;

        /* ethernet modes below are either not supported at all, or not
           supported for the time being or can be set only through
           auto-negotiation and therefore can't be set directly */
        case FM_ETH_MODE_1000BASE_KX:
        case FM_ETH_MODE_6GBASE_KR:
        case FM_ETH_MODE_6GBASE_CR:
        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_25GBASE_KR:
        case FM_ETH_MODE_25GBASE_CR:
        case FM_ETH_MODE_XAUI:
        case FM_ETH_MODE_10GBASE_KX4:
        case FM_ETH_MODE_10GBASE_CX4:
        case FM_ETH_MODE_24GBASE_KR4:
        case FM_ETH_MODE_24GBASE_CR4:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_XLAUI:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_KR4:

            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Unsupported ethernet mode %08x on port %d\n",
                             ethMode,
                             port );
            err =  FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

            break;

        default:

            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Invalid ethernet mode %08x on port %d\n",
                             ethMode,
                             port );
            err =  FM_ERR_INVALID_ETH_MODE;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

            break;

    }   /* end switch ( ethMode ) */


    /* Swap the port state machine type, if needed */
    err = SwapPortStateMachineType( sw, port, curPortSmType, newPortSmType );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );


    /**************************************************
     * For the SerDes State machine(s) the process
     * to change the new State Transition type is
     * a bit trickier, because the number of lanes 
     * associated to the port may change between the 
     * current and new ethMode. If there is a change 
     * we have to: 
     * 1) disable all serdes required by the new ethMode
     * 2) stop all serdes' state machine instances  
     * 3) restart all serdes' state machine instances 
     * with the State Transition Type required by the 
     * new ethMode 
     **************************************************/
    
    /* Proceed if the new serdes-level state transition table is now known */
    if ( newSerdesSmType != FM_SMTYPE_UNSPECIFIED )
    {
        /* do we need to change the state transition table? */
        if ( newSerdesSmType != curSerdesSmType )
        {
            /* yes, determine all lanes involved */
            baseFabricPort = portExt->fabricPort;
            numLanes = 1;
            if ( ethMode & FM_ETH_MODE_4_LANE_BIT_MASK )
            {
                baseFabricPort &= EPL_PORT_GROUP_MASK;
                numLanes = 4;
            }

            for (i = 0 ; i < numLanes ; i++)
            {
                /* determine the serDes ID(s) */
                err = fm10000MapFabricPortToSerdes( sw, 
                                                    baseFabricPort + i,
                                                    &serDes,
                                                    &ring );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                /* and remember all lane extension pointers */
                laneExt[i] = GET_LANE_EXT( sw, serDes );
            }

            /* is there a State Transition Table currently bound to the
               serdes state machine instance(s)? */
            if ( curSerdesSmType != FM_SMTYPE_UNSPECIFIED )
            {
                /* yes, then disable it first for ALL serdes/lanes
                   that will be linked to this port in the new
                   ethMode */
                for (i = 0 ; i < numLanes ; i++)
                {
                    /* notify the DISABLE event.. */
                    eventInfo.smType  = laneExt[i]->smType;
                    eventInfo.eventId = FM10000_SERDES_EVENT_DISABLE_REQ;
                    eventInfo.lock    = FM_GET_STATE_LOCK( sw );
                    eventInfo.dontSaveRecord = FALSE;
                    err = fmNotifyStateMachineEvent( laneExt[i]->smHandle,
                                                     &eventInfo,
                                                     &laneExt[i]->eventInfo,
                                                     &laneExt[i]->serDes );
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                    /* ...then stop the instance... */
                    err = fmStopStateMachine( laneExt[i]->smHandle );
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );
                }
            }

            /* ...then start it with the new type */
            for (i = 0 ; i < numLanes ; i++)
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Binding SerDes State Transition Table Type %d "
                                 "to SerDes %d's State Machine\n",
                                 newSerdesSmType,
                                 laneExt[i]->serDes );

                laneExt[i]->smType = FM_SMTYPE_UNSPECIFIED;
                err = fmStartStateMachine( laneExt[i]->smHandle,
                                           newSerdesSmType,
                                           FM10000_SERDES_STATE_DISABLED );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                /* finally restart it and reconfigure it */
                laneExt[i]->smType   = newSerdesSmType;

                /* start dfe state machines only if the basic serdes sm is being used */
                if (newSerdesSmType == FM10000_BASIC_SERDES_STATE_MACHINE)
                {
                    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                     port,
                                     "Binding SerDes-DFE State Transition Table Type %d "
                                     "to SerDes %d's DFE-State Machine\n",
                                     FM10000_BASIC_SERDES_DFE_STATE_MACHINE,
                                     laneExt[i]->serDes );
                    laneExt[i]->dfeExt.smType = FM_SMTYPE_UNSPECIFIED;

                    err = fmStartStateMachine(laneExt[i]->dfeExt.smHandle,
                                              FM10000_BASIC_SERDES_DFE_STATE_MACHINE,
                                              FM10000_SERDES_DFE_STATE_START);

                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

                    /* finally restart it and reconfigure it */
                    laneExt[i]->dfeExt.smType   = FM10000_BASIC_SERDES_DFE_STATE_MACHINE;
                }
            }
        }

    }   /* end if ( newSerdesSmType != FM_SMTYPE_UNSPECIFIED ) */


    /**************************************************
     * we have now made the changes, if needed, in the 
     * port-level and serdes-level state transition 
     * tables. Now go ahead with the ethMode change 
     * notification                                                 
     **************************************************/
    
    if ( newPortSmType != FM_SMTYPE_UNSPECIFIED )
    {
        /* notify the new configuration */
        eventInfo.smType = newPortSmType;
        if ( ethMode == FM_ETH_MODE_DISABLED )
        {
            eventInfo.eventId = FM10000_PORT_EVENT_DISABLE_REQ;
            portExt->eventInfo.info.config.ethMode = FM_ETH_MODE_DISABLED;
            portExt->eventInfo.info.config.speed   = 0;
        }
        else
        {
            eventInfo.eventId = FM10000_PORT_EVENT_CONFIG_REQ;
            portExt->eventInfo.info.config.ethMode = ethMode;
            portExt->eventInfo.info.config.speed   = speed;
        }
        eventInfo.lock = FM_GET_STATE_LOCK( sw );
        eventInfo.dontSaveRecord = FALSE;
        portExt->eventInfo.regLockTaken = FALSE;
        err = fmNotifyStateMachineEvent( portExt->smHandle,
                                         &eventInfo,
                                         &portExt->eventInfo,
                                         &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    }

    /* If autoneg was running, it was stopped above when the ethMode changed,
       by sending the appropriate event to the port state machine. Now we start
       autoneg if the ethmMode change made the Port State Machine ready 
       for it */
    err = fm10000AnRestartOnNewConfig( sw, 
                                       port,
                                       ethMode,
                                       portAttr->autoNegMode,
                                       portAttr->autoNegBasePage,
                                       portAttr->autoNegNextPages );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    /* if we have switched port-level state transition table, we need
       to restore the current admin mode */
    if ( newPortSmType != FM_SMTYPE_UNSPECIFIED &&
         newPortSmType != curPortSmType )
    {
        *restoreMode = TRUE;
    }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, err );

}   /* end fm10000ConfigureEthMode */




/*****************************************************************************/
/** fm10000DbgDumpSAFTable
 * \ingroup intPort
 *
 * \desc            This function prints out the current store-and-forward
 *                  table by logical port or physical port.
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
void fm10000DbgDumpSAFTable( fm_int sw )
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_int     srcPort;
    fm_int     dstPort;
    fm_int     physPort;
    fm_uint32  safMatrix[FM10000_SAF_MATRIX_WIDTH];
    fm_int     snfEnable;
    fm_int     cutThruMode;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_PRINT("Store-And-Forward Table\n");
    FM_LOG_PRINT("----------------------------------------------------\n");
    FM_LOG_PRINT("Legend: \n");
    FM_LOG_PRINT("All port indexes are physical port numbers as \n"
                 "indexed in frame control registers\n");
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Left Column: Ingress Port\n");
    FM_LOG_PRINT("Top Row:     Egress Port\n");
    FM_LOG_PRINT(" *:          SNF\n");
    FM_LOG_PRINT(" .:          cutThrough (per CutThruMode)\n");
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("   ");
    for ( physPort = FM10000_SAF_MATRIX_l_EnableSNF ;
          physPort <= FM10000_SAF_MATRIX_h_EnableSNF ;
          physPort++ )
    {
        FM_LOG_PRINT("%d", physPort/10);
    }
    FM_LOG_PRINT("\n   ");
    for ( physPort = FM10000_SAF_MATRIX_l_EnableSNF ;
          physPort <= FM10000_SAF_MATRIX_h_EnableSNF ;
          physPort++ )
    {
        FM_LOG_PRINT("%d", physPort % 10);
    }

    FM_LOG_PRINT("  CutThruMode");

    FM_LOG_PRINT("\n--+----------------------------------------------------------------\n");

    for ( srcPort = 0 ; srcPort <= switchPtr->maxPhysicalPort ; srcPort++ )
    {
        FM_LOG_PRINT("%02d|", srcPort);

        err = switchPtr->ReadUINT32Mult(sw,
                                        FM10000_SAF_MATRIX(srcPort, 0),
                                        FM10000_SAF_MATRIX_WIDTH,
                                        safMatrix);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_DEBUG, err);

        for ( dstPort = FM10000_SAF_MATRIX_l_EnableSNF ;
              dstPort <= FM10000_SAF_MATRIX_h_EnableSNF ;
              dstPort++ )
        {
            physPort = dstPort;

            snfEnable = fmMultiWordBitfieldGet32(safMatrix,
                                                 physPort,
                                                 physPort);

            FM_LOG_PRINT("%s", snfEnable ? "*" : ".");
        }

        cutThruMode = FM_ARRAY_GET_FIELD(safMatrix,
                                         FM10000_SAF_MATRIX,
                                         CutThruMode);

        switch (cutThruMode)
        {
            case 0:
                FM_LOG_PRINT("  Full cut-thru\n");
                break;
            case 1:
                FM_LOG_PRINT("  1 segment SnF\n");
                break;
            case 2:
                FM_LOG_PRINT("  2 segment SnF\n");
                break;
            case 3:
                FM_LOG_PRINT("  EOF SnF\n");
                break;
            default:
                FM_LOG_PRINT("\n");
        }
    }

ABORT:
    return;

}   /* end fm10000DbgDumpSAFTable */




/*****************************************************************************/
/** fm10000DbgRead100GBipErrRegs
 * \ingroup intSBus
 *
 * \desc            Read the content of the PCS_100GBASER_BIP register
 *                  specified by regSelector. There are 20 registers, so
 *                  regSelector, to select individual registers regSelector
 *                  should be in the range [0..19]. The special value 20 is
 *                  used to read and accumulate errors from all
 *                  PCS_100GBASER_BIP registers.
 *                  The result is returned in the caller allocated storage
 *                  pointed by pResult. This pointer may remain NULL to
 *                  only clear the specified registers.
 *                  The option clearReg allows clearing the register after
 *                  reading it.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       regSelector is the PCS_100GBASER_BIP register selector.
 *                  There are 20 registers [0,,19]. Set regSelector to 20
 *                  to read and accumulate the error from all BIP registers.
 * 
 * \param[in]       pResult points to caller-allocated storage where this
 *                  function should place the value of the selected register
 *                  of the accumulated value of all registers if regSelector
 *                  is equal to 20. It may be NULL.
 *
 * \param[in]       clearReg is a flag to clear the register after reading it.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgRead100GBipErrRegs(fm_int      sw,
                                       fm_int      port,
                                       fm_int      regSelector,
                                       fm_uint32  *pResult,
                                       fm_bool     clearReg)
{
    fm_status       err;
    fm_int          epl;
    fm_int          physPort;
    fm_int          channel;
    fm_bool         readAllRegs;
    fm_switch *     switchPtr;
    fm_uint32       addr;
    fm_uint32       value;


    switchPtr   = GET_SWITCH_PTR(sw);

    /* convert the logical port to physical port */
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /* Retrieve the EPL and channel numbers. */
    err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    readAllRegs = FALSE;

    if (regSelector > 20 || regSelector < 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else  if (regSelector == 20)
    {
        readAllRegs = TRUE;
        regSelector = 0;
    }

    if (err == FM_OK)
    {
        /* set result to 0 */
        if (pResult)
        {
            *pResult = 0;
        }

        TAKE_REG_LOCK(sw);
        while (regSelector < 20)
        {
            switch (regSelector)
            {
                case 0:
                    addr = FM10000_PCS_100GBASER_BIP_0(epl);
                    break;
                case 1:
                    addr = FM10000_PCS_100GBASER_BIP_1(epl);
                    break;
                case 2:
                    addr = FM10000_PCS_100GBASER_BIP_2(epl);
                    break;
                case 3:
                    addr = FM10000_PCS_100GBASER_BIP_3(epl);
                    break;
                case 4:
                    addr = FM10000_PCS_100GBASER_BIP_4(epl);
                    break;
                case 5:
                    addr = FM10000_PCS_100GBASER_BIP_5(epl);
                    break;
                case 6:
                    addr = FM10000_PCS_100GBASER_BIP_6(epl);
                    break;
                case 7:
                    addr = FM10000_PCS_100GBASER_BIP_7(epl);
                    break;
                case 8:
                    addr = FM10000_PCS_100GBASER_BIP_8(epl);
                    break;
                case 9:
                    addr = FM10000_PCS_100GBASER_BIP_9(epl);
                    break;
                case 10:
                    addr = FM10000_PCS_100GBASER_BIP_10(epl);
                    break;
                case 11:
                    addr = FM10000_PCS_100GBASER_BIP_11(epl);
                    break;
                case 12:
                    addr = FM10000_PCS_100GBASER_BIP_12(epl);
                    break;
                case 13:
                    addr = FM10000_PCS_100GBASER_BIP_13(epl);
                    break;
                case 14:
                    addr = FM10000_PCS_100GBASER_BIP_14(epl);
                    break;
                case 15:
                    addr = FM10000_PCS_100GBASER_BIP_15(epl);
                    break;
                case 16:
                    addr = FM10000_PCS_100GBASER_BIP_16(epl);
                    break;
                case 17:
                    addr = FM10000_PCS_100GBASER_BIP_17(epl);
                    break;
                case 18:
                    addr = FM10000_PCS_100GBASER_BIP_18(epl);
                    break;
                case 19:
                    addr = FM10000_PCS_100GBASER_BIP_19(epl);
                    break;
                default:
                    addr = 0;
                    err = FM_ERR_INVALID_ARGUMENT;

    
            }   /* end switch (regSelector) */

            if (pResult != NULL && err == FM_OK)
            {
                /* read register and add it to result */
                err = switchPtr->ReadUINT32( sw, addr, &value );
    
                if (err == FM_OK)
                {
                    *pResult += value;
                }
            }

            if (clearReg && err == FM_OK)
            {
                /* clear register */
                err == switchPtr->WriteUINT32(sw,addr,0);
            }


            /* inc selector if reading all registers or quit the loop otherwise */
            if (readAllRegs && err == FM_OK)
            {
                regSelector++;
            }
            else
            {
                break;
            }
        }   /* end while (regSelector < 20) */

        DROP_REG_LOCK(sw);
    }

ABORT:
    return err;

}   /* end fm10000DbgRead100GBipErrRegs */





/*****************************************************************************/
/** fm10000DbgReadFecUncorrectedErrReg
 * \ingroup intSBus
 *
 * \desc            Read the content of the PCS_100GBASER_BIP register
 *                  specified by regSelector. There are 20 registers, so
 *                  regSelector, to select individual registers regSelector
 *                  should be in the range [0..19]. The special value 20 is
 *                  used to read and accumulate errors from all
 *                  PCS_100GBASER_BIP registers.
 *                  The result is returned in the caller allocated storage
 *                  pointed by pResult. This pointer may remain NULL to
 *                  only clear the specified registers.
 *                  The option clearReg allows clearing the register after
 *                  reading it.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       pResult points to caller-allocated storage where this
 *                  function should place the value of the selected register
 *                  of the accumulated value of all registers if regSelector
 *                  is equal to 20. It may be NULL.
 *
 * \param[in]       clearReg is a flag to clear the register after reading it.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgReadFecUncorrectedErrReg(fm_int      sw,
                                             fm_int      port,
                                             fm_uint32  *pResult,
                                             fm_bool     clearReg)
{
    fm_status       err;
    fm_int          epl;
    fm_int          physPort;
    fm_int          channel;
    fm_switch *     switchPtr;
    fm_uint32       addr;


    switchPtr   = GET_SWITCH_PTR(sw);

    /* convert the logical port to physical port */
    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    /* Retrieve the EPL and channel numbers. */
    err = fm10000MapPhysicalPortToEplChannel(sw, physPort, &epl, &channel);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, err);

    if (err == FM_OK)
    {
        addr = FM10000_RS_FEC_UNCORRECTED(epl);

        TAKE_REG_LOCK(sw);
        if (pResult)
        {
            /* read register and add it to result */
            err = switchPtr->ReadUINT32( sw, addr, pResult );
        }

        if (clearReg && err == FM_OK)
        {
            /* clear register */
            err == switchPtr->WriteUINT32(sw,addr,0);
        }
        DROP_REG_LOCK(sw);
    }

ABORT:
    return err;

}   /* end fm10000DbgReadFecUncorrectedErrReg */



/*****************************************************************************/
/** fm10000GetMultiLaneCapabilities
 * \ingroup intPort
 *
 * \desc            Determine whether a given port is capable to function as
 *                  40G and/or 100G.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port to be checked for the multi-lane 
 *                  possibility.
 *
 * \param[out]      is40GCapable returns whether port is 40G capable. 
 *
 * \param[out]      is100GCapable returns whether port is 100G capable. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetMultiLaneCapabilities(fm_int   sw,
                                          fm_int   port,
                                          fm_bool *is40GCapable,
                                          fm_bool *is100GCapable)
{
    fm_status              status;
    fm_int                 epl;
    fm_int                 lane;
    fm_int                 masterEplLane;
    fm_int                 logPort;
    fm10000_portAttr *     portAttrExt;
    fm_int                 physSw;
    fm_int                 physPort;
    fm_uint32              capabilities;

    FM_LOG_ENTRY_V2( FM_LOG_CAT_PORT,
                     port,
                     "sw%d, port=%d is40GCapable=%p, is100GCapable=%p\n",
                     sw,
                     port,
                     (void *)is40GCapable,
                     (void *)is100GCapable);
    lane = -1;

    status = fmPlatformMapLogicalPortToPhysical(sw, port, &physSw, &physPort);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    status = fmPlatformGetPortCapabilities(sw, physPort, &capabilities);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    if ( (capabilities & FM_PORT_CAPABILITY_SPEED_40G) ||
         (capabilities & FM_PORT_CAPABILITY_SPEED_100G) )
    {
        /* Find the Master Lane (Port Lane 0) which drive the autoneg. This
         * is to cover the lane reversal or lane remapping case. */
        status = fm10000MapPhysicalPortToEplLane(sw,
                                                 physPort,
                                                 &epl,
                                                 &lane);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        masterEplLane = -1;
        status = fmPlatformMapPortLaneToEplLane(sw, port, 0, &masterEplLane);
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        if ( lane == masterEplLane )
        {
           /* Validate that all lanes are available */
            for ( lane = 0 ; lane < 4 ; lane++ )
            {
                if ( lane != masterEplLane )
                {
                    status = fm10000MapEplLaneToLogicalPort(sw,
                                                            epl,
                                                            lane,
                                                            &logPort);
                    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

                    portAttrExt = GET_FM10000_PORT_ATTR(sw, logPort);
                    if ( portAttrExt->ethMode != FM_ETH_MODE_DISABLED )
                    {
                        break;
                    }
                }
            }
        }
    }

    if (lane == 4)
    {
        /* MultiLane is Possible. Now find out multilane speed capabilities.*/
        if (capabilities & FM_PORT_CAPABILITY_SPEED_100G)
        {
            *is100GCapable = TRUE;
        }
        if (capabilities & FM_PORT_CAPABILITY_SPEED_40G)
        {
            *is40GCapable  = TRUE;
        }
    }
    else
    {
        *is100GCapable = FALSE;
        *is40GCapable  = FALSE;
    }

ABORT:
    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT, port, status);

}   /* end fm10000GetMultiLaneCapabilities */
