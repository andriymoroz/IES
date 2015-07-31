/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_hw_int.h
 * Creation Date:   June 8, 2007
 * Description:     Contains FM10000 Hardware Constants.
 *                  For use only by FM10000-Specific Code.
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

#ifndef __FM_FM10000_API_HW_INT_H
#define __FM_FM10000_API_HW_INT_H

/* The maximum EPL number, numbering from 0 as the first EPL. */
#define FM10000_MAX_EPL                 8
#define FM10000_NUM_EPLS                (FM10000_MAX_EPL + 1)

/* The maximum fabric logical port number, numbering from 0. */
#define FM10000_MAX_FABRIC_LOG_PORT     47

/* The maximum fabric physical port number, numbering from 0. */
#define FM10000_MAX_FABRIC_PHYS_PORT    63

/* The maximum number of ports per EPL */
#define FM10000_PORTS_PER_EPL             4

/* The maximum port number, numbering from 0 as the first port. */
#define FM10000_MAX_PORT                FM10000_MAX_FABRIC_LOG_PORT

/** The number of ports.
 *  \ingroup constSystem */
#define FM10000_NUM_PORTS               (FM10000_MAX_PORT + 1)

/** The number of fabric ports.
 *  \ingroup constSystem */
#define FM10000_NUM_FABRIC_PORTS        (FM10000_MAX_FABRIC_PHYS_PORT + 1)

/* The number of quad port channels */
#define FM10000_NUM_QPC                 (FM10000_NUM_FABRIC_PORTS / 4)

/* EBI */
#define FM10000_NUM_MGMT_PORTS          1

/** The maximum number of entries in the MAC Address Table for FM3000 and
 *  FM10000 devices.
 *  \ingroup constSystem */
#define FM10000_MAX_ADDR                16384

/* The number of words in a MAC Address Table entry. */
#define FM10000_MA_TABLE_ENTRY_SIZE     3

/* The number of banks in the MAC Address Table. */
#define FM10000_MAC_ADDR_BANK_COUNT     4

/* The size of each bank in the MAC Address Table. */
#define FM10000_MAC_ADDR_BANK_SIZE      4096

/* The maximum number of VLANS */
#define FM10000_MAX_VLAN                4096

/** The maximum number of STP instances for FM10000 devices.
 *  \ingroup constSystem */
#define FM10000_MAX_STP_INSTANCE        256

/* The maximum number of hw triggers */
#define FM10000_MAX_HW_TRIGGERS         64

/* The maximum number of mirrors group */
#define FM10000_MAX_MIRRORS_GRP         (FM10000_MAX_HW_TRIGGERS / 2)

/* The maximum number of hw rate limiters */
#define FM10000_MAX_HW_RATELIMITERS     16

/* The maximum number of physical ports that can be in a single LAG group */
#define FM10000_MAX_PHYS_PORTS_PER_LAG  16

/* The value of the highest numbered VLAN counter supported */
#define FM10000_MAX_VLAN_COUNTER        63

/* The total number of hardware FFU slices and their size */
#define FM10000_MAX_FFU_SLICES          32
#define FM10000_FFU_ENTRIES_PER_SLICE   1024

/* The case field is an arbitrary 4 bits for FM10xxx so this constant is
 * not used. */
#define FM10000_FFU_NUM_CASES           1

/* The size and hash limit for next hops for FM10xxx */
#define FM10000_ARP_TABLE_SIZE          16384

/** The maximum allowed size for variable sized ECMP groups.
 *  
 * Note that the size of fixed size ECMP groups is
 * given by:
 *    sizeFixedEcmpGroup = 1 << n;
 * where (1 <= n <= 12). 
 * Possible values are: 
 * (2,4,8,16,32,64,128,256,512,1024,2048,4096).
 * \ingroup  constSystem  */
#define FM10000_MAX_ECMP_GROUP_SIZE     16

/* The maximum number of routers supported. */
#define FM10000_MAX_VIRTUAL_ROUTERS     14

#define FM10000_MAX_TRAFFIC_CLASS       8
#define FM10000_MAX_SHMEM_PARTITION     2

/* The maximum number of ports (logical and physical) to allocate for a
 * switch.  Note that this number may be greater than the total of the
 * actual physical ports and the actual physical LAGs supported by the
 * chip. */
#define FM10000_MAX_PORTS_ALL           FM_MAX_LOGICAL_PORT 

/* The maximum number of supported traffic classes. */
#define FM10000_MAX_TRAFFIC_CLASSES     FM10000_MAX_TRAFFIC_CLASS

/** The maximum number of available memory segments in an FM10000 device.
 *  \ingroup constSystem */
#define FM10000_MAX_MEMORY_SEGMENTS     24576

/** The size of an FM10000 segment in bytes. */
#define FM10000_SEGMENT_SIZE            192

/* The maximum number of switch priorities.  */
#define FM10000_MAX_SWITCH_PRIORITIES   16

/* The maximum number of switch memory partitions.  */
#define FM10000_MAX_SWITCH_SMPS         2

/* The maximum number of hardware CRMs. */
#define FM10000_MAX_HW_CRM              64

/* The number of policer banks. */
#define FM10000_MAX_POLICER_BANKS       4

