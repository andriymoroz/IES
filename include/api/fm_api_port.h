/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_port.h
 * Creation Date:   May 16, 2005
 * Description:     Contains functions dealing with the state of individual
 *                  ports
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

#ifndef __FM_FM_API_PORT_H
#define __FM_FM_API_PORT_H


/**************************************************
 * Flag value for the mac argument of
 * ''fmSetPortAttributeV2'' and 
 * ''fmGetPortAttributeV2''.
 **************************************************/

#define FM_PORT_ACTIVE_MAC                      -1
#define FM_PORT_MAC_ALL                         -2 /* Apply to all macs */
#define FM_PORT_LANE_NA                         -1
#define FM_PORT_LANE_ALL                        -2 /* Apply to all lanes */

/* Egress Timestamp destination masks for API property 
 * ''api.egressTimestampDest''. */

/* Egress Timestamp destination is Application. */
#define FM_EGRESS_TIMESTAMP_DEST_APP             (1U << 0)

/* Egress Timestamp destination is Mailbox. */
#define FM_EGRESS_TIMESTAMP_DEST_MBX             (1U << 1)

/****************************************************************************/
/** \ingroup constPortState
 *
 *  Port States, reported by ''fmGetPortState'' and ''fmGetPortStateV2''. 
 *  These states are "get-only", and cannot be specified in a call to 
 *  ''fmSetPortState'' or ''fmSetPortStateV2''.
 ****************************************************************************/
enum _fm_portState
{
    /* NOTE! Order is important.  See the table in fmSetPortState. */

    /** The port is in working order (symbol lock, alignment done) and
     *  ready to receive a frame. */
    FM_PORT_STATE_UP = 0,

    /** Deprecated. Use ''FM_PORT_MODE_ADMIN_DOWN'' instead. */
    FM_PORT_STATE_ADMIN_DOWN,

    /** Deprecated. Use ''FM_PORT_MODE_ADMIN_PWRDOWN'' instead. */
    FM_PORT_STATE_ADMIN_PWRDOWN,

    /** Deprecated. Use ''FM_PORT_MODE_BIST'' instead. */
    FM_PORT_STATE_BIST,

    /** The SERDES transmits remote fault constantly and the receiver is
     *  disabled. */
    FM_PORT_STATE_REMOTE_FAULT,

    /** Indicates the absence of any signal on any lane. */
    FM_PORT_STATE_DOWN,

    /** Some lanes have either signal or partial synchronization. */
    FM_PORT_STATE_PARTIALLY_UP,

    /** Indicates continuous reception of a local fault condition. A remote
     *  fault is automatically sent continuously. */
    FM_PORT_STATE_LOCAL_FAULT,

    /** Indicates that DFE tuning is still in progress. */
    FM_PORT_STATE_DFE_TUNING

};  /* end enum _fm_portState */


/****************************************************************************/
/** \ingroup typeEnum
 *
 *  Port Mode. Used as an argument to ''fmSetPortState'', ''fmSetPortStateV2'',
 *  ''fmGetPortState'' and ''fmGetPortStateV2'', as well as
 *  ''fmSetStackLogicalPortState''.
 ****************************************************************************/
typedef enum
{
    /* NOTE! Order is important.  See the table in fmSetPortState. */

    /** The port is in working order (symbol lock, alignment done) and
     *  ready to receive a frame. */
    FM_PORT_MODE_UP            = FM_PORT_STATE_UP,

    /** The port is administratively set down with the SERDES
     *  transmitting an idle pattern but the receiver disabled.
     *                                                                  \lb\lb
     *  Note: on the FM2000, an errata in the silicon dictates that the RX MAC
     *  must never be disabled, so on the FM2000, a port in this state will
     *  still receive ingressing frames. */
    FM_PORT_MODE_ADMIN_DOWN    = FM_PORT_STATE_ADMIN_DOWN,

    /** The port is administratively set down with the SERDES
     *  shut down and no signal transmitted.
     *                                                                  \lb\lb
     *  Note: on the FM10000, for a PEP port, FM_PORT_MODE_ADMIN_PWRDOWN is
     *  interpreted as FM_PORT_MODE_ADMIN_DOWN. */
    FM_PORT_MODE_ADMIN_PWRDOWN = FM_PORT_STATE_ADMIN_PWRDOWN,

    /** The receiver is expecting a Built In Self Test sequence. The subMode
     *  argument to ''fmSetPortState'' or ''fmSetPortStateV2'' is used to 
     *  specify the test pattern (see ''Port State Submodes''). */
    FM_PORT_MODE_BIST          = FM_PORT_STATE_BIST,

    /** The SERDES transmits remote fault constantly and the receiver is
     *  disabled. */
    FM_PORT_MODE_REMOTE_FAULT  = FM_PORT_STATE_REMOTE_FAULT,

    /** Indicates continuous reception of a local fault condition. A remote
     *  fault is automatically sent continuously. */
    FM_PORT_MODE_LOCAL_FAULT   = FM_PORT_STATE_LOCAL_FAULT

} fm_portMode;


