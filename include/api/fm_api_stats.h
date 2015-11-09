/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stats.h
 * Creation Date:   June 7, 2005
 * Description:     Structures and functions for dealing with counters
 *                  (statistics)
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

#ifndef __FM_FM_API_STATS_H
#define __FM_FM_API_STATS_H

/**************************************************/
/** Port Statistics Versions
 *  \ingroup constPortStatVersions
 *  \page portsStatVersions
 *
 *  These bit masks represent sets of port statistics,
 *  as reported in a ''fm_portCounters'' structure
 *  returned by ''fmGetPortCounters''. See 
 *  ''fm_portCounters'' for more details.
 **************************************************/
/** \ingroup constPortStatVersions
 * @{ */

/** Indicates that all FM2000 counters in ''fm_portCounters'' are valid. 
 *
 *  \chips  FM2000 */
#define FM2000_STATS_VERSION       (1 << 0)

/** Indicates that all IP statistics counters in ''fm_portCounters'' are valid. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_VALID_IP_STATS_VERSION  (1 << 1)

/** Indicates that all FM3000 and FM4000 counters in ''fm_portCounters'' 
 *  are valid. 
 *
 *  \chips  FM3000, FM4000 */
#define FM4000_STATS_VERSION       (1 << 2)

/** Indicates that all FM6000 counters in ''fm_portCounters'' are valid. 
 *
 *  \chips  FM6000 */
#define FM6000_STATS_VERSION       (1 << 3)

/** Indicates that all FM10000 counters in ''fm_portCounters''
 *  are valid.
 *
 *  \chips  FM10000 */
#define FM10000_STATS_VERSION      (1 << 4)

/** @} (end of Doxygen group) */


/*****************************************************************************/
/** \ingroup typeStruct typeStructUcode
 *  Per-port statistics returned by ''fmGetPortCounters''.
 *                                                                      \lb\lb
 *  Not all fields in an instance of this structure should always be
 *  considered valid. See the family table to see which counters
 *  are valid for a given chip-family. The family table is
 *  divided into groups. All groups that have a numbered index
 *  contain counters that are mutually exclusive (only one
 *  counter within the group will increment for a given packet).
 *  To keep backward compatibility, some counters are emulated
 *  in software by summing up counters, such counters go under
 *  the "software" group. List of counter groups:
 *                                                                      \lb\lb
 *  Group1:     Rx Type (Frames)                                            \lb
 *  Group2A:    Rx Size (Frames)                                            \lb
 *  Group2B:    Rx Size (Octets)                                            \lb
 *  Group3:     Rx Type (octets)                                            \lb
 *  Group4:     Rx Priority (Frames)                                        \lb
 *  Group5:     Rx Priority (Octets)                                        \lb
 *  Group6A:    Rx Forwarding (Frames)                                      \lb
 *  Group6B:    Rx Forwarding (Octets)                                      \lb
 *  Group7:     Tx Types (Frames)                                           \lb
 *  Group8A:    Tx Size (Frames)                                            \lb
 *  Group8B:    Tx Size (Octets)                                            \lb
 *  Group9:     Tx Types (Octets)                                           \lb
 *  Group10:    Tx Congestion Management (Frames)                           \lb
 *  Group16:    Tx Priority (Octets)                                        \lb
 *  GroupSoft:  Software Emulated Counters                                  \lb
 *  GroupOther: Counters that don't fit in other groups (not mutually 
 *              exclusive)                                                  \lb
 *  GroupEPL:   EPL Counters                                                \lb
 *                                                                          \lb
 *  For each counter, the group with which that counter is associated
 *  will be indicated for each chip that supports that counter. Where
 *  FM4000 is indicated, FM3000 also applies. Where FM6000 is indicated,
 *  FM5000 also applies.
 *                                                                      \lb\lb
 *  Fields only valid in certain versions will have a "Versions:
 *  X" comment, where "X" is the name of a version bit mask (see
 *  ''Port Statistics Versions'') that must be set in the
 *  cntVersion member of this structure for the field to be
 *  considered valid.
 *                                                                      \lb\lb
 *  Counters that are not valid will have a value of 0.
 *  
 *****************************************************************************/
