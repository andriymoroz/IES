/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_attr.h
 * Creation Date:   June 2, 2014
 * Description:     Platform specific attributes
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

#ifndef __FM_PLATFORM_ATTR_H
#define __FM_PLATFORM_ATTR_H


/* For application to distinguish between old and new implementations. */
#define FM_PLATFORM_ATTR_NOT_USED


/************************************************************************
 ****                                                                ****
 ****               BEGIN DOCUMENTED ATTRIBUTES                      ****
 ****                                                                ****
 ************************************************************************/

/** \ingroup freedomConfig
 * @{ */

/**
 * (Required) Number of switches in the system. This number must include any 
 * virtual switch (SWAG).
 */
#define FM_AAK_API_PLATFORM_NUM_SWITCHES                    "api.platform.config.numSwitches"
#define FM_AAT_API_PLATFORM_NUM_SWITCHES                    FM_API_ATTR_INT


/**
 * (Optional) Customer-defined name for the platform. Value is a text 
 * string, up to 31 characters in length. 
 */
#define FM_AAK_API_PLATFORM_NAME                            "api.platform.config.platformName"
#define FM_AAT_API_PLATFORM_NAME                            FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_NAME                            "libertyTrail"


/**
 * (Optional) Specify the file name used for I2C bus locking.
 *                                                                      \lb\lb
 * For cases where the Liberty Trail platform code and the 
 * board level management software (i.e. board-manager) share the same 
 * I2C bus then a file locking mechanism must be used to prevent simultaneous 
 * accesses. 
 *                                                                      \lb\lb
 * The suggested file name for this property is "/tmp/.fm_lock.lck"
 *                                                                      \lb\lb
 * By default no file locking mechanism is used.
 */
#define FM_AAK_API_PLATFORM_FILE_LOCK_NAME               "api.platform.config.fileLockName"
#define FM_AAT_API_PLATFORM_FILE_LOCK_NAME               FM_API_ATTR_TEXT


/**
 * (Optional) Switch number for the specified switch index. 
 *                                                                      \lb\lb
 * The "switch index" is the value that the API uses as its switch
 * number (the sw parameter).
 *                                                                      \lb\lb
 * The "switch number" is the value that will be passed down to the
 * shared library interfaces, as well as to the kernel module driver
 * interfaces.
 *                                                                      \lb\lb
 * If not specified, the switch number will be the same as the switch 
 * index. 
 */
#define FM_AAK_API_PLATFORM_SW_NUM                         "api.platform.config.switch.%d.switchNumber"
#define FM_AAT_API_PLATFORM_SW_NUM                          FM_API_ATTR_INT


/**
 * (Required) Number of ports on the given switch, including the CPU 
 * port. 
 */ 
#define FM_AAK_API_PLATFORM_SW_NUMPORTS                     "api.platform.config.switch.%d.numPorts"
#define FM_AAT_API_PLATFORM_SW_NUMPORTS                     FM_API_ATTR_INT


/**
 * (Optional) LED management polling period, in milliseconds.
 *                                                                      \lb\lb
 * LED management thread is required for LED blinking. If LED blinking is 
 * not required then set this property to 0 to disable it.
 *                                                                      \lb\lb
 * See api.platform.config.switch.%d.ledBlinkMode for details.
 *                                                                      \lb\lb
 * Default is set to 500 msec 
 */ 
#define FM_AAK_API_PLATFORM_LED_POLL_MSEC                   "api.platform.config.switch.%d.ledPollPeriodMsec"
#define FM_AAT_API_PLATFORM_LED_POLL_MSEC                   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_LED_POLL_MSEC                   500

/**
 * (Optional) Specify the port LED blinking mode of operation used by the 
 * LED management thread.
 *                                                                      \lb\lb
 * Applies only if api.platform.config.switch.%d.ledPollPeriodMsec is set 
 * different than 0.
 *                                                                      \lb\lb
 * The supported modes are:
 *                                                                      \lb\lb
 * SW_CONTROL  - Must be used when the LEDs are connected to devices like 
 *               PCA IO expander. In this mode the software will periodically
 *               turn ON and OFF the LED on presence of TX/RX traffic. 
 *                                                                      \lb\lb
 * NO_BLINK    - In this mode the software turns ON the LED on presence of 
 *               TX/RX traffic, otherwise it turns it OFF. This mode is
 *               intended to be used when the LEDs are connected to a PCA IO
 *               expander and blinking is not required.
 *                                                                      \lb\lb
 * HW_ASSISTED - Must be used when the LEDs are connected to I2C LED driver 
 *               that provides blink capability, like the PCA9634/35.
 *                                                                      \lb\lb
 * Default is set to NO_BLINK.
 */
#define FM_AAK_API_PLATFORM_LED_BLINK_MODE               "api.platform.config.switch.%d.ledBlinkMode"
#define FM_AAT_API_PLATFORM_LED_BLINK_MODE               FM_API_ATTR_TEXT

/**
 * (Optional) Transceiver management polling period, in milliseconds.
 *                                                                      \lb\lb
 * Set to 0 to disable. 
 *                                                                      \lb\lb
 * Default is set to 1000 msec 
 */ 
#define FM_AAK_API_PLATFORM_XCVR_POLL_MSEC                  "api.platform.config.switch.%d.xcvrPollPeriodMsec"
#define FM_AAT_API_PLATFORM_XCVR_POLL_MSEC                  FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_XCVR_POLL_MSEC                  1000


/** 
 * (Required) Port mapping between a logical port and an FM10000
 * physical port.
 *                                                                      \lb\lb
 * The physical port represents either an EPL/LANE, a PCIE port,
 * a Tunneling Engine port or a Loopback port.
 *                                                                      \lb\lb
 * For example: 
 *                                                                      \lb\lb
 * switch.%d.portIndex.%d.portMapping text "LOG=0 PCIE=8" 
 * switch.%d.portIndex.%d.portMapping text "LOG=1 EPL=0 LANE=0" 
 * switch.%d.portIndex.%d.portMapping text "LOG=2 EPL=0 LANE=1" 
 * switch.%d.portIndex.%d.portMapping text "LOG=3 EPL=0 LANE=2" 
 * switch.%d.portIndex.%d.portMapping text "LOG=4 EPL=0 LANE=3" 
 * switch.%d.portIndex.%d.portMapping text "LOG=10 LPBK=0" 
 * switch.%d.portIndex.%d.portMapping text "LOG=11 LPBK=1" 
 * switch.%d.portIndex.%d.portMapping text "LOG=20 TE=0" 
 * switch.%d.portIndex.%d.portMapping text "LOG=21 TE=1" 
 * switch.%d.portIndex.%d.portMapping text "LOG=22 FIBM" 
 *                                                                      \lb\lb
 * The quote " " are required. 
 *                                                                      \lb\lb
 * The valid range for each port type are: 
 * EPL : 0-8 
 * LANE: 0-3 
 * PCIE: 0-8 
 * LPBK: 0-1 
 * TE: 0-1 
 * FIBM: no index required 
 *                                                                      \lb\lb
 * SWAG Port mapping is also required if the platform abstract multiple switch 
 * to a single one using SWAG. In that case, the Port mapping between the 
 * virtual SWAG switch and each physical switch must be defined. The SWAG 
 * switch always refer to switch 0 while physical switch are 
 * 1..("api.platform.config.numSwitches" - 1)
 *                                                                      \lb\lb
 * For example: 
 *                                                                      \lb\lb
 * switch.%d.portIndex.%d.portMapping text "SWAG=0 SW=2 LOG=0" 
 * switch.%d.portIndex.%d.portMapping text "SWAG=1 SW=1 LOG=1" 
 * switch.%d.portIndex.%d.portMapping text "SWAG=2 SW=1 LOG=2" 
 */
#define FM_AAK_API_PLATFORM_PORT_MAPPING                    "api.platform.config.switch.%d.portIndex.%d.portMapping"
#define FM_AAT_API_PLATFORM_PORT_MAPPING                    FM_API_ATTR_TEXT


/** 
 * (Optional) QSFP port to EPL/LANE port mapping. 
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when
 * the api.platform.config.switch.%d.portIndex.%d.portMapping 
 * property format is not used. QSFP refers to both 40G and 
 * 100G ports. 
 *                                                                      \lb\lb
 * This port mapping format must be used when the lane ordering 
 * is not one-to-one between the QSFP lanes and the EPL lanes.
 * When this format is used then the four lanes must be defined. 
 *                                                                      \lb\lb
 * For example: 
 *                                                                      \lb\lb
 * portIndex.1.lane.0.portMapping text "LOG=1 EPL=0 LANE=3 
 * portIndex.1.lane.1.portMapping text "LOG=1 EPL=0 LANE=2" 
 * portIndex.1.lane.2.portMapping text "LOG=1 EPL=0 LANE=1" 
 * portIndex.1.lane.3.portMapping text "LOG=1 EPL=0 LANE=0" 
 *                                                                      \lb\lb
 * The quote " " are required. 
 *                                                                      \lb\lb
 * The valid range are: 
 * EPL : 0-8 
 * LANE: 0-3
 */