/* Ethernet Reference Clock (ETH_REFCLK). */
#define FM10000_ETH_REF_CLOCK_FREQ      156250000

/* The maximum frame size suppported in bytes (15KB). */
#define FM10000_MAX_FRAME_SIZE          15368

/** Maximum number of Virtual Network Tunnels supported by FM10000
 *  hardware. */
#define FM10000_MAX_VN_TUNNELS          FM10000_TE_LOOKUP_ENTRIES_0

/* The number of PEPs. */
#define FM10000_MAX_PEP                 8
#define FM10000_NUM_PEPS                (FM10000_MAX_PEP + 1)
#define FM10000_MAX_DUAL_PEP            7
#define FM10000_NUM_DUAL_PEPS           (FM10000_MAX_DUAL_PEP + 1)
#define FM10000_NUM_SINGLE_PEPS         1
#define FM10000_SERDES_PER_DUAL_PEP     4
#define FM10000_SERDES_PER_SINGLE_PEP   1

/* There are 69 SerDes total: 9*4=36 EPL Serdes + 8*4+1=33 PCIE SerDes */
#define FM10000_NUM_SERDES              ( FM10000_NUM_EPLS             *  \
                                          FM10000_PORTS_PER_EPL        +  \
                                          FM10000_NUM_DUAL_PEPS        *  \
                                          FM10000_SERDES_PER_DUAL_PEP  +  \
                                          FM10000_NUM_SINGLE_PEPS      *  \
                                          FM10000_SERDES_PER_SINGLE_PEP )

/** Number of Tunnel Engines (TEs). */
#define FM10000_NUM_TUNNEL_ENGINES      2

/*****************************************************************
 * Chip Revisions
 *****************************************************************/
/* per FM10000_CHIP_VERSION register */
#define FM10000_CHIP_VERSION_A0         0
#define FM10000_CHIP_VERSION_B0         1

/*****************************************************************
 * PCIE Register Access indexing macros
 *****************************************************************/

/* Calculate PCIE PF register offset for specified PEP
 * (register addresses in fm10000_api_regs_int.h are defined for PEP 0) */
#define FM10000_PCIE_PF_ADDR(regAddr, pepId)     \
    ((regAddr) + ((pepId) * FM10000_PCIE_PF_SIZE))

/* Calculate PCIE PF global register address from local PF address 
 * of specified PEP */
#define FM10000_PCIE_PF_GLOBAL_ADDR(pfAddr, pepId) \
    (FM10000_PCIE_PF_BASE + ((pepId) * FM10000_PCIE_PF_SIZE) + (pfAddr))

/* Calculate PCIE PF local register address from its global address */
#define FM10000_PCIE_PF_LOCAL_ADDR(glAddr)  ((glAddr) % FM10000_PCIE_PF_SIZE)

/* Calculate PEP ID from PCIE PF global register address */
#define FM10000_PEP_ID(glAddr) \
    (((glAddr) - FM10000_PCIE_PF_BASE) / FM10000_PCIE_PF_SIZE)



/*****************************************************************
 * PCIE-specific registers not defined in fm10000_api_regs_int.h
 *****************************************************************/

#define FM10000_GET_PL_ADDR(regName, pepId)                                 \
    FM10000_PCIE_PF_ADDR(FM10000_PCIE_PORTLOGIC() +                         \
                         regName,                                           \
                         pepId)

#define FM10000_PCIE_PL_REG_010()                               (0x010/4)
#define FM10000_PCIE_PL_REG_010_l_fieldA                        16
#define FM10000_PCIE_PL_REG_010_h_fieldA                        21

#define FM10000_PCIE_PL_REG_028()                               (0x028/4)
#define FM10000_PCIE_PL_REG_028_h_fieldA                        5                                                 
#define FM10000_PCIE_PL_REG_028_l_fieldA                        0

#define FM10000_PCIE_PL_REG_02C()                               (0x02C/4)
#define FM10000_PCIE_PL_REG_02C_b_fieldA                        0

#define FM10000_PCIE_PL_REG_10C()                               (0x10C/4)
#define FM10000_PCIE_PL_REG_10C_l_fieldA                        8
#define FM10000_PCIE_PL_REG_10C_h_fieldA                        12
#define FM10000_PCIE_PL_REG_10C_b_fieldB                        16

#define FM10000_PCIE_PL_REG_194()                               (0x194/4)
#define FM10000_PCIE_PL_REG_194_l_fieldA                        0
#define FM10000_PCIE_PL_REG_194_h_fieldA                        5
#define FM10000_PCIE_PL_REG_194_l_fieldB                        6
#define FM10000_PCIE_PL_REG_194_h_fieldB                        11

#define FM10000_PCIE_PL_REG_198()                               (0x198/4)
#define FM10000_PCIE_PL_REG_198_l_fieldA                        0
#define FM10000_PCIE_PL_REG_198_h_fieldA                        5
#define FM10000_PCIE_PL_REG_198_l_fieldB                        6
#define FM10000_PCIE_PL_REG_198_h_fieldB                        11
#define FM10000_PCIE_PL_REG_198_l_fieldC                        12
#define FM10000_PCIE_PL_REG_198_h_fieldC                        17