typedef struct _fm_portCounters
{
    /** Identifies the port statistics structure version number. */
    fm_uint64 cntVersion;

    /*********************************************/
    /* Group 1 Counters - Rx Type (frames)       */
    /*********************************************/

    /** Number of valid packets received with unicast L2 DMACs.
     *  
     *  \counterGroups FM2000:Group1, FM4000:GroupSoft,
     *                 FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxUcstPkts;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid non-IP 
     *  packets received with unicast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxUcstPktsNonIP;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid IPv4 packets 
     *  received with unicast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxUcstPktsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid IPv6 packets 
     *  received with unicast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxUcstPktsIPv6;

    /** Number of valid packets received with broadcast L2 DMACs.
     *  
     *  \counterGroups FM2000:Group1, FM4000:GroupSoft,
     *                 FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxBcstPkts;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid non-IP 
     *  packets received with broadcast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxBcstPktsNonIP;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid IPv4 packets 
     *  received with broadcast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxBcstPktsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid IPv6 packets 
     *  received with broadcast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxBcstPktsIPv6;

    /** Number of valid packets received with multicast L2 DMACs.
     *  
     *  \counterGroups FM2000:Group1, FM4000:GroupSoft,
     *                 FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxMcstPkts;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid non-IP 
     *  packets received with multicast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxMcstPktsNonIP;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid IPv4 packets 
     *  received with multicast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxMcstPktsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid IPv6 packets 
     *  received with multicast L2 DMACS.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxMcstPktsIPv6;

    /** Number of received valid IEEE 802.3 PAUSE frames.
     *  
     *  \counterGroups FM2000:Group1, FM4000:Group1,
     *                 FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxPausePkts;

    /** Number of class-based pause packets received.
     *                                                                  \lb\lb
     *  Note: On FM6000 devices, FM_PORT_PARSE_PAUSE and
     *  FM_PORT_PARSE_CBP_PAUSE must be enabled for this counter to work.
     *  
     *  \counterGroups FM4000:Group1, FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxCBPausePkts;

    /** Number of received packets with CRC error, but proper size.
     *  
     *  \counterGroups FM2000:Group1, FM4000:Group1,
     *                 FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxFCSErrors;

    /** Number of received packets with symbol error, but proper size.
     *  
     *  \counterGroups FM2000:Group1, FM4000:Group1 */
    fm_uint64 cntRxSymbolErrors;

    /** Number of received packets that are undersized or oversized.
     *  
     *  \counterGroups FM4000:Group1 */
    fm_uint64 cntRxFrameSizeErrors;

    /** Number of received packets with any type of framing error
     *  (symbol, disparity, etc.).
     *  
     *  \counterGroups FM6000:Group1, FM10000:Group1 */
    fm_uint64 cntRxFramingErrorPkts;

    /*********************************************/
    /* Group 2 Counters - Rx Size (frames)       */
    /*********************************************/

    /** Number of received valid packets containing 63 or fewer octets
     *  because the minimum frame size is configured to be less than
     *  the Ethernet minimum.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A */
    fm_uint64 cntRxMinTo63Pkts;

    /** Number of received valid packets containing 64 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx64Pkts;

    /** Number of received valid packets containing 65 to 127 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx65to127Pkts;

    /** Number of received valid packets containing 128 to 255 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx128to255Pkts;

    /** Number of received valid packets containing 256 to 511 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx256to511Pkts;

    /** Number of received valid packets containing 512 to 1023 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx512to1023Pkts;

    /** Number of received valid packets containing 1024 to 1522 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx1024to1522Pkts;

    /** Number of received valid packets containing 1523 to 2047 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx1523to2047Pkts;

    /** Number of received valid packets containing 2048 to 4095 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx2048to4095Pkts;

    /** Number of received valid packets containing 4096 to 8191 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx4096to8191Pkts;

    /** Number of received valid packets containing 8192 to 10239 octets.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx8192to10239Pkts;

    /** Number of received valid packets containing 10240 or more octets
     *  because the maximum frame size is configured to be more than
     *  10240.
     *  
     *  \counterGroups FM2000:Group2A, FM4000:Group2A,
     *                 FM6000:Group2A, FM10000:Group2A  */
    fm_uint64 cntRx10240toMaxPkts;

    /*********************************************/
    /* Group 2 Counters - Rx Size (octets)       */
    /*********************************************/

    /** Number of received octets in valid packets containing 63 or
     *  fewer octets because the minimum frame size is configured to
     *  be less than the Ethernet minimum.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRxMinTo63octets;

    /** Number of received octets in valid packets containing 64
     *  octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx64octets;

    /** Number of received octets in valid packets containing 65 to
     *  127 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx65to127octets;

    /** Number of received octets in valid packets containing 128 to
     *  255 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx128to255octets;

    /** Number of received octets in valid packets containing 256 to
     *  511 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx256to511octets;

    /** Number of received octets in valid packets containing 512 to
     *  1023 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx512to1023octets;

    /** Number of received octets in valid packets containing 1024
     *  to 1522 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx1024to1522octets;

    /** Number of received octets in valid packets containing 1523
     *  to 2047 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx1523to2047octets;

    /** Number of received octets in valid packets containing 2048
     *  to 4095 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx2048to4095octets;

    /** Number of received octets in valid packets containing 4096
     *  to 8191 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx4096to8191octets;

    /** Number of received octets in valid packets containing 8192
     *  to 10239 octets.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx8192to10239octets;

    /** Number of received octets in valid packets containing 10240
     *  or more octets because the maximum frame size is configured
     *  to be more than 10240.
     *  
     *  \counterGroups FM10000:Group2B */
    fm_uint64 cntRx10240toMaxOctets;

    /*********************************************/
    /* Group 3 Counters - Rx Type (octets)       */
    /*********************************************/

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of received octets in 
     *  valid Non-IP packets.
     *  
     *  \counterGroups FM4000:Group3, FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxOctetsNonIp;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of received octets in 
     *  valid IPv4 packets.
     *  
     *  \counterGroups FM4000:Group3, FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxOctetsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of received octets in 
     *  valid IPv6 packets.
     *  
     *  \counterGroups FM4000:Group3, FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxOctetsIPv6;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  non-IP octets received with unicast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxUcstOctetsNonIP;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  IPv4 octets received with unicast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxUcstOctetsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  IPv6 octets received with unicast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxUcstOctetsIPv6;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  non-IP octets received with broadcast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxBcstOctetsNonIP;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  IPv4 octets received with broadcast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxBcstOctetsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  IPv6 octets received with broadcast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxBcstOctetsIPv6;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  non-IP octets received with multicast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxMcstOctetsNonIP;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  IPv4 octets received with multicast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxMcstOctetsIPv4;

    /** Versions: ''FM_VALID_IP_STATS_VERSION''. Number of valid
     *  IPv6 octets received with multicast L2 DMACS.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxMcstOctetsIPv6;

    /** Number of received valid IEEE 802.3 PAUSE octets.
     *  
     * \counterGroups FM6000:Group3, FM10000:Group3 */ 
    fm_uint64 cntRxPauseOctets;

    /** Number of class-based pause octets received.
     *                                                                  \lb\lb
     *  Note: FM_PORT_PARSE_PAUSE and FM_PORT_PARSE_CBP_PAUSE must be
     *  enabled for this counter to work.
     *  
     *  \counterGroups FM6000:Group3, FM10000:Group3 */
    fm_uint64 cntRxCBPauseOctets;

    /** Number of received octets with CRC error, but proper size.
     *  
     *  \counterGroups FM6000:Group3, FM10000:Group3 */
    fm_uint64 cntRxFCSErrorsOctets;

    /** Number of received octets with any type of framing error
     *  (symbol, disparity, etc.).
     *  
     *  \counterGroups FM6000:Group3, FM10000:Group3 */
    fm_uint64 cntRxFramingErrorOctets;

    /** Number of received octets in valid packets.
     *  
     *  \counterGroups FM2000:Group3, FM4000:GroupSoft,
     *                 FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntRxGoodOctets;

    /** Number of received octets in bad packets.
     *  
     *  \counterGroups FM2000:Group3, FM4000:Group3 */
    fm_uint64 cntRxBadOctets;

    /*********************************************/
    /* Group 4 Counters - Rx Priority (Frames)   */
    /*********************************************/

    /** An array, indexed by frame priority, the entry being the number of
     *  packets received at that priority.
     *                                                                  \lb\lb
     *  FM2000 and FM4000 devices support only the first 8
     *  priorities (0..7) while FM6000 and FM10000 devices support
     *  all 16 priorities (0..15).
     *  
     *  \counterGroups FM2000:Group4, FM4000:Group4,
     *                 FM6000:Group4, FM10000:Group4 */
    fm_uint64 cntRxPriorityPkts[16];

    /** For frames that are invalid on ingress or are invalidated by
     *  the switch because of a performance issue, the priority is
     *  counted as invalid as this value may not be derived from the
     *  frame (the priority value can be erroneous).
     *  
     *  \counterGroups FM6000:Group4 */
    fm_uint64 cntRxInvalidPriorityPkts;

    /*********************************************/
    /* Group 5 Counters - Rx Priority (Octets)   */
    /*********************************************/

    /** An array, indexed by frame priority, the entry being the number of
     *  octets received in frames at that priority.
     *                                                                  \lb\lb
     *  FM2000 and FM3000/FM4000 devices support only the first 8 priorities
     *  (0..7) while FM6000 devices support all 16 priorities (0..15).
     *  
     *  \counterGroups FM2000:Group5, FM4000:Group5,
     *                 FM6000:Group5, FM10000:Group5 */
    fm_uint64 cntRxPriorityOctets[16];

    /** For frames that are invalid on ingress or are invalidated by
     *  the switch because of a performance issue, the priority is
     *  counted as invalid as this value may not be derived from the
     *  frame (the priority value can be erroneous).
     *  
     *  \counterGroups FM6000:Group5 */
    fm_uint64 cntRxInvalidPriorityOctets;

    /*********************************************/
    /* Group 6 Counters - Rx Forwarding (frames) */
    /*********************************************/

    /** Number of frames that were forwarded normally, either unicast or
     *  multicast, as a result of a lookup of a valid entry in the MAC
     *  address table, or a broadcast. Note: This counter does not count
     *  mirrored frames.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntFIDForwardedPkts;

    /** Number of valid frames that were flooded either because it was a
     *  unicast packet with an unknown destination or an unregistered
     *  multicast packet.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntFloodForwardedPkts;

    /** Number of valid frames that were switched based on the
     *  destination glort.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntGlortSwitchedPkts;

    /** Number of valid frames that were routed based on the
     *  destination glort.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntGlortRoutedPkts;

    /** Number of frames processed with FTYPE==0x2 (Special
     *  Delivery). All standard filtering, forwarding, and lookup
     *  rules are bypassed in this case.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntSpeciallyHandledPkts;

    /** Number of frames dropped due to header parse errors.
     *  
     *  \counterGroups FM4000:Group6A, FM6000:Group6A,FM10000:Group6A */
    fm_uint64 cntParseErrDropPkts;

    /** Number of frames dropped due to memory parity errors
     *  encountered in the Frame Processing pipeline.
     *  
     *  \counterGroups FM4000:Group6A, FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntParityErrorPkts;

    /** Number of frames trapped to the CPU for any reason not
     *  covered by other counters in this group (e.g. Security
     *  violations). These include frames with reserved IEEE
     *  multicast addresses (BPDU, 802.1X, LACP, etc.), trapped IP
     *  frames (e.g. ICMP), and programmably trapped frames (due to
     *  the FFU, triggers, etc.).
     *  
     *  \counterGroups FM4000:Group6A, FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntTrappedPkts;

    /** Number of MAC Control frames dropped (either standard IEEE
     *  pause frames or class-based pause frames).
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntPauseDropPkts;

    /** Number of frames that were dropped on ingress because either the
     *  ingress or egress port was not in the forwarding spanning tree state.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:GroupSoft, FM10000:Group6A */
    fm_uint64 cntSTPDropPkts;

    /** Number of frames that were dropped on ingress because the
     *  ingress port was not in the forwarding spanning tree state.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntSTPIngressDropsPkts;

    /** Number of frames that were dropped on egress because the
     *  egress port was not in the forwarding spanning tree state.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntSTPEgressDropsPkts;

    /** Number of frames that were trapped to the CPU and not
     *  forwarded normally, as a result of any of the three specific
     *  trap functions:
     *                                                                  \lb\lb
     *  (1) Destination address = IEEE reserved group address
     *                                                                  \lb\lb
     *  (2) Destination address = CPU MAC address
     *                                                                  \lb\lb
     *  (3) Ether-type = Ether-type trap.
     *  
     *  \counterGroups FM2000:Group6A */
    fm_uint64 cntReservedTrapPkts;

    /** Number of frames that were dropped or trapped because they were
     *  considered a security violation.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntSecurityViolationPkts;

    /** Number of frames dropped because the frame was untagged and
     *  the switch is configured to drop untagged frames, or the frame
     *  was tagged and the switch is configured to drop tagged frames.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntVLANTagDropPkts;

    /** Number of frames dropped for an ingress VLAN boundary violation.
     *  Note: This only applies to 802.1Q; in a port-based VLAN there is no
     *  such thing as an ingress violation.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntVLANIngressBVPkts;

    /** Number of frames dropped for an egress VLAN boundary violation.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:Group6A,
     *                 FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntVLANEgressBVPkts;

    /** Number of frames dropped because of loopback suppression.
     *  
     *  \counterGroups FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntLoopbackDropsPkts;

    /** Number of frames dropped due to not finding a matching entry
     *  in the GLORT_CAM.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntGlortMissDropPkts;

    /** Number of frames dropped due to an FFU action.
     *  
     *  \counterGroups FM4000:Group6A, FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntFFUDropPkts;

    /** Number of frames dropped because the frame was either
     *  invalid when parsed or invalid because the frame processing
     *  pipeline was overloaded and marked this frame as invalid.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntInvalidDropPkts;

    /** Number of frames dropped due to policer rate limitation.
     *  
     *  \counterGroups FM4000:Group6A, FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntPolicerDropPkts;

    /** Number of IP frames dropped due to TTL less than or equal to
     *  1.
     *  
     *  \counterGroups FM4000:Group6A, FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntTTLDropPkts;

    /** Number of frames dropped due to the global usage exceeding
     *  the global watermark.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntGlobalWMDropPkts;

    /** Number of frames dropped due to the RX memory partition
     *  usage exceeding the RX memory partition watermark.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntRXMPDropPkts;

    /** Number of frames dropped due to the the RX memory
     *  partition's hog watermark exceeded on this port.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntRxHogDropPkts;

    /** Number of frames dropped due to the TX memory partition's
     *  hog watermarks exceeded on all ports that the frame was
     *  forwarded on.
     *  
     *  \counterGroups FM6000:Group6A */
    fm_uint64 cntTxHogDropPkts;

    /** Number of frames not counted by any other group 6 statistic.
     *  
     *  \counterGroups FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntOtherPkts;

    /** Number of frames dropped because of flood control rules.
     *  
     *  \counterGroups FM6000:Group6A, FM10000:Group6A */
    fm_uint64 cntFloodControlDropPkts;

    /** (Congestion Management) Number of frames dropped due to the
     *  global watermark (CM_GLOBAL_WM) being exceeded.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntCmPrivDropPkts;

    /** (Congestion Management) Number of frames dropped due to
     *  insufficient memory in shared partition 0.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntSmp0DropPkts;

    /** (Congestion Management) Number of frames dropped due to
     *  insufficient memory in shared partition 1.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntSmp1DropPkts;

    /** (Congestion Management) Number of frames dropped due to the
     *  SMP 0 RX hog watermark.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntRxHog0DropPkts;

    /** (Congestion Management) Number of frames dropped due to the
     *  SMP 1 RX hog watermark.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntRxHog1DropPkts;

    /** (Congestion Management) Number of frames dropped to all
     *  egress ports due to the SMP 0 TX hog watermark.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntTxHog0DropPkts;

    /** (Congestion Management) Number of frames dropped to all
     *  egress ports due to the SMP 1 TX hog watermark.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntTxHog1DropPkts;

    /** (Congestion Management) Number of frames dropped due to
     *  ingress rate limiting on SMP 0.
     *  
     *  \counterGroups FM4000:Group6A */
    fm_uint64 cntRateLimit0DropPkts;

    /** (Congestion Management) Number of frames dropped due to
     *  ingress rate limiting on SMP 1.
     *  
     *  \counterGroups FM4000:Group6A */
    fm_uint64 cntRateLimit1DropPkts;

    /** (Congestion Management) Number of frames dropped due to
     *  illegal membership.
     *  
     *  \counterGroups FM4000:Group6A */
    fm_uint64 cntBadSmpDropPkts;

    /** Number of frames dropped or redirected due to a
     *  user-defined trigger.
     *  
     *  \counterGroups FM2000:Group6A, FM4000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntTriggerDropRedirPkts;

    /** Number of frames dropped due to a trigger drop action.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntTriggerDropPkts;

    /** Number of frames redirected due to a trigger action.
     *  
     *  \counterGroups FM4000:Group6A, FM10000:Group6A */
    fm_uint64 cntTriggerRedirPkts;

    /** Number of frames forwarded using a DGLORT coming from FFU or ARP.
     *  
     *  \counterGroups FM10000:Group6A */
    fm_uint64 cntGlortForwardedPkts;

    /** Number of valid frames that were mirrored. Note: This
     *  counter is only incremented if flooding is enabled in the
     *  switch.
     *  
     *  \counterGroups FM2000:Group6A */
    fm_uint64 cntTriggerMirroredPkts;

    /** Number of frames with a broadcast destination address
     *  dropped because ''FM_BCAST_FLOODING'' switch attribute 
     *  is set to FM_BCAST_DISCARD. 
     *  
     *  \counterGroups FM2000:Group6A */
    fm_uint64 cntBroadcastDropPkts;

    /** Number of unicast and multicast frames dropped due to a
     *  destination lookup failure when flooding is disabled.
     *  
     *  \counterGroups FM2000:Group6A */
    fm_uint64 cntDLFDropPkts;

    /** Number of received packets dropped for exceeding the RX
     *  shared watermark.
     *  
     *  \counterGroups FM2000:Group6A */
    fm_uint64 cntRxCMDropPkts;

    /*********************************************/
    /* Group 6 Counters - Rx Forwarding (octets) */
    /*********************************************/

    /** Number of octets in frames that were forwarded normally,
     *  either unicast or multicast, as a result of a lookup of a
     *  valid entry in the MAC address table, or a broadcast. Note:
     *  This counter does not count mirrored frames.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntFIDForwardedOctets;

    /** Number of octets in valid frames that were flooded either
     *  because it was a unicast packet with an unknown destination
     *  or an unregistered multicast packet.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntFloodForwardedOctets;

    /** Number of octets in frames processed with FTYPE==0x2
     *  (Special Delivery). All standard filtering, forwarding, and
     *  lookup rules are bypassed in this case.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntSpeciallyHandledOctets;

    /** Number of octets in frames dropped due to header parse
     *  errors.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntParseErrDropOctets;

    /** Number of octets in frames dropped due to memory parity
     *  errors encountered in the Frame Processing pipeline.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntParityErrorOctets;

    /** Number of octets in frames trapped to the CPU for any reason
     *  not covered by other counters in this group (e.g. Security
     *  violations). These include frames with reserved IEEE
     *  multicast addresses (BPDU, 802.1X, LACP, etc.), trapped IP
     *  frames (e.g. ICMP), and programmably trapped frames (due to
     *  the FFU, triggers, etc.).
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntTrappedOctets;

    /** Number octets of MAC Control frames dropped (either standard
     *  IEEE pause frames or class-based pause frames).
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntPauseDropOctets;

    /** Number of octets in frames that were dropped on ingress
     *  because either the ingress or egress port was not in the
     *  forwarding spanning tree state.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntSTPDropOctets;

    /** Number of octets in frames that were dropped or trapped
     *  because they were considered a security violation.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntSecurityViolationOctets;

    /** Number of octets in frames dropped because the frame was
     *  untagged and the switch is configured to drop untagged
     *  frames, or the frame was tagged and the switch is configured
     *  to drop tagged frames.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntVLANTagDropOctets;

    /** Number of octets in frames dropped for an ingress VLAN
     *  boundary violation. Note: This only applies to 802.1Q; in a
     *  port-based VLAN there is no such thing as an ingress
     *  violation.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntVLANIngressBVOctets;

    /** Number of octets in frames dropped for an egress VLAN
     *  boundary violation.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntVLANEgressBVOctets;

    /** Number of octets in frames that were loopback suppressed.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntLoopbackDropOctets;

    /** Number of octets in frames dropped due to not finding a
     *  matching entry in the GLORT_CAM.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntGlortMissDropOctets;

    /** Number of octets in frames dropped due to an FFU action.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntFFUDropOctets;

    /** Number of octets in frames dropped due to policer rate
     *  limitation.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntPolicerDropOctets;

    /** Number of octets in IP frames dropped due to TTL less than
     *  or equal to 1.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntTTLDropOctets;

    /** Number of octets in frames not counted by any other group 6
     *  statistic.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntOtherOctets;

    /** Number of octets in frames dropped because of flood control
     *  rules.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntFloodControlDropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  due to the global watermark (CM_GLOBAL_WM) being exceeded.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntCmPrivDropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  due to insufficient memory in shared partition 0.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntSmp0DropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  due to insufficient memory in shared partition 1.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntSmp1DropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  due to the SMP 0 RX hog watermark.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntRxHog0DropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  due to the SMP 1 RX hog watermark.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntRxHog1DropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  to all egress ports due to the SMP 0 TX hog watermark.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntTxHog0DropOctets;

    /** (Congestion Management) Number of octets in frames dropped
     *  to all egress ports due to the SMP 1 TX hog watermark.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntTxHog1DropOctets;

    /** Number of octets in frames dropped due to a trigger drop
     *  action.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntTriggerDropOctets;

    /** Number of octets in frames redirected due to a trigger
     *  action.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntTriggerRedirOctets;

    /** Number of octets in frames forwarded using a DGLORT coming
     *  from FFU or ARP.
     *  
     *  \counterGroups FM10000:Group6B */
    fm_uint64 cntGlortForwardedOctets;

    /*********************************************/
    /* Group 7 Counters - Tx Types (Frames)      */
    /*********************************************/
    
    /** Number of transmitted valid unicast packets.
     *  
     *  \counterGroups FM2000:Group7, FM4000:Group7,
     *                 FM6000:GroupSoft, FM10000:Group7 */
    fm_uint64 cntTxUcstPkts;

    /** Number of transmitted valid broadcast packets.
     *  
     *  \counterGroups FM2000:Group7, FM4000:Group7,
     *                 FM6000:GroupSoft, FM10000:Group7 */
    fm_uint64 cntTxBcstPkts;

    /** Number of transmitted valid multicast packets.
     *  
     *  \counterGroups FM2000:Group7, FM4000:Group7,
     *                 FM6000:GroupSoft, FM10000:Group7 */
    fm_uint64 cntTxMcstPkts;

    /** Number of transmitted valid non-IP unicast packets .
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxUcstPktsNonIP;

    /** Number of transmitted valid broadcast packets.
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxBcstPktsNonIP;

    /** Number of transmitted valid non-IP multicast packets.
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxMcstPktsNonIP;

    /** Number of transmitted valid IP unicast packets .
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxUcstPktsIP;

    /** Number of transmitted valid IP broadcast packets.
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxBcstPktsIP;

    /** Number of transmitted valid IP multicast packets.
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxMcstPktsIP;

    /** Number of transmitted valid PAUSE frames. For FM2000, FM3000 and
     *  FM4000, this counter is 32 bits. For FM10000, this
     *  counter is for IEEE 802.3 PAUSE frames only.
     *  
     *  \counterGroups FM2000:Group7, FM4000:GroupEPL,
     *                 FM6000:Group7, FM10000:Group7 */
    fm_uint64 cntTxPausePkts;

    /** Number of transmitted valid class-based PAUSE frames.
     *  
     *  \counterGroups FM10000:Group7 */
    fm_uint64 cntTxCBPausePkts;

    /** Number of frames dropped due to an ingress FCS error that
     *  was dropped by the scheduler prior to transmission start.
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxFCSErrDropPkts;

    /** Number of transmitted packets with any type of framing error
     *  (symbol, disparity, etc.).
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxFramingErrorPkts;

    /** Number of transmitted packets that were marked on ingress as
     *  erroneous (either due to an CRC or symbol error) which the switch
     *  element did not manage to discard (due to cut-through). 
     *  
     * \counterGroups FM10000:Group7 */
    fm_uint64 cntTxErrorSentPkts;

    /** Number of transmitted packets that were marked on ingress as
     *  erroneous (either due to an CRC or symbol error, or due to
     *  under/over size problems) which the switch element actually
     *  managed to discard. Frames marked as erroneous on ingress
     *  which were transmitted (due to cut-through) will not be
     *  included in this counter.
     *  
     * \counterGroups FM2000:Group7, FM4000:Group7, FM10000:Group7 */
    fm_uint64 cntTxErrorDropPkts;

    /** Number of frames dropped due to the frame timeout mechanism
     *  
     *  \counterGroups FM2000:Group7, FM4000:Group7,
     *                 FM6000:Group7, FM10000:Group7 */
    fm_uint64 cntTxTimeOutPkts;

    /** Number of frames that were discarded due to no memory in TX.
     *  
     *  \counterGroups FM6000:Group7 */
    fm_uint64 cntTxOutOfMemErrPkts;

    /** Number of frames that were discarded due to unrepairable ECC
     *  erors in TX.
     *  
     *  \counterGroups FM6000:Group7, FM10000:Group7 */
    fm_uint64 cntTxUnrepairEccPkts;

    /** Number of frames that were discarded due to loopback
     *  suppression in MODIFY (in TX).
     *  
     *  \counterGroups FM4000:Group7, FM6000:Group7, FM10000:Group7 */
    fm_uint64 cntTxLoopbackPkts;

    /** Number of frames that were supposed to be routed but had TTL
     *  <= 1.
     *  
     *  \counterGroups FM10000:Group7 */
    fm_uint64 cntTxTTLDropPkts;

    /*********************************************/
    /* Group 8 Counters - Tx Size (frames)       */
    /*********************************************/

    /** Number of transmitted valid packets containing 63 or fewer octets
     *  because the minimum frame size is configured to be less than
     *  the Ethernet minimum. This counter also includes errored frames
     *  that were transmitted anyway because MAC_CFG_2[Min Frame Discard]
     *  (see FM2000 datasheet) was not set.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTxMinTo63Pkts;

    /** Number of transmitted valid packets containing 64 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx64Pkts;

    /** Number of transmitted valid packets containing 65 to 127 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx65to127Pkts;

    /** Number of transmitted valid packets containing 128 to 255 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx128to255Pkts;

    /** Number of transmitted valid packets containing 256 to 511 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx256to511Pkts;

    /** Number of transmitted valid packets containing 512 to 1023 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx512to1023Pkts;

    /** Number of transmitted valid packets containing 1024 to 1522 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx1024to1522Pkts;

    /** Number of transmitted valid packets containing 1523 to 2047 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx1523to2047Pkts;

    /** Number of transmitted valid packets containing 2048 to 4095 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx2048to4095Pkts;

    /** Number of transmitted valid packets containing 4096 to 8191 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx4096to8191Pkts;

    /** Number of transmitted valid packets containing 8192 to 10239 octets.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx8192to10239Pkts;

    /** Number of transmitted valid packets containing 10240 or more octets
     *  because the maximum frame size is configured to be more than
     *  10240.
     *  
     *  \counterGroups FM2000:Group8A, FM4000:Group8A,
     *                 FM6000:Group8A, FM10000:Group8A */
    fm_uint64 cntTx10240toMaxPkts;

    /*********************************************/
    /* Group 8 Counters - Tx Size (octets)       */
    /*********************************************/
    /** Number of transmitted octets in valid packets containing 63
     *  or fewer octets because the minimum frame size is configured
     *  to be less than the Ethernet minimum. This counter also
     *  includes errored frames that were transmitted anyway because
     *  MAC_CFG_2[Min Frame Discard] (see datasheet) was not
     *  set.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTxMinTo63octets;

    /** Number of octets in transmitted valid packets containing 64
     *  octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx64octets;

    /** Number of octets in transmitted valid packets containing 65
     *  to 127 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx65to127octets;

    /** Number of octets in transmitted valid packets containing 128
     *  to 255 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx128to255octets;

    /** Number of octets in transmitted valid packets containing 256
     *  to 511 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx256to511octets;

    /** Number of octets in transmitted valid packets containing 512
     *  to 1023 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx512to1023octets;

    /** Number of octets in transmitted valid packets containing
     *  1024 to 1522 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx1024to1522octets;

    /** Number of octets in transmitted valid packets containing
     *  1523 to 2047 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx1523to2047octets;

    /** Number of octets in transmitted valid packets containing
     *  2048 to 4095 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx2048to4095octets;

    /** Number of octets in transmitted valid packets containing
     *  4096 to 8191 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx4096to8191octets;

    /** Number of octets in transmitted valid packets containing
     *  8192 to 10239 octets.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx8192to10239octets;

    /** Number of transmitted octets in valid packets containing
     *  10240 or more octets because the maximum frame size is
     *  configured to be more than 10240.
     *  
     *  \counterGroups FM10000:Group8B */
    fm_uint64 cntTx10240toMaxOctets;

    /*********************************************/
    /* Group 9 Counters - Tx Types (Octets)      */
    /*********************************************/

    /** Number of transmitted valid non-IP unicast packets .
     *  
     * \counterGroups FM6000:Group9 */ 
    fm_uint64 cntTxUcstOctetsNonIP;

    /** Number of transmitted valid non-IP broadcast packets. 
     *  
     * \counterGroups FM6000:Group9 */ 
    fm_uint64 cntTxBcstOctetsNonIP;

    /** Number of transmitted valid non-IP multicast packets. 
     *  
     * \counterGroups FM6000:Group9 */ 
    fm_uint64 cntTxMcstOctetsNonIP;

    /** Number of transmitted valid IPv4 unicast packets . 
     *  
     * \counterGroups FM6000:Group9 */ 
    fm_uint64 cntTxUcstOctetsIP;

    /** Number of transmitted valid IPv4 broadcast packets. 
     *  
     * \counterGroups FM6000:Group9 */ 
    fm_uint64 cntTxBcstOctetsIP;

    /** Number of transmitted valid IPv4 multicast packets. 
     *  
     * \counterGroups FM6000:Group9 */ 
    fm_uint64 cntTxMcstOctetsIP;

    /** Number of transmitted octets in valid unicast packets . 
     *  
     * \counterGroups FM10000:Group9 */ 
    fm_uint64 cntTxUcstOctets;

    /** Number of transmitted octets in valid multicast packets. 
     *  
     * \counterGroups FM10000:Group9 */ 
    fm_uint64 cntTxMcstOctets;

    /** Number of transmitted octets in valid broadcast packets. 
     *  
     * \counterGroups FM10000:Group9 */ 
    fm_uint64 cntTxBcstOctets;

    /** Number of octets dropped due to an ingress FCS error that
     *  was dropped by the scheduler prior to transmission start.
     *  
     *  \counterGroups FM6000:Group9 */
    fm_uint64 cntTxFCSErrDropOctets;

    /** Number of transmitted octets, including CRCs but excluding preambles
     *  and inter-frame characters.
     *  
     *  \counterGroups FM2000:Group9, FM4000:Group9,
     *                 FM6000:GroupSoft, FM10000:GroupSoft */
    fm_uint64 cntTxOctets;

    /** Number of transmitted octets in packets that were marked
     *  on ingress as erroneous (either due to an CRC or symbol error,
     *  or due to under/over size problems) which the switch element actually
     *  managed to discard. Frames marked as erroneous on ingress
     *  which were transmitted (due to cut-through) will not be
     *  included in this counter.
     *  
     *  For FM4000, this counter will also include octets in frames dropped
     *  due to frame timeout. 
     *  
     *  \counterGroups FM4000:Group9, FM10000:Group9 */
    fm_uint64 cntTxErrorOctets;

    /** Number of transmitted octets with any type of framing error
     *  (symbol, disparity, etc.).
     *  
     *  \counterGroups FM6000:Group9 */
    fm_uint64 cntTxFramingErrorOctets;

    /** Number of transmitted octets in valid PAUSE frames.
     *  
     *  \counterGroups FM6000:Group9, FM10000:Group9 */
    fm_uint64 cntTxPauseOctets;

    /** Number of transmitted octets in valid class-based PAUSE
     *  frames.
     *  
     *  \counterGroups FM10000:Group9 */
    fm_uint64 cntTxCBPauseOctets;

    /** Number of transmitted octets with CRC error, but proper size.
     *  
     *  \counterGroups FM6000:Group9 */
    fm_uint64 cntTxFCSErroredOctets;

    /** Number of transmitted octets in packets that were marked on ingress as
     *  erroneous (either due to an CRC or symbol error) which the switch
     *  element did not manage to discard (due to cut-through). 
     *  
     * \counterGroups FM10000:Group9 */
    fm_uint64 cntTxErrorSentOctets;

    /** Number of octets dropped due to the frame timeout mechanism.
     *  
     *  \counterGroups FM6000:Group9, FM10000:Group9 */
    fm_uint64 cntTxTimeOutOctets;

    /** Number of octets that were discarded due to no memory in TX.
     *  
     *  \counterGroups FM6000:Group9 */
    fm_uint64 cntTxOutOfMemErrOctets;

    /** Number of octets that were discarded due to unrepairable ECC
     *  erors in TX.
     *  
     *  \counterGroups FM6000:Group9, FM10000:Group9 */
    fm_uint64 cntTxUnrepairEccOctets;

    /** Number of octets that were discarded due to loopback
     *  suppression in MODIFY (in TX).
     *  
     *  \counterGroups FM6000:Group9, FM10000:Group9 */
    fm_uint64 cntTxLoopbackOctets;

    /** Number of octets in frames that were supposed to be routed
     *  but had TTL <= 1.
     *  
     *  \counterGroups FM10000:Group9 */
    fm_uint64 cntTxTTLDropOctets;

    /*********************************************/
    /* Group 16 Counters - Tx Priority (Octets)  */
    /*********************************************/

    /** An array, indexed by priority (0..15), each entry being the
     *  number of octets transmitted in frames at that priority.
     *  
     *  \counterGroups FM6000:Group16 */
    fm_uint64 cntTxPriorityOctets[16];

    /*********************************************/
    /* EPL Counters                              */
    /*********************************************/

    /** Number of frames that were terminated early or dropped due to
     *  underflow during transmission. For FM6000, this counter is 32 bits.
     *  
     *  \counterGroups FM4000:GroupEPL, FM6000:GroupEPL, FM10000:GroupEPL */
    fm_uint64 cntUnderrunPkts;

    /** Number of frames that overflowed the receiver and were dropped.
     *  For FM6000, this counter is 32 bits.
     *  
     *  \counterGroups FM4000:GroupEPL, FM6000:GroupEPL, FM10000:GroupEPL */
    fm_uint64 cntOverrunPkts;

    /** Number of received packets smaller than the configured minimum size
     *  with either a CRC or alignment error. For FM6000, this counter is
     *  32 bits
     *  
     *  \counterGroups FM2000:Group2, FM4000:Group2,
     *                 FM6000:GroupEPL, FM10000:GroupEPL */
    fm_uint64 cntRxFragmentPkts;

    /** Number of received packets smaller than the configured minimum size,
     *  but with a valid CRC. For FM6000, this counter is 32 bits
     *  
     *  \counterGroups FM2000:Group2, FM4000:Group2,
     *                 FM6000:GroupEPL, FM10000:GroupEPL */
    fm_uint64 cntRxUndersizedPkts;

    /** Number of received packets larger than the configured maximum size
     *  with either a CRC or alignment error. For FM2000 and FM4000 
     *  this counter is 16 bits. For FM6000 this counter is 32 bits.
     *  
     *  \counterGroups FM2000:Group2, FM4000:GroupEPL,
     *                 FM6000:GroupEPL, FM10000:GroupEPL */
    fm_uint64 cntRxJabberPkts;

    /** Number of frames that were corrupted within the switch. When the frame
     *  had a correct CRC on RX but not on TX this counter is incremented.
     *  
     *  \counterGroups FM2000:GroupEPL, FM4000:GroupEPL */
    fm_uint64 cntCorruptedPkts;

    /** Number of /E/ symbols decoded. Disparity Errors will cause
     *  /E/ symbols to be generated. This counter is 32 bits.
     *  
     * \counterGroups FM6000:GroupEPL, FM10000:GroupEPL */ 
    fm_uint64 cntCodeErrors;

    /** Number of received packets larger than the configured maximum size.
     *  For FM2000 and FM4000, this applies to packets well-formed or with
     *  either a CRC or alignment error. For FM6000, this applies to packets
     *  that are well-formed. For FM6000 devices, this counter is 32 bits.
     *  
     *  \counterGroups FM2000:Group2, FM4000:Group2,
     *                 FM6000:GroupEPL, FM10000:GroupEPL */
    fm_uint64 cntRxOversizedPkts;

    /** Number of transmitted packets with CRC error, but proper size.
     *  For FM2000 and FM3000/FM4000, this counter is 32 bits.
     *  
     *  \counterGroups FM2000:GroupEPL, FM4000:GroupEPL, FM6000:Group7 */
    fm_uint64 cntTxFCSErroredPkts;

    /*********************************************/
    /* Other Counters                            */
    /*********************************************/

    /** Number of counter updates to counter groups 1 through 6 (RX counters)
     *  that were missed due to insufficient counter bandwidth.
     *  
     *  \counterGroups FM4000:GroupOther */
    fm_uint64 cntStatsDropCountTx;

    /** Number of counter updates to counter groups 7 through 9 (TX
     *  counters) that were missed due to insufficient counter
     *  bandwidth.
     *  
     *  \counterGroups FM4000:GroupOther */
    fm_uint64 cntStatsDropCountRx;

    /** Number of mirrored packets transmitted on this port.
     *  
     * \counterGroups FM6000:GroupOther */ 
    fm_uint64 cntTxMirrorPkts;

    /** Number of mirrored octets transmitted on this port
     *  
     * \counterGroups FM6000:GroupOther */ 
    fm_uint64 cntTxMirrorOctets;

    /** Number of transmitted packets dropped
     *  for congestion management.
     *  
     *  \counterGroups FM2000:Group10, FM4000:Group10,
     *                 FM6000:Group10, FM10000:Group10 */
    fm_uint64 cntTxCMDropPkts;
    
    /** Time at which the counter read started in microseconds. May
     *  be used on two consecutive reads to determine packet, octet
     *  or error rates. */
    fm_uint64 timestamp;
    
} fm_portCounters;