#define FM_AAK_API_PLATFORM_LANE_PORT_MAPPING               "api.platform.config.switch.%d.portIndex.%d.lane.%d.portMapping"
#define FM_AAT_API_PLATFORM_LANE_PORT_MAPPING               FM_API_ATTR_TEXT


/** 
 * (Optional) SWAG Internal Link port mapping.
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when
 * the platform is managed by SWAG. 
 *                                                                      \lb\lb
 * This mapping defines an internal connection between two FM10000 
 * switches. 
 *                                                                      \lb\lb
 * For example: 
 *                                                                      \lb\lb
 * internalPortIndex.0.portMapping text "SWAG=13 SWAG=14"
 * internalPortIndex.1.portMapping text "SWAG=15 SWAG=16"
 * internalPortIndex.2.portMapping text "SWAG=17 SWAG=18"
 * internalPortIndex.3.portMapping text "SWAG=19 SWAG=20"
 */
#define FM_AAK_API_PLATFORM_INTERNAL_PORT_MAPPING           "api.platform.config.switch.%d.internalPortIndex.%d.portMapping"
#define FM_AAT_API_PLATFORM_INTERNAL_PORT_MAPPING           FM_API_ATTR_TEXT


/** 
 * (Optional) SWAG Topology.
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when
 * the platform is managed by SWAG. 
 *                                                                      \lb\lb
 * Supported topologies are: FAT_TREE and MESH
 */
#define FM_AAK_API_PLATFORM_TOPOLOGY                        "api.platform.config.switch.topology"
#define FM_AAT_API_PLATFORM_TOPOLOGY                        FM_API_ATTR_TEXT


/** 
 * (Optional) SWAG Role.
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when
 * the platform is managed by SWAG. 
 *                                                                      \lb\lb
 * Role of each switch in a SWAG topology must be defined.
 *                                                                      \lb\lb
 * Supported roles are: SWAG, LEAF, SPINE and SPINE_LEAF
 */
#define FM_AAK_API_PLATFORM_ROLE                            "api.platform.config.switch.%d.role"
#define FM_AAT_API_PLATFORM_ROLE                            FM_API_ATTR_TEXT


/**
 * (Optional) Default HW resource ID (32-bit value).
 *                                                                      \lb\lb
 * If not specified, the default resource identifier for the port will
 * be set to -1.
 *                                                                      \lb\lb
 * See portIndex.%%d.hwResourceId property for details.
 */
#define FM_AAK_API_PLATFORM_HW_RESOURCE_ID_DEFAULT          "api.platform.config.default.hwResourceId"
#define FM_AAT_API_PLATFORM_HW_RESOURCE_ID_DEFAULT           FM_API_ATTR_INT


/**
 * (Optional) Default HW resource ID (32-bit value). Applies to all ports on
 * the specified switch.
 *                                                                      \lb\lb
 * If not specified, the default resource identifier for the port will
 * be set to default.hwResourceId.
 *                                                                      \lb\lb
 * See portIndex.%%d.hwResourceId property for details.
 *
 * This attribute is DEPRECATED. Use portIndex.%d.hwResourceId instead.
 */
#define FM_AAK_API_PLATFORM_PORT_HW_RESOURCE_ID_DEFAULT     "api.platform.config.switch.%d.port.default.hwResourceId"
#define FM_AAT_API_PLATFORM_PORT_HW_RESOURCE_ID_DEFAULT     FM_API_ATTR_INT


/**
 * Unique 32-bit value associated with a port for the shared library.
 *                                                                      \lb\lb
 * This ID is used by the platform shared library to select the hardware
 * resource for a given port. The platform shared library can use this 
 * field in any way it sees fit. For example, the platform shared 
 * library can encode this field as (busNo << 16) | (pcaIdx << 8) | (idx). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.hwResourceId property 
 * for this switch. If not specified, the resource identifier for the port 
 * will be set to -1.
 */
#define FM_AAK_API_PLATFORM_PORT_HW_RESOURCE_ID             "api.platform.config.switch.%d.portIndex.%d.hwResourceId"
#define FM_AAT_API_PLATFORM_PORT_HW_RESOURCE_ID             FM_API_ATTR_INT


/**
 * (Optional) Default Ethernet mode. Applies to all ports on the 
 * specified switch. 
 *                                                                      \lb\lb
 * See the portIndex.%%d.ethernetMode property for a list of supported 
 * values. 
 *                                                                      \lb\lb
 * The default value is DISABLED.
 */
#define FM_AAK_API_PLATFORM_PORT_ETHMODE_DEFAULT            "api.platform.config.switch.%d.port.default.ethernetMode"
#define FM_AAT_API_PLATFORM_PORT_ETHMODE_DEFAULT            FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PORT_ETHMODE_DEFAULT            "DISABLED"


/**
 * (Optional) Ethernet mode for the given switch port.
 *                                                  \lb\lb
 * Value is a string specifying one of the following:
 *                                                  \lb\lb
 * DISABLED - ''FM_ETH_MODE_DISABLED''
 *                                                  \lb
 * 1000Base-X - ''FM_ETH_MODE_1000BASE_X''
 *                                                  \lb
 * 10GBase-CR - ''FM_ETH_MODE_10GBASE_CR''
 *                                                  \lb
 * 10GBase-CX4 - ''FM_ETH_MODE_10GBASE_CX4''
 *                                                  \lb
 * 10GBase-SR - ''FM_ETH_MODE_10GBASE_SR''
 *                                                  \lb
 * 24GBase-CR4 - ''FM_ETH_MODE_24GBASE_CR4''
 *                                                  \lb
 * 2500Base-X - ''FM_ETH_MODE_2500BASE_X''
 *                                                  \lb
 * 25GBase-SR - ''FM_ETH_MODE_25GBASE_SR''
 *                                                  \lb
 * 40GBase-SR4 - ''FM_ETH_MODE_40GBASE_SR4''
 *                                                  \lb
 * 100GBase-SR4 - ''FM_ETH_MODE_100GBASE_SR4''
 *                                                  \lb
 * AN-73 - ''FM_ETH_MODE_AN_73''
 *                                                  \lb
 * SGMII - ''FM_ETH_MODE_SGMII''
 *                                                  \lb
 * AUTODETECT - Ethernet mode will be automatically configured based on the 
 *              transceiver type. This mode applies to SFP or QSFP port only.
 *              See "api.platform.config.switch.%d.portIndex.%d.interfaceType"
 *                                                  \lb\lb
 * The default value is specified by the port.default.ethernet property 
 * for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_ETHMODE                    "api.platform.config.switch.%d.portIndex.%d.ethernetMode"
#define FM_AAT_API_PLATFORM_PORT_ETHMODE                    FM_API_ATTR_TEXT


/**
 * (Optional) Maximum speed at which the port will be operated 
 * in Mbps. 
 *  
 * This parameter controls the amount of bandwidth allocated to
 * the given port. 
 *  
 * This property should be set equal or greater than the 
 * default port ethernet mode (portIndex.%%d.ethernetMode 
 * property) 
 *  
 * Supported speeds are: 2500, 10000, 25000, 40000, 60000 and 
 * 100000. Note that 50000 is rounded down to 40000. Any other 
 * value will be rounded up to the nearest supported speed. 
 *                                                                      \lb\lb
 * Default is automatically selected per port type
 *                                                                      \lb\lb
 * EPL      -> 2500
 * PCIE_X1  -> 10000
 * PCIE_X4  -> 50000
 * PCIE_X8  -> 50000
 * TE       -> 60000
 * LOOPBACK -> 25000 
 * FIBM     -> 2500 
 */ 
#define FM_AAK_API_PLATFORM_HW_PORT_SPEED                   "api.platform.config.switch.%d.portIndex.%d.speed"
#define FM_AAT_API_PLATFORM_HW_PORT_SPEED                   FM_API_ATTR_INT


/**
 * (Optional) Lane termination for the given switch port.
 *                                                  \lb\lb
 * Value is a string specifying one of the following: 
 *                                                  \lb\lb
 * TERM_HIGH - ''FM_PORT_TERMINATION_HIGH''
 *                                                  \lb
 * TERM_LOW - ''FM_PORT_TERMINATION_LOW''
 *                                                  \lb
 * TERM_FLOAT - ''FM_PORT_TERMINATION_FLOAT''
 *                                                  \lb
 * The default value is specified by the API based on port type. 
 */