/****************************************************************************/
/** \ingroup constPortSubmode
 *
 *  Port State Submodes for the ''FM_PORT_MODE_BIST'' (Built In Self Test) 
 *  port mode, used as arguments to ''fmSetPortState'', ''fmSetPortStateV2'', 
 *  ''fmGetPortState'' and ''fmGetPortStateV2''.
 ****************************************************************************/
enum _fm_portSubmode
{
    /* Only generate test patterns on the transmit side. */

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 9, 4 and 1,
     *  repeating every 511 cycles. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_BIST_TX_PRBS_512A = 0,

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 9, 5 and 1,
     *  repeating every 511 cycles. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_BIST_TX_PRBS_512B,

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 10, 3 and 1,
     *  repeating every 1023 cycles. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_BIST_TX_PRBS_1024,

    /** Generate K28.5 IDLE character for 8B/10B encoding. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_BIST_TX_IDLECHAR,

    /** Generate K28.7 low frequency test data, 0001111100b. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_BIST_TX_TESTCHAR,

    /** Generate K28.7 low frequency test data, 0001111100b. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_BIST_TX_LOWFREQ,

    /** Generate K28.7 high frequency test data, 0101010101b. 
     *
     *  \chips   FM3000, FM4000, FM6000, FM10000 */
    FM_BIST_TX_HIGHFREQ,

    /** Generate K28.5 mixed frequency test data,
     *  11111010110000010100b
     *  \chips  FM10000 */
    FM_BIST_TX_MIXEDFREQ,

    /** Generate CJPAT frames.
     *
     *  \chips  FM3000, FM4000 */
    FM_BIST_TX_CJPAT,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  7, 6 and 1, repeating every 127 cycles (ITU V.29).
     *
     *  \chips  FM6000 */
    FM_BIST_TX_PRBS_128,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  15, 14 and 1, repeating every 32K-1 cycles (ITU O.151).
     *
     *  \chips  FM6000 */
    FM_BIST_TX_PRBS_32K,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  23, 18 and 1, repeating every 8M-1 cycles (ITU O.151).
     *
     *  \chips  FM6000 */
    FM_BIST_TX_PRBS_8M,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  31, 28 and 1, repeating every 2G-1 cycles.
     *
     *  \chips  FM6000 */
    FM_BIST_TX_PRBS_2G,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  11, 9 and 1, repeating every 2047 cycles (IEEE Std
     *  802.3ap-2007).
     *
     *  \chips  FM6000 */
    FM_BIST_TX_PRBS_2048,

    /** Generate square-8 test pattern, , 1111111100000000b.
     *
     *  \chips  FM10000 */
    FM_BIST_TX_SQUARE8,

    /** Generate square-10 test pattern, , 11111111110000000000b.
     *
     *  \chips  FM10000 */
    FM_BIST_TX_SQUARE10,

    /** 10-bit custom test pattern. */
    FM_BIST_TX_CUSTOM10,

    /** 20-bit pattern as a 10-bit custom pattern followed by its inverse. */
    FM_BIST_TX_CUSTOM20,

    /** 40-bit custom test pattern.  */
    FM_BIST_TX_CUSTOM40,

    /** 80-bit custom test pattern.  */
    FM_BIST_TX_CUSTOM80,

    /* Only check test patterns on the receive side. */

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 9, 4 and 1,
     *  repeating every 511 cycles. */
    FM_BIST_RX_PRBS_512A,

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 9, 5 and 1,
     *  repeating every 511 cycles. 
     *
     *  \chips  FM6000 */
    FM_BIST_RX_PRBS_512B,

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 10, 3 and 1,
     *  repeating every 1023 cycles. */
    FM_BIST_RX_PRBS_1024,

