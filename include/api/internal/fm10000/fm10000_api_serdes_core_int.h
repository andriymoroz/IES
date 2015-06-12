/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_serdes_core_int.h
 * Creation Date:   October 22, 2014
 * Description:     low level serdes macros and declarations.
 *
 * Copyright (c) 2007 - 2014, Intel Corporation
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

#ifndef FM10000_API_SERDES_CORE_H
#define FM10000_API_SERDES_CORE_H

 
 
#define FM10000_SERDES_REG_00               0x0
#define FM10000_SERDES_REG_00_b_BIT_30          30
#define FM10000_SERDES_REG_01               0x1
#define FM10000_SERDES_REG_02               0x2
#define FM10000_SERDES_REG_03               0x3
#define FM10000_SERDES_REG_04               0x4
#define FM10000_SERDES_REG_04_b_BIT_16          16
#define FM10000_SERDES_REG_04_b_BIT_17          17
#define FM10000_SERDES_REG_07               0x7
#define FM10000_SERDES_REG_07_b_BIT_0           0
#define FM10000_SERDES_REG_07_b_BIT_1           1
#define FM10000_SERDES_REG_07_b_BIT_4           4
#define FM10000_SERDES_REG_08               0x8
#define FM10000_SERDES_REG_08_b_BIT_4           4
#define FM10000_SERDES_REG_08_b_BIT_5           5
#define FM10000_SERDES_REG_09               0x9
#define FM10000_SERDES_REG_09_b_BIT_0           0
#define FM10000_SERDES_REG_0A               0x0A
#define FM10000_SERDES_REG_0B               0x0B
#define FM10000_SERDES_REG_0B_b_BIT_18          18
#define FM10000_SERDES_REG_0B_b_BIT_19          19
#define FM10000_SERDES_REG_0C               0x0C
#define FM10000_SERDES_REG_25               0x25
#define FM10000_SERDES_REG_FD               0xFD
#define FM10000_SERDES_REG_FF               0xFF


 
 
#define FM10000_SPICO_SERDES_INTR_0X00                      0x0
#define FM10000_SPICO_SERDES_INTR_0X00_l_FIELD_1               0
#define FM10000_SPICO_SERDES_INTR_0X00_h_FIELD_1               15

#define FM10000_SPICO_SERDES_INTR_0X01                      0x1
#define FM10000_SPICO_SERDES_INTR_0X01_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X01_b_BIT_1                  1
#define FM10000_SPICO_SERDES_INTR_0X01_b_BIT_2                  2

#define FM10000_SPICO_SERDES_INTR_0X02                      0x2
#define FM10000_SPICO_SERDES_INTR_0X02_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X02_h_FIELD_1                2
#define FM10000_SPICO_SERDES_INTR_0X02_b_BIT_4                  4
#define FM10000_SPICO_SERDES_INTR_0X02_b_BIT_5                  5
#define FM10000_SPICO_SERDES_INTR_0X02_b_BIT_7                  7
#define FM10000_SPICO_SERDES_INTR_0X02_b_BIT_8                  8
#define FM10000_SPICO_SERDES_INTR_0X02_b_BIT_9                  9
#define FM10000_SPICO_SERDES_INTR_0X02_l_FIELD_2                0
#define FM10000_SPICO_SERDES_INTR_0X02_h_FIELD_2                9

#define FM10000_SPICO_SERDES_INTR_0X03                      0x3
#define FM10000_SPICO_SERDES_INTR_0X03_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X03_b_BIT_1                  1
#define FM10000_SPICO_SERDES_INTR_0X03_b_BIT_2                  2
#define FM10000_SPICO_SERDES_INTR_0X03_b_BIT_3                  3
#define FM10000_SPICO_SERDES_INTR_0X03_l_FIELD_1                4
#define FM10000_SPICO_SERDES_INTR_0X03_h_FIELD_1                6
#define FM10000_SPICO_SERDES_INTR_0X03_l_FIELD_2                8
#define FM10000_SPICO_SERDES_INTR_0X03_h_FIELD_2                10
#define FM10000_SPICO_SERDES_INTR_0X03_l_FIELD_2B               8
#define FM10000_SPICO_SERDES_INTR_0X03_h_FIELD_2B               9