#define FM_AAK_API_PLATFORM_PORT_TERMINATION                "api.platform.config.switch.%d.portIndex.%d.rxTermination"
#define FM_AAT_API_PLATFORM_PORT_TERMINATION                FM_API_ATTR_TEXT

/**
 * (Optional) Lane polarity for the given switch port and lane.
 *                                                  \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                  \lb\lb
 * Value is a string specifying one of the following: 
 *                                                  \lb\lb
 * TERM_HIGH - ''FM_PORT_TERMINATION_HIGH''
 *                                                  \lb
 * TERM_LOW - ''FM_PORT_TERMINATION_LOW''
 *                                                  \lb
 * TERM_FLOAT - ''FM_PORT_TERMINATION_FLOAT''
 *                                                  \lb
 * The default value is specified by the API based on port type. 
 */
#define FM_AAK_API_PLATFORM_PORT_PER_LANE_TERMINATION             "api.platform.config.switch.%d.portIndex.%d.lane.%d.rxTermination"
#define FM_AAT_API_PLATFORM_PORT_PER_LANE_TERMINATION             FM_API_ATTR_TEXT


/**
 * (Optional) Default lane polarity value. Applies to all ports on the 
 * specified switch. 
 *                                                                      \lb\lb
 * See the portIndex.%%d.lanePolarity property for a list of supported 
 * values. 
 *                                                                      \lb\lb
 * The default value is INVERT_NONE.
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_POLARITY_DEFAULT      "api.platform.config.switch.%d.port.default.lanePolarity"
#define FM_AAT_API_PLATFORM_PORT_LANE_POLARITY_DEFAULT      FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PORT_LANE_POLARITY_DEFAULT      "INVERT_NONE"


/**
 * (Optional) Lane polarity for the given switch port.
 *                                                  \lb\lb
 * Value is a string specifying one of the following: 
 *                                                  \lb\lb
 * INVERT_NONE - ''FM_POLARITY_INVERT_NONE''
 *                                                  \lb
 * INVERT_RX - ''FM_POLARITY_INVERT_RX''
 *                                                  \lb
 * INVERT_TX - ''FM_POLARITY_INVERT_TX''
 *                                                  \lb
 * INVERT_RX_TX - ''FM_POLARITY_INVERT_RX_TX''
 *                                                  \lb\lb
 * The default value is specified by the port.default.lanePolarity 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_POLARITY              "api.platform.config.switch.%d.portIndex.%d.lanePolarity"
#define FM_AAT_API_PLATFORM_PORT_LANE_POLARITY              FM_API_ATTR_TEXT


/**
 * (Optional) Lane polarity for the given switch port and lane.
 *                                                  \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                  \lb\lb
 * Value is a string specifying one of the following: 
 *                                                  \lb\lb
 * INVERT_NONE - ''FM_POLARITY_INVERT_NONE''
 *                                                  \lb
 * INVERT_RX - ''FM_POLARITY_INVERT_RX''
 *                                                  \lb
 * INVERT_TX - ''FM_POLARITY_INVERT_TX''
 *                                                  \lb
 * INVERT_RX_TX - ''FM_POLARITY_INVERT_RX_TX''
 *                                                  \lb\lb
 * The default value is specified by the port.default.lanePolarity 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_PER_LANE_POLARITY          "api.platform.config.switch.%d.portIndex.%d.lane.%d.lanePolarity"
#define FM_AAT_API_PLATFORM_PORT_PER_LANE_POLARITY          FM_API_ATTR_TEXT


/**
 * (Optional) Default SERDES preCursor for DA cables, for ports on the 
 * given switch. 
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_PRECURSOR'' port 
 * attribute. 
 *                                                                      \lb\lb
 * The default value is 0.
 */
#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_COPPER_DEFAULT   "api.platform.config.switch.%d.port.default.preCursorCopper"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_COPPER_DEFAULT   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PORT_PRECURSOR_COPPER_DEFAULT   0


/**
 * (Optional) SERDES preCursor for DA cables on the given switch port.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_PRECURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.preCursorCopper 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_1G_COPPER        "api.platform.config.switch.%d.portIndex.%d.preCursor1GCopper"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_1G_COPPER        FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_10G_COPPER       "api.platform.config.switch.%d.portIndex.%d.preCursor10GCopper"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_10G_COPPER       FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_25G_COPPER       "api.platform.config.switch.%d.portIndex.%d.preCursor25GCopper"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_25G_COPPER       FM_API_ATTR_INT


/**
 * (Optional) SERDES preCursor for DA cables on the given switch port and lane.
 *                                                                      \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_PRECURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.preCursorCopper 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_1G_COPPER   "api.platform.config.switch.%d.portIndex.%d.lane.%d.preCursor1GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_PRECURSOR_1G_COPPER   FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_10G_COPPER  "api.platform.config.switch.%d.portIndex.%d.lane.%d.preCursor10GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_PRECURSOR_10G_COPPER  FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_25G_COPPER  "api.platform.config.switch.%d.portIndex.%d.lane.%d.preCursor25GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_PRECURSOR_25G_COPPER  FM_API_ATTR_INT


/**
 * (Optional) Default SERDES preCursor for optical modules, for ports on the 
 * given switch. 
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_PRECURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The default value is 0.
 */
#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_OPTICAL_DEFAULT  "api.platform.config.switch.%d.port.default.preCursorOptical"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_OPTICAL_DEFAULT  FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PORT_PRECURSOR_OPTICAL_DEFAULT  0


/**
 * (Optional) SERDES preCursor for optical modules on the given switch port.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_PRECURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.preCursorOptical 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_1G_OPTICAL       "api.platform.config.switch.%d.portIndex.%d.preCursor1GOptical"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_1G_OPTICAL       FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_10G_OPTICAL      "api.platform.config.switch.%d.portIndex.%d.preCursor10GOptical"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_10G_OPTICAL      FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_PRECURSOR_25G_OPTICAL      "api.platform.config.switch.%d.portIndex.%d.preCursor25GOptical"
#define FM_AAT_API_PLATFORM_PORT_PRECURSOR_25G_OPTICAL      FM_API_ATTR_INT


/**
 * (Optional) SERDES preCursor for optical modules on the given switch 
 * port and lane.
 *                                                                      \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_PRECURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.preCursorOptical 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_1G_OPTICAL  "api.platform.config.switch.%d.portIndex.%d.lane.%d.preCursor1GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_PRECURSOR_1G_OPTICAL  FM_API_ATTR_INT                                     
                                                                                                                
#define FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_10G_OPTICAL "api.platform.config.switch.%d.portIndex.%d.lane.%d.preCursor10GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_PRECURSOR_10G_OPTICAL FM_API_ATTR_INT                                     
                                                                                                                
#define FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_25G_OPTICAL "api.platform.config.switch.%d.portIndex.%d.lane.%d.preCursor25GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_PRECURSOR_25G_OPTICAL FM_API_ATTR_INT


/**
 * (Optional) Default SERDES Cursor for DA cables, for ports on the 
 * given switch. 
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_CURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The default value is 0.
 */
#define FM_AAK_API_PLATFORM_PORT_CURSOR_COPPER_DEFAULT      "api.platform.config.switch.%d.port.default.cursorCopper"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_COPPER_DEFAULT      FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PORT_CURSOR_COPPER_DEFAULT      0


/**
 * (Optional) SERDES Cursor for DA cables on the given switch port.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_CURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.cursorCopper 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_CURSOR_1G_COPPER           "api.platform.config.switch.%d.portIndex.%d.cursor1GCopper"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_1G_COPPER           FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_CURSOR_10G_COPPER          "api.platform.config.switch.%d.portIndex.%d.cursor10GCopper"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_10G_COPPER          FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_CURSOR_25G_COPPER          "api.platform.config.switch.%d.portIndex.%d.cursor25GCopper"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_25G_COPPER          FM_API_ATTR_INT


/**
 * (Optional) SERDES Cursor for DA cables on the given switch port and lane.
 *                                                                      \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_CURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.cursorCopper 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_1G_COPPER      "api.platform.config.switch.%d.portIndex.%d.lane.%d.cursor1GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_CURSOR_1G_COPPER      FM_API_ATTR_INT                                     
                                                                                                                
#define FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_10G_COPPER     "api.platform.config.switch.%d.portIndex.%d.lane.%d.cursor10GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_CURSOR_10G_COPPER     FM_API_ATTR_INT                                     
                                                                                                                
#define FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_25G_COPPER     "api.platform.config.switch.%d.portIndex.%d.lane.%d.cursor25GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_CURSOR_25G_COPPER     FM_API_ATTR_INT


/**
 * (Optional) Default SERDES Cursor for optical modules, for ports on the 
 * given switch. 
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_CURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The default value is 0.
 */