#define FM10000_PCIE_PL_REG_19C()                               (0x19C/4)
#define FM10000_PCIE_PL_REG_19C_l_fieldA                        0
#define FM10000_PCIE_PL_REG_19C_h_fieldA                        3

#define FM10000_PCIE_PL_REG_1A8()                               (0x1A8/4)
#define FM10000_PCIE_PL_REG_1A8_b_fieldA                        5
#define FM10000_PCIE_PL_REG_1A8_l_fieldB                        8
#define FM10000_PCIE_PL_REG_1A8_h_fieldB                        23
#define FM10000_PCIE_PL_REG_1A8_b_fieldC                        24




/* Maximum IP Multicast entries possible  */
#define FM10000_IP_MULTICAST_TABLE_ENTRIES                   \
                                         FM10000_MOD_MCAST_VLAN_TABLE_ENTRIES
/** Highest possible multicast dest index */
#define FM10000_MAX_MCAST_DEST_INDEX          ( FM10000_SCHED_MCAST_DEST_TABLE_ENTRIES - 1 )

/** Highest possible multicast len index */
#define FM10000_MAX_MCAST_LEN_INDEX          ( FM10000_SCHED_MCAST_LEN_TABLE_ENTRIES - 1 )

/** Highest possible multicast vlan index */
#define FM10000_MAX_MCAST_VLAN_INDEX     ( FM10000_MOD_MCAST_VLAN_TABLE_ENTRIES - 1 )

/** Maximum number of multicast groups  */
#define FM10000_NUM_MCAST_GROUPS         ( FM10000_MAX_MCAST_DEST_INDEX + 1 )


/***************************************************
 * Hardware constants.
 **************************************************/

/* FABRIC Port Numbering */
#define FM10000_FIRST_EPL_FABRIC_PORT                0
#define FM10000_LAST_EPL_FABRIC_PORT                 35
#define FM10000_FIRST_PCIE_FABRIC_PORT               36
#define FM10000_LAST_PCIE_FABRIC_PORT                51
#define FM10000_PORTS_PER_PCIE_FABRIC_PORT           2
#define FM10000_FIRST_TE_FABRIC_PORT                 52
#define FM10000_LAST_TE_FABRIC_PORT                  59
#define FM10000_PORTS_PER_TE_FABRIC_PORT             4
#define FM10000_FIRST_LOOPBACK_FABRIC_PORT           61
#define FM10000_PORTS_PER_LOOPBACK_FABRIC_PORT       1

/* Converts an EPL/Lane index to fabric port */
#define FM10000_EPL_LANE_TO_FABRIC_PORT(epl, lane)                          \
    ((FM10000_PORTS_PER_EPL * epl) + lane)     
    
/* Converts a loopback port to fabric port */
#define FM10000_LOOPBACK_TO_FABRIC_PORT(port)                               \
    ((FM10000_PORTS_PER_LOOPBACK_FABRIC_PORT * port) +                      \
      FM10000_FIRST_LOOPBACK_FABRIC_PORT)
    
/* Converts a PCIE index (0-7) to fabric port */
#define FM10000_PCIE_TO_FABRIC_PORT(pcie)                                   \
    ((FM10000_PORTS_PER_PCIE_FABRIC_PORT * pcie) +                          \
      FM10000_FIRST_PCIE_FABRIC_PORT)

      
/* Special fabric ports */      
#define FM10000_FIBM_TO_FABRIC_PORT                  60
#define FM10000_PCIE8_TO_FABRIC_PORT                 63

/* Converts a Tunnel Engine (0-1) to fabric port */
#define FM10000_TE_TO_FABRIC_PORT(engine)                                   \
    ((FM10000_PORTS_PER_TE_FABRIC_PORT * engine) +                          \
      FM10000_FIRST_TE_FABRIC_PORT)



/* Spanning Tree States */
#define FM10000_STPSTATE_DISABLED       0
#define FM10000_STPSTATE_LISTENING      1
#define FM10000_STPSTATE_LEARNING       2
#define FM10000_STPSTATE_FORWARDING     3


/***************************************************
 * Hardware trap codes.
 **************************************************/
enum
{
    /* Frame trapped by an FFU rule. */
    FM10000_TRAP_FFU = 0x80,

    /* Frame with an IEEE reserved MAC address. */
    FM10000_TRAP_IEEE_MAC = 0x83,

    /* IGMP frame. */
    FM10000_TRAP_IGMP = 0x86,

    /* ICMP frame with TTL <= 1. */
    FM10000_TRAP_ICMP_TTL = 0x90,

    /* Frame with IP options. */
    FM10000_TRAP_IP_OPT = 0x91,

    /* Frame matches CPU MAC address. */
    FM10000_TRAP_CPU_MAC = 0x92,

    /* Frame with MTU too big for egress VLAN. */
    FM10000_TRAP_MTU = 0x94,

    /* Frame with TTL <= 1. */
    FM10000_TRAP_TTL = 0x96,
};




/**************************************************
 * GLOBAL_INTERRUPT_DETECT bit masks.
 **************************************************/