    /** Generate K28.5 IDLE character for 8B/10B encoding.
     *  \chips  FM6000 */
    FM_BIST_RX_IDLECHAR,

    /** Generate K28.7 low frequency test data, 0001111100b.
     *  \chips  FM6000 */
    FM_BIST_RX_TESTCHAR,

    /** Generate K28.7 low frequency test data, 0001111100b.
     *  \chips  FM6000, FM10000 */
    FM_BIST_RX_LOWFREQ,

    /** Generate K28.7 high frequency test data, 0101010101b.
     *  \chips  FM6000, FM10000 */
    FM_BIST_RX_HIGHFREQ,

    /** Generate K28.5 mixed frequency test data,
     *  11111010110000010100b.
     *  \chips  FM10000 */
    FM_BIST_RX_MIXEDFREQ,

    /** Generate CJPAT frames. */
    FM_BIST_RX_CJPAT,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  7, 6 and 1, repeating every 127 cycles (ITU V.29).
     *
     *  \chips  FM6000 */
    FM_BIST_RX_PRBS_128,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  15, 14 and 1, repeating every 32K-1 cycles (ITU O.151).
     *
     *  \chips  FM6000 */
    FM_BIST_RX_PRBS_32K,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  23, 18 and 1, repeating every 8M-1 cycles (ITU O.151).
     *
     *  \chips  FM6000 */
    FM_BIST_RX_PRBS_8M,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  31, 28 and 1, repeating every 2G-1 cycles.
     *
     *  \chips  FM6000 */
    FM_BIST_RX_PRBS_2G,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  11, 9 and 1, repeating every 2047 cycles (IEEE Std
     *  802.3ap-2007).
     *
     *  \chips  FM6000 */
    FM_BIST_RX_PRBS_2048,

    /** Check square-8 test pattern, , 1111111100000000b.
     *
     *  \chips  FM10000 */
    FM_BIST_RX_SQUARE8,

    /** Check square-10 test pattern, , 11111111110000000000b.
     *
     *  \chips  FM10000 */
    FM_BIST_RX_SQUARE10,

    /** 10-bit custom test pattern. */
    FM_BIST_RX_CUSTOM10,

    /** 20-bit pattern as a 10-bit custom pattern followed by its inverse. */
    FM_BIST_RX_CUSTOM20,

    /** 40-bit custom test pattern. */
    FM_BIST_RX_CUSTOM40,

    /** 80-bit custom test pattern. */
    FM_BIST_RX_CUSTOM80,

    /* Both generate and check test patterns */

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 9, 4 and 1,
     *  repeating every 511 cycles. */
    FM_BIST_TXRX_PRBS_512A,

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 9, 5 and 1,
     *  repeating every 511 cycles. 
     *
     *  \chips  FM6000 */
    FM_BIST_TXRX_PRBS_512B,

    /** Pseudo-Random Binary Sequence with polynomial coefficients, 10, 3 and 1,
     *  repeating every 1023 cycles. */
    FM_BIST_TXRX_PRBS_1024,

    /** Generate K28.5 IDLE character for 8B/10B encoding.
     *  \chips  FM6000 */
    FM_BIST_TXRX_IDLECHAR,

    /** Generate K28.7 low frequency test data, 0001111100b.
     *  \chips  FM6000, FM10000 */
    FM_BIST_TXRX_TESTCHAR,

    /** Generate K28.7 low frequency test data, 0001111100b.
     *  \chips  FM6000, FM10000 */
    FM_BIST_TXRX_LOWFREQ,

    /** Generate K28.7 high frequency test data, 0101010101b.
     *  \chips  FM6000, FM10000 */
    FM_BIST_TXRX_HIGHFREQ,

    /** Generate K28.5 mixed frequency test data,
     *  11111010110000010100b
     *  \chips  FM10000 */
    FM_BIST_TXRX_MIXEDFREQ,

    /** Generate CJPAT frames. */
    FM_BIST_TXRX_CJPAT,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  7, 6 and 1, repeating every 127 cycles (ITU V.29).
     *
     *  \chips  FM6000 */
    FM_BIST_TXRX_PRBS_128,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  15, 14 and 1, repeating every 32K-1 cycles (ITU O.151).
     *
     *  \chips  FM6000 */
    FM_BIST_TXRX_PRBS_32K,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  23, 18 and 1, repeating every 8M-1 cycles (ITU O.151).
     *
     *  \chips  FM6000 */
    FM_BIST_TXRX_PRBS_8M,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  31, 28 and 1, repeating every 2G-1 cycles.
     *
     *  \chips  FM6000 */
    FM_BIST_TXRX_PRBS_2G,

