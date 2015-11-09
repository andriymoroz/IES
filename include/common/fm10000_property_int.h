/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_property_int.h
 * Creation Date:   October 2015
 * Description:     Internal FM10000 API property definitions.
 *
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __FM_FM10000_PROPERTY_INT_H
#define __FM_FM10000_PROPERTY_INT_H


typedef struct _fm10000_property
{
    /* Watermark methodology */
    fm_char wmSelect[16];

    /* Amount of memory to allocate to each port for RX private  */
    fm_int  cmRxSmpPrivBytes;

    /* Amount of memory to allocate to each port for the TX hog watermark */
    fm_int  cmTxTcHogBytes;

    /* Congestion management SMP soft drop versus hog watermark delta  */
    fm_int  cmSmpSdVsHogPercent;

    /* Number of jitter bits to be used in the congestion management */
    fm_int  cmSmpSdJitterBits;

    /* Apply a random soft drop when TX private is exceeded */
    fm_bool cmTxSdOnPrivate;

    /* Apply a random soft drop based on total SMP usage */
    fm_bool cmTxSdOnSmpFree;

    /* Amount of memory (in bytes) to reserve for pause frame */
    fm_int  cmPauseBufferBytes;

    /* Number of destination entries associated with each GLORT CAM */
    fm_int  mcastMaxEntriesPerCam;

    /* First FFU slice allocated for unicast routing */
    fm_int  ffuUcastSliceRangeFirst;

    /* First FFU slice allocated for unicast routing */
    fm_int  ffuUcastSliceRangeLast;

    /* First FFU slice allocated for multicast routing */
    fm_int  ffuMcastSliceRangeFirst;

    /* Last FFU slice allocated for multicast routing */
    fm_int  ffuMcastSliceRangeLast;

    /* First FFU slice allocated for ACL */
    fm_int  ffuAclSliceRangeFirst;
 
   /* First FFU slice allocated for ACL */
    fm_int  ffuAclSliceRangeLast;

    /* Number of entries in the MAC Mapper reserved for router MAC addresses */
    fm_int  ffuMapMacResvdForRoute;

    /* Minimum precedence value allowed for unicast FFU entries */
    fm_int  ffuUcastPrecedenceMin;

     /* Maximun precedence value allowed for unicast FFU entries */
    fm_int  ffuUcastPrecedenceMax;

    /* Minimum precedence value allowed for multicast FFU entries */
    fm_int  ffuMcastPrecedenceMin;

    /* Maximum precedence value allowed for multicast FFU entries */
    fm_int  ffuMcastPrecedenceMax;

    /* Minimum precedence value allowed for ACL FFU entries */
    fm_int  ffuAclPrecedenceMin;

    /* Maximum precedence value allowed for ACL FFU entries */
    fm_int  ffuAclPrecedenceMax;

    /* Enable strict counter and policer validation */
    fm_bool ffuAclStrictCountPolice;

    /* Initialize unicast flooding triggers at boot time */
    fm_bool initUcastFloodTriggers;

    /* Initialize multicast flooding triggers at boot time */
    fm_bool initMcastFloodTriggers;

    /* Initialize broadcast flooding triggers at boot time */
    fm_bool initBcastFloodTriggers;

    /* Initialize reseved MAC triggers at boot time */
    fm_bool initResvdMacTriggers;

    /* Force the switch priority of packets trapped */
    fm_int  floodingTrapPriority;

    /* Enable auto-negotiation events */
    fm_bool autonegGenerateEvents;

    /* Link status is considered dependent on the DFE completed */
    fm_bool linkDependsOfDfe;

    /* Use shared encapsulation flows during VN tunneling */
    fm_bool vnUseSharedEncapFlows;

    /* Maximum number of remote addresses the Virtual Networking  */
    fm_int  vnMaxRemoteAddress;

    /* Hash size for VN tunnel groups */
    fm_int  vnTunnelGroupHashSize;

    /* VLAN Id for all traffic coming from the tunnel engine back to switch */
    fm_int  vnTeVid;

    /* ACL rule that will be used for VN Encapsulation rules */
    fm_int vnEncapAclNumber;

    /* ACL rule that will be used for VN Decapsulation rules */
    fm_int  vnDecapAclNumber;

    /* Number of Multicast Groups that will be available stack-global */
    fm_int  mcastNumStackGroups;

    /* Remove TE ports from internal port masks */
    fm_bool vnTunnelOnlyOnIngress;

    /* Watermark for MTable garbage collection */
    fm_int  mtableCleanupWm;

    /* Scheduler mode  */
    fm_char schedMode[16];

    /* Scheduler bandwidth assignments should be updated on link down/up */
    fm_bool updateSchedOnLinkChange;

    /* Automatically create logical ports for remote glorts */
    fm_bool createRemoteLogicalPorts;

    /* Clause 37 auto-negotiation state machine link timeout */
    fm_int  autonegCl37Timeout;

    /* SGMII auto-negotiation state machine link timeout */
    fm_int  autonegSgmiiTimeout;

    /* Use loopback mode for host network interface services */
    fm_bool useHniServicesLoopback;

    /* EPL TX FIFO Anti-Bubble Watermark */
    fm_int  antiBubbleWm;

    /* SERDES operational mode*/
    fm_int  serdesOpMode;

    /* SERDES debug level */
    fm_int  serdesDbgLevel;

    /* Enable parity interrupts */
    fm_bool parityEnableInterrupts;

    /* Enable the TCAM monitors */
    fm_bool parityStartTcamMonitors;

    /* Parity CRM timeout */
    fm_int parityCrmTimeout;

    /* Scheduler overspeed */
    fm_int  schedOverspeed;

    /* Override interrupt masks */
    fm_int  intrLinkIgnoreMask;
    fm_int  intrAutonegIgnoreMask;
    fm_int  intrSerdesIgnoreMask;
    fm_int  intrPcieIgnoreMask;
    fm_int  intrMaTcnIgnoreMask;
    fm_int  intrFhTailIgnoreMask;
    fm_int  intrSwIgnoreMask;
    fm_int  intrTeIgnoreMask;

    /* Enable EEE spico interrupt */
    fm_bool enableEeeSpicoIntr;

    /* Use alternate SPICO firmware */
    fm_bool useAlternateSpicoFw;

    /* Allow Kr PCAL on EEE */
    fm_bool allowKrPcalOnEee;

} fm10000_property;

#endif /* __FM_FM10000_PROPERTY_INT_H */