#define FM10000_INT_CORE                FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, CORE)
#define FM10000_INT_SOFTWARE            FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, SOFTWARE)
#define FM10000_INT_GPIO                FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, GPIO)
#define FM10000_INT_I2C                 FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, I2C)
#define FM10000_INT_MDIO                FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, MDIO)
#define FM10000_INT_CRM                 FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, CRM)
#define FM10000_INT_FH_TAIL             FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, FH_TAIL)
#define FM10000_INT_FH_HEAD             FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, FH_HEAD)
#define FM10000_INT_SBUS_EPL            FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, SBUS_EPL)
#define FM10000_INT_SBUS_PCIE           FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, SBUS_PCIE)
#define FM10000_INT_PINS                FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PINS)
#define FM10000_INT_PCIe_0              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_0)
#define FM10000_INT_PCIe_1              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_1)
#define FM10000_INT_PCIe_2              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_2)
#define FM10000_INT_PCIe_3              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_3)
#define FM10000_INT_PCIe_4              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_4)
#define FM10000_INT_PCIe_5              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_5)
#define FM10000_INT_PCIe_6              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_6)
#define FM10000_INT_PCIe_7              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_7)
#define FM10000_INT_PCIe_8              FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, PCIE_8)
#define FM10000_INT_EPL_0               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_0)
#define FM10000_INT_EPL_1               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_1)
#define FM10000_INT_EPL_2               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_2)
#define FM10000_INT_EPL_3               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_3)
#define FM10000_INT_EPL_4               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_4)
#define FM10000_INT_EPL_5               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_5)
#define FM10000_INT_EPL_6               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_6)
#define FM10000_INT_EPL_7               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_7)
#define FM10000_INT_EPL_8               FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, EPL_8)
#define FM10000_INT_TUNNEL_0            FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, TUNNEL_0)
#define FM10000_INT_TUNNEL_1            FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, TUNNEL_1)
#define FM10000_INT_FIBM                FM_MASKOF_BIT64(FM10000_INTERRUPT_MASK_INT, FIBM)


/**************************************************
 * CORE_INTERRUPT_DETECT bit masks.
 **************************************************/

#define FM10000_INT_CORE_MODIFY         FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, MODIFY)
#define FM10000_INT_CORE_FH_HEAD        FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, FH_HEAD)
#define FM10000_INT_CORE_FH_TAIL        FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, FH_TAIL)
#define FM10000_INT_CORE_SCHEDULER      FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, SCHEDULER)
#define FM10000_INT_CORE_SRAM_ERR       FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, SRAM_ERR)
#define FM10000_INT_CORE_INGRESS_ERR    FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, INGRESS_ERR)
#define FM10000_INT_CORE_EGRESS_ERR     FM_MASKOF_BIT(FM10000_CORE_INTERRUPT_DETECT, EGRESS_ERR)

#define FM10000_INT_CORE_CROSSBAR_ERR   \
    (FM10000_INT_CORE_INGRESS_ERR |     \
     FM10000_INT_CORE_EGRESS_ERR)


/**************************************************
 * MOD_IP bit masks.
 **************************************************/