#define FM_AAK_API_PLATFORM_PORT_CURSOR_OPTICAL_DEFAULT      "api.platform.config.switch.%d.port.default.cursorOptical"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_OPTICAL_DEFAULT      FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PORT_CURSOR_OPTICAL_DEFAULT      0


/**
 * (Optional) SERDES Cursor for optical modules on the given switch port.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_CURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.cursorOptical 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_CURSOR_1G_OPTICAL          "api.platform.config.switch.%d.portIndex.%d.cursor1GOptical"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_1G_OPTICAL          FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_CURSOR_10G_OPTICAL         "api.platform.config.switch.%d.portIndex.%d.cursor10GOptical"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_10G_OPTICAL         FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_CURSOR_25G_OPTICAL         "api.platform.config.switch.%d.portIndex.%d.cursor25GOptical"
#define FM_AAT_API_PLATFORM_PORT_CURSOR_25G_OPTICAL         FM_API_ATTR_INT


/**
 * (Optional) SERDES Cursor for optical modules on the given switch 
 * port and lane.
 *                                                                      \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_CURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.cursorOptical 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_1G_OPTICAL      "api.platform.config.switch.%d.portIndex.%d.lane.%d.cursor1GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_CURSOR_1G_OPTICAL      FM_API_ATTR_INT                                     
                                                                                                                 
#define FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_10G_OPTICAL     "api.platform.config.switch.%d.portIndex.%d.lane.%d.cursor10GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_CURSOR_10G_OPTICAL     FM_API_ATTR_INT                                     
                                                                                                                 
#define FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_25G_OPTICAL     "api.platform.config.switch.%d.portIndex.%d.lane.%d.cursor25GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_CURSOR_25G_OPTICAL     FM_API_ATTR_INT


/**
 * (Optional) Default SERDES postCursor for DA cables, for ports on the 
 * given switch. 
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_POSTCURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The default value is 15.
 */
#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_COPPER_DEFAULT  "api.platform.config.switch.%d.port.default.postCursorCopper"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_COPPER_DEFAULT  FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PORT_POSTCURSOR_COPPER_DEFAULT  15


/**
 * (Optional) SERDES postCursor for DA cables on the given switch port.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_POSTCURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.postCursorCopper 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_1G_COPPER       "api.platform.config.switch.%d.portIndex.%d.postCursor1GCopper"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_1G_COPPER       FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_10G_COPPER      "api.platform.config.switch.%d.portIndex.%d.postCursor10GCopper"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_10G_COPPER      FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_25G_COPPER      "api.platform.config.switch.%d.portIndex.%d.postCursor25GCopper"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_25G_COPPER      FM_API_ATTR_INT


/**
 * (Optional) SERDES postCursor for DA cables on the given switch port and lane.
 *                                                                      \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_POSTCURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.postCursorCopper 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_1G_COPPER  "api.platform.config.switch.%d.portIndex.%d.lane.%d.postCursor1GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_POSTCURSOR_1G_COPPER  FM_API_ATTR_INT                                     
                                                                                                                
#define FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_10G_COPPER "api.platform.config.switch.%d.portIndex.%d.lane.%d.postCursor10GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_POSTCURSOR_10G_COPPER FM_API_ATTR_INT                                     
                                                                                                                
#define FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_25G_COPPER "api.platform.config.switch.%d.portIndex.%d.lane.%d.postCursor25GCopper"
#define FM_AAT_API_PLATFORM_PORT_LANE_POSTCURSOR_25G_COPPER FM_API_ATTR_INT


/**
 * (Optional) Default SERDES postCursor for optical modules, for ports on the 
 * given switch. 
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_POSTCURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The default value is 15.
 */
#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_OPTICAL_DEFAULT "api.platform.config.switch.%d.port.default.postCursorOptical"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_OPTICAL_DEFAULT FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PORT_POSTCURSOR_OPTICAL_DEFAULT 15


/**
 * (Optional) SERDES postCursor for optical modules on the given switch port.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_POSTCURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.postCursorOptical 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_1G_OPTICAL      "api.platform.config.switch.%d.portIndex.%d.postCursor1GOptical"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_1G_OPTICAL      FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_10G_OPTICAL     "api.platform.config.switch.%d.portIndex.%d.postCursor10GOptical"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_10G_OPTICAL     FM_API_ATTR_INT

#define FM_AAK_API_PLATFORM_PORT_POSTCURSOR_25G_OPTICAL     "api.platform.config.switch.%d.portIndex.%d.postCursor25GOptical"
#define FM_AAT_API_PLATFORM_PORT_POSTCURSOR_25G_OPTICAL     FM_API_ATTR_INT


/**
 * (Optional) SERDES postCursor for optical modules on the given switch 
 * port and lane.
 *                                                                      \lb\lb
 * This property format MUST be used for QSFP ports.
 *                                                                      \lb\lb
 * Value is as specified by the ''FM_PORT_TX_LANE_POSTCURSOR'' port attribute. 
 *                                                                      \lb\lb
 * The property should be set for each of the SerDes bit rates 
 * (1.25Gbps, 10.3125Gbps and 25.78125Gbps). 
 *                                                                      \lb\lb
 * The default value is specified by the port.default.postCursorOptical 
 * property for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_1G_OPTICAL    "api.platform.config.switch.%d.portIndex.%d.lane.%d.postCursor1GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_POSTCURSOR_1G_OPTICAL    FM_API_ATTR_INT                                     
                                                                                                                   
#define FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_10G_OPTICAL   "api.platform.config.switch.%d.portIndex.%d.lane.%d.postCursor10GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_POSTCURSOR_10G_OPTICAL   FM_API_ATTR_INT                                     
                                                                                                                   
#define FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_25G_OPTICAL   "api.platform.config.switch.%d.portIndex.%d.lane.%d.postCursor25GOptical"
#define FM_AAT_API_PLATFORM_PORT_LANE_POSTCURSOR_25G_OPTICAL   FM_API_ATTR_INT


/**
 * (Optional) Default interface type. Applies to all ports on the 
 * specified switch. 
 *                                                                      \lb\lb
 * See the portIndex.%%d.interfaceType property for a list of supported 
 * values. 
 *                                                                      \lb\lb
 * Default value is NONE.
 */ 
#define FM_AAK_API_PLATFORM_PORT_INTF_TYPE_DEFAULT          "api.platform.config.switch.%d.port.default.interfaceType"
#define FM_AAT_API_PLATFORM_PORT_INTF_TYPE_DEFAULT          FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PORT_INTF_TYPE_DEFAULT          "NONE"


/**
 * (Optional) Interface type for the given switch port.
 *                                                                      \lb\lb
 * Value is one of the following:
 * NONE, SFPP, QSFP_LANE0, QSFP_LANE1, QSFP_LANE2, QSFP_LANE3.
 *                                                                      \lb\lb
 * Default value is specified by the port.default.interfaceType
 * property for this switch.
 */
#define FM_AAK_API_PLATFORM_PORT_INTF_TYPE                  "api.platform.config.switch.%d.portIndex.%d.interfaceType"
#define FM_AAT_API_PLATFORM_PORT_INTF_TYPE                  FM_API_ATTR_TEXT


/**
 * (Optional) Default port capabilities. Applies to all ports on the 
 * specified switch. 
 *                                                                      \lb\lb
 * See the portIndex.%%d.capability property for a list of supported 
 * values. 
 *                                                                      \lb\lb
 * Default value is NONE.
 */
#define FM_AAK_API_PLATFORM_PORT_CAPABILITY_DEFAULT         "api.platform.config.switch.%d.port.default.capability"
#define FM_AAT_API_PLATFORM_PORT_CAPABILITY_DEFAULT         FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PORT_CAPABILITY_DEFAULT         "NONE"


