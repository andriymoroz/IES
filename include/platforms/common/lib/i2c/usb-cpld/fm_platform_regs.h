/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_regs.h
 * Creation Date:   March 8, 2012
 * Description:     Platform board/CPU specific definitions
 *
 * Copyright (c) 2012 - 2013, Intel Corporation
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

#ifndef __FM_PLATFORM_REGS_H
#define __FM_PLATFORM_REGS_H


/*****************************************************************************
 * CPU Registers
 *****************************************************************************/


/*****************************************************************************
 * CPLD/FPGA Command Registers via FT245BL
 *****************************************************************************/
#define FM_SC_CMD_CPLD_WRITE       0x0
#define FM_SC_CMD_CPLD_READ        0x1
#define FM_SC_CMD_FPGA_WRITE       0x2
#define FM_SC_CMD_FPGA_READ        0x3
#define FM_SC_CMD_FPGA_DOWNLOAD    0x4
#define FM_SC_CMD_MDIO_WRITE       0x6
#define FM_SC_CMD_MDIO_READ        0x7



/*****************************************************************************
 * CPLD Registers
 * Refer to Seacliff Trail CPLD specification documentation.
 *****************************************************************************/

#define FM_CPLD_RESET_CTRL                          0
#define FM_CPLD_RESET_CTRL_b_USB_RESET              0
#define FM_CPLD_RESET_CTRL_b_MST_RESET_REQ          1
#define FM_CPLD_RESET_CTRL_b_FPGA_RESET             2
#define FM_CPLD_RESET_CTRL_b_SV_RESET_REQ_N         3

#define FM_CPLD_SCRATCH                             1
#define FM_CPLD_SCRATCH_l_DATA                      0
#define FM_CPLD_SCRATCH_h_DATA                      7

#define FM_CPLD_VERSION                             2
#define FM_CPLD_VERSION_l_VERSION                   0
#define FM_CPLD_VERSION_h_VERSION                   7

#define FM_CPLD_FPGA_PROG_CTRL                      3
#define FM_CPLD_FPGA_PROG_CTRL_b_CONFIG_N           0
#define FM_CPLD_FPGA_PROG_CTRL_b_STATUS_N           1
#define FM_CPLD_FPGA_PROG_CTRL_b_CONF_DONE          2
#define FM_CPLD_FPGA_PROG_CTRL_b_CE_N               3

#define FM_CPLD_DEBUG                               5
#define FM_CPLD_DEBUG_b_VERBOSE                     0
#define FM_CPLD_DEBUG_b_FPGA_DOWNLOAD               1

#define FM_CPLD_CMD_CNTR_LOW                        6
#define FM_CPLD_CMD_CNTR_LOW_l_CNTR                 0
#define FM_CPLD_CMD_CNTR_LOW_h_CNTR                 7

#define FM_CPLD_CMD_CNTR_HI                         7
#define FM_CPLD_CMD_CNTR_HI_l_CNTR                  0
#define FM_CPLD_CMD_CNTR_HI_h_CNTR                  7

#define FM_CPLD_BOARD_ID                            8
#define FM_CPLD_BOARD_ID_l_ID                       0
#define FM_CPLD_BOARD_ID_h_ID                       7

#define FM_CPLD_REV_ID                              9
#define FM_CPLD_REV_ID_l_ID                         0
#define FM_CPLD_REV_ID_h_ID                         7

/*****************************************************************************
 * FPGA Registers
 * Refer to Seacliff Trail FGPA specification documentation.
 *****************************************************************************/

#define FM_FPGA_VERSION                             0
#define FM_FPGA_VERSION_l_VERSION                   0
#define FM_FPGA_VERSION_h_VERSION                   7

#define FM_FPGA_SCRATCH                             1
#define FM_FPGA_SCRATCH_l_DATA                      0
#define FM_FPGA_SCRATCH_h_DATA                      7

#define FM_FPGA_RESET_CTRL                          2
#define FM_FPGA_RESET_CTRL_b_FP_RESET               0
#define FM_FPGA_RESET_CTRL_b_FC_RESET               1
#define FM_FPGA_RESET_CTRL_b_I2C_RESET              2