#define FM10000_SPICO_SERDES_INTR_0X04                      0x4
#define FM10000_SPICO_SERDES_INTR_0X04_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X04_h_FIELD_1                2
#define FM10000_SPICO_SERDES_INTR_0X04_b_BIT_3                  3
#define FM10000_SPICO_SERDES_INTR_0X04_b_BIT_4                  4
#define FM10000_SPICO_SERDES_INTR_0X04_b_BIT_5                  5
#define FM10000_SPICO_SERDES_INTR_0X04_b_BIT_6                  6
#define FM10000_SPICO_SERDES_INTR_0X04_b_BIT_7                  7
#define FM10000_SPICO_SERDES_INTR_0X04_l_FIELD_2                8
#define FM10000_SPICO_SERDES_INTR_0X04_h_FIELD_2                11
#define FM10000_SPICO_SERDES_INTR_0X04_l_FIELD_3                2
#define FM10000_SPICO_SERDES_INTR_0X04_h_FIELD_3                15

#define FM10000_SPICO_SERDES_INTR_0X05                      0x05
#define FM10000_SPICO_SERDES_INTR_0X05_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X05_h_FIELD_1                7
#define FM10000_SPICO_SERDES_INTR_0X05_l_FIELD_2                8
#define FM10000_SPICO_SERDES_INTR_0X05_h_FIELD_2                10
#define FM10000_SPICO_SERDES_INTR_0X05_b_BIT_12                 12
#define FM10000_SPICO_SERDES_INTR_0X05_b_BIT_15                 15

#define FM10000_SPICO_SERDES_INTR_0X08                      0x08
#define FM10000_SPICO_SERDES_INTR_0X08_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X08_b_BIT_1                  1
#define FM10000_SPICO_SERDES_INTR_0X08_b_BIT_4                  4
#define FM10000_SPICO_SERDES_INTR_0X08_b_BIT_8                  8
#define FM10000_SPICO_SERDES_INTR_0X08_b_BIT_9                  9

#define FM10000_SPICO_SERDES_INTR_0X0A                      0x0A
#define FM10000_SPICO_SERDES_INTR_0X0A_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X0A_b_BIT_1                  1
#define FM10000_SPICO_SERDES_INTR_0X0A_b_BIT_2                  2

#define FM10000_SPICO_SERDES_INTR_0X0B                      0x0B
#define FM10000_SPICO_SERDES_INTR_0X0B_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X0B_b_BIT_1                  1

#define FM10000_SPICO_SERDES_INTR_0X0C                      0x0C
#define FM10000_SPICO_SERDES_INTR_0X0C_l_FIELD_                 0
#define FM10000_SPICO_SERDES_INTR_0X0C_h_FIELD_1                6
#define FM10000_SPICO_SERDES_INTR_0X0C_b_BIT_7                  7
#define FM10000_SPICO_SERDES_INTR_0X0C_b_BIT_8                  8
#define FM10000_SPICO_SERDES_INTR_0X0C_b_BIT_9                  9

#define FM10000_SPICO_SERDES_INTR_0X0D                      0x0D
#define FM10000_SPICO_SERDES_INTR_0X0D_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X0D_h_FIELD_1                4
#define FM10000_SPICO_SERDES_INTR_0X0D_b_BIT_15                 15

#define FM10000_SPICO_SERDES_INTR_0X0E                      0x0E
#define FM10000_SPICO_SERDES_INTR_0X0E_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X0E_h_FIELD_1                4
#define FM10000_SPICO_SERDES_INTR_0X0E_b_BIT_15                 15