    /** Pseudo-Random Binary Sequence with polynomial coefficients,
     *  11, 9 and 1, repeating every 2047 cycles (IEEE Std
     *  802.3ap-2007).
     *
     *  \chips  FM6000 */
    FM_BIST_TXRX_PRBS_2048,

    /** Generate and check square-8 test pattern, , 1111111100000000b.
     *
     *  \chips  FM10000 */
    FM_BIST_TXRX_SQUARE8,

    /** Generate and check square-10 test pattern, , 11111111110000000000b.
     *
     *  \chips  FM10000 */
    FM_BIST_TXRX_SQUARE10,

    /** 10-bit custom test pattern. */
    FM_BIST_TXRX_CUSTOM10,

    /** 20-bit pattern as a 10-bit custom pattern followed by its inverse. */
    FM_BIST_TXRX_CUSTOM20,

    /** 40-bit custom test pattern. */
    FM_BIST_TXRX_CUSTOM40,

    /** 80-bit custom test pattern. */
    FM_BIST_TXRX_CUSTOM80,

    /** UNPUBLISHED: For internal use only. */
    FM_BIST_MAX

};  /* end enum _fm_portSubmode */


/****************************************************************************/
/** \ingroup constLaneStatus
 *
 * Port lane status, as reported by ''fmGetPortState'' and ''fmGetPortStateV2''.
 ****************************************************************************/
