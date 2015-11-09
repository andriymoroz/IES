/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_property_int.h
 * Creation Date:   October 2015
 * Description:     Internal API Property definitions.
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

#ifndef __FM_FM_PROPERTY_INT_H
#define __FM_FM_PROPERTY_INT_H


typedef struct _fm_property
{

    /* Default spanning tree state for VLAN member port */
    fm_int  defStateVlanMember;

    /* Allow the CPU to send to the CPU in the directed packet send mode */
    fm_bool directSendToCpu;

    /* Default spanning tree state for VLAN non-member port */
    fm_int  defStateVlanNonMember;

    /* Automatically identify the switch device type upon a insert event */
    fm_bool bootIdentifySw;

    /* Reboot the switch following identification */
    fm_bool bootReset;

    /* Automatically generate insert events for switches known at startup. */
    fm_bool autoInsertSwitches;

    /* Length of time in ns to hold the device in reset */
    fm_int  deviceResetTime;

    /* Automatically enable witch-Aggregate links */
    fm_bool swagAutoEnableLinks;

    /* Threshold to block low priority event request */
    fm_int  eventBlockThreshold;

    /* Threshold to unblock low priority event request */
    fm_int  eventUnblockThreshold;

    /* Timeout (millisecond) value for the semaphore blocking on event free */
    fm_int  eventSemTimeout;

    /* Received packets are to be queued directly to the application */
    fm_bool rxDirectEnqueueing;

    /* Destinations to which received packets are to be forwarded */
    fm_int  rxDriverDestinations;

    /* Wait for the LAG group deletion to complete */
    fm_bool lagAsyncDeletion;

    /* Generate event on static MAC address */
    fm_bool maEventOnStaticAddr;

    /* Generate event on dymanic MAC address */
    fm_bool maEventOnDynAddr;

    /* Generate event on MAC address change */
    fm_bool maEventOnAddrChange;

    /* Flush all the addresses associated with a port when down */
    fm_bool maFlushOnPortDown;

    /* Flush MAC addresss on VLAN changed */
    fm_bool maFlushOnVlanChange;

    /* Flush MAC addresses associated with a port on LAG changed */
    fm_bool maFlushOnLagChange;

    /* Maximum number of TCN FIFO entries to be processed in a single cycle */
    fm_int  maTcnFifoBurstSize;

    /* Collect VLAN statistics for internal ports */
    fm_bool swagIntVlanStats;

    /* Control on each of the LAG member ports */
    fm_bool perLagManagement;

    /* Enable parity repair */
    fm_bool parityRepairEnable;

    /* Maximum number of ACL port sets supported on a SWAG */
    fm_int  swagMaxAclPortSets;

    /* Maximum number of port sets */
    fm_int  maxPortSets;

    /* Enable the packet receive thread */
    fm_bool packetReceiveEnable;

    /* Enables single multicast address per group */
    fm_bool multicastSingleAddress;

    /* White model position */
    fm_int  modelPosition;

    /* Number of next-hop records to be reserved for virtual network tunnels */
    fm_int  vnNumNextHops;

    /* Encapsulation protocol */
    fm_int  vnEncapProtocol;

    /* Encapsulation version */
    fm_int  vnEncapVersion;

    /* Specify route lookups by IP address are supported */
    fm_bool supportRouteLookups;

    /* Enable routing maintenance thread  */
    fm_bool routeMaintenanceEnable;

    /* Enable automatic vlan2 tagging */
    fm_bool autoVlan2Tagging;
    
    /* Disable interrupt handler */
    fm_bool interruptHandlerDisable;

    /* Enable the MAC table maintenance thread */
    fm_bool maTableMaintenanceEnable;

    /* Enable the fast maintenance thread */
    fm_bool fastMaintenanceEnable;

    /* Fast maintenance thread period */
    fm_int  fastMaintenancePer;

    /* Disable the physical port GLORT LAG Filtering */
    fm_bool strictGlotPhysical;

    /* Resetting the watermark registers when pause mode is disabled */
    fm_bool resetWmAtPauseOff;

    /* Automatically maintain sub-switch state for the application */
    fm_bool swagAutoSubSwitches;

    /* Automatically manage internal (inter-switch) ports */
    fm_bool swagAutoIntPorts;

    /* Automatically manage Virtual Networking VSI values */
    fm_bool swagAutoVNVsi;

    /* Enables bypass mode on startup */
    fm_bool byPassEnable;

    /* Delete semaphore timeout */
    fm_int  lagDelSemTimeout;

    /* Enable changing spanning tree state of internal ports */
    fm_bool stpEnIntPortCtrl;

    /* Selects the logical port map to use with the White Model */
    fm_int  modelPortMapType;

    /* Model switch type */
    fm_char modelSwitchType[32];

    /* Send EOT on each received packet */
    fm_bool modelSendEOT;

    /* Log egress info */
    fm_bool modelLogEgressInfo;

    /* Enable IEEE 1588 reference clock */
    fm_bool enableRefClock;

    /* Initialize the IEEE 1588 reference clock */
    fm_bool setRefClock;

    /* Enable priority buffer queues  */
    fm_bool priorityBufQueues;

    /* Specifies the type of packet scheduling to application */
    fm_int  pktSchedType;

    /* Enable separate buffer pool */
    fm_bool separateBufPoolEnable;

    /* Number of RX buffers to be allocated */
    fm_int  numBuffersRx;

    /* Number of TX buffers to be allocated */
    fm_int  numBuffersTx;

    /* Name of the network interface that should be used by the white model  */
    fm_char modelPktInterface[32];

    /* Method of receiving or injecting a packet into the fabric */
    fm_char pktInterface[32];

    /* Multi-switch topology to use for a multi-node/multi-switch white model platform*/
    fm_char modelTopologyName[16];

    /* Use the model file path when opening the multi-switch topology */
    fm_bool modelUseModelPath;

    /* Specifies SERDES development board IP address */
    fm_char modelDevBoardIp[16];

    /* Specifies SERDES development board TCP port */
    fm_int  modelDevBoardPort;

    /* Value of the DEVICE_CFG register for the White Mode */
    fm_int  modelDeviceCfg;

    /* Value of the CHIP_VERSION register for the White Model */
    fm_int  modelChipVersion;

    /* TCP port where the SERDES SBUS Server is listening on */
    fm_int  sbusServerPort;

    /* Indicates whether the API is running on top of the White Model */
    fm_bool isWhiteModel;

    /* Logging catergories to enable when API is initializing */
    fm_char initLoggingCat[256];

    /* PEP ports should be added to flooding lists */
    fm_bool addPepsToFlooding;

    /* Indicates if VLAN tagging is allowed for FTAG enabled ports. */
    fm_bool allowFtagVlanTagging;

    /* Ignore scheduler bandwidth violation */
    fm_int  ignoreBwViolation;

    /* Enable early link up mode */
    fm_bool dfeAllowEarlyLinkUp;

    /* Enable pCal on KR modes */
    fm_bool dfeAllowKrPcal;

    /* Enable signalOk debouncing */
    fm_bool dfeEnableSigOkDebounce;

    /* Enable port status polling */
    fm_bool enableStatusPolling;

    /* GSME timestamping mode */
    fm_int  gsmeTimestampMode;

    /* Create multicast groups on HNI request as flooding ones */
    fm_bool hniMcastFlooding;

    /* Maximum number of MAC table entries per PEP port */
    fm_int  hniMacEntriesPerPep;

    /* Maximum number of MAC table entries per virtual port */
    fm_int  hniMacEntriesPerPort;

    /* Maximum number of inner/outer MAC filtering entries per PEP port */
    fm_int  hniInnOutEntriesPerPep;

    /* Maximum number of inner/outer MAC filtering entries per virtual port */
    fm_int  hniInnOutEntriesPerPort;

    /* Use values out of the valid range */
    fm_bool anTimerAllowOutSpec;

} fm_property;


fm_status fmLoadApiPropertyTlv(fm_byte *tlv);



#endif /* __FM_FM_PROPERTY_INT_H */