/**
 * (Optional) Port capabilities for the given switch port.
 *                                                  \lb\lb
 * Value is a comma-separated string (with no spaces), consisting of
 * one or more of the following: 
 *                                                  \lb\lb
 * NONE - no port capabilities specified.
 *                                                  \lb
 * 10M - ''FM_PORT_CAPABILITY_SPEED_10M''
 *                                                  \lb
 * 100M - ''FM_PORT_CAPABILITY_SPEED_100M''
 *                                                  \lb
 * 1G - ''FM_PORT_CAPABILITY_SPEED_1G''
 *                                                  \lb
 * 2PT5G - ''FM_PORT_CAPABILITY_SPEED_2PT5G''
 *                                                  \lb
 * 5G - ''FM_PORT_CAPABILITY_SPEED_5G''
 *                                                  \lb
 * 10G - ''FM_PORT_CAPABILITY_SPEED_10G''
 *                                                  \lb
 * 20G - ''FM_PORT_CAPABILITY_SPEED_20G''
 *                                                  \lb
 * 25G - ''FM_PORT_CAPABILITY_SPEED_25G''
 *                                                  \lb
 * 40G - ''FM_PORT_CAPABILITY_SPEED_40G''
 *                                                  \lb
 * 100G - ''FM_PORT_CAPABILITY_SPEED_100G''
 *                                                  \lb
 * LAG - ''FM_PORT_CAPABILITY_LAG_CAPABLE''
 *                                                  \lb
 * ROUTE - ''FM_PORT_CAPABILITY_CAN_ROUTE''
 *                                                  \lb
 * SW_LED - port supports software-driven LED.
 *                                                  \lb\lb
 * Default value is specified by the port.default.capability property 
 * for this switch. 
 */
#define FM_AAK_API_PLATFORM_PORT_CAPABILITY                 "api.platform.config.switch.%d.portIndex.%d.capability"
#define FM_AAT_API_PLATFORM_PORT_CAPABILITY                 FM_API_ATTR_TEXT


/**
 * (Optional) Default port DFE mode. Applies to all ports on the 
 * specified switch. 
 *                                                                      \lb\lb
 * See the portIndex.%%d.dfeMode property for a list of supported values. 
 *                                                                      \lb\lb
 * Default value is ONE_SHOT.
 */
#define FM_AAK_API_PLATFORM_PORT_DFE_MODE_DEFAULT           "api.platform.config.switch.%d.port.default.dfeMode"
#define FM_AAT_API_PLATFORM_PORT_DFE_MODE_DEFAULT           FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PORT_DFE_MODE_DEFAULT           "ONE_SHOT"


/**
 * (Optional) Port DFE mode for the given switch port.
 *                                                  \lb\lb
 * Value is a string specifying one of the following: 
 *                                                  \lb\lb
 * STATIC - ''FM_DFE_MODE_STATIC''
 *                                                  \lb
 * ONE_SHOT - ''FM_DFE_MODE_ONE_SHOT''
 *                                                  \lb
 * CONTINOUS - ''FM_DFE_MODE_CONTINUOUS''
 *                                                  \lb
 * KR - ''FM_DFE_MODE_KR''
 *                                                  \lb\lb
 * Default value is specified by the port.default.dfeMode property. 
 *  
 */
#define FM_AAK_API_PLATFORM_PORT_DFE_MODE                   "api.platform.config.switch.%d.portIndex.%d.dfeMode"
#define FM_AAT_API_PLATFORM_PORT_DFE_MODE                   FM_API_ATTR_TEXT


/**
 * (Optional) AN73 abilities for the given switch port.
 *                                                  \lb\lb
 * Value is a comma-separated string (with no spaces), consisting of
 * one or more of the following: 
 *                                                  \lb\lb
 * RF
 *                                                  \lb
 * NP
 *                                                  \lb
 * 1GBase-KX
 *                                                  \lb
 * 10GBase-KR
 *                                                  \lb
 * 40GBase-KR4
 *                                                  \lb
 * 40GBase-CR4 
 *                                                  \lb
 *                                               \lb\lb
 * Default abilities advertised are: 
 *                                                  \lb
 * 1GBase-KX,10GBase-KR,40GBase-KR4,40GBase-CR4 
 *  
 */
#define FM_AAK_API_PLATFORM_PORT_AN73_ABILITY            "api.platform.config.switch.%d.portIndex.%d.an73Ability"
#define FM_AAT_API_PLATFORM_PORT_AN73_ABILITY            FM_API_ATTR_TEXT


/**
 * (Optional) Shared library name from which to load the switch 
 * management function interfaces. 
 *                                                                     \lb\lb
 * WARNING: This configuration parameter is case sensitive!
 *                                                                     \lb\lb
 * All switch management features are disabled if this property is not 
 * specified. 
 */
#define FM_AAK_API_PLATFORM_SHARED_LIB_NAME                 "api.platform.config.switch.%d.sharedLibraryName"
#define FM_AAT_API_PLATFORM_SHARED_LIB_NAME                 FM_API_ATTR_TEXT


/**
 * (Optional) Disables loading of the specified function interfaces.
 *                                                                      \lb\lb
 * Value is a comma-separated string (with no spaces), consisting of
 * one or more of the following:
 *                                                                      \lb\lb
 * NONE
 *                                                  \lb
 * InitSwitch
 *                                                  \lb
 * ResetSwitch
 *                                                  \lb
 * I2cWriteRead
 *                                                  \lb
 * SelectBus
 *                                                  \lb
 * GetPortXcvrState
 *                                                  \lb
 * SetPortXcvrState
 *                                                  \lb
 * SetPortLed
 *                                                  \lb
 * EnablePortIntr
 *                                                  \lb
 * GetPortIntrPending
 *                                                  \lb
 * LibPostInit
 *                                                  \lb
 * SetVrmVoltage
 *                                                                      \lb\lb
 * WARNING: This configuration parameter is case sensitive!
 *                                                                      \lb\lb
 * For clarity and convenience the fmPlatformLib prefix is stripped 
 * from the actual function interface. 
 *                                                                      \lb\lb
 * For example, fmPlatformLibInitSwitch becomes InitSwitch 
 * in the value list above.
 *                                                                      \lb\lb
 * All function interfaces will be loaded if this property is not specified. 
 */ 
#define FM_AAK_API_PLATFORM_SHARED_LIB_DISABLE              "api.platform.config.switch.%d.sharedLibrary.disable"
#define FM_AAT_API_PLATFORM_SHARED_LIB_DISABLE              FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_SHARED_LIB_DISABLE              "NONE"

/**
 * (Optional) Specifies the FM10000 GPIO number used for port logic 
 * interrupts. 
 */
#define FM_AAK_API_PLATFORM_PORT_INTERRUPT_GPIO             "api.platform.config.switch.%d.portIntrGpio"
#define FM_AAT_API_PLATFORM_PORT_INTERRUPT_GPIO             FM_API_ATTR_INT

/**
 * (Optional) Specifies the FM10000 GPIO number used for the I2C reset 
 * Control. 
 */
#define FM_AAK_API_PLATFORM_I2C_RESET_GPIO                  "api.platform.config.switch.%d.i2cResetGpio"
#define FM_AAT_API_PLATFORM_I2C_RESET_GPIO                  FM_API_ATTR_INT

/**
 * (Optional) Specifies the FM10000 GPIO number used to control the 
 * write protect pin on the SPI flash (switch's NVM). 
 *                                                                      \lb\lb
 * Setting this property to -1 (default) indicates either the write protection 
 * is not used or the protection is done using a different method. 
 */
#define FM_AAK_API_PLATFORM_FLASH_WP_GPIO                   "api.platform.config.switch.%d.flashWriteProtectGpio"
#define FM_AAT_API_PLATFORM_FLASH_WP_GPIO                   FM_API_ATTR_INT

/**
 * (Optional) Specifies whether the SerDes preserve their configuration upon 
 * change of the FM_PORT_ETHERNET_INTERFACE_MODE or the platform re-applies 
 * the default values. 
 *  
 * By default (1) the SerDes preserve the configured values.
 */
#define FM_AAK_API_PLATFORM_KEEP_SERDES_CFG                 "api.platform.config.switch.%d.keepSerdesCfg"
#define FM_AAT_API_PLATFORM_KEEP_SERDES_CFG                 FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_KEEP_SERDES_CFG                 1

/**
 * (Optional) Number of PHY/retimers connected to a given switch. 
 *                                                                      \lb\lb
 * Default value is 0.
 */ 
#define FM_AAK_API_PLATFORM_NUM_PHYS                        "api.platform.config.switch.%d.numPhys"
#define FM_AAT_API_PLATFORM_NUM_PHYS                        FM_API_ATTR_INT

/** 
 * (Optional) Specifies the PHY/retimer model. 
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when 
 * api.platform.config.numPhys > 0 
 *                                                                      \lb\lb
 * Supported models: retimer GN2412. 
 */
#define FM_AAK_API_PLATFORM_PHY_MODEL                       "api.platform.config.switch.%d.phy.%d.model"
#define FM_AAT_API_PLATFORM_PHY_MODEL                       FM_API_ATTR_TEXT

