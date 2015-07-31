/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_port_int.h
 * Creation Date:   2005
 * Description:     Contains functions dealing with the state of individual
 *                  ports
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

#ifndef __FM_FM_API_PORT_INT_H
#define __FM_FM_API_PORT_INT_H

/**************************************************/
/** \ingroup intTypeEnum
 * Identifies the device family this port belongs
 * to.
 **************************************************/
typedef enum
{
    FM_PORT_FAMILY_UNKNOWN = 0,
    FM_PORT_FAMILY_FM2000,
    FM_PORT_FAMILY_FM4000,
    FM_PORT_FAMILY_REMOTE_FM4000,

    /* This entry must be last */
    FM_PORT_FAMILY_MAX

} fm_portFamily;


/**************************************************
 * Identifies the port attribute storage type.
 **************************************************/
typedef enum
{
    FM_TYPE_INT = 0,
    FM_TYPE_UINT16,
    FM_TYPE_UINT32,
    FM_TYPE_UINT64,
    FM_TYPE_BOOL,
    FM_TYPE_PORTMASK,
    FM_TYPE_BITARRAY,
    FM_TYPE_MACADDR,
    FM_TYPE_PAUSE_PACING_TIME,

} fm_attrStorageType;


/**************************************************
 * Identifies the port attribute structure this 
 * attribute belongs to.
 **************************************************/
typedef enum
{
    /* Indicates the attribute belongs to the port generic structure,
     * that is, fm_portAttr */
    FM_PORT_ATTR_GENERIC = 0,

    /* Indicates the attribute belongs to the port extension structure,
     * i.e. fm6000_portAttr */
    FM_PORT_ATTR_EXTENSION,

    /* Indicates the attribute belongs to the microcode structure
     * i.e. fm6000_ucAttr */
    FM_PORT_ATTR_MICROCODE

} fm_attrType;

/**************************************************
 * Generic port attribute entry structure
 **************************************************/
typedef struct _fm_portAttrEntry
{
    /* The FM_PORT_XXX attribute associated with this entry */
    fm_int attr;

    /* The FM_PORT_XXX attribute put in " " (e.g. "FM_PORT_MASK") to have it
     * as a string for 'printf' purpose */
    fm_text str;

    /* Indicates the attribute storage type as defined in fm_attrStorageType
     * (i.e. FM_TYPE_UINT32)  */
    fm_attrStorageType type;

    /* Indicates whether the attribute can be set on the LAG logical port */
    fm_bool perLag;

    /* Indicates the port attribute structure the attribute belongs to.
     * Either fm_portAttr or fmX000_portAttr structure as defined by one
     * of the FM_PORT_ATTR_XXX (i.e. FM_PORT_ATTR_GENERIC) */
    fm_attrType attrType;

    /* Offset of the variable in the port attribute structure (fm_portAttr or
     * fmX000_portAttr) */
    fm_uint offset;

    /* chip specific bitmasks indicating which physical port types an
       attribute is not applicable to */
    fm_uint32 excludedPhyPortTypes;

    /* Attribute's default value. It is used to initialize the LAG logical port
     * port attribute structures (fm_portAttr and fmX000_portAttr) when a LAG
     * is created */
    fm_uint32 defValue;

} fm_portAttrEntry;


/**************************************************
 * Generic Port Attributes Structure 
 *  
 * A variable is present for each FM_PORT_XXX 
 * attributes defined in _fm_portAttr that are 
 * common to almost all chip types. A lot of them 
 * don't apply to FM2000 but are kept here to 
 * minimize duplication as they are used by the 
 * FM4000 and FM6000. 
 *  
 * The following convention is used to name each
 * of the attributes: the 'FM_PORT' prefix is 
 * removed and all the "_" are replaced by the 
 * letter that follow it in upper case. 
 * Ex:
 * FM_PORT_DEF_VLAN        --->  defVlan
 * FM_PORT_UCAST_FLOODING  --->  ucastFlooding 
 *  
 * For clarity few attributes don't follow that 
 * convention.
 **************************************************/