#define FM10000_SPICO_SERDES_INTR_0X11                      0x11
#define FM10000_SPICO_SERDES_INTR_0X11_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X11_b_BIT_1                  1

#define FM10000_SPICO_SERDES_INTR_0X13                      0x13

#define FM10000_SPICO_SERDES_INTR_0X14                      0x14
#define FM10000_SPICO_SERDES_INTR_0X14_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X14_h_FIELD_1                1
#define FM10000_SPICO_SERDES_INTR_0X14_l_FIELD_2                4
#define FM10000_SPICO_SERDES_INTR_0X14_h_FIELD_2                5

#define FM10000_SPICO_SERDES_INTR_0X15                      0x15
#define FM10000_SPICO_SERDES_INTR_0X15_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X15_h_FIELD_1                7
#define FM10000_SPICO_SERDES_INTR_0X15_b_BIT_8                  8
#define FM10000_SPICO_SERDES_INTR_0X15_l_FIELD_2                14
#define FM10000_SPICO_SERDES_INTR_0X15_h_FIELD_2                15

#define FM10000_SPICO_SERDES_INTR_0X17                      0x17

#define FM10000_SPICO_SERDES_INTR_0X18                      0x18

#define FM10000_SPICO_SERDES_INTR_0X19                      0x19

#define FM10000_SPICO_SERDES_INTR_0X1A                      0x1A

#define FM10000_SPICO_SERDES_INTR_0X1B                      0x1B

#define FM10000_SPICO_SERDES_INTR_0X20                      0x20

#define FM10000_SPICO_SERDES_INTR_0X26                      0x26

#define FM10000_SPICO_SERDES_INTR_0X27                      0x27
#define FM10000_SPICO_SERDES_INTR_0X27_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X27_b_BIT_1                  1

#define FM10000_SPICO_SERDES_INTR_0X26_READ                 0x126

#define FM10000_SPICO_SERDES_INTR_0X2B                      0x2B
#define FM10000_SPICO_SERDES_INTR_0X2B_b_BIT_0                  0
#define FM10000_SPICO_SERDES_INTR_0X2B_b_BIT_1                  1
#define FM10000_SPICO_SERDES_INTR_0X2B_b_BIT_4                  4
#define FM10000_SPICO_SERDES_INTR_0X2B_b_BIT_5                  5

#define FM10000_SPICO_SERDES_INTR_0X30                      0x30
#define FM10000_SPICO_SERDES_INTR_0X30_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X30_h_FIELD_1                2
#define FM10000_SPICO_SERDES_INTR_0X30_l_FIELD_2                4
#define FM10000_SPICO_SERDES_INTR_0X30_h_FIELD_2                7

#define FM10000_SPICO_SERDES_INTR_0X3C                      0x3C
#define FM10000_SPICO_SERDES_INTR_0X3C_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X3C_h_FIELD_2                8

#define FM10000_SPICO_SERDES_INTR_0X3D                      0x3D
#define FM10000_SPICO_SERDES_INTR_0X3D_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X3D_h_FIELD_1                11
#define FM10000_SPICO_SERDES_INTR_0X3D_l_FIELD_2                12
#define FM10000_SPICO_SERDES_INTR_0X3D_h_FIELD_2                15

#define FM10000_SPICO_SERDES_INTR_0X3F                      0x3F
#define FM10000_SPICO_SERDES_INTR_0X3F_l_FIELD_1                0
#define FM10000_SPICO_SERDES_INTR_0X3F_h_FIELD_1                15


 
 
#define FM10000_SPICO_SBUS_INTR_0x00            0x00
#define FM10000_SPICO_SBUS_INTR_0x00_l_FIELD_1                  0
#define FM10000_SPICO_SBUS_INTR_0x00_h_FIELD_1                  15