/**************************************************/
/** \ingroup typeStruct
 * Per VLAN statistics
 **************************************************/
typedef struct _fm_vlanCounters
{
    /** Identifies the VLAN statistics structure version number. */
    fm_uint64 cntVersion;

    /** Versions: FM2000_STATS_VERSION, FM4000_STATS_VERSION.
     * Number of octets received on VLAN in unicast frames. */
    fm_uint64 cntUcstOctets;

    /** Versions: FM2000_STATS_VERSION, FM4000_STATS_VERSION.
     * Number of octets received on VLAN in multiicast and broadcast frames. */
    fm_uint64 cntXcstOctets;

    /** Versions: FM2000_STATS_VERSION, FM4000_STATS_VERSION.
     * Number of received unicast frames on VLAN. */
    fm_uint64 cntUcstPkts;

    /** Versions: FM2000_STATS_VERSION, FM4000_STATS_VERSION.
     * Number of received multicast and broadcast frames on VLAN. */
    fm_uint64 cntXcstPkts;

    /** Versions: ''FM6000_STATS_VERSION'',
     * ''FM10000_STATS_VERSION''. Number of received unicast 
     * frames on VLAN. */ 
    fm_uint64 cntRxUcstPkts;

    /** Versions: ''FM6000_STATS_VERSION'',
     * ''FM10000_STATS_VERSION''. Number of received multicast 
     * frames on VLAN. */ 
    fm_uint64 cntRxMcstPkts;

    /** Versions: ''FM6000_STATS_VERSION'',
     * ''FM10000_STATS_VERSION''. Number of received broadicast 
     * frames on VLAN. */ 
    fm_uint64 cntRxBcstPkts;

    /** Versions: ''FM6000_STATS_VERSION''. 
     * Number of frames dropped on receive on VLAN. */
    fm_uint64 cntRxDropPkts;

    /** Versions: ''FM6000_STATS_VERSION'',
     * ''FM10000_STATS_VERSION''. Number of received unicast 
     * octets on VLAN. */
    fm_uint64 cntRxUcstOctets;

    /** Versions: ''FM6000_STATS_VERSION'',
     * ''FM10000_STATS_VERSION''. Number of received multicast 
     * octets on VLAN. */
    fm_uint64 cntRxMcstOctets;

    /** Versions: ''FM6000_STATS_VERSION'',
     * ''FM10000_STATS_VERSION''. Number of received broadicast 
     * octets on VLAN. */
    fm_uint64 cntRxBcstOctets;

    /** Versions: ''FM6000_STATS_VERSION''. 
     * Number of octets dropped on receive on VLAN. */
    fm_uint64 cntRxDropOctets;

} fm_vlanCounters;