/** 
 * (Optional) Specifies the PHY I2C address. 
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when 
 * api.platform.config.numPhys > 0 
 */
#define FM_AAK_API_PLATFORM_PHY_ADDR                        "api.platform.config.switch.%d.phy.%d.addr"
#define FM_AAT_API_PLATFORM_PHY_ADDR                        FM_API_ATTR_INT

/**
 * (Optional) Unique 32-bit value associated with a PHY for the shared library.
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when 
 * api.platform.config.numPhys > 0 
 *                                                                      \lb\lb
 * This ID is used by the platform shared library to select the hardware
 * resource for a given PHY. 
 */
#define FM_AAK_API_PLATFORM_PHY_HW_RESOURCE_ID              "api.platform.config.switch.%d.phy.%d.hwResourceId"
#define FM_AAT_API_PLATFORM_PHY_HW_RESOURCE_ID              FM_API_ATTR_INT

/**
 * (Optional) Specifies the PHY/RETIMER transmit equalization parameters 
 * for the given lane on a given phy.
 *                                                                      \lb\lb
 * The following parameters are used as argument to the GN2412 "Control 
 * Non-KR Transmit Equalization" command (Command code 0x17). 
 *                                                                      \lb\lb
 * Example (showing the default values): 
 *                                                                      \lb\lb
 * switch.%d.phy.%d.lane.%d.txEqualizer text "POL=5 PRE=0 ATT=15 POST=0" 
 *                                                                      \lb\lb
 * The quote " " are required and the values must decimal values. 
 *                                                                      \lb\lb
 * The valid range for each parameter are: 
 *  
 * Tx tap polarity (POL):          0 to 31
 * Tx pre-tap coefficient (PRE):   0 to 15
 * Tx attenuation (ATT):           0 to 15
 * Tx post-tap coefficient (POST): 0 to 31
 */
#define FM_AAK_API_PLATFORM_PHY_TX_EQUALIZER                "api.platform.config.switch.%d.phy.%d.lane.%d.txEqualizer"
#define FM_AAT_API_PLATFORM_PHY_TX_EQUALIZER                FM_API_ATTR_TEXT

/**
 * (Optional) Specifies the PHY/RETIMER application mode for the given lane 
 * on a given phy.
 *                                                                      \lb\lb
 * For the GN2412 this property is used with the "Configure Application Modes" 
 * command (Command code 0x19). 
 */
#define FM_AAK_API_PLATFORM_PHY_APP_MODE                   "api.platform.config.switch.%d.phy.%d.lane.%d.appMode"
#define FM_AAT_API_PLATFORM_PHY_APP_MODE                   FM_API_ATTR_INT

/**
 * (Optional) Unique 32-bit value associated with a voltage 
 * regulator module for the shared library. 
 *                                                                      \lb\lb
 * Although this is marked as optional it becomes required when 
 * VRM resources is added. 
 *                                                                      \lb\lb
 * This ID is used by the platform shared library to select the hardware
 * resource for a given VRM. 
 */
#define FM_AAK_API_PLATFORM_VRM_HW_RESOURCE_ID             "api.platform.config.switch.%d.%s.hwResourceId"
#define FM_AAT_API_PLATFORM_VRM_HW_RESOURCE_ID             FM_API_ATTR_INT

/**
 * (Optional) Indicates to program the VRM with the default 
 * voltage values (defined in the EAS) when the fuse box is not 
 * programmed with the voltage scaling values (set to 0). 
 *                                                                      \lb\lb
 * Default value is 0. So if the fuse box is not programmed then
 * the VRM are not set and use the default settings set by HW. 
 */
#define FM_AAK_API_PLATFORM_VRM_USE_DEF_VOLTAGE            "api.platform.config.switch.%d.useDefVoltageScaling"
#define FM_AAT_API_PLATFORM_VRM_USE_DEF_VOLTAGE            FM_API_ATTR_INT


/** @} (end of Doxygen group) */


/************************************************************************
 ****                                                                ****
 ****               END DOCUMENTED PLATFORM ATTRIBUTES               ****
 ****                                                                ****
 ****               BEGIN UNDOCUMENTED ATTRIBUTES                    ****
 ****                                                                ****
 ************************************************************************/
 
/************************************************************
 * The following attributes are not documented in the platform
 * User Guide as they are intended for Intel Internal Use
 * only.
 ************************************************************/

/* Indicates whether the interrupt will be received from PCIe MSI or not */
#define FM_AAK_API_PLATFORM_MSI_ENABLED  	"api.platform.config.switch.%d.msiEnabled"
#define FM_AAT_API_PLATFORM_MSI_ENABLED     FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_MSI_ENABLED     FALSE

/* Specifies the Frame Handler Clock in Hz
 * 
 * Setting this property to -1 (default) indicates that the frame
 * handler clock will be default to the part's default value.
 *
 * This parameters has no impact on certain parts (Frame Handler Clock 
 * is locked in certain SKUs) */
#define FM_AAK_API_PLATFORM_FH_CLOCK        "api.platform.config.switch.%d.fhClock"
#define FM_AAT_API_PLATFORM_FH_CLOCK        FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_FH_CLOCK        -1

/*
 * (Optional) Device name used by EBI through FTDI. Value is a text 
 * string, up to 31 characters in length. 
 */
#define FM_AAK_API_PLATFORM_EBI_DEV_NAME        "api.platform.config.ebiDevName"
#define FM_AAT_API_PLATFORM_EBI_DEV_NAME        FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_EBI_DEV_NAME        "ebiDevName_not_set"

/*
 * (Optional) Memory offset to the switch memory via /dev/mem interface.
 */
#define FM_AAK_API_PLATFORM_DEVMEM_OFFSET       "api.platform.config.switch.%d.devMemOffset"
#define FM_AAT_API_PLATFORM_DEVMEM_OFFSET       FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_DEVMEM_OFFSET       "devMemOffset_not_set"

/*
 * (Optional) Network device name used to communicate with the host driver 
 * through the raw socket. There is a netdevice per switch.
 */
#define FM_AAK_API_PLATFORM_NETDEV_NAME        "api.platform.config.switch.%d.netDevName"
#define FM_AAT_API_PLATFORM_NETDEV_NAME        FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_NETDEV_NAME        "no_netDevName"

/*
 * (Optional) The uio device name associated to the switch.
 *                                                                      \lb\lb
 * For example: /dev/uio0
 *                                                                      \lb\lb
 * When not defined the software will iterate the /sys/class/uio/ 
 * directory and find the proper uio device if it exists.
 */ 
#define FM_AAK_API_PLATFORM_UIO_DEV_NAME        "api.platform.config.switch.%d.uioDevName"
#define FM_AAT_API_PLATFORM_UIO_DEV_NAME        FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_UIO_DEV_NAME        "no_uioDevName"

/* 
 * (Optional) Defines the number of interrupt timeouts prior declaring 
 * UIO interrupt issue with the host driver. 
 *  
 * Set this property to 0 to disable.
 */
#define FM_AAK_API_PLATFORM_INTR_TIMEOUT_CNT    "api.platform.config.switch.%d.intrTimeoutCnt"
#define FM_AAT_API_PLATFORM_INTR_TIMEOUT_CNT    FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_INTR_TIMEOUT_CNT    2

/* 
 * (Optional) Platform layer debug options.
 *                                                                     \lb\lb
 * Value is a comma-separated string (with no spaces), consisting of
 * one or more of the following:
 * NONE, CONFIG, MOD_STATE, MOD_INTR, MOD_TYPE, PLAT_LOG. 
 *                                                                     \lb\lb
 * The default value is NONE.
 */ 
#define FM_AAK_API_PLATFORM_CONFIG_DEBUG        "api.platform.config.debug"
#define FM_AAT_API_PLATFORM_CONFIG_DEBUG        FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_CONFIG_DEBUG        "NONE"

/* 
 * (Optional) Defines the switch boot mode. 
 *  
 *   'SPI' : The switch will be booted from the SPI memory device (default)
 *   'EBI' : The switch will be booted from EBI bus (debug)
 *   'I2C' : The switch will be booted from I2C bus (debug)
 *  
 */ 
#define FM_AAK_API_PLATFORM_BOOT_MODE           "api.platform.config.switch.%d.bootMode"
#define FM_AAT_API_PLATFORM_BOOT_MODE           FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_BOOT_MODE           "SPI"

/* 
 * (Optional) Defines the register access mode after boot.
 * 
 *    'PCIE' : The switch will be managed from PCIE bus
 *    'EBI'  : The switch will be managed from EBI bus (debug)
 *    'I2C'  : The switch will be managed from I2C bus (debug)
 */