/***************************************************
 * I2C registers
 ***************************************************/
#define FM_FPGA_I2C_CTRL0                           3
#define FM_FPGA_I2C_CTRL0_l_Addr                    0
#define FM_FPGA_I2C_CTRL0_h_Addr                    7

#define FM_FPGA_I2C_CTRL1                           4
#define FM_FPGA_I2C_CTRL1_b_RW_N                    0
#define FM_FPGA_I2C_CTRL1_b_I2CSMB_Mode             1
#define FM_FPGA_I2C_CTRL1_b_unused                  2
#define FM_FPGA_I2C_CTRL1_b_WR_SMB_Type             3
#define FM_FPGA_I2C_CTRL1_b_WR_RD_SMB               4
#define FM_FPGA_I2C_CTRL1_b_Enable                  5
#define FM_FPGA_I2C_CTRL1_l_Status                  6
#define FM_FPGA_I2C_CTRL1_h_Status                  7
#define FM_FPGA_I2C_CTRL1_l_GO                      6
#define FM_FPGA_I2C_CTRL1_h_GO                      7

#define FM_FPGA_I2C_LEN                             5
#define FM_FPGA_I2C_LEN_l_Length                    0
#define FM_FPGA_I2C_LEN_h_Length                    4

/* LED control, 32 registers for 64 ports */
#define FM_FPGA_LED_CTRL(idx)                      (0x20 + idx)
#define FM_FPGA_LED_CTRL_b_Mode0                    0
#define FM_FPGA_LED_CTRL_l_Ctrl0                    1
#define FM_FPGA_LED_CTRL_h_Ctrl0                    3
#define FM_FPGA_LED_CTRL_b_Mode1                    4
#define FM_FPGA_LED_CTRL_l_Ctrl1                    5
#define FM_FPGA_LED_CTRL_h_Ctrl1                    7

/* 36 registers */
#define FM_FPGA_I2C_DATA(idx)                       (0x40 + idx)


/***************************************************
 * Switch temperature registers
 ***************************************************/
/* Current switch temperature */
#define FM_FPGA_SW_TEMPERATURE                      9
#define FM_FPGA_SW_TEMPERATURE_l_C                  0
#define FM_FPGA_SW_TEMPERATURE_h_C                  7

/* Temperature value when Over Temp is triggered */
#define FM_FPGA_SW_OVER_TEMP                        10
#define FM_FPGA_SW_OVER_TEMP_l_C                    0
#define FM_FPGA_SW_OVER_TEMP_h_C                    7

#define FM_FPGA_SW_CRIT_TEMP                        17
#define FM_FPGA_SW_CRIT_TEMP_b_OT                   0
#define FM_FPGA_SW_CRIT_TEMP_b_OT_WARN              1
#define FM_FPGA_SW_CRIT_TEMP_b_FC_OT                2
#define FM_FPGA_SW_CRIT_TEMP_b_FC_OT_WARN           3

/* Temperature to shutdown switch if reached limit */
#define FM_FPGA_SW_SHUTDOWN_TEMP                    18
#define FM_FPGA_SW_SHUTDOWN_TEMP_l_C                0
#define FM_FPGA_SW_SHUTDOWN_TEMP_h_C                7


/***************************************************
 * Interrupts registers
 ***************************************************/
#define FM_FPGA_INT                                12
#define FM_FPGA_INT_b_PHY_INT_N                     0
#define FM_FPGA_INT_b_I2C_SFP0                      1
#define FM_FPGA_INT_b_I2C_SFP1                      2
#define FM_FPGA_INT_b_I2C_SFP2                      3
#define FM_FPGA_INT_b_I2C_SFP3                      4
#define FM_FPGA_INT_b_I2C_SFP4                      5
#define FM_FPGA_INT_b_I2C_SFP5                      6
#define FM_FPGA_INT_b_I2C_QSFP                      7
/* Same as above, but just one single field for some code to use */
#define FM_FPGA_INT_l_I2C                           1
#define FM_FPGA_INT_h_I2C                           7