typedef struct _fm_portAttribute
{
    fm_uint32      autoNegMode;
    fm_uint64      autoNegBasePage;
    fm_uint64      autoNegPartnerBasePage;
    fm_anNextPages autoNegNextPages;
    fm_anNextPages autoNegPartnerNextPages;
    fm_int         bcastFlooding;
    fm_bool        bcastPruning;
    fm_bool        enableTxCutThrough;
    fm_bool        enableRxCutThrough;
    fm_uint32      defCfi;
    fm_uint32      defDscp;
    fm_uint32      defVlanPri;
    fm_uint32      defVlanPri2;
    fm_uint32      defSwpri;
    fm_uint32      defIslUser;
    fm_uint32      defVlan;
    fm_uint32      defVlan2;
    fm_bool        dicEnable;
    fm_dot1xState  dot1xState;
    fm_bool        dropBv;
    fm_bool        dropManagementIsl;
    fm_bool        dropTagged;
    fm_bool        dropUntagged;
    fm_uint32      ifg;
    fm_bool        ignoreIfgErrors;
    fm_bool        internalPort;
    fm_uint32      islTagFormat;
    fm_bool        learning;
    fm_bool        linkInterruptEnabled;
    fm_bool        loopbackSuppression;
    fm_bool        losslessPause;
    fm_macaddr     macAddr;
    fm_portmask    portMask;
    fm_int         maxFrameSize;
    fm_int         mcastFlooding;
    fm_bool        mcastPruning;
    fm_int         minFrameSize;
    fm_uint32      parser;
    fm_uint32      parserFlagOptions;
    fm_bool        replaceDscp;
    fm_bool        routable;
    fm_uint32      rxClassPause;
    fm_uint32      txClassPause;
    fm_bool        rxPause;
    fm_uint32      rxPauseMode;
    fm_uint32      saf;
    fm_uint32      security;
    fm_uint32      securityTrap;
    fm_uint32      smpLosslessPause;
    fm_uint32      speed;
    fm_bool        swpriDscpPref;
    fm_uint32      swpriSource;
    fm_uint32      tagging;
    fm_uint32      tagging2;
    fm_uint32      txPause;
    fm_uint32      txPauseMode;
    fm_uint32      txPauseResendTime;
    fm_uint32      txCfi;
    fm_uint32      txCfi2;
    fm_bool        txVpri;
    fm_bool        txVpri2;
    fm_int         ucastFlooding;
    fm_bool        ucastPruning;
    fm_bool        updateDscp;
    fm_bool        updateRoutedFrame;
    fm_bool        updateTtl;
    fm_uint32      txClkCompensation;
    fm_bool        discardFrameErrors;
    fm_uint32      txDrainMode;
    fm_uint32      rxDrainMode;
    fm_int         serdesLoopback;
    fm_bool        autoDetectModule;

    /* NOTE: These are moved to fm_laneAttr for FM10000 */
    fm_int         driveStrength;
    fm_int         emphasis;
    fm_uint32      txLaneOrdering;
    fm_uint32      txLanePolarity;
    fm_uint32      rxLaneOrdering;
    fm_uint32      rxLanePolarity;

} fm_portAttr;



/**************************************************
 * Generic Lane Attributes Structure 
 *  
 * This is a new structure to support FM10000, such
 * that when changing ethernet mode, the attributes
 * are not lost due to port numbering being changed.
 * These attributes are moved from fm_portAttr.
 * The index to this structure is chip family specific.
 *
 **************************************************/
typedef struct _fm_laneAttr
{
    fm_int         preCursor;
    fm_int         cursor;
    fm_int         postCursor;
    fm_int         initializePreCursor;
    fm_int         initializeCursor;
    fm_int         initializePostCursor;
    fm_bool        txLaneEnableConfigKrInit;
    fm_int         preCursorDecOnPreset;
    fm_int         postCursorDecOnPreset;
    fm_uint32      rxPolarity;
    fm_uint32      txPolarity;
    fm_rxTermination rxTermination;
    fm_dfeMode     dfeMode;
    /* This field make mapping of pep to serdes more efficient */
    fm_int         pepOffset;

} fm_laneAttr;



/* Used by the port debounce logic to track the AN state of a port */
typedef enum _fm_autoNegState
{
    FM_PORT_AUTONEG_IDLE = 0,
    FM_PORT_AUTONEG_IN_PROGRESS,
    FM_PORT_AUTONEG_FAIL,
    FM_PORT_AUTONEG_PASS,
    FM_PORT_AUTONEG_IN_REMOTE_FAULT

} fm_autoNegState;