#define FM10000_INT_MOD_CmTxInfoFifo_CERR       FM_MASKOF_BIT64(FM10000_MOD_IP, CmTxInfoFifo_CERR)
#define FM10000_INT_MOD_CmTxInfoFifo_UERR       FM_MASKOF_BIT64(FM10000_MOD_IP, CmTxInfoFifo_UERR)
#define FM10000_INT_MOD_Ctrl2DpFifo0_CERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo0_CERR)
#define FM10000_INT_MOD_Ctrl2DpFifo0_UERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo0_UERR)
#define FM10000_INT_MOD_Ctrl2DpFifo1_CERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo1_CERR)
#define FM10000_INT_MOD_Ctrl2DpFifo1_UERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo1_UERR)
#define FM10000_INT_MOD_Ctrl2DpFifo2_CERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo2_CERR)
#define FM10000_INT_MOD_Ctrl2DpFifo2_UERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo2_UERR)
#define FM10000_INT_MOD_Ctrl2DpFifo3_CERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo3_CERR)
#define FM10000_INT_MOD_Ctrl2DpFifo3_UERR       FM_MASKOF_BIT64(FM10000_MOD_IP, Ctrl2DpFifo3_UERR)
#define FM10000_INT_MOD_EschedTxInfoFifo_CERR   FM_MASKOF_BIT64(FM10000_MOD_IP, EschedTxInfoFifo_CERR)
#define FM10000_INT_MOD_EschedTxInfoFifo_UERR   FM_MASKOF_BIT64(FM10000_MOD_IP, EschedTxInfoFifo_UERR)
#define FM10000_INT_MOD_HeadStorage_CERR        FM_MASKOF_BIT64(FM10000_MOD_IP, HeadStorage_CERR)
#define FM10000_INT_MOD_HeadStorage_UERR        FM_MASKOF_BIT64(FM10000_MOD_IP, HeadStorage_UERR)
#define FM10000_INT_MOD_McVlanTable_CERR        FM_MASKOF_BIT64(FM10000_MOD_IP, McVlanTable_CERR)
#define FM10000_INT_MOD_McVlanTable_UERR        FM_MASKOF_BIT64(FM10000_MOD_IP, McVlanTable_UERR)
#define FM10000_INT_MOD_MgmtRdFifo_CERR         FM_MASKOF_BIT64(FM10000_MOD_IP, MgmtRdFifo_CERR)
#define FM10000_INT_MOD_MgmtRdFifo_UERR         FM_MASKOF_BIT64(FM10000_MOD_IP, MgmtRdFifo_UERR)
#define FM10000_INT_MOD_MirProfTable_CERR       FM_MASKOF_BIT64(FM10000_MOD_IP, MirProfTable_CERR)
#define FM10000_INT_MOD_MirProfTable_UERR       FM_MASKOF_BIT64(FM10000_MOD_IP, MirProfTable_UERR)
#define FM10000_INT_MOD_PerPortCfg1_CERR        FM_MASKOF_BIT64(FM10000_MOD_IP, PerPortCfg1_CERR)
#define FM10000_INT_MOD_PerPortCfg1_UERR        FM_MASKOF_BIT64(FM10000_MOD_IP, PerPortCfg1_UERR)
#define FM10000_INT_MOD_PerPortCfg2_CERR        FM_MASKOF_BIT64(FM10000_MOD_IP, PerPortCfg2_CERR)
#define FM10000_INT_MOD_PerPortCfg2_UERR        FM_MASKOF_BIT64(FM10000_MOD_IP, PerPortCfg2_UERR)
#define FM10000_INT_MOD_Refcount_CERR           FM_MASKOF_BIT64(FM10000_MOD_IP, Refcount_CERR)
#define FM10000_INT_MOD_Refcount_UERR           FM_MASKOF_BIT64(FM10000_MOD_IP, Refcount_UERR)
#define FM10000_INT_MOD_StatsByteCnt0_CERR      FM_MASKOF_BIT64(FM10000_MOD_IP, StatsByteCnt0_CERR)
#define FM10000_INT_MOD_StatsByteCnt0_UERR      FM_MASKOF_BIT64(FM10000_MOD_IP, StatsByteCnt0_UERR)
#define FM10000_INT_MOD_StatsByteCnt1_CERR      FM_MASKOF_BIT64(FM10000_MOD_IP, StatsByteCnt1_CERR)
#define FM10000_INT_MOD_StatsByteCnt1_UERR      FM_MASKOF_BIT64(FM10000_MOD_IP, StatsByteCnt1_UERR)
#define FM10000_INT_MOD_StatsFrameCnt0_CERR     FM_MASKOF_BIT64(FM10000_MOD_IP, StatsFrameCnt0_CERR)
#define FM10000_INT_MOD_StatsFrameCnt0_UERR     FM_MASKOF_BIT64(FM10000_MOD_IP, StatsFrameCnt0_UERR)
#define FM10000_INT_MOD_StatsFrameCnt1_CERR     FM_MASKOF_BIT64(FM10000_MOD_IP, StatsFrameCnt1_CERR)
#define FM10000_INT_MOD_StatsFrameCnt1_UERR     FM_MASKOF_BIT64(FM10000_MOD_IP, StatsFrameCnt1_UERR)
#define FM10000_INT_MOD_Vid2Map_CERR            FM_MASKOF_BIT64(FM10000_MOD_IP, Vid2Map_CERR)
#define FM10000_INT_MOD_Vid2Map_UERR            FM_MASKOF_BIT64(FM10000_MOD_IP, Vid2Map_UERR)
#define FM10000_INT_MOD_Vpri1Map_CERR           FM_MASKOF_BIT64(FM10000_MOD_IP, Vpri1Map_CERR)
#define FM10000_INT_MOD_Vpri1Map_UERR           FM_MASKOF_BIT64(FM10000_MOD_IP, Vpri1Map_UERR)
#define FM10000_INT_MOD_Vpri2Map_CERR           FM_MASKOF_BIT64(FM10000_MOD_IP, Vpri2Map_CERR)
#define FM10000_INT_MOD_Vpri2Map_UERR           FM_MASKOF_BIT64(FM10000_MOD_IP, Vpri2Map_UERR)
#define FM10000_INT_MOD_VtagVid1Map_CERR        FM_MASKOF_BIT64(FM10000_MOD_IP, VtagVid1Map_CERR)
#define FM10000_INT_MOD_VtagVid1Map_UERR        FM_MASKOF_BIT64(FM10000_MOD_IP, VtagVid1Map_UERR)

/* Correctable MOD_IP errors. */
#define FM10000_INT_MOD_CORRECTABLE_ERR         \
    (FM10000_INT_MOD_CmTxInfoFifo_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo0_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo1_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo2_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo3_CERR |        \
     FM10000_INT_MOD_EschedTxInfoFifo_CERR |    \
     FM10000_INT_MOD_HeadStorage_CERR |         \
     FM10000_INT_MOD_McVlanTable_CERR |         \
     FM10000_INT_MOD_MgmtRdFifo_CERR |          \
     FM10000_INT_MOD_MirProfTable_CERR |        \
     FM10000_INT_MOD_PerPortCfg1_CERR |         \
     FM10000_INT_MOD_PerPortCfg2_CERR |         \
     FM10000_INT_MOD_Refcount_CERR |            \
     FM10000_INT_MOD_StatsByteCnt0_CERR |       \
     FM10000_INT_MOD_StatsByteCnt1_CERR |       \
     FM10000_INT_MOD_StatsFrameCnt0_CERR |      \
     FM10000_INT_MOD_StatsFrameCnt1_CERR |      \
     FM10000_INT_MOD_Vid2Map_CERR |             \
     FM10000_INT_MOD_Vpri1Map_CERR |            \
     FM10000_INT_MOD_Vpri2Map_CERR |            \
     FM10000_INT_MOD_VtagVid1Map_CERR)