/* FPGA Rev 0x5A and lower has these registers
 * write inversed. So write 0xFF to get 0x00 */
#define FM_FPGA_INT_MASK1                          24
#define FM_FPGA_INT_MASK1_b_I2C_SFP0                1
#define FM_FPGA_INT_MASK1_b_I2C_SFP1                2
#define FM_FPGA_INT_MASK1_b_I2C_SFP2                3
#define FM_FPGA_INT_MASK1_b_I2C_SFP3                4
#define FM_FPGA_INT_MASK1_b_I2C_SFP4                5
#define FM_FPGA_INT_MASK1_b_I2C_SFP5                6
#define FM_FPGA_INT_MASK1_b_I2C_QSFP                7

#define FM_FPGA_INT_MASK2                          25
#define FM_FPGA_INT_MASK2_b_OT_WARN                 0 /* Alta Overtemp Warning */
#define FM_FPGA_INT_MASK2_l_FC_OT                   2 /* Fan Controller Overtemp */
#define FM_FPGA_INT_MASK2_h_FC_OT                   3
#define FM_FPGA_INT_MASK2_b_PHY                     4 /* Marvell PHY */
#define FM_FPGA_INT_MASK2_b_FM6000                  5 /* Alta interrupt */
#define FM_FPGA_INT_MASK2_b_VDDS_VRM_STATUS         6
#define FM_FPGA_INT_MASK2_b_VDD_VRM_STATUS          7

#define FM_FPGA_INT_MASK3                          26
#define FM_FPGA_INT_MASK3_b_VDDS_HOT_ALERT          0
#define FM_FPGA_INT_MASK3_b_VDD_HOT_ALERT           1
#define FM_FPGA_INT_MASK3_b_VDDS_SVID_ALERT         2
#define FM_FPGA_INT_MASK3_b_VDD_SVID_ALERT          3
#define FM_FPGA_INT_MASK3_b_VDDS_VRM_FAULT          4
#define FM_FPGA_INT_MASK3_b_AVDD_VRM_FAULT          5
#define FM_FPGA_INT_MASK3_b_VDD_VRM_FAULT           6

#define FM_FPGA_INT_MASK4                          27
#define FM_FPGA_INT_MASK4_b_PSB_INT                 0
#define FM_FPGA_INT_MASK4_b_PSA_INT                 1
#define FM_FPGA_INT_MASK4_b_PSB_OKAY                2
#define FM_FPGA_INT_MASK4_b_PSA_OKAY                3


/***************************************************
 * Switch IR power controller registers
 ***************************************************/
#define FM_FPGA_FC_PS_INT                          13
#define FM_FPGA_FC_PS_INT_b_FC_FAULT_N              0
#define FM_FPGA_FC_PS_INT_b_FC_ALERT_N              1
#define FM_FPGA_FC_PS_INT_b_VDD_FAULT_N             2
#define FM_FPGA_FC_PS_INT_b_VDDS_FAULT_N            3
#define FM_FPGA_FC_PS_INT_b_AVDD_FAULT_N            4
#define FM_FPGA_FC_PS_INT_b_SVID1_ALERT_N           5
#define FM_FPGA_FC_PS_INT_b_SVID2_ALERT_N           6

#define FM_FPGA_PS_CTRL                            14
#define FM_FPGA_PS_CTRL_l_PSAB_I2C_EN               0
#define FM_FPGA_PS_CTRL_h_PSAB_I2C_EN               1
#define FM_FPGA_PS_CTRL_l_PSAB_PG_N                 2
#define FM_FPGA_PS_CTRL_h_PSAB_PG_N                 3
#define FM_FPGA_PS_CTRL_l_PSAB_PG_OK                4
#define FM_FPGA_PS_CTRL_h_PSAB_PG_OK                5
#define FM_FPGA_PS_CTRL_l_PSAB_INT_N                6
#define FM_FPGA_PS_CTRL_h_PSAB_INT_N                7