// This is a hack for CP.
typedef enum
{
    FM_PHY_TYPE_NOT_PRESENT = 0,
    FM_PHY_TYPE_BASE_CX4,
    FM_PHY_TYPE_MIX_CX4,
    FM_PHY_TYPE_MIX_SFP,
    FM_PHY_TYPE_MIX_XFP,
    FM_PHY_TYPE_MIX_X2,
    FM_PHY_TYPE_MIX_BASET,
    FM_PHY_TYPE_MIX_SMB,
    FM_PHY_TYPE_AIM_XFP,
    FM_PHY_TYPE_AMCC_SFPP,
    FM_PHY_TYPE_MEZZ_CX4,
    FM_PHY_TYPE_LUXTERA,
    FM_PHY_TYPE_QUELLAN_CX4,
    FM_PHY_TYPE_AEL_SFPP,
    FM_PHY_TYPE_TERANETICS_BASET,
    FM_PHY_TYPE_TERANETICS_SFPP,
    FM_PHY_TYPE_NLP1342_QSFP

} fm_phy_type;


typedef enum
{
    FM_PHY_STATUS_DISABLED = 0,
    FM_PHY_STATUS_DISABLING,
    FM_PHY_STATUS_UNKNOWN,
    FM_PHY_STATUS_LINK_DOWN,
    FM_PHY_STATUS_LINK_UP

} fm_phy_status;


/**************************************************
 * Phy Interface table
 **************************************************/
typedef struct
{
    /* phy type */
    fm_phy_type   phyType;

    /* board id */
    fm_int        boardId;

    /* phy status flag */
    fm_phy_status phyStatus;

    /* phy link speed */
    fm_int        phySpeed;

    /* phy link duplex */
    fm_bool       phyDuplex;

    /* NOTE: The mac argument is for platforms with multiple MACs per port */
    /* phy enable function pointer */
    fm_status (*phyEnable)(fm_int   switchNum,
                           fm_int   physPort,
                           fm_int   mac,
                           fm_port *pPort);

    /* phy disable function pointer */
    fm_status (*phyDisable)(fm_int   switchNum,
                            fm_int   physPort,
                            fm_int   mac,
                            fm_port *pPort);

    fm_status (*phyAutoNegEnable)(fm_int switchNum,
                                  fm_int physPort,
                                  fm_int mac,
                                  fm_port *pPort);

    fm_status (*phyAutoNegDisable)(fm_int switchNum,
                                   fm_int physPort,
                                   fm_int mac,
                                   fm_port *pPort);

    fm_status (*phySetPortSpeed)(fm_int switchNum,
                                 fm_int physPort,
                                 fm_int mac,
                                 fm_port *pPort);

    fm_status (*phyOutputEnable)(fm_int switchNum,
                                 fm_int physPort,
                                 fm_int mac,
                                 fm_port *pPort,
                                 fm_bool enable);

    void (*phyPoll)(fm_switch *pSwitch,
                    fm_port *  pPort,
                    fm_int     switchNum,
                    fm_int     port,
                    fm_int     logPort,
                    fm_int     mac);

    /* FM6000 Only: The platform can hook to this function to set any MAC  
       or SERDES attribute that is specific to each ethernet mode */
    fm_status (*notifyEthModeChange)(fm_int switchNum,
                                     fm_int port,
                                     fm_int mac,
                                     fm_ethMode mode);

} fmPhyInterfaceTable;


/**************************************************
 * Generic Port Structure used for ALL port types
 **************************************************/
struct _fm_port
{
    /**************************************************
     * Port Identification and Capabilities
     **************************************************/

    /* switch number, kept here for cross-referencing purposes */
    fm_int                 switchNumber;

    /* port number */
    /* NOTE: For LAG member glort port, the port number
     * is changed to the physical port or remote port
     * So when the function to find the port from glort
     * It can return the correct logical member port
     */
    fm_int                 portNumber;

    /* port family */
    fm_portFamily          portFamily;

    /* port type */
    fm_portType            portType;

    /* port capabilities bit field */
    fm_uint                capabilities;

    /* pointer to the switch definition table */
    fm_switch *            switchPtr;

    /* pointer to additional port information specific to the port type */
    void *                 extension;

    /* switch aggregate port number for this port, if its switch is in a
     * switch aggregate. -1 if not in a switch aggregate. */
    fm_int                 swagPort;