#define FM_AAK_API_PLATFORM_REGISTER_ACCESS     "api.platform.config.switch.%d.regAccess"
#define FM_AAT_API_PLATFORM_REGISTER_ACCESS     FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_REGISTER_ACCESS     "PCIE"

/* 
 * (Optional) Defines who executes the PCIE event ISR
 * 
 *    'AUTO' : (default) The ISR execution is selected based on
 *             api.platform.config.switch.%d.bootMode. The ISR is 
 *             done in SW if boot mode equals EBI or I2C and SPI otherwise.
 *    'SW'   : Force the ISR to be done from software
 *    'SPI'  : Force the ISR to be done from SPI flash (NVM)
 */
#define FM_AAK_API_PLATFORM_PCIE_ISR            "api.platform.config.switch.%d.pcieIsr"
#define FM_AAT_API_PLATFORM_PCIE_ISR            FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PCIE_ISR            "AUTO"

/* 
 * (Optional) Defines which logical port should act as the CPU port
 */
#define FM_AAK_API_PLATFORM_CPU_PORT            "api.platform.config.switch.%d.cpuPort"
#define FM_AAT_API_PLATFORM_CPU_PORT            FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_CPU_PORT            0

/* 
 * (Optional) Defines the interrupt polling period in msec.
 *                                                                     \lb\lb
 *    0: polling disabled
 */
#define FM_AAK_API_PLATFORM_INT_POLL_MSEC    "api.platform.config.switch.%d.intPollMsec"
#define FM_AAT_API_PLATFORM_INT_POLL_MSEC    FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_INT_POLL_MSEC    10

/* (optional) Use as global flag to enable(1)/disable(0) the De-Emphasis
 * configuration of the re-timer/PHY.
 */
#define FM_AAK_API_PLATFORM_PHY_ENABLE_DEEMPHASIS     "api.platform.config.switch.%d.phyEnableDeemphasis"
#define FM_AAT_API_PLATFORM_PHY_ENABLE_DEEMPHASIS     FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PHY_ENABLE_DEEMPHASIS     0

/************************************************************
 * The following attributes are used as the boot 
 * configuration when not booting from SPI Flash. 
 *  
 * NOTE: These attributes are in the NOT DOCUMENTED section 
 * because they are documented as part of the FM10000
 * Boot Image Generator (rrcBig) .
 ************************************************************/


/* (optional) Define the SPI memory transfer mode when this boot mode is used.
 * Refer to EAS for more information about each mode. 
 *  
 * Note that this attribute is only required when using EBI/I2C boot with a 
 * SPI ISR (api.platform.config.switch.%d.pcieIsr) 
 * 
 *   0: 'single' (default)
 *   1: 'dual-pin'
 *   2: 'quad-pin'
 *   3: 'single-fast'
 */ 
#define FM_AAK_API_PLATFORM_SPI_TRANSFER_MODE       "api.platform.config.switch.%d.bootCfg.spiTransferMode"
#define FM_AAT_API_PLATFORM_SPI_TRANSFER_MODE       FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_SPI_TRANSFER_MODE       0


/* (optional) Define the SPI memory transfer speed when this boot mode is used.
 *
 * Note that this attribute is only required when using EBI/I2C boot with a 
 * SPI ISR (api.platform.config.switch.%d.pcieIsr) 
 * 
 *   0: 0.39 Mhz
 *   1: 0.78 Mhz
 *   2: 1.56 Mhz
 *   3: 3.12 Mhz
 *   4: 6.25 Mhz
 *   5: 12.5 Mhz
 *   6: 25.0 Mhz
 *   7: 50.0 Mhz (default)
 */ 
#define FM_AAK_API_PLATFORM_SPI_TRANSFER_SPEED      "api.platform.config.switch.%d.bootCfg.spiTransferSpeed"
#define FM_AAT_API_PLATFORM_SPI_TRANSFER_SPEED      FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_SPI_TRANSFER_SPEED      7


/* (optional) Define which PEP will be used for managing the switch. Only this
 * PEP will be allowed full register access through BAR4. 
 * 
 * Valid values are 0..8. Note that odd PEP numbers are only valid if 
 * the mode is 2x4. Default mgmtPep is 8.
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_MGMT_PEP        "api.platform.config.switch.%d.bootCfg.mgmtPep"
#define FM_AAT_API_PLATFORM_BOOTCFG_MGMT_PEP        FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_BOOTCFG_MGMT_PEP        8


/* (optional) Define the mode of operation for each pair of PCIe Host Interface:
 *  
 * Valid PEP indexes are 0,2,4,6
 *  0: 1x8 lanes
 *  1: 2x4 lanes (default)
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_MODE        "api.platform.config.switch.%d.bootCfg.pep.%d.mode"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_MODE        FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_MODE        1


/* (optional) Define which PEPs should be enabled
 *  
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if 
 * the mode is 2x4.
 *   0: disabled
 *   1: enabled
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_ENABLE      "api.platform.config.switch.%d.bootCfg.pep.%d.enable"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_ENABLE      FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_ENABLE      1


/* (optional) Define the number of PCIE lanes used for each PEP.
 * Must be consistent with PCIE mode. i.e a PEP in 2x4 mode must                          .
 * have 4 or less lanes 
 *  
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if                                      .
 * the mode is 2x4. Default is 1
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_NUM_LANES   "api.platform.config.switch.%d.bootCfg.pep.%d.numberOfLanes"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_NUM_LANES   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_NUM_LANES   1


/* (optional) Define the serial number of each PEP in EUI-64 format.
 *
 * You may refer to the EUI-64 official document at:
 * http://standards.ieee.org/regauth/oui/tutorials/EUI64.html
 *
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if
 * the mode is 2x4.
 * 
 * Default value is 00-A0-C9-FF-FF-23-45-67
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_SN          "api.platform.config.switch.%d.bootCfg.pep.%d.serialNumber"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_SN          FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_SN          "00:A0:C9:FF:FF:23:45:67"


/* (optional) Define custom MAC addresses for platform/application usage in EUI-64 format.
 *
 * You may refer to the EUI-64 official document at:
 * http://standards.ieee.org/regauth/oui/tutorials/EUI64.html
 * 
 * Note: The custom MACs are loaded into FM10000 scratch memory for later 
 * retrieval by software. By default, they are not applied for any purpose, 
 * e.g. Routing, pause mac, etc.
 * 
 * There are 4 custom MAC slots. Valid custom MAC slots are 0..3
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_CUSTOM_MAC      "api.platform.config.switch.%d.bootCfg.customMac.%d"
#define FM_AAT_API_PLATFORM_BOOTCFG_CUSTOM_MAC      FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_BOOTCFG_CUSTOM_MAC      "00:00:00:FF:FF:00:00:00"


/* (optional) Define the PCIE subsystem vendor ID of each PEP.
 *
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if
 * the mode is 2x4.
 * 
 * Default value is 0x8086.
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_SUB_VENDOR_ID   "api.platform.config.switch.%d.bootCfg.pep.%d.subVendorId"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_SUB_VENDOR_ID   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_SUB_VENDOR_ID   0x8086


/* (optional) Define the PCIE subsystem Device ID of each PEP.
 *
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if
 * the mode is 2x4.
 * 
 * Default value is 0.
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_SUB_DEVICE_ID   "api.platform.config.switch.%d.bootCfg.pep.%d.subDeviceId"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_SUB_DEVICE_ID   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_SUB_DEVICE_ID   0


/* (optional) Define the reference clock source for SYSTIME
 * 
 *   0: PCIE_REFCLK (100MHz) (default)
 *   1: IEEE1588_REFCLK (50MHZ-256MHZ)
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_SYSTIME_CLK_SRC     "api.platform.config.switch.%d.bootCfg.systimeClockSource"
#define FM_AAT_API_PLATFORM_BOOTCFG_SYSTIME_CLK_SRC     FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_SYSTIME_CLK_SRC     0


/* (optional) Override mode to skip PCIE initialization steps.
 * See EAS Initialization Sequence steps 13.9 - 16 
 *  
 *   0: Do not skip PCIE init (default)
 *   1: Skip PCIE init
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_SKIP_PCIE_INIT  "api.platform.config.switch.%d.bootCfg.skipPcieInitialization"
#define FM_AAT_API_PLATFORM_BOOTCFG_SKIP_PCIE_INIT  FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_SKIP_PCIE_INIT  0


/* (optional) Override mode to skip Mem Repair.
 * See EAS Initialization Sequence step 12.
 *
 *   0: Do not skip Mem Repair (default)
 *   1: Skip Mem Repair
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_SKIP_MEM_REPAIR "api.platform.config.switch.%d.bootCfg.skipMemRepair"
#define FM_AAT_API_PLATFORM_BOOTCFG_SKIP_MEM_REPAIR FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_SKIP_MEM_REPAIR 0


/* (optional) In addition to initializing memories with BIST, run the BIST test.
 *
 *   0: Do not run BIST (default)
 *   1: Run BIST
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_ENABLE_BIST_CHECK   "api.platform.config.switch.%d.bootCfg.enableBistCheck"
#define FM_AAT_API_PLATFORM_BOOTCFG_ENABLE_BIST_CHECK   FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_ENABLE_BIST_CHECK   0


/* (optional) PCIe Master/SerDes Firmware Verification Enable
 * CRC and Version Check
 *  
 *   0: Disabled (default)
 *   1: Enabled
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_ENABLE_FW_VERIFY    "api.platform.config.switch.%d.bootCfg.enableFwVerify"
#define FM_AAT_API_PLATFORM_BOOTCFG_ENABLE_FW_VERIFY    FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_ENABLE_FW_VERIFY    0


/* (optional) Define which PEPs should be allowed full register access through BAR4. 
 *
 * IMPORTANT: Only one PEP should have BAR4 enabled. Enabling BAR4 access on more than
 * one PEP poses a security risk, it should only be done if you know what you are doing
 * or for debug purposes.
 *
 * IMPORTANT: Note that these parameters will be overwritten if 
 * "api.platform.config.switch.0.bootCfg.mgmtPep" is not -1.
 *
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if
 * the mode is 2x4.
 *
 *   0: PEP is NOT allowed to access BAR4 (default)
 *   1: PEP is allowed to access BAR4
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED    "api.platform.config.switch.%d.bootCfg.pep.%d.bar4Allowed"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED    FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED    0


/* (optional) Define the Auto Rx/Tx Lane Flipping behavior.
 *  
 * When disabled the lane flipping can be controlled with 
 * "api.platform.config.switch.0.bootCfg.pep.%d.{rx,tx}LaneFlip". 
 *  
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if 
 * the mode is 2x4. 
 *  
 *    0: Manual Lane Flip
 *    1: Auto Lane Flip (default)
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_AUTO_LANE_FLIP   "api.platform.config.switch.%d.bootCfg.pep.%d.autoLaneFlip"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_AUTO_LANE_FLIP   FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_AUTO_LANE_FLIP   1


/* (optional) Define the Rx Lane Flip For Each PEP.
 *  
 * IMPORTANT, these parameters will have no impact if 
 * "api.platform.config.switch.0.bootCfg.pep.%d.autoLaneFlip" is not disabled (0).
 *  
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if 
 * the mode is 2x4. 
 *  
 *    0: Not Flipped  (default)
 *    1: Flipped      
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_RX_LANE_FLIP   "api.platform.config.switch.%d.bootCfg.pep.%d.rxLaneFlip"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_RX_LANE_FLIP   FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_RX_LANE_FLIP   0


/* (optional) Define the Tx Lane Flip For Each PEP.
 *  
 * IMPORTANT, these parameters will have no impact if 
 * "api.platform.config.switch.0.bootCfg.pep.%d.autoLaneFlip" is not disabled (0).
 *  
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if 
 * the mode is 2x4. 
 *  
 *    0: Not Flipped  (default)
 *    1: Flipped      
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_TX_LANE_FLIP   "api.platform.config.switch.%d.bootCfg.pep.%d.txLaneFlip"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_TX_LANE_FLIP   FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_TX_LANE_FLIP   0


/* (optional) Define the master lane for each PEP.
 *  
 * Valid PEP indexes are 0..8. Note that odd PEP numbers are only valid if 
 * the mode is 2x4. 
 *  
 *  -1: Default for Pep (i.e. 0 for even PEPs and 4 for odd PEPs)
 *   0: Even PEP Default
 *   1: Even PEP x2 reversed
 *   3: Even PEP x4 reversed
 *   7: Even PEP x8 reversed
 *   4: Odd PEP Default
 *   5: Odd PEP x2 reversed
 *   7: Odd PEP x4 reversed
 */
