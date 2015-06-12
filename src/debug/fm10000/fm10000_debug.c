
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug.c
 * Creation Date:   January 16, 2014
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2006 - 2014, Intel Corporation
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
#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm6000DbgDumpVid
 * \ingroup intDiagMisc
 *
 * \desc            Dumps VID table for a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpVid(int sw)
{
    int        vid;
    fm_uint64  vidEntryLow;
    fm_uint32  vidEntryHigh;
    fm_status  status;
    fm_uint    vcnt;
    fm_bool    reflect;
    fm_int     fid1, fid2;
    fm_bool    fid2_ivl;
    char       members[600];
    int        memberCount;
    int        physPort;
    int        logPort;
    fm_int     cpi;
    int        portMembership;
    fm_uint32  portTag;
    char       tempBuf[10];
    fm_bool    trapIGMP;
    fm_bool    trapARP;
    fm_switch *switchPtr;
    fm_vlanEntry     *vtentry;
    fm10000_vlanEntry vtentryExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_PRINT("%4s %19s %7s %6s %6s %8s %6s %6s %6s %s\n",
                 "Vid#", "Entry", "Reflect", "Vcnt", "FID1", "FID2_IVL",
                 "FID2", "IGMP", "ARP", "Membership/Tagging");

    for (vid = 0 ; vid < 4096 ; vid++)
    {
        vtentry    = GET_VLAN_PTR(sw, vid);

        if (!vtentry->valid)
            continue;

        FM_TAKE_L2_LOCK(sw);
        FM_MEMCPY_S(&vtentryExt,
                    sizeof(vtentryExt),
                    (fm10000_vlanEntry *) GET_VLAN_EXT(sw, vid),
                    sizeof(fm10000_vlanEntry));
        FM_DROP_L2_LOCK(sw);

        vidEntryLow = (((fm_uint64) vtentryExt.member.maskWord[1]) << 32)
                    | vtentryExt.member.maskWord[0];
        vidEntryHigh = vtentryExt.member.maskWord[2];
        vcnt = vtentryExt.statIndex;
        status = fmGetVlanAttribute(sw,
                                    vid,
                                    FM_VLAN_REFLECT,
                                    &reflect);
        /* Successful? */
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

        status = fmGetVlanAttribute(sw,
                                    vid,
                                    FM_VLAN_IGMP_TRAPPING,
                                    &trapIGMP);
        /* Successful? */
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

#if 0
        status = fmGetVlanAttribute(sw,
                                    vid,
                                    FM_VLAN_ARP_TRAPPING,
                                    &trapARP);
        /* Successful? */
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
#else
        trapARP = FALSE;
#endif

        switch (switchPtr->stpMode)
        {
            case FM_SPANNING_TREE_SHARED:
                fid1 = 0;
                break;

            case FM_SPANNING_TREE_MULTIPLE:
                status = fmFindInstanceForVlan(sw, vid, &fid1);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
                break;

            default:
                fid1 =-1;
                break;
        }

#if 0
        status = fmGetVlanAttribute(sw,
                                    vid,
                                    FM_VLAN_FID2_IVL,
                                    &fid2_ivl);
        /* Successful? */
        if ( status != FM_OK )
        {
            /* no, return the error code*/
            return status;
        }
#else
        fid2_ivl = -1;
#endif

#if 0
        status = fmGetVlanAttribute(sw,
                                    vid,
                                    FM6000_INGRESS_VID2_FID,
                                    &fid2);
        /* Successful? */
        if ( status != FM_OK )
        {
            /* no, return the error code*/
            return status;
        }
#else
        fid2 = -1;
#endif

        members[0]  = 0;
        memberCount = 0;

        /* Enumerate cardinal ports. */
        for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
        {
            /* Get logical and physical port numbers. */
            fmMapCardinalPortInternal(switchPtr, cpi, &logPort, &physPort);

            portMembership = FM_PORTMASK_GET_BIT(&vtentryExt.member, physPort);
            portTag = FM_PORTMASK_GET_BIT(&vtentryExt.tag, physPort);

            if (portMembership)
            {
                if ((memberCount > 0) && ((memberCount % 13) == 0))
                {
                    fmStringAppend(members,
                                 "\n                                      "
                                 "                                       ",
                                 sizeof(members));
                }

                FM_SNPRINTF_S(tempBuf, sizeof(tempBuf),
                              " %2d%c", logPort, (portTag) ? 'T' : ' ');
                fmStringAppend(members, tempBuf, sizeof(members));
                memberCount++;
            }
        }

        FM_LOG_PRINT("%4d %03x%016llx %7d %6d %6d %8d %6d %6s %6s %s\n",
                     vid,
                     vidEntryHigh,
                     vidEntryLow,
                     reflect,
                     vcnt,
                     fid1,
                     fid2_ivl,
                     fid2,
                     trapIGMP ? "trap" : "notrap",
                     trapARP ? "trap" : "notrap",
                     members);
    }

    return FM_OK;

}   /* end fm10000DbgDumpVid */