    /* switch aggregate link type for this port.  Ignored if not in a
     * switch aggregate. */
    fm_swagLinkType        swagLinkType;

    /* The index of this port in the cardinal port map.
     * Also used to index logical port bit masks and bit arrays.
     * Will be >= 0 if this is a cardinal port (a logical port that 
     * represents one of the switch's physical ports). 
     * Will be -1 if this is not a cardinal port. */
    fm_int                 portIndex;

    /* Physical port number if this is a cardinal port. */
    fm_int                 physicalPort;

    /**************************************************
     * Glort table management fields. (FM4000/FM6000/FM10000)
     **************************************************/

    /* Points to the data structure for the glort table entry. */
    fm_glortCamEntry *      camEntry;

    /* Points to the data structure for the destination table entry. */
    fm_glortDestEntry *     destEntry;

    /* Number of destination table entries assigned to this port.
     * Normally, numDestEntries is the same as camEntry->destCount.
     * However, to avoid dest table fragmentation, numDestEntries
     * could be greater than camEntry->destCount, such as in LAG. 
     * There will be some inefficiency of dest entries not being used,
     * but that is the compromise. */
    fm_int                  numDestEntries;

    /* Global resource tag for this port. */
    fm_uint32               glort;

    /* Number of logical ports to delete when port is freed. */
    fm_int                  freeCount;

    /**************************************************
     * Current Configuration
     **************************************************/

    /* port state */
    fm_int                 mode;
    fm_int                 submode;

    /* current link state whether up or down */
    fm_bool                linkUp;

    /* The index in the switch LAG table (fm_lagInfo.lag) of the 
     * link aggregation group to which this port belongs.
     * Will be >= 0 if this is a LAG port or a LAG member port.
     * Will be -1 if the port is not associated with a LAG. */
    fm_int                 lagIndex;

    /* The index of this port in the member table (fm_lag.memberPorts) 
     * of the link aggregation group to which this port belongs.
     * Will be >= 0 if the port is a LAG member port.
     * Will be -1 if it is not a LAG member port. */
    fm_int                 memberIndex;

    /* phy interface table */
    fmPhyInterfaceTable    phyInfo;

    /* link interrupt pending flag */
    fm_bool                linkInterruptPending;

    /* link state change pending flag */
    fm_bool                linkStateChangePending;

    /* pending link state value */
    fm_uint32              pendingLinkStateValue;

    /* timestamp after which pending link state should be accepted */
    fm_timestamp           linkStateChangeExpiration;

    /* Indicates whether port security was activated via fmSetPortSecurity */
    fm_uint32              portSecurityEnabled;

    /* saved copies of register values that need to be temporarily changed
     *   during link fault conditions, so that these values can be restored
     *   when the fault is removed */
    fm_uint32              globalPauseWm;
    fm_uint32              txSharedWm;

    /* list of all multicast groups to which this port is listening */
    fm_tree                mcastGroupList;

    /* Indicate if the port must be forced up in the port mask */

    fm_bool                isPortForceUp;

    /* Structure used to cache all port attributes */
    fm_portAttr            attributes;

    /* Entry in the MA purge list that records pending purges for this 
     * logical port.  This is created when needed, and not freed until 
     * (or after) the logical port is destroyed.  See fm_switch for 
     * further info. */
    fm_maPurgeListEntry *  maPurgeListEntry;

    /* lbgHandle points to lbgHandle which is associated with the 
     * LBG logical port. */
    fm_int                 lbgHandle;

    /**************************************************
     * Port Support Function Pointers
     **************************************************/

    /* Port Initialization */
    fm_status              (*InitPort)(fm_int sw, fm_port *portPtr);

    /* Port State */
    fm_status              (*SetPortState)(fm_int sw,
                                           fm_int              port,
                                           fm_int              mac,
                                           fm_int              mode,
                                           fm_int              subMode);
    fm_status              (*GetPortState)(fm_int  sw,
                                           fm_int  port,
                                           fm_int  mac,
                                           fm_int  numBuffers,
                                           fm_int *numLanes,
                                           fm_int *mode,
                                           fm_int *state,
                                           fm_int *info);
    fm_status              (*SetFaultState)(fm_int sw,
                                            fm_int              port,
                                            fm_bool             enable);