#define FM_AAK_API_PLATFORM_BOOTCFG_PEP_MASTER_LANE   "api.platform.config.switch.%d.bootCfg.pep.%d.masterLane"
#define FM_AAT_API_PLATFORM_BOOTCFG_PEP_MASTER_LANE   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_BOOTCFG_PEP_MASTER_LANE   -1


/********************************************************************
 * The following properties are used for scheduler configuration.
 * They will define scheduler configuration mode and token lists when
 * configured as manual.
 *******************************************************************/

/*
 * (optional) Defines which scheduler ring to use for a specific switch 
 *  
 * By default an automatic scheduler ring will be used, based on port usage. 
 * This will be selected by setting this attribute to -1.
 */ 
#define FM_AAK_API_PLATFORM_SCHED_LIST_SELECT         "api.platform.config.switch.%d.schedListSelect"
#define FM_AAT_API_PLATFORM_SCHED_LIST_SELECT         FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_SCHED_LIST_SELECT         -1


/*
 * (optional) Defines a scheduler token list length for a specific switch and ring
 *  
 * IMPORTANT, slot 0 must always be an idle slot 
 */ 
#define FM_AAK_API_PLATFORM_SCHED_LIST_LENGTH         "api.platform.config.switch.%d.schedList.%d.length"
#define FM_AAT_API_PLATFORM_SCHED_LIST_LENGTH         FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_SCHED_LIST_LENGTH         0


/*
 * (optional) Defines a scheduler token item for a specific switch and ring
 *  
 * IMPORTANT, slot 0 must always be an idle slot 
 */ 
#define FM_AAK_API_PLATFORM_SCHED_TOKEN               "api.platform.config.switch.%d.schedList.%d.slot.%d"
#define FM_AAT_API_PLATFORM_SCHED_TOKEN               FM_API_ATTR_TEXT


/**
 * Specifies the FM10000 I2C bus clock divider
 * Control.
 * By default (10) the bus speed is set to 400kHz
 */
#define FM_AAK_API_PLATFORM_I2C_CLKDIVIDER             "api.platform.config.switch.%d.i2cClkDivider"
#define FM_AAT_API_PLATFORM_I2C_CLKDIVIDER             FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_I2C_CLKDIVIDER             10


#ifdef FM_LT_WHITE_MODEL_SUPPORT
/****************************************************************************
 * Platform attributes, used as an argument to ''fmPlatformSetAttribute'' and
 * ''fmPlatformGetAttribute''.
 ****************************************************************************/
enum _fm_platformAttr
{
    /** Type fm_int: The maximum number of bytes send by the socket thread 
     *  to other sockets before switching back to its server socket. The
     *  server stops after this limit is reached, a value of 0 will send
     *  one message max. */
    FM_PLATFORM_ATTR_EGRESS_LIMIT = 0,

    /** UNPUBLISHED: Type fm_text: Specify the name of the topology file
     *  that lists the internal link connections between white model
     *  instances. This file must be located in the path specified by the
     *  WMODEL_INFO_PATH environment variable. If multiple units (API
     *  instances) are included in a single topology, each unit must use the
     *  same path.
     *                                                                  \lb\lb
     *  The topology file is an ASCII text file, one line per internal link,
     *  each link specified using the following format:
     *                                                                  \lb\lb
     *  <Unit1>,<Switch1>,<Port1>,<Unit2>,<Switch2>,<Port2>
     *                                                                  \lb\lb
     *  For example:
     *                                                                  \lb\lb
     *  0,0,1,0,1,10                                                    \lb
     *  0,0,2,1,2,20
     *                                                                  \lb\lb
     *  specifies that:
     *                                                                  \lb\lb
     *  Unit 0, Switch 0, Port 1 is connected to Unit 0, Switch 1, Port 10
     *                                                                  \lb\lb
     *  Unit 0, Switch 0, Port 2 is connected to Unit 1, Switch 2, Port 20
     *                                                                  \lb\lb
     *  Note that each Unit (API instance) can manage one or more switches.
     *  However, ''FM_MAX_NUM_FOCALPOINTS'' must be defined as the maximum
     *  number of switches per unit and all API instances must be initialized
     *  prior to setting this attribute. The API attribute 
     *  ''api.platform.model.position'' is used to identify the unit number
     *  for each API instance. */
    FM_PLATFORM_ATTR_TOPOLOGY,

    /** UNPUBLISHED: For internal use only. */
    FM_PLATFORM_ATTR_MAX

};
#endif


#endif /* __FM_PLATFORM_ATTR_H */