#define FM10000_SPICO_SBUS_INTR_0x01            0x01
#define FM10000_SPICO_SBUS_INTR_0x01_l_FIELD_1                  0
#define FM10000_SPICO_SBUS_INTR_0x01_h_FIELD_1                  15

#define FM10000_SPICO_SBUS_INTR_0x02            0x02

#define FM10000_SPICO_SBUS_INTR_0x03            0x03
#define FM10000_SPICO_SBUS_INTR_0x03_l_FIELD_1                  0
#define FM10000_SPICO_SBUS_INTR_0x03_h_FIELD_1                  9


 
 

#define FM10000_SPICO_REG_01                    0x01
#define FM10000_SPICO_REG_01_b_BIT_6                        6
#define FM10000_SPICO_REG_01_b_BIT_7                        7
#define FM10000_SPICO_REG_01_b_BIT_8                        8
#define FM10000_SPICO_REG_01_b_BIT_9                        9
#define FM10000_SPICO_REG_01_b_BIT_10                       10
#define FM10000_SPICO_REG_01_b_BIT_11                       11
#define FM10000_SPICO_REG_02                    0x02
#define FM10000_SPICO_REG_03                    0x03
#define FM10000_SPICO_REG_04                    0x04
#define FM10000_SPICO_REG_05                    0x05
#define FM10000_SPICO_REG_05_b_BIT_0                        0
#define FM10000_SPICO_REG_06                    0x06
#define FM10000_SPICO_REG_07                    0x07
#define FM10000_SPICO_REG_07_b_BIT_0                        0
#define FM10000_SPICO_REG_08                    0x08
#define FM10000_SPICO_REG_09                    0x09
#define FM10000_SPICO_REG_0A                    0x0A
#define FM10000_SPICO_REG_0B                    0x0B
#define FM10000_SPICO_REG_0C                    0x0C
#define FM10000_SPICO_REG_0D                    0x0D
#define FM10000_SPICO_REG_0E                    0x0E
#define FM10000_SPICO_REG_0F                    0x0F
#define FM10000_SPICO_REG_10                    0x10
#define FM10000_SPICO_REG_14                    0x14
#define FM10000_SPICO_REG_15                    0x15
#define FM10000_SPICO_REG_16                    0x16
#define FM10000_SPICO_REG_16_b_BIT_18                       18
#define FM10000_SPICO_REG_16_b_BIT_19                       19
#define FM10000_SPICO_REG_17                    0x17
#define FM10000_SPICO_REG_18                    0x18
#define FM10000_SPICO_REG_19                    0x19
#define FM10000_SPICO_REG_1A                    0x1A
#define FM10000_SPICO_REG_1B                    0x1B



 
 
#define FM10000_AVSD_LSB_ADDR_0X00D             0x00d
#define FM10000_AVSD_LSB_ADDR_0X00E             0x00e
#define FM10000_AVSD_LSB_ADDR_0X00F             0x00f
#define FM10000_AVSD_LSB_ADDR_0X021             0x021
#define FM10000_AVSD_LSB_ADDR_0X024             0x024
#define FM10000_AVSD_LSB_ADDR_0X026             0x026
#define FM10000_AVSD_LSB_ADDR_0X027             0x027
#define FM10000_AVSD_LSB_ADDR_0X029             0x029
#define FM10000_AVSD_LSB_ADDR_0X02A             0x02a
#define FM10000_AVSD_LSB_ADDR_0X02B             0x02b

 
#define FM10000_AVSD_ESB_ADDR_0X020             0x020
#define FM10000_AVSD_ESB_ADDR_0X060             0x060
#define FM10000_AVSD_ESB_ADDR_0X080             0x080
#define FM10000_AVSD_ESB_ADDR_0X211             0x211
#define FM10000_AVSD_ESB_ADDR_0X213             0x213


 
#define FM10000_SERDES_DFE_PARAM_DFE_STATUS_REG     (0x0b << 8)


#endif