    /* Port Attributes */
    fm_status              (*SetPortAttribute)(fm_int sw,
                                               fm_int              port,
                                               fm_int              mac,
                                               fm_int              lane,
                                               fm_int              attribute,
                                               void *              value);
    fm_status              (*GetPortAttribute)(fm_int sw,
                                               fm_int              port,
                                               fm_int              mac,
                                               fm_int              lane,
                                               fm_int              attribute,
                                               void *              value);

    fm_status              (*UpdatePortMask)(fm_int sw, fm_int port);

    /* General notification of link events */
    fm_status              (*NotifyLinkEvent)(fm_int sw, fm_int port);

    /* QOS Support */
    fm_status              (*SetPortQOS)(fm_int sw,
                                         fm_int              port,
                                         fm_int              attr,
                                         fm_int              index,
                                         void *              value);
    fm_status              (*GetPortQOS)(fm_int sw,
                                         fm_int              port,
                                         fm_int              attr,
                                         fm_int              index,
                                         void *              value);

    /* Statistics Support */
    fm_status              (*GetPortCounters)(fm_int sw,
                                              fm_int              port,
                                              fm_portCounters *counters);
    fm_status              (*ResetPortCounters)(fm_int sw,
                                                fm_int port);

    /* Vlan Support */
    fm_status              (*SetVlanMembership)(fm_int sw,
                                                fm_vlanEntry *entry,
                                                fm_int              port,
                                                fm_bool             state);
    fm_status              (*SetVlanTag)(fm_int        sw,
                                         fm_vlanSelect vlanSel,
                                         fm_vlanEntry *entry,
                                         fm_int        port,
                                         fm_bool       tag);
    fm_status              (*GetVlanMembership)(fm_int        sw,
                                                fm_vlanEntry *entry,
                                                fm_int        port,
                                                fm_bool *     state);
    fm_status              (*GetVlanTag)(fm_int        sw,
                                         fm_vlanSelect vlanSel,
                                         fm_vlanEntry *entry,
                                         fm_int        port,
                                         fm_bool       *tag);

    /* Debug Support */
    fm_status              (*DbgDumpPort)(fm_int sw, fm_int port);

    /* Transceiver state notification */
    fm_status              (*NotifyXcvrState)( fm_int     sw,
                                               fm_int     port,
                                               fm_int     mac,
                                               fm_int     lane,
                                               fm_uint32  xcvrSignals,
                                               void      *xcvrInfo );
    
    fm_status              (*SetPortSecurity)(fm_int  sw,
                                              fm_int  port,
                                              fm_bool enable,
                                              fm_bool strict);


    fm_status              (*isPciePort)( fm_int   sw, 
                                          fm_int   port, 
                                          fm_bool *isPcie );

    fm_status              (*isSpecialPort)( fm_int   sw, 
                                             fm_int   port, 
                                             fm_bool *isSpecialPort );

    fm_status              (*GetNumPortLanes)( fm_int sw, 
                                               fm_int port,
                                               fm_int mac,
                                               fm_int *numLanes );

    fm_status              (*IsPortDisabled)( fm_int   sw, 
                                              fm_int   port,
                                              fm_int   mac,
                                              fm_bool *isDisabled );


};  /* end struct _fm_port */


/**************************************************
 * Generic Lane Structure
 **************************************************/
struct _fm_lane
{
    /* Structure used to cache all lane attributes */
    fm_laneAttr            attributes;

    /* pointer to additional chip-specific lane information */
    void                  *extension;

};   /* end struct _fm_lane */


fm_status fmInitPort(fm_int sw, fm_port *port);
fm_status fmSetPortAttributeInternal(fm_int sw,
                                     fm_int port,
                                     fm_int attr,
                                     void * value);
fm_status fmUpdateSwitchPortMasks(fm_int sw);
fm_status fmSetFaultState(fm_int  sw,
                          fm_int  port,
                          fm_bool enable);
fm_status fmCheckFaultStates(fm_int sw);

void fmPrintPortAttributeValues(fm_int  attrType,
                                fm_char *attrName,
                                fm_bool perLag,
                                void *  cachedValue,
                                void *  value,
                                void *  lagValue);

fm_int fmComparePortAttributes(fm_int attrType, void *attr1, void *attr2);

fm_int fmGetCpuPortInt(fm_int sw, fm_int *cpuPort);

fm_bool fmIsValidPortAttribute(fm_uint attr);

#endif /* __FM_FM_API_PORT_INT_H */