enum _fm_laneStatus
{
    /** Lane is active and does not have symbol lock. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_LANE_NOT_LOCKED = 0,

    /** Lane is active and has symbol lock. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_LANE_LOCKED,

    /** Lane is not active. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_LANE_UNUSED

};  /* end enum _fm_laneStatus */


/****************************************************************************/
/** Port Capabilities
 *  \ingroup constPortCapabilities
 *  \page portCapabilities
 *
 *  The following set of bit masks may be ORed together to identify the
 *  capabilities of a physical port, as returned by the platform API service,
 *  ''fmPlatformGetPortCapabilities''.
 ****************************************************************************/
/** \ingroup constPortCapabilities
 * @{ */

/** Port can be a member of a Link Aggregation Group. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_LAG_CAPABLE  (1 << 0)

/** Traffic ingressing on this port can be routed. Note that the
 *  ''FM_PORT_ROUTABLE'' port attribute must be set. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_CAN_ROUTE    (1 << 1)

/** Port speed can be set to 10M bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_SPEED_10M    (1 << 2)

/** Port speed can be set to 100M bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_SPEED_100M   (1 << 3)

/** Port speed can be set to 1G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_SPEED_1G     (1 << 4)

/** Port speed can be set to 2.5G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_SPEED_2PT5G  (1 << 5)

/** Port speed can be set to 5G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000 */
#define FM_PORT_CAPABILITY_SPEED_5G     (1 << 6)

/** Port speed can be set to 10G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_PORT_CAPABILITY_SPEED_10G    (1 << 7)

/** Port speed can be set to 20G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM6000 */
#define FM_PORT_CAPABILITY_SPEED_20G    (1 << 8)

/** Port speed can be set to 40G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM6000, FM10000 */
#define FM_PORT_CAPABILITY_SPEED_40G    (1 << 9)

/** Port speed can be set to 25G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM10000 */
#define FM_PORT_CAPABILITY_SPEED_25G    (1 << 10)

/** Port speed can be set to 100G bits per second using the ''FM_PORT_SPEED''
 *  port attribute. 
 *
 *  \chips  FM10000 */
#define FM_PORT_CAPABILITY_SPEED_100G    (1 << 11)

/** @} (end of Doxygen group) */


/****************************************************************************/
/** Transceiver Signals 
 *  \ingroup constXcvrSignals
 *  \page xcveSignals
 *  
 *  The following definitions represent bitmasks that can be OR-ed
 *  together to notifiy the API of a change of one or more transceiver
 *  signals
 *  *************************************************************************/
/** \ingroup constXcvrSignals
 * @{ */

/** Transceiver module present or not
 *
 *  \chips  FM6000 */
#define FM_PORT_XCVRSIG_MODPRES    (1UL << 0) 

/** Transceiver Loss Of Signal state
 *
 *  \chips  FM6000 */
#define FM_PORT_XCVRSIG_RXLOS      (1UL << 1) 

/** Transmitter fault port attribute.
 *
 *  \chips  FM6000 */
#define FM_PORT_XCVRSIG_TXFAULT    (1UL << 2)

/** @} (end of Doxygen group) */


/****************************************************************************/
/** DFE Options 
 *  \ingroup constDfeOptions
 *  \page dfeOptions
 *  
 *  The following definitions represent bitmasks that can be OR-ed
 *  together to control some specific dfe options
 ****************************************************************************/
/** \ingroup constDfeOptions
 * @{ */

/** Stop performing DFE fine tuning, it is only applicable to
 *  ports that have already completed dfe tuning. This flag is
 *  automatically reset when DFE tuning is stopped. */
#define FM_PORT_PAUSE_DFE_FINE_TUNING    (1UL << 0) 

/** Stop reading port eye score, it is only applicable if the
 *  port has completed dfe tuning, this flag is automatically
 *  reset when DFE tuning is stopped. */
#define FM_PORT_STOP_READING_EYE_SCORE  (1UL << 1) 

/** @} (end of Doxygen group) */



/****************************************************************************/
/** \ingroup typeEnum
 *
 *  Indicates the type of port identifier used in an ''fm_portIdentifier'' 
 *  structure.
 ****************************************************************************/
typedef enum
{
    /** Port Number.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    FM_PORT_IDENTIFIER_PORT_NUMBER = 0,

    /** Port Mask.
     *  
     *  \chips  FM6000 */
    FM_PORT_IDENTIFIER_PORT_MASK

} fm_portIdentifierType;



/****************************************************************************/
/** \ingroup intTypeEnum
 *
 *  SerDes Operational Mode. Used by the api.FM10000.serdesOpMode API
 *  attribute to determine the kind of state machine and serdes register
 *  access to be used at serdes level. 
 ****************************************************************************/
typedef enum
{
    /** Serdes operational mode to be used on the real hardware. Accesses to
     *  the serdes are performed via LANE_SAI_XXX registers. This is the
     *  default serdes opMode. */
    FM_SERDES_OPMODE_CHIP_VIA_SAI,

    /** Serdes operational mode to be used on the real hardware. Access to
     *  the serdes is performed via SBus, which is slower than using
     *  LANE_SAI_XXX registers. */
    FM_SERDES_OPMODE_CHIP_VIA_SBUS, 

    /** Op mode to be used when Serdes are emulated by the Test Bench. */
    FM_SERDES_OPMODE_TEST_BENCH, 

    /** Serdes operational mode to be used with the test board. Only Serdes
     *  1..6 are available on the board, and they use the basic state
     *  machine. The other serdes use the stub state machine. Access to
     *  serdes registers is performed via SBus only. */
    FM_SERDES_OPMODE_TEST_BOARD,

    /** This mode makes abstraction of the serdes and uses the stub state
     *  machine for all ports. */
    FM_SERDES_OPMODE_STUB_SM

} fm_serDesOpMode;



/****************************************************************************/
/** Port Identifier
 * \ingroup typeStruct
 *
 *  Structure to identify a port or port mask. Used as an argument to
 *  ''fmSetMirrorDestinationExt'' and ''fmGetMirrorDestinationExt''.
 ****************************************************************************/
typedef struct _fm_portIdentifier
{
    /** Identifies which type of port descriptor is being used. */
    fm_portIdentifierType identifierType;

    /** Port Number */
    fm_int                port;

    /** Port Mask */
    fm_bitArray           portMask;

} fm_portIdentifier;



/****************************************************************************/
/** \ingroup intTypeEnum
 *
 *  PCIe Virtual Port Type. Used as an argument to ''fmGetPcieLogicalPort''
 *  and ''fmGetLogicalPortPcie'' functions.
 ****************************************************************************/
typedef enum
{
    /** This Virtual Port refer to the PF */
    FM_PCIE_PORT_PF = 0,

    /** This Virtual Port refer to a VF that make use of SR-IOV */
    FM_PCIE_PORT_VF,

    /** This Virtual Port refer to a VMDq instance */
    FM_PCIE_PORT_VMDQ,

    /** UNPUBLISHED: For internal use only. */
    FM_PCIE_PORT_MAX
    
} fm_pciePortType;




/* state change functions */
fm_status fmSetPortState(fm_int sw, fm_int port, fm_int mode, fm_int submode);
fm_status fmSetPortStateV2(fm_int sw, fm_int port, fm_int mac, fm_int mode, fm_int submode);
fm_status fmGetPortState(fm_int  sw,
                         fm_int  port,
                         fm_int *mode,
                         fm_int *state,
                         fm_int *info);
fm_status fmGetPortStateV2(fm_int  sw,
                           fm_int  port,
                           fm_int  mac,
                           fm_int *mode,
                           fm_int *state,
                           fm_int *info);
fm_status fmGetPortStateV3( fm_int   sw,
                            fm_int   port,
                            fm_int   mac,
                            fm_int   numBuffers,
                            fm_int  *numLanes,
                            fm_int  *mode,
                            fm_int  *state,
                            fm_int  *info );
fm_status fmNotifyXcvrChange( fm_int     sw, 
                              fm_int     port, 
                              fm_int     mac, 
                              fm_int     lane, 
                              fm_uint32  xcvrSignals, 
                              void      *xcvrInfo );

fm_status fmGetNumPortLanes( fm_int sw, 
                             fm_int port, 
                             fm_int mac, 
                             fm_int *numLanes );

fm_status fmIsPortDisabled( fm_int   sw, 
                            fm_int   port, 
                            fm_int   mac, 
                            fm_bool *isDisabled );

/* attribute change functions */
fm_status fmSetPortAttribute(fm_int sw, fm_int port, fm_int attr, void *value);
fm_status fmGetPortAttribute(fm_int sw, fm_int port, fm_int attr, void *value);
fm_status fmSetPortAttributeV2(fm_int sw,
                               fm_int port,
                               fm_int mac,
                               fm_int lane,
                               fm_int attr,
                               void * value);
fm_status fmGetPortAttributeV2(fm_int sw,
                               fm_int port,
                               fm_int mac,
                               fm_int lane,
                               fm_int attr,
                               void * value);
fm_status fmSetPortSecurity(fm_int  sw,
                            fm_int  port,
                            fm_bool enable,
                            fm_bool strict);

fm_status fmGetCpuPort(fm_int sw, fm_int *cpuPort);
fm_status fmSetCpuPort(fm_int sw, fm_int cpuPort);

fm_bool fmIsPerLagPortAttribute(fm_int sw, fm_uint attr);

fm_int fmGetISLTagSize(fm_islTagFormat islTagFormat);
fm_int fmGetPortISLTagSize(fm_int sw, fm_int port);

void fmDbgDumpPortAttributes(fm_int sw, fm_int port);

fm_status fmDbgDumpPortStateTransitionsV2( fm_int  sw,
                                           fm_int  *portList,
                                           fm_int  portCnt,
                                           fm_int  maxEntries,
                                           fm_text optionStr);
fm_status fmDbgDumpPortStateTransitions( fm_int sw, fm_int port );
fm_status fmDbgSetPortStateTransitionHistorySize( fm_int sw, 
                                                  fm_int port,
                                                  fm_int size );
fm_status fmDbgClearPortStateTransitions( fm_int sw, fm_int port );


fm_status fmMapCardinalPort(fm_int   sw,
                            fm_int   portIndex,
                            fm_int * logicalPort,
                            fm_int * physicalPort);

fm_status fmGetCardinalPortList(fm_int   sw,
                                fm_int * numPorts,
                                fm_int * portList,
                                fm_int   maxPorts);

fm_status fmIsPciePort( fm_int sw, fm_int port, fm_bool *isPciePort );

fm_status fmIsSpecialPort( fm_int sw, fm_int port, fm_bool *isSpecialPort );

fm_status fmGetPcieLogicalPort(fm_int sw,
                               fm_int pep,
                               fm_pciePortType type,
                               fm_int index,
                               fm_int *logicalPort);
fm_status fmGetLogicalPortPcie(fm_int sw,
                               fm_int logicalPort,
                               fm_int *pep,
                               fm_pciePortType *type,
                               fm_int *index);

fm_status fmDbgDumpPortEeeStatus(fm_int sw, fm_int port, fm_bool clear);
fm_status fmDbgEnablePortEee(fm_int sw, fm_int port, fm_int mode);

#endif /* __FM_FM_API_PORT_H */
