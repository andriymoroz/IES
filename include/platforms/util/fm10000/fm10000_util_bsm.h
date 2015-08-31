/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_util_bsm.h
 * Creation Date:   April 2015
 * Description:     BSM utility shared functions.
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

#ifndef __FM_FM10000_UTIL_BSM_H
#define __FM_FM10000_UTIL_BSM_H

/* BSM_SCRATCH registers filtering masks used for dbg dump */
#define REG_MASK_BSM_LOCKS                (1 << 0)    /* registers 0..1      */
#define REG_MASK_BSM_CONFIG               (1 << 1)    /* registers 10..199   */
#define REG_MASK_BSM_INIT_STATUS          (1 << 2)    /* registers 400..409  */
#define REG_MASK_BSM_INIT_STATUS_ARCHIVE  (1 << 3)    /* registers 430..440  */
#define REG_MASK_BSM_INIT_OOR             (1 << 4)    /* registers 450..451  */

#define REG_MASK_BSM_ISR_STATUS_PCIE0     (1 << 5)    /* register 441 */
#define REG_MASK_BSM_ISR_STATUS_PCIE1     (1 << 6)    /* register 442 */
#define REG_MASK_BSM_ISR_STATUS_PCIE2     (1 << 7)    /* register 443 */
#define REG_MASK_BSM_ISR_STATUS_PCIE3     (1 << 8)    /* register 444 */
#define REG_MASK_BSM_ISR_STATUS_PCIE4     (1 << 9)    /* register 445 */
#define REG_MASK_BSM_ISR_STATUS_PCIE5     (1 << 10)   /* register 446 */
#define REG_MASK_BSM_ISR_STATUS_PCIE6     (1 << 11)   /* register 447 */
#define REG_MASK_BSM_ISR_STATUS_PCIE7     (1 << 12)   /* register 448 */
#define REG_MASK_BSM_ISR_STATUS_PCIE8     (1 << 13)   /* register 449 */
#define REG_MASK_BSM_ISR_PCIE_ENABLED     (1 << 14)   /* only enabled PCIEs */

/* all PCIE ISR registers <441..449>*/
#define REG_MASK_BSM_PCIE_ISR_STATUS      (REG_MASK_BSM_ISR_STATUS_PCIE0    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE1    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE2    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE3    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE4    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE5    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE6    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE7    | \
                                           REG_MASK_BSM_ISR_STATUS_PCIE8)
/* all init registers */
#define REG_MASK_BSM_INIT_ALL             (REG_MASK_BSM_LOCKS               | \
                                           REG_MASK_BSM_CONFIG              | \
                                           REG_MASK_BSM_INIT_STATUS         | \
                                           REG_MASK_BSM_INIT_STATUS_ARCHIVE | \
                                           REG_MASK_BSM_ISR_PCIE_ENABLED    | \
                                           REG_MASK_BSM_INIT_OOR)

/* BSM Scratch dbg services */
fm_status fm10000DbgDumpBsmScratch(fm_int                    sw,
                                   fm_registerReadUINT32Func readFunc,
                                   fm_uint32                 regMask);
fm_status fm10000DbgPollBsmStatus(fm_int                    sw,
                                  fm_registerReadUINT32Func readFunc,
                                  fm_uint32                 miliSec);
fm_status fm10000DbgPollLtssm(fm_int                    sw,
                              fm_registerReadUINT32Func readFunc,
                              fm_int                    pep, 
                              fm_uint32                 miliSec);

#endif /* __FM_FM10000_UTIL_BSM_H */
