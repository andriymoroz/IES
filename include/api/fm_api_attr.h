/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_attr.h
 * Creation Date:   April 20, 2005
 * Description:     Constants for attributes and attribute values
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

#ifndef __FM_FM_API_ATTR_H
#define __FM_FM_API_ATTR_H


/****************************************************************************/
/** \ingroup constSwAttr
 *
 * Switch Attributes, used as an argument to ''fmSetSwitchAttribute'' and
 * ''fmGetSwitchAttribute''.
 *                                                                          \lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 ****************************************************************************/
enum _fm_switchAttr
{
    /** Type fm_int: VLAN tunneling behavior. Value is:
     *                                                                  \lb\lb
     *  FM_VLAN_TUNNEL_OFF (default),                                       
     *                                                                  \lb\lb
     *  FM_VLAN_TUNNEL_UCAST,                                               
     *                                                                  \lb\lb
     *  FM_VLAN_TUNNEL_MCAST or                                             
     *                                                                  \lb\lb
     *  FM_VLAN_TUNNEL_ALL.
     *                                                                  \lb\lb
     *  On devices other than FM2000, ACLs may be used to offer the same 
     *  services. 
     *
     *  \chips  FM2000 */
    FM_VLAN_TUNNELING = 0,

    /** Type fm_bool: Deprecated. Use ''FM_SPANNING_TREE_MODE'' instead.
     *                                                                  \lb\lb
     *  Setting this attribute to FM_ENABLED is identical to setting the
     *  ''FM_SPANNING_TREE_MODE'' attribute to ''FM_SPANNING_TREE_PER_VLAN''. 
     *  Setting this attribute to FM_DISABLED (default) is identical to setting the
     *  ''FM_SPANNING_TREE_MODE'' attribute to ''FM_SPANNING_TREE_SHARED''. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_MULTIPLE_SPT,

    /** Type ''fm_stpMode'': Multiple spanning tree mode. Default is
     *  ''FM_SPANNING_TREE_SHARED''. See ''fm_stpMode'' for possible values. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_SPANNING_TREE_MODE,

    /** Type ''fm_vlanLearningMode'': Vlan learning mode. Default is
     *  ''FM_VLAN_LEARNING_MODE_INDEPENDENT''. See ''fm_vlanLearningMode'' 
     *  for possible values. 
     *                                                          \lb\lb
     *  Note: When the value of this attribute is changed, all
     *  MA Table entries will be immediately deleted, including static entries.
     *  AGE events will not be generated for the deleted entries.
     *
     *  \chips  FM6000, FM10000 */
    FM_VLAN_LEARNING_MODE,

    /** Type fm_int: The Vlan to learn on when ''FM_VLAN_LEARNING_MODE''
     *  is set to ''FM_VLAN_LEARNING_MODE_SHARED''.  Default is 1.
     *
     *  \chips  FM6000, FM10000 */
    FM_VLAN_LEARNING_SHARED_VLAN,

    /** Type ''fm_vlanEtherType'': One of a set of 16-bit Ethernet types
     *  used to identify ingressing VLAN tagged frames. Default for all
     *  indices is 0x8100.
     *
     *  \chips  FM10000 */
    FM_SWITCH_PARSER_VLAN_ETYPES,

    /** Type ''fm_vlanEtherType'': One of a set of 16-bit Ethernet types
     *  used to modify the VLAN EtherType of an egressing frame. Default for
     *  all indices is 0x8100.
     *
     *  \chips  FM10000 */
    FM_SWITCH_MODIFY_VLAN_ETYPES,

    /** Type fm_macaddr: The 48-bit CPU MAC address (default: 0x000000000000). 
     *  Frames received by the switch with this destination MAC address will
     *  be automatically trapped to the CPU. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_CPU_MAC,

    /** Type ''fm_cpuMacTrapDisp'': Indicates the disposition for frames
     *  received with a DMAC matching the MAC configured via FM_CPU_MAC.
     *  Default is ''FM_CPU_MAC_TRAP_DISP_IBV_CHECK''.
     *  See ''fm_cpuMacTrapDisp'' for possible values.
     *
     *  \chips  FM6000 */
    FM_CPU_MAC_TRAP_DISP,

    /** Type fm_bool: Trapping of 802.1x frames to the CPU: FM_ENABLED (default)
     *  or FM_DISABLED. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute acts as a "master switch" for
     *  the ''FM_PORT_TRAP_IEEE_8021X'' port attribute; this attribute must 
     *  be set to FM_ENABLED for any port's ''FM_PORT_TRAP_IEEE_8021X'' 
     *  attribute to be effective.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_TRAP_IEEE_8021X,

    /** Type fm_bool: Trapping of spanning tree BPDU frames to the CPU:
     *  FM_ENABLED (default) or FM_DISABLED. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute acts as a "master switch" for 
     *  the ''FM_PORT_TRAP_IEEE_BPDU'' port attribute; this attribute must 
     *  be set to FM_ENABLED for any port's ''FM_PORT_TRAP_IEEE_BPDU'' 
     *  attribute to be effective.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_TRAP_IEEE_BPDU,

    /** Type fm_bool: Trapping of 802.3ad link aggregation protocol frames to
     *  the CPU: FM_ENABLED (default) or FM_DISABLED. Note that this
     *  attribute interacts with the ''FM_LAG_MODE'' switch attribute and
     *  with the ''FM_LAG_LACP_DISPOSITION'' LAG attribute (see
     *  ''Link Aggregation Group Attributes''.) This attribute controls
     *  whether LACP frames get trapped to the CPU at the hardware level.
     *  Whether those frames get passed on to the application or are discarded
     *  by the API is controlled by the other two attributes. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute acts as a "master switch" for 
     *  the ''FM_PORT_TRAP_IEEE_LACP'' port attribute; this attribute must 
     *  be set to FM_ENABLED for any port's ''FM_PORT_TRAP_IEEE_LACP'' 
     *  attribute to be effective.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_TRAP_IEEE_LACP,

    /** Type fm_bool: Trapping of GARP frames to the CPU: FM_ENABLED (default)
     *  or FM_DISABLED. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute acts as a "master switch" for 
     *  the ''FM_PORT_TRAP_IEEE_GARP'' port attribute; this attribute must 
     *  be set to FM_ENABLED for any port's ''FM_PORT_TRAP_IEEE_GARP'' 
     *  attribute to be effective.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_TRAP_IEEE_GARP,

    /** Type fm_bool: This attribute is non-functional as IGMPv3 
     *  trapping is not supported on FM2000 devices. For FM3000, FM4000
     *  and FM6000, use the VLAN attribute FM_VLAN_IGMP_TRAPPING.
     *
     *  \chips  FM2000 */
    FM_TRAP_IEEE_IGMPV3,

    /** Type fm_bool: Trapping of all IEEE reserved multicast frames (not
     *  otherwise enumerated) to the CPU: FM_ENABLED (default) or FM_DISABLED. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute acts as a "master switch" for 
     *  the ''FM_PORT_TRAP_IEEE_OTHER'' port attribute; this attribute must 
     *  be set to FM_ENABLED for any port's ''FM_PORT_TRAP_IEEE_OTHER'' 
     *  attribute to be effective.
     *                                                                  \lb\lb
     *  On FM10000 devices, use the ''FM_SWITCH_RESERVED_MAC_CFG'' switch
     *  attribute to enable trapping of individual reserved MAC addresses.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_TRAP_IEEE_OTHER,

    /** Type fm_int: The 16-bit Ethernet type value that should
     *  be trapped to the CPU. Specify -1 to disable trapping a previously set
     *  Ethernet type (trapping is disabled by default). 
     *
     *  \chips  FM2000 */
    FM_TRAP_ETH_TYPE,

    /** Type fm_bool: Trapping of MTU violations: FM_ENABLED
     *  (default) or FM_DISABLED. This trap is only valid for routed frames.
     *                                                                  \lb\lb
     *  For FM3000, FM4000, the API property api.FM4000.enableVidAsFid
     *  must be set to TRUE in order for this attribute to work properly. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAP_MTU_VIOLATIONS,

    /** Type fm_bool: Frames with Ethertype 8808 and MAC control address
     *  01-80-c2-00-00-01 are discarded: FM_ENABLED (default) or FM_DISABLED.
     *  Note that if disabled, frames with Ethertype 8808 and MAC control
     *  address 01-80-c2-00-00-01 are treated as ordinary multicast.
     *  
     *  For FM6000 and FM10000 devices, the frames will be dropped if the
     *  MAC matches 01-80-c2-00-00-01 OR if the Ethertype matches 8808.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_DROP_PAUSE,

    /** Type fm_int: Broadcast flooding control. Specifies the set of ports
     *  to which broadcast frames will be forwarded. Value is:
     *                                                                  \lb\lb
     *  FM_BCAST_DISCARD to discard all broadcast frames.                   
     *                                                                  \lb\lb
     *  FM_BCAST_FWD to forward broadcast frames to all ports including the
     *  CPU. Note that the CPU will only receive broadcasts on VLANs for
     *  which the CPU port is a member.                                     
     *                                                                  \lb\lb
     *  FM_BCAST_FWD_EXCPU (default) to forward broadcasts to all ports
     *  excluding the CPU.
     *  
     *  FM_BCAST_FLOODING_PER_PORT (FM6000 only) to handle broadcast frames
     *  based on the configuration of the port the frame arrives on (see the
     *  ''FM_PORT_BCAST_FLOODING'' port attribute).
     *                                                                  \lb\lb
     *  For FM6000 and FM10000 devices, using this attribute will overwrite
     *  the configuration of the ''FM_PORT_BCAST_FLOODING'' port attribute.
     *                                                                  \lb\lb
     *  For FM10000 devices, this attribute must be set to ''FM_BCAST_FWD''
     *  to ensure that VLAN information retrieved from a packet received
     *  from a PEP port is properly forwarded in the FTAG of the egress
     *  packet going to the CPU port. If the ''FM_PORT_BCAST_FLOODING'' port
     *  attribute is used instead to forward broadcast packets to the CPU
     *  port, the VLAN information will not be preserved when such packets
     *  are forwarded to the CPU port.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_BCAST_FLOODING,

    /** Type fm_int: Multicast flooding control. Value is:                  
     *                                                                  \lb\lb
     *  FM_MCAST_DISCARD to discard all multicast frames for which the
     *  multicast address is unknown on lookup,                             
     *                                                                  \lb\lb
     *  FM_MCAST_FWD to flood all multicast frames, for which the
     *  multicast address is unknown on lookup, to all ports including
     *  the CPU port. Note that the CPU will only receive multicast frames
     *  on VLANs for which the CPU port is a member.                                     
     *                                                                  \lb\lb
     *  FM_MCAST_FWD_EXCPU (default) to forward multicast frames to all ports
     *  excluding the CPU.
     *                                                                  \lb\lb
     *  FM_MCAST_FLOODING_PER_PORT to handle the multicast frames for which
     *  the multicast address is unknown according to the configuration
     *  of the port the frame arrives on (see the ''FM_PORT_MCAST_FLOODING''
     *  port attribute).
     *                                                                  \lb\lb
     *  For FM3000 or FM4000 devices, setting this attribute to FM_MCAST_FWD
     *  will cause all ports to have their ''FM_PORT_MCAST_FLOODING'' port
     *  attribute reset to FM_PORT_MCAST_FWD.
     *                                                                  \lb\lb
     *  For FM6000 and FM10000 devices, using this attribute will overwrite   
     *  the configuration of the ''FM_PORT_MCAST_FLOODING'' port attribute.
     *                                                                  \lb\lb
     *  For FM10000 devices, this attribute must be set to ''FM_MCAST_FWD''
     *  to ensure that the VLAN information retrieved from a packet received
     *  from a PEP port is properly forwarded in the FTAG of the egress
     *  packet going to the CPU port. If the ''FM_PORT_MCAST_FLOODING'' port
     *  attribute is used instead to forward unknown multicast packets to
     *  the CPU port, the VLAN information will not be preserved when such
     *  packets are forwarded to the CPU port.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MCAST_FLOODING,

    /** Type fm_int: Unicast flooding control. Value is:                    
     *                                                                  \lb\lb
     *  FM_UCAST_DISCARD to discard all unicast frames for which the
     *  destination address is unknown on lookup,                           
     *                                                                  \lb\lb
     *  FM_UCAST_FWD to flood all unicast frames, for which the
     *  unicast address is unknown on lookup, to all ports including
     *  the CPU port. Note that the CPU will only receive unicast frames
     *  on VLANs for which the CPU port is a member.                                     
     *                                                                  \lb\lb
     *  FM_UCAST_FWD_EXCPU (default) to forward unicast frames to all ports
     *  excluding the CPU.
     *                                                                  \lb\lb
     *  FM_UCAST_FLOODING_PER_PORT to handle the unicast frames for which
     *  the destination address is unknown according to the configuration
     *  of the port the frame arrives on. See the ''FM_PORT_UCAST_FLOODING''
     *  port attribute. 
     *                                                                  \lb\lb
     *  For FM3000 or FM4000 devices, setting this attribute to FM_UCAST_FWD
     *  will cause all ports to have their ''FM_PORT_UCAST_FLOODING'' port
     *  attribute reset to FM_PORT_UCAST_FWD.
     *                                                                  \lb\lb
     *  For FM6000 and FM10000 devices, using this attribute will overwrite
     *  the configuration of the ''FM_PORT_UCAST_FLOODING'' port attribute.
     *                                                                  \lb\lb
     *  For FM10000 devices, this attribute must be set to ''FM_UCAST_FWD''
     *  to ensure that the VLAN information retrieved from a packet received
     *  from a PEP port is properly forwarded in the FTAG of the egress
     *  packet going to the CPU port. If the ''FM_PORT_UCAST_FLOODING'' port
     *  attribute is used instead to forward unknown unicast packets to the
     *  CPU port, the VLAN information will not be preserved when such
     *  packets are forwarded to the CPU port.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_UCAST_FLOODING,

    /** Type fm_int: Frame timeout. Deprecated: use ''FM_FRAME_AGING_TIME_MSEC''
     *  instead. This 28-bit value is used to determine whether a frame has
     *  been in the switch element for too long. Once the timer has expired,
     *  the frame will be discarded. Specify zero to disable the feature.
     *  Otherwise the value will represent increments of the CPU interface
     *  cycle time times 1024. For example if the CPU interface is 66MHz,
     *  0x00001 is 15 microseconds, 0xF4240 (default) is 15 seconds and
     *  xFFFFFFF is 1 hour. The default and maximum values are a function
     *  of the EBI (CPU interface) clock speed.
     *                                                                  \lb\lb
     *  For FM2000:                                                     \lb
     *  25MHz => default: 40 secs, max: 10,995 secs;                    \lb
     *  33MHz => default: 31 secs, max: 8,329 secs;                     \lb
     *  50MHz => default: 20 secs, max: 5,497 secs;                     \lb
     *  66MHz => default: 15 secs, max: 4,164 secs.
     *                                                                  \lb\lb
     *  For FM4000:                                                     \lb
     *  25MHz => default: 0 secs, max: 10,995 secs;                     \lb
     *  33MHz => default: 0 secs, max: 8,329 secs;                      \lb
     *  50MHz => default: 0 secs, max: 5,497 secs;                      \lb
     *  66MHz => default: 0 secs, max: 4,164 secs. 
     *                                                                  \lb\lb
     *  For FM6000 and FM10000 devices, use ''FM_FRAME_AGING_TIME_MSEC''.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_FRAME_AGING_TIME,

    /** Type fm_int: Frame timeout in milliseconds. This value is used to
     *  determine whether a frame has been in the switch element for too
     *  long. Once the timer has expired, the frame will be discarded.
     *  Specify zero to disable the feature. The default and maximum values
     *  are a function of the CPU interface clock speed. For FM2000, FM3000,
     *  and FM4000, this is the EBI clock. For FM6000, this is the PCIE
     *  clock. For FM10000, this is the frame handler clock.
     *                                                                  \lb\lb
     *  This attribute is a clock-independent variant of ''FM_FRAME_AGING_TIME''.
     *  Note that the aging time is converted to internal units (with
     *  rounding), which means that the value returned by ''fmGetSwitchAttribute''
     *  may not be identical to the value specified by ''fmSetSwitchAttribute''.
     *                                                                  \lb\lb
     *  For FM2000:                                                     \lb
     *  25MHz => default: 40,960 ms, max: 10,995,116 ms;                \lb
     *  33MHz => default: 31,030 ms, max: 8,329,633 ms;                 \lb
     *  50MHz => default: 20,480 ms, max: 5,497,558 ms;                 \lb
     *  66MHz => default: 15,515 ms, max: 4,164,816 ms;                 \lb
     *                                                                  \lb\lb
     *  For FM3000 and FM4000:                                          \lb
     *  25MHz => default: 0 ms, max: 10,995,116 ms;                     \lb
     *  33MHz => default: 0 ms, max: 8,329,633 ms;                      \lb
     *  50MHz => default: 0 ms, max: 5,497,558 ms;                      \lb
     *  66MHz => default: 0 ms, max: 4,164,816 ms;                      \lb 
     *                                                                  \lb\lb
     *  For FM6000:                                                     \lb
     *  125Mhz => default: 0 ms, max: 4,398,046 ms;                     \lb
     *                                                                  \lb\lb
     *  For FM10000:                                                    \lb
     *  300MHz  => default: 0 ms, max: 114,532 ms.                      \lb
     *  700MHz  => default: 0 ms, max: 49,127 ms.                       \lb
     *  1000MHz => default: 0 ms, max: 34,359 ms.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_FRAME_AGING_TIME_MSEC,

    /** Type ''fm_lagMode'': Select link aggregation group mode. Default is
     *  ''FM_MODE_STATIC''. Note that setting this attribute to any value will
     *  cause the ''FM_TRAP_IEEE_LACP'' attribute to be overridden to FM_ENABLED.
     *                                                                  \lb\lb
     *  Note that this attribute interacts with the ''FM_LAG_LACP_DISPOSITION'' 
     *  LAG attribute. Setting this attribute to ''FM_MODE_STATIC'' is 
     *  equivalent to setting ''FM_LAG_LACP_DISPOSITION'' to FM_PDU_DISCARD. 
     *  Setting this attribute to ''FM_MODE_DYNAMIC'' is equivalent to setting
     *  ''FM_LAG_LACP_DISPOSITION'' to FM_PDU_TOCPU. See 
     *  ''FM_LAG_LACP_DISPOSITION'' for more information. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_LAG_MODE,

    /** Type fm_bool: Enable pruning on LAGs with members spanning
     *  multiple switches, otherwise use filtering: FM_ENABLED (default)
     *  or FM_DISABLED. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LAG_PRUNING,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *                                                                  \lb\lb
     *  Configures hashing of L3/L4 headers for ECMP and LAG. The following
     *  bits may be ORed together to define the hashing algorithm:
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_SYM to symmetrized the SIP/DIP and L4SRC/L4DST 
     *  fields prior to hashing.
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_SIP to include the source IP address.
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_DIP to include the destination IP address.
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_PROT to include the L3 protocol field.
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_TCP to enable the FM_ROUTING_HASH_L4SRC and
     *  FM_ROUTING_HASH_L4DST bits when protocol field is TCP.
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_UDP to enable the FM_ROUTING_HASH_L4SRC and
     *  FM_ROUTING_HASH_L4DST bits when the protocol field is UDP.
     *  (FM3000 and FM4000 only)
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_PROT1 to enable the FM_ROUTING_HASH_L4SRC and
     *  FM_ROUTING_HASH_L4DST bits when the protocol field matches the
     *  FM_ROUTING_HASH_PROT_1 attribute. (FM3000 and FM4000 only)
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_PROT2 to enable the FM_ROUTING_HASH_L4SRC and
     *  FM_ROUTING_HASH_L4DST bits when the protocol field matches the
     *  FM_ROUTING_HASH_PROT_2 attribute. (FM3000 and FM4000 only)
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_L4SRC to include the L4 source port when the
     *  corresponding bit is set for the frame's protocol.
     *                                                                  \lb\lb
     *  FM_ROUTING_HASH_L4DST to include the L4 destination port when the
     *  corresponding bit is set for the frame's protocol.
     *                                                                   \lb\lb
     *  The default value is (FM_ROUTING_HASH_SIP | FM_ROUTING_HASH_DIP |
     *  FM_ROUTING_HASH_PROT | FM_ROUTING_HASH_TCP | FM_ROUTING_HASH_UDP |
     *  FM_ROUTING_HASH_L4SRC | FM_ROUTING_HASH_L4DST). 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_ROUTING_HASH,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *                                                                  \lb\lb
     *  Represents the value used for the hash rotation portion of the
     *  routing hash. For FM3000 and FM4000 devices, this attribute
     *  specifies one of three 12-bit hash rotations for use in ECMP
     *  binning. The attribute value may range from 0 to 2 with a default
     *  value of 0.
     *
     *  \chips  FM3000, FM4000 */
    FM_ROUTING_HASH_ROTATION,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *                                                                  \lb\lb
     *  When the FM_ROUTING_HASH_PROT1 bit of the ''FM_ROUTING_HASH''
     *  attribute is set, the layer 4 source and destination will contribute
     *  to the hash only if the frame's protocol field matches the value of
     *  this attribute. The attribute value ranges from 0 to 255 with a
     *  default value of 1.
     *
     *  \chips  FM3000, FM4000 */
    FM_ROUTING_HASH_PROT_1,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *                                                                  \lb\lb
     *  When the FM_ROUTING_HASH_PROT2 bit of the ''FM_ROUTING_HASH''
     *  attribute is set, the layer 4 source and destination will contribute
     *  to the hash only if the frame's protocol field matches the value of
     *  this attribute. The attribute value ranges from 0 to 255 with a
     *  default value of 1.
     *
     *  \chips  FM3000, FM4000 */
    FM_ROUTING_HASH_PROT_2,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *                                                                  \lb\lb
     *  Masks the IPv4 DiffServ field in the
     *  L34 hash function. The attribute value is a 6 bit mask with a
     *  default value of 0x00. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ROUTING_HASH_FLOW_DIFFSERV_MASK,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *                                                                  \lb\lb
     *  Masks the ISL tag's USER field in the
     *  L34 hash function. The attribute value is an 8 bit mask with a
     *  default value of 0x00. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ROUTING_HASH_FLOW_USER_MASK,

    /** Type fm_uint32: Deprecated - Use ''FM_L3_HASH_CONFIG'' instead.
     *  Masks the IPv6 Flow Label field in the
     *  L34 hash function. The attribute value is a 20 bit mask with a
     *  default value of 0x00000. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ROUTING_HASH_FLOW_LABEL_MASK,

    /** Type ''fm_L2HashKey'': Configures the L2 hashing key profile.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_L2_HASH_KEY,

    /** Type ''fm_L2HashRot'': Configures the L2 hash rotation A.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_L2_HASH_ROT_A,

    /** Type ''fm_L2HashRot'': Configures the L2 hash rotation B.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_L2_HASH_ROT_B,

    /** Type ''fm_L3HashConfig'': Configures the L3 hashing profile.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_L3_HASH_CONFIG,

    /** Type ''fm_hashRotationValue'': Specifies the arguments to the L3 hash
     *  rotation. The exponent should be in the range 0 to 15.  The default 
     *  value set by the API on bootup is:                                  \lb
     *  mantissa  = ''FM_L3_HASH_ROTATION_DEF_MANTISSA''                    \lb
     *  exponent = ''FM_L3_HASH_ROTATION_DEF_EXPONENT''                     \lb
     *                                                                      \lb
     *  See the FM6000 datasheet for an explanation of suitable values.
     *
     *  \chips FM6000 */
    FM_L3_HASH_ROTATION_VALUE,

    /** Type fm_bool: Remapping of any frame with a IEEE reserved 
     *  destination address to Switch Priority 15: FM_ENABLED (default) or 
     *  FM_DISABLED. 
     *
     *  \chips  FM2000 */
    FM_REMAP_IEEE_SP15,

    /** Type fm_bool: Remapping of any frame for which the 
     *  destination address is the programmable CPU MAC address (''FM_CPU_MAC'' 
     *  switch attribute) to Switch Priority 15: FM_ENABLED or FM_DISABLED 
     *  (default). 
     *
     *  \chips  FM2000 */
    FM_REMAP_CPU_SP15,

    /** Type fm_bool: Remapping of any frame for which the
     *  Ethernet type is the programmed Ethernet type trap (''FM_TRAP_ETH_TYPE''
     *  switch attribute) to Switch Priority 15: FM_ENABLED or FM_DISABLED
     *  (default). 
     *
     *  \chips  FM2000 */
    FM_REMAP_ET_SP15,

    /** Type fm_int: Indicates whether 802.1q based VLANs or
     *  port based VLANs should be used by the switch.  Value is:           
     *                                                                  \lb\lb
     *  FM_VLAN_MODE_PORT (default) for port based VLANs,                   
     *                                                                  \lb\lb
     *  FM_VLAN_MODE_8021Q for 802.1q based VLANs. 
     *
     *  \chips  FM2000 */
    FM_VLAN_TYPE,

    /** Type fm_uint: FM2000 family devices require that one VLAN
     *  be reserved for internal use by the API. Any VLAN in the
     *  the range 1 to 4095 may be used, but once selected, that VLAN cannot
     *  be used for any other purpose. The default reserved VLAN is specified
     *  by the constant FM_FM2000_DEFAULT_RESERVED_VLAN and is recommended
     *  to be 4095.
     *                                                                  \lb\lb
     *  FM4000 family devices require a reserved VLAN for per-vlan spanning
     *  tree mode (see the ''FM_MULTIPLE_SPT'' switch attribute), however this 
     *  VLAN must be specified by the platform layer at compile time as 
     *  ''FM_FM4000_RESERVED_VLAN'' and cannot be changed with this 
     *  attribute.
     *
     *  \chips  FM2000 */
    FM_RESERVED_VLAN,

    /** Type fm_int: Indicates whether packet payloads across the Logical
     *  CPU Interface (LCI) should be big-endian or little-endian, as
     *  required by the platform's packet engine. The packet engine is the
     *  mechanism by which the platform sends and receives packets to/from
     *  the switch (typically either via software or DMA).  Value is one of:
     *                                                                  \lb\lb
     *  FM_LCI_BIG_ENDIAN for big-endian packet payload. Choose this
     *  option if the platform's packet engine is big-endian.
     *                                                                  \lb\lb
     *  FM_LCI_LITTLE_ENDIAN for little-endian packet payload. Choose this 
     *  option if the platform's packet engine is little-endian.
     *                                                                  \lb\lb
     *  The hardware defaults this attribute to FM_LCI_LITTLE_ENDIAN, but
     *  the platform layer should specify the endianness explicitly, typically
     *  in ''fmPlatformSwitchPostInitialize''. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_LCI_ENDIANNESS,

    /** Type fm_int: Specifies the disposition for IP frames
     *  with options.  Value is one of:
     *                                                                  \lb\lb
     *  FM_IP_OPTIONS_TRAP to trap all frames to the CPU.
     *                                                                  \lb\lb
     *  FM_IP_OPTIONS_TRAP_MCST to trap the multicast frames to the CPU.
     *                                                                  \lb\lb
     *  FM_IP_OPTIONS_TRAP_UCST to trap the unicast frames to the CPU.
     *                                                                  \lb\lb
     *  FM_IP_OPTIONS_LOG_UCST to log the unicast frames to the CPU
     *  (FM6000 only).
     *                                                                  \lb\lb
     *  FM_IP_OPTIONS_FWD (default) to forwarded the frame normally.
     *                                                                  \lb\lb
     *  When restricted to FM_IP_OPTIONS_TRAP and FM_IP_OPTIONS_FWD, 
     *  this attribute produces the same effect as the 
     *  ''FM_ROUTER_TRAP_IP_OPTIONS'' router attribute, but that attribute is
     *  deprecated in favor of this one.
     *                                                                  \lb\lb
     *  Note that to use this attribute, one also needs to configure the
     *  ''FM_PORT_PARSER_FLAG_OPTIONS'' port attribute on the
     *  relevant ports to appropriately flag the relevant options. 
     *                                                                  \lb\lb
     *  For FM3000 and FM4000: The API property api.FM4000.enableVidAsFid
     *  must be set to TRUE in order for any of the TRAP values for this
     *  attribute to work properly.
     *                                                                  \lb\lb
     *  For FM6000: The values may be ORed together to perform both trapping
     *  and logging.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_IP_OPTIONS_DISPOSITION,

    /** Type fm_uint32: A mask specifying which statistics groups are
     *  enabled for counting.  Value is a mask of the following constants:
     *                                                                  \lb\lb
     *  FM_STAT_GROUP_PER_PORT_RX_FRAME_CLASS,                              \lb
     *  FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_LEN,                             \lb
     *  FM_STAT_GROUP_PER_PORT_RX_OCTETS,                                   \lb
     *  FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_PRIORITY,                        \lb
     *  FM_STAT_GROUP_PER_PORT_RX_OCTETS_BY_PRIORITY,                       \lb
     *  FM_STAT_GROUP_PER_PORT_FWD_ACTION,                                  \lb
     *  FM_STAT_GROUP_PER_PORT_TX_FRAME_CLASS,                              \lb
     *  FM_STAT_GROUP_PER_PORT_TX_COUNT_BY_LEN. 
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, see the ''FM_PORT_STAT_GROUP_ENABLE'' 
     *  port attribute.   
     *
     *  \chips  FM2000 */
    FM_STAT_GROUP_ENABLE,

    /** Type ''fm_LBGMode'':  Deprecated, the LBG mode can be selected
     *  in ''fmCreateLBGExt''. Indicates which load balancing
     *  group mode to use. Note that this mode can be overridden on a per
     *  load balancing group basis when the LBG is created
     *  with ''fmCreateLBGExt''.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_MODE,

    /** Type fm_int:  Specifies which logical port to redirect CPU traffic
     *  to. The logical port can be a physical port or a LAG port.
     *                                                                  \lb\lb
     *  This is used in stacking configurations where all CPU traffic in
     *  the stack must be passed to one primary switch, which will then
     *  trap the traffic to its CPU. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_REDIRECT_CPU_TRAFFIC,

    /** Type ''fm_mtuEntry'': Specifies a VLAN MTU. On FM3000, FM4000, and
     *  FM10000 devices, up to eight MTU values may be specified, each
     *  identified by an index field in the ''fm_mtuEntry'' structure. On
     *  FM6000 devices, up to 16 MTU values may be specified. Once an MTU
     *  value is established, it may be associated with any VLAN by setting
     *  the VLAN's ''FM_VLAN_MTU'' attribute.
     *                                                                  \lb\lb
     *  The MTU is checked only for IP frames and the check is made against
     *  the packet length found in the IP header, not the actual frame
     *  length.
     *                                                                  \lb\lb
     *  When retrieving this attribute, specify the index of the MTU to be
     *  retrieved in the ''fm_mtuEntry'' structure prior to calling
     *  ''fmGetSwitchAttribute''. That function will then fill in the
     *  structure with the corresponding MTU value. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_MTU_LIST,

    /** Type fm_int: A trapped frame may be identified as a 
     *  security violation (see ''fmSetPortSecurity'') if the value of the 
     *  trapAction member of the frame's associated ''fm_eventPktRecv'' event 
     *  structure is equal to the value of this attribute. This attribute is 
     *  read-only.
     *                                                                  \lb\lb
     *  The value of this attribute is established at initialization time,
     *  and remains static. 
     *
     *  \chips  FM3000, FM4000 */
    FM_SV_TRAP_ACTION,

    /** Type fm_int: Defines the number of bytes that will be added to each 
     *  frame for the purpose of ingress or egress rate-limiter and DRR 
     *  scheduling.
     *                                                                  \lb\lb
     *  For FM6000, the per-port QoS attribute ''FM_QOS_SCHED_IFGPENALTY'' 
     *  should be used. Using this switch attribute will overwrite any 
     *  configuration made with the ''FM_QOS_SCHED_IFGPENALTY'' attribute. 
     *  It is also possible that the value retrieved through this switch 
     *  attribute may not be accurate if ''FM_QOS_SCHED_IFGPENALTY'' is used.
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_IFG_PENALTY,

    /** Type fm_bool: Controls if the TRAP and LOG
     *  action will be executed in parallel or if TRAP action will
     *  have precedence over LOG action: 
     *                                                                  \lb\lb
     *  FM_ENABLED (default): TRAP and LOG will be executed in parallel.
     *                                                                  \lb\lb
     *  FM_DISABLED: TRAP action will have precedence over LOG action.
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_TRAP_PLUS_LOG,

    /** Type ''fm_ffuSliceAllocations'': Specifies how the FFU
     *  is partitioned between ACLs and route types. See 
     *  ''fm_ffuSliceAllocations'' for a detailed explanation.
     *                                                                  \lb\lb
     *  This attribute defaults to the values supplied in the relevant
     *  ''API Properties''.  These attributes are listed for each structure 
     *  member in the definition of the ''fm_ffuSliceAllocations'' structure.
     *  If an allocation other than the default is to be used, this attribute 
     *  should be set initially after the switch is brought up and prior to 
     *  adding any routes or ACLs.
     *
     * \chips   FM4000, FM6000, FM10000 */ 
    FM_FFU_SLICE_ALLOCATIONS,

    /** Type fm_uint32: Specifies the starting 32-bit word offset for L2 deep
     *  inspection. The offset can range between 0 and 11 with a default 
     *  value of 0.
     *  
     * \chips   FM6000 */
    FM_SWITCH_DI_L2_START_INDEX,

    /** Type fm_uint32: Specifies the starting 32-bit word offset for L4 deep
     *  inspection. The offset can range between 0 and 8 with a default 
     *  value of 0. The first two 16-bit words of L4 payload following the 
     *  IP header are always inspected as if they were an L4 source port and 
     *  L4 destination port. This offset begins following those two 16-bit 
     *  words.
     *  
     * \chips   FM6000 */
    FM_SWITCH_DI_L4_START_INDEX,

    /** Type fm_uint32: Specifies the starting 32-bit word offset for the L4
     *  deep inspection vector. The offset can range between 0 and 8, with a
     *  default value of 0.
     *                                                                  \lb\lb
     *  For tunneling applications, this attribute should be appropriately
     *  set. Note that for VXLAN and NVGRE tunneling, the attribute is
     *  automatically set to 1 when the first Virtual Network is created,
     *  and cleared to 0 when the last Virtual Network is deleted. This is
     *  required as the Virtual Network Identifier (VNI) is stored at the
     *  beginning of the deep inspection vector, and would be overwritten if
     *  the offset is 0.
     *  
     * \chips   FM6000 */
    FM_SWITCH_DI_L4_VECTOR_INDEX,

    /** Type ''fm_paritySweeperConfig'': Specifies the behavior of the 
     *  parity sweeper for the switch. See ''fm_paritySweeperConfig'' for
     *  the various parameters that can be tuned and their default values. 
     *                                                                  \lb\lb
     *  Note that this attribute has no effect if the ''api.paritySweeper.enable'' 
     *  API property is not set to TRUE.
     *  
     *  \chips  FM4000 */
    FM_PARITY_SWEEPER_CONFIG,

    /** Type ''fm_trapCodeMapping'': (Read Only) Identifies the numeric
      * trap code used for a particular trap type. The caller must set the
      * trap type in the ''fm_trapCodeMapping'' structure pointed to by the 
      * value argument to ''fmGetSwitchAttribute''. The function will return 
      * the associated trap code value in the same structure.
      *  
      * \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_SWITCH_TRAP_CODE, 

    /** Type ''fm_trapCodeMapping'': (Read Only) Identifies the trap type
      * associated with a particular trap code. The caller must set the
      * trap code in the ''fm_trapCodeMapping'' structure pointed to by the 
      * value argument to ''fmGetSwitchAttribute''. The function will return 
      * the associated trap type in the same structure.
      *  
      * \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_SWITCH_TRAP_TYPE, 

    /** Type fm_bool: Learning of source addresses for dropped frames:
     *  FM_ENABLED (default) or FM_DISABLED.
     *
     *  \chips  FM6000 */
    FM_LEARN_DROP_FRAME,

    /** Type fm_bool: Enable counting of frames that were mirrored in the
     *  cntTxMirrorPkts and cntTxMirrorOctets counters in ''fm_portCounters'',
     *  as returned by ''fmGetPortCounters''. Disabling this attribute will 
     *  prevent the mirror counter rules from hitting and therefore count the 
     *  transmitted frame (mirrored copy) as a unicast, multicast or broadcast 
     *  packet: FM_ENABLED (Default) or FM_DISABLED.
     *  
     *  \chips  FM6000 */
    FM_MIRROR_COUNTERS,

    /** Type ''fm_policerBankDropMask'': Specifies a policer bank (created by
     *  ''fmCreatePolicer'') and a set of logical ports to which the ACL
     *  policer drop action is to be applied. By default, all policer bank
     *  drop masks are configured to drop traffic on all destination ports.
     * 
     *  \chips  FM6000 */
    FM_POLICER_BANK_DROP_MASK,

    /** Type: fm_uint16. Defines the Ethernet type of a RBridge
     *  encapsulated frame. Defaults to 0x22F3 as assigned by the
     *  IEEE Registration Authority for the TRILL protocol.
     *                                                                  \lb\lb
     *  Note that this attribute only applies to the TRILL microcode.
     *
     * \chips   FM6000 */
    FM_SWITCH_RBRIDGE_ETHERNET_TYPE,

    /** Type fm_macaddr: The 48-bit RBridge MAC address (default:
     *  0x000000000000).
     *                                                                  \lb\lb
     *  Note that this attribute only applies to the TRILL microcode.
     *
     * \chips   FM6000 */
    FM_SWITCH_RBRIDGE_MAC,

    /** Type fm_macaddr: The 48-bit VxLan MAC address (default:
     *  0x000000000000).
     *                                                                  \lb\lb
     *  Note that this attribute only applies to the VN microcode.
     *
     * \chips   FM6000 */
    FM_SWITCH_VXLAN_MAC,

    /** Type fm_ipAddr: Specifies the IPv4 destination address used by the
     *  VXLAN tunneling protocol.
     *                                                                  \lb\lb
     *  Note that this attribute only applies to the VN microcode.
     *
     * \chips   FM6000 */
    FM_SWITCH_VXLAN_DIP,

    /** Type fm_uint32: Specifies the UDP destination port used by the
     *  VXLAN tunneling protocol. Default value is 4789.
     *                                                                  \lb\lb
     *  For FM6000, this attribute only applies to the VN microcode.
     *
     * \chips   FM6000, FM10000 */
    FM_SWITCH_TUNNEL_DEST_UDP_PORT,

    /** Type fm_uint32: Specifies the UDP destination port used by the Geneve
     *  (NGE) tunneling protocol. Default value is 6081.
     *
     * \chips   FM10000 */
    FM_SWITCH_GENEVE_TUNNEL_DEST_UDP_PORT,

    /** UNPUBLISHED: For internal use only. */
    FM_SPECIAL_POLICER_REMAP,

    /** UNPUBLISHED: For internal use only. */
    FM_SPECIAL_POLICER_CREDIT,

    /** Type fm_bool: Whether to drop frames with MTU violations: FM_ENABLED
     *  or FM_DISABLED (default). Only applies to routed frames.
     *  ''FM_TRAP_MTU_VIOLATIONS'' takes precedence over this attribute.
     *                                                                  
     *  \chips  FM6000 */
    FM_DROP_MTU_VIOLATIONS,

    /** Type fm_bool: Whether to parse the Virtual Network endpoint tag if
     *  one is present: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  If a VNTAG is detected, its data words will be available as deep
     *  inspection fields G and H. Note that VN-Tag parsing is incompatible
     *  with Deep Inspection parsing (FM_PORT_DI_PARSING).
     *  
     *  \chips  FM6000 */
    FM_SWITCH_PARSE_VNTAG,

    /** Type fm_bool: Whether to parse the MPLS label if one is present:
     *  FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Note that MPLS label parsing is incompatible with the modifications
     *  made to routed frames. All ports to which MPLS-labeled frames are
     *  switched should have their ''FM_PORT_UPDATE_DSCP'' and
     *  ''FM_PORT_UPDATE_TTL'' attributes set to FM_DISABLED.
     *                                                                  \lb\lb
     *  This attribute only applies to the VN microcode.
     *
     *  \chips  FM6000 */
    FM_SWITCH_PARSE_MPLS,

    /** Type fm_bool: Whether to compute a tunnel hash. Used by a
     *  tunneling protocol such as VxLAN or NVGRE: FM_ENABLED or
     *  FM_DISABLED (default).
     *
     *  \chips  FM6000 */
    FM_SWITCH_TUNNEL_HASH,

    /** Type fm_bool: Whether to parse tunneling IP protocols over IPv4 protocol:
     *  FM_ENABLED (default) or FM_DISABLED.
     *  
     *  This attribute enables parsing of two protocols: IPv4-in-IPv4, and
     *  IPv6-in-IPv4.
     *
     *  \chips  FM6000 */
    FM_SWITCH_PARSE_IP_IN_IP,

    /** Type fm_bool: Whether to parse GRE protocol over IP protocol:
     *  FM_ENABLED (default) or FM_DISABLED.
     *  
     *  \chips  FM6000 */
    FM_SWITCH_PARSE_GRE,

    /** Type fm_bool: Whether to parse the ARP frame if one is
     *  present: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  If an ARP is detected, its data words will be available in
     *  the deep inspection fields. Note that ARP parsing is
     *  enabled if L3/L4 parsing is enabled and Deep Inspection
     *  parsing is enabled (FM_PORT_DI_PARSING).
     *
     *  \chips  FM6000 */
    FM_SWITCH_PARSE_ARP,

    /** Type fm_bool: Whether to detect the Cisco Fabric Path header
     *  if one is present: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  If a CFP header is detected, its innner frame will be
     *  decoded.
     *  
     *  \chips  FM6000 */
    FM_SWITCH_PARSE_CFP,

    /** Type fm_bool: Whether to storm control based on flooded
     *  frames or all frames: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  \chips  FM6000 */
    FM_SWITCH_STORM_FLOOD_ONLY,

    /** Type fm_bool: Specifies whether the frame parsing logic should
     *  handle frames with more than two VLAN tags: FM_ENABLED or
     *  FM_DISABLED (default).
     *                                                                  \lb\lb
     *  This attribute allows the switch to parse L3 and higher if VTAG3 is
     *  present. Extra tags are recognized using SWITCH_VLAN1_ETHERTYPE_A,
     *  SWITCH_VLAN1_ETHERTYPE_B, SWITCH_VLAN2_ETHERTYPE_A, and
     *  SWITCH_VLAN2_ETHERTYPE_B. When disabled, VLAN3 and any following
     *  VLAN tags are treated as part of the frame payload.
     *                                                                  \lb\lb
     *  The switch should be configured so as not to modify any part of the
     *  frame following the VLAN tags (including DSCP and L3 checksum).
     *  Otherwise, the frame may be corrupted.
     *                                                                  \lb\lb
     *  Note that this attribute only applies to the non-TRILL microcode.
     *                                                                  \lb\lb
     *  \chips  FM6000 */
    FM_SWITCH_SKIP_EXTRA_VLAN,

    /** Type ''fm_customTag'': Configurations for all fields of global custom 
     *  tags. Maximum of 4 global custom tags can be configured.  
     *  
     *  \chips  FM10000 */
    FM_SWITCH_PARSER_CUSTOM_TAG,

    /** Type ''fm_parserDiCfg'': Configurations for all fields of Deep 
     *  Inspection (DI) filters. A maximum of 6 DI filters can be configured. When 
     *  multiple filters are matching for a frame then the one with the highest
     *  index will be chosen. When L4 Parsing is enabled via the ''FM_PORT_PARSER'' port
     *  attribute and none of the DI filters are matching, then DI filter 0 is 
     *  applied automatically. Since DI filter 0 could be seen as the default
     *  profile, it is not possible to configure any matching condition for
     *  this specific index.
     * 
     *  \chips  FM10000 */
    FM_SWITCH_PARSER_DI_CFG,

    /** Type fm_bool: Whether frames with an invalid SMAC (all zeroes, all
     *  ones, or multicast bit [40] is a 1) should be dropped: FM_ENABLED
     *  (default) or FM_DISABLED.
     *
     *  \chips  FM10000 */
    FM_DROP_INVALID_SMAC,

    /** Type: ''fm_mplsEtherType''. Defines the Ethernet types used to
     *  identify MPLS tagged frames. Defaults to 0x8847 (first index) and
     *  0x8848 (second index).
     *
     * \chips   FM10000   */
    FM_SWITCH_MPLS_ETHER_TYPES,

    /** Type: fm_macaddr. Defines the source MAC address to use for
     *  generated PAUSE frames.
     *  
     *  \chips  FM10000 */
    FM_SWITCH_PAUSE_SMAC,

    /** Type: ''fm_reservedMacCfg''. Configuration for one of the IEEE
     *  reserved multicast addresses. These are MAC addresses of the form
     *  01-80-C2-00-00-XX. A maximum of ''FM_NUM_RESERVED_MACS'' (64)
     *  addresses may be specified.
     *  
     *  \chips  FM10000 */
    FM_SWITCH_RESERVED_MAC_CFG,

    /** Type: fm_int. The switch priority of a frame with a reserved
     *  multicast destination MAC address that is trapped to the CPU.
     *  
     *  \chips  FM10000 */
    FM_SWITCH_RESERVED_MAC_TRAP_PRI,

    /** Type ''fm_vlanMapEntry'': Used to configure the entries of a map
     *  table which provides the VLAN1 tag to use on a given frame when
     *  tagging occurs.
     *
     *  \chips  FM10000 */
    FM_SWITCH_VLAN1_MAP,

    /** Type ''fm_vlanMapEntry'': Used to configure the entries of a map
     *  table which provides the VLAN2 tag to use on a given frame when
     *  tagging occurs.
     *
     *  \chips  FM10000 */
    FM_SWITCH_VLAN2_MAP,

    /** Type ''fm_sglortVsiMap'': Used to configure the SGLORT to VSI
     *  mapping for use with VXLAN, NVGRE, and Geneve (NGE) tunneling.
     *                                                                  \lb\lb
     *  Default values cause all 8 entries to be disabled.
     *  
     *  \chips  FM10000 */
    FM_SWITCH_SGLORT_VSI_MAP,

    /** Type ''fm_vsiData'': Used to configure VN tunnel encapsulation
     *  fields for a VSI.
     *                                                                  \lb\lb
     *  Default values for encapsulation SIP and VNI are 0.
     *  
     *  \chips  FM10000 */
    FM_SWITCH_VSI_DATA,

    /** Type fm_int: Specifies the mirror trapcode ID associated with
     *  packets for which PEP-to-PEP timestamps are to be retrieved. The
     *  value must match the ''FM_MIRROR_TRAPCODE_ID'' attribute of the
     *  mirror created for timestamp retrieval purposes. A trapcode ID of
     *  zero disables this feature.
     *                                                                  \lb\lb
     *  Default value is 0.
     *  
     *  \chips  FM10000 */
    FM_SWITCH_PEP_TIMESTAMP_TRAP_ID,

    /** Type fm_bool: Whether the packet processing logic should drop frames
     *  received on an unknown port: FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  When the packet receive logic receives a packet but is unable to
     *  map the source GloRT to a logical port, it can drop the frame (disabled)
     *  or set the source port value to -1 and process the frame normally
     *  (enabled).
     *                                                                  \lb\lb
     *  \chips  FM6000, FM10000 */
    FM_SWITCH_RX_PKT_DROP_UNKNOWN_PORT,

    /** Type fm_bool: Whether Egress Timestamping should be performed on
     *  Ethernet ports: FM_ENABLED or FM_DISABLED (default). When enabling
     *  timestamping, this attribute should be set after the required
     *  timestamping configurations are made.
     *
     *  \chips  FM10000 */
    FM_SWITCH_TX_TIMESTAMP_MODE,

    /** Type fm_int: Specifies the virtual port number of the PEP to which
     *  timestamps from ethernet ports are to be delivered. A value of -1
     *  (the default) disables this feature.
     *
     *  \chips  FM10000 */
    FM_SWITCH_ETH_TIMESTAMP_OWNER,

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_ATTRIBUTE_MAX

};  /* end enum _fm_switchAttr */


/** A legacy synonym for ''FM_IFG_PENALTY''.
 *  \ingroup macroSynonym */
#define FM_IFG_PENALITY FM_IFG_PENALTY

/** A legacy synonym for ''FM_SWITCH_TUNNEL_HASH''.
 *  ''FM_SWITCH_NVGRE_HASH'' has been deprecated in favor of the
 *  ''FM_SWITCH_TUNNEL_HASH''
 *  \ingroup macroSynonym */
#define FM_SWITCH_NVGRE_HASH   FM_SWITCH_TUNNEL_HASH

/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_LBG_MODE'' switch attribute.
 **************************************************/
typedef enum
{
    /** Initial traffic distribution is hashed equally and failover is to 
     *  one replacement port (N+1 redundancy if the replacement port is not 
     *  already an active port). On FM4000 devices, the LBG may contain 
     *  a maximum of 16 member ports.
     *
     *  \chips  FM3000, FM4000 */
    FM_LBG_MODE_NPLUS1,
    
    /** Initial traffic distribution is hashed equally and failover can be 
     *  to one replacement port or equally over all remaining ports. 
     *  On FM4000 devices, the LBG may contain a maximum of 16 member ports.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_MODE_REDIRECT,
    
    /** Initial and failover traffic distribution is specified by the 
     *  application using a distribution map of type ''fm_LBGDistributionMap''
     *  (FM3000 and FM4000 devices) or ''fm_LBGDistributionMapRange'' (FM6000 
     *  devices) or ''fm_LBGDistributionMapRangeV2'' (FM10000 devices) 
     *                                                                  \lb\lb
     *  On FM3000 and FM4000 devices only:
     *                                                                  \lb\lb
     *  (a) The LBG may contain a maximum of 20 member ports. 
     *                                                                  \lb\lb
     *  (b) All LBG member ports must have their ''FM_PORT_UPDATE_ROUTED_FRAME''
     *  attribute set to FM_DISABLED. 
     *                                                                  \lb\lb
     *  (c) This mode cannot be used concurrently with routing on LBG member
     *  ports. 
     *                                                                  \lb\lb
     *  (d) This mode will not work for IP parsed frames (''FM_PORT_PARSER''
     *  set to parse beyond L2 on the ingress port) that have a multicast or
     *  broadcast DMAC and a unicast IP address. Such frames will be
     *  L2 switched normally rather than exclusively distributed within the
     *  LBG. (This restriction can be worked around by adding an ACL that
     *  explicitly matches on such frames and directs them to one of the
     *  member ports of the LBG. Note that the application must explicitly
     *  manage the destination port of the ACL if it is removed from the
     *  LBG or goes down.)
     *                                                                  \lb\lb
     *  FM6000 devices do not have these restrictions.
     *
     *  On FM10000 devices, only Layer 3 and Layer 4 fields of IP frames are 
     *  used in hash computation for traffic distribution. To load balance 
     *  non-IP frames LBG mode ''FM_LBG_MODE_MAPPED_L234HASH'' should be used.
     *                                                                  \lb\lb
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_MODE_MAPPED,

    /** Initial and failover traffic distribution is specified by the 
     *  application using a distribution map of type
     *  ''fm_LBGDistributionMap''. This mode should be used if hash 
     *  computation for traffic distribution should include L2 fields. 
     *
     *  \chips  FM10000 */
    FM_LBG_MODE_MAPPED_L234HASH
         
} fm_LBGMode;


/** A legacy synonym for ''fm_LBGMode''.            \ingroup macroSynonym */
#define fm_lbgMode fm_LBGMode


/**************************************************
 * For FM_IP_OPTIONS_DISPOSITION
 * 1 bit field in SYS_CFG_ROUTER
 **************************************************/
#define FM_IP_OPTIONS_FWD          (1 << 0)
#define FM_IP_OPTIONS_TRAP         (1 << 1)
#define FM_IP_OPTIONS_TRAP_UCST    (1 << 2)
#define FM_IP_OPTIONS_TRAP_MCST    (1 << 3)
#define FM_IP_OPTIONS_LOG_UCST     (1 << 4)


/**************************************************
 * For FM_VLAN_TYPE
 * 1-bit field in SYS_CFG_2
 * Deprecated, always assume 802.1Q.
 **************************************************/
#define FM_VLAN_MODE_PORT           0
#define FM_VLAN_MODE_8021Q          1


/**************************************************
 * For FM_VLAN_TUNNELING
 * These are 2 bits in SYS_CFG_2, mcast being more
 * significant.
 **************************************************/
#define FM_VLAN_TUNNEL_OFF          0x0
#define FM_VLAN_TUNNEL_UCAST        0x1
#define FM_VLAN_TUNNEL_MCAST        0x2
#define FM_VLAN_TUNNEL_ALL  \
    (FM_VLAN_TUNNEL_UCAST | \
     FM_VLAN_TUNNEL_MCAST)


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_BCAST_FLOODING'' switch attribute.
 **************************************************/
typedef enum
{
    /** Forward broadcast frames to all ports including the CPU. Note that
     *  the CPU will only receive broadcasts on VLANs for which the CPU port
     *  is a member.
     *  
     * \chips   FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_BCAST_FWD = 0,

    /** Discard all broadcast frames.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_BCAST_DISCARD,

    /** Forward broadcasts to all ports excluding the CPU (default).
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_BCAST_FWD_EXCPU,

    /** Handle broadcast frames based on the configuration of the port the
     *  frame arrives on (see the ''FM_PORT_BCAST_FLOODING'' port
     *  attribute).
     *  
     *  \chips  FM6000, FM10000 */
    FM_BCAST_FLOODING_PER_PORT,

} fm_bcastFlooding;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_MCAST_FLOODING'' switch attribute.
 **************************************************/
typedef enum
{
    /** Flood all multicast frames for which the multicast address is
     *  unknown on lookup to all ports including the CPU. Note that
     *  the CPU will only receive unknown multicast frames on VLANs for
     *  which the CPU port is a member.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MCAST_FWD = 0,

    /** Discard all multicast frames for which the multicast address is
     *  unknown on lookup.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MCAST_DISCARD,

    /** Forward all multicast frames for which the multicast address is
     *  unknown on lookup to all ports excluding the CPU port (default).
     *  
     *  \chips  FM10000 */
    FM_MCAST_FWD_EXCPU,

    /** Handle multicast frames for which the multicast address is unknown
     *  according to the configuration of the port the frame arrives on (see
     *  the ''FM_PORT_MCAST_FLOODING'' port attribute).
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MCAST_FLOODING_PER_PORT,

} fm_mcastFlooding;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_UCAST_FLOODING'' switch attribute.
 **************************************************/
typedef enum
{
    /** Flood all unicast frames for which the destination address is
     *  unknown on lookup.  Note that the CPU will only receive unknown
     *  unicast frames on VLANs for which the CPU port is a member.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_UCAST_FWD = 0,

    /** Discard all unicast frames for which the destination address is
     *  unknown on lookup.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_UCAST_DISCARD,

    /** Forward all unicast frames for which the unicast address is
     *  unknown on lookup to all ports excluding the CPU port (default).
     *  
     *  \chips  FM10000 */
    FM_UCAST_FWD_EXCPU,

    /** Handle unicast frames for which the destination address is unknown
     *  according to the configuration of the port the frame arrives on. See
     *  the ''FM_PORT_UCAST_FLOODING'' port attribute.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_UCAST_FLOODING_PER_PORT,

} fm_ucastFlooding;


/**************************************************
 * For FM_STAT_GROUP_ENABLE and 
 * FM_PORT_STAT_GROUP_ENABLE
 **************************************************/
#define FM_STAT_GROUP_PER_PORT_RX_FRAME_CLASS           (1U << 0)
#define FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_LEN          (1U << 1)
#define FM_STAT_GROUP_PER_PORT_RX_OCTETS                (1U << 2)
#define FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_PRIORITY     (1U << 3)
#define FM_STAT_GROUP_PER_PORT_RX_OCTETS_BY_PRIORITY    (1U << 4)
#define FM_STAT_GROUP_PER_PORT_FWD_ACTION               (1U << 5)
#define FM_STAT_GROUP_PER_PORT_TX_FRAME_CLASS           (1U << 6)
#define FM_STAT_GROUP_PER_PORT_TX_COUNT_BY_LEN          (1U << 7)
#define FM_STAT_GROUP_PER_PORT_TX_OCTETS                (1U << 8)
#define FM_STAT_GROUP_VLAN_RX_FRAME_CLASS               (1U << 9)
#define FM_STAT_GROUP_VLAN_RX_OCTETS                    (1U << 10)
#define FM_STAT_GROUP_EGRESS_ACL                        (1U << 11)
#define FM_STAT_GROUP_PER_PORT_TX_DROP                  (1U << 12)
#define FM_STAT_GROUP_PER_PORT_TRIGGER_COUNTERS         (1U << 13)


/**************************************************/
/** \ingroup typeStruct
 * A tuple defining the index number and value for
 * one of the switch global MTU entries.  This is 
 * used as an argument to the ''FM_MTU_LIST'' switch
 * attribute.
 *                                                  \lb\lb
 *  The MTU is checked only for IP frames and the 
 *  check is made against the packet length found 
 *  in the IP header, not the actual frame length.
 **************************************************/
typedef struct _fm_mtuEntry
{
    /** Index number of the MTU entry. */
    fm_uint index;

    /** MTU size in bytes.
     *  Maximum value is 0x3fff for most devices.
     *  FM6000 accepts a maximum value of 0xffff. */
    fm_uint32 mtu;

} fm_mtuEntry;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_LAG_MODE'' switch attribute.
 **************************************************/
typedef enum
{
    /** Use static link aggregation groups. LACP frames will be discarded. */
    FM_MODE_STATIC,

    /** Use dynamic link aggregation groups. LACP frames will be sent to
     *  the application for processing. */
    FM_MODE_DYNAMIC

} fm_lagMode;


/**************************************************/
/** \ingroup typeStruct
 *
 *  Structure to specify PAUSE pacing time.  Used 
 *  as an argument to ''fmSetPortAttribute'' and 
 *  ''fmGetPortAttribute'' for the port attribute
 *  ''FM_PORT_PAUSE_PACING_TIME''.
 **************************************************/
typedef struct _fm_pausePacingTime
{
    /** The PAUSE class */
    fm_uint     pauseClass;

    /** The time (in nanoseconds) that the port should remain in the PAUSE off
     *  state.  This value should be less than the PAUSE resend interval
     *  (''FM_PORT_TX_PAUSE_RESEND_TIME''). */
    fm_uint32   timeNs;
    
} fm_pausePacingTime;


/**************************************************/
/** \ingroup typeStruct
 *
 *  Structure to specify a policer bank with its
 *  associated drop mask for the switch attribute
 *  ''FM_POLICER_BANK_DROP_MASK''.
 **************************************************/
typedef struct _fm_policerBankDropMask
{
    /** The policer bank number created by ''fmCreatePolicer''.  */
    fm_int bankIndex; 
   
    /** Logical port mask specifying the destination ports on which frames are
     *  to be dropped. */
    fm_bitArray dropMask;

} fm_policerBankDropMask;


/**************************************************
 * Flag bits used for fm_ethMode.
 **************************************************/
#define FM_ETH_MODE_ENABLED_BIT_MASK     0x010000
#define FM_ETH_MODE_4_LANE_BIT_MASK      0x020000
#define FM_ETH_MODE_40G_BIT_MASK         0x040000
#define FM_ETH_MODE_100G_BIT_MASK        0x080000
#define FM_ETH_MODE_MULTI_LANE_MASK      ( FM_ETH_MODE_4_LANE_BIT_MASK )


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  ''FM_PORT_ETHERNET_INTERFACE_MODE'' port 
 *  attribute, which may be set per-MAC. See the
 *  description of ''FM_PORT_ETHERNET_INTERFACE_MODE''
 *  for a discussion of the constraints imposed
 *  on the selection of each of these values.
 **************************************************/
 
/**************************************************
 * Note that values are grouped according to 
 * whether they are a 1-lane, 4-lane or 40G
 * mode and the first value in each group is
 * initialized with an appropriate bit mask to
 * make it simpler for the code to classify each
 * mode.
 **************************************************/
 
typedef enum
{
    /** Port is disabled on the specified MAC. No lanes will be used.
     *  A port must be put in this state when another port sharing the
     *  same MAC is using a 4-lane mode. This is the default value 
     *  for the ''FM_PORT_ETHERNET_INTERFACE_MODE'' attribute. */
    FM_ETH_MODE_DISABLED = 0,

    /**************************************************
     * Non-40G, 1-lane modes
     **************************************************/
     
    /** SGMII: 1G, 1 lane, 8b/10b encoding. */
    FM_ETH_MODE_SGMII = FM_ETH_MODE_ENABLED_BIT_MASK,

    /** 1000BASE-X: 1G, 1 lane, 8b/10b encoding. */
    FM_ETH_MODE_1000BASE_X,

    /** 1000BASE-KX: 1G, 1 lane, 8b/10b encoding. */
    FM_ETH_MODE_1000BASE_KX,

    /** 2500BASE-X: 1G, 1 lane, 8b/10b encoding.
     *  This is experimental and to be used for internal purposes only. */
    FM_ETH_MODE_2500BASE_X,

    /** 6GBASE-KR: 6G, 1 lane, 64b/66b encoding.
     *  This is experimental and to be used for internal purposes only. */
    FM_ETH_MODE_6GBASE_KR,

    /** 6GBASE-CR: 6G, 1 lane, 64b/66b encoding. 
     *  This is experimental and to be used for internal purposes only. */
    FM_ETH_MODE_6GBASE_CR,

    /** 10GBASE-KR: 10G, 1 lane, 64b/66b encoding.
     *  This mode is read-only, i.e., it can be set only through Clause-73
     *  autonegotiation. */
    FM_ETH_MODE_10GBASE_KR,

    /** 10GBASE-CR (SFP+): 10G, 1 lane, 64b/66b encoding. */
    FM_ETH_MODE_10GBASE_CR,
    
    /** 10GBASE-SR (SFP+, SFI): 10G, 1 lane, 64b/66b encoding. */
    FM_ETH_MODE_10GBASE_SR,

    /** 25GBASE-SR (SFP+, SFI): 25G, 1 lane, 64/66b encoding. */
    FM_ETH_MODE_25GBASE_SR,

    /** 25GBASE-KR: 25G, 1 lane, 64/66b encoding.
     *  This mode is read-only, i.e., it can be set only through Clause-73
     *  autonegotiation. */
    FM_ETH_MODE_25GBASE_KR
,
    /** 25GBASE-CR: 25G, 1 lane, 64/66b encoding.
     *  This mode is read-only, i.e., it can be set only through Clause-73
     *  autonegotiation. */
    FM_ETH_MODE_25GBASE_CR,

    /** AN-73: Auto-negotiation Clause 73. */
    FM_ETH_MODE_AN_73,


    /**************************************************
     * Non-40G, 4-lane modes
     **************************************************/
    
    /** XAUI: 10G, 4 lanes, 8b/10b encoding.
     *                                                                  \lb\lb
     *  Note: After configuring this mode on the second port (P1) of the
     *  set of four ports sharing a MAC, before setting the port to another 
     *  mode, you must first set the mode to ''FM_ETH_MODE_DISABLED''. */
    FM_ETH_MODE_XAUI = (FM_ETH_MODE_4_LANE_BIT_MASK |
                        FM_ETH_MODE_ENABLED_BIT_MASK),

    /** 10GBASE-KX4: 10G, 4 lanes, 8b/10b encoding. */
    FM_ETH_MODE_10GBASE_KX4,

    /** 10GBASE-CX4: 10G, 4 lanes, 8b/10b encoding. */
    FM_ETH_MODE_10GBASE_CX4,

    /**************************************************
     * 40G, 4-lane modes (treat 24GBASE as 40G)
     **************************************************/

    /** 24GBASE-KR4: 24G, 4 lanes, 64b/66b encoding.
     * This is experimental and to be used for internal purposes only. */
    FM_ETH_MODE_24GBASE_KR4 = (FM_ETH_MODE_40G_BIT_MASK    |
                               FM_ETH_MODE_4_LANE_BIT_MASK |
                               FM_ETH_MODE_ENABLED_BIT_MASK), 
    
    /** 24GBASE-CR4: 24G, 4 lanes, 64b/66b encoding.
     * This is experimental and to be used for internal purposes only. */
    FM_ETH_MODE_24GBASE_CR4,

    /** 40GBASE-KR4: 40G, 4 lane, 64b/66b encoding.
     *  This mode is read-only, i.e., it can be set only through Clause-73
     *  autonegotiation. */
    FM_ETH_MODE_40GBASE_KR4,

    /** XLAUI: 40G, 4 lane, 64b/66b encoding. */
    FM_ETH_MODE_XLAUI,

    /** 40GBASE-CR4 (QSFP 5M Direct Attach): 40G, 4 lane, 64b/66b encoding.
     *  This mode is read-only, i.e., it can be set only through Clause-73
     *  autonegotiation. */
    FM_ETH_MODE_40GBASE_CR4,

    /** 40GBASE-SR4 (QSFP PMD Service Interface): 40G, 4 lane, 64b/66b 
     *  encoding. */
    FM_ETH_MODE_40GBASE_SR4,

    /** 100GBASE-SR4 (QSFP PMD Service Interface): 100G, 4 lane, 64b/66b
     *  encoding. */
    FM_ETH_MODE_100GBASE_SR4 = ( FM_ETH_MODE_100G_BIT_MASK    |
                                 FM_ETH_MODE_4_LANE_BIT_MASK  |
                                 FM_ETH_MODE_ENABLED_BIT_MASK ), 

    /** 40GBASE-CR4: 100G, 4 lanes, 64b/66b encoding. This mode is read-only,
     *  i.e., it can be set only through Clause-73 autonegotiation. */
    FM_ETH_MODE_100GBASE_CR4,


    /** 40GBASE-KR4: 100G, 4 lanes, 64b/66b encoding. This mode is read-only,
     *  i.e., it can be set only through Clause-73 autonegotiation. */
    FM_ETH_MODE_100GBASE_KR4

} fm_ethMode;


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  PCI Express Endpoint configuration mode
 *  port attribute (''FM_PORT_PEP_MODE'') 
 **************************************************/

typedef enum
{
    /** PEP unused */
    FM_PORT_PEP_MODE_DISABLED = 0,

    /** 1x1 GEN3 SerDes */
    FM_PORT_PEP_MODE_1X1,

    /** 2x4 GEN3 SerDes */
    FM_PORT_PEP_MODE_2X4,

    /** 1x8 GEN3 SerDes */
    FM_PORT_PEP_MODE_1X8,

    /** UNPUBLISHED: For internal use only. */
    FM_PORT_PEP_MODE_MAX

} fm_pepMode;


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  PCI Express link width port attribute
 *  (''FM_PORT_PCIE_MODE'').
 **************************************************/

typedef enum
{
    /** LTSSM is in a state where link width is not known yet. */
    FM_PORT_PCIE_MODE_DISABLED = 0,

    /** Negotiated link width is x1. */
    FM_PORT_PCIE_MODE_X1,

    /** Negotiated link width is x2. */
    FM_PORT_PCIE_MODE_X2,

    /** Negotiated link width is x4. */
    FM_PORT_PCIE_MODE_X4,

    /** Negotiated link width is x8. */
    FM_PORT_PCIE_MODE_X8,

    /** Negotiated link width is x16 (currently unsupported). */
    FM_PORT_PCIE_MODE_X16,

    /** Negotiated link width is x32 (currently unsupported). */
    FM_PORT_PCIE_MODE_X32,

    /** UNPUBLISHED: For internal use only. */
    FM_PORT_PCIE_MODE_MAX

} fm_pcieMode;



/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  PCI Express clock speed (per lane) port attribute
 *  (''FM_PORT_PCIE_SPEED'').
 **************************************************/

typedef enum
{
    /** LTSSM is in a state where clock speed is not known yet. */
    FM_PORT_PCIE_SPEED_UNKNOWN = 0,

    /** Base clock speed is 2.5 GHz (PCIe Gen1). */
    FM_PORT_PCIE_SPEED_2500,

    /** Base clock speed is 5 GHz (PCIe Gen2). */
    FM_PORT_PCIE_SPEED_5000,

    /** Base clock speed is 8 GHz (PCIe Gen3). */
    FM_PORT_PCIE_SPEED_8000,

} fm_pcieSpeed;



/**************************************************/
/** \ingroup typeStruct
 *  For the ''FM_PORT_AUTONEG_NEXTPAGES'' and
 *  ''FM_PORT_AUTONEG_PARTNER_NEXTPAGES'' port 
 *  attributes, this structure provides storage for 
 *  auto-negotiation next pages. 
 **************************************************/
typedef struct _fm_anNextPages
{
    /** Number of next pages. */
    fm_int numPages;

    /** Pointer to next pages for Auto-negotiation
     *  Clause 37 and Clause 73. Clause 37 and SGMII use
     *  only the low-order 32 bits of each next page. */
    fm_uint64 *nextPages;

} fm_anNextPages;


/**************************************************
 * For FM_ROUTING_HASH
 *
 * Note: These bit masks are defined to map to the
 * FM4000 hardware.
 **************************************************/
#define FM_ROUTING_HASH_SYM         0x1
#define FM_ROUTING_HASH_SIP         0x2
#define FM_ROUTING_HASH_DIP         0x4
#define FM_ROUTING_HASH_PROT        0x8
#define FM_ROUTING_HASH_TCP         0x10
#define FM_ROUTING_HASH_UDP         0x20
#define FM_ROUTING_HASH_PROT1       0x40
#define FM_ROUTING_HASH_PROT2       0x80
#define FM_ROUTING_HASH_L4SRC       0x100
#define FM_ROUTING_HASH_L4DST       0x200


/****************************************************************************/
/** \ingroup constVlanAttr
 *
 * VLAN Attributes, used as an argument to ''fmSetVlanAttribute'' and
 * ''fmGetVlanAttribute''.
 *                                                                          \lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 ****************************************************************************/
enum _fm_vlanAttr
{
    /** Type fm_bool: Reflection on a VLAN: FM_ENABLED or FM_DISABLED (default).
     *  If enabled, then a frame may be sent out the VLAN it came in on.
     *                                                                  \lb\lb
     *  On FM2000, FM3000 and FM4000 devices, this attribute affects only
     *  switched frames, not routed, and reflection is subject to the ingress
     *  port's ''FM_PORT_MASK'' attribute.
     *                                                                  \lb\lb
     *  On FM6000 devices, the effect of setting this attribute is identical
     *  to setting the FM_VLAN_ROUTING_REFLECT_UNICAST,
     *  FM_VLAN_ROUTING_REFLECT_MULTICAST and FM_VLAN_SWITCHING_REFLECT
     *  attributes individually to the same value. See those attributes for
     *  more information.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_VLAN_REFLECT = 0,

    /** Type fm_bool: Routing on this VLAN: FM_ENABLED or FM_DISABLED
     *  (default). A VLAN must have this attribute enabled in order for
     *  traffic ingressing on the VLAN to be routed. When this attribute is
     *  disabled, ingressing traffic will only be switched at layer 2.
     *                                                                  \lb\lb
     *  Note: When a VLAN is assigned to a router interface (by calling
     *  ''fmSetInterfaceAttribute'' for the ''FM_INTERFACE_VLAN'' attribute),
     *  the assigned VLAN should normally have this attribute enabled. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_VLAN_ROUTABLE,

    /** Type fm_int: Specifies which MTU value to use for this 
     *  VLAN. The value of this attribute is the MTU index number. The actual
     *  MTU value associated with that index is set with the ''FM_MTU_LIST'' 
     *  switch attribute. 
     *                                                                  \lb\lb
     *  The MTU is checked only for IP frames and the check is made against
     *  the packet length found in the IP header, not the actual frame
     *  length. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_VLAN_MTU,

    /** Type fm_bool: Specifies whether IGMP frames are trapped on this
     *  VLAN.  Set to FM_ENABLED to enable IGMP trapping, and FM_DISABLED
     *  (default) to disable trapping.
     *                                                                  \lb\lb
     *  Note: if enabling this attribute, you must also set the ''FM_PORT_PARSER''
     *  port attribute to an appropriate value. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_VLAN_IGMP_TRAPPING,

    /** Type fm_bool: Specifies whether FID2 must be part of the MAC lookup
     *  on this VLAN.  Set to FM_ENABLED to enable FID2 matching, and
     *  FM_DISABLED (default) to disable it.
     *
     *  \chips  FM6000 */
    FM_VLAN_FID2_IVL,

    /** Type: fm_bool. Enables trapping of ARP frames. This attribute has
     *  been deprecated in favor of the FM_VLAN_ARP_LOGGING attribute
     *
     * \chips FM6000 */
    FM_VLAN_ARP_TRAPPING,

    /** Type fm_bool: Specifies whether unknown unicast frames are flooded
     *  on this VLAN.  Set to FM_ENABLED to enable flooding, and FM_DISABLED
     *  (default) to disable flooding.
     *
     *  \chips  FM6000 */
    FM_VLAN_UCAST_FLOODING,

    /** UNPUBLISHED: For internal use only. */
    FM_VLAN_ATTRIBUTE_MAX

};  /* end enum _fm_vlanAttr */


/****************************************************************************/
/** \ingroup intConstVlanPortAttr
 *
 * VLAN Port Attributes, used as an argument to ''fmGetVlanPortAttribute''.
 *                                                                          \lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 * Note that these attributes were implemented to facilitate software testing,
 * and are not currently part of the published API.
 ****************************************************************************/
enum _fm_vlanPortAttr
{
    /** Type fm_bool: Indicates whether the port is a member of the VLAN.
     *  
     *  \chips FM3000, FM4000, FM6000 */
    FM_VLAN_PORT_MEMBERSHIP = 0,

    /** Type fm_bool: Specifies the tagging state of the port in the VLAN.
     *  
     *  \chips FM3000, FM4000, FM6000 */
    FM_VLAN_PORT_TAG1,

    /** Type fm_bool: Specifies the tagging state of the inner VLAN.
     *  Will be FALSE if the switch only supports one level of tagging.
     *  
     *  \chips FM3000, FM4000, FM6000 */
    FM_VLAN_PORT_TAG2,

    /** Type fm_int: The spanning tree forwarding state of the port in the
     *  VLAN.
     *  
     *  \chips FM3000, FM4000, FM6000 */
    FM_VLAN_PORT_STP_STATE,

    /** UNPUBLISHED: For internal use only. */
    FM_VLAN_PORT_ATTRIBUTE_MAX

};  /* end enum _fm_vlanPortAttr */


/**************************************************
 * For FM_ACL_MODE
 **************************************************/
#define FM_ACL_MODE_STANDARD          0
#define FM_ACL_MODE_INCREMENTAL       1


/****************************************************************************/
/** \ingroup constACLAttr
 *
 *  ACL Attributes, used as an argument to ''fmSetACLAttribute'' and 
 *  ''fmGetACLAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_aclAttr
{
    /** Type fm_int: Indicates which mode to use for this ACL. Value is:
     *                                                                  \lb\lb
     *  FM_ACL_MODE_STANDARD (default): ACL cannot be updated incrementally,
     *  but can only be updated by calling ''fmCompileACL'' and ''fmApplyACL''.
     *                                                                  \lb\lb
     *  FM_ACL_MODE_INCREMENTAL: ACL can be updated incrementally without 
     *  requiring a call to ''fmCompileACL'' and ''fmApplyACL''. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ACL_MODE = 0,

    /** Type ''fm_aclCondition'': Indicates which conditions are supported 
     *  for an ACL whose ''FM_ACL_MODE'' attribute is set to 
     *  FM_ACL_MODE_INCREMENTAL. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ACL_INCR_SUP_COND,

    /** Type ''fm_aclValue'': Indicates the mask associated with each 
     *  condition indicated by the ''FM_ACL_INCR_SUP_COND'' attribute. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ACL_INCR_SUP_COND_MASK,

    /** Type fm_uint32: Indicates the maximum number of actions to 
     *  perfom on an ACL whose ''FM_ACL_MODE'' attribute is set to 
     *  FM_ACL_MODE_INCREMENTAL. 
     *
     *  \chips  FM3000, FM4000 */
    FM_ACL_INCR_NUM_ACTIONS,

    /** Type ''fm_aclTable'': Indicates which FFU or BST table to use for
     *  this particular ACL.
     *
     *  \chips  FM6000 */
    FM_ACL_TABLE_SELECTION,

    /** Type fm_bool: When a rule is deleted from an ACL, if the condition
     *  used by that rule required additional hardware key resources (FFU
     *  slices) that will now no longer be used by any other rule in the ACL,
     *  the ''fmCompileACL'' and fmApplyACL'' operations will automatically
     *  reduce the number of hardware resources in use by the ACL. However,
     *  if rules are added to and deleted from the ACL frequently, reallocating
     *  and freeing the hardware resources can take some time. When this
     *  attribute is enabled, hardware resources are not freed when the last
     *  rule that uses them are deleted. The benefit is an improvement in 
     *  performance. The cost is that those hardware resources are not made
     *  available for other ACLs to use. Possible values for this
     *  attribute are FM_ENABLED or FM_DISABLED (default).
     *
     *  \chips  FM6000, FM10000 */
    FM_ACL_KEEP_UNUSED_KEYS,

    /** Type fm_bool: Specifies whether the BST 4-bit top key can be used by
     *  the ACL compiler for this particular ACL, if located in the BST
     *  table (see ''FM_ACL_TABLE_SELECTION''). Disabling forces the use of
     *  BST 32-bit standard keys only. Possible values for this attribute
     *  are FM_ENABLED (default) or FM_DISABLED.
     *
     *  \chips  FM6000 */
    FM_ACL_TOP_KEY_SUPPORT,

    /** Type ''fm_aclSliceUsage'': (Read-only) Provides information on the
     *  FFU TCAM/BST slices used by the ACL.
     *
     *  \chips  FM6000 */
    FM_ACL_SLICE_USAGE,

    /** Type fm_int: Indicates which instance this ACL is assigned to. Possible
     *  values for this attribute are any positive IDs. All the ACLs that
     *  belongs to the same instance must have mutually exclusive scenario
     *  and consecutive ACL ID. In that case, a single cascade of FFU slice
     *  will be reserved to match on all the rules of all the ACLs that
     *  belongs to the same instance up to the point where the total number
     *  of rules of this instance exceed the FFU slice capacity. In that case,
     *  the instance is automatically broken in multiple ACLs. Other possible
     *  value for this attribute is ''FM_ACL_NO_INSTANCE'' (default) to
     *  never share FFU resources with other ACLs. Non Disruptive ACL Compiler
     *  does not support that feature and would refuse to compile if set.
     *
     *  \chips  FM10000 */
    FM_ACL_INSTANCE,

    /** UNPUBLISHED: For internal use only. */
    FM_ACL_ATTRIBUTE_MAX

};  /* end enum _fm_aclAttr */



/****************************************************************************/
/** \ingroup constACLRuleAttr
 *
 *  ACL Rule Attributes, used as an argument to ''fmGetACLRuleAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_aclRuleAttr
{
    /** Type ''fm_aclRulePolicerInfo'': Indicates which policer banks and
     *  indexes within those banks are being used by a specific ACL Rule.
     *
     *  \chips  FM6000, FM10000 */
    FM_ACL_RULE_POLICER_INFO,

    /** Type ''fm_aclRuleSliceUsage'': (Read-only) Provides information
     *  about the FFU TCAM/BST slices and rows used by the ACL rule.
     *
     *  \chips  FM6000 */
    FM_ACL_RULE_SLICE_USAGE,

    /** UNPUBLISHED: For internal use only. */
    FM_ACL_RULE_ATTRIBUTE_MAX

};  /* end enum _fm_aclRuleAttr */


/****************************************************************************/
/** \ingroup constTriggerAttr
 *
 *  Trigger Attributes, used as an argument to ''fmSetTriggerAttribute'' and 
 *  ''fmGetTriggerAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_triggerAttr
{
    /** Type fm_uint64: The number of times a trigger has hit. To reset a
     *  trigger counter, a value of zero should be written to this
     *  attribute.
     *  
     *  The counter value is initialized to zero on trigger creation. */
    FM_TRIGGER_ATTR_COUNTER = 0,

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_ATTR_MAX,

};  /* end enum _fm_triggerAttr */


/****************************************************************************/
/** \ingroup constMATAttr
 *
 * MA Table Attributes, used as an argument to ''fmSetAddressTableAttribute''
 * and ''fmGetAddressTableAttribute''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 ****************************************************************************/
enum _fm_maTableAttr
{
    /** Type fm_uint32: The size of the MA Table in bytes (read-only). Used
     *  to determine the number of bytes to allocate for the array passed to
     *  the ''fmGetAddressTableExt'' function.
     *  
     *  For physical switches, the API returns a value based on the capacity
     *  of the hardware MA Table.
     *  
     *  For SWAG switches, the API returns a value based on the number of
     *  entries in the SWAG MA Table, which may exceed the capacity of a
     *  single switch. If the SWAG MA Table is empty, its size will be zero.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MAC_TABLE_SIZE = 0,

    /** Type fm_uint32: The MA Table hash function produces a 16 bit value.
     *  The hash address is only 12 bits. This attribute specifies which 4 bits
     *  are excluded:                                                       
     *                                                                  \lb\lb
     *  FM_MAC_TABLE_HASH_POLYNOMIAL_0 to exclude bits 15:12 (default),     
     *                                                                  \lb\lb
     *  FM_MAC_TABLE_HASH_POLYNOMIAL_1 to exclude bits 11:8,                
     *                                                                  \lb\lb
     *  FM_MAC_TABLE_HASH_POLYNOMIAL_2 to exclude bits 7:4,                 
     *                                                                  \lb\lb
     *  FM_MAC_TABLE_HASH_POLYNOMIAL_3 to exclude bits 3:0. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_MAC_TABLE_HASH_POLYNOMIAL,

    /** (Deprecated) See ''FM_MAC_TABLE_HASH_POLYNOMIAL'', which functions
     *  identically to this attribute. */
    FM_HASH_POLYNOMIAL,

    /** Type fm_uint32: This attribute indicates when source address lookups
     *  should be performed. Value is:                                      
     *                                                                  \lb\lb
     *  FM_SA_LOOKUP_ALWAYS to do a source address lookup on every frame,   
     *                                                                  \lb\lb
     *  FM_SA_LOOKUP_BEST_EFFORT (default) to do a source address lookup
     *  only while the frame processor is ahead of the requests for
     *  destination address lookups. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_MAC_TABLE_SA_LOOKUP_MODE,

    /** (Deprecated) See ''FM_MAC_TABLE_SA_LOOKUP_MODE'', which functions
     *  identically to this attribute. */
    FM_SA_LOOKUP_MODE,

    /** Type fm_uint32: Minimum number of seconds for MA Table aging. A value
     *  of zero disables aging. Aging is disabled by default.
     *                                                                  \lb\lb
     *  The hardware performs aging of entries by sweeping through the MA 
     *  Table one entry at a time, changing "young" entries to "old" and
     *  deleting "old" entries. This attribute sets the number of seconds
     *  to take to sweep the entire table one time. Since each entry must
     *  be processed by the sweeper twice to age it out, the actual amount
     *  of time a newly learned entry will persist in the table is dependent
     *  on the position of the sweeper at the time the entry is first learned. 
     *  If the sweeper has just passed by the new entry's position when it is 
     *  learned, that entry will persist for nearly two times the attribute 
     *  value. If the sweeper is positioned just before the entry at the time 
     *  it is learned, the entry will be immediately transitioned from "young" 
     *  to "old" and so will persist in the table only slightly longer than 
     *  the attribute value. The average persistence will be 1.5 times the 
     *  attribute value. 
     *                                                                  \lb\lb
     *  On the FM2000, the maximum value is a function of the EBI
     *  (CPU interface) clock speed:                                        
     *                                                                  \lb\lb
     *  25MHz => max: 1,400,000 seconds.                                    \lb
     *  33MHz => max: 1,060,000 seconds.                                    \lb
     *  50MHz => max: 700,000 seconds.                                      \lb
     *  66MHz => max: 530,000 seconds.
     *                                                                  \lb\lb
     *  On FM3000 and FM4000 devices, the maximum value is 94,000 seconds. 
     *                                                                  \lb\lb
     *  On FM6000 devices, the maximum value is 2,251,799 seconds. 
     *                                                                  \lb\lb
     *  On FM10000 devices, MA Table aging is performed in software. The
     *  maximum value for this attribute is 4,294,967 seconds.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MAC_TABLE_ADDRESS_AGING_TIME,

    /** (Deprecated). See ''FM_MAC_TABLE_ADDRESS_AGING_TIME'', which
     *  functions identically to this attribute. */
    FM_ADDRESS_AGING_TIME,

    /** Type fm_uint32: Table event mode. Chooses whether
     *  learning events are guaranteed or not.
     *                                                                  \lb\lb
     *  FM_MAC_TABLE_EVENT_MODE_GUARANTEED ensures that no table events
     *  are missed due to overflow by forcing the hardware to not learn
     *  entries unless it can push them into the update queue.
     *                                                                  \lb\lb
     *  FM_MAC_TABLE_EVENT_MODE_BEST_EFFORT (default) allows the hardware to
     *  overflow the table event queue, causing the API to rescan the entire
     *  table.
     *                                                                  \lb\lb
     *  Note that FM2000 only supports the best effort mode of operation.
     *  FM6000 devices support guaranteed event reporting without affecting 
     *  the learning rate.
     *
     *  \chips  FM3000, FM4000 */
    FM_MAC_TABLE_EVENT_MODE,

    /** Type fm_uint32: Aging events threshold.  Set the maximum
     *  threshold for enqueuing aging events in the hardware FIFO, valid values
     *  are from 0 to 511 (default).  If the number of queue entries exceeds
     *  this value, then no aging events are generated until the queue
     *  occupancy drops below this level.
     *                                                                  \lb\lb
     *  Note: When ''FM_MAC_TABLE_EVENT_MODE'' is set to
     *  FM_MAC_TABLE_EVENT_MODE_GUARANTEED, this attribute should provide
     *  a "skidpad" in the hardware FIFO to allow for aging events in flight to
     *  have a place to land in the FIFO after the threshold has been
     *  exceeded. 487 is the maximum value that still provides an adequate
     *  skidpad. 
     *
     *  \chips  FM3000, FM4000 */
    FM_MAC_TABLE_EVENT_AGING_THRESHOLD,

    /** Type fm_uint32: Learning events threshold.  Set the
     *  maximum threshold for enqueuing learning events in the hardware FIFO,
     *  valid values are from 0 to 511 (default).  If the number of queue
     *  entries exceeds this value, then no learning events are generated
     *  until the queue occupancy drops below this level.
     *                                                                  \lb\lb
     *  Note: When ''FM_MAC_TABLE_EVENT_MODE'' is set to
     *  FM_MAC_TABLE_EVENT_MODE_GUARANTEED, this attribute should provide
     *  a "skidpad" in the hardware FIFO to allow for learning events in
     *  flight to have a place to land in the FIFO after the threshold has been
     *  exceeded. 487 is the maximum value that still provides an adequate
     *  skidpad. 
     *
     *  \chips  FM3000, FM4000 */
    FM_MAC_TABLE_EVENT_LEARNING_THRESHOLD,

    /** Type fm_uint32: Security violation events threshold.
     *  Set the maximum threshold for enqueuing security violation events
     *  in the hardware FIFO, valid values are from 0 to 511 (default).
     *  If the number of queue entries exceeds this value, then no security
     *  violation events are generated until the queue occupancy drops below
     *  this level.
     *                                                                  \lb\lb
     *  Note: When ''FM_MAC_TABLE_EVENT_MODE'' is set to
     *  FM_MAC_TABLE_EVENT_MODE_GUARANTEED, there is no "skidpad" associated
     *  with this threshold. It is recommended that this threshold be set no
     *  higher than 255 to leave at least half the FIFO available for learning
     *  events. 
     *
     *  \chips  FM3000, FM4000 */
    FM_MAC_TABLE_EVENT_SEC_EVENT_THRESHOLD,

    /** Type fm_bool: Software learning: FM_ENABLED or FM_DISABLED
     *  (default). The learning rate will be limited to what the software can
     *  support. 
     *
     *  \chips  FM3000, FM4000 */
    FM_MAC_TABLE_SOFT_LEARNING,

    /** (Deprecated) Legacy synonym for ''FM_MAC_TABLE_SOFT_LEARNING''. */
    FM_MAC_SOFT_LEARNING = FM_MAC_TABLE_SOFT_LEARNING,

    /** Type fm_bool: Software aging: FM_ENABLED or FM_DISABLED
     *  (default). The aging rate will be limited to what the software can
     *  support. 
     *
     *  \chips  FM3000, FM4000 */
    FM_MAC_TABLE_SOFT_AGING,

    /** (Deprecated) Legacy synonym for ''FM_MAC_TABLE_SOFT_AGING''. */
    FM_MAC_SOFT_AGING = FM_MAC_TABLE_SOFT_AGING,

    /** Type fm_uint32: Dictates the maximum rate at which hardware will sweep
     *  the MAC Table for identifying newly learned and aged entries and
     *  reporting them to software. The period is specified in microseconds
     *  and corresponds to the time spent to sweep the entire table once. 
     *                                                                  \lb\lb
     *  A sufficiently large value is required to ensure correct operation
     *  of the MAC Table. A value is sufficiently large if it is longer
     *  than the longest time possible for the switch to receive an entire
     *  Ethernet packet on any port (including the CPU port) that has learning 
     *  enabled (''FM_PORT_LEARNING''). Packet receive time is dependent on 
     *  the maximum frame length (''FM_PORT_MAX_FRAME_SIZE'') and the port 
     *  speed (''FM_PORT_SPEED''). Some examples (all values are approximate):                                            \lb
     *                                                                      \lb
     *  A 1500 byte frame on a 10G port requires 1.2 usec.                  \lb
     *  A 10,000 byte frame on a 10G port requires 8 usec.                  \lb
     *  A 10,000 byte frame on a 1G port requires 80 usec.                  \lb
     *  A 10,000 byte frame on a 100M port requires 800 usec.               \lb
     *  A 10,000 byte frame on a 10M port requires 8 msec.                  \lb
     *                                                                      \lb
     *  The PCIe CPU port may be considered the same as a 1G port. The EBI
     *  CPU port is unbounded and is dependent on processor speed and the
     *  task-switching characteristics of the operating system (unless an FPGA
     *  is used to pass frames on the EBI).
     *                                                                  \lb\lb
     *  Failure to select a sufficiently large value can result in any of 
     *  the following errant behaviors:
     *                                                                  \lb\lb
     *  (a) Learning events reported for MAC addresses that do not actually
     *  appear in the MAC Table. This behavior can occur if a packet with 
     *  SMAC A is received followed by a packet with SMAC B, where A and B 
     *  hash to the same bin in the MAC Table and the head of packet B 
     *  arrives before the tail of packet A. A sufficiently large attribute 
     *  value prevents a learn event for SMAC A to be reported before it 
     *  is overwritten by SMAC B.
     *                                                                  \lb\lb
     *  (b) Entries written to the MAC table by software (with a call to
     *  ''fmAddAddress'') immediately disappear. This behavior can occur 
     *  when software selects an empty location in the MAC table to be 
     *  written just as the same position is selected by the hardware for 
     *  an SMAC in an arriving packet and the tail of the packet is not 
     *  received until after software writes the entry. A sufficiently 
     *  large attribute value prevents the received packet from overwriting 
     *  the software-written entry.
     *                                                                  \lb\lb
     *  The value set for this attribute will be rounded up to the nearest 
     *  granularity provided by the hardware. Note that the hardware is 
     *  capable of a minimum period of 500us in full table mode and 250us
     *  in split mode (''FM_MAC_TABLE_SPLIT_MODE''). 
     *                                                                  \lb\lb
     *  The default value is 1000 usec, which accommodates jumbo frames on 
     *  ports down to 100M. Applications using 10M ports or sending learned 
     *  frames from a CPU on the EBI port should increase the value of this 
     *  attribute accordingly. 
     *                                                                  \lb\lb
     *  Setting a value of zero disables sweeping entirely which offers a
     *  modest power savings for applications that do not require dynamic 
     *  MAC Table support. 
     *
     *  \chips  FM6000 */
    FM_MAC_TABLE_SWEEP_PERIOD,

    /** (Deprecated) Legacy synonym for ''FM_MAC_TABLE_SWEEP_PERIOD''. */
    FM_MAC_SWEEP_PERIOD = FM_MAC_TABLE_SWEEP_PERIOD,

    /** Type fm_bool: Indicates whether the MAC Table should operate in split
     *  mode: FM_ENABLED or FM_DISABLED (default). Normally the MAC Table 
     *  provides 64K entries and the switch is fully provisioned to perform 
     *  a DMAC lookup on every received frame. SMAC lookups (for learning 
     *  and refresh) are statistically likely to succeed, but are not 
     *  guaranteed. Split mode guarantees both DMAC and SMAC lookups on every 
     *  frame, but at the cost of reducing the MAC Table size to 32K entries.
     *                                                                  \lb\lb
     *  Note: This mode is normally set when the switch is idle. The MAC Table
     *  should be empty, and no learning should be taking place. 
     *
     *  \chips  FM6000 */
    FM_MAC_TABLE_SPLIT_MODE,

    /** Type fm_uint32: Specifies the action to be taken in the event of a
     *  MAC Security violation (a secure source MAC address is received with
     *  a GLORT that does not match the entry in the MA table).
     *                                                                  \lb\lb
     *  See ''fm_macSecurityAction'' for possible values.
     * 
     *  \chips   FM10000 */
    FM_MAC_TABLE_SECURITY_ACTION,

    /** UNPUBLISHED: For internal use only. */
    FM_MAC_TABLE_ATTRIBUTE_MAX

};  /* end enum _fm_maTableAttr */


/**************************************************
 * For FM_MAC_TABLE_HASH_POLYNOMIAL
 **************************************************/
#define FM_MAC_TABLE_HASH_POLYNOMIAL_0       0
#define FM_MAC_TABLE_HASH_POLYNOMIAL_1       1
#define FM_MAC_TABLE_HASH_POLYNOMIAL_2       2
#define FM_MAC_TABLE_HASH_POLYNOMIAL_3       3

#define FM_HASH_POLYNOMIAL_0                 FM_MAC_TABLE_HASH_POLYNOMIAL_0
#define FM_HASH_POLYNOMIAL_1                 FM_MAC_TABLE_HASH_POLYNOMIAL_1
#define FM_HASH_POLYNOMIAL_2                 FM_MAC_TABLE_HASH_POLYNOMIAL_2
#define FM_HASH_POLYNOMIAL_3                 FM_MAC_TABLE_HASH_POLYNOMIAL_3


/**************************************************
 * For FM_MAC_TABLE_SA_LOOKUP_MODE
 **************************************************/
#define FM_SA_LOOKUP_ALWAYS                  0
#define FM_SA_LOOKUP_BEST_EFFORT             1


/**************************************************
 * For FM_MAC_TABLE_EVENT_MODE
 **************************************************/
#define FM_MAC_TABLE_EVENT_MODE_BEST_EFFORT  0
#define FM_MAC_TABLE_EVENT_MODE_GUARANTEED   1


/**************************************************/
/** \ingroup typeEnum 
 *  MAC Table security violation action. Specifies
 *  the action to be taken in the event of a MAC
 *  Security violation. Used at the value of the
 *  ''FM_MAC_TABLE_SECURITY_ACTION'' MAC Table 
 *  attribute. 
 **************************************************/
typedef enum
{
    /** The violating frame is silently dropped. */
    FM_MAC_SECURITY_ACTION_DROP,

    /** The violating frame is dropped, and the SMAC address is reported
     *  to the application, via a security violation event, for
     *  authentication. */
    FM_MAC_SECURITY_ACTION_EVENT,

    /** The violating frame is trapped to the application for
     *  authentication. */
    FM_MAC_SECURITY_ACTION_TRAP,

    /** UNPUBLISHED: For internal use only. */
    FM_MAC_SECURITY_ACTION_MAX

} fm_macSecurityAction;


/****************************************************************/
/** \ingroup constSystem
 *  Value used to disable mirror truncation at port level.
 ****************************************************************/
#define FM_PORT_DISABLE_TRUNCATION 252


/****************************************************************************/
/** \ingroup constPortAttr
 *
 * Port Attributes, used as an argument to ''fmSetPortAttribute'' and
 * ''fmGetPortAttribute''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 * For each attribute that is supported on the FM6000 or FM10000, the port
 * types to which that attribute is applicable will be listed. The possible
 * port types are:                                                      \lb
 * Ethernet                                                             \lb
 * PCIe                                                                 \lb
 * Tunneling Engine (TE)                                                \lb
 * Loopback                                                             \lb
 * LAG (see ''Link Aggregation (LAG) Management'' for further details).
 *                                                                      \lb\lb
 * If the attribute is applicable to a different set of port types on
 * FM6000 devices than for FM10000 devices, separate lists are provided 
 * for each device. When only one list of port types is provided, TE and 
 * Loopback port types apply to FM10000 only and should be ignored for 
 * FM6000 devices, as those port types are not supported on FM6000.
 *                                                                      \lb\lb
 * For FM6000 devices, some attributes are MAC or lane specific, for ports 
 * that have multiple MACs or lanes available. Attributes that are
 * MAC or lane specific are noted as such. If not otherwise indicated, an 
 * attribute may be assumed to apply to the port irrespective of MAC or lane.
 ****************************************************************************/
enum _fm_portAttr
{
    /** Type fm_int: Minimum frame size in bytes, ranging from 0 to 252 with
     *  a default value of 64. Note that a specified value will be rounded
     *  down to the nearest multiple of 4.
     *  
     *  Note: For ISL Tag enabled ports, the internal port configuration is
     *  adjusted by the number of bytes required for the ISL Tag.
     *  ''fmGetPortAttribute'' will not return the additional bytes required
     *  for the ISL Tag. 
     *
     *  \portType6K ETH, PCIE, LAG
     *  \portType10K ETH, PCIE:ro, TE:ro, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_MIN_FRAME_SIZE = 0,

    /** Type fm_int: Maximum frame size in bytes, ranging from 0 to 15360 for
     *  FM10000 and 0 to 15864 for all other chips. Default value is 1536 for
     *  all ports in FM6000, For FM10000, default value of Ethernet port is 
     *  1536 and PCIE port is 15358. Note that a specified value will be 
     *  rounded up to the nearest multiple of 4.
     *                                                                  \lb\lb
     *  Note: For ISL Tag enabled ports, the internal port configuration is
     *  adjusted by the number of bytes required for the ISL Tag.
     *  ''fmGetPortAttribute'' will not return the additional bytes required
     *  for the ISL Tag. 
     *
     *  \portType6K ETH, PCIE, LAG
     *  \portType10K ETH, PCIE:ro, TE:ro, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_MAX_FRAME_SIZE,

    /** (Not supported) 
     *  \chips  (None) */
    FM_PORT_TYPE,

    /** Type fm_uint32: Port security configuration. Value is:
     *                                                                  \lb\lb
     *  FM_PORT_SECURITY_OFF (default) to turn off all source address security
     *  checks,
     *                                                                  \lb\lb
     *  FM_PORT_SECURITY_SAV to consider an unknown source address to be a
     *  security violation,
     *                                                                  \lb\lb
     *  FM_PORT_SECURITY_SPV to consider a known source address on a port other
     *  than what it was learned on to be a security violation (non FM2000
     *  devices only),
     *                                                                  \lb\lb
     *  FM_PORT_SECURITY_SHV to consider an unknown source address or known
     *  source address previously learned on another port (station move) to be
     *  a security violation.
     *                                                                  \lb\lb
     *  Use of this attribute is deprecated in favor of using the
     *  ''fmSetPortSecurity'' API service, which will override the value of
     *  this attribute.
     *                                                                  \lb\lb
     *  Note: If enabling FM_PORT_SECURITY on a port, it may be desirable to
     *  disable ''FM_PORT_LEARNING''. Otherwise, the first frame will be 
     *  reported as a security violation, but the source address will be learned
     *  resulting in all subsequent frames from that source address to be
     *  forwarded normally without incurring further security violations.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_SECURITY,

    /** Type fm_uint32: The action to be taken if this port receives a frame
     *  with an unknown SMAC address.
     *                                                                  \lb\lb
     *  See ''fm_portSecurityAction'' for possible values. Defaults to
     *  ''FM_PORT_SECURITY_ACTION_NONE''.
     *  
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_SECURITY_ACTION,

    /** Type fm_uint32: Priority of frames that incur a security
     *  violation and are trapped to the CPU. Value is:
     *  FM_SECURITY_PRI_HIGH to remap to switch priority 15,
     *                                                                  \lb\lb
     *  FM_SECURITY_PRI_NORMAL (default) to leave the priority unchanged. 
     *
     *  \chips  FM2000 */
    FM_PORT_SECURITY_PRIORITY,

    /** Type fm_uint32: This attribute specifies what to do with
     *  frames that incur a security violation. Value is:
     *                                                                  \lb\lb
     *  FM_SECURITY_DISCARD (default) to discard the frame,
     *                                                                  \lb\lb
     *  FM_SECURITY_SEND2CPU to trap the frame to the CPU.
     *                                                                  \lb\lb
     *  Use of this attribute is deprecated in favor of using the
     *  ''fmSetPortSecurity'' API service, which will override the value of
     *  this attribute.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_SECURITY_TRAP,

    /** Type fm_bool: Learning of source addresses on this port: FM_ENABLED
     *  (default) or FM_DISABLED (default for TE ports).
     *                                                                  \lb\lb
     *  Note: the ''fmSetPortSecurity'' API service will override the value of
     *  this attribute. 
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_LEARNING,

    /** Type fm_uint32: VLAN1 tagging behavior for this port. Value is
     *  one of the options listed in ''Port Tagging Options''.
     *                                                                  \lb\lb
     *  For VLAN2, see ''FM_PORT_TAGGING2''.
     *
     *  \portType ETH, LAG
     *  \chips FM2000, FM4000, FM6000 */
    FM_PORT_TAGGING,

    /** Type fm_uint32: VLAN2 tagging behavior for this port. Value is
     *  one of the options listed in ''Port Tagging Options''.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TAGGING2,

    /** Type ''fm_portTaggingMode'': Selects the port tagging mode for this port.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_TAGGING_MODE,

    /** Type fm_uint32: The port's default VLAN (1 - 4095). The default value
     *  is 1. See the ''FM_PORT_TAGGING'' attribute for the various ways the
     *  default VLAN can affect a frame.
     *                                                                  \lb\lb
     *  For the FM2000 family, a port's default VLAN cannot be set to the
     *  switch's reserved VLAN (see the ''FM_RESERVED_VLAN'' switch attribute).
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DEF_VLAN,

    /**
     * Type: fm_uint32. The port's default second VLAN (1 - 4095). The
     * default value is 1.                                              
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     * \chips FM6000, FM10000 */
    FM_PORT_DEF_VLAN2,

    /** Type fm_uint32: The port's default priority (0 - 7). The default value
     *  is 0.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DEF_PRI,

    /**
     * Type: fm_uint32. The port's default second 802.1q priority (0 - 7).
     * The default value is 0.                                             
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     * \chips FM6000, FM10000 */
    FM_PORT_DEF_PRI2,

    /** Type fm_uint32: The port's default CFI (0 - 1).  The
     *  default value is 0.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DEF_CFI,

    /** Type fm_uint32: The port's default DSCP (0 - 63).
     *  Default value is 0.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_DEF_DSCP,

    /** Type fm_uint32: The port's default internal switch
     *  priority (0 - 15).  Default value is 0.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DEF_SWPRI,

    /** Type fm_uint32: The port's default ISL tag user bits value
     *  (0 - 255). The default value is 0.
     *  
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM6000, FM10000 */
    FM_PORT_DEF_ISL_USER,
    
    /** Type fm_bool: Whether the frame's DSCP field is replaced
     *  with the value indicated by the ''FM_PORT_DEF_DSCP'' attribute: 
     *  FM_ENABLED or FM_DISABLED (default).
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_REPLACE_DSCP,

    /** Type fm_bool: Dropping frames that incur an ingress VLAN boundary
     *  violation: FM_ENABLED or FM_DISABLED.
     *
     *  For FM10000 switch family, the default value is FM_ENABLED. For all
     *  other switch families, the default value is FM_DISABLED.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DROP_BV,

    /** Type fm_bool: Dropping of ISL tagged management frames on 
     *  ingress: FM_ENABLED (default) or FM_DISABLED.
     *  
     *  \chips  FM3000, FM4000 */
    FM_PORT_DROP_MANAGEMENT_ISL,

    /** Type fm_bool: Dropping of untagged frames on ingress: FM_ENABLED or
     *  FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Note that if enabled on FM2000, FM4000, FM6000 and the 
     *  ''FM_PORT_TAGGING'' attribute is set to FM_TAG_ADD or 
     *  FM_TAG_ADD_NO_DEFAULT, then this filter will result in a discard if 
     *  the incoming frame does not have its first level tag. That is, the 
     *  ethertype does not equal the VLAN. 
     *                                                                  \lb\lb
     *  On FM2000, FM3000, FM4000 and FM10000 devices, if Ethertype = VLAN but 
     *  the VLAN ID is 0 (priority tagged frame), the frame is considered VLAN 
     *  untagged and will be dropped per this attribute.
     *                                                                  \lb\lb
     *  On FM6000 devices, dropping of priority tagged frames
     *  (VLAN ID is 0) is controlled by a separate attribute, 
     *  FM_PORT_DROP_PRIORITY_TAGGED.
     *                                                                  \lb\lb
     *  On FM10000 devices, this attribute is not applicable if 
     *  port attribute ''FM_PORT_ISL_TAG_FORMAT'' is set to 
     *  ''FM_ISL_TAG_F56''. 
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DROP_UNTAGGED,

    /** Type fm_bool: Dropping of tagged frames on ingress: FM_ENABLED or
     *  FM_DISABLED (default). 
     *                                                                  \lb\lb
     *  On FM2000, FM3000, FM4000 and FM10000 devices, if Ethertype = VLAN but 
     *  the VLAN ID is 0 (priority tagged frame), the frame is considered VLAN 
     *  untagged and whether it is dropped is determined by the
     *  ''FM_PORT_DROP_UNTAGGED'' attribute.
     *                                                                  \lb\lb
     *  On FM6000 devices, dropping of priority tagged frames
     *  (VLAN ID is 0) is controlled by a separate attribute, 
     *  FM_PORT_DROP_PRIORITY_TAGGED.
     *                                                                  \lb\lb
     *  On FM10000 devices, this attribute is not applicable if 
     *  port attribute ''FM_PORT_ISL_TAG_FORMAT'' is set to 
     *  ''FM_ISL_TAG_F56''. 
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DROP_TAGGED,

    /** Type fm_uint32: Deprecated.  Use ''FM_PORT_TX_CUT_THROUGH'' instead. 
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_SAF,

    /** Type fm_uint32: The port speed in units of megabits per second.
     *  Acceptable values are 40000 for 40G, 20000 for 20G, 10000 for 10G, 
     *  2500 for 2.5G, 1000 for 1G, 100 for 100M and 10 for 10M. The 
     *  specified value will be rounded down to the nearest acceptable value 
     *  with 10M being the minimum. The value returned may be different than 
     *  the value set if the specific value requested is not available or 
     *  if auto-negotiation is enabled.
     *                                                                  \lb\lb
     *  The API defaults to 10000 (10G) on all ports, however, the platform
     *  layer will normally change this default on system startup depending on
     *  the capabilities of any PHYs used on the platform. 
     *                                                                  \lb\lb
     *  On FM6000 devices this attribute is read-only for most Ethernet
     *  interface modes. The port speed is automatically adjusted by the
     *  ''FM_PORT_ETHERNET_INTERFACE_MODE'' port attribute. There are two
     *  exceptions to the above: the speed may be configured for ports in
     *  the ''FM_ETH_MODE_SGMII'' Ethernet mode, and for the internal
     *  loopback port.
     *                                                                  \lb\lb
     *  On FM10000 devices the  port speed is automatically adjusted by the
     *  ''FM_PORT_ETHERNET_INTERFACE_MODE'' port attribute for ethernet
     *  ports and matches link speed. For other types of ports, the speed 
     *  reported by this attribute is the scheduler speed for 84B packets. 
     *  For non-ethernet ports, higher speeds may be achieved with larger
     *  packets.
     *
     *  Note: 84 Bytes comes from the fact that the scheduler is based on
     *  minimum Ethernet Frame size (64B + 12B (IFG) + 8B (preamble))
     *
     *  \portType6K ETH
     *  \portType10K ETH:ro, PCIE:ro, TE:ro, LPBK:ro
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_SPEED,

    /** Type fm_uint32: The 25-bit egress port mask for when this port is the
     *  ingress port. A one bit in the mask indicates that this port can
     *  forward to the port represented by the bit. The default value is
     *  0x1fffff, indicating that all ports can forward to all other ports.
     *  Note that the default value for the ''FM_VLAN_REFLECT'' VLAN 
     *  attribute prevents a port from transmitting received packets back to 
     *  itself.
     *                                                                  \lb\lb
     *  For FM6000 devices (or environments that mix these devices with 
     *  other switches), use ''FM_PORT_MASK_WIDE'' instead.
     *                                                                  \lb\lb
     *  This attribute may be set for the CPU interface port. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_MASK,

    /** Type ''fm_bitArray'': The N-bit egress port mask to use when this port is
     *  the ingress port. A one bit in the mask indicates that frames from
     *  this port can be forwarded to the port represented by the bit. Each
     *  bit position corresponds to the logical port number of the egress
     *  port. 
     *                                                                  \lb\lb
     *  In a Switch Aggregate environment, this attribute is supported only 
     *  on individual switches. It cannot be set directly using Switch
     *  Aggregate Id, but it can be retrieved using Switch Aggregate Id.
     *                                                                  \lb\lb
     *  See ''fm_bitArray'' for a list of helper functions required to
     *  initialize, read and write the ''fm_bitArray'' structure.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_MASK_WIDE,

    /** Type fm_uint32: The number of bit-times that the link partner needs to
     *  pause in units of 512 bit-times. The attribute value may range from
     *  0 to 65535. The default value is 65535.
     *                                                                  \lb\lb
     *  On FM2000, specify zero to disable PAUSE. 
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM6000, enabling PAUSE on a port is 
     *  controlled by the ''FM_PORT_LOSSLESS_PAUSE'' or 
     *  ''FM_PORT_SMP_LOSSLESS_PAUSE'' port attributes in conjunction with 
     *  the switch QoS attribute ''FM_AUTO_PAUSE_MODE''.
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_TX_PAUSE,

    /** Type fm_uint32: The amount of time before the TX resends the PAUSE
     *  ON frame.
     *
     *  For FM2000, FM3000 and FM4000 devices, the value is in units of 512 
     *  bit-times. The attribute value may range from 0 to 65535. 
     *                                                                  \lb\lb
     *  For FM4000 devices, The hardware uses the frame handler clock as a 
     *  reference and not port speed. Internally, The bit time configuration is 
     *  converted to units of the frame handler clock. The application is 
     *  responsible to reconfigure the setting in order to keep the number 
     *  of bit-times constant, when the port speed changes.
     *                                                                  \lb\lb 
     *  For FM6000 devices, the value is specified in nanoseconds and is tied 
     *  to port speed and the PAUSE sweeper period. Because these may change, 
     *  the value range can vary. Read the value of 
     *  ''FM_PORT_TX_PAUSE_RESEND_TIME_MAX'' to determine the maximum value 
     *  for the current configuration.  Writes to this attribute will not take 
     *  effect until the previous resend interval expires. The specified value 
     *  will be adjusted to the closes value supported by the hardware, so
     *  the value read back may not exactly match the value set.
     *                                                                  \lb\lb 
     *  For FM10000 devices, the value is specified in nanoseconds and is 
     *  calculated based on frame handler clock frequency. The maximum value can
     *  be calculated as follows: the number of sweeper ports (48) * maximum
     *  interval (65535) divided by the frequency of the switch. The attribute
     *  value is converted to internal units , which means that the value
     *  returned by ''fmGetPortAttribute'' may not be identical to the value
     *  specified by ''fmSetPortAttribute''.
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_TX_PAUSE_RESEND_TIME,

    /** Type fm_uint32: The maximum number of nanoseconds that can be 
     *  specified for the ''FM_PORT_TX_PAUSE_RESEND_TIME'' attribute for
     *  the current configuration.  
     *                                                                  \lb\lb 
     *  This attribute is dependent on the PAUSE quanta and sweeper 
     *  configuration. If the PAUSE period has not been configured in the 
     *  sweeper or if the PAUSE quanta is not set properly for the given port 
     *  speed, this attribute may return 0.
     *
     *  \portType ETH:ro
     *  \chips  FM6000 */
    FM_PORT_TX_PAUSE_RESEND_TIME_MAX,

    /** Type fm_bool: Receive PAUSE frame processing: FM_ENABLED or
     *  FM_DISABLED (default).
     *                                                                  \lb\lb
     *  On FM3000, FM4000, FM6000 and FM10000 devices, this attribute will 
     *  enable or disable RX PAUSE for all traffic classes.  Use 
     *  ''FM_PORT_RX_CLASS_PAUSE'' to enable or disable PAUSE for specific 
     *  traffic classes.
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_RX_PAUSE,

    /** Type fm_uint32: Enables or disables pause pacing per pause class.
     *  The mask is specified with 1 bit per pause class.  Bit 0 is pause
     *  class 0, bit 1 is pause class 1, etc. Only bits 0 through 7 are used,
     *  to represent the eight classes.
     *  
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_PAUSE_PACING_MASK,

    /** Type ''fm_pausePacingTime'': Specifies the amount of time, in 
     *  nanoseconds, per ''FM_PORT_TX_PAUSE_RESEND_TIME'' that a PAUSE class 
     *  will remain unpaused.  
     *                                                                  \lb\lb
     *  As ''FM_PORT_TX_PAUSE_RESEND_TIME'' specifies the PAUSE pacing
     *  period, this attribute specifies the duty cycle by defining the amount
     *  of time the PAUSE class is not paused.
     *                                                                  \lb\lb
     *  The possible range of values for this attribute is the same range as
     *  ''FM_PORT_TX_PAUSE_RESEND_TIME'' and is dependent on PAUSE quanta
     *  values and sweeper configuration.  To determine the maximum for the
     *  given switch configuration, read ''FM_PORT_TX_PAUSE_RESEND_TIME_MAX''.
     *  The value of this attribute must be less than the PAUSE resend interval
     *  specified in ''FM_PORT_TX_PAUSE_RESEND_TIME''.
     *                                                                  \lb\lb
     *  When set, the API will choose the closest approximate value that
     *  correlates to the specified time (in nanoseconds). Accordingly, the
     *  value of this attribute when read back may not be identical to the
     *  value specified when set.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_PAUSE_PACING_TIME,

    /** Deprecated. Type fm_uint32: The pacing rate in units of
     *  1/256 of the speed of the port. The attribute value may range from 0
     *  to 255. Specify zero to disable pacing (default). Specify 1 for 1/256
     *  of the port speed, specify 255 for 255/256 of the port speed, etc.
     *                                                                  \lb\lb
     *  Note: the value of this attribute functions linearly for 64-byte frames
     *  from 0% to 100% of the port speed, but it is not linear for other
     *  frame sizes. The mapping of attribute value to percentage of port
     *  speed is beyond the scope of this document and so this attribute is
     *  deprecated. 
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM2000 */
    FM_PORT_PACING_RATE,

    /** Type fm_bool: Indicates whether this port will have its PAUSE
     *  watermarks automatically calculated by the API when the 
     *  ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is enabled: FM_ENABLED or 
     *  FM_DISABLED (default).
     *                                                                  \lb\lb  
     *
     *  \portType ETH, LAG
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_PORT_LOSSLESS_PAUSE,

    /** Type fm_uint32: A bitmask representing a set of Shared Memory
     *  Partitions (bit 0 represents SMP0).  Setting a 1 in each bit position
     *  indicates to the auto pause watermark calculator that pause should be
     *  enabled on this SMP. To apply watermark calculation this attribute 
     *  requires that the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute be 
     *  enabled. The default value is zero. 
     *  On FM6000 devices this attribute is a superset of 
     *  ''FM_PORT_LOSSLESS_PAUSE'', which represents the PAUSE enabled bit for
     *  SMP0 only. 
     *  On FM10000 devices this attribute is read-only on the CPU interface 
     *  port.
     *                                                                  \lb\lb  
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_SMP_LOSSLESS_PAUSE,

    /** Type fm_int: The pacing watermark per priority. The
     *  attribute value is a pointer to an array of eight 32-bit words, indexed
     *  by switch priority. Each element of the array may range from 0x0 to
     *  0x1ffffff and represents the number of byte-times that the transmitter
     *  places after a frame before sending the start of the next frame (in
     *  addition to the standard preamble and IFG. Default value is zero.
     *  See "IFG Stretch (IFGS)" in the FM2000 datasheet for a detailed
     *  explanation. 
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM2000 */
    FM_PORT_PACING_WM,

    /** Type fm_int: Internal loopback control. Frames emitted from the
     *  switch fabric towards an egress port are processed all the way
     *  through the Ethernet port logic, then looped back from transmit
     *  to receive at the SerDes level, complete with preamble and IFG.
     *  Value is:
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_OFF (default) to turn off loopback,
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_TX2RX to loopback transmit to receive.
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_RX2TX (FM10000 only) to loopback receive to transmit.
     *                                                                  \lb\lb
     *  When reading the current loopback state, a returned value of
     *  FM_PORT_LOOPBACK_UNKNOWN indicates an invalid setting.
     *                                                                  \lb\lb
     *  On FM6000, FM10000 devices, if ''FM_PORT_FABRIC_LOOPBACK'' is enabled,
     *  then setting to FM_PORT_LOOPBACK_TX2RX will have no effect.
     *  
     *  \portType6K ETH
     *  \portType10K ETH, PCIE:ro, TE:ro, LPBK:ro
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_LOOPBACK,

    /** Type fm_int: External loopback control per MAC. Frames received
     *  on an ingress port are de-serialized, then re-serialized and 
     *  looped back out the port. Value is:
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_OFF (default) to turn off loopback,
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_RX2TX to loopback receive to transmit.
     *                                                                  \lb\lb
     *  When reading the current loopback state, a returned value of
     *  FM_PORT_LOOPBACK_UNKNOWN indicates an invalid setting. 
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_EXTERNAL_LOOPBACK,

    /** Type fm_int: Whether frames emitted out of the switch fabric 
     *  toward an egress port are instead looped back into the fabric.
     *  Since such frames do not touch the Ethernet port logic of the chip,
     *  they will not have a preamble, nor do they invoke an IFG. The
     *  associated port logic will be put in a standby mode so that any
     *  frames received at the port will not enter the switch fabric.
     *  Value is:
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_OFF (default) to turn off loopback,
     *                                                                  \lb\lb
     *  FM_PORT_LOOPBACK_TX2RX to loopback transmit to receive.
     *                                                                  \lb\lb
     *  When reading the current loopback state, a returned value of
     *  FM_PORT_LOOPBACK_UNKNOWN indicates an invalid setting. 
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK:ro
     *  \chips  FM6000, FM10000 */
    FM_PORT_FABRIC_LOOPBACK,

    /** Type fm_uint32: Receive lane ordering. Value is:
     *                                                                  \lb\lb
     *  FM_PORT_LANE_ORDERING_NORMAL to set normal lane ordering,
     *                                                                  \lb\lb
     *  FM_PORT_LANE_ORDERING_INVERT to reverse the lane ordering.
     *                                                                  \lb\lb
     *  The API defaults to FM_PORT_LANE_ORDERING_NORMAL on all ports,
     *  however, the platform layer may change this default on system
     *  startup depending on the layout of the board or the capabilities
     *  of any PHYs used on the platform. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-MAC.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_PORT_RX_LANE_ORDERING,

    /** Type fm_uint32: Transmit lane ordering. Value is:
     *                                                                  \lb\lb
     *  FM_PORT_LANE_ORDERING_NORMAL to set normal lane ordering,
     *                                                                  \lb\lb
     *  FM_PORT_LANE_ORDERING_INVERT to reverse the lane ordering.
     *                                                                  \lb\lb
     *  The API defaults to FM_PORT_LANE_ORDERING_NORMAL on all ports,
     *  however, the platform layer may change this default on system
     *  startup depending on the layout of the board or the capabilities
     *  of any PHYs used on the platform. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-MAC.
     *
     *  \portType ETH
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_PORT_TX_LANE_ORDERING,

    /** Type fm_int: The 16-bit Ethernet type field to use on
     *  double VLAN tagged frames when the first tag uses Ethernet type 0x8100.
     *  If there is no VLAN tag 0x8100 present in the frame, then the Ethernet
     *  type used will be 0x8100 regardless of the setting of this attribute.
     *  Default value is 0x8100. 
     *
     *  \chips  FM2000 */
    FM_PORT_VLAN_ETHER_TYPE,

    /** Type fm_macaddr: The 48-bit source MAC address to appear in PAUSE
     *  frames sent by this port. Default value is 0x000000000000. 
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_MAC_ADDR,

    /** Type ''fm_dot1xState'': The 802.1x authorization state. Default
     *  value is ''FM_DOT1X_STATE_NOT_ACTIVE''.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM2000, FM3000, FM4000, FM10000 */
    FM_PORT_DOT1X_STATE,

    /** Type fm_bool: The deficit idle count: FM_ENABLED or FM_DISABLED.
     *                                                                  \lb\lb
     *  On FM10000 devices the API defaults to FM_ENABLED on all ports. On all
     *  other devices the API defaults to FM_DISABLED on all ports, however,
     *  the platform layer may change this default on system startup. For
     *  example, on the FM85XXEP Evaluation Platform, it is set to FM_ENABLED
     *  on system start up.
     *                                                                  \lb\lb
     *  On FM6000 devices, the value set will be applied to both MACs for
     *  ports that have two MACs.
     *
     *  \portType ETH
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_DIC_ENABLE,

    /** Type fm_bool: Routing on this port: FM_ENABLED (default for TE ports)
     *  or FM_DISABLED (default). A port must have this attribute enabled in
     *  order for traffic ingressing on the port to be routed. When this
     *  attribute is disabled, ingressing traffic will only be switched at
     *  layer 2.
     *  
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_ROUTABLE,

    /** Type fm_uint32: The transmitter inter-frame gap in bytes (0 - 63).
     *                                                                  \lb\lb
     *  Note: if ''FM_PORT_SPEED'' is set to 2.5G or less, the programmed IFG will
     *  be rounded up to the next even value. It is the responsibility
     *  of the user to adjust this attribute if necessary when changing port
     *  speed.
     *                                                                  \lb\lb
     *  The API defaults to a value of 12 on all ports, however, the platform
     *  layer may change this default on system startup. For example, on the
     *  FM85XXEP Evaluation Platform, it is set to a value of 10 on system 
     *  start up. 
     *                                                                  \lb\lb
     *  On FM6000 devices, the value set will be applied to both MACs for
     *  ports that have two MACs.
     *
     *  \portType ETH
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_IFG,

    /** Type fm_uint32.  Deprecated - Use ''FM_PORT_TX_PAUSE_MODE'' instead. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_RX_PAUSE_MODE,

    /** Type fm_uint32.  Determines the PAUSE frame format on transmit. 
     *  Valid values are:
     *                                                                  \lb\lb
     *  FM_PORT_TX_PAUSE_CLASS_BASED
     *                                                                  \lb\lb
     *  FM_PORT_TX_PAUSE_NORMAL (default).
     *
     *  On FM10000 devices, for class based PAUSE frame format, use the 
     *  ''FM_PORT_TX_CLASS_PAUSE'' attribute to enable or disable priority 
     *  vector bit, and associated time value field in generated PAUSE frame 
     *  for specific traffic classes.
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_TX_PAUSE_MODE,

    /** Type fm_uint32: A bit mask indicating which traffic
     *  classes have PAUSE enabled. Bit zero corresponds to traffic class
     *  (queue) 0, bit one to class 1, etc. A "1" in a bit position indicates
     *  that the corresponding class has PAUSE enabled. 
     *                                                                  \lb\lb
     *  For FM3000, FM4000 and FM10000 devices, the value is 8-bits and 
     *  the default value is 0, indicating that PAUSE is disabled for all
     *  traffic classes.
     *                                                                  \lb\lb
     *  For FM6000 devices, the value is 12-bits and the default value
     *  is 0, indicating that PAUSE is disabled for all traffic classes.
     *                                                                  \lb\lb
     *  Note: Setting ''FM_PORT_RX_PAUSE'' to FM_ENABLED will enable PAUSE 
     *  for all traffic classes.  Setting ''FM_PORT_RX_PAUSE'' to FM_DISABLED
     *  will disable PAUSE for all traffic classes.
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_RX_CLASS_PAUSE,

    /** Type fm_uint32: A bit mask indicating which traffic
     *  classes have priority vector bit, and associated time value enabled in 
     *  generated PAUSE frame. Bit zero corresponds to traffic class
     *  (queue) 0, bit one to class 1, etc. A "1" in a bit position indicates
     *  that the corresponding class has PAUSE enabled. 
     *                                                                  \lb\lb
     *  The value is 8-bits and the default value is 255, indicating that 
     *  PAUSE priority vector is enabled for alltraffic classes.
     *  Note: The attribute does not take effect until ''FM_PORT_TX_PAUSE_MODE''
     *  is set to FM_PORT_TX_PAUSE_CLASS_BASED.
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM10000 */
    FM_PORT_TX_CLASS_PAUSE,

    /** Type fm_uint32: Defines how to transmit the CFI bit on
     *  an outgoing frame. Value is:
     *                                                                  \lb\lb
     *  FM_PORT_TXCFI_ASIS (default) to set the CFI bit equal to what was
     *  received.
     *                                                                  \lb\lb
     *  FM_PORT_TXCFI_ISPRIBIT to replace the CFI bit.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_TXCFI,

    /** Type fm_uint32: Defines how to transmit the CFI bit on VLAN2 on
     *  an outgoing frame. Value is:
     *                                                                  \lb\lb
     *  FM_PORT_TXCFI_ASIS (default) to set the CFI bit equal to what was
     *  received.
     *                                                                  \lb\lb
     *  FM_PORT_TXCFI_ISPRIBIT to replace the CFI bit.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_TXCFI2,

    /** Type fm_uint32: A bit mask indicating the allowed source(s)
     *  for the switch priority.  The mask consists of the OR of any of the
     *  following fields:
     *                                                                  \lb\lb
     *  FM_PORT_SWPRI_VPRI1 indicates the VLAN1 priority,
     *                                                                  \lb\lb
     *  FM_PORT_SWPRI_VPRI (a synonym FM_PORT_SWPRI_VPRI1),
     *                                                                  \lb\lb
     *  FM_PORT_SWPRI_VPRI2 indicates the VLAN2 priority (FM6000 only),
     *                                                                  \lb\lb
     *  FM_PORT_SWPRI_DSCP indicates the Differentiated Services Code Point,
     *                                                                  \lb\lb
     *  FM_PORT_SWPRI_ISL_TAG indicates the Inter-Switch Link tag.
     *                                                                  \lb\lb
     *  The default value is (FM_PORT_SWPRI_VPRI1 | FM_PORT_SWPRI_ISL_TAG). 
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_SWPRI_SOURCE,

    /** Type fm_bool: Defines whether the switch priority prefers
     *  using the DSCP field when both the VPRI and DSCP fields are present:
     *  FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Note that the presence of an ISL tag is always the switch priority
     *  source if ''FM_PORT_SWPRI_SOURCE'' includes FM_PORT_SWPRI_ISL_TAG. 
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_SWPRI_DSCP_PREF,

    /** Type fm_uint32: Defines how the parser parses the packet.
     *  Value is:
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_STOP_AFTER_L2 (default)
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_STOP_AFTER_L3 - Note, you must set FM_PORT_PARSER to
     *  this value to enable any L3 features, such as IGMP snooping, routing,
     *  IGMP storm control and L3 ACLs.
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_STOP_AFTER_L4 - Note, you must set FM_PORT_PARSER to
     *  this value to enable L4 ACLs (default for TE ports).
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_PARSER,

    /** Type fm_uint32: Defines the 16-bit customer VLAN which
     *  identifies the C-VLAN in a stacked tag environment. The default value
     *  is 0x8100. 
     *                                                                  \lb\lb
     *  For FM6000 devices, use switch attributes
     *  FM_SWITCH_VLAN1_ETHERTYPE_A, FM_SWITCH_VLAN1_ETHERTYPE_B,
     *  FM_SWITCH_VLAN2_ETHERTYPE_A, and FM_SWITCH_VLAN2_ETHERTYPE_B.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_CVLAN_TAG,

    /** Type fm_uint32: Defines the 16-bit service VLAN tag A,
     *  which identifies the service VLAN in a stacked tag environment. The
     *  default value is 0x88A8. If set equal to the value of the
     *  ''FM_PORT_PARSER_CVLAN_TAG'' attribute, effectively disables this
     *  attribute. 
     *                                                                  \lb\lb
     *  For FM6000 devices, use switch attributes
     *  FM_SWITCH_VLAN1_ETHERTYPE_A, FM_SWITCH_VLAN1_ETHERTYPE_B,
     *  FM_SWITCH_VLAN2_ETHERTYPE_A, and FM_SWITCH_VLAN2_ETHERTYPE_B.
     *
     *  For FM10000 devices, use switch attribute
     *  ''FM_SWITCH_PARSER_VLAN_ETYPES'', and port attributes
     *  ''FM_PORT_PARSER_VLAN1_TAG'' and ''FM_PORT_PARSER_VLAN2_TAG''.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_VLAN_TAG_A,

    /** Type fm_uint32: Defines the 16-bit service VLAN tag B,
     *  which identifies the service VLAN in a stacked tag environment. The
     *  default value is 0x9100. If set equal to the value of the
     *  ''FM_PORT_PARSER_CVLAN_TAG'' attribute, it effectively disables this
     *  attribute. 
     *                                                                  \lb\lb
     *  For FM6000 devices, use switch attributes
     *  FM_SWITCH_VLAN1_ETHERTYPE_A, FM_SWITCH_VLAN1_ETHERTYPE_B,
     *  FM_SWITCH_VLAN2_ETHERTYPE_A, and FM_SWITCH_VLAN2_ETHERTYPE_B.
     *
     *  For FM10000 devices, use switch attribute
     *  ''FM_SWITCH_PARSER_VLAN_ETYPES'', and port attributes
     *  ''FM_PORT_PARSER_VLAN1_TAG'' and ''FM_PORT_PARSER_VLAN2_TAG''.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_VLAN_TAG_B,

    /** Type fm_uint32: Defines the 8-bit custom protocol "A"
     *  for deep packet inspection. Default value is zero. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_PROTOCOL_A,

    /** Type fm_uint32: Defines the 8-bit custom protocol "B"
     *  for deep packet inspection. Default value is zero. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_PROTOCOL_B,

    /** Type fm_uint64: Defines which 16-bit words in the TCP
     *  payload (up to 96 bytes following the TCP header) to forward to
     *  the switch device's frame handler for parsing. The value is a 48-bit 
     *  mask where each bit identifies a 16-bit word in the TCP payload. Bit 0
     *  indicates the first 16-bit word in the payload, bit 1 the second and
     *  so on. A "1" bit indicates that the corresponding 16-bit word in the
     *  payload should be forwarded to the frame handler for parsing. The
     *  default value is zero indicating that no part of the TCP payload is
     *  forwarded. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_TCP_PAYLOAD,

    /** Type fm_uint64: Defines which 16-bit words in the UDP
     *  payload (up to 96 bytes following the UDP header) to forward to
     *  the switch device's frame handler for parsing. The value is a 48-bit 
     *  mask where each bit identifies a 16-bit word in the UDP payload. Bit 0
     *  indicates the first 16-bit word in the payload, bit 1 the second and
     *  so on. A "1" bit indicates that the corresponding 16-bit word in the
     *  payload should be forwarded to the frame handler for parsing. The
     *  default value is zero indicating that no part of the UDP payload is
     *  forwarded. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_UDP_PAYLOAD,

    /** Type fm_uint64: Defines which 16-bit words in the
     *  payload of the protocol identified by ''FM_PORT_PARSER_PROTOCOL_A''
     *  (up to 96 bytes following the protocol header) to forward to
     *  the switch device's frame handler for parsing. The value is a 48-bit 
     *  mask where each bit identifies a 16-bit word in the protocol payload. 
     *  Bit 0 indicates the first 16-bit word in the payload, bit 1 the second 
     *  and so on. A "1" bit indicates that the corresponding 16-bit word in 
     *  the payload should be forwarded to the frame handler for parsing. The
     *  default value is zero indicating that no part of the protocol payload
     *  is forwarded. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_PROT_A_PAYLOAD,

    /** Type fm_uint64: Defines which 16-bit words in the
     *  payload of the protocol identified by ''FM_PORT_PARSER_PROTOCOL_B''
     *  (up to 96 bytes following the protocol header) to forward to
     *  the switch device's frame handler for parsing. The value is a 48-bit 
     *  mask where each bit identifies a 16-bit word in the protocol payload. 
     *  Bit 0 indicates the first 16-bit word in the payload, bit 1 the second 
     *  and so on. A "1" bit indicates that the corresponding 16-bit word in 
     *  the payload should be forwarded to the frame handler for parsing. The
     *  default value is zero indicating that no part of the protocol payload
     *  is forwarded. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_PROT_B_PAYLOAD,

    /** Type fm_uint32: The number of bytes following the
     *  Ethertype field in a non-IP frame that is forwarded to the switch 
     *  device's frame handler for parsing. The attribute value may range 
     *  from 0 to 32, but must be even. An odd value will be rounded down. 
     *  The default value is 32. The value must be set high enough to 
     *  accommodate MAC Control and Congestion Notification parsing, when 
     *  needed. 
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_NOT_IP_PAYLOAD,

    /** Type fm_bool: Enables whether or not the frame parsing
     *  logic parses frames that are not IP.  If this is set, then the number
     *  of bytes into the L2 payload to parse is determined by the
     *  ''FM_PORT_PARSER_NOT_IP_PAYLOAD'' attribute. FM_ENABLED or
     *  FM_DISABLED (default).
     *                                                                  \lb\lb
     *  For FM6000 devices, use FM_PORT_DI_PARSING.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_ENABLE_NOT_IP,

    /** Type fm_uint32: A bit mask indicating which options should
     *  be flagged for trap or FFU actions. The mask consists of the OR of any
     *  of the following:
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_FLAG_IPV4_OPTIONS to flag if IPv4 options are
     *  present,
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_FLAG_IPV6_ROUTING to flag if an IPv6 routing header
     *  is present,
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_FLAG_IPV6_FRAGMENT to flag if an IPv6 fragment header
     *  is present,
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_FLAG_IPV6_HOPBYHOP to flag if an IPv6 hop-by-hop option
     *  is present,
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_FLAG_IPV6_AUTH to flag if an IPv6 authentication header
     *  is present,
     *                                                                  \lb\lb
     *  FM_PORT_PARSER_FLAG_IPV6_DEST to flag if an IPv6 destination option
     *  is present.
     *                                                                  \lb\lb
     *  The default value is zero. 
     *                                                                  \lb\lb
     *  Note this attribute is only effective when the 
     *  ''FM_IP_OPTIONS_DISPOSITION'' switch attribute is set to a value 
     *  that enables trapping frames to the CPU.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_PARSER_FLAG_OPTIONS,

    /** Type fm_uint32: Defines the auto-negotiation standard
     *  used on this port.  Valid values are:
     *                                                                  \lb\lb
     *  FM_PORT_AUTONEG_NONE (default),                                     \lb
     *  FM_PORT_AUTONEG_SGMII,                                              \lb
     *  FM_PORT_AUTONEG_CLAUSE_37,                                          \lb
     *  FM_PORT_AUTONEG_CLAUSE_73.
     *
     *  \portType ETH
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_AUTONEG,

    /** Type fm_uint32: Deprecated - Use ''FM_PORT_AUTONEG_BASEPAGE'' instead.
     *                                                                  \lb\lb
     *  Defines the auto-negotiation capabilities to be advertised,
     *  as a raw codeword value.  If this value is set to non-zero, 
     *  it overrides the value of FM_PORT_AUTONEG_CAPABILITIES.  
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_AUTONEG_RAW_CAPABILITIES,

    /** Type fm_uint32: Deprecated - Use ''FM_PORT_AUTONEG_PARTNER_BASEPAGE''
     *  instead. 
     *                                                                  \lb\lb
     *  Defines the auto-negotiation capabilities received from the
     *  link partner. The attribute value is the raw codeword received via 
     *  the auto-negotiation protocol.  This attribute is read-only.
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_AUTONEG_PARTNER_CAPABILITY,

    /** Type fm_uint64: Defines the auto-negotiation capabilities to be 
     *  advertised, as a base page codeword value. The default value is
     *  dependent on the auto-negotiation mode, as specified by the
     *  ''FM_PORT_AUTONEG'' attribute. For Clause 73, the default
     *  is also dependent on whether the port is a "P0" port, defined as
     *  a port whose physical port number is evenly divisible by 4, as 
     *  described under ''Port Configuration Restrictions and Constraints'' 
     *  in the API User Guide.
     *                                                                  \lb\lb
     *  SGMII:     0x4001                                                   \lb
     *  Clause 37: 0x40a0                                                   \lb
     *  Clause 73: 0x33a04001 for "P0" ports                                \lb
     *  Clause 73: 0x0a04001 for "non-P0" ports                             \lb
     *  where a "P0" port is the first of four ports sharing a MAC, as, as
     *  described under ''API Abstraction of Port Resources'' in the API
     *  User Guide.
     *                                                                  \lb\lb 
     *  Setting this attribute to zero will automatically restore the mode and 
     *  port dependent default value.
     *                                                                  \lb\lb 
     *  NOTE: Clause 37 and SGMII use only the low-order 32 bits.
     *
     *  \portType ETH
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_AUTONEG_BASEPAGE,

    /** Type fm_uint64: Provides the auto-negotiation capabilities
     *  received from the link partner. The attribute value is the base page
     *  codeword received via the auto-negotiation protocol.
     *                                                                  \lb\lb 
     *  NOTE: Clause 37 and SGMII use only the low-order 32 bits.
     *
     *  \portType ETH:ro
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_AUTONEG_PARTNER_BASEPAGE,

    /** Type ''fm_anNextPages'': The auto-negotiation next pages to be sent to
     *  the link partner.
     *                                                                  \lb\lb  
     *  NOTE: When setting this attribute, the API will not free the storage 
     *  pointed to by the nextPages member of ''fm_anNextPages''; the caller 
     *  is responsible for allocating and deallocating it. When getting this
     *  attribute, the API will allocate memory for the nextPages member of 
     *  ''fm_anNextPages'', but the application must deallocate it with a 
     *  call to ''fmAlloc''. 
     *
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_AUTONEG_NEXTPAGES,

    /** Type ''fm_anNextPages'': Provides the auto-negotiation next pages 
     *  received from the link partner. 
     *                                                                  \lb\lb  
     *  NOTE: The application must free the API-allocated storage pointed to
     *  by the nextPages member of the ''fm_anNextPages'' structure with a 
     *  call to ''fmAlloc''. 
     *
     *  \portType ETH:ro
     *  \chips  FM6000, FM10000 */
    FM_PORT_AUTONEG_PARTNER_NEXTPAGES,

    /** Type fm_uint32: Provides the OUI used by 25G auto-negotiation next
     *  pages used by the 25G-Consortium
     *                                                                  \lb\lb
     *  
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_AUTONEG_25G_NEXTPAGE_OUI,

    /** Type fm_uint32: The Clause 73 link_fail_inhibit_timer in milliseconds
     *  for all links supported. The default is 0.5 second, a valid range is
     *  <0..511> milliseconds. Setting a value of zero will automatically reset
     *  this attribute to its default value.
     *                                                                  \lb\lb  
     *  In FM6000 devices, this timer is implemented as a number of ticks 
     *  between 0 and 511, which will give you a range from 1 hundredth of a
     *  second to 5.11 seconds.
     *
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_AUTONEG_LINK_INHB_TIMER,

    /** Type fm_uint32: Deprecated - Use ''FM_PORT_AUTONEG_LINK_INHB_TIMER''
     *  instead.  Due to hardware limitation, we can't handle two different
     *  timers for the link inhibit timer.
     *                                                                  \lb\lb
     *  The Clause 73 link_fail_inhibit_timer in milliseconds for 1000BASEX-KX
     *  and 10GBASE-KX4 links only (see ''FM_PORT_AUTONEG_LINK_INHB_TIMER''
     *  for other types of links). The default is 50ms, a valid range is
     *  <0..511> milliseconds. Setting a value of zero will automatically
     *  reset this attribute to its default value.
     *                                                                  \lb\lb  
     *  NOTE: If auto-negotiation is already running, the application must
     *  restart auto-negotiation for this configuration to take effect. 
     *                                                                  \lb\lb
     *  The API will round the specified value to the closest value supported
     *  by the hardware, so the value retrieved with ''fmGetPortAttribute''
     *  may not be identical to the value set with ''fmSetPortAttribute''. 
     *                                                                  \lb\lb
     *  In FM6000 devices, this timer is implemented as a number of ticks 
     *  between 0 and 512, where a tick could be one of the following: 1 us, 
     *  10 us, 100 us, 1 ms, 10 ms, 100 ms or 1 s.
     *
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_AUTONEG_LINK_INHB_TIMER_KX,

    /** Type fm_bool: Ignore the NONCE field during Clause 73 autonegotiation:
     *  FM_ENABLED or FM_DISABLED (default). When enabled it allows the port
     *  to complete Clause 73 autonegotiation in loopback mode.
     *                                                                  \lb\lb  
     *  NOTE: If auto-negotiation is already running, the application must
     *  restart auto-negotiation for this configuration to take effect.
     *
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_AUTONEG_IGNORE_NONCE,

    /** Type ''fm_islTagFormat'': Defines the type of ISL tag
     *  (if any) present on frames ingressing on this port. 
     *                                                                  \lb\lb
     *  Note: This will internally adjust the FM_PORT_MAX_FRAME_SIZE
     *  configuration to allow a packet with the configured max size to pass 
     *  thru an ISL tag enabled port.
     *                                                                  \lb\lb
     *  Note also that F64 ISL tags add an extra 8 bytes to untagged frames 
     *  and an extra 4 bytes to VLAN tagged frames. To account for this 
     *  added header, the minimum IFG (''FM_PORT_IFG'') on internal links 
     *  should be reduced to 4 and DIC (''FM_PORT_DIC_ENABLE'') should be 
     *  disabled.  
     *                                                                  \lb\lb
     *  On FM10000, this attribute supports FM_ISL_TAG_F56 or FM_ISL_TAG_NONE 
     *  on Ethernet ports, but only FM_ISL_TAG_F56 is supported for PCIe and 
     *  TE ports. Default value for PCIe and TE ports is FM_ISL_TAG_F56 and 
     *  for other ports FM_ISL_TAG_NONE. 
     *  See ''fm_islTagFormat'' for details.
     *                                                                  \lb\lb
     *  On FM6000, default value for ethernet port is FM_ISL_TAG_NONE and for
     *  CPU port(PCIe) default value is FM_ISL_TAG_F64.
     *                                                                  \lb\lb
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_ISL_TAG_FORMAT,

    /** Type fm_bool: Defines whether the port replaces MAC
     *  addresses and VLANs, etc., on outgoing routed frames:
     *  FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb  
     *  For FM10000 devices, see ''FM_PORT_ROUTED_FRAME_UPDATE_FIELDS'' port
     *  attribute.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_UPDATE_ROUTED_FRAME,

    /** Type fm_bool: Defines whether the port should decrement
     *  the TTL field on outgoing routed frames:
     *  FM_ENABLED (default) or FM_DISABLED.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_UPDATE_TTL,

    /** Type fm_bool: Defines whether the port may modify the DSCP field on
     *  outgoing routed frames: FM_ENABLED (default) or FM_DISABLED.
     *
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_UPDATE_DSCP,

    /** Type fm_bool: Defines whether the port makes the TCP
     *  Flags field available to the FFU/ACL subsystem.  Doing so reduces
     *  the amount of available deep inspection data by 16 bits. 
     *  FM_ENABLED or FM_DISABLED (default).
     *  
     *  For FM6000 devices, see the ''FM_PORT_CAPTURE_L4_ENTRY'' port
     *  attribute.
     *  
     *  \chips  FM3000, FM4000 */
    FM_PORT_CAPTURE_TCP_FLAGS,

    /** Type fm_bool: Specifies whether the port should capture L4 header
     *  fields for use by the FFU/ACL subsystem. The extracted data depends
     *  on the supported IP Protocol. Setting this attribute reduces the
     *  amount of available deep inspection data by 16 bits. FM_ENABLED or
     *  FM_DISABLED (default). Supported protocols and extracted data are:
     *                                                                  \lb\lb
     *  TCP  : [Offset(4-bit) | reserved(3-bit) | ECN(3-bit) |Control(6-bit)]
     *                                                                  \lb\lb
     *  ICMP : [Type(8-bit) | Code(8-bit)]
     *                                                                  \lb\lb
     *  VxLan : [VNI(24-bit)]
     *
     *  \portType ETH, LAG
     *  \chips  FM6000 */
    FM_PORT_CAPTURE_L4_ENTRY,

    /** Type fm_bool: Defines whether the port should modify the
     *  VLAN priority field on egress: FM_ENABLED or FM_DISABLED (default).
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG    
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_TXVPRI,

    /** Type fm_bool: Defines whether the port should modify the
     *  VLAN2 priority field on egress: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_TXVPRI2,

    /** Type fm_int: Defines the scheme for handling broadcast frames
     *  received on this port.
     *  Valid values are:
     *                                                                  \lb\lb
     *  FM_PORT_BCAST_FWD_EXCPU (default) to flood the broadcast frames to
     *  all ports except CPU.
     *                                                                  \lb\lb
     *  FM_PORT_BCAST_FWD to flood the broadcast frame (including CPU port)
     *                                                                  \lb\lb
     *  FM_PORT_BCAST_DISCARD to drop broadcast frames.
     *                                                                  \lb\lb
     *  Note: Setting the ''FM_BCAST_FLOODING'' switch attribute may override
     *  the setting of this attribute for all ports.
     *  See ''FM_BCAST_FLOODING'' for more information.
     *                                                                  \lb\lb
     *  Setting this attribute will change the ''FM_BCAST_FLOODING'' switch
     *  attribute to FM_BCAST_FLOODING_PER_PORT.
     *
     *  \portType ETH, LAG
     *  \chips  FM6000, FM10000 */
    FM_PORT_BCAST_FLOODING,

    /** Type fm_int: Defines the scheme for handling
     *  multicast frames received on this port with an unknown address.
     *  Valid values are:
     *                                                                  \lb\lb
     *  FM_PORT_MCAST_FWD_EXCPU (default) to flood the unknown multicast
     *  frames to all ports except CPU.
     *                                                                  \lb\lb
     *  FM_PORT_MCAST_TRAP to intercept unknown multicast frames and send
     *  them to the CPU instead of flooding them to other ports.
     *                                                                  \lb\lb
     *  FM_PORT_MCAST_FWD to send a copy of unknown multicast frames to
     *  the CPU. The frames are still flooded to other ports.
     *                                                                  \lb\lb
     *  FM_PORT_MCAST_DISCARD to drop unknown multicast frames.
     *                                                                  \lb\lb
     *  For FM3000, FM4000, FM6000 and FM10000: Setting the ''FM_MCAST_FLOODING''
     *  switch attribute may override the setting of this attribute for all
     *  ports. See ''FM_MCAST_FLOODING'' for more information.
     *                                                                  \lb\lb
     *  For FM6000 and FM10000: Setting this attribute will change the
     *  ''FM_MCAST_FLOODING'' switch attribute to FM_MCAST_FLOODING_PER_PORT.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_MCAST_FLOODING,

    /** Type fm_int: Defines the scheme for handling unicast 
     *  frames received on this port with an unknown destination address.  
     *  Valid values are:
     *                                                                  \lb\lb
     *  FM_PORT_UCAST_FWD_EXCPU (default) to flood the unicast frames to all
     *  ports (in the associated VLAN) except the CPU port.
     *                                                                  \lb\lb
     *  FM_PORT_UCAST_TRAP to trap the unicast frames to the CPU. Frames are
     *  not flooded to any other ports.
     *                                                                  \lb\lb
     *  FM_PORT_UCAST_FWD to log the unicast frames to the CPU. Frames are
     *  also flooded to all ports (in the associated VLAN).
     *                                                                  \lb\lb
     *  FM_PORT_UCAST_DISCARD to drop the unicast frames. 
     *                                                                  \lb\lb
     *  For FM3000, FM4000, FM6000 and FM10000: Setting the 
     *  ''FM_UCAST_FLOODING'' switch attribute may override the setting of 
     *  this attribute for all ports. See ''FM_UCAST_FLOODING'' for more 
     *  information.
     *                                                                  \lb\lb
     *  For FM6000 and FM10000: Setting this attribute will change the
     *  ''FM_UCAST_FLOODING'' switch attribute to FM_UCAST_FLOODING_PER_PORT.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_UCAST_FLOODING,

    /** Type fm_bool: Whether a port is to transmit flooded broadcast
     *  frames: FM_DISABLED (default) to allow flooding of broadcast frames,
     *  or FM_ENABLED to drop flooded broadcast frames.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_BCAST_PRUNING,

    /** Type fm_bool: Whether a port is to transmit flooded multicast
     *  frames: FM_DISABLED (default) to allow flooding of multicast frames,
     *  or FM_ENABLED to drop flooded multicast frames.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PORT_MCAST_PRUNING,

    /** Type fm_bool: Whether a port is to transmit flooded unicast frames:
     *  FM_DISABLED (default) to allow flooding of unicast frames, or
     *  FM_ENABLED to drop flooded unicast frames.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_UCAST_PRUNING,

    /** Type fm_bool: Deprecated.  Use ''FM_PORT_TX_CUT_THROUGH'' instead. 
     *                                                                  \lb\lb
     *  This attribute cannot be set on a LAG logical port.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_PORT_CUT_THROUGH,

    /** Type fm_bool: Specifies whether a port is to transmit frames in
     *  cut-through mode (if possible) or force store-and-forward mode.
     *  FM_ENABLED (default) permits cut-through operation, FM_DISABLED
     *  forces store-and-forward operation.
     *                                                                  \lb\lb
     *  In a Switch Aggregate environment, this attribute is supported only 
     *  on individual switches. It cannot be set directly using Switch 
     *  Aggregate Id, but it can be retrieved using Switch Aggregate Id. 
     *                                                                  \lb\lb
     *  This attribute cannot be set for the CPU interface port because that
     *  port must always be in store-and-forward mode.  
     *
     *  \portType6K ETH 
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_TX_CUT_THROUGH,

    /** Type fm_bool: Specifies whether a port is to receive frames in
     *  cut-through mode (if possible) or force store-and-forward mode.
     *  FM_ENABLED (default) permits cut-through operation, FM_DISABLED
     *  forces store-and-forward operation.
     *                                                                  \lb\lb
     *  In a Switch Aggregate environment, this attribute is supported only 
     *  on individual switches. It cannot be set directly using Switch 
     *  Aggregate Id, but it can be retrieved using Switch Aggregate Id. 
     *                                                                  \lb\lb
     *  This attribute cannot be set for the CPU interface port because that
     *  port must always be in store-and-forward mode.  
     *
     *  \portType6K ETH 
     *  \portType10K ETH, PCIE, TE, LPBK
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_RX_CUT_THROUGH,

    /** Type fm_uint32: A mask indicating which statistic groups 
     *  are enabled for counting on the given port.  Value is a mask of the 
     *  following constants:
     *                                                                  \lb\lb
     *  FM_STAT_GROUP_PER_PORT_RX_FRAME_CLASS,                              \lb
     *  FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_LEN,                             \lb
     *  FM_STAT_GROUP_PER_PORT_RX_OCTETS,                                   \lb
     *  FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_PRIORITY,                        \lb
     *  FM_STAT_GROUP_PER_PORT_RX_OCTETS_BY_PRIORITY,                       \lb
     *  FM_STAT_GROUP_PER_PORT_FWD_ACTION,                                  \lb
     *  FM_STAT_GROUP_PER_PORT_TX_FRAME_CLASS,                              \lb
     *  FM_STAT_GROUP_PER_PORT_TX_COUNT_BY_LEN,                             \lb
     *  FM_STAT_GROUP_PER_PORT_TX_OCTETS,                                   \lb
     *  FM_STAT_GROUP_VLAN_RX_FRAME_CLASS,                                  \lb
     *  FM_STAT_GROUP_VLAN_RX_OCTETS,                                       \lb
     *  FM_STAT_GROUP_EGRESS_ACL,                                           \lb
     *  FM_STAT_GROUP_PER_PORT_TX_DROP,                                     \lb
     *  FM_STAT_GROUP_PER_PORT_TRIGGER_COUNTERS
     *                                                                  \lb\lb
     *  The default value is:                                               \lb
     *  FM_STAT_GROUP_PER_PORT_RX_FRAME_CLASS |                             \lb
     *  FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_LEN |                            \lb
     *  FM_STAT_GROUP_PER_PORT_RX_OCTETS |                                  \lb
     *  FM_STAT_GROUP_PER_PORT_RX_COUNT_BY_PRIORITY |                       \lb
     *  FM_STAT_GROUP_PER_PORT_RX_OCTETS_BY_PRIORITY |                      \lb
     *  FM_STAT_GROUP_PER_PORT_FWD_ACTION |                                 \lb
     *  FM_STAT_GROUP_PER_PORT_TX_FRAME_CLASS |                             \lb
     *  FM_STAT_GROUP_PER_PORT_TX_COUNT_BY_LEN |                            \lb
     *  FM_STAT_GROUP_VLAN_RX_FRAME_CLASS |                                 \lb
     *  FM_STAT_GROUP_VLAN_RX_OCTETS |                                      \lb
     *  FM_STAT_GROUP_EGRESS_ACL |                                          \lb
     *  FM_STAT_GROUP_PER_PORT_TRIGGER_COUNTERS.
     *                                                                  \lb\lb
     *  For FM2000 devices, see the ''FM_STAT_GROUP_ENABLE'' 
     *  switch attribute.   
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_STAT_GROUP_ENABLE,

    /** Type fm_bool: Indicates whether loopback suppression
     *  is enabled or disabled for a port. Value is either FM_ENABLED (default)
     *  or FM_DISABLED (default for TE ports). Note that this attribute has
     *  no effect when a port is a member of a link aggregation group.
     *                                                                  \lb\lb
     *  This attribute is read-only on the CPU interface port.
     *
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_LOOPBACK_SUPPRESSION,

    /** Type fm_bool: Indicates whether the port is an internal port in a
     *  multi-switch environment (port connects the switch to another
     *  switch). Value is either FM_ENABLED or FM_DISABLED (default).  
     *                                                                   \lb\lb
     *  On FM10000, this attribute is always FM_DISABLED for PCIe ports
     *  and FM_ENABLED for TE ports.
     *                                                                   \lb\lb
     *  \portType6K ETH, LAG
     *  \portType10K ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_INTERNAL,

    /** Type fm_int: SERDES transmitter emphasis.
     *                                                                   \lb\lb
     *  For the FM2000, FM3000 and FM4000 devices the value ranges
     *  from 0 to 15 with a default of 10. The specified value
     *  corresponds to the emphasis according to the following:
     *                                                                   \lb\lb
     *  0  => 0%                                                            \lb
     *  1  => 4%                                                            \lb
     *  2  => 8%                                                            \lb
     *  3  => 12%                                                           \lb
     *  4  => 16%                                                           \lb
     *  5  => 20%                                                           \lb
     *  6  => 24%                                                           \lb
     *  7  => 28%                                                           \lb
     *  8  => 32%                                                           \lb
     *  9  => 36%                                                           \lb
     *  10 => 40%                                                           \lb
     *  11 => 44%                                                           \lb
     *  12 => 48%                                                           \lb
     *  13 => 52%                                                           \lb
     *  14 => 60%                                                           \lb
     *  15 => 65%
     *                                                                  \lb\lb
     *  Note: if the platform employs Quellan equalizers and supports the
     *  equalizer Auto-mode, the platform software may override any value
     *  for this attribute set by the application. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-lane
     *  per-MAC. Its value is made of two parts: 
     *                                                                  \lb\lb
     *  Kpre: Tx Equalization control for pulse Precursor (bits 4-6)       \lb
     *  Kpost: Tx Equalization control for pulse Postcursor (bits          
     *  0-3)
     *                                                                  \lb\lb
     *  Those control the values of V1, V2, V3, V4, V5 and V6 as
     *  defined in Figure 72-12 of IEEE Std 802.3ap-2007, according
     *  to the following equations:
     *                                                                  \lb\lb
     *  V1 = -V4 = Vmax-Vc*(A+2*Kpre)                                      \lb
     *  V2 = -V5 = Vmax-Vc*(A+2*(Kpre+Kpost))                              \lb
     *  V3 = -V6 = Vmax-Vc*(A+2*Kpost)           
     *                                                                  \lb\lb
     *  where:
     *                                                                  \lb\lb
     *  A is the value of the ''FM_PORT_DRIVE_STRENGTH'' attribute         \lb
     *  Vmax=575 mV for 1.1V operations, Vmax=517 mV for 1.0V              \lb
     *  Vc=16.2 mV for 1.1V operations, Vc=14.5 mV for 1.0V 
     *
     *  \portType ETH
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_PORT_EMPHASIS,
    
    /** Type fm_int: SERDES transmitter drive strength
     *                                                                   \lb\lb
     *  For the FM2000, FM3000, and FM4000 devices, this attribute
     *  represents the drive strength in mA. Value ranges from 6 to 35 with
     *  a default of 24.
     *                                                                   \lb\lb
     *  On FM6000 devices, this attribute is applied per-lane
     *  per-MAC. Its value ranges from 0 to 23 with a default of 0
     *  and provides control of the values of V1, V2, V3, V4, V5 and
     *  V6 as defined in Figure 72-12 of IEEE Std 802.3ap-2007,
     *  according to the following equations:
     *                                                                   \lb\lb
     *  V1 = -V4 = Vmax-Vc*(A+2*Kpre)                                       \lb
     *  V2 = -V5 = Vmax-Vc*(A+2*(Kpre+Kpost))                               \lb
     *  V3 = -V6 = Vmax-Vc*(A+2*Kpost)                     
     *                                                                   \lb\lb
     *  where:                                                           
     *                                                                   \lb\lb
     *  A is the value of this attribute                                    \lb
     *  Kpre and Kpost are described in ''FM_PORT_EMPHASIS''                \lb
     *  Vmax=575 mV for 1.1V operations, Vmax=517 mV for 1.0V               \lb
     *  Vc=16.2 mV for 1.1V operations, Vc=14.5 mV for 1.0V 
     *
     *  \portType ETH
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_PORT_DRIVE_STRENGTH,

    /** Type fm_int: SERDES transmitter preCursor.
     *                                                                  \lb\lb
     *  For the FM10000 the value ranges from -7 to +15 with a          \lb
     *  default of 0.                                                   \lb\lb
     *
     *  On FM10000 devices, this attribute is applied per-lane
     *  per-MAC.                                                        \lb\lb
     *
     *  This attribute provides control of the values of V1, V2, V3,
     *  V4, V5 and V6 as defined in Figure 72-11 of IEEE Std 802.3ap,
     *  according to the following equations:
     *                                                                  \lb\lb
     *  V1 = -V4 = Vmax-Vc*(Attn+2*Pre)                                 \lb
     *  V2 = -V5 = Vmax-Vc*(Attn+2*(Pre+Post))                          \lb
     *  V3 = -V6 = Vmax-Vc*(Atnn+2*Post)                                \lb  
     *                                                                  \lb\lb
     *  where:
     *                                                                  \lb\lb
     *  Attn is the value of the ''FM_PORT_TX_LANE_CURSOR'' attribute   \lb
     *  Pre is the value of the ''FM_PORT_TX_LANE_PRECURSOR'' attr.     \lb
     *  Post is the value of the ''FM_PORT_TX_LANE_POSTCURSOR'' attr.   \lb
     *  Vmax=560 mV                                                     \lb
     *  Vc=15 mV                                                        \lb
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_TX_LANE_PRECURSOR,

    /** Type fm_int: SERDES transmitter postCursor.
     *                                                                  \lb\lb
     *  For the FM10000 the value ranges from -31 to +31 with a         \lb
     *  default of 0.                                                   \lb\lb
     *
     *  On FM10000 devices, this attribute is applied per-lane
     *  per-MAC.                                                        \lb\lb
     *
     *  This attribute provides control of the values of V1, V2, V3,
     *  V4, V5 and V6 as defined in Figure 72-11 of IEEE Std 802.3ap,
     *  according to the following equations:
     *                                                                  \lb\lb
     *  V1 = -V4 = Vmax-Vc*(Attn+2*Pre)                                 \lb
     *  V2 = -V5 = Vmax-Vc*(Attn+2*(Pre+Post))                          \lb
     *  V3 = -V6 = Vmax-Vc*(Atnn+2*Post)                                \lb  
     *                                                                  \lb\lb
     *  where:
     *                                                                  \lb\lb
     *  Attn is the value of the ''FM_PORT_TX_LANE_CURSOR'' attribute   \lb
     *  Pre is the value of the ''FM_PORT_TX_LANE_PRECURSOR'' attr.     \lb
     *  Post is the value of the ''FM_PORT_TX_LANE_POSTCURSOR'' attr.   \lb
     *  Vmax=560 mV                                                     \lb
     *  Vc=15 mV                                                        \lb
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_TX_LANE_POSTCURSOR,

    /** Type fm_int: SERDES transmitter cursor
     *                                                                  \lb\lb
     *  For the FM10000 the value ranges from 0 to 31 with a default    \lb
     *  of 0.                                                           \lb\lb
     *                                                                 
     *  This attribute provides control the values of V1, V2, V3, V4,
     *  V5 and V6 as defined in Figure 72-11 of IEEE Std 802.3ap,
     *  according to the following equations:
     *                                                                  \lb\lb
     *  V1 = -V4 = Vmax-Vc*(Attn+2*Pre)                                 \lb
     *  V2 = -V5 = Vmax-Vc*(Attn+2*(Pre+Post))                          \lb
     *  V3 = -V6 = Vmax-Vc*(Atnn+2*Post)                                \lb  
     *                                                                  \lb\lb
     *  where:
     *                                                                  \lb\lb
     *  Attn is the value of the ''FM_PORT_TX_LANE_CURSOR'' attribute   \lb
     *  Pre is the value of the ''FM_PORT_TX_LANE_PRECURSOR'' attr.     \lb
     *  Post is the value of the ''FM_PORT_TX_LANE_POSTCURSOR'' attr.   \lb
     *  Vmax=560 mV                                                     \lb
     *  Vc=15 mV                                                        \lb
     *
     *  \portType ETH
     *  \chips  FM10000 */       
    FM_PORT_TX_LANE_CURSOR,


    /** Type fm_int: SERDES transmitter KR init preCursor.
     *                                                                  \lb\lb
     *  For the FM10000 the value ranges from -7 to +15 with a default of 0.
     *                                                                  \lb\lb
     *  On FM10000 devices, this attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  This attribute provides control of the equalization settings
     *  for the initialize state as defined in IEEE Std 802.3 section
     *  FIVE, clause 72.
     *  
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_TX_LANE_KR_INIT_PRECURSOR,

    /** Type fm_int: SERDES transmitter KR init postCursor.
     *                                                                  \lb\lb
     *  For the FM10000, the value ranges from -31 to +31 with a
     *  default of 4.
     *                                                                  \lb\lb
     *  On FM10000 devices, this attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  This attribute provides control of the equalization settings
     *  for the initialize state as defined in IEEE Std 802.3 section
     *  FIVE, clause 72.
     *  
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_TX_LANE_KR_INIT_POSTCURSOR,

    /** Type fm_int: SERDES transmitter KR init cursor
     *                                                                  \lb\lb
     *  For the FM10000, the value ranges from 0 to 31 with a default of 0.
     *                                                                  \lb\lb
     *  This attribute provides control of the equalization settings
     *  for the initialize state as defined in IEEE Std 802.3 section
     *  FIVE, clause 72.
     *  
     *  \portType ETH
     *  \chips  FM10000 */       
    FM_PORT_TX_LANE_KR_INIT_CURSOR,

    /** Type fm_bool: Whether to configure the initialize state cursor
     *  values during KR training: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  For FM10000 devices, it is applied per-lane per-MAC.
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_TX_LANE_ENA_KR_INIT_CFG,

    /** Type fm_int: SERDES transmitter KR, number of initial pre-cursor DEC
     *  requests when a PRESET request is received.
     *                                                                  \lb\lb
     *  For the FM10000, the value ranges from 0 to 15 with a default of 0.
     *                                                                 
     *  \portType ETH
     *  \chips  FM10000 */       
    FM_PORT_TX_LANE_KR_INITIAL_PRE_DEC,

    /** Type fm_int: SERDES transmitter KR, number of initial post-cursor
     *  DEC requests.
     *                                                                  \lb\lb
     *  For the FM10000, the value ranges from 0 to 15 with a default of 0.
     *                                                                 
     *  \portType ETH
     *  \chips  FM10000 */       
    FM_PORT_TX_LANE_KR_INITIAL_POST_DEC,

    /** Type fm_uint64: Specifies the BIST User Patterns for the custom10,
     *  custom20, and custom40 BIST submodes. Also defines the lower 40 bits
     *  for the custom80 BIST submode. In all cases, bit 0 is transmitted
     *  first. The default pattern for all submodes produces a square wave
     *  of a frequency equal to the portSpeed/4. The only restriction to the
     *  patterns is to have the same number of '1's and '0's to not
     *  introduce a DC offset.
     *                                                                  \lb\lb
     *  Bit fields for each BIST submode are defined as follows:
     *                                                                  \lb\lb
     *  custom10[0..9]:  FM_PORT_BIST_USER_PATTERN_LOW40[0..9]          \lb
     *  custom20[0..19]: FM_PORT_BIST_USER_PATTERN_LOW40[0..19]         \lb
     *  custom40[0..39]: FM_PORT_BIST_USER_PATTERN_LOW40[0..39]         \lb
     *  custom80[0..39]: FM_PORT_BIST_USER_PATTERN_LOW40[0..39]         \lb
     *                                                                  \lb\lb 
     *  Setting this attribute to zero will automatically restore the
     *  full (80 bits) default pattern.
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_BIST_USER_PATTERN_LOW40,

    /** Type fm_uint64: Defines upper 40 bits of the BIST User Patterns
     *  for the custom80 BIST submodes.
     *  The default pattern for all submodes produces a square wave of
     *  a frequency equal to the portSpeed/4. The only restriction to
     *  the pattern is to have the same number of '1's and '0's to not
     *  introduce a DC offset.                                          \lb
     *  Bit field is defined as follows:
     *                                                                  \lb\lb
     *  custom80[40..79]: FM_PORT_BIST_USER_PATTERN_UPP[0..39]          \lb
     *                                                                  \lb\lb 
     *  Setting this attribute to zero will automatically restore the
     *  default pattern for the upper 40 bits.
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_BIST_USER_PATTERN_UPP40,

    /** Type fm_bool: Enable link detection via interrupt: FM_ENABLED or
     *  FM_DISABLED (default). Polling is used when this is disabled. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-MAC.
     *                                                                  \lb\lb 
     *  On FM10000 devices, the default value is FM_ENABLED.
     *
     *  \portType6K ETH 
     *  \portType10K ETH:ro, PCIE:ro
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_LINK_INTERRUPT,

    /** Type fm_bool: Enables or disables whether or not the port
     *  logic ignores IFG errors.  Default is FM_DISABLED. It applies to
     *  ethernet modes other than 40GBase-R and 100GBase-R for which IFG
     *  errors are not checked in the first place.
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-MAC.
     *
     *  \portType ETH
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_IGNORE_IFG_ERRORS,

    /** Type fm_uint32: Indicates the number of bytes of unknown
     *  L4 frame payload to send to the frame filtering logic.  Note that this
     *  implies the protocol is not TCP and is not one of the user defined
     *  custom L4 protocols.  The value will be rounded down to the nearest 
     *  even value.  Default is zero. 
     *                                                                  \lb\lb
     *  For FM6000 devices, use FM_PORT_DI_PARSING.
     *
     *  \chips  FM3000, FM4000 */
    FM_PORT_PARSER_UNKNOWN_L4_PAYLOAD,

    /** Type ''fm_rxTermination'': The port receiver termination.
     *  This attribute is per-lane, per-MAC.
     *                                                                  \lb\lb
     *  See ''fm_rxTermination'' for a complete description of all
     *  possible values for this attribute.
     *
     *  \portType10K ETH, PCIE:ro
     *  \chips FM10000 */

    FM_PORT_RX_TERMINATION,

    /** Type fm_uint32: Receive lane polarity. On FM2000, FM3000 and FM4000
     *  devices, the value is a 4-bit mask (0x0 - 0xF), where bit 0 (the
     *  low-order bit) represents lane A, bit 1 is lane B, bit 2 is lane C,
     *  and bit 3 is lane D. A bit value of zero indicates normal polarity,
     *  and a bit value of 1 indicates reverse polarity. For convenience, the
     *  following mask values are pre-defined:
     *                                                                  \lb\lb
     *  FM_PORT_LANE_POLARITY_NORMAL (default) to set normal lane polarity 
     *  on all lanes.
     *                                                                  \lb\lb
     *  FM_PORT_LANE_POLARITY_INVERT to reverse lane polarity on all lanes.
     *                                                                  \lb\lb
     *  This attribute is applied to lanes A, B, C and D prior to any lane 
     *  reordering that may be applied by ''FM_PORT_RX_LANE_ORDERING''. 
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-MAC, per-lane.
     *                                                                  \lb\lb
     *  On FM10000 devices, this attribute is applied per-lane. On "set"
     *  operations a value of zero indicates normal polarity and any non-zero
     *  value indicates reverse polarity. On "get" operations a value of zero
     *  indicates normal polarity and a value of 1 indicates reverse polarity.
     *                                                                  \lb\lb
     *  While the API defaults to FM_PORT_LANE_POLARITY_NORMAL on all ports,
     *  the platform layer may change this default on system startup depending 
     *  on the layout of the board or the capabilities of any PHYs used on the 
     *  platform. 
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE:ro
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_RX_LANE_POLARITY,

    /** Type fm_uint32: Transmit lane polarity. On FM2000, FM3000 and FM4000
     *  devices, the value is:
     *                                                                  \lb\lb
     *  FM_PORT_LANE_POLARITY_NORMAL (default) to set normal lane polarity 
     *  on all lanes.
     *                                                                  \lb\lb
     *  FM_PORT_LANE_POLARITY_INVERT to reverse lane polarity on all lanes.
     *                                                                  \lb\lb
     *  For those devices, the lanes cannot be individually controlled for TX
     *  as they can for RX. All lanes are set to the same polarity.
     *                                                                  \lb\lb
     *  On FM6000 devices, this attribute is applied per-MAC, per-lane.
     *                                                                  \lb\lb
     *  On FM10000 devices, this attribute is applied per-lane. On "set"
     *  operations, a value of zero indicates normal polarity and any non-zero
     *  value indicates reverse polarity. On "get" operations, a value of zero
     *  indicates normal polarity and a value of 1 indicates reverse polarity.
     *                                                                  \lb\lb
     *  While the API defaults to FM_PORT_LANE_POLARITY_NORMAL on all ports,
     *  the platform layer may change this default on system startup depending 
     *  on the layout of the board or the capabilities of any PHYs used on the 
     *  platform. 
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE:ro
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_TX_LANE_POLARITY,

    /** Type fm_int: For ports that have more than one MAC available to them, 
     *  this attribute indicates which is the active one. Frame traffic is
     *  only transmitted and received over the active MAC. Note that the other 
     *  MAC on the port may still have a link UP state and may participate in 
     *  auto-negotiation, but cannot pass traffic. The value is zero-based,
     *  i.e., the first MAC is 0 and the second is 1. The default is zero, 
     *  however, the platform layer will normally change this default on 
     *  system startup as required. 
     *                                                                  \lb\lb
     *  The active MAC may be specified generically in calls to 
     * ''fmSetPortAttributeV2'' and ''fmGetPortAttributeV2'' as 
     *  FM_PORT_ACTIVE_MAC.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_SELECT_ACTIVE_MAC,

    /** Type ''fm_ethMode'': The Ethernet interface mode, per MAC. The mode 
     *  dictates the number of SERDES lanes that will be used by the port, 
     *  which can affect the availability of other ports that share those 
     *  lanes:
     *                                                                  \lb\lb
     *  Any 40G mode may be enabled only after all other ports sharing 
     *  this port's MAC have had their Ethernet interface mode disabled
     *  on all of this port's MACs.
     *                                                                  \lb\lb
     *  Any non-40G 4-lane mode may be enabled only after all other ports
     *  sharing this port's MAC have had their Ethernet interface mode
     *  disabled on the specified MAC.
     *                                                                  \lb\lb
     *  Any 1-lane mode may be enabled only as long as all other ports
     *  sharing this port's MAC(s) are not in a 40G mode on any MAC and
     *  are not in a 4-lane mode on the specified MAC.
     *                                                                  \lb\lb
     *  Not all ports are 4-lane (or 40G) capable. Refer to the FM6000
     *  datasheet to identify which ports can support 4-lane and 40G operation.
     *                                                                  \lb\lb
     *  This attribute constrains possible values for the ''FM_PORT_SPEED''
     *  attribute.
     *                                                                  \lb\lb
     *  ''FM_ETH_MODE_DISABLED'' is the default value, however, the platform
     *  layer will normally change this default on system startup depending on
     *  the capabilities and layout of the platform.
     *                                                                  \lb\lb
     *  The following Ethernet interface modes are ready-only, in that they
     *  can only be set via autonegotiation:
     *                                                                   \lb\lb
     *  ''FM_ETH_MODE_10GBASE_KR''                                          \lb
     *  ''FM_ETH_MODE_40GBASE_KR4''                                         \lb
     *  ''FM_ETH_MODE_40GBASE_CR4''                                         
     *                                                                   \lb\lb
     *  See ''fm_ethMode'' for a complete description of all possible values
     *  for this attribute.
     *
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_ETHERNET_INTERFACE_MODE,
    
    /** Type fm_bool: Whether received errored frames
     *  will be discarded if the error is detected prior to
     *  transmission: FM_ENABLED (default on FM6000) or FM_DISABLED
     *  (default on FM4000). A frame is considered errored if it is
     *  received with a bad CRC, PHY error or invalid length field.
     *  The attribute applies to the ingress port.
     *  
     *  \portType ETH
     *  \chips  FM3000, FM4000, FM6000 */
    FM_PORT_RX_ERRORED_FRAME_DISCARD,

    /** Type fm_uint32: Pad short frames to the number of bytes specified by 
     *  this value. The default value is 64. Note that the value must be a
     *  multiple of 4, otherwise it will be truncated to the closest lower
     *  4-byte boundary. For the FM6000, a value of 0 means that padding is
     *  disabled. For the FM10000, padding is always enabled and values may
     *  range from 40 to 116 bytes
     *
     *  \portType ETH, LAG
     *  \chips  FM6000, FM10000 */
    FM_PORT_TX_PAD_SIZE,

    /** Type ''fm_dfeMode'': Receiver DFE Tuning Mode for the port. This
     *  attribute is applied per-lane, per-MAC. This attribute may be set at
     *  any time, but it will not be applied to the hardware until the
     *  Ethernet mode is set.
     *                                                                  \lb\lb
     *  See ''fm_dfeMode'' for a description of the possible values for this
     *  attribute, including the default value.
     *
     *  \portType6K ETH
     *  \portType10K ETH, PCIE:ro
     *  \chips  FM6000, FM10000 */
    FM_PORT_DFE_MODE,

    /** Type fm_uint32: Current value for the Receiver DFE tuning
     *  parameters. This attribute is applied per-lane, per-MAC. It 
     *  defaults to ''FM_PORT_DFE_DEFAULTS''. This attribute is read/write 
     *  if ''FM_PORT_DFE_MODE'' is set to ''FM_DFE_MODE_STATIC'' (except for
     *  the DAC field), read-only otherwise. The DAC field described below is
     *  always read-only
     *                                                                  \lb\lb
     *  The attribute can be interpreted as the combination of six
     *  4-bit coefficients and one 6-bit coefficient:
     *                                                                  \lb\lb
     *  DAC   (bits 24-29)                                                 \lb
     *  ADV   (bits 20-23)                                                 \lb
     *  GainA (bits 16-19)                                                 \lb
     *  GainB (bits 12-15)                                                 \lb
     *  GainC (bits  8-11)                                                 \lb
     *  GainD (bits  4- 7)                                                 \lb
     *  GainI (bits  0- 3)                                                 
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_DFE_PARAMETERS,

    /** Type fm_uint32: Bits 0 - 7 are the current value of the vertical
     *  height of the eye diagram at its horizontal center. Bits 8 - 15 are
     *  the current value of the horizontal width of the eye diagram at its
     *  vertical center. Values may range from 0 to 64, or be set to 255
     *  if the value is temporarily unavailable. This attribute is 
     *  per-lane, per-MAC.
     *
     *  \portType6K ETH:ro
     *  \portType10K ETH:ro, PCIE:ro
     *  \chips  FM6000, FM10000 */
    FM_PORT_EYE_SCORE,

    /** Type ''fm_dfeState'': The port receiver coarse DFE tuning
     *  state.  This attribute is per-lane, per-MAC.
     *                                                                  \lb\lb
     *  See ''fm_dfeState'' for a complete description of all
     *  possible values for this attribute.
     *
     *  \portType6K ETH:ro
     *  \portType10K ETH:ro, PCIE:ro
     *  \chips  FM6000, FM10000 */
    FM_PORT_COARSE_DFE_STATE,

    /** Type ''fm_dfeState'': The port receiver fine DFE tuning
     *  state. This attribute is per-lane, per-MAC.
     *                                                                  \lb\lb
     *  See ''fm_dfeState'' for a complete description of all
     *  possible values for this attribute.
     *
     *  \portType6K ETH:ro
     *  \portType10K ETH:ro, PCIE:ro
     *  \chips  FM6000, FM10000 */
    FM_PORT_FINE_DFE_STATE,


    /** Type fm_int: Represents three possible settings of the transmitter 
     *  output waveform rise times. This attribute is applied per-lane,
     *  per-MAC.
     *                                                                  \lb\lb
     *  Valid values are 0 (default) through 2 
     *                                                                  \lb\lb
     *  0: slew control disabled: TTXmin =  17ps - TTXmax =  21ps           \lb
     *  1: slew control enabled:  TTXmin =  63ps - TTXmax = 115ps           \lb
     *  2: slew control enabled:  TTXmin = 104ps - TTXmax = 202ps        
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_SLEW_RATE,


    /** Type fm_bool: Whether ARP packets should be trapped to the 
     *  CPU: FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     * This attribute has been deprecated in favor of the ''FM_PORT_LOG_ARP'' 
     * attribute.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TRAP_ARP,


    /** Type fm_bool: Indicates whether this port will check if learning is
     *  enabled on VID2: FM_ENABLED (default) or FM_DISABLED.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_CHECK_LEARNING_VID2,


    /* Undocumented CJPAT enable, to be superseded by BIST mode CJPAT support */
    /** Type fm_bool: Unsupported - do not use. */
    FM_PORT_TRANSMIT_CJPAT,

    /** Type fm_int: The threshold for signal detection. This attribute is 
     *  applied per-lane, per-MAC. Valid values are 0 through 63 with a 
     *  default of 32. Lower values correspond to a higher signal detection 
     *  sensitivity, but have a higher probability to detect false positives.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_SIGNAL_THRESHOLD,

    /** Type fm_uint32: Used to filter out certain temporary link
     *  fault transitions.  It is expressed in microseconds. Acceptable values
     *  are betwen 0 and 31000000 (i.e. 31 seconds). It defaults to 0 (no
     *  debouncing). The fault transitions controlled by this attribute are
     *  the following:
     *                                                                  \lb\lb
     *  Local Fault -> Remote Fault                                        \lb
     *  Local Fault -> No Fault
     *                                                                  \lb\lb
     *  The API will round the specified value to the closest value supported
     *  by the hardware, so the value retrieved with ''fmGetPortAttribute''
     *  may not be identical to the value set with ''fmSetPortAttribute''. 
     *                                                                  \lb\lb
     *  In FM6000 devices, this timer is implemented as a number of ticks 
     *  between 0 and 31, where a tick could be one of the following: 1 us, 
     *  10 us, 100 us, 1 ms, 10 ms, 100 ms or 1 s.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_UP_DEBOUNCE_TIME,

    /** Type fm_uint32: Used to filter out certain temporary link
     *  fault transitions.  It is expressed in microseconds. Acceptable values
     *  are betwen 0 and 31000000 (i.e. 31 seconds). It defaults to 0 (no
     *  debouncing). The fault transitions controlled by this attribute are
     *  the following:
     *                                                                  \lb\lb
     *  Remote Fault -> Local Fault                                        \lb
     *  No Fault     -> Local Fault                                        \lb
     *  No Fault     -> Remote Fault
     *                                                                  \lb\lb
     *  The API will round the specified value to the closest value supported
     *  by the hardware, so the value retrieved with ''fmGetPortAttribute''
     *  may not be identical to the value set with ''fmSetPortAttribute''. 
     *                                                                  \lb\lb
     *  In FM6000 devices, this timer is implemented as a number of ticks 
     *  between 0 and 31, where a tick could be one of the following: 1 us, 
     *  10 us, 100 us, 1 ms, 10 ms, 100 ms or 1 s.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_DOWN_DEBOUNCE_TIME,

    /** Type fm_uint32: Transmit clock compensation value expressed in PPM.
     *  Setting it to 0 disables the clock compensation circuit.
     *                                                                  \lb\lb
     *  On FM10000 devices the API defaults to 100ppm on all ports. On all
     *  other devices the API defaults to a value of 0 on all ports, however,
     *  the platform layer may change this default on system startup, as is
     *  done on the reference platform, for example.
     *  
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_TX_CLOCK_COMPENSATION,

    /** Type fm_bool: Whether LACP frames should be trapped to the CPU:
     *  FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  Note that the ''FM_TRAP_IEEE_LACP'' switch attribute must be
     *  enabled for this attribute to be effective.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TRAP_IEEE_LACP,

    /** Type fm_bool: Trapping of spanning tree BPDU frames to the CPU: 
     *  FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  Note that the ''FM_TRAP_IEEE_BPDU'' switch attribute must be
     *  enabled for this attribute to be effective.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TRAP_IEEE_BPDU,

    /** Type fm_bool: Whether GARP frames should be trapped to the CPU:
     *  FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  Note that the ''FM_TRAP_IEEE_GARP'' switch attribute must be
     *  enabled for this attribute to be effective.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TRAP_IEEE_GARP,

    /** Type fm_bool: Trapping of IEEE reserved multicast frames 
     *  (not otherwise enumerated) to the CPU: 
     *  FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  Note that the ''FM_TRAP_IEEE_OTHER'' switch attribute must be
     *  enabled for this attribute to be effective.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TRAP_IEEE_OTHER,

    /** Type fm_bool: Whether 802.1x frames should be trapped to the CPU:
     *  FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  Note that the ''FM_TRAP_IEEE_8021X'' switch attribute must be
     *  enabled for this attribute to be effective.
     *
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_TRAP_IEEE_8021X,

    /** Type ''fm_eyeRecoveryMode'': Low eye score recovery mechanism to use
     *  when DFE is enabled. The default value is
     *  ''FM_PORT_LOW_EYE_SCORE_SOFT_RECOVERY''. See ''fm_eyeRecoveryMode''
     *  for a description of all possible values.
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_LOW_EYE_SCORE_RECOVERY,

    /** Type fm_int: Low eye score recovery threshold, i.e., the minimum value
     *  of the eye height below which recovery is attempted. The recovery
     *  mechanism is configured with the ''FM_PORT_LOW_EYE_SCORE_RECOVERY''
     *  attribute. The attribute value may range from 0 to 63. The default
     *  value is ''FM_PORT_LOW_EYE_SCORE_DEFAULT_THRESHOLD''.
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_LOW_EYE_SCORE_THRESHOLD,

    /** Type fm_int: Low eye score recovery timeout in seconds, i.e., the
     *  time interval after which recovery is attempted. The attribute
     *  value may range from 0 to 60. The default value is 5.
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_LOW_EYE_SCORE_TIMEOUT,

    /** Type fm_int: Signal detect debounce time in 1/4 of seconds.
     *  This is the minimum time interval after DFE tuning was
     *  stopped to re-enable signal detect. The attribute
     *  value may range from 0 to 16 (0 to 4 seconds). The default
     *  value is 3 (750ms).
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_SIGNAL_DETECT_DEBOUNCE_TIME,

    /** Type fm_bool: Whether to generate ingress and egress timestamps
     *  for this port: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  When a frame is received, the switch will store a high-precision
     *  ingress timestamp in packet memory, overwriting the FCS. The
     *  timestamp will be sent to the API if the frame is trapped.
     *                                                                  \lb\lb
     *  When a frame is transmitted with the egress timestamp request
     *  bit set, the switch will store the ingress and egress timestamps
     *  in the MAC_1588_STATUS register, and then signal an
     *  EgressTimeStamp interrupt on the port.
     *
     *  \portType6K ETH
     *  \portType10K ETH, LAG
     *  \chips FM6000, FM10000 */
    FM_PORT_TIMESTAMP_GENERATION,

    /** Type fm_bool: Whether to handle egress timestamp interrupts for
     *  this port: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  When an EgressTimeStamp interrupt occurs, the interrupt handler
     *  will read the ingress and egress timestamps from the
     *  MAC_1588_STATUS register and reset the status register,
     *  allowing another pair of timestamps to be captured.
     *                                                                  \lb\lb
     *  On FM6000, timestamps are delivered to the application as
     *  ''FM_EVENT_EGRESS_TIMESTAMP'' events.
     *                                                                  \lb\lb
     *  On FM10000, timestamps are delivered to the Host Driver, which will
     *  communicate them to the application via the network interface.
     *                                                                  \lb\lb
     *  Note that ''FM_PORT_TIMESTAMP_GENERATION'' must also be enabled
     *  for this attribute to be effective.
     *  
     *  \portType6K ETH
     *  \portType10K ETH, LAG
     *  \chips FM6000 FM10000 */
    FM_PORT_EGRESS_TIMESTAMP_EVENTS,

    /** Type ''fm_txFcsMode'': Transmit FCS processing mode.
     *  The default value is ''FM_FCS_REPLACE_NORMAL''.
     *
     *  \portType ETH
     *  \chips  FM6000, FM10000 */
    FM_PORT_TX_FCS_MODE,

    /** Type fm_uint32: Drain or hold all packets.
     *                                                                  \lb\lb
     *  FM_PORT_TX_DRAIN_ALWAYS (FM6000 only) - Port drains all packets
     *  regardless of link status. The drain is applied immediately and may
     *  cut short an in-flight frame.
     *                                                                  \lb\lb
     *  FM_PORT_TX_HOLD_ALWAYS - Port holds all packets regardless of link
     *  status. The hold is applied cleanly at the beginning of a frame.
     *                                                                  \lb\lb
     *  FM_PORT_TX_DRAIN_ON_LINK_DOWN (default) - Port drains all packets
     *  automatically when link is down.
     *                                                                  \lb\lb
     *  FM_PORT_TX_HOLD_ON_LINK_DOWN (FM6000 only) - Port holds transmission
     *  when link is down.
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM4000, FM6000 */
    FM_PORT_TX_DRAIN_MODE,

    /** Type fm_uint32: Drain or hold all packets.
     *                                                                  \lb\lb
     *  FM_PORT_RX_DRAIN (default) - Normal operation.
     *                                                                  \lb\lb
     *  FM_PORT_RX_HOLD - Drain all packets. Do not forward packets to the
     *  switch.
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM4000, FM6000 */
    FM_PORT_RX_DRAIN_MODE,

    /** Type fm_bool: Specifies whether an ARP packet with DMAC equal to
     *  FF:FF:FF:FF:FF:FF or to one of the router MACs and received on
     *  a VLAN for which FM_VLAN_ARP_LOGGING is set to FM_ENABLED should be
     *  logged to the CPU. Value is FM_ENABLED (default) or FM_DISABLED.
     *                                                                  \lb\lb
     *  \portType ETH, LAG
     *  \chips FM6000 */
    FM_PORT_LOG_ARP,

    /** Type fm_bool: Whether Virtual Network endpoint tags should be
     *  removed on egress frames: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Note that ''FM_SWITCH_PARSE_VNTAG'' must be enabled for this
     *  attribute to have any effect, and the ''FM_PORT_ISL_TAG_FORMAT''
     *  port attribute must be set to ''FM_ISL_TAG_NONE''. VNTAGs will be
     *  stripped implicitly if VNTAG parsing is enabled on the switch and
     *  ISL tagging is enabled on the port.
     *  
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_STRIP_VNTAG,

    /** Type: fm_bool. Whether this port should drop frames due to parsing
     *  error or a L3 checksum error: FM_ENABLED (default) or FM_DISABLED. 
     *  
     *  \portType ETH, LAG
     *  \chips  FM6000 */
    FM_PORT_DROP_PARSER_ERR,

    /** Type: fm_bool. Whether transmitter should be enabled on this port
     *  when it is operating in TX2RX loopback mode: FM_DISABLED (default)
     *  or FM_ENABLED. Note that this attribute does not take effect until 
     *  ''FM_PORT_LOOPBACK'' is set to FM_PORT_LOOPBACK_TX2RX.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_TX_ON_IN_LOOPBACK,

    /** UNPUBLISHED: Type: fm_bool. Experimental attribute. For internal use
     *  only: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Note that this attribute only applies to the TRILL microcode.
     *  
     *  \portType ETH
     *  \chips FM6000 */
    FM_PORT_APPLY_VLAN_TRANSLATION,

    /** UNPUBLISHED: Type: fm_bool. Experimental attribute. For internal use
     *  only: FM_ENABLED or FM_DISABLED (default).
     *  
     *  \portType ETH
     *  \chips FM6000 */
    FM_PORT_SWITCHING_NEXTHOP_VID,

    /** Type fm_int: Offset compensation for receiver channel A. The attribute
     *  value may range from -15 to 15 in 4mV units. The default value is 0
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_RX_OFFSET_A_COMPENSATION,

    /** Type fm_int: Offset compensation for receiver channel B. The
     *  attribute value may range from -15 to 15 in 4mV units. The default
     *  value is 0
     *                                                                  \lb\lb
     *  This attribute is applied per-lane per-MAC.
     *
     *  \portType ETH
     *  \chips  FM6000 */
    FM_PORT_RX_OFFSET_B_COMPENSATION,

    /** Type fm_int: Specifies which VLAN Tags are enabled for VLAN1. Value
     *  is a bitmask to select which of the four tags defined in
     *  ''FM_SWITCH_PARSER_VLAN_ETYPES'' are supported as the VLAN1 tag.  
     *  The default value is 1.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_PARSER_VLAN1_TAG,

    /** Type fm_int: Specifies which VLAN Tags are enabled for VLAN2. Value
     *  is a bitmask to select which of the four tags defined in
     * ''FM_SWITCH_PARSER_VLAN_ETYPES'' are supported as the VLAN2 tag. The 
     *  default value is 0.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_PARSER_VLAN2_TAG,

    /** Type fm_int: An index that specifies one of the EtherTypes defined
     *  by ''FM_SWITCH_MODIFY_VLAN_ETYPES''. Frames are modified to have
     *  the corresponding VLAN1 Ethernet Type. The default value is 1.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_MODIFY_VLAN1_TAG,

    /** Type fm_int: An index that specifies one of the EtherTypes defined
     *  by ''FM_SWITCH_MODIFY_VLAN_ETYPES''. Frames are modified to have the
     *  corresponding VLAN2 Ethernet Type. The default value is 1.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_MODIFY_VLAN2_TAG,

    /** Type fm_uint32: Defines the maximum frame length if mirror truncation
     *  is asserted. Value range from 64 (default) to 192 Bytes. Note that the
     *  value must be a multiple of 4, otherwise it will be truncated to the
     *  closest lower 4 byte boundary. It is also possible to disable
     *  truncation on a per-port basis using special Value
     *  ''FM_PORT_DISABLE_TRUNCATION''.
     *
     *  \portType ETH, PCIE, TE, LPBK
     *  \chips  FM10000 */
    FM_PORT_MIRROR_TRUNC_SIZE,

    /** Type fm_bool: Specifies whether MPLS parsing is enabled. Default is
     *  FM_DISABLED. When MPLS parsing is enabled, MPLS packets are identified
     *  by comparing its EtherType against the EtherTypes configured in
     *  ''FM_SWITCH_MPLS_ETHER_TYPES''.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_PARSE_MPLS,

    /** Type fm_uint32: A bit mask indicating the fields that will be updated
     *  on a routed frame. The mask consists of the OR of any of the following
     *  fields:
     *                                                                  \lb\lb
     *  FM_PORT_ROUTED_FRAME_UPDATE_DMAC indicates DMAC update,
     *                                                                  \lb\lb
     *  FM_PORT_ROUTED_FRAME_UPDATE_SMAC indicates SMAC update,
     *                                                                  \lb\lb
     *  FM_PORT_ROUTED_FRAME_UPDATE_VLAN indicates VLAN update.
     *                                                                  \lb\lb
     *  The default value is (FM_PORT_ROUTED_FRAME_UPDATE_DMAC |
     *                        FM_PORT_ROUTED_FRAME_UPDATE_SMAC |
     *                        FM_PORT_ROUTED_FRAME_UPDATE_VLAN).
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_ROUTED_FRAME_UPDATE_FIELDS,

    /** Type fm_int: Specifies which custom tags are enabled for first occuring 
     *  tag. It is a bitmask to enable which of the 4 tags defined in 
     *  ''FM_SWITCH_PARSER_CUSTOM_TAG'' are supported as the first tag. See 
     *  switch attribute ''FM_SWITCH_PARSER_CUSTOM_TAG'' for setting fields of
     *  global custom tags.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     * \chips FM10000 */
    FM_PORT_PARSER_FIRST_CUSTOM_TAG,

    /** Type fm_int: Specifies which custom tags are enabled for second tag. It
     *  is a bitmask to enable which of the 4 tags defined in 
     *  ''FM_SWITCH_PARSER_CUSTOM_TAG'' are supported as the second tag.See 
     *  switch attribute ''FM_SWITCH_PARSER_CUSTOM_TAG'' for setting fields of
     *  global custom tags.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     * \chips FM10000 */
    FM_PORT_PARSER_SECOND_CUSTOM_TAG,

    /** Type fm_int: Specifies the number of 16-bit words in the outermost MPLS
     *  tags to save for use in ACL. ACL condition
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT'' can be used to match against
     *  the saved MPLS tags. The attribute value may range from 0 to
     *  ''FM_MAX_NUM_HALF_WORDS_MPLS_TAG''. Default value is 0.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_PARSER_STORE_MPLS,

    /** Type fm_uint32: The maximum number of TCN FIFO entries this port may
     *  use at any given time. The attribute value may range from 0 to 511,
     *  with 10 being the default. A value of 10 should ensure space for
     *  all ports in a 48-port configuration.
     *  
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips  FM10000 */
    FM_PORT_TCN_FIFO_WM,


    /** Type fm_bool: Whether to enable Energy Efficient Ethernet mode on
     *  the port. Defaults to FM_DISABLED on all ports.
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_EEE_MODE,

    /** Type fm_int: The Energy Efficient Ethernet state of the port. This
     *  attribute is read-only.
     *                                                                  \lb\lb
     *  FM_PORT_EEE_NORMAL indicates normal mode.
     *                                                                  \lb\lb
     *  FM_PORT_EEE_RX_LPI indicates that the receiver is in Low Power Idle
     *  mode.
     *                                                                  \lb\lb
     *  FM_PORT_EEE_TX_LPI indicates that the transmitter is in Low Power
     *  Idle mode.
     *                                                                  \lb\lb
     *  FM_PORT_EEE_RX_TX_LPI indicates that both RX and TX are in Low Power
     *  Idle.
     *
     *  \portType ETH:ro
     *  \chips  FM10000 */
    FM_PORT_EEE_STATE,

    /** Type fm_uint32: The time interval in microseconds the transmitter
     *  has to be idle on a given port before the port enters EEE low-power
     *  mode and starts sending /LI/ codes to the link partner. It defaults
     *  to 1000 usec (1 msec).
     *                                                                  \lb\lb
     *  The value set for this attribute will be rounded down to the nearest 
     *  granularity provided by the hardware, which is a multiple of 10usec.
     *  The valid values are from 0 to 2550.
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_EEE_TX_ACTIVITY_TIMEOUT,

    /** Type fm_uint32: The time interval in microseconds the transmitter
     *  waits before starting a new frame transmission upon exiting EEE
     *  low-power mode. It defaults to 180 usec. Note that an extra 20 usec
     *  is added to this timer. That extra time is related to the
     *  TxLpiHoldTimeout, which is hard coded to 20 usec.
     *  The valid values are from 0 to 255.
     *
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_EEE_TX_LPI_TIMEOUT,

    /** Type ''fm_pepMode'': The current configuration of the PCIE Express
     *  endpoint the port belongs to. For the FM10000, the configured mode
     *  is determined during the boot sequence and remains static. Therefore
     *  it is read-only from an API perspective.
     *                                                                   \lb\lb
     *  See ''fm_pepMode'' for a description of the possible values for this
     *  attribute.
     *
     *  \portType PCIE:ro
     *  \chips  FM10000 */
    FM_PORT_PEP_MODE,

    /** Type ''fm_pcieMode'': The current PCIE link width negotiated during
     *  the LTSSM training procedure.
     *                                                                   \lb\lb
     *  See ''fm_pcieMode'' for a description of the possible values for
     *  this attribute.
     *
     *  \portType PCIE:ro
     *  \chips  FM10000 */
    FM_PORT_PCIE_MODE,

    /** Type fm_bool: When two VLAN tags are present and identical in a packet, 
     *  then this attribute defines which one is considered as first. Setting 
     *  to FM_ENABLED makes VLAN2 first, then VLAN1 second. Setting to 
     *  FM_DISABLED (default) makes VLAN1 first, VLAN2 second. 
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_PARSER_VLAN2_FIRST,

    /** Type fm_bool: Whether VID2 (VLAN ID in the second VLAN tag) is
     *  sent before VID1 (VLAN ID in the first VLAN tag) or after VID1.
     *  Setting to FM_ENABLED sends the VID2 first, then VID1 second.
     *  Setting to FM_DISABLED (default) sends the VID1 first, VID2 second.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_MODIFY_VID2_FIRST,

    /** Type fm_bool: Whether the ingress VLAN-tagged frame's VLAN fields
     *  should be replaced with the port's default VLAN fields, as specified
     *  by the ''FM_PORT_DEF_VLAN'', ''FM_PORT_DEF_PRI'', and
     *  ''FM_PORT_DEF_CFI'' attributes: FM_ENABLED or FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Note that this attribute does not affect the selection of the
     *  internal switch priority of the frame (in the case of SWPRI being
     *  chosen from VLAN priority), as SWPRI is chosen before the port's
     *  default VLAN values are applied. See ''FM_PORT_SWPRI_SOURCE'' and
     *  ''FM_PORT_SWPRI_DSCP_PREF'' for details on the selection of
     *  SWPRI.
     *
     *  \portType ETH, PCIE, TE, LPBK, LAG
     *  \chips FM10000 */
    FM_PORT_REPLACE_VLAN_FIELDS,

    /** Type ''fm_pcieSpeed'': The current PCIE clock speed negotiated during
     *  the LTSSM training procedure.
     *                                                                   \lb\lb
     *  See ''fm_pcieSpeed'' for a description of the possible values for
     *  this attribute.
     *
     *  \portType PCIE:ro
     *  \chips  FM10000 */
    FM_PORT_PCIE_SPEED,

    /** Type fm_bool: Whether to enable automatic detection of the module
     *  or cable type on the QSFP/SFP+ port.
     *                                                                  \lb\lb
     *  The API defaults to FM_DISABLED on all ports; however, the platform
     *  layer may change this default on system startup by setting the
     *  following property in the Liberty Trail configuration file:
     *  api.platform.config.switch.0.portIndex.1.ethernetMode text AUTODETECT
     *                                                                  \lb\lb
     *  When auto detection is enabled the ''FM_PORT_ETHERNET_INTERFACE_MODE''
     *  is automatically adjusted based on the module type detected:
     *                                                                  \lb\lb
     *  If the module is a 10/100/1000BASE-T module the Ethernet mode is set
     *  to ''FM_ETH_MODE_SGMII''.
     *                                                                  \lb\lb
     *  If the module is a 1G optical SFP module the Ethernet mode is set
     *  to ''FM_ETH_MODE_1000BASE_X''.
     *                                                                  \lb\lb
     *  If the module is a dual speed or 10G optical SFP+ module the Ethernet
     *  mode is set to ''FM_ETH_MODE_10GBASE_SR''.
     *                                                                  \lb\lb
     *  If the module is a 10G SFP+ Direct Attach Cable the Ethernet mode is
     *  set to ''FM_ETH_MODE_10GBASE_CR''.
     *                                                                  \lb\lb
     *  If the module is a 40G QSFP Direct Attach Cable the Ethernet mode is
     *  set to ''FM_ETH_MODE_AN_73'' with 40GBASE-CR4 advertised.
     *                                                                  \lb\lb
     *  If the module is a 100G QSFP28 Direct Attach Cable the Ethernet mode
     *  is set to ''FM_ETH_MODE_AN_73'' with 100GBASE-CR4 and 40GBASE-CR4
     *  advertised. Note the advertised mode can be limited to 40GBASE-CR4 by
     *  setting this property:
     *  api.platform.config.switch.0.portIndex.1.an73Ability text 40GBase-CR4
     *                                                                  \lb\lb
     *  If the module is a 40G optical module or 40G Active Optical Cable
     *  the Ethernet mode is set to ''FM_ETH_MODE_40GBASE_SR4''.
     *                                                                  \lb\lb
     *  If the module is a 100G optical module or 100G Active Optical Cable
     *  the Ethernet mode is set to ''FM_ETH_MODE_100GBASE_SR4''.
     *                                                                  \lb\lb
     *  \portType ETH
     *  \chips  FM10000 */
    FM_PORT_AUTODETECT_MODULE,

    /** UNPUBLISHED: For internal use only. */
    FM_PORT_ATTRIBUTE_MAX

};  /* end enum _fm_portAttr */


/**************************************************
 * For FM_PORT_SECURITY. 
 *  
 * Port security configuration. 
 **************************************************/
/* Turn off all source address security checks. */
#define FM_PORT_SECURITY_OFF                0

/* Consider an unknown source address to be a security violation. */
#define FM_PORT_SECURITY_SAV                1

/* Consider an unknown source address or a known source address
 * previously learned on another port (station move) to be a
 * security violation. */
#define FM_PORT_SECURITY_SHV                2

/* Consider a known source address on a port other than the port it was
 * learned on to be a security violation. (non FM2000 devices only) */
#define FM_PORT_SECURITY_SPV                3


/**************************************************/
/** \ingroup typeEnum 
 * Port security action. Specifies the action to 
 * be taken if a port receives a frame with an 
 * unknown SMAC address. Used as the value of the 
 * ''FM_PORT_SECURITY_ACTION'' port attribute. 
 **************************************************/
typedef enum
{
    /** No security action taken. The SMAC address is learned, and the frame
     *  is forwarded normally. This value essentially disables port security
     *  on the port. (default) */
    FM_PORT_SECURITY_ACTION_NONE,

    /** The violating frame is silently dropped. */
    FM_PORT_SECURITY_ACTION_DROP,

    /** The violating frame is dropped, and the SMAC address is reported
     *  to the application, via a security violation event, for
     *  authentication. */
    FM_PORT_SECURITY_ACTION_EVENT,

    /** The violating frame is trapped to the application for
     *  authentication. */
    FM_PORT_SECURITY_ACTION_TRAP,

    /** UNPUBLISHED: For internal use only. */
    FM_PORT_SECURITY_ACTION_MAX

} fm_portSecurityAction;


/**************************************************
 * For FM_PORT_SECURITY_TRAP. 
 *  
 * What to do with frames that incur a security 
 * violation. 
 **************************************************/
/* Silently discard the frame. */
#define FM_SECURITY_DISCARD                 0

/* Trap the frame and send it to the CPU. */
#define FM_SECURITY_SEND2CPU                1


/**************************************************
 * For FM_PORT_SECURITY_PRIORITY. 
 *  
 * Priority of frames that incur a security violation 
 * and are trapped to the CPU. 
 **************************************************/
/* Leave the frame priority unchanged. */
#define FM_SECURITY_PRI_NORMAL              0

/* Remap frame to switch priority 15. */
#define FM_SECURITY_PRI_HIGH                1


/**************************************************/
/** Port Tagging Options
 *  \ingroup constPortTagOptions
 *  \page portTagOptions
 *
 **************************************************/
/** \ingroup constPortTagOptions
 * @{ */

/** FM_TAG_KEEP (default) to tag only untagged frames.
 *                                                                      \lb\lb
 * For FM10000, this option is only valid for VLAN2 and is the default value.
 *                                                                      \lb\lb
 *  \chips  FM2000, FM3000, FM4000, FM6000 */
#define FM_TAG_KEEP                         0

/** Treat the incoming frame as if it is untagged for the purpose of
 *  VLAN1 association and tagging. The frame is associated with the
 *  per-port VLAN1 default. If the frame leaves the switch tagged in
 *  802.1Q mode, it gets an additional VLAN1 tag. If the frame leaves the
 *  switch untagged in 802.1Q mode, then any original VLAN1 is preserved,
 *  but this tag is not added.
 *                                                                      \lb\lb
 *  \chips  FM2000, FM3000, FM4000, FM6000 */
#define FM_TAG_ADD                          1

/** Overwrite all frames with the port's default VLAN1 and VPRI1.
 *                                                                  \lb\lb
 *  Note that the port's default VLAN1 is set with the ''FM_PORT_DEF_VLAN''
 *  attribute and the default VPRI1 is set with the ''FM_PORT_DEF_PRI''
 *  attribute.
 *                                                                      \lb\lb
 *  \chips  FM2000, FM3000, FM4000, FM6000 */
#define FM_TAG_REPLACE                      2

/** Treat the incoming frame as untagged but do not associate the port default
*   vlan1 and vpri1 with the frame. In FM3000 and FM4000 devices, this
*   allows the vlan1 tag to be passed into the FFU for processing, so a
*   new vlan1 and vpri1 can be assigned.
 *                                                                      \lb\lb
 *  \chips  FM2000, FM3000, FM4000, FM6000 */
#define FM_TAG_ADD_NO_DEFAULT               3

/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_PORT_TAGGING_MODE'' port attribute.
 **************************************************/
typedef enum
{
    /** Tagging Mode 802.1Q. Frames will egress with or without a VID1 VLAN
     *  tag, on a per-vlan basis, based upon the tag parameter provided in a
     *  call to ''fmAddVlanPort'' or ''fmChangeVlanPort''. A VID2 VLAN tag
     *  will only be present if one appeared in the frame on ingress. This
     *  is the default value. */
    FM_PORT_TAGGING_8021Q,

    /** Tagging Mode 802.1AD Customer Port. Frames will egress with or
     *  without a VID2 VLAN tag, on a per-VLAN basis, based upon the tag
     *  parameter provided in a call to ''fmAddVlanPort'' or
     *  ''fmChangeVlanPort'', or if a VID2 VLAN tag was present on
     *  ingress. A VID1 VLAN tag will never be present. */
    FM_PORT_TAGGING_8021AD_CUST,

    /** Tagging Mode 802.1AD Provider Port. Frames will always egress with a
     *  VID1 VLAN tag. Frames will egress with or without a VID2 VLAN tag,
     *  on a per-VLAN basis, based upon the tag parameter provided in a call
     *  to ''fmAddVlanPort'' or ''fmChangeVlanPort'', or if a VID2 VLAN
     *  tag was present on ingress. */
    FM_PORT_TAGGING_8021AD_PROV,

    /** Tagging Mode Pseudo-1. Frames will egress with or without a VID1
     *  VLAN tag, on a per-VLAN basis, based upon the tag parameter provided
     *  in a call to ''fmAddVlanPort'' or ''fmChangeVlanPort''. A VID2 VLAN
     *  tag will always be present. */
    FM_PORT_TAGGING_PSEUDO1,

    /** Tagging Mode Pseudo-2. Frames will egress with or without a VID1
     *  VLAN tag, on a per-VLAN basis, based upon the tag parameter provided
     *  in a call to ''fmAddVlanPort'' or ''fmChangeVlanPort''. A VID2 VLAN
     *  tag will never be present. */
    FM_PORT_TAGGING_PSEUDO2,

    /** UNPUBLISHED: For internal use only. Must be last. */
    FM_PORT_TAGGING_MAX,

} fm_portTaggingMode;


/**************************************************
 * For FM_PORT_LOOPBACK
 **************************************************/
#define FM_PORT_LOOPBACK_OFF               0
#define FM_PORT_LOOPBACK_TX2RX             1
#define FM_PORT_LOOPBACK_RX2TX             2
#define FM_PORT_LOOPBACK_UNKNOWN           -1


/**************************************************
 * For FM_PORT_RX_LANE_ORDERING
 * and FM_PORT_TX_LANE_ORDERING
 **************************************************/
#define FM_PORT_LANE_ORDERING_NORMAL       0
#define FM_PORT_LANE_ORDERING_INVERT       1

/**************************************************
 * For FM_PORT_RX_LANE_POLARITY
 * and FM_PORT_TX_LANE_POLARITY
 **************************************************/
#define FM_PORT_LANE_POLARITY_NORMAL       0x00
#define FM_PORT_LANE_POLARITY_INVERT       0x0F


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_PORT_DOT1X_STATE'' port attribute.
 **************************************************/
typedef enum
{
    /** Not participating in 802.1x. */
    FM_DOT1X_STATE_NOT_ACTIVE = 0,

    /** Indicate not 802.1x authorized. */
    FM_DOT1X_STATE_NOT_AUTH,

    /** Indicate 802.1x authorized. */
    FM_DOT1X_STATE_AUTH

} fm_dot1xState;


/**************************************************
 * For FM_PORT_TX_PAUSE_MODE
 **************************************************/
#define FM_PORT_TX_PAUSE_NORMAL            0
#define FM_PORT_TX_PAUSE_CLASS_BASED       1


/**************************************************
 * For FM_PORT_RX_PAUSE_MODE (deprecated)
 **************************************************/
#define FM_PORT_RX_PAUSE_NORMAL            0
#define FM_PORT_RX_PAUSE_CLASS_BASED       1


/**************************************************
 * For FM_PORT_RXCFI
 **************************************************/
#define FM_PORT_RXCFI_IGNORED              0
#define FM_PORT_RXCFI_ISPRIBIT             1


/**************************************************
 * For FM_PORT_TXCFI
 **************************************************/
#define FM_PORT_TXCFI_ISPRIBIT             0
#define FM_PORT_TXCFI_ASIS                 1


/***************************************************
 * For FM_PORT_SWPRI_SOURCE 
 *  
 * Note that the order of these parameters should 
 * not be changed (order important for FM4000 port 
 * attributes) 
 **************************************************/
#define FM_PORT_SWPRI_VPRI1                (1 << 0)
#define FM_PORT_SWPRI_DSCP                 (1 << 1)
#define FM_PORT_SWPRI_ISL_TAG              (1 << 2)
#define FM_PORT_SWPRI_VPRI2                (1 << 3)
#define FM_PORT_SWPRI_VPRI                FM_PORT_SWPRI_VPRI1

/**************************************************
 * For FM_PORT_PARSER
 **************************************************/
/* the conditions are as follows */
#define FM_PORT_PARSER_STOP_AFTER_L2       0
#define FM_PORT_PARSER_STOP_AFTER_L3       1
#define FM_PORT_PARSER_STOP_AFTER_L4       2
#define FM_PORT_PARSER_PARSE_ALL           3


/**************************************************
 * For FM_PORT_PARSER_FLAG
 **************************************************/
#define FM_PORT_PARSER_FLAG_IPV4_OPTIONS   (1 << 0)
#define FM_PORT_PARSER_FLAG_IPV6_ROUTING   (1 << 1)
#define FM_PORT_PARSER_FLAG_IPV6_FRAGMENT  (1 << 2)
#define FM_PORT_PARSER_FLAG_IPV6_HOPBYHOP  (1 << 3)
#define FM_PORT_PARSER_FLAG_IPV6_AUTH      (1 << 4)
#define FM_PORT_PARSER_FLAG_IPV6_DEST      (1 << 5)


/**************************************************
 * For FM_PORT_SMP_LOSSLESS_PAUSE
 **************************************************/
#define FM_PORT_SMP0                       (1 << 0)
#define FM_PORT_SMP1                       (1 << 1)
#define FM_PORT_SMP_ALL         (FM_PORT_SMP0 | FM_PORT_SMP1)


/**************************************************
 * For FM_PORT_ROUTED_FRAME_UPDATE_FIELDS
 **************************************************/
#define FM_PORT_ROUTED_FRAME_UPDATE_DMAC   (1 << 0)
#define FM_PORT_ROUTED_FRAME_UPDATE_SMAC   (1 << 1)
#define FM_PORT_ROUTED_FRAME_UPDATE_VLAN   (1 << 2)

#define FM_PORT_ROUTED_FRAME_UPDATE_ALL         \
        (FM_PORT_ROUTED_FRAME_UPDATE_DMAC   |   \
         FM_PORT_ROUTED_FRAME_UPDATE_SMAC   |   \
         FM_PORT_ROUTED_FRAME_UPDATE_VLAN)


/**************************************************
 * For FM_PORT_AUTONEG
 **************************************************/
enum
{
    FM_PORT_AUTONEG_NONE = 0,
    FM_PORT_AUTONEG_SGMII,
    FM_PORT_AUTONEG_CLAUSE_37,
    FM_PORT_AUTONEG_CLAUSE_73,
    FM_PORT_AUTONEG_ANNOUNCE_10G,
    FM_PORT_AUTONEG_ANNOUNCE_2500M,
    FM_PORT_AUTONEG_ANNOUNCE_1000M,
    FM_PORT_AUTONEG_ANNOUNCE_100M,
    FM_PORT_AUTONEG_ANNOUNCE_10M,
    FM_PORT_AUTONEG_ANNOUNCE_FLOW_CONTROL,

};

/**************************************************
 * For FM_PORT_TX_DRAIN_MODE
 **************************************************/
#define FM_PORT_TX_DRAIN_ALWAYS             0
#define FM_PORT_TX_DRAIN_ON_LINK_DOWN       1
#define FM_PORT_TX_HOLD_ON_LINK_DOWN        2
#define FM_PORT_TX_HOLD_ALWAYS              3

/**************************************************
 * For FM_PORT_RX_DRAIN_MODE
 **************************************************/
#define FM_PORT_RX_DRAIN             0
#define FM_PORT_RX_HOLD              1

/***************************************************
 * For FM_PORT_EEE_STATE 
 **************************************************/
#define FM_PORT_EEE_NORMAL      0
#define FM_PORT_EEE_RX_LPI      1
#define FM_PORT_EEE_TX_LPI      2
#define FM_PORT_EEE_RX_TX_LPI   3

/***************************************************
 * For FM_PORT_ISL_TAG_FORMAT
 **************************************************/

/***************************************************
 * For FM_PORT_AUTONEG_25G_NEXTPAGE_OUI 
 * The default OUI is the one defined by the 
 * 25G-Consortium, draft 1.5 
 **************************************************/
#define FM_PORT_25GAN_DEFAULT_OUI   0x6a737d


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_PORT_ISL_TAG_FORMAT'' port attribute.
 **************************************************/
typedef enum
{
    /** Indicates that no ISL tag is present.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ISL_TAG_NONE = 0,

    /** Indicates the presence of an Intel 32-bit Inter-Switch Link tag.
     *  
     *  \chips  FM3000, FM4000 */
    FM_ISL_TAG_F32,

    /** Indicates the presence of an Intel 56-bit Inter-Switch Link tag.
     * 
     *  \chips  FM6000, FM10000 */
    FM_ISL_TAG_F56,

    /** Indicates the presence of an Intel 64-bit Inter-Switch Link tag.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_ISL_TAG_F64,

    /** Indicates the presence of an Intel 96-bit Inter-Switch Link tag.
     *  
     *  \chips  FM3000, FM4000 */
    FM_ISL_TAG_F96,

    /** Indicates the presence of a non-Intel 32-bit tag in place of
     *  the first VLAN tag.
     *  
     *  \chips  FM3000, FM4000 */
    FM_ISL_TAG_OTHER_32B,

    /** Indicates the presence of a non-Intel 64-bit tag in place of
     *  the first VLAN tag.
     *  
     *  \chips  FM3000, FM4000 */
    FM_ISL_TAG_OTHER_64B,

    /** Indicates the presence of a non-Intel 96-bit tag in place of
     *  the first VLAN tag.
     *  
     *  \chips  FM3000, FM4000 */
    FM_ISL_TAG_OTHER_96B

} fm_islTagFormat;


/****************************************************************************/
/** \ingroup constLagAttr
 *
 * Link aggregation group attributes, used as an argument to ''fmGetLAGAttribute''
 * and ''fmSetLAGAttribute''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 * Unless otherwise indicated, the attribute index argument to 
 * ''fmGetLAGAttribute'' and ''fmSetLAGAttribute'' is not used.
 ****************************************************************************/
enum _fm_lagAttr
{
    /** Type fm_uint32: Indicates how LACP frames should be handled:        
     *                                                                  \lb\lb
     *  FM_PDU_TOCPU to trap LACP frames to the CPU and forward them to the
     *  application,                                                        
     *                                                                  \lb\lb
     *  FM_PDU_DISCARD (default) to discard LACP frames (note that they
     *  will still be trapped to the CPU, but discarded by the API),        
     *                                                                  \lb\lb
     *  FM_PDU_FORWARD to forward LACP frames through the switch without
     *  trapping them to the CPU.
     *                                                                  \lb\lb
     *  Note that this attribute interacts with the ''FM_LAG_MODE'' and
     *  ''FM_TRAP_IEEE_LACP'' switch attributes. Setting this attribute to
     *  FM_PDU_TOCPU is equivalent to setting ''FM_LAG_MODE'' to 
     *  ''FM_MODE_DYNAMIC''. Setting this attribute to FM_PDU_DISCARD is 
     *  equivalent to setting ''FM_LAG_MODE'' to ''FM_MODE_STATIC''. Setting 
     *  this attribute to FM_PDU_FORWARD is equivalent to setting ''FM_LAG_MODE''
     *  to ''FM_MODE_STATIC'' followed by setting ''FM_TRAP_IEEE_LACP'' to 
     *  FM_DISABLED. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_LAG_LACP_DISPOSITION,

    /** Type fm_uint32: Deprecated - Use the ''FM_L2_HASH_KEY'' switch 
     *  attribute instead.
     *                                                                  \lb\lb
     *  The link aggregation group hashing algorithm parameters
     *  in the form of a bit mask. The following bits may be ORed together to
     *  define the hashing algorithm.                                       
     *                                                                  \lb\lb
     *  FM_LAG_HASH_DA to include the destination MAC address. Setting
     *  FM_LAG_HASH_SYM overrides this bit to 1.                            
     *                                                                  \lb\lb
     *  FM_LAG_HASH_SA to include the source MAC address. Setting
     *  FM_LAG_HASH_SYM overrides this bit to 1.                            
     *                                                                  \lb\lb
     *  FM_LAG_HASH_TYPE to include the Ethernet type. If the Ethernet type
     *  is less than 0x600, it will be forced to zero for the hash so as not
     *  to hash over the length field. On FM2000 devices, this bit must be
     *  zero if FM_LAG_HASH_SRCPORT is used.                                
     *                                                                  \lb\lb
     *  FM_LAG_HASH_SRCPORT (FM2000 only) to include the source port. This
     *  bit must be zero if FM_LAG_HASH_TYPE is used. On revision A4 and
     *  earlier silicon, this bit must be zero if FM_LAG_HASH_SYM is used.  
     *                                                                  \lb\lb
     *  FM_LAG_HASH_VLANID to include the VLAN ID.                          
     *                                                                  \lb\lb
     *  FM_LAG_HASH_VLANPRI to include the VLAN priority.                   
     *                                                                  \lb\lb
     *  FM_LAG_HASH_SYM turns on symmetric hashing, overriding the settings
     *  of FM_LAG_HASH_SA and FM_LAG_HASH_DA.                               
     *                                                                  \lb\lb
     *  FM_LAG_HASH_L2IFIP (FM3000 and FM4000 devices only) include the L2 
     *  header in the hash function if the frame is IP. This bit has no 
     *  effect if the ''FM_PORT_PARSER'' port attribute is set to 
     *  FM_PORT_PARSER_STOP_AFTER_L2.
     *                                                                  \lb\lb
     *  FM_LAG_HASH_L34 (FM3000 and FM4000 devices only) include the Layer 
     *  3/4 hash, controlled by the ''FM_ROUTING_HASH'' switch attribute, 
     *  in the LAG hash. This bit has no effect if the ''FM_PORT_PARSER'' 
     *  port attribute is set to FM_PORT_PARSER_STOP_AFTER_L2.
     *                                                                  \lb\lb
     *  The default value for FM2000 devices is (FM_LAG_HASH_VLANPRI |
     *  FM_LAG_HASH_VLANID | FM_LAG_HASH_SA | FM_LAG_HASH_DA).
     *                                                                  \lb\lb
     *  The default value for FM3000 and FM4000 devices is 
     *  (FM_LAG_HASH_L2IFIP | FM_LAG_HASH_L34 | FM_LAG_HASH_SA | 
     *  FM_LAG_HASH_DA | FM_LAG_HASH_TYPE | FM_LAG_HASH_VLANPRI | 
     *  FM_LAG_HASH_VLANID). 
     *                                                                  \lb\lb
     *  Note: If neither FM_LAG_HASH_L2IFIP nor FM_LAG_HASH_L34 is set, which 
     *  is an invalid configuration, then FM_LAG_HASH_L2IFIP will be set by 
     *  the API.
     *  
     *  \chips  FM2000, FM3000, FM4000 */
    FM_LAG_HASH,

    /** Type fm_uint32: Specifies which of rotation A or rotation
     *  B to use for a given LAG.  The index specifies the LAG number.  A 
     *  value of 0 implies rotation A (default) and a value of 1 implies
     *  rotation B. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LAG_HASH_ROTATION,

    /** Type fm_uint32: Deprecated - Use the ''FM_L2_HASH_ROT_A'' switch 
     *  attribute instead.
     *                                                                  \lb\lb
     *  Specifies one of four 12-bit rotations (0 - 3) for use as Rotation A. 
     *  Default value is 1. 
     *
     *  \chips  FM3000, FM4000 */
    FM_LAG_HASH_ROTATION_A,

    /** Type fm_uint32: Deprecated - Use the ''FM_L2_HASH_ROT_B'' switch 
     *  attribute instead.
     *                                                                  \lb\lb
     *  Specifies one of four 12-bit rotations (0 - 3) for use as Rotation B. 
     *  Default value is 1. 
     *
     *  \chips  FM3000, FM4000 */
    FM_LAG_HASH_ROTATION_B,

    /** Type fm_uint32: Deprecated - Use the ''FM_L2_HASH_KEY'' switch 
     *  attribute instead.
     *                                                                  \lb\lb
     *  Specifies whether the FM4000 should
     *  hash in a manner that is compatible with the FM2000: FM_ENABLED or
     *  FM_DISABLED (default). Setting this attribute by itself is not enough
     *  to provide compatible hashing. See the datasheet for details. 
     *
     *  \chips  FM3000, FM4000 */
    FM_LAG_HASH_COMPATIBILITY,

    /** Type fm_bool: Whether LAG filtering is enabled: FM_ENABLED (default)
     *  or FM_DISABLED. The index specifies the LAG number.
     *                                                                  \lb\lb
     *  When disabled, packets will not be hashed when forwarded to the LAG.
     *  All LAG member ports will transmit a copy of the packet (similar to
     *  a multicast group).
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    FM_LAG_FILTERING,

    /** UNPUBLISHED: For internal use only. */
    FM_LAG_ATTRIBUTE_MAX

};  /* end enum _fm_lagAttr */


/**************************************************
 * For FM_LAG_LACP_DISPOSITION
 **************************************************/
#define FM_PDU_DISCARD                    0
#define FM_PDU_TOCPU                      1
#define FM_PDU_FORWARD                    2


/**************************************************
 * For FM_LAG_HASH
 **************************************************/
/* Note: by design, the following FM_LAG_HASH API constants
 * match the hardware implementation of this feature for the
 * FM2000. If you change these values, the code will break.
 */
#define FM_LAG_HASH_DA                    0x01
#define FM_LAG_HASH_SA                    0x02
#define FM_LAG_HASH_TYPE                  0x04
#define FM_LAG_HASH_SRCPORT               0x08
#define FM_LAG_HASH_VLANID                0x10
#define FM_LAG_HASH_VLANPRI               0x20
#define FM_LAG_HASH_SYM                   0x40
#define FM_LAG_HASH_L2IFIP                0x80
#define FM_LAG_HASH_L34                   0x100


/**************************************************
 * For FM_LAG_HASH_COMPATIBILITY
 **************************************************/
#define FM_LAG_HASH_COMPATIBILITY_FM2000  0x1


/**************************************************
 * For FM_LAG_HASH_ROTATION_A/B
 * Note: Apply to FM6000 only.
 **************************************************/

/* To select HASH_LAYER2_KEY_PROFILE[0].Value_HASH_ROT as rotation value. */
#define FM_LAG_HASH_L2_HASH_ROTATION        0

/* To select L3AR_HASH_ROTATION as rotation value. */
#define FM_LAG_HASH_L3_HASH_ROTATION        1

/* To select the pseudo random number as hash value. */
#define FM_LAG_HASH_RANDOM_HASH_VALUE       2


/****************************************************************************/
/** \ingroup constQosPortAttr
 *
 * Port QoS Attributes, used as an argument to ''fmSetPortQOS'' and
 * ''fmGetPortQOS''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 * Unless otherwise indicated, the attribute index argument to ''fmSetPortQOS''
 * and ''fmGetPortQOS'' is ignored.
 *                                                                      \lb\lb
 * Unless otherwise indicated, attributes are applicable to the CPU interface
 * port.
 ****************************************************************************/
enum _fm_qosPortAttr
{
    /** Type fm_uint32. The mapping of ingress VLAN1 priority to
     *  an internal 4-bit normalized form. The attribute index represents the
     *  ingress VLAN1 priority. The 4-bit attribute value is the internal
     *  normalized priority associated with the ingress VLAN1 priority. 
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_L2_VPRI1_MAP,

    /** Type fm_uint32. The mapping of ingress VLAN2 priority to
     *  an internal 4-bit normalized form. The attribute index represents the
     *  ingress VLAN2 priority. The 4-bit attribute value is the internal
     *  normalized priority associated with the ingress VLAN2 priority. 
     *
     *  \chips  FM6000 */
    FM_QOS_L2_VPRI2_MAP,

    /** Type fm_uint32. The mapping of QOS W4 header field to
     *  an internal 4-bit normalized form. The attribute index represents the
     *  the value of QOS.W4. The 4-bit attribute value is the internal
     *  normalized priority associated with the QOS.W4. 
     *
     *  \chips  FM6000 */
    FM_QOS_W4_MAP,

    /** Type fm_uint32: Associates a receive PAUSE frame PAUSE class with a
     *  traffic class. The value is the PAUSE class and may range from 0 to
     *  7. 
     *
     *  For FM6000 devices, the attribute index is the traffic class 
     *  and may range from 0 to 11.

     *  For FM10000 devices, the attribute index is the traffic class 
     *  and may range from 0 to 7.
     *                                                                  \lb\lb
     *  A PAUSE class may be mapped to more than one traffic class. When a 
     *  class-based PAUSE frame is received specifying that the PAUSE 
     *  Class be paused, all traffic classes mapped to that PAUSE class
     *  will be paused.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_TC_PC_MAP,
    
    /** Type fm_uint32: PAUSE class memory partition mapping. The attribute
     *  index is the PAUSE class and may range from 0 to 7.
     *
     *  For FM6000 devices, the attribute value is the RX memory partition 
     *  and may range from 0 to 11.
     *
     *  For FM10000 devices, the attribute value is the memory partition number
     *  which may range from 0 to (FM_MAX_MEMORY_PARTITIONS -1) and the value 
     *  can be also specified to FM_MAX_MEMORY_PARTITIONS (Default) to
     *  inactivates associated PAUSE class.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_PC_RXMP_MAP,

    /** Type fm_uint32: The mapping of frame ingress VLAN priority to internal
     *  priority. The attribute index represents the frame VLAN ingress priority.
     *  The 4-bit attribute value is the internal priority to associate with the
     *  frame priority.
     *                                                                  \lb\lb
     *  For FM2000 devices, the VLAN ingress priority (index) is 0 through 
     *  7 and the internal priority (value) is the switch priority. On FM2000 
     *  devices, the default internal priority is equal to the VLAN 
     *  priority. One exception is VLAN priority zero on the CPU interface 
     *  port, which is mapped to internal switch priority 15 to ensure that 
     *  slow protocol frames from the CPU are given highest priority.
     *                                                                  \lb\lb
     *  For devices other than FM2000, the VLAN ingress priority (index) 
     *  is 0 through 15, using the CFI bit as an additional bit of the 
     *  priority. The CFI bit is the low order bit of the 4-bit index. 
     *  The internal priority (value) is an internal VLAN priority code which 
     *  gets further mapped into a switch priority via the global 
     *  ''FM_QOS_VPRI_SWPRI_MAP'' switch QoS attribute. The default internal 
     *  priority is equal to the VLAN priority. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_RX_PRIORITY_MAP,

    /** Type fm_uint32: The mapping of frame internal priority to VLAN egress
     *  priority. The 4-bit attribute index represents the internal priority
     *  and the value represents the VLAN egress priority. The default values
     *  map switch priority 0 through 7 to VLAN priority 0 through 7,
     *  respectively and switch priority 8 through 15 to VLAN priority
     *  0 through 7, respectively.
     *                                                                  \lb\lb
     *  For FM2000 devices, the VLAN egress priority is 0 through 7 and
     *  the internal priority is the switch priority.
     *  This QoS attribute may not be set for the CPU interface port on the
     *  FM2000 family.
     *                                                                  \lb\lb
     *  For FM3000, FM4000 and FM10000 devices, the VLAN egress priority is 0
     *  through 7 if the ''FM_PORT_TXCFI'' is set to FM_PORT_TXCFI_ASIS or 0
     *  through 15 if the ''FM_PORT_TXCFI'' is set to FM_PORT_TXCFI_ISPRIBIT.
     *  The internal priority is a VLAN priority.
     *                                                                  \lb\lb
     *  This QoS attribute may be set for the CPU interface port on all
     *  devices except the FM2000. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM10000 */
    FM_QOS_TX_PRIORITY_MAP,

    /** Type fm_uint32: The mapping of frame internal priority to VLAN2 egress
     *  priority. The 4-bit attribute index represents the internal priority
     *  and the value represents the VLAN2 egress priority. The default values
     *  map switch priority 0 through 7 to VLAN2 priority 0 through 7,
     *  respectively and switch priority 8 through 15 to VLAN2 priority
     *  0 through 7, respectively.
     *                                                                  \lb\lb
     *  The VLAN2 egress priority is 0 through 7 if the ''FM_PORT_TXCFI2'' is 
     *  set to FM_PORT_TXCFI_ASIS or 0 through 15 if the ''FM_PORT_TXCFI2'' 
     *  is set to FM_PORT_TXCFI_ISPRIBIT. The internal priority is a VLAN 
     *  priority.
     *                                                                  \lb\lb
     *  This QoS attribute may be set for the CPU interface port.
     *
     *  \chips  FM10000 */
    FM_QOS_TX_PRIORITY2_MAP,

    /** Type fm_bool: Indicates whether or not the switch
     *  should update the egress priority on a frame.  The index indicates
     *  the switch priority for which the egress priority can be updated. 
     *
     *  \chips  FM2000 */
    FM_QOS_TX_PRIORITY_UPDATE,

    /** Type fm_uint32: The hog transmit watermark in bytes. Frames are 
     *  dropped 100% at this watermark if TX private is used.
     *                                                                  \lb\lb
     *  Note that a frame causing the watermark to be exceeded is not dropped.
     *  The next frame considered for that queue is dropped since the watermark
     *  is now exceeded.
     *                                                                  \lb\lb
     *  For FM2000 devices, the attribute value may range from 0 to 
     *  1,047,552 with a default value of 261,120. The API will round the 
     *  specified value up to the nearest 1024-byte boundary. The attribute 
     *  index is not used. 
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 0 to 
     *  4,193,792 with a default value of 4,193,792. The API will round the
     *  specified value up to the nearest 512-byte boundary. The attribute 
     *  index (0 - 3) indicates which one of the four hog watermarks to set 
     *  (use the ''FM_QOS_TX_HOG_WM_MAP'' switch QoS attribute to map 
     *  traffic classes to one of these four).
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 0 to 7,863,840
     *  with a default value of 7,863,840.  The API will round the specified
     *  value up to the nearest 480-byte boundary.  The attribute index 
     *  (0 - 15) indicates which ISL priority to set the hog watermark for.
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to 6,291,264
     *  with a default value of 6,291,264.  The API will round the specified
     *  value up to the nearest 192-byte boundary.  The attribute index 
     *  (0 - 7) indicates which traffic class to set the hog watermark for.
     *                                                                  \lb\lb
     *  Because the specified number of bytes is rounded up by the API, the
     *  value returned by ''fmGetPortQOS'' may not exactly match the value
     *  set with ''fmSetPortQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API,
     *  however it can still be explicitly set by the application if
     *  required. This can be useful in some overbooked mirroring
     *  configurations to prevent the monitor port from consuming too much
     *  memory.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_TX_HOG_WM,

    /** Type fm_int: Deprecated in favor of ''FM_QOS_TX_HOG_WM''. See the 
     *  description of ''FM_QOS_TX_HOG_WM'' for details as the 
     *  functionality is indentical.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly will result in an
     *  ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000 */
    FM_QOS_TX_SHARED_WM,

    /** Type fm_uint32: The hog receive watermark in bytes. Frames are 
     *  dropped 100% at this watermark.
     *                                                                  \lb\lb
     *  Note that a frame causing the watermark to be exceeded is not dropped.
     *  The next frame considered for that queue is dropped since the watermark
     *  is now exceeded.
     *                                                                  \lb\lb
     *  For FM2000 devices, the attribute value may range from 0 to 
     *  1,047,552 with a default value of 261,120. The API will round the 
     *  specified value up to the nearest 1024-byte boundary. The attribute 
     *  index is not used. 
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 0 to 
     *  4,193,792 with a default value of 4,193,792. The API will round the
     *  specified value up to the nearest 512-byte boundary. The attribute 
     *  index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 0 to
     *  10,485,600 with a default value of 10,485,600. If the value is set 
     *  above the total amount of memory, the watermark is effectively disabled.  
     *  The API will round the specified value up to the nearest 160-byte 
     *  boundary. The attribute index (0 - 15) indicates the ISL priority.
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to
     *  6,291,264 with a default value of 6,291,264. If the value is set 
     *  above the total amount of memory, the watermark is effectively disabled.  
     *  The API will round the specified value up to the nearest 192-byte 
     *  boundary. The attribute index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  Because the specified number of bytes is rounded up by the API, the
     *  value returned by ''fmGetPortQOS'' may not exactly match the value
     *  set with ''fmSetPortQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_RX_HOG_WM,

    /** Type fm_int: Deprecated in favor of ''FM_QOS_RX_HOG_WM''. See the 
     *  description of ''FM_QOS_RX_HOG_WM'' for details as the functionality 
     *  is indentical.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly will result in an
     *  ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000 */
    FM_QOS_RX_SHARED_WM,

    /** Type fm_uint32: The private receive watermark in bytes. This memory is 
     *  protected from congestion management for unicast frames.
     *                                                                  \lb\lb
     *  For FM2000 devices, the attribute value may range from 0 to 
     *  1,047,552 with a default value of 16,384. The API will round the 
     *  specified value up to the nearest 1024-byte boundary. The attribute 
     *  index is not used. 
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 0 to 
     *  4,193,792 with a default value of 4,193,792. The API will round the
     *  specified value up to the nearest 512-byte boundary. The attribute 
     *  index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 0 to 10,485,600
     *  with a default value of 10,485,600. A value larger than the total 
     *  amount of memory effectively makes all memory private. The API will 
     *  round the specified value up to the nearest 160-byte boundary. The 
     *  attribute index (0 - 11) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to 
     *  6,291,264 with a default value of 6,291,264. The API will round the
     *  specified value up to the nearest 192-byte boundary. The attribute 
     *  index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  Because the specified number of bytes is rounded up by the API, the
     *  value returned by ''fmGetPortQOS'' may not exactly match the value
     *  set with ''fmSetPortQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_RX_PRIVATE_WM,

    /** Type fm_int: The private transmit watermark in bytes.
     *  The attribute value may range from 0 to 4,193,792 with a default value 
     *  of 4,193,792. The API will round up the specified attribute value to 
     *  the nearest 512-byte boundary, so the value returned by ''fmGetPortQOS'' 
     *  may not exactly match the value set with ''fmSetPortQOS''. The 
     *  attribute index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API.
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_TX_PRIVATE_WM,

    /** Type fm_uint32: The private transmit watermark per traffic
     *  class in bytes. The attribute index (0 - 7) indicates the traffic class.
     *
     *  For  FM3000 and FM4000 devices the attribute value and may range from
     *  0 to 4,193,792 with a default value of 4,193,792. The API will round 
     *  up the specified attribute value to the nearest 512-byte boundary
     *                                                                  \lb\lb
     *  For  FM10000 devices the attribute value and may range from 0 to 
     *  6,291,264 with a default value of 6,291,264. The API will round  up the 
     *  specified attribute value to the nearest 192-byte boundary
     *                                                                  \lb\lb
     *  Note: Because the specified number of bytes is rounded up by the API,
     *  so the value returned by ''fmGetPortQOS'' may not exactly match the 
     *  value set with ''fmSetPortQOS''.
     *  When the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API,
     *  however it can still be explicitly set by the application. In that
     *  case, other watermarks under the control of ''FM_AUTO_PAUSE_MODE''
     *  will be automatically recalculated. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_QOS_TX_TC_PRIVATE_WM,

    /** Type fm_uint32: Determines whether to enable soft drop when the TX
     *  private watermark is surpassed: FM_ENABLED or FM_DISABLED (default). 
     *  The attribute index is the ISL priority (0 - 15).
     *                                                                  \lb\lb
     *  This attribute only takes effect if ''FM_QOS_TX_SOFT_DROP_ON_RXMP_FREE'' 
     *  is not enabled and the RXMP usage is past the 
     *  ''FM_QOS_SHARED_SOFT_DROP_WM''. 
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_TX_SOFT_DROP_ON_PRIVATE,

    /** Type fm_uint32: Determines whether to enable soft drop when the TX
     *  passes half of the free RXMP usage: FM_ENABLED or FM_DISABLED (default).
     *  The attribute index is the ISL priority (0 - 15).
     *                                                                  \lb\lb
     *  This attribute only takes effect when the RXMP usage is past the 
     *  ''FM_QOS_SHARED_SOFT_DROP_WM''.
     *                                                                  \lb\lb
     *  This attribute is generally enabled in lossy memory partitions.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_TX_SOFT_DROP_ON_RXMP_FREE,

    /** Type fm_int: The global PAUSE on watermark in bytes. When 
     *  the occupancy of the global shared memory exceeds this value, 
     *  transmission of PAUSE frames is initiated out of the port. In addition, 
     *  the receive private watermark must be surpassed on any port before 
     *  PAUSE frames are generated. 
     *                                                                  \lb\lb
     *  The attribute value may range from 0 to 4,193,280 with a default value 
     *  of 4,193,280. The API will round up the specified attribute value to 
     *  the nearest 1024-byte boundary, so the value returned by ''fmGetPortQOS'' 
     *  may not exactly match the value set with ''fmSetPortQOS''. The 
     *  attribute index is not used.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly will result in an
     *  ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000 */
    FM_QOS_GLOBAL_PAUSE_ON_WM,

    /** Type fm_int: The global PAUSE off watermark in bytes. 
     *  When the occupancy of the global shared memory drops to this value, 
     *  PAUSE frames will no longer be transmitted out of the port.
     *                                                                  \lb\lb
     *  The attribute value may range from 0 to 4,193,280 with a default value 
     *  of 4,193,280. The API will round up the specified attribute value to 
     *  the nearest 1024-byte boundary, so the value returned by ''fmGetPortQOS'' 
     *  may not exactly match the value set with ''fmSetPortQOS''. The 
     *  attribute index is not used.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly will result in an
     *  ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000 */
    FM_QOS_GLOBAL_PAUSE_OFF_WM,

    /** Type fm_uint32: The private PAUSE on watermark in bytes. When the 
     *  occupancy of memory exceeds this watermark, the port will transmit 
     *  PAUSE frames.
     *                                                                  \lb\lb
     *  For FM2000 devices, the attribute value may range from 0 to 
     *  4,193,280 with a default value of 4,193,280. The API will round up 
     *  the specified attribute value to the nearest 1024-byte boundary. The
     *  attribute index is not used.
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 0 to 
     *  4,193,792 with a default value of 4,193,792. The API will round up 
     *  the specified attribute value to the nearest 512-byte boundary.
     *  The attribute index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 0 to 10,485,600
     *  with a default value of 10,485,600. If the value is set above the 
     *  total amount of memory, the watermark is effectively disabled. The API 
     *  will round up the specified attribute value to the nearest 160-byte 
     *  boundary. The attribute index (0 - 11) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to
     *  6,291,264 with a default value of 6,291,264. If the value is
     *  set above the total amount of memory, the watermark is
     *  effectively disabled. The API will round up the specified
     *  attribute value to the nearest 192-byte boundary. The
     *  attribute index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  Because the specified number of bytes is rounded up by the API, the
     *  value returned by ''fmGetPortQOS'' may not exactly match the value
     *  set with ''fmSetPortQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_PRIVATE_PAUSE_ON_WM,

    /** Type fm_uint32: The private PAUSE off watermark in bytes. When the 
     *  occupancy of memory drops to this value, the port will stop 
     *  transmitting PAUSE frames.
     *                                                                  \lb\lb
     *  For FM2000 devices, the attribute value may range from 0 to 
     *  4,193,280 with a default value of 250,880. The API will round up 
     *  the specified attribute value to the nearest 1024-byte boundary. The
     *  attribute index is not used.
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 0 to 
     *  4,193,792 with a default value of 4,193,792. The API will round up 
     *  the specified attribute value to the nearest 512-byte boundary.
     *  The attribute index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 480
     *  to 10,485,600 with a default value of 10,485,600.  The API
     *  will round up the specified attribute value to the nearest
     *  160-byte boundary.  The attribute index (0 - 11) indicates
     *  the memory partition.
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to 
     *  6,291,264 with a default value of 6,291,264. The API will round up 
     *  the specified attribute value to the nearest 192-byte boundary.
     *  The attribute index (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  Because the specified number of bytes is rounded up by the API, the
     *  value returned by ''fmGetPortQOS'' may not exactly match the value
     *  set with ''fmSetPortQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_PRIVATE_PAUSE_OFF_WM,

    /** Type fm_uint32: Specifies whether each RXMP for the given port may
     *  generate pause frames due to usage crossing 
     *  ''FM_QOS_SHARED_PAUSE_ON_WM'' and ''FM_QOS_SHARED_PAUSE_OFF_WM''.
     *                                                                  \lb\lb
     *  Each bit of the attribute value corresponds to the memory partition.
     *  Bit 0 is RXMP 0, bit 1 is RXMP 1, etc.  A bit value of 1 indicates
     *  that the port may generate a pause frame due to the shared watermarks
     *  and a value of 0 indicates that the port disregards shared watermarks.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SHARED_PAUSE_ENABLE,

    /** Type fm_int: The queue weighting for Weighted Round Robin
     *  queuing.  The attribute index represents the queue (0 - 3). The
     *  attribute value is the number of packets per turn for the
     *  associated queue and may range from 1 to 255. The default values for
     *  each queue are 1, 3, 7 and 15 for queues 0, 1, 2 and 3, respectively. 
     *
     *  \chips  FM2000 */
    FM_QOS_QUEUE_WEIGHT,

    /** Type fm_int: The service mode for Weighted Round Robin
     *  queuing. Value is:
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_RRPRIORITY (default) for priority round robin,
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_INTERLEAVE for interleaved (not supported),
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_RRPURE for pure round robin. 
     *
     *  \chips  FM2000 */
    FM_QOS_QUEUE_SERVICE,

    /** Type fm_int: This attribute indicates which queues operate
     *  in weighted round robin mode. Values are:
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_ALL_STRICT (default) to set all queues to strict
     *  priority,
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_TWO_STRICT for the two lowest priority queues to be
     *  weighted round robin,
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_ONE_STRICT for the lowest three queues to be weighted
     *  round robin,
     *                                                                  \lb\lb
     *  FM_QOS_QUEUE_ALL_WRR for all four queues to be weighted round robin.
     *                                                                  \lb\lb
     *  Any queues which are not WRR are strict priority. If they are weighted
     *  round robin, then the service order and weights are used to determine
     *  the scheduling. 
     *
     *  \chips  FM2000 */
    FM_QOS_QUEUE_MODE,

    /** Type fm_uint32: Maps traffic class to shaping group. The 
     *  attribute value is the shaping group number and defaults to 0.
     *  The attribute index is the traffic class.
     *                                                                  \lb\lb
     *  For FM3000, FM4000 and FM10000 devices, the shaping group value may 
     *  range from 0 to 7 and the traffic class index may range from 0 to 7.
     *                                                                  \lb\lb
     *  For FM6000 devices, the shaping group value may range from 0 to 11 
     *  and the traffic class index may range from 0 to 11.
     *                                                                  \lb\lb
     *  Note: ''FM_QOS_TC_GROUP_MAP'' is a legacy synonym for this attribute.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_TC_SHAPING_GROUP_MAP,

    /** Type fm_uint32: Defines groups of traffic classes that
     *  use a common a deficit counter for the purpose of delay-deficit
     *  round-robin scheduling. Classes within a scheduling group have strict
     *  priority relative to each other.
     *                                                                  \lb\lb
     *  The 8-bit attribute value is a bit mask, 1 bit per class. If a bit
     *  is set to 0 then the class belongs to the same group as the class to
     *  the left (the class with higher bit index). If this bit is 1, then the
     *  class is at the head of a new group. The default value is 0xFF, placing
     *  each class into its own separate group. 
     *
     *  \chips  FM3000, FM4000 */
    FM_QOS_SCHED_GROUPS,

    /** Type fm_uint32: Defines priority sets, clustering classes
     *  into sets of shared priority. Note that all classes in a scheduling
     *  group ( see ''FM_QOS_SCHED_GROUPS'') must belong to the same priority 
     *  set.
     *                                                                  \lb\lb
     *  The 8-bit attribute value is a bit mask, 1 bit per class. If a bit
     *  is set to 1, then this is the first class of the set, if the bit is
     *  set to 0, then this is not the first class of the set and the class
     *  remains part of the same priority set as the class to the left
     *  (higher bit index). the default value is 0xFF, placing each class into
     *  its own separate priority set. 
     *
     *  \chips  FM3000, FM4000 */
    FM_QOS_SCHED_PRI_SETS,

    /** Type fm_uint32: The cost in bytes added on a per-frame basis when 
     *  computing DRR credit deductions during dequeue. The value may
     *  range from 0 to 255. 
     *                                                                  \lb\lb
     *  For FM6000 devices default is 20. Setting this attribute overrides the 
     *  value of ''FM_QOS_SHAPING_GROUP_IFG_PENALTY''.
     *                                                                  \lb\lb
     *  For FM10000 devices default is 0.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SCHED_IFGPENALTY,

    /** Type fm_uint32: Defines the deficit round robin quantum
     *  for a traffic class group. This attribute is only used in reference
     *  to other round-robin classes of the same egress priority. The
     *  attribute value is the quantum and ranges from 0x000000 - 0xFFFFFF
     *  with a default value of 0x800. The attribute index is the group
     *  (0 - 7). 
     *
     *  \chips  FM3000, FM4000 */
    FM_QOS_TC_GROUP_WEIGHT,

    /** Type fm_uint32: Defines the priority mode for a traffic
     *  class. The attribute index is the traffic class (0 - 7) while the
     *  attribute value is one of the following:
     *                                                                  \lb\lb
     *  FM_QOS_TC_GROUP_MODE_STRICT (default) - all packets are drained unless
     *  the throughput limit is achieved,
     *                                                                  \lb\lb
     *  FM_QOS_TC_GROUP_MODE_BALANCED - packets are drained according to
     *  weight. 
     *
     *  \chips  FM3000, FM4000 */
    FM_QOS_TC_GROUP_MODE,

    /** Type fm_uint64: Specifies the maximum transmission rate (token
     *  bucket replenishment rate) for a shaping group. The attribute value
     *  is in units of bits per second, from 0 to (100 * 1e9), or may be set
     *  to FM_QOS_SHAPING_GROUP_RATE_DEFAULT to disable shaping (default).
     *  If the rate is greater than the physical port speed, this will also
     *  disable shaping.
     *  
     *  Note that the attribute value is converted to internal units, which
     *  means the value returned by ''fmGetPortQOS'' may not exactly match
     *  the value set with ''fmSetPortQOS''. The attribute index is the
     *  shaping group.
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the shaping group value may range 
     *  from 0 to 7.
     *                                                                  \lb\lb
     *  For FM6000 devices, the shaping group value may range from 0
     *  to 11.
     *                                                                  \lb\lb
     *  For FM10000 devices, the shaping group value may range from 0 to 7.
     *                                                                  \lb\lb
     *  Note: ''FM_QOS_TC_GROUP_RATE'' is a legacy synonym for this 
     *  attribute.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SHAPING_GROUP_RATE,
    
    /** Type fm_uint64: Defines the maximum transmission burst
     *  size (token bucket saturation value) for a shaping group. The
     *  attribute value is in units of bits. and may range from 0 to
     *  67,108,352 bits with a default value of 131,072 bits. The attribute
     *  index is the shaping group (0 - 7). 
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to 67,100,672
     *  bits with a default value of 67,100,672 bits. Note that the specified 
     *  number of bits is converted to internal units (with rounding), which 
     *  means the value returned by ''fmGetPortQOS'' may not exactly match 
     *  the value set with ''fmSetPortQOS''.
     *                                                                  \lb\lb
     *  Note: ''FM_QOS_TC_GROUP_MAX_BURST'' is a legacy synonym for this 
     *  attribute.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SHAPING_GROUP_MAX_BURST,
    
    /** Type fm_int: Defines the IFG penalty used to calculate the
     *  target rate. The value of this attribute may range from 0 to 255 and
     *  defaults to 0. This attribute will be overridden when 
     *  ''FM_QOS_SCHED_IFGPENALTY'' is set.
     *
     *  \chips  FM6000 */
    FM_QOS_SHAPING_GROUP_IFG_PENALTY,
    
    /** Type fm_uint32: Defines how the egress port will react
     *  for a Congestion Notification frame. Value is:
     *                                                                  \lb\lb
     *  FM_QOS_CN_REACTION_MODE_FORWARD to forward the frame on the port,
     *                                                                  \lb\lb
     *  FM_QOS_CN_REACTION_MODE_DISCARD (default) to discard the frame,
     *                                                                  \lb\lb
     *  FM_QOS_CN_REACTION_MODE_RATELIM to process the frame, using a rate
     *  limiter to control the number of PAUSE frames that will be sent
     *  if there is congestion. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_REACTION_MODE,

    /** Type fm_uin64: Defines the rate limit in bits per ssecond
     *  to apply when the port needs to be rate limited following capture of
     *  the Congestion Notification frame on egress. This attribute applies
     *  only if ''FM_QOS_CN_REACTION_MODE'' is set to 
     *  FM_QOS_CN_REACTION_MODE_RATELIM. The attribute value may range from 
     *  0 to 10,000,000,000 (10G) with a default value of approxamitely 
     *  2.5Gbps. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_REACTION_RATE,

    /** Type fm_uint32: Defines the lower 16 bits of the
     *  destination MAC address for Vitual-output-queue Congestion Notification
     *  frames on the port. The default value is 0xEEFF, which when combined
     *  with the switch-global upper 48 bits, defined with the 
     *  ''FM_QOS_CN_FRAME_DMAC'' switch QoS attribute, yields a default MAC 
     *  address of 00:EF:FE:FF:EE:FF. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_DMAC_SUFFIX,

    /** Type fm_uint32: Defines the memory usage at which
     *  point a congestion condition on the port is considered cleared for
     *  a memory partition. The attribute value is in units of 512-byte
     *  blocks and may range from 0 to 4095 with a default value of 0.
     *  The attribute index (0 - 1) indicates the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_MIN,

    /** Type fm_uint32: Defines the memory usage at which
     *  point a congestion condition on the port is considered to exist for
     *  a memory partition. The attribute value is in units of 512-byte
     *  blocks and may range from 0 to 4095 with a default value of 0.
     *  The attribute index (0 - 1) indicates the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_MAX,

    /** Type fm_uint32: Defines the FCN congestion notification
     *  equilibrum point memory usage on the port for a memory partition.
     *  The attribute value is in units of 512-byte blocks and may range from
     *  0 to 4095 with a default value of 0. The attribute index (0 - 1)
     *  indicates the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_EQ,

    /** Type fm_uint32: Defines the FCN congestion notification
     *  severe congestion point memory usage on the port for a memory partition.
     *  The attribute value is in units of 512-byte blocks and may range from
     *  0 to 4095 with a default value of 0. The attribute index (0 - 1)
     *  indicates the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_SC,

    /** Type fm_uint32: The amount of memory used by receive queues in bytes.
     *  This attribute is read-only.
     *                                                                  \lb\lb
     *  For FM2000 devices, the granularity of the measurement is 1024 bytes.
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the granularity of the measurement 
     *  is 512 bytes. 
     *                                                                  \lb\lb
     *  For FM6000 devices, the granularity of the measurement is 160 bytes.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_QOS_RX_USAGE,

    /** Type fm_uint32: The number of frames currently in receive queues for
     *  a particular RX memory partition. The attribute index (0 - 11) 
     *  indicates the memory partition. This attribute is read-only.
     *                                                                  \lb\lb
     *  \chips  FM6000 */
    FM_QOS_RX_USAGE_FRAMES,

    /** Type fm_uint32: The number of bytes of memory used by receive queues
     *  for a particular memory partition. This attribute is read-only. The 
     *  index indicates the memory partition. 
     *                                                                  \lb\lb
     *  On FM3000 and FM4000 devices, the index represents the SMP and
     *  may range from 0 to 1.
     *                                                                  \lb\lb
     *  On FM6000 devices, the index represents the RXMP and may range from
     *  0 to 11.
     *                                                                  \lb\lb
     *  On FM10000 devices, the index represents the SMP and may range 
     *  from 0 to 1.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_RX_SMP_USAGE,

    /** Type fm_uint32: The amount of memory in bytes used by transmit queues.
     *  This attribute is read-only.
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute index indicates the 
     *  memory partition and may range from 0 to 1. 
     *
     *  For FM6000 devices, the attribute index indicates the TX memory 
     *  partition and may range from 0 to 11.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000 */
    FM_QOS_TX_USAGE,

    /** Type fm_uint32: The amount of memory used by a traffic
     *  class in a port's transmit queue in units of bytes. This attribute is
     *  read-only. The attribute index (0 - 7) indicates the traffic class. 
     *
     *  For FM3000 and FM4000 devices, the granularity of the measurement 
     *  is 512 bytes.  
     *
     *  For FM10000 devices, the granularity of the measurement is 192 bytes.
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_QOS_TX_TC_USAGE,

    /** Type fm_uint32: The number of credits in the egress rate limiter.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_ERL_USAGE,

    /** Type fm_uint64: Defines the maximum reception
     *  rate (token bucket replenishment rate) for a port ingress
     *  rate limiter. The attribute value is in units of bits per
     *  second and may range from 0 to 10,000,000,000 (10Gbps) with
     *  a default value of 10Gbps. The attribute index (0 - 1) indicates
     *  the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_RX_RATE_LIM_RATE,

    /** Type fm_uint64: Defines the maximum reception
     *  burst size (token bucket saturation value) for a port ingress
     *  rate limiter. The attribute value is in units of bits. and
     *  may range from 0 to 67,108,352 bits with a default value of
     *  131,072 bits. The attribute index (0 - 1) indicates the
     *  memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_RX_RATE_LIM_MAX_BURST,

    /** Type fm_uint32: Defines the level of
     * the rate limiter token bucket at which PAUSE_OFF will be 
     * generated for an ingress port rate limiter. The attribute 
     * value is in units of bytes and may range from -32,768 to 32,767 
     * with a default value of 0, which means PAUSE will 
     * be sent when the configured rate has been reached. The 
     * attribute index (0 - 1) indicates the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */ 
    FM_QOS_RX_RATE_LIM_PAUSE_OFF_THRESHOLD,

    /** Type fm_uint32: Defines the level of
     * the rate limiter token bucket at which frames will be dropped 
     * for an ingress port rate limiter. The attribute value is in 
     * units of bytes and may range from -32,768 to 32,767 with a 
     * default value of 0, which means that frames will be dropped
     * when the configured rate has been reached. The attribute 
     * index (0 - 1) indicates the memory partition. 
     *
     *  \chips  FM3000, FM4000, FM6000 */ 
    FM_QOS_RX_RATE_LIM_DROP_THRESHOLD,

    /** Type fm_uint32: The number of scheduling groups on a port.
     *                                                                  \lb\lb
     *  On FM6000 devices, the maximum number of scheduling groups is 12. The 
     *  minimum number of scheduling groups is 1. The default is 12.
     *                                                                  \lb\lb
     *  On FM10000 devices, the maximum number of scheduling groups is 8. The 
     *  minimum number of scheduling groups is 1. The default is 8.
     *                                                                  \lb\lb
     *  This attribute does not take effect until the FM_QOS_APPLY_NEW_SCHED
     *  attribute is set. See ''FM_QOS_APPLY_NEW_SCHED'' for details.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_NUM_SCHED_GROUPS,

    /** Type fm_unit32: The priority set of a scheduling group.
     *  Each scheduling group belongs to a priority set. All scheduling groups
     *  in the same priority set are successive in their group numbers. For
     *  example, with 8 scheduling groups, groups 0 to 4 could be in priority set
     *  0, and groups 5 to 7 in priority set 1. The attribute index is
     *  the group number (0 to NUM_SCHED_GROUPS - 1). The default is for each
     *  scheduling group to belong to its own priority set so that the priority
     *  set number is equal to the scheduling group number. 
     *                                                                  \lb\lb
     *  This attribute does not take effect until the FM_QOS_APPLY_NEW_SCHED
     *  attribute is set. See ''FM_QOS_APPLY_NEW_SCHED'' for details.
     *
     *  \chips  FM6000 */
    FM_QOS_SCHED_GROUP_PRISET_NUM,

    /** Type fm_uint32: The strict property of a scheduling group.
     *  A scheduling group may be set with strict priority, or with non-strict
     *  priority for round-robin. The value must be 0 or 1 and the attribute 
     *  index is the group number (0 to NUM_SCHED_GROUPS - 1). The default 
     *  value is 1. 
     *                                                                  \lb\lb
     *  This attribute does not take effect until the FM_QOS_APPLY_NEW_SCHED
     *  attribute is set. See ''FM_QOS_APPLY_NEW_SCHED'' for details.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SCHED_GROUP_STRICT,

    /** Type fm_uint32: The DRR weight, a.k.a., DRR quantum. The
     *  register storing DRR weight is 24 bits wide. Thus, the range of DRR
     *  weight is 0 to 0xffffff. The attribute index is the group number
     *  (0 to NUM_SCHED_GROUPS - 1). The default is 0.
     *  
     *  On FM10000, setting this attribute to a value smaller than 2 times
     *  the MTU configured on the port will change the bandwidth allotted to
     *  each of the groups. The bandwidth will be dependent on the actual
     *  frames queued. For example, 2 groups, one with weight < 2*MTU and
     *  the other with MAX weight, will end up with an equivalent of 50%
     *  bandwidth each.
     *                                                                  \lb\lb
     *  This attribute does not take effect until the FM_QOS_APPLY_NEW_SCHED
     *  attribute is set. See ''FM_QOS_APPLY_NEW_SCHED'' for details.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SCHED_GROUP_WEIGHT,

    /** Type fm_uint32: a traffic class number, being one of the traffic class 
     *  boundaries of a scheduling group. A scheduling group includes one or 
     *  more consecutive traffic classes. Two boundaries in the set of traffic 
     *  classes are used to define the traffic classes in a scheduling group
     *  (this one and ''FM_QOS_SCHED_GROUP_TCBOUNDARY_B''). If the two boundaries 
     *  are the same, the scheduling group contains only that one traffic class. 
     *  The attribute index is the group number (0 to NUM_SCHED_GROUPS - 1).
     *  The default is one traffic class per scheduling group, so that this
     *  attribute's default value is equal to the scheduling group number. 
     *                                                                  \lb\lb
     *  This attribute does not take effect until the FM_QOS_APPLY_NEW_SCHED
     *  attribute is set. See ''FM_QOS_APPLY_NEW_SCHED'' for details.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SCHED_GROUP_TCBOUNDARY_A,

    /** Type fm_uint32: a traffic class number, being one of the traffic class 
     *  boundaries of a scheduling group. A scheduling group includes one or 
     *  more consecutive traffic classes. Two boundaries in the set of traffic 
     *  classes are used to define the traffic classes in a scheduling group
     *  (this one and ''FM_QOS_SCHED_GROUP_TCBOUNDARY_A''). If the two boundaries 
     *  are the same, the scheduling group contains only that one traffic class. 
     *  The attribute index is the group number (0 to NUM_SCHED_GROUPS - 1).
     *  The default is one traffic class per scheduling group, so that this
     *  attribute's default value is equal to the scheduling group number. 
     *                                                                  \lb\lb
     *  This attribute does not take effect until the FM_QOS_APPLY_NEW_SCHED
     *  attribute is set. See ''FM_QOS_APPLY_NEW_SCHED'' for details.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SCHED_GROUP_TCBOUNDARY_B,

    /** Type fm_uint32: Indicates whether a traffic class is
     *  enabled. The value of this attribute is bitmask with each bit
     *  corresponding to a traffic class. When the corresponding  bit of
     *  a traffic class is 1, that traffic class is enabled. Otherwise, 
     *  it is disabled.
     *
     *  For FM6000 devices, the value of this attribute is 12 bits, 
     *  bit 0 represents traffic class 0 and bit 11 represents traffic 
     *  class 11.  The default is 0xFFF (all traffic classes enabled). 
     *
     *  For FM10000 devices, the value of this attribute is 8 bits, 
     *  bit 0 represents traffic class 0 and bit 7 represents traffic 
     *  class 7.  The default is 0xFF (all traffic classes enabled). 
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_TC_ENABLE,

    /** Type fm_uint32: Indicates whether packet lengths in a traffic class 
     *  are counted as zero. The value of this attribute is bitmask with each 
     *  bit corresponding to a traffic class, such that bit 0 represents 
     *  traffic class 0, bit 1 represents traffic class 1, etc. When the 
     *  corresponding bit of a traffic class is 1, the length of packets from 
     *  that traffic class is counted as zero.
     *                                                                  \lb\lb 
     *  This attribute is read only.
     *                                                                  \lb\lb 
     *  \chips  FM6000, FM10000 */
    FM_QOS_TC_ZERO_LENGTH,

    /** Type fm_uint32: Apply a new egress scheduler configuration
     *  to the switch by calling ''fmSetPortQOS'' for this attribute
     *  (the specified value will be ignored). The scheduler
     *  configuration to be applied may be specified by these
     *  attributes:                                             \lb
     *                                                          \lb
     *  ''FM_QOS_NUM_SCHED_GROUPS''                             \lb
     *  ''FM_QOS_SCHED_GROUP_PRISET_NUM''                       \lb
     *  ''FM_QOS_SCHED_GROUP_STRICT''                           \lb
     *  ''FM_QOS_SCHED_GROUP_WEIGHT''                           \lb
     *  ''FM_QOS_SCHED_GROUP_TCBOUNDARY_A''                     \lb
     *  ''FM_QOS_SCHED_GROUP_TCBOUNDARY_B''                     \lb
     *                                                          \lb
     *  Since these attributes are interdependent, the
     *  FM_QOS_APPLY_NEW_SCHED attribute is needed to allow
     *  the scheduler configuration to first be completely 
     *  specified, then the entire configuration applied atomically.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_APPLY_NEW_SCHED,

    /** Type fm_uint32: Retrieve the active egress scheduler
     *  configuration from switch. This attribute is read-only.
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_RETRIEVE_ACTIVE_SCHED,
    
    /** UNPUBLISHED: For internal use only. */
    FM_QOS_PORT_ATTRIBUTE_MAX

};  /* end enum _fm_qosPortAttr */


/** A legacy synonym for ''FM_QOS_TC_SHAPING_GROUP_MAP''. 
 *  \ingroup macroSynonym */
#define FM_QOS_TC_GROUP_MAP FM_QOS_TC_SHAPING_GROUP_MAP

/** A legacy synonym for ''FM_QOS_SHAPING_GROUP_MAX_BURST''. 
 *  \ingroup macroSynonym */
#define FM_QOS_TC_GROUP_MAX_BURST   FM_QOS_SHAPING_GROUP_MAX_BURST

/** A legacy synonym for ''FM_QOS_SHAPING_GROUP_RATE''. 
 *  \ingroup macroSynonym */
#define FM_QOS_TC_GROUP_RATE    FM_QOS_SHAPING_GROUP_RATE


/**************************************************
 * For FM_QOS_QUEUE_SERVICE
 **************************************************/
#define FM_QOS_QUEUE_RRPRIORITY          0
#define FM_QOS_QUEUE_INTERLEAVE          1
#define FM_QOS_QUEUE_RRPURE              2


/**************************************************
 * For FM_QOS_QUEUE_MODE
 **************************************************/
#define FM_QOS_QUEUE_ALL_STRICT          0
#define FM_QOS_QUEUE_TWO_STRICT          1
#define FM_QOS_QUEUE_ONE_STRICT          2
#define FM_QOS_QUEUE_ALL_WRR             3


/***************************************************
 * For FM_QOS_TC_GROUP_MODE
 **************************************************/
#define FM_QOS_TC_GROUP_MODE_STRICT      0
#define FM_QOS_TC_GROUP_MODE_BALANCED    1


/**************************************************
 * For FM_QOS_CN_REACTION_MODE
 **************************************************/
#define FM_QOS_CN_REACTION_MODE_FORWARD  0
#define FM_QOS_CN_REACTION_MODE_DISCARD  1
#define FM_QOS_CN_REACTION_MODE_RATELIM  2


/**************************************************
 * For FM_QOS_TRAP_CLASS_SWPRI_MAP
 **************************************************/
#define FM_QOS_SWPRI_DEFAULT     (~0U)


/**************************************************
 * For FM_QOS_SHAPING_GROUP_RATE
 **************************************************/
#define FM_QOS_SHAPING_GROUP_RATE_DEFAULT FM_LITERAL_U64(0xFFFFFFFFFFFFFFFF)


/****************************************************************************/
/** \ingroup constQosSwAttr
 *
 * Switch QoS Attributes, used as an argument to ''fmSetSwitchQOS'' and
 * ''fmGetSwitchQOS''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 * Unless otherwise indicated, the attribute index argument to
 * ''fmSetSwitchQOS'' and ''fmGetSwitchQOS'' is ignored.
 ****************************************************************************/
enum _fm_qosSwAttr
{
    /** Type fm_int: Watermark selection per switch priority.
     *  The attribute index represents the switch priority (0 to
     *  ''FM_SWITCH_PRIORITY_MAX'' - 1) and the attribute value selects the
     *  watermark as one of the following:                                  
     *                                                                  \lb\lb
     *  FM_SWITCH_USE_LOW_WM to make all frames in this switch priority
     *  checked against the low global watermark for priority weighted discard
     *  (PWD),                                                              
     *                                                                  \lb\lb
     *  FM_SWITCH_USE_HLUX_WM to make all multicast and broadcast
     *  frames in this switch priority checked against the low global watermark
     *  for PWD, but all unicast frames in this switch priority are checked
     *  against the high global watermark,                                  
     *                                                                  \lb\lb
     *  FM_SWITCH_USE_HIGH_WM to make all frames in this switch priority
     *  checked against the high global watermark for PWD,                  
     *                                                                  \lb\lb
     *  FM_SWITCH_USE_PRIV_WM to make all frames in this switch priority
     *  checked against the privileged watermark only (no PWD).
     *                                                                  \lb\lb
     *  FM_SWITCH_USE_PRIV_WM is the default for switch priority 15 and
     *  FM_SWITCH_USE_HLUX_WM is the default for all other priorities. 
     *
     *  \chips  FM2000 */
    FM_QOS_SELECT_WM = 0,

    /** Type fm_uint32: The global queue size in bytes. Frames are dropped 100% 
     *  at this watermark irrespective of priority. This attribute needs to be 
     *  set sufficiently off from the top of device memory so that the chip 
     *  never tries to use more memory than it has.  Doing so can cause frame
     *  corruption, or even worse, chip deadlock.  Since the accounting
     *  of the memory use is done on the tail of each frame, you need to give
     *  a "skidpad" of one maximum frame size (rounded up to the nearest
     *  segment) for each port.  This will guarantee correctness, accounting 
     *  for the very improbable scenario that you get a max size frame on each 
     *  port all perfectly aligned.
     *                                                                  \lb\lb
     *  For FM2000 devices, the attribute value may range from 0 to 
     *  4,193,280 with a default value of 999,424. The API will round the 
     *  specified value up to the nearest 1024-byte boundary. Alternatively,
     *  specify the attribute value as -1 to have the API automatically
     *  calculate an optimal value based on the available memory size, the 
     *  total size of all private receive queues and the maximum frame size.
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 0 to 
     *  33,553,920 with a default value of 2,048,000. The API will round the
     *  specified value up to the nearest 512-byte boundary.
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 0 to 7,864,320 
     *  with a default value of 7,731,200. The API will round the specified 
     *  value up to the nearest 160-byte boundary.
     *                                                                  \lb\lb
     *  For FM10000 devices, the attribute value may range from 0 to 4,669,440
     *  with a default value of 3,932,160. The API will round the specified 
     *  value  up to the nearest 192-byte boundary.
     *                                                                  \lb\lb
     *  Because the specified number of bytes is rounded up by the API, the
     *  value returned by ''fmGetSwitchQOS'' may not exactly match the value
     *  set with ''fmSetSwitchQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_PRIV_WM,

    /** Type fm_int: Global high watermark in bytes. If the 
     *  frame matches a type specified by the ''FM_QOS_SELECT_WM'' attribute to 
     *  be checked against the high watermark, then the PWD line
     *  for that frame intersects this watermark at 100% drop probability.
     *                                                                  \lb\lb
     *  This attribute may range from 0 to 4,193,280 with a default value of 
     *  552,960. The API will round the specified value up to the nearest 
     *  1024-byte boundary, so the value returned by ''fmGetSwitchQOS'' may 
     *  not exactly match the value set with ''fmSetSwitchQOS''.
     *                                                                  \lb\lb
     *  Alternatively, specify the attribute value as -1 to have the API
     *  automatically calculate an optimal value based on the available 
     *  memory size, the total size of all private receive queues and the 
     *  maximum frame size.
     *                                                                  \lb\lb
     *  Note: when ''FM_AUTO_PAUSE_MODE'' is enabled, this attribute will be
     *  automatically calculated by the API and attempts to set it explicitly
     *  will result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000 */
    FM_QOS_HIGH_WM,

    /** Type fm_int: Global low watermark in bytes. If the frame 
     *  matches a type specified by the ''FM_QOS_SELECT_WM'' attribute to be 
     *  checked against the low watermark, then the PWD line for that frame 
     *  intersects this watermark at 100% drop probability. 
     *                                                              \lb\lb
     *  This attribute may range from 0 to 4,193,280 with a default value of 
     *  552,960. The API will round the specified value up to the nearest 
     *  1024-byte boundary, so the value returned by ''fmGetSwitchQOS'' 
     *  may not exactly match the value set with ''fmSetSwitchQOS''.
     *                                                                  \lb\lb
     *  Alternatively, specify the attribute value as -1 to have the API
     *  automatically calculate an optimal value based on the available 
     *  memory size, the total size of all private receive queues and the 
     *  maximum frame size.
     *                                                                  \lb\lb
     *  Note: when ''FM_AUTO_PAUSE_MODE'' is enabled, this attribute will be
     *  automatically calculated by the API and attempts to set it explicitly
     *  will result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM2000 */
    FM_QOS_LOW_WM,

    /** Type fm_uint32: The global per-priority shared watermark
     *  in bytes. Defines the watermark at which packets with the given 
     *  switch priority would be dropped if the switch priority usage exceeds 
     *  the per-memory partition shared pool. The attribute index (0 - 15) 
     *  indicates for which switch priority the watermark should be set.
     *                                                                  \lb\lb
     *  On FM4000 devices, the attribute value may range from 0 to 4,193,792 
     *  with a default value of 276,480. 
     *                                                                  \lb\lb
     *  On FM6000 devices, the attribute value may range from 0 to 10,485,600
     *  with a default value of 10,485,600. A setting higher than the amount
     *  of memory effectively disables this watermark.
     *                                                                  \lb\lb
     *  On FM10000 devices, the attribute value may range from 0 to 6,291,264 
     *  with a default value of 6,291,264. 
     *                                                                  \lb\lb
     *  The API will round the specified value up to the nearest memory
     *  segment boundary, so the value returned by ''fmGetSwitchQOS'' may 
     *  not exactly match the value set with ''fmSetSwitchQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SHARED_PRI_WM,

    /** Type fm_uint32: The global per-partition PAUSE on watermark
     *  in bytes. When the occupancy of global shared memory exceeds this 
     *  value, the port will transmit PAUSE frames for traffic classes 
     *  associated with this partition. In addition, the receive private
     *  watermark must be surpassed on any port before PAUSE frames are
     *  generated. 
     *                                                                  \lb\lb
     *  For FM4000 devices, the attribute value may range from 0 to 
     *  4,193,792 with a default value of 4,193,792. The attribute index 
     *  (0 - 1) indicates the memory partition.
     *                                                                  \lb\lb
     *  For FM6000 devices, the attribute value may range from 0 to 
     *  10,485,600 with a default value of 10,485,600. The attribute index
     *  (0 - 11) indicates the memory partition. Values larger than the 
     *  total amount of switch memory will effectively disable this 
     *  watermark. 
     *
     *  For FM10000 devices, the attribute value may range from 0 to 6,291,264
     *  with a default value of 6,291,264. The attribute index (0 - 1) indicates
     *  the memory partition.
     *                                                                  \lb\lb
     *  The API will round the specified value up to the nearest memory
     *  segment boundary, so the value returned by ''fmGetSwitchQOS'' may 
     *  not exactly match the value set with ''fmSetSwitchQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SHARED_PAUSE_ON_WM,

    /** Type fm_uint32: The global per-partition PAUSE off watermark
     *  in bytes. When the occupancy of global shared memory drops to this 
     *  value, the port will stop transmitting PAUSE frames for traffic 
     *  classes allocated with the partition. PAUSE will still be transmitted 
     *  if the per-port private PAUSE on watermark is exceeded or the receive 
     *  rate limiter is exceeded. 
     *                                                                  \lb\lb
     *  For FM3000 and FM4000 devices, the attribute value may range from 
     *  0 to 4,193,792 with a default value of 4,193,792. 
     *  The attribute index (0 - 1) indicates the memory partition.
     *      
     *  For FM6000 devices, the attribute value may range from 0 to 
     *  10,485,600 with a default value of 10,485,600. The attribute index 
     *  (0 - 11) indicates the memory partition.
     *                                                                  
     *  For FM10000 devices, the attribute value may range from 0 to 6,291,264
     *  with a default value of 6,291,264. The attribute index (0 - 1) indicates
     *  the memory partition.
     *                                                                  \lb\lb
     *  The API will round the specified value up to the nearest memory
     *  segment boundary, so the value returned by ''fmGetSwitchQOS'' 
     *  may not exactly match the value set with ''fmSetSwitchQOS''.
     *                                                                  \lb\lb
     *  Note: when the ''FM_AUTO_PAUSE_MODE'' switch QoS attribute is
     *  enabled, this attribute will be automatically calculated by the API
     *  and attempts to set it explicitly (on FM10000 devices only) will
     *  result in an ''FM_ERR_INVALID_QOS_MODE'' error being returned. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SHARED_PAUSE_OFF_WM,

    /** Type fm_uint32: The RXMP soft drop watermark in bytes per ISL priority.
     *  The attribute index (0 - 15) indicates the ISL priority. Frames are 
     *  eligible for probability based dropping when this watermark (plus 
     *  ''FM_QOS_SHARED_SOFT_DROP_WM_JITTER'') is exceeded.
     * 
     *  For FM6000 devices, the attribute value may range from 0 to 
     *  7,863,840 with a default value of 7,863,840. 
     *                                 
     *  For FM10000 devices, the attribute value may range from 0 to 
     *  6,291,264 with a default value of 6,291,264. 
     *
     *  The API will round up the specified attribute value to the nearest 
     *  memory segment boundary, so the value returned by ''fmGetSwitchQOS''
     *  may not exactly match the value set with ''fmSetSwitchQOS''.  
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SHARED_SOFT_DROP_WM,

    /** Type fm_uint32: The number of jitter bits to shift 
     *  ''FM_QOS_SHARED_SOFT_DROP_WM'' by when determining soft drop 
     *  probability. The value ranges from 0 to 7 and defaults to 0. The 
     *  index (0 - 15) indicates the ISL priority. 
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SHARED_SOFT_DROP_WM_JITTER,

    /** Type fm_uint32: The RXMP soft drop watermark hog limit in bytes per ISL
     *  priority. The attribute index (0 - 15) indicates the ISL priority. 
     *  Frames are TX dropped when this watermark is surpassed.
     *
     *  For FM6000 devices, the attribute value may range from 0 to 7,863,840
     *  with a default value of 7,863,840. 
     *
     *  For FM10000 devices, the attribute value may range from 0 to
     *  6,291,264 with a default value of 6,291,264.
     *                                                                  \lb\lb
     *  The API will round up the specified attribute value to the nearest 
     *  memory segment boundary, so the value returned by ''fmGetSwitchQOS'' 
     *  may not exactly match the value set with ''fmSetSwitchQOS''.  
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_SHARED_SOFT_DROP_WM_HOG,


    /** Type fm_int: Congestion management priority mapping,
     *  mapping switch priority to PWD priority. The attribute value is the
     *  PWD priority and may rangme from 0 to 15 with a default value of 13.
     *  The attribute index is the switch priority (0 - 15). 
     *
     *  \chips  FM2000 */
    FM_QOS_PWD,

    /** Type fm_int: Maps each traffic class to one of the
     *  four transmit hog watermarks. The attribute value may range from 0 to 3
     *  with a default value of 0. The attribute index indicates the traffic
     *  class. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_TX_HOG_WM_MAP,

    /** Type fm_uint32: Maps each internal VLAN priority code to 
     *  an internal switch priority. The internal VLAN priority code is mapped 
     *  from the ingress VLAN priority by the ''FM_QOS_RX_PRIORITY_MAP'' port 
     *  QoS attribute.
     *                                                                  \lb\lb
     *  The attribute value is the switch priority and may range from 0 to 15. 
     *  The attribute index is the internal VLAN priority code being mapped 
     *  (0 - 15). The default is to map each internal VLAN as a 7-bit value
     *  (by discarding the low-order CFI bit) to the corresponding switch
     *  priority:                                                           \lb
     *  4-bit VPRI 0 (3-bit 0) -> SWPRI 0                                   \lb
     *  4-bit VPRI 1 (3-bit 0) -> SWPRI 0                                   \lb
     *  4-bit VPRI 2 (3-bit 1) -> SWPRI 1                                   \lb
     *  4-bit VPRI 3 (3-bit 1) -> SWPRI 1                                   \lb
     *  4-bit VPRI 4 (3-bit 2) -> SWPRI 2                                   \lb
     *  4-bit VPRI 5 (3-bit 2) -> SWPRI 2                                   \lb
     *  .                                                                   \lb
     *  .                                                                   \lb
     *  .                                                                   \lb
     *  4-bit VPRI 14 (3-bit 7) -> SWPRI 7                                  \lb
     *  4-bit VPRI 15 (3-bit 7) -> SWPRI 7
     *                                                                  \lb\lb
     *  For FM6000 devices, use ''FM_QOS_L2_VPRI1_TO_ISLPRI_MAP''
     *  and ''FM_QOS_L2_VPRI2_TO_ISLPRI_MAP''.
     *                                                                  \lb\lb
     *  \chips  FM3000, FM4000, FM10000 */
    FM_QOS_VPRI_SWPRI_MAP,

    /** Type fm_uint32: Global DSCP priority to switch priority map. The
     *  attribute value is the switch priority and may range from 0  to 15
     *  with a default value of 0. The attribute index indicates the DSCP
     *  value (0 - 63).
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_DSCP_SWPRI_MAP,

    /** Type fm_uint32: Global switch priority to traffic class map. The
     *  attribute value is the traffic class and the attribute index is the
     *  switch priority (0 - 15)
     *                                                                  \lb\lb
     *  On the FM3000 and FM4000, the traffic class value may range from 0 to 7.
     *  The default maps switch priorities 0 through 7 to traffic classes 0 
     *  through 7, respectively and swtich priorities 8 through 15 to traffic 
     *  classes  0 through 7, respectively.     
     *                                                                  \lb\lb
     *  On the FM6000, the traffic class value may range from 0 to 11. The 
     *  default maps all switch priorities to traffic class as follows:     \lb
     *  0 -> 0                                                              \lb
     *  1 -> 1                                                              \lb
     *  2 -> 2                                                              \lb
     *  3 -> 3                                                              \lb
     *  4 -> 4                                                              \lb
     *  5 -> 5                                                              \lb
     *  6 -> 6                                                              \lb
     *  7 -> 7                                                              \lb
     *  8 -> 8                                                              \lb
     *  9 -> 9                                                              \lb
     *  10 -> 10                                                            \lb
     *  11 -> 11                                                            \lb
     *  12 -> 0                                                             \lb
     *  13 -> 1                                                             \lb
     *  14 -> 2                                                             \lb
     *  15 -> 3
     *                                                                  \lb\lb
     *  On the FM10000 devices, the traffic class value may range from 0 to 7.
     *  The default maps switch priorities 0 through 7 to traffic classes 0 
     *  through 7, respectively and swtich priorities 8 through 15 to traffic 
     *  classes  0 through 7, respectively.     
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SWPRI_TC_MAP,

    /** Type fm_uint32: Global switch priority to RXMP map. The attribute
     *  value is the RXMP and ranges from 0 to 11. The attribute index is the 
     *  switch priority (0 - 15). The default mapping is all switch priorities
     *  to RXMP zero.
     *
     *  \chips  FM6000 */ 
    FM_QOS_SWPRI_RXMP_MAP,

    /** Type fm_uint32: Global switch priority to TXMP mapping. The attribute
     *  value is the TXMP and may range from 0 to 11. The attribute index is 
     *  the switch priority (0 - 15). The default mapping maps all TXMPs to
     *  switch priority zero.
     *
     *  \chips  FM6000 */ 
    FM_QOS_SWPRI_TXMP_MAP,

    /** Type fm_uint32: Global traffic class to shared memory
     *  partition map. 
     *
     *  For FM3000 and FM4000 devices the attribute value is:
     *                                                                  \lb\lb
     *  FM_QOS_TC_SMP_NONE (default) for non-SMP controlled.
     *                                                                  \lb\lb
     *  FM_QOS_TC_SMP_0 for SMP 0.
     *                                                                  \lb\lb
     *  FM_QOS_TC_SMP_1 for SMP 1.
     *                                                                  \lb\lb
     *  The attribute index is the traffic class (0 - 7).
     *
     *  For FM6000 devices the attribute value could contain multiple 
     *  memory partitions per traffic class, depending on configuration.  
     *  Setting the attribute will set all occurrences of the specified traffic
     *  class to the provided RXMP value. Reading the attribute will return the 
     *  RXMP constant for the first occurrence of the specified traffic class.
     *                                                                  \lb\lb
     *  Use FM_QOS_TC_SMP_X for RXMP X, where X may range from 0 to 11.
     *                                                                  \lb\lb
     *  The attribute index is the traffic class (0 - 11).
     *
     *  For FM10000 devices the attribute value is:
     *                                                                  \lb\lb
     *  FM_QOS_TC_SMP_0 for SMP 0.
     *                                                                  \lb\lb
     *  FM_QOS_TC_SMP_1 for SMP 1.
     *                                                                  \lb\lb
     *  The attribute index is the traffic class (0 - 7).
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_TC_SMP_MAP,

    /** Type fm_uint32: The congestion notification operating
     *  mode. Value is:
     *                                                                  \lb\lb
     *  FM_QOS_CN_MODE_DISABLED (default) for no congestion notification
     *  frame generation.
     *                                                                  \lb\lb
     *  FM_QOS_CN_MODE_VCN to multicast VCN frames periodically to report
     *  memory partition utilization (congested or not).
     *                                                                  \lb\lb
     *  FM_QOS_CN_MODE_FCN to transmit congestion notification frames
     *  toward the source by randomly sampling frames when the equilibrum
     *  point as been exceeded. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_MODE,

    /** Type fm_uint32: The Ethertype value that will be used
     *  for congestion notification when the ''FM_QOS_CN_MODE'' attribute is set
     *  to something other than FM_QOS_CN_MODE_DISABLED. This Ethertype is
     *  used both for generated congestion notification frames and for parsing
     *  received frames. Default value is 0x1001. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_ETYPE,

    /** Type fm_uint32: The VLAN priority used in congestion
     *  notification frames when the ''FM_QOS_CN_MODE'' attribute is set
     *  to something other than FM_QOS_CN_MODE_DISABLED. The attribute
     *  value may range from 0 to 15 (it includes the CFI/DEI bit) with a
     *  default value of 0. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_VPRI,

    /** Type fm_uint32: The VLAN ID used in congestion
     *  notification frames when the ''FM_QOS_CN_MODE'' attribute is set
     *  to something other than FM_QOS_CN_MODE_DISABLED. The attribute
     *  value may range from 0 to 4095 with a default value of 0. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_VLAN,

    /** Type fm_int: The source port used in congestion
     *  notification frames when the ''FM_QOS_CN_MODE'' attribute is set
     *  to something other than FM_QOS_CN_MODE_DISABLED. The attribute
     *  value may be any valid port number for the system with a default
     *  value of 0.
     *                                                                  \lb\lb
     *  This value is intended to identify the physical switch that
     *  generated the congestion notification frame. Accordingly, it is not
     *  relevant to switch aggregates and cannot be set on a SWAG. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_SRC_PORT,

    /** Type fm_macaddr: The source MAC address used in congestion
     *  notification frames when the ''FM_QOS_CN_MODE'' attribute is set
     *  to something other than FM_QOS_CN_MODE_DISABLED. For a switch aggregate
     *  (SWAG), the API will replace the low-order 8 bits with the physical 
     *  switch number within the SWAG. Default value is 0x000000000000. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_SMAC,

    /** Type fm_macaddr: The destination MAC address used in
     *  congestion notification frames when the ''FM_QOS_CN_MODE'' attribute 
     *  is set to something other than FM_QOS_CN_MODE_DISABLED. Only the 
     *  high-order 32 bits are used. Default value is 0x00effeff0000. 
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_QOS_CN_FRAME_DMAC,

    /** Type fm_bool: Deprecated - Access control lists: FM_ENABLED
     *  or FM_DISABLED (default). 
     *
     *  \chips  FM2000 */
    FM_QOS_ACL_ENABLE,

    /** Deprecated. Use ''FM_QOS_PWD''. 
     *
     *  \chips  FM2000 */
    FM_QOS_WRED,

    /** Type fm_int: Internal switch priority to scheduling
     *  priority map. The attribute value is the scheduling priority
     *  and may range from 0 to 3. The attribute index is the internal switch
     *  priority and may range from 0 to 15. The default scheduling priorities
     *  for the 16 internal priorities, starting from internal priority 0 and
     *  going up to 15 are:
     *                                                                  \lb\lb
     *  1, 0, 0, 1, 2, 2, 3, 3, 1, 0, 0, 1, 2, 2, 3, 3. 
     *
     *  \chips  FM2000 */
    FM_QOS_SCHEDULER_PRIORITY,

    /** Type fm_uint32: The global memory usage in bytes. This attribute is 
     *  read-only.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_GLOBAL_USAGE,

    /** Type fm_uint32: The number of frames in global memory. This attribute 
     *  is read-only.
     *
     *  \chips  FM6000 */
    FM_QOS_GLOBAL_USAGE_FRAMES,


    /** Type fm_uint32: The RXMP memory usage in bytes. The attribute index
     *  (0 - 11) is the RX memory partition. This attribute is read-only.
     *
     *  \chips  FM6000 */
    FM_QOS_RXMP_USAGE,

    /** Type fm_uint32: The global shared memory usage in bytes, not including
     *  the amount used by the private per-port receive queues. This attribute 
     *  is read-only.
     *                                                                  \lb\lb
     *  On FM3000 and FM4000 devices, the attribute index indicates the 
     *  memory partition and may range from 0 to 1.
     *                                                                  \lb\lb
     *  On FM6000 devices, the attribute index indicates the RX memory 
     *  partition and may range from 0 to 11.
     *                                                                  \lb\lb
     *  On FM10000 devices, the attribute index indicates the memory 
     *  partition and may range from 0 to 1.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_QOS_SHARED_USAGE,

    /** Type fm_uint32: The number of frames in an RX memory partition, but 
     *  not including the amount used by the private per-port receive queues.
     *  The attribute index (0 - 11) indicates the memory partition. This 
     *  attribute is read-only. 
     *
     *  \chips  FM6000 */
    FM_QOS_SHARED_USAGE_FRAMES,

    /** Type fm_uint32: The global TXMP memory usage in bytes. This attribute
     *  is read-only.
     *
     *  \chips  FM6000 */
    FM_QOS_GLOBAL_TXMP_USAGE,

    /** Type fm_bool: Indicates whether the API should automatically calculate
     *  watermarks for lossless PAUSE operation: FM_ENABLED (default) or
     *  FM_DISABLED.
     *                                                                  \lb\lb
     *  When this attribute is enabled, the ''FM_PORT_LOSSLESS_PAUSE'' port
     *  attribute can be used to enable or disable lossless PAUSE individually 
     *  on each port.
     *                                                                  \lb\lb
     *  When this attribute is enabled, the following attributes will be
     *  manipulated automatically by the API.
     *                                                                  \lb\lb
     *  Port QoS Attributes:                                                \lb
     *  ''FM_QOS_TX_SHARED_WM''                                             \lb
     *  ''FM_QOS_TX_HOG_WM''                                                \lb
     *  ''FM_QOS_TX_PRIVATE_WM''                                            \lb
     *  ''FM_QOS_RX_SHARED_WM''                                             \lb
     *  ''FM_QOS_RX_HOG_WM''*                                               \lb
     *  ''FM_QOS_RX_PRIVATE_WM''*                                           \lb
     *  ''FM_QOS_GLOBAL_PAUSE_ON_WM''                                       \lb
     *  ''FM_QOS_GLOBAL_PAUSE_OFF_WM''                                      \lb
     *  ''FM_QOS_PRIVATE_PAUSE_ON_WM''*                                     \lb
     *  ''FM_QOS_PRIVATE_PAUSE_OFF_WM''*                                    \lb
     *                                                                      \lb
     *  Switch QoS Attributes:                                              \lb
     *  ''FM_QOS_PRIV_WM''*                                                 \lb
     *  ''FM_QOS_HIGH_WM''                                                  \lb
     *  ''FM_QOS_LOW_WM''                                                   \lb
     *  ''FM_QOS_SHARED_PAUSE_ON_WM''*                                      \lb
     *  ''FM_QOS_SHARED_PAUSE_OFF_WM''*                                     \lb
     *  ''FM_QOS_SHARED_PRI_WM''*                                           \lb
     *  ''FM_QOS_TC_SMP_MAP''                                               \lb 
     *                                                                  \lb\lb
     *  *On FM10000 devices, these attributes cannot be set individually 
     *  when FM_AUTO_PAUSE_MODE is enabled. Attempts to do so will return
     *  ''FM_ERR_INVALID_QOS_MODE''.
     *                                                                  \lb\lb
     *  When this attribute is disabled, the above attributes will revert 
     *  to their default values. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_AUTO_PAUSE_MODE,

    /** Type fm_uint32: Maps a class of trapped frames to a switch priority.
     *  The purpose is to allow prioritization of various slow protocol
     *  traffic. The attribute index represents the frame class, and the
     *  attribute value is the switch priority, in the range 0 to 
     *  (''FM_SWITCH_PRIORITY_MAX'' - 1). May also be set to
     *  FM_QOS_SWPRI_DEFAULT to clear previously configured mapping for the
     *  specified frame class. See ''fm_qosTrapClass'' for possible frame
     *  class values.
     *                                                                  \lb\lb
     *  By default, trapped frames will keep whatever switch priority
     *  they are assigned on ingress, until overridden with this attribute. 
     *
     *  \chips  FM3000, FM4000, FM6000,  FM10000 */
    FM_QOS_TRAP_CLASS_SWPRI_MAP,

    /** Type fm_uint32: Map VLAN1 priority (VPRI1) to ISL priority.
     *  The index is VPRI1 and the value is the ISL priority.
     *                                                                  \lb\lb
     *  VPRI1 is the internal 4-bit priority that results after being
     *  mapped by ''FM_QOS_L2_VPRI1_MAP'' on a per-port basis. 
     *
     *  \chips  FM6000 */
    FM_QOS_L2_VPRI1_TO_ISLPRI_MAP,

    /** Type fm_uint32: Map VLAN2 priority (VPRI2) to ISL priority.
     *  The index is VPRI2 and the value is the ISL priority.
     *                                                                  \lb\lb
     *  VPRI2 is the internal 4-bit priority that results after being
     *  mapped by ''FM_QOS_L2_VPRI2_MAP'' on a per-port basis. 
     *
     *  \chips  FM6000 */
    FM_QOS_L2_VPRI2_TO_ISLPRI_MAP,

    /** Type fm_uint32: Mapping QOS.W4 to ISL priority.
     *  The QOS.W4 is in internal priority form, which is obtained after
     *  QOS_PER_PORT_W4 mapping. The index is QOS.W4. The value is the ISL
     *  priority. 
     *
     *  \chips  FM6000 */
    FM_QOS_W4_TO_ISLPRI_MAP,

    /** Type fm_uint32: Defines the number of bytes that will be added to each 
     *  frame for the purposes of egress rate limiting. The effective length of
     *  a packet is corrected using the value of this attribute. 
     *  The default value is 0.
     *
     *  \chips  FM10000 */
    FM_QOS_SWITCH_IFG_PENALTY,

    /** UNPUBLISHED: For internal use only. */
    FM_QOS_SWITCH_ATTRIBUTE_MAX

};  /* end enum _fm_qosSwAttr */


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_SPANNING_TREE_MODE'' switch attribute.
 **************************************************/
typedef enum
{
    /** All VLANs share a single spanning tree instance. Port spanning tree
     *  states are set using ''fmSetVlanPortState''. 
     *                                                                  \lb\lb
     *  For FM2000 and FM4000 devices, source MAC addresses are learned
     *  globally (not per-VLAN). For the FM6000 and FM10000, MAC address
     *  learning is configured independently, using FM_VLAN_LEARNING_MODE.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_SPANNING_TREE_SHARED = 0,

    /** Each VLAN has its own spanning tree instance. Port spanning tree
     *  states are set using ''fmSetVlanPortState''.
     *                                                                  \lb\lb
     *  For FM2000 and FM4000 devices, source MAC addresses are learned
     *  per-VLAN. For the FM6000 and FM10000, MAC address learning is
     *  configured independently, using FM_VLAN_LEARNING_MODE.
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_SPANNING_TREE_PER_VLAN,

    /** Multiple spanning tree instances may be created using the
     *  ''Spanning Tree Management'' API services. VLANs are assigned to
     *  spanning tree instances in any combination (all to one, each to its
     *  own or some VLANs sharing one instance while other VLANs share another
     *  instance).
     *                                                                  \lb\lb
     *  For FM2000 and FM4000 devices, source MAC addresses are learned
     *  per-VLAN. For the FM6000 and FM10000, MAC address learning is
     *  configured independently, using FM_VLAN_LEARNING_MODE.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_SPANNING_TREE_MULTIPLE

} fm_stpMode;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_VLAN_LEARNING_MODE'' switch attribute.
 **************************************************/
typedef enum
{
    /** Source MAC addresses are learned using the VLAN on which they are
     *  received. The spanning tree state is independently derived based on
     *  the setting of FM_SPANNING_TREE_MODE. */
    FM_VLAN_LEARNING_MODE_INDEPENDENT,

    /** Source MAC addresses are learned on a single shared VLAN, which is
     *  configured by setting the FM_VLAN_LEARNING_SHARED_VLAN switch
     *  attribute. The spanning tree state is independently derived based on
     *  the setting of FM_SPANNING_TREE_MODE. */
    FM_VLAN_LEARNING_MODE_SHARED,

} fm_vlanLearningMode;


/**************************************************
 * For FM_QOS_SELECT_WM
 **************************************************/
#define FM_SWITCH_USE_LOW_WM     0
#define FM_SWITCH_USE_HLUX_WM    1
#define FM_SWITCH_USE_HIGH_WM    2
#define FM_SWITCH_USE_PRIV_WM    3


/**************************************************
 * For FM_LCI_ENDIANNESS
 **************************************************/
#define FM_LCI_LITTLE_ENDIAN     0
#define FM_LCI_BIG_ENDIAN        1


/**************************************************
 * For FM_QOS_CN_MODE
 **************************************************/
#define FM_QOS_CN_MODE_DISABLED  0
#define FM_QOS_CN_MODE_VCN       1
#define FM_QOS_CN_MODE_FCN       2


/**************************************************
 * For FM_QOS_TC_SMP_MAP
 **************************************************/
#define FM_QOS_TC_SMP_NONE       0
#define FM_QOS_TC_SMP_0          1
#define FM_QOS_TC_SMP_1          2
#define FM_QOS_TC_SMP_2          3
#define FM_QOS_TC_SMP_3          4
#define FM_QOS_TC_SMP_4          5
#define FM_QOS_TC_SMP_5          6
#define FM_QOS_TC_SMP_6          7
#define FM_QOS_TC_SMP_7          8
#define FM_QOS_TC_SMP_8          9
#define FM_QOS_TC_SMP_9          10
#define FM_QOS_TC_SMP_10         11
#define FM_QOS_TC_SMP_11         12
#define FM_QOS_TC_SMP_INVALID    16


/**************************************************/
/** \ingroup constSystem 
 *  The default value for ''FM_PORT_DFE_PARAMETERS''
 **************************************************/
#define FM_PORT_DFE_DEFAULTS 0x2070000F


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  ''FM_PORT_DFE_MODE'' port attribute. 
 **************************************************/
typedef enum
{
    /** Fixed, static assignment of the Receiver DFE tuning
     *  parameters. When this mode is selected the attribute
     *  ''FM_PORT_DFE_PARAMETERS'' is read/write and contains the
     *  fixed value of the DFE parameters. This is the only possible
     *  mode for non-KR configurations with bit rates less than or
     *  equal to 2.5 Gbps. */
    FM_DFE_MODE_STATIC = 0,

    /** Single-loop Dynamic DFE Tuning. This is the default value for
     *  ''FM_PORT_DFE_MODE'' for non-KR configurations on FM10000
     *  devices. */
    FM_DFE_MODE_ONE_SHOT,

    /** Dynamic DFE tuning, featuring continuous fine tuning adaptations.
     *  This is the default value for ''FM_PORT_DFE_MODE'' for non-KR
     *  configurations on FM6000 devices. */
    FM_DFE_MODE_CONTINUOUS,

    /** Adaptive hardware-assisted DFE tuning. Supported only by KR-type
     *  interfaces. This is the default value for ''FM_PORT_DFE_MODE'' for
     *  KR configurations. It is chosen automatically for KR-type
     *  interfaces, and should not be set manually. */
    FM_DFE_MODE_KR,

    /** Dynamic DFE Tuning, perform only the iCal (initial calibration)
     *  tuning stage. This is only for debugging purposes on FM10000
     *  devices. */
    FM_DFE_MODE_ICAL_ONLY


} fm_dfeMode;


/*************************************************************/
/** \ingroup constSystem 
 *  The default value for ''FM_PORT_LOW_EYE_SCORE_THRESHOLD''
 *************************************************************/
#define FM_PORT_LOW_EYE_SCORE_DEFAULT_THRESHOLD     10


/*************************************************************/
/** \ingroup constSystem 
 *  The default value for ''FM_PORT_LOW_EYE_SCORE_TIMEOUT''
 *************************************************************/
#define FM_PORT_LOW_EYE_SCORE_DEFAULT_TIMEOUT     5


/****************************************************************/
/** \ingroup constSystem 
 *  The default value for ''FM_PORT_SIGNAL_DETECT_DEBOUNCE_TIME''
 ****************************************************************/
#define FM_PORT_SIGNAL_DETECT_DEFAULT_DEBOUNCE_TIME     3


/*****************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  ''FM_PORT_LOW_EYE_SCORE_RECOVERY'' port attribute. 
 *****************************************************/
typedef enum
{
    /** No recovery is attempted.  */
    FM_PORT_LOW_EYE_SCORE_RECOVERY_OFF = 0,

    /** Recover by restarting the DFE process from coarse tuning
     *  (default). */
    FM_PORT_LOW_EYE_SCORE_SOFT_RECOVERY,

    /** Recover by power-cycling the port associated with this lane. */
    FM_PORT_LOW_EYE_SCORE_HARD_RECOVERY

} fm_eyeRecoveryMode;


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for 
 *  the ''FM_PORT_COARSE_DFE_STATE'' and 
 *  ''FM_PORT_FINE_DFE_STATE'' port attributes. 
 **************************************************/
typedef enum
{
    /** DFE Tuning not started. */
    FM_PORT_DFE_STATE_NOT_STARTED = 0,

    /** DFE Tuning still in progress. */
    FM_PORT_DFE_STATE_IN_PROGRESS,

    /** DFE Tuning complete successfully. */
    FM_PORT_DFE_STATE_COMPLETE,   

    /** DFE Tuning aborted due to an unrecoverable error. */
    FM_PORT_DFE_STATE_ERROR      

} fm_dfeState;


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for 
 *  the ''FM_PORT_RX_TERMINATION'' port attribute. 
 **************************************************/
typedef enum
{
    /** AVDD termination. */
    FM_PORT_TERMINATION_HIGH = 0,

    /** AGND termination. */
    FM_PORT_TERMINATION_LOW,

    /** FLOAT termination. */
    FM_PORT_TERMINATION_FLOAT,

} fm_rxTermination;


/*****************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  ''FM_PORT_TX_FCS_MODE'' port attribute.
 *  In the following descriptions, a frame is invalid
 *  (marked as having an error) if it is received with
 *  an FCS error, or if a parity error is detected
 *  during processing.
 *****************************************************/
typedef enum
{
    /** Transmit the frame with the existing FCS. Used for debugging.
     *  
     *  \chips FM6000, FM10000 */
    FM_FCS_PASSTHRU = 0,

#if 0
    /** If the frame is valid, transmit it with the existing FCS. If the
     *  frame is marked as having an error, replace the existing FCS with
     *  an invalid FCS. Used for debugging. */
    FM_FCS_PASSTHRU_CHECK = 1,

    /** Regardless of the validity of the frame, compute the correct FCS 
     *  and append it to the transmitted frame. Used for debugging. */
    FM_FCS_INSERT_GOOD = 2,

    /** Regardless of the validity of the frame, append an invalid FCS to
     *  the transmitted frame. Used for debugging. */
    FM_FCS_INSERT_BAD = 3,
#endif

    /** If the frame is valid, compute the correct FCS and append it to the
     *  transmitted frame. If the frame is marked as having an error, append
     *  an invalid FCS. Used for normal operation.
     *  
     *  \chips FM6000 */
    FM_FCS_INSERT_NORMAL = 4,

#if 0
    /** Regardless of the validity of the frame, replace the last four
     *  octets with the correct FCS when transmitting it. Used for
     *  debugging. */
    FM_FCS_REPLACE_GOOD = 5,

    /** Regardless of the validity of the frame, replace the last four
     *  octets with an incorrect FCS when transmitting it. Used for
     *  debugging. */
    FM_FCS_REPLACE_BAD = 6,
#endif

    /** If the frame is valid, replace the last four octets in the
     *  transmitted frame with the correct FCS. If the frame is marked as
     *  having an error, replace the last four octets with an invalid
     *  FCS. Used for normal operation.
     *  
     *  \chips FM6000, FM10000 */
    FM_FCS_REPLACE_NORMAL = 7

} fm_txFcsMode;

#define FM_MIN_TX_FCS_MODE  FM_FCS_PASSTHRU
#define FM_MAX_TX_FCS_MODE  FM_FCS_REPLACE_NORMAL


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_PORT_UCAST_FLOODING'' port attribute. 
 *  
 * For flooding/trapping/logging/dropping of  
 * unicast frames with unknown MAC address    
 **************************************************/
typedef enum
{
    /** Flood unicast frames to all ports (in the associated VLAN) except
     *  the CPU port. (default) */
    FM_PORT_UCAST_FWD_EXCPU = 0,

    /** Flood unicast frames to all ports (in the associated VLAN) including
     *  the CPU port. */
    FM_PORT_UCAST_FWD,

    /** Drop unicast frames. */
    FM_PORT_UCAST_DISCARD,

    /** Trap unicast frames to the CPU. Frames are not flooded to any
     *  other ports. */
    FM_PORT_UCAST_TRAP,

} fm_portUcastFlooding;

/** A legacy synonym for ''FM_PORT_UCAST_FWD''.
 *  \ingroup macroSynonym */
#define FM_PORT_UCAST_LOG   FM_PORT_UCAST_FWD


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_PORT_MCAST_FLOODING'' port attribute. 
 *  
 * For flooding/trapping/logging/dropping of  
 * multicast frames with unknown MAC address    
 **************************************************/
typedef enum
{
    /** Flood unknown multicast frames to all ports except CPU. (default) */
    FM_PORT_MCAST_FWD_EXCPU = 0,

    /** Flood unknown multicast frames (including CPU port).*/
    FM_PORT_MCAST_FWD,

    /** Drop unknown multicast frames. */
    FM_PORT_MCAST_DISCARD,

    /** Intercept unknown multicast frames and send them to the CPU
     *  instead of flooding them to other ports. */
    FM_PORT_MCAST_TRAP,

} fm_portMcastFlooding;

/** A legacy synonym for ''FM_PORT_MCAST_FWD''.
 *  \ingroup macroSynonym */
#define FM_PORT_MCAST_LOG   FM_PORT_MCAST_FWD


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * ''FM_PORT_BCAST_FLOODING'' port attribute. 
 *  
 * For flooding/logging/dropping of  
 * broadcast frames   
 **************************************************/
typedef enum
{
    /** Flood broadcast frames to all ports except CPU. (default) */
    FM_PORT_BCAST_FWD_EXCPU = 0,

    /** Flood the broadcast frame (including CPU port). */
    FM_PORT_BCAST_FWD,

    /** Drop broadcast frames. */
    FM_PORT_BCAST_DISCARD,

    /** Intercept broadcast frames and send them to the CPU
     *  instead of flooding them to other ports. */
    FM_PORT_BCAST_TRAP,

} fm_portBcastFlooding;


/**************************************************/
/** \ingroup typeEnum
 * Frame class types for use with the
 * ''FM_QOS_TRAP_CLASS_SWPRI_MAP'' QoS attribute.
 **************************************************/
typedef enum
{
    /** For STP protocol frames. Trapping of these frames is enabled with the
     *  ''FM_TRAP_IEEE_BPDU'' switch attribute.
     *  
     *  \chips  FM6000 */
    FM_QOS_TRAP_CLASS_BPDU = 0,

    /** For frames that are received with the configured CPU MAC address as
     *  the destination MAC address. The CPU MAC address is configured with
     *  the ''FM_CPU_MAC'' switch attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_CPU_MAC,

    /** For GARP protocol frames. Trapping of these frames is enabled
     *  with the ''FM_TRAP_IEEE_GARP'' switch attribute.
     *  
     *  \chips  FM6000 */
    FM_QOS_TRAP_CLASS_GARP,

    /** For IEEE frames not otherwise enumerated. Trapping of these frames
     *  is enabled with the ''FM_TRAP_IEEE_OTHER'' switch attribute on
     *  FM6000 devices. On FM10000 devices this class refers to all IEEE frames
     *  for which trapping is enabled by attributes: ''FM_TRAP_IEEE_8021X'',
     *  ''FM_TRAP_IEEE_BPDU'' , ''FM_TRAP_IEEE_LACP'', ''FM_TRAP_IEEE_GARP''
     *  or by attribute ''FM_SWITCH_RESERVED_MAC_CFG''.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_IEEE,

    /** For ICMP protocol frames.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_ICMP,

    /** For IGMP protocol frames. Trapping of these frames is enabled with
     *  the ''FM_VLAN_IGMP_TRAPPING'' VLAN attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_IGMP,

    /** For trapped frames with IP options. Trapping of these frames is
     *  enabled with the ''FM_IP_OPTIONS_DISPOSITION'' switch attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_IP_OPTION,

    /** For LACP protocol frames. Trapping of these frames is enabled with the
     *  ''FM_TRAP_IEEE_LACP'' switch attribute.
     *  
     *  \chips  FM6000 */
    FM_QOS_TRAP_CLASS_LACP,

    /** For 802.1x protocol frames. Trapping of these frames is enabled with
     *  the ''FM_TRAP_IEEE_8021X'' switch attribute.
     *  
     *  \chips  FM6000 */
    FM_QOS_TRAP_CLASS_802_1X,

    /** For frames trapped due to MTU violations. Trapping of these frames is enabled with the
     *  ''FM_TRAP_MTU_VIOLATIONS'' switch attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_MTU_VIOLATION,

    /** For broadcast frames received. Trapping of these frames is
     *  enabled with the ''FM_PORT_BCAST_FLOODING'' port attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_BCAST_FLOODING,

    /** For multicast frames received with an unknown address. Trapping of
     *  these frames is enabled with the ''FM_PORT_MCAST_FLOODING'' port
     *  attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_MCAST_FLOODING,

    /** For unicast frames received with an unknown destination address.
     *  Trapping of these frames is enabled with the
     *  ''FM_PORT_UCAST_FLOODING'' port attribute. 
     *
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_UCAST_FLOODING,

    /** For routed frames received with the TTL field set to 1 or 0.
     *  Trapping of these frames is enabled with the
     *  ''FM_ROUTER_TRAP_TTL1'' router attribute.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_TTL1,

    /** For frames trapped because of a miss in the next-hop (ARP) table.
     *  
     *  \chips  FM6000, FM10000 */
    FM_QOS_TRAP_CLASS_NEXTHOP_MISS,

    /** For ARP frames logged to the CPU. Logging of these frames is enabled
     *  with the FM_VLAN_ARP_LOGGING VLAN attribute and ''FM_PORT_LOG_ARP''
     *  port attribute.
     *  
     *  \chips  FM6000 */
    FM_QOS_TRAP_CLASS_ARP,

    /** UNPUBLISHED: For internal use only. */
    FM_QOS_TRAP_CLASS_MAX

} fm_qosTrapClass;


#if 0
/**************************************************/
/** \ingroup intTypeEnum
 * Set of possible enumerated values for the
 * ''FM_QOS_WATERMARK_SCHEME'' qos attribute.
 *  
 * For selection of the default watermark scheme
 **************************************************/
typedef enum
{
    /* \chips FM6000 */
    FM_QOS_WM_DISABLED = 0,

    /* \chips FM6000 */
    FM_QOS_WM_LOSSY,

    /* \chips FM6000 */
    FM_QOS_WM_LOSSY_LOSSLESS,

    /* \chips FM6000 */
    FM_QOS_WM_LOSSLESS,

} fm_qosWatermarkScheme;
#endif


/**************************************************/
/** \ingroup typeEnum
 * List of trap types that may appear in 
 * ''fm_trapCodeMapping''.
 **************************************************/
typedef enum
{
    /** Trapped BPDU frame.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_TRAPCODE_BPDU = 0,

    /** Trapped 802.1X frame.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_TRAPCODE_802_1X,

    /** Trapped GARP frame.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_TRAPCODE_GARP,

    /** Trapped LACP frame.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_TRAPCODE_LACP,

    /** Trapped other IEEE reserved address frame.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_IEEE,

    /** Trapped CPU MAC address frame.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_CPU,

    /** Trapped CPU MAC address frame that was remapped to priority
     *  15.
     *  
     *  \chips  FM3000, FM4000 */
    FM_TRAPCODE_REMAP_CPU,

    /** Trapped frame due to FFU action.
     * 
     *  For FM6000 devices, this also includes frames logged due to
     *  FFU action.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_FFU,

    /** Trapped frame due to TTL <= 1 for ICMP frame.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    FM_TRAPCODE_ICMP_TTL,

    /** Trapped IP frame with options.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_IP_OPTION,

    /** Trapped frame due to MTU violation.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_MTU,

    /** Trapped IGMP frame.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_IGMP,

    /** On FM3000, FM4000 and FM10000 devices, trapped frame due to TTL <= 1
     *  for non-ICMP frames.
     *                                                                  \lb\lb
     *  For FM6000 devices, trapped frame due to TTL <= 1 for both ICMP and
     *  non-ICMP frames.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_TRAPCODE_TTL,

    /** Logged frame due to ingress FFU action.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    FM_TRAPCODE_LOG_INGRESS_FFU,

    /** Logged frame due to egress FFU action.
     *  
     *  \chips  FM3000, FM4000 */
    FM_TRAPCODE_LOG_EGRESS_FFU,

    /** Trapped PAUSE frame.
     *  
     *  \chips  FM6000 */
    FM_TRAPCODE_PAUSE,

    /** Trapped unauthorized MAC frame.
     *  
     *  \chips  FM6000 */
    FM_TRAPCODE_UNAUTHORIZED_MAC,

    /** Trapped ARP frame.
     *  
     *  \chips  FM6000 */
    FM_TRAPCODE_ARP,
    
    /** Trapped frame that hit unresolved nexthop entry (code 0).
     *  
     *  \chips  FM6000, FM10000 */
    FM_TRAPCODE_L3_ROUTED_NO_ARP_0,

    /** Trapped frame that hit unresolved nexthop entry (code 1).
     *  
     *  \chips  FM6000, FM10000 */
    FM_TRAPCODE_L3_ROUTED_NO_ARP_1,

    /** Trapped or logged frame due to flood control.
     *  
     *  \chips  FM6000 */
    FM_TRAPCODE_FLOOD_CONTROL,

    /** Trapped VxLAN frames.
     *
     *  \chips  FM6000 */
    FM_TRAPCODE_VXLAN,

    /** Trapped NAT frames.
     *
     *  \chips  FM6000 */
    FM_TRAPCODE_NAT,

    /** Logged ARP redirect (unicast frame routed back to ingress VLAN).
     *  
     *  \chips  FM10000 */
    FM_TRAPCODE_LOG_ARP_REDIRECT,

    /** Trapped frame from Tunnel Engine 0. This trap code refers to the
     *  base used by the engine to specify up to 4 consecutive values
     *  (FM_TRAPCODE_TE_0_BASE .. FM_TRAPCODE_TE_0_BASE + 3).
     *  
     *  \chips  FM10000 */
    FM_TRAPCODE_TE_0_BASE,

    /** Trapped frame from Tunnel Engine 1. This trap code refers to the
     *  base used by the engine to specify up to 4 consecutive values
     *  (FM_TRAPCODE_TE_1_BASE .. FM_TRAPCODE_TE_1_BASE + 3) .
     *  
     *  \chips  FM10000 */
    FM_TRAPCODE_TE_1_BASE,

    /** Mirrored frame to the CPU. This trap code refers to the
     *  base used by the mirror api to specify up to 16 consecutive values
     *  (FM_TRAPCODE_MIRROR_BASE .. FM_TRAPCODE_MIRROR_BASE + 15) . These
     *  offsets refer to the mirror attribute ''FM_MIRROR_TRAPCODE_ID''.
     *  
     *  \chips  FM10000 */
    FM_TRAPCODE_MIRROR_BASE,

    /** UNPUBLISHED: For internal use only. */
    FM_TRAPCODE_MAX,
   
} fm_trapType;


/****************************************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmSetSwitchAttribute'' for the 
 *  ''FM_FFU_SLICE_ALLOCATIONS'' switch attribute.
 *                                                                      \lb\lb
 *  The members of this structure specify how the FFU
 *  is partitioned between ACLs and each of the following route types:  \lb
 *                                                                      \lb
 *  IPv4 unicast                                                        \lb
 *  IPv6 unicast                                                        \lb
 *  IPv4 multicast                                                      \lb
 *  IPv6 multicast                                                      \lb
 *                                                                      \lb
 *  By default, the first two route types will share slices and the 
 *  second two will share slices, though sharing can be specified for
 *  any two route types or none at all. ACLs must not share slices with 
 *  route types. Sharing slices between route types can make more
 *  efficient use of FFU slices.
 *                                                                  \lb\lb
 *  Typically, routes are placed at the beginning of the FFU (starting at
 *  slice 0) and ACLs are placed at the end of the FFU (ending at slice
 *  31, since this is the only slice that supports egress ACLs). The
 *  boundary between routes and ACLs is arbitrary. As routes are added,
 *  they are placed at lower numbered slices and the table grows towards
 *  higher numbered slices. As ACLs are added, they are placed at higher
 *  numbered slices and the table grows towards lower numbered slices.
 *  This behavior keeps the region of slices between routes and ACLs empty
 *  for as long as possible, facilitating moving the boundary if needed.
 *                                                                  \lb\lb
 *  In many cases, the ''FM_FFU_SLICE_ALLOCATIONS'' attribute can be 
 *  changed on the fly without disrupting traffic. The API will 
 *  reposition the boundary between ACLs and routes or relocate routes 
 *  automatically unless there is no way for it to do so, in which 
 *  case ''fmSetSwitchAttribute'' will return an error code. In such 
 *  a case, the application must take the following steps to effect 
 *  the change:
 *                                                                  \lb\lb
 *  1. Stop traffic (call ''fmSetPortState'' for affected ports).       \lb
 *  2. If changing the route slice allocation, delete all routes.       \lb
 *  3. If changing the ACL allocation, delete all ACLs.                 \lb
 *  4. Set ''FM_FFU_SLICE_ALLOCATIONS'' to the new allocation.          \lb
 *  5. If changing the route slice allocation, add back all routes.     \lb
 *  6. If changing the ACL allocation, re-apply the ACLs.               \lb
 *  7. Restart traffic (call ''fmSetPortState'' for affected ports).
 *                                                                  \lb\lb
 *  ACLs and CVLANs (if used) should each be given their own range of 
 *  slices, distinct from each other and from all route types. Each route
 *  type in use may also be given a distinct range of slices, however, the
 *  FFU may be more efficiently utilized if pairs of route types share
 *  slices. Slices are considered shared if the range for two route types
 *  overlap. Slices may not be shared by more than two route types.
 *                                                                  \lb\lb
 *  For each structure member listed, a value of -1 means "not used", i.e.,
 *  no slices should be used.  If -1 is used for either the first or last
 *  slice of a member pair (such as unicast routing), -1 must also be used
 *  for the other element in the pair.
 *
 *  When allocating BST slices for routing support, the following rules 
 *  are used:
 *                                                                  \lb\lb
 *  Only unicast routing is supported.
 *                                                                  \lb\lb
 *  All routing BST updates are transparent to the application and 
 *  done in a non-disruptive fashion.
 *                                                                  \lb\lb
 *  The BST table stores route entries with the most used prefixes. 
 *  Depending on the slice allocation, it can store entries with up 
 *  to 4 different prefixes for IPv4 and 1 prefix for IPv6 frames.
 *                                                                  \lb\lb
 *  Route entries are moved to the BST from the FFU in batches to 
 *  avoid performance issues. Therefore, new routes are first added 
 *  to FFU.
 *                                                                  \lb\lb
 *  The minimum number of entries in a batch is defined 
 *  by the api.FM6000.bst.minRouteEntriesInBatch API property. 
 *                                                                  \lb\lb 
 *  Route entries are swapped between the FFU and the BST when they 
 *  meet the following criteria:
 *                                                                  \lb\lb 
 *  ffuX  > (bstY + threshold)
 *                                                                  \lb\lb
 *  where:
 *                                                                  \lb\lb
 *  ffuX is a number of route entries with prefix X located in FFU     \lb
 *  bstY is a number of route entries with prefix Y located in BST     \lb
 *  threshold is an additional parameter to avoid frequent swapping 
 *  of route entries. The default value is defined by the 
 *  api.FM6000.thresholdValueForRouteSwapping API property.
 ****************************************************************************/
typedef struct _fm_ffuSliceAllocations
{
    /** First FFU slice used for IPv4 unicast routing. This attribute defaults 
     *  to the value supplied in the api.FMXXXX.ffu.unicastSliceRangeFirst
     *  API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv4UnicastFirstSlice;

    /** Last FFU slice used for IPv4 unicast routing. Each IPv4 unicast entry
     *  is 1 slice wide. This attribute defaults to the value supplied in the 
     *  api.FMXXXX.ffu.unicastSliceRangeLast API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv4UnicastLastSlice;

    /** First FFU slice used for IPv4 multicast routing. This attribute 
     *  defaults to the value supplied in the 
     *  api.FMXXXX.ffu.multicastSliceRangeFirst API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv4MulticastFirstSlice;

    /** Last FFU slice used for IPv4 multicast routing. Each IPv4 multicast 
     *  entry is 3 slices wide, so allocations should be in multiples of 3
     *  slices. This attribute defaults to the value supplied in the 
     *  api.FMXXXX.ffu.multicastSliceRangeLast API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv4MulticastLastSlice;

    /** First FFU slice used for IPv6 unicast routing. This attribute defaults 
     *  to the value supplied in the api.FMXXXX.ffu.unicastSliceRangeFirst
     *  API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv6UnicastFirstSlice;

    /** Last FFU slice used for IPv6 unicast routing. Each IPv6 unicast 
     *  entry is 4 slices wide, so allocations should be in multiples of 4
     *  slices. This attribute defaults to the value supplied in the 
     *  api.FMXXXX.ffu.unicastSliceRangeLast API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv6UnicastLastSlice;

    /** First FFU slice used for IPv6 multicast routing. This attribute 
     *  defaults to the value supplied in the
     *  api.FMXXXX.ffu.multicastSliceRangeFirst API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv6MulticastFirstSlice;

    /** Last FFU slice used for IPv6 multicast routing. Each IPv6 multicast 
     *  entry is 8 slices wide (9 for FM10000), so allocations should be in
     *  multiples of 8 slices (9 for FM10000). This attribute defaults to
     *  the value supplied in the api.FMXXXX.ffu.multicastSliceRangeLast API
     *  property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int ipv6MulticastLastSlice;

    /** First FFU slice used for ACLs. This attribute defaults to the value 
     *  supplied in the api.FMXXXX.ffu.aclSliceRangeFirst API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int aclFirstSlice;

    /** Last FFU slice used for ACLs. ACL entries vary in the how many slices
     *  wide they are based on the matching condition of the ACL rules. The
     *  number of slices allocated for ACLs should at a minimum be sufficient 
     *  to accommodate the widest matching condition that will be required.
     *  This attribute defaults to the value supplied in the 
     *  api.FMXXXX.ffu.aclSliceRangeLast API property.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int aclLastSlice;

    /** First FFU slice used for Customer VLANs. This attribute defaults to 
     *  the value supplied in the api.FM4000.ffu.cVlanSliceRangeFirst 
     *  API property.
     *  
     *  \chips  FM4000 */
    fm_int cVlanFirstSlice;

    /** Last FFU slice used for Customer VLANs. Each CVLAN entry is 1 slice 
     *  wide. This attribute defaults to the value supplied in the
     *  api.FM4000.ffu.cVlanSliceRangeLast API property.
     *  
     *  \chips  FM4000 */
    fm_int cVlanLastSlice;

    /** First BST slice used for unicast routing. This attribute defaults 
     *  to the value supplied in the api.FM6000.bst.routingSliceRangeFirst
     *  API property.
     *  
     *  \chips  FM6000 */
    fm_int bstRoutingFirstSlice;

    /** Last BST slice used for unicast routing. This attribute defaults 
     *  to the value supplied in the api.FM6000.bst.routingSliceRangeLast
     *  API property.
     *  
     *  \chips  FM6000 */
    fm_int bstRoutingLastSlice;

} fm_ffuSliceAllocations;




/**************************************************/
/** \ingroup typeEnum
 *  Set of possible enumerated values for the
 *  ''FM_CPU_MAC_TRAP_DISP'' switch attribute.  
 **************************************************/
typedef enum
{
     /** Activate the trap with IBV and EBV check. */
    FM_CPU_MAC_TRAP_DISP_IBV_AND_EBV_CHECK,

     /** Activate the trap with IBV check. */
    FM_CPU_MAC_TRAP_DISP_IBV_CHECK,

     /** Activate the trap. */
    FM_CPU_MAC_TRAP_DISP_ALWAYS,

     /** Deactivate the trap. */
    FM_CPU_MAC_TRAP_DISP_NONE,

} fm_cpuMacTrapDisp;


/**************************************************/
/** \ingroup typeStruct
 * This type is used as the argument for certain
 * FM6000 attributes that require a mask/value pair.
 **************************************************/
typedef struct _fm_maskValue
{
    fm_uint32 mask;
    fm_uint32 value;

} fm_maskValue;


/**************************************************/
/** \ingroup typeStruct
 * This type is used as the argument for certain
 * FM6000 attributes that require a 64-bit mask/value 
 * pair.
 **************************************************/
typedef struct _fm_maskValue64
{
    fm_uint64 mask;
    fm_uint64 value;

} fm_maskValue64;


/**************************************************/
/** \ingroup intTypeStruct
 * This type is used as the argument for certain
 * FM6000 attributes that require a 96-bit mask/value 
 * pair.
 **************************************************/
typedef struct _fm_maskValue96
{
    fm_uint32 mask[3];
    fm_uint32 value[3];

} fm_maskValue96;


/* Default hashing profile */
#define FM_L2_HASH_DEFAULT_PROFILE 0
#define FM_L3_HASH_DEFAULT_PROFILE 0

/**************************************************
 * These should be different from the routing one
 **************************************************/

/** \ingroup constSystem
 * @{ */
 
/** The default mantissa value used for the default rotation of
 *  ''FM_L2_HASH_KEY''. */
#define FM_L2_HASH_ROTATION_DEF_MANTISSA    51407

/** The default exponent value used for the default rotation of
 *  ''FM_L2_HASH_KEY''. */
#define FM_L2_HASH_ROTATION_DEF_EXPONENT    15



/**************************************************
 * These should be different from the LAG one
 **************************************************/

/** The default mantissa value used for the ''FM_L3_HASH_ROTATION_VALUE''
 *  switch attribute. */
#define FM_L3_HASH_ROTATION_DEF_MANTISSA    2       /* was 27017 */

/** The default exponent value used for the ''FM_L3_HASH_ROTATION_VALUE''
 *  switch attribute. The spec specifies a value of 8, but the hardware
 *  will add one to the value provided by the api. */
#define FM_L3_HASH_ROTATION_DEF_EXPONENT    0       /* was (8 - 1) */

/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeStruct
 * This type is used as the argument for hash
 * rotation configurations, ''FM_L3_HASH_ROTATION_VALUE''
 * and elements of ''FM_L2_HASH_KEY''.
 **************************************************/
typedef struct _fm_hashRotationValue
{
    /** The shift amount in the operation is one plus this value. */
    fm_byte   exponent;

    /** The amount the input hash value is multiplied by before shifting. */
    fm_uint16 mantissa;

} fm_hashRotationValue;


/**************************************************/
/** \ingroup typeStruct
 * This type is used as the argument for the hash
 * configuration switch attribute 
 * ''FM_L2_HASH_KEY''.
 **************************************************/
typedef struct _fm_L2HashKey
{
    /** Indicate the profile index to configure.
     *  Set to FM_L2_HASH_DEFAULT_PROFILE to refer to the default L2 hash
     *  profile.
     *  
     *  \chips  FM6000 */
    fm_uint32  profileIndex;

    /** Indicates the inclusion bit mask for the SMAC field.  
     *  The valid range is 0 (disable) to 0xffffffffffff (all bits
     *  included).  The default is 0xffffffffffff.
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_macaddr SMACMask;

    /** Indicates the inclusion bit mask for the DMAC field.  
     *  The valid range is 0 (disable) to 0xffffffffffff (all bits
     *  included).  The default is 0xffffffffffff.
     *                                                                  \lb\lb
     *  On FM300, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_macaddr DMACMask;

    /** Indicates the inclusion bit mask for the EtherType field.  
     *  The valid range is 0 (disable) to 0xffff (all bits
     *  included).  The default is 0xffff.
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  etherTypeMask;

    /** Indicates the inclusion bit mask for the VLAN ID 1 field.  
     *  The valid range is 0 (disable) to 0xfff (all bits
     *  included).  The default is 0xfff.
     *                                                                  \lb\lb
     *  On FM3000, FM4000, FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  vlanId1Mask;

    /** Indicates the inclusion bit mask for the VLAN Priority 1 field.  
     *  The valid range is 0 (disable) to 0xf. The default is 0xf (all
     *  bits included).
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  vlanPri1Mask;

    /** Indicates the inclusion bit mask for the VLAN ID 2 field.  
     *  The valid range is 0 (disable) to 0xfff (all bits included). The 
     *  default is 0xfff.
     *  
     *  \chips  FM6000 */
    fm_uint32  vlanId2Mask;

    /** Indicates the inclusion bit mask for the VLAN Priority 2 field.  
     *  The valid range is 0 (disable) to 0xf (all bits included). The 
     *  default is 0xf.
     *  
     *  \chips  FM6000 */
    fm_uint32  vlanPri2Mask;

    /** Indicates the default hash rotation to use if the rotation used
     *  is not derived from the L3 hash profile. The default value is:      \lb
     *  mantissa = ''FM_L2_HASH_ROTATION_DEF_MANTISSA''                     \lb
     *  exponent = ''FM_L2_HASH_ROTATION_DEF_EXPONENT''.
     *  
     *  \chips  FM6000 */
    fm_hashRotationValue defaultHashRot;

    /** Enable symmetrizing of the source and destination MAC fields.
     *  The default is FALSE.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_bool    symmetrizeMAC;

    /** Include the L34 hash value in the L2 hash. The default is TRUE.
     *  
     *  Note that setting both useL3Hash and useL2ifIP to false is not a
     *  valid configuration.
     * 
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool    useL3Hash;

    /** Include the Layer 2 header in the L2 hash in case of an IPv4/IPv6 
     *  packet or a non-ip packet. The default is TRUE.
     *  
     *  Note that setting both useL3Hash and useL2ifIP to false is not a
     *  valid configuration.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool    useL2ifIP;

    /** Enable FM2000 hash compatibility mode. FM_ENABLED or
     *  FM_DISABLED (default). Setting this attribute by itself is not enough
     *  to provide compatible hashing. See the datasheet for details.
     *  
     *  \chips  FM3000, FM4000 */
    fm_bool    useCompatibleMode;
      
} fm_L2HashKey;


/**************************************************/
/** \ingroup typeStruct
 * This type is used as the argument for hash rotation
 * configuration switch attribute ''FM_L2_HASH_ROT_A'' 
 * and ''FM_L2_HASH_ROT_B''.
 **************************************************/
typedef struct _fm_L2HashRot
{
    /** Indicate the profile index to configure. Set to 
     *  FM_L2_HASH_DEFAULT_PROFILE to refer to the default L2 hash
     *  profile.
     *  
     *  \chips  FM6000 */
    fm_uint32  profileIndex;

    /** Specifies one of four 12-bit L23 Hash rotations. The default is 1.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_uint32  hashRotation;

    /** Indicate if the L3 Hash result will be included as a key in this
     *  L2 Hash calculation. The default is TRUE.
     *  
     *  \chips  FM6000 */
    fm_bool    useL3HashKey;

    /** Indicate which rotation number to use. A value of FALSE (default) will
     *  use the L2 Hash default rotation (see ''fm_L2HashKey'') and a value of 
     *  TRUE will use the configured L3 Hash rotation.
     *  
     *  \chips  FM6000 */
    fm_bool    useL3HashRot;

    /** Enable use of the hardware PTable to facilitate improved hash results.
     *  Note that this will require configuring the PTable (register
     *  HASH_LAYER3_PTABLE). The default 
     *  is FALSE.
     *  
     *  \chips  FM6000 */
    fm_bool    usePTable;

    /** Enable use of a random L3 hash selection. The default is FALSE.
     *  
     *  \chips  FM6000 */
    fm_bool    randomSelection;

} fm_L2HashRot;


/**************************************************/
/** \ingroup typeStruct
 * This type is used as the argument for hash
 * configuration switch attribute 
 * ''FM_L3_HASH_CONFIG''.
 **************************************************/
typedef struct _fm_L3HashConfig
{
    /** Indicate the profile index to configure. Set to 
     *  FM_L3_HASH_DEFAULT_PROFILE to refer to the default L3 hash
     *  profile.
     *  
     *  \chips  FM6000 */
    fm_uint32  profileIndex;

    /** Indicates the inclusion byte mask for the SIP field. Each bit of this
     *  mask indicates a full byte of the SIP with bit 0 corresponding to 
     *  byte 0, bit 1 to byte 1, etc. The valid range is 0 (disable) to
     *  0x000f (all IPV4 bytes included) or 0xffff (all IPv6 bytes
     *  included, default).
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  On FM6000, if deep inspection is enabled, the SIP data
     *  channels may be filled with deep inspection data for
     *  non-ip / ipv4 frames. For such scenarios, multiple hashing
     *  profiles should be created as needed to apply a proper
     *  hashing mask to non-ip, ipv4, and ipv6 frames.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  SIPMask;

    /** Indicates the inclusion byte mask for the DIP field. Each bit of this
     *  mask indicates a full byte of the SIP with bit 0 corresponding to 
     *  byte 0, bit 1 to byte 1, etc. The valid range is 0 (disable) to
     *  0x000f (all IPV4 bytes included) or 0xffff (all IPv6 bytes
     *  included, default).
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  On FM6000, if deep inspection is enabled, the DIP data
     *  channel bits may be filled with deep inspection data for
     *  non-ip / ipv4 frames. For such scenarios, multiple hashing
     *  profiles should be created as needed to apply a proper
     *  hashing mask to non-ip, ipv4, and ipv6 frames.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  DIPMask;

    /** Indicates the inclusion bit mask for the layer 4 source port.
     *  The valid range is 0 (disable) to 0xffff (all bits included).
     *  The default is 0xffff.
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  L4SrcMask;

    /** Indicates the inclusion bit mask for the layer 4 destination port.
     *  The valid range is 0 (disable) to 0xffff (all bits included).
     *  The default is 0xffff.
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  L4DstMask;

    /** Indicates the inclusion bit mask for the DSCP field value.
     *  The valid range is 0 (disable) to 0x3f (all bits included). The
     *  default is 0xff.
     * 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  DSCPMask;

    /** Indicates the inclusion bit mask for the ISL_USER field value.
     *  The valid range is 0 (disable) to 0xff (all bits included).
     *  The default is 0.
     * 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  ISLUserMask;

    /** Indicates the inclusion bit mask for the layer 3 protocol field
     *  value. The valid range is 0 (disable) to 0xff (all bits included).
     *  The default is 0xff.
     *                                                                  \lb\lb
     *  On FM3000, FM4000 and FM10000 devices, this field acts as a Boolean
     *  and any value other than 0 will enable this specific key.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  protocolMask;

    /** Indicates the inclusion byte mask for the custom parser field value.
     *  This value is dependent on the parser microcode and FFU rules. Each 
     *  bit of this mask indicates a full byte of the custome parser field 
     *  with bit 0 corresponding to byte 0, bit 1 to byte 1, etc. The valid 
     *  range is 0 (disable) to 0xff (all bytes included). The default is 0.
     * 
     *  \chips  FM6000 */
    fm_uint32  customFieldMask;

    /** Indicates the inclusion bit mask for the IPv6 flow field value.
     *  The valid range is 0 (disabled) to 0xfffff (all bits included).
     *  The default is 0xfffff.
     * 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32  flowMask;

    /** Enable symmetrizing of the SIP & DIP fields. This ensures that frames
     *  with opposite SIP & DIP fields will hash the same with respect to
     *  those fields. The default is FALSE.
     *  
     *  On FM3000, FM4000 and FM10000 devices, this field also controls L4
     *  symmetry (source and destination).
     * 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_bool    symmetrizeL3Fields;

    /** Enable symmetrizing of the layer 4 source and destination port fields. 
     *  This ensures that frames with opposite source & destination port fields
     *  will hash the same with respect to those fields. The default is FALSE.
     * 
     *  \chips  FM6000 */
    fm_bool    symmetrizeL4Fields;

    /** Enable use of the hardware PTable to facilitate improved hash results.
     *  Note that this will require configuring the PTable (register
     *  HASH_LAYER3_PTABLE). The default 
     *  is FALSE.
     * 
     *  \chips  FM6000 */
    fm_bool    usePTable;

    /** Enable producing a random value to the next-hop evaluation stage.
     *  Default is FALSE.
     *  
     *  \chips  FM6000 */
    fm_bool    randomNextHop;

    /** Enable producing a random value to other stages downstream of the
     *  hash calculation. Default is FALSE.
     *  
     *  \chips  FM6000 */
    fm_bool    randomOther;

    /** If set to true, disable all hash computation except for the random 
     *  outputs, if they are enabled. Default is FALSE.
     *  
     *  \chips  FM6000 */
    fm_bool    disableAllButRandom;

    /** Specifies one of three 12-bit hash rotations for use in ECMP binning.
     *  Default is 0.
     *  
     *  \chips  FM4000, FM10000 */
    fm_uint32  ECMPRotation;

    /** When the useProtocol1 flag in this structure is set, the layer 4 
     *  source and destination  will contribute to the hash only if the 
     *  frame's protocol field matches the value of this attribute. The 
     *  attribute value ranges from 0 to 255 with a default value of 1.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_uint32  protocol1;

    /** When the useProtocol2 flag in this structure is set, the layer 4 
     *  source and destination  will contribute to the hash only if the 
     *  frame's protocol field matches the value of this attribute. The 
     *  attribute value ranges from 0 to 255 with a default value of 1.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_uint32  protocol2;

    /** Enable use of the L4SRC and L4DST in the L3 hash calculation
     *  for the TCP protocol. One or more of the following fields must be
     *  enabled for this to work: L4SrcMask L4DstMask. Default is TRUE.
     * 
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool    useTCP;

    /** Enable use of the L4SRC and L4DST in the L3 hash calculation
     *  for the UDP protocol. One or more of the following fields must be
     *  enabled for this to work: L4SrcMask L4DstMask. Default is TRUE.
     * 
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool    useUDP;

    /** Enable use of the L4SRC and L4DST in the L3 hash calculation
     *  for the "protocol1" protocol. One or more of the following fields must 
     *  be enabled for this to work: L4SrcMask L4DstMask. Default is FALSE.
     * 
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool    useProtocol1;

    /** Enable use of the L4SRC and L4DST in the L3 hash calculation
     *  for the "protocol2" protocol. One or more of the following fields must 
     *  be enabled for this to work: L4SrcMask L4DstMask. Default is FALSE.
     * 
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool    useProtocol2;

} fm_L3HashConfig;


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the 
 *  ''FM_SWITCH_TRAP_CODE'' and ''FM_SWITCH_TRAP_TYPE''
 *  switch attributes.
 **************************************************/
typedef struct _fm_trapCodeMapping
{
    /** TrapType ID. */
    fm_trapType type;

    /** Trap code. */
    fm_int      code;

} fm_trapCodeMapping;


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the switch attributes
 *  ''FM_SWITCH_PARSER_VLAN_ETYPES'' and 
 *  ''FM_SWITCH_MODIFY_VLAN_ETYPES''.
 **************************************************/
typedef struct _fm_vlanEtherType
{
    /** Tag index, 0 through
     *  ''FM_MAX_PARSER_VLAN_ETYPE_INDEX'' for ''FM_SWITCH_PARSER_VLAN_ETYPES'',  
     *  ''FM_MAX_MODIFY_VLAN_ETYPE_INDEX'' for ''FM_SWITCH_MODIFY_VLAN_ETYPES''. 
     */
    fm_int      index;

    /** L2 Ethernet type for this index. */
    fm_uint16   etherType;

} fm_vlanEtherType;


/****************************************************************/
/** \ingroup constSystem
 *  Maximum index value allowed for the
 * ''FM_SWITCH_PARSER_VLAN_ETYPES'' switch attribute.
 ****************************************************************/
#define FM_MAX_PARSER_VLAN_ETYPE_INDEX 3


/****************************************************************/
/** \ingroup constSystem
 *  Maximum index value allowed for the
 *  ''FM_SWITCH_MODIFY_VLAN_ETYPES'' switch attribute.
 ****************************************************************/
#define FM_MAX_MODIFY_VLAN_ETYPE_INDEX 3


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the
 *  ''FM_SWITCH_MPLS_ETHER_TYPES'' switch attribute.
 **************************************************/
typedef struct _fm_mplsEtherType
{
    /** Tag index, 0 through ''FM_NUM_MPLS_ETHER_TYPES'' - 1. */
    fm_int      index;

    /** L2 Ethernet type for this index. */
    fm_uint16   etherType;

} fm_mplsEtherType;


/****************************************************************/
/** \ingroup constSystem 
 *  The maximum number of Ethernet types that may be defined
 *  via the ''FM_SWITCH_MPLS_ETHER_TYPES'' switch attribute. 
 ****************************************************************/
#define FM_NUM_MPLS_ETHER_TYPES     2


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the
 *  ''FM_SWITCH_RESERVED_MAC_CFG'' switch attribute.
 **************************************************/
typedef struct _fm_reservedMacCfg
{
    /** The low-order octet of the destination MAC address to be
     *  configured, in the range 0..(FM_NUM_RESERVED_MACS-1). The
     *  full address is of the form 01-80-C2-00-00-XX. */
    fm_int  index;

    /** The action to be performed. See ''fm_resMacAction'' for possible
     *  values. */
    fm_int  action;

    /** Whether to use the switch priority specified by the
     *  ''FM_SWITCH_RESERVED_MAC_TRAP_PRI'' switch attribute when a frame
     *  is trapped to the CPU. */
    fm_bool useTrapPri;

} fm_reservedMacCfg;


/****************************************************************/
/** \ingroup constSystem 
 *  The maximum number of reserved MAC addresses that may be
 *  configured using the ''FM_SWITCH_RESERVED_MAC_CFG'' switch
 *  attribute. 
 ****************************************************************/
#define FM_NUM_RESERVED_MACS        64


/****************************************************************/
/** \ingroup typeEnum 
 *  Set of possible actions that may be specified for an IEEE
 *  reserved multicast address via the ''FM_SWITCH_RESERVED_MAC_CFG''
 *  switch attribute. 
 ****************************************************************/
typedef enum
{
    /** Switch the frame normally. */
    FM_RES_MAC_ACTION_SWITCH = 0,

    /** Trap the frame to the CPU.  */
    FM_RES_MAC_ACTION_TRAP,

    /** Drop the frame. */
    FM_RES_MAC_ACTION_DROP,

    /** Log the frame. */
    FM_RES_MAC_ACTION_LOG,

    /** UNPUBLISHED: For internal use only. */
    FM_RES_MAC_ACTION_MAX

} fm_resMacAction;


/****************************************************************/
/** \ingroup constSystem
 *  The maximum index value allowed for the index element in the
 *  fm_customTag structure.
 ****************************************************************/
#define FM_MAX_CUSTOM_TAG_INDEX 3


/**************************************************/
/** \ingroup typeStruct
 *  Used as a member in fm_customTag.
 *                                          \lb\lb
 *  See ''Deep Inspection'' in the User Guide for
 *  important information about the members of
 *  this structure.
 **************************************************/
typedef struct _fm_customTagConfig
{
    /** 16-bit Ethernet type field to identify custom Ethernet type tagged 
     *  frames. Values should be distinct from the standard Ethertypes 
     *  0x0800 (IPv4), 0x86dd (IPv6), 0x8808 (MAC Control), and the MPLS 
     *  ethertypes configured in ''FM_SWITCH_MPLS_ETHER_TYPES''. 
     *  The default is 0 for all indices. */
    fm_uint16   etherType;

    /** Four bits to specify which 16-bit half words to capture in the next four 
     *  half words following the EtherType. A maximum of two 16-bit half words 
     *  may be captured. Default is 0 for all indices.
     *                                                                 \lb\lb
     *  For example, if a frame contains a custom EtherType followed by the
     *  half word values X, Y and Z:
     *                                                                 \lb\lb
     *  CustomEtherType                                                 \lb
     *  X                                                               \lb
     *  Y                                                               \lb
     *  Z                                                               \lb
     *                                                                 \lb\lb
     *  then a binary value of 0011 would select half words X and Y,
     *  extracting them to L4A and L4B, respectively or L4C and L4D,
     *  respectively,  depending on captureSelect. The length of such a
     *  custom header would be 4. */
    fm_byte     capture;

    /** Indicate whether to capture the half words specified by capture
     *  into L4A/B (FALSE) or L4C/D (TRUE). The Default is FALSE for all 
     *  indices. The captured data can be matched using the ACL condition 
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT'':
     *                                                                  \lb\lb
     *  fm_aclValue.L4DeepInspectionExt[0..1] == L4A                    \lb
     *  fm_aclValue.L4DeepInspectionExt[2..3] == L4B                    \lb
     *  fm_aclValue.L4DeepInspectionExt[4..5] == L4C                    \lb
     *  fm_aclValue.L4DeepInspectionExt[6..7] == L4D                    \lb
     *                                                                  \lb
     *  This ACL Condition also works on non-IP frames if capture is set. */
    fm_bool     captureSelect;

    /** Length of the custom tag in 16-bit increments, including the Ethertype
     *  itself. Valid values are 1..9. Default is 1 for all indices. */
    fm_byte     length;

} fm_customTagConfig;


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the
 *  ''FM_SWITCH_PARSER_CUSTOM_TAG'' switch attribute.
 **************************************************/
typedef struct _fm_customTag
{
    /** Tag index, 0 through ''FM_MAX_CUSTOM_TAG_INDEX''. This is used to identify
     *  the index of the global custom tags. */
    fm_int             index;

    /** Structure to hold all fields of custom tag. */
    fm_customTagConfig customTagConfig;

} fm_customTag;


/****************************************************************/
/** \ingroup constSystem
 *  The maximum index value allowed for the index element in the
 *  ''fm_parserDiCfg'' structure.
 ****************************************************************/
#define FM_MAX_PARSER_DI_CFG_INDEX 5


/**************************************************/
/** \ingroup typeStruct
 *  Used as a member in ''fm_parserDiCfg''.
 **************************************************/
typedef struct _fm_parserDiCfgFields
{
    /** Whether this entry is enabled. */
    fm_bool      enable;

    /** L4 Protocol number to consider while DI matching. Default is 0 for 
     *  all indices. Must be set to 0 for ''fm_parserDiCfg'' index zero. */
    fm_byte      protocol;

    /** L4 Port (Source/Destination) number to be captured. Default is 0
     *  for all indices. */
    fm_uint16    l4Port;

    /** Whether L4 port is considered in the match.  Must be set to 0 for 
     *  ''fm_parserDiCfg'' index zero. */
    fm_bool      l4Compare;

    /** Each of the eight 4-bit nybbles in this field represents a word 
     *  offset to capture following the L4 header. A nybble value of 0xF 
     *  means do not capture, and is the default for all nybbles.
     *                                                              \lb\lb
     *  See ''Deep Inspection'' in the User Guide for more information,
     *  including restrictions and special circumstances. */
    fm_uint32    wordOffset;

    /** Whether to capture TCP flags and header length in L4A for TCP
     *  frames. Affects the values that may be used for wordOffset. See
     *  ''Deep Inspection'' in the User Guide for more information. */
    fm_bool      captureTcpFlags;

} fm_parserDiCfgFields;


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the
 *  ''FM_SWITCH_PARSER_DI_CFG'' switch attribute.
 *                                           \lb\lb
 *  Note that index 0 is the default configuration
 *  that will be selected if the frame does not
 *  match any of the other DI configurations.
 **************************************************/
typedef struct _fm_parserDiCfg
{
    /** Tag index, 0 through ''FM_MAX_PARSER_DI_CFG_INDEX''. This is used to
     *  identify the index of the global DI Filters. */
    fm_int    index;

    /** Structure to hold all fields of a DI configuration */
    fm_parserDiCfgFields parserDiCfgFields;

} fm_parserDiCfg;


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the ''FM_SWITCH_VLAN1_MAP''
 *  and ''FM_SWITCH_VLAN2_MAP'' switch attributes.
 **************************************************/
typedef struct _fm_vlanMapEntry
{
    /** Set to a value in the range 0..4095 */
    fm_uint32 entry;

    /** The mapped value */
    fm_uint32 value;

} fm_vlanMapEntry;


/****************************************************************/
/** \ingroup constSystem
 *  The maximum value allowed for the length element in the
 *  fm_customTagConfig structure.
 ****************************************************************/
#define FM_MAX_CUSTOM_TAG_LENGTH 9


/****************************************************************/
/** \ingroup constSystem
 *  The minimum value allowed for the length element in the
 *  fm_customTagConfig structure.
 ****************************************************************/
#define FM_MIN_CUSTOM_TAG_LENGTH 1


/****************************************************************/
/** \ingroup constSystem
 *  The maximum number of 16-bit words(half words) that can be
 *  stored from outermost MPLS tag.
 ****************************************************************/
#define FM_MAX_NUM_HALF_WORDS_MPLS_TAG 4


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the
 * ''FM_SWITCH_SGLORT_VSI_MAP'' switch attribute.
 **************************************************/
typedef struct _fm_sglortVsiMap
{
    /** Tunneling Engine Number: 0 or 1 */
    fm_int    te;

    /** Index into the SGLORT/VSI table. Valid values are 0 through 7. */
    fm_int    index;

    /** SGLORT value. This value will be masked using sglortMask before the
     *  comparison is made. Setting mask and value both to 0 means always match.
     *  Setting mask to 0 and value to non-zero means never match. */
    fm_uint16 sglortValue;

    /** SGLORT mask. Selects which bits should be compared. Set to 0xFFFF
     *  means that all bits are compared. Setting mask and value both to 0 means
     *  always match. Setting mask to 0 and value to something else means never
     *  match. */
    fm_uint16 sglortMask;

    /** Location of the first bit of the VSI in the SGLORT, in the range of
     *  0 through 31. */
    fm_int    vsiStart;

    /** Length of the VSI in the SGLORT. Valid values are 0 to 31. 0 means
     *  the VSI will be set to the value of vsiOffset. */
    fm_int    vsiLen;

    /** Offset to add to the VSI extracted from SGLORT. Valid values are 0
     *  to 255. */
    fm_int    vsiOffset;

} fm_sglortVsiMap;


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible fields in a bit mask which can
 * be modified by the ''FM_SWITCH_VSI_DATA'' switch
 * attribute. For every value selected, the corresponding
 * field in the ''fm_vsiData'' structure must be set.
 **************************************************/
typedef enum
{
    /** Set the Source IP address used when encapsulating traffic for this VSI. */
    FM_VSI_DATA_ENCAP_SIP = 1 << 0,

    /** Set the VNI used when encapsulating traffic for this VSI. */
    FM_VSI_DATA_ENCAP_VNI = 1 << 1,

} fm_vsiDataMask;


/**************************************************/
/** \ingroup typeStruct
 *  Used as the argument type for the
 * ''FM_SWITCH_VSI_DATA'' switch attribute.
 **************************************************/
typedef struct _fm_vsiData
{
    /** Tunneling Engine. Values are 0 or 1. TE 1 can be set, but the VN API
     *  only uses Tunnel 0 for encapsulation. */
    fm_int te;

    /** VSI number. Values are 0 through 255. */
    fm_int vsi;

    /** Mask of fields to set for the VSI, taken from ''fm_vsiDataMask''. */
    fm_uint dataMask;

    /** Encapsulation SIP Value, if ''FM_VSI_DATA_ENCAP_SIP'' is set in the
     *  dataMask field. */
    fm_ipAddr encapSip;

    /** Encapsulation VNI value, if ''FM_VSI_DATA_ENCAP_VNI'' is set in the
     *  dataMask field. */
    fm_uint32 encapVni;

} fm_vsiData;


/**************************************************/
/** \ingroup intTypeEnum
 * Logical port specific attributes used with
 * fmGetLogicalPortAttribute and 
 * fmSetLogicalPortAttribute. 
 **************************************************/
typedef enum
{
    /** Type fm_portmask: Destination mask this logical port maps to.
     *  Note that this is a lightweight internal representation of a
     *  physical port mask. This attribute is NOT supported on SWAG
     *  switches.
     *  \chips  FM2000, FM4000, FM6000, FM1000 */
    FM_LPORT_DEST_MASK = 0,

    /** Type fm_uint32: Multicast index associated with this logical port.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_LPORT_MULTICAST_INDEX,

    /** Type fm_portType: Logical port type.
     *  \chips  FM10000 */
    FM_LPORT_TYPE,

    /** Type fm_portTxTimestampMode: Tx Timestamp mode of logical port
     *  belonging to a PCIE port. This is valid only for PCIE physical port
     *  and virtual ports. See fm_portTxTimestampMode for more details.
     *  \chips  FM10000 */
    FM_LPORT_TX_TIMESTAMP_MODE,

    /** UNPUBLISHED: For internal use only. */
    FM_LPORT_ATTR_MAX

} fm_logicalPortAttribute;


/* functions to get and set global attributes */
fm_status fmGetSwitchAttribute(fm_int sw, fm_int attr, void *value);
fm_status fmSetSwitchAttribute(fm_int sw, fm_int attr, void *value);
fm_status fmStopSwitchTraffic(fm_int sw);
fm_status fmRestartSwitchTraffic(fm_int sw);
fm_status fmGetLogicalPortAttribute(fm_int sw,
                                    fm_int port,
                                    fm_int attr,
                                    void * value);


#endif /* __FM_FM_API_ATTR_H */