/* Uncorrectable MOD_IP errors. */
#define FM10000_INT_MOD_UNCORRECTABLE_ERR       \
    (FM10000_INT_MOD_CmTxInfoFifo_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo0_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo1_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo2_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo3_UERR |        \
     FM10000_INT_MOD_EschedTxInfoFifo_UERR |    \
     FM10000_INT_MOD_HeadStorage_UERR |         \
     FM10000_INT_MOD_McVlanTable_UERR |         \
     FM10000_INT_MOD_MgmtRdFifo_UERR |          \
     FM10000_INT_MOD_MirProfTable_UERR |        \
     FM10000_INT_MOD_PerPortCfg1_UERR |         \
     FM10000_INT_MOD_PerPortCfg2_UERR |         \
     FM10000_INT_MOD_Refcount_UERR |            \
     FM10000_INT_MOD_StatsByteCnt0_UERR |       \
     FM10000_INT_MOD_StatsByteCnt1_UERR |       \
     FM10000_INT_MOD_StatsFrameCnt0_UERR |      \
     FM10000_INT_MOD_StatsFrameCnt1_UERR |      \
     FM10000_INT_MOD_Vid2Map_UERR |             \
     FM10000_INT_MOD_Vpri1Map_UERR |            \
     FM10000_INT_MOD_Vpri2Map_UERR |            \
     FM10000_INT_MOD_VtagVid1Map_UERR)

#define FM10000_INT_MOD_CtrlToDpFifo            \
    (FM10000_INT_MOD_Ctrl2DpFifo0_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo0_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo1_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo1_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo2_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo2_UERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo3_CERR |        \
     FM10000_INT_MOD_Ctrl2DpFifo3_UERR)

#define FM10000_INT_MOD_StatsByteCnt            \
    (FM10000_INT_MOD_StatsByteCnt0_CERR |       \
     FM10000_INT_MOD_StatsByteCnt0_UERR |       \
     FM10000_INT_MOD_StatsByteCnt1_CERR |       \
     FM10000_INT_MOD_StatsByteCnt1_UERR)

#define FM10000_INT_MOD_StatsFrameCnt           \
    (FM10000_INT_MOD_StatsFrameCnt0_CERR |      \
     FM10000_INT_MOD_StatsFrameCnt0_UERR |      \
     FM10000_INT_MOD_StatsFrameCnt1_CERR |      \
     FM10000_INT_MOD_StatsFrameCnt1_UERR)


/**************************************************
 * SCHED_IP bit masks.
 **************************************************/

#define FM10000_INT_SCHED_SchedConfigSramErr    FM_MASKOF_FIELD(FM10000_SCHED_IP, SchedConfigSramErr)
#define FM10000_INT_SCHED_FreelistSramErr       FM_MASKOF_FIELD(FM10000_SCHED_IP, FreelistSramErr)
#define FM10000_INT_SCHED_SschedSramErr         FM_MASKOF_FIELD(FM10000_SCHED_IP, SschedSramErr)
#define FM10000_INT_SCHED_MonitorSramErr        FM_MASKOF_FIELD(FM10000_SCHED_IP, MonitorSramErr)
#define FM10000_INT_SCHED_EschedSramErr         FM_MASKOF_FIELD(FM10000_SCHED_IP, EschedSramErr)
#define FM10000_INT_SCHED_TxqSramErr            FM_MASKOF_FIELD(FM10000_SCHED_IP, TxqSramErr)
#define FM10000_INT_SCHED_RxqMcastSramErr       FM_MASKOF_FIELD(FM10000_SCHED_IP, RxqMcastSramErr)
#define FM10000_INT_SCHED_FifoSramErr           FM_MASKOF_FIELD(FM10000_SCHED_IP, FifoSramErr)

#define FM10000_INT_SCHED_PARITY_ERR            \
    (FM10000_INT_SCHED_SchedConfigSramErr |     \
     FM10000_INT_SCHED_FreelistSramErr |        \
     FM10000_INT_SCHED_SschedSramErr |          \
     FM10000_INT_SCHED_MonitorSramErr |         \
     FM10000_INT_SCHED_EschedSramErr |          \
     FM10000_INT_SCHED_TxqSramErr |             \
     FM10000_INT_SCHED_RxqMcastSramErr |        \
     FM10000_INT_SCHED_FifoSramErr)