/**************************************************/
/** \ingroup typeStruct
 * Switch-wide global statistics
 * Not all fields in an instance of this structure should always be
 * considered valid. Fields only valid in certain versions will have a
 * "Versions: X" comment; Where "X" is the name of a version bit identifier
 * that must be set in cntVersion for the field to be considered valid.
 * If a field's documentation indicates no version then the field is always
 * valid.
 * If cntVersion does not have the required version bit set then the field is
 * set equal to 0.
 **************************************************/
typedef struct _fm_switchCounters
{
    /** Identifies the switch statistics structure version number. */
    fm_uint64 cntVersion;

    /** Number of frames dropped for congestion management due to the
     *  global low Priority Weighted Discard (PWD) watermark.
     */
    fm_uint64 cntGlobalLowDropPkts;

    /** Number of frames dropped due to the global high Priority Weighted
     *  Discard (PWD) watermark.
     */
    fm_uint64 cntGlobalHighDropPkts;

    /** Number of frames dropped due to the global privilege watermark.
     */
    fm_uint64 cntGlobalPrivilegeDropPkts;

    /** Number of counter updates to counter groups 1 through 6 (RX counters)
     *  that were missed due to insufficient counter bandwidth.
     *  This a global count. FM3000/FM4000 devices only support a per-port
     *  stat drop counter. */
    fm_uint64 cntStatsDropCountTx;

    /** Number of counter updates to counter groups 7 through 9 (TX counters)
     *  that were missed due to insufficient counter bandwidth.
     *  This a global count. FM3000/FM4000 devices only support a per-port
     *  stat drop counter. */
    fm_uint64 cntStatsDropCountRx;

} fm_switchCounters;

fm_status fmGetPortCounters(fm_int sw, fm_int port, fm_portCounters *cnt);
fm_status fmResetPortCounters(fm_int sw, fm_int port);
fm_status fmGetVLANCounters(fm_int sw, fm_int vlan, fm_vlanCounters *cnt);
fm_status fmResetVLANCounters(fm_int sw, fm_int vlan);
fm_status fmAllocateVLANCounters(fm_int sw, fm_int vlan);
fm_status fmFreeVLANCounters(fm_int sw, fm_int vlan);
fm_status fmGetSwitchCounters(fm_int sw, fm_switchCounters *cnt);
fm_status fmResetSwitchCounters(fm_int sw);


#endif /* __FM_FM_API_STATS_H */