#define FM_FPGA_VRM_ON                             15
#define FM_FPGA_VRM_ON_b_VRM_OFF                    0

#define FM_FPGA_VRM_STATUS                         16
#define FM_FPGA_VRM_STATUS_b_VDD_RDY                0
#define FM_FPGA_VRM_STATUS_b_VDDS_RDY               1
#define FM_FPGA_VRM_STATUS_b_AVDD_RDY               2
#define FM_FPGA_VRM_STATUS_b_VR1_STATUS_N           3
#define FM_FPGA_VRM_STATUS_b_VR2_STATUS_N           4
#define FM_FPGA_VRM_STATUS_b_VR1HOT_ALERT_N         5
#define FM_FPGA_VRM_STATUS_b_VR2HOT_ALERT_N         6

#if 0
#define FM_FPGA_VID1                               17
#define FM_FPGA_VID1_l_VID                          0
#define FM_FPGA_VID1_h_VID                          7

#define FM_FPGA_VID2                               18
#define FM_FPGA_VID2_l_VID                          0
#define FM_FPGA_VID2_h_VID                          7
#endif

#define FM_FPGA_VR_DEBUG                           22
#define FM_FPGA_VR_DEBUG_b_VRHOT_6_1                0
#define FM_FPGA_VR_DEBUG_b_VRHOT_3_0                1
#define FM_FPGA_VR_DEBUG_b_Enable_drive             2


/***************************************************
 * GPIO registers
 * GPIO_OE registers are only valid with FPGA
 * version from 0x5b
 ***************************************************/
#define FM_FPGA_GPIO0_OE                            7
#define FM_FPGA_GPIO0_OE_b_INTR                     0 /* Can only be input */
#define FM_FPGA_GPIO0_OE_b_SPI_SCK                  3
#define FM_FPGA_GPIO0_OE_b_SPI_CS_N                 4
#define FM_FPGA_GPIO0_OE_b_SPI_MOSI                 5
#define FM_FPGA_GPIO0_OE_b_SPI_MISO                 6
#define FM_FPGA_GPIO0_OE_b_BOOTMODE0                7

#define FM_FPGA_GPIO1_OE                            8
#define FM_FPGA_GPIO1_OE_b_BOOTMODE1                0
#define FM_FPGA_GPIO1_OE_b_BOOTMODE2                1
#define FM_FPGA_GPIO1_OE_b_SPI_WP_N                 3 /* shared with I2C ADDR */
#define FM_FPGA_GPIO1_OE_l_I2C_ADDR                 3
#define FM_FPGA_GPIO1_OE_h_I2C_ADDR                 5

#define FM_FPGA_GPIO0                              19
#define FM_FPGA_GPIO0_b_INTR                        0 /* Can only be input */
#define FM_FPGA_GPIO0_b_SPI_SCK                     3
#define FM_FPGA_GPIO0_b_SPI_CS_N                    4
#define FM_FPGA_GPIO0_b_SPI_MOSI                    5
#define FM_FPGA_GPIO0_b_SPI_MISO                    6
#define FM_FPGA_GPIO0_b_BOOTMODE0                   7

#define FM_FPGA_GPIO1                              20
#define FM_FPGA_GPIO1_b_BOOTMODE1                   0
#define FM_FPGA_GPIO1_b_BOOTMODE2                   1
#define FM_FPGA_GPIO1_b_SPI_WP_N                    3 /* shared with I2C ADDR */
#define FM_FPGA_GPIO1_l_I2C_ADDR                    3
#define FM_FPGA_GPIO1_h_I2C_ADDR                    5


/***************************************************
 * Switch control registers
 ***************************************************/
#define FM_FPGA_FP_CTRL                            21
#define FM_FPGA_FP_CTRL_b_Buffer_Sel                0
#define FM_FPGA_FP_CTRL_b_INTR_N                    1
#define FM_FPGA_FP_CTRL_b_CS_N                      2
#define FM_FPGA_FP_CTRL_b_EBICLK                    3


#endif /* __FM_PLATFORM_REGS_H */