#define FM10000_INT_SCHED_EschedSram_CERR       (1 << FM10000_SCHED_IP_l_EschedSramErr)
#define FM10000_INT_SCHED_EschedSram_UERR       (1 << FM10000_SCHED_IP_h_EschedSramErr)
#define FM10000_INT_SCHED_FifoSram_CERR         (1 << FM10000_SCHED_IP_l_FifoSramErr)
#define FM10000_INT_SCHED_FifoSram_UERR         (1 << FM10000_SCHED_IP_h_FifoSramErr)
#define FM10000_INT_SCHED_FreelistSram_CERR     (1 << FM10000_SCHED_IP_l_FreelistSramErr)
#define FM10000_INT_SCHED_FreelistSram_UERR     (1 << FM10000_SCHED_IP_h_FreelistSramErr)
#define FM10000_INT_SCHED_MonitorSram_CERR      (1 << FM10000_SCHED_IP_l_MonitorSramErr)
#define FM10000_INT_SCHED_MonitorSram_UERR      (1 << FM10000_SCHED_IP_h_MonitorSramErr)
#define FM10000_INT_SCHED_RxqMcastSram_CERR     (1 << FM10000_SCHED_IP_l_RxqMcastSramErr)
#define FM10000_INT_SCHED_RxqMcastSram_UERR     (1 << FM10000_SCHED_IP_h_RxqMcastSramErr)
#define FM10000_INT_SCHED_SchedConfigSram_CERR  (1 << FM10000_SCHED_IP_l_SchedConfigSramErr)
#define FM10000_INT_SCHED_SchedConfigSram_UERR  (1 << FM10000_SCHED_IP_h_SchedConfigSramErr)
#define FM10000_INT_SCHED_SschedSram_CERR       (1 << FM10000_SCHED_IP_l_SschedSramErr)
#define FM10000_INT_SCHED_SschedSram_UERR       (1 << FM10000_SCHED_IP_h_SschedSramErr)
#define FM10000_INT_SCHED_TxqSram_CERR          (1 << FM10000_SCHED_IP_l_TxqSramErr)
#define FM10000_INT_SCHED_TxqSram_UERR          (1 << FM10000_SCHED_IP_h_TxqSramErr)


/**************************************************
 * SERDES_IP bit masks.
 **************************************************/

#define FM10000_INT_SERDES_TxCdcFifoUErr        FM_MASKOF_BIT(FM10000_SERDES_IP, TxCdcFifoUErr)
#define FM10000_INT_SERDES_RxCdcFifoUErr        FM_MASKOF_BIT(FM10000_SERDES_IP, RxCdcFifoUErr)

#define FM10000_INT_SERDES_PARITY_ERR           \
    (FM10000_INT_SERDES_TxCdcFifoUErr |         \
     FM10000_INT_SERDES_RxCdcFifoUErr)


/**************************************************
 * EPL_IP bit masks.
 **************************************************/

#define FM10000_INT_EPL_ErrorInterrupt          FM_MASKOF_BIT(FM10000_EPL_IP, ErrorInterrupt)


/**************************************************
 * TE_IP bit masks.
 **************************************************/

#define FM10000_INT_TE_TeSramErr                FM_MASKOF_FIELD64(FM10000_TE_IP, TeSramErr)
#define FM10000_INT_TE_TePcErr                  FM_MASKOF_BIT64(FM10000_TE_IP, TePcErr)

#define FM10000_INT_TE_PARITY_ERR \
        (FM10000_INT_TE_TeSramErr)


/**************************************************
 * FH_HEAD_IP bit masks.
 **************************************************/

#define FM10000_INT_FH_HEAD_ParserEarlySramErr      FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, ParserEarlySramErr)
#define FM10000_INT_FH_HEAD_ParserLateSramErr       FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, ParserLateSramErr)
#define FM10000_INT_FH_HEAD_MapperSramErr           FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, MapperSramErr)
#define FM10000_INT_FH_HEAD_FFUSramErr_0            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_0)
#define FM10000_INT_FH_HEAD_FFUSramErr_1            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_1)
#define FM10000_INT_FH_HEAD_FFUSramErr_2            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_2)
#define FM10000_INT_FH_HEAD_FFUSramErr_3            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_3)
#define FM10000_INT_FH_HEAD_FFUSramErr_4            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_4)
#define FM10000_INT_FH_HEAD_FFUSramErr_5            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_5)
#define FM10000_INT_FH_HEAD_FFUSramErr_6            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_6)
#define FM10000_INT_FH_HEAD_FFUSramErr_7            FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr_7)
#define FM10000_INT_FH_HEAD_ArpSramErr              FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, ArpSramErr)
#define FM10000_INT_FH_HEAD_VlanSramErr             FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, VlanSramErr)
#define FM10000_INT_FH_HEAD_MaTableSramErr_0        FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, MaTableSramErr_0)
#define FM10000_INT_FH_HEAD_MaTableSramErr_1        FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, MaTableSramErr_1)
#define FM10000_INT_FH_HEAD_FGLSramErr              FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FGLSramErr)
#define FM10000_INT_FH_HEAD_GlortRamSramErr         FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, GlortRamSramErr)
#define FM10000_INT_FH_HEAD_GlortTableSramErr       FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, GlortTableSramErr)
#define FM10000_INT_FH_HEAD_FhHeadOutputFifoSramErr FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FhHeadOutputFifoSramErr)
#define FM10000_INT_FH_HEAD_FFUSramErr              FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, FFUSramErr)
#define FM10000_INT_FH_HEAD_MaTableSramErr          FM_MASKOF_FIELD64(FM10000_FH_HEAD_IP, MaTableSramErr)

#define FM10000_INT_FH_HEAD_PARITY_ERR              \
    (FM10000_INT_FH_HEAD_ParserEarlySramErr |       \
     FM10000_INT_FH_HEAD_ParserLateSramErr |        \
     FM10000_INT_FH_HEAD_MapperSramErr |            \
     FM10000_INT_FH_HEAD_FFUSramErr |               \
     FM10000_INT_FH_HEAD_ArpSramErr |               \
     FM10000_INT_FH_HEAD_VlanSramErr |              \
     FM10000_INT_FH_HEAD_MaTableSramErr |           \
     FM10000_INT_FH_HEAD_FGLSramErr |               \
     FM10000_INT_FH_HEAD_GlortRamSramErr |          \
     FM10000_INT_FH_HEAD_GlortTableSramErr |        \
     FM10000_INT_FH_HEAD_FhHeadOutputFifoSramErr)


/**************************************************
 * FH_TAIL_IP bit masks.
 **************************************************/

#define FM10000_INT_FH_TAIL_SafSramErr              FM_MASKOF_FIELD(FM10000_FH_TAIL_IP, SafSramErr)
#define FM10000_INT_FH_TAIL_EgressPauseSramErr      FM_MASKOF_FIELD(FM10000_FH_TAIL_IP, EgressPauseSramErr)
#define FM10000_INT_FH_TAIL_RxStatsSramErr          FM_MASKOF_FIELD(FM10000_FH_TAIL_IP, RxStatsSramErr)
#define FM10000_INT_FH_TAIL_PolicerUsageSramErr     FM_MASKOF_FIELD(FM10000_FH_TAIL_IP, PolicerUsageSramErr)
#define FM10000_INT_FH_TAIL_TcnSramErr              FM_MASKOF_FIELD(FM10000_FH_TAIL_IP, TcnSramErr)
#define FM10000_INT_FH_TAIL_TCN                     FM_MASKOF_BIT(FM10000_FH_TAIL_IP, TCN)

#define FM10000_INT_FH_TAIL_PARITY_ERR          \
    (FM10000_INT_FH_TAIL_SafSramErr |           \
     FM10000_INT_FH_TAIL_EgressPauseSramErr |   \
     FM10000_INT_FH_TAIL_RxStatsSramErr |       \
     FM10000_INT_FH_TAIL_PolicerUsageSramErr |  \
     FM10000_INT_FH_TAIL_TcnSramErr)


/**************************************************
 * PCIE_IP bit masks.
 **************************************************/

#define FM10000_INT_PCIE_IP_HOT_RESET               (1 << FM10000_PCIE_IP_b_HotReset)
#define FM10000_INT_PCIE_IP_DEVICE_STATE_CHANGE     (1 << FM10000_PCIE_IP_b_DeviceStateChange)
#define FM10000_INT_PCIE_IP_MAILBOX                 (1 << FM10000_PCIE_IP_b_Mailbox)
#define FM10000_INT_PCIE_IP_VPD_REQUEST             (1 << FM10000_PCIE_IP_b_VPD_Request)
#define FM10000_INT_PCIE_IP_SRAM_ERROR              (1 << FM10000_PCIE_IP_b_SramError)
#define FM10000_INT_PCIE_IP_PFLR                    (1 << FM10000_PCIE_IP_b_PFLR)
#define FM10000_INT_PCIE_IP_DATA_PATH_RESET         (1 << FM10000_PCIE_IP_b_DataPathReset)
#define FM10000_INT_PCIE_IP_OUT_OF_RESET            (1 << FM10000_PCIE_IP_b_OutOfReset)
#define FM10000_INT_PCIE_IP_NOT_IN_RESET            (1 << FM10000_PCIE_IP_b_NotInReset)
#define FM10000_INT_PCIE_IP_TIMEOUT                 (1 << FM10000_PCIE_IP_b_Timeout)
#define FM10000_INT_PCIE_IP_VFLR                    (1 << FM10000_PCIE_IP_b_VFLR)

/*
 * Mailbox control header registers.
 * Header is written in 32-bit word.
 */ 
#define FM10000_MAILBOX_SM_CONTROL_HEADER()         \
    FM10000_PCIE_MBMEM(FM10000_MAILBOX_SM_MIN_QUEUE_INDEX)

#define FM10000_MAILBOX_PF_CONTROL_HEADER()         \
    FM10000_PCIE_MBMEM(FM10000_MAILBOX_PF_MIN_QUEUE_INDEX)

/* Default hardware value of Step of SYSTIME_CFG */
#define FM10000_SYSTIME_CFG_DEFAULT_STEP        10

/* SKU fields in FUSE_DATA_0 register */
#define FM10000_FUSE_DATA_0_l_Sku               11
#define FM10000_FUSE_DATA_0_h_Sku               15

#endif /* __FM_FM10000_API_HW_INT_H */
