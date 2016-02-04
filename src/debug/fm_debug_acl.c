/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_debug_acl.c
 * Creation Date:  April 01, 2008
 * Description:    Dump current state of ACLs in a format that can be
 *                 recompiled into a test program.
 *
 * Copyright (c) 2008 - 2013, Intel Corporation
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define MAX_NUM_SCENARIOS   8
#define MAX_NUM_CONDITIONS  32
#define MAX_NUM_ACTIONS     16

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/
 
/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/
 
static void PrintIP(FILE *dest, const char *name, const fm_ipAddr *ip)
{
    fm_uint i;
    fm_uint n;

    fprintf(dest, "        value.%s.isIPv6 = %s;\n", name,
            (ip->isIPv6 ? "TRUE" : "FALSE"));
    n = (ip->isIPv6 ? 4 : 1);

    for (i = 0 ; i < n ; i++)
    {
        fprintf(dest, "        value.%s.addr[%u] = 0x%08x;\n",
                name, i, ip->addr[i]);
    }
}

/*****************************************************************************/
/** DumpCheckDeclarationsAsC
 * \ingroup intAddr
 *
 * \desc            This function dumps the declarations of ACL attributes,
 *                  if only they are needed.
 *                  It should be called from fmDbgDumpACLsAsC only.
 *                  
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       dest points on output file.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DumpCheckDeclarationsAsC(fm_int sw, FILE  *dest)
{
    fm_status        err;
    fm_bool          keep_unused_keys;
    fm_bool          top_key_support;
    fm_aclTable      table_selection;
    fm_bool          dumped_keep_unused_keys = FALSE;
    fm_bool          dumped_top_key_support = FALSE;
    fm_bool          dumped_table_selection = FALSE;
    fm_int           acl;

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG, "sw = %d, dest = %p", sw, (void *)dest);
    
    for (err = fmGetACLFirst(sw, &acl) ;
         err == FM_OK ;
         err = fmGetACLNext(sw, acl, &acl))
    {            
        err = fmGetACLAttribute(sw, acl, FM_ACL_KEEP_UNUSED_KEYS, &keep_unused_keys);
        
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);
        
        err = fmGetACLAttribute(sw, acl, FM_ACL_TOP_KEY_SUPPORT, &top_key_support);
        
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);
        
        err = fmGetACLAttribute(sw, acl, FM_ACL_TABLE_SELECTION, &table_selection);
        
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_DEBUG, err);

        /**************************************************
         * Checking if values are default.
         **************************************************/
         
        if (keep_unused_keys != FM_DISABLED)
        {
            dumped_keep_unused_keys = TRUE;
        }
        if (top_key_support != FM_ENABLED)
        {
            dumped_top_key_support = TRUE;
        }
        if (table_selection != FM_ACL_TABLE_BEST_FIT)
        {
            dumped_table_selection = TRUE;
        }
    }
    if (dumped_keep_unused_keys == TRUE)
    {
        fprintf(dest, "        fm_bool     keep_unused_keys;\n");
    }
    if (dumped_top_key_support == TRUE)
    {
        fprintf(dest, "        fm_bool     top_key_support;\n");
    }
    if (dumped_table_selection == TRUE)
    {
        fprintf(dest, "        fm_aclTable table_selection;\n");
    }
    if (dumped_keep_unused_keys || dumped_top_key_support || dumped_table_selection)
    {
        fprintf(dest, "\n");
    }

    if (err == FM_ERR_NO_ACLS)
    {
        err = FM_OK;
    }
    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, err);

}

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmDbgDumpACLsAsC
 * \ingroup intDiagACL
 *
 * \desc            Dump the current set of ACLs to a file, as a snippet
 *                  of C code that could be used to recreate the ACLs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       fileName is the file to write the snippet to.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpACLsAsC(fm_int sw, const char *fileName)
{
    const char* scenarioList[MAX_NUM_SCENARIOS];
    const char* conditionList[MAX_NUM_CONDITIONS];
    const char* actionList[MAX_NUM_ACTIONS];
    fm_int      numScenarios;
    fm_int      numConditions;
    fm_int      numActions;
    fm_status   err;
    fm_int      aclId;
    fm_int      ruleId;
    fm_aclArguments args;
    fm_aclCondition cond;
    fm_aclValue value;
    fm_aclActionExt action;
    fm_aclParamExt param;
    FILE *      dest;
    fm_aclPortAndType portAndType;
    fm_int      i;
    fm_switch * switchPtr;
    fm_bool     keep_unused_keys;
    fm_bool     top_key_support;
    fm_aclTable table_selection;
    fm_bool     is_fm6000 = FALSE;

    /* Get chip family */
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->switchFamily == FM_SWITCH_FAMILY_FM6000)
    {
        is_fm6000 = TRUE;
    }

    dest = (fileName) ? fopen(fileName, "a") : stdout;

    if (dest == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_DEBUG,
                     "Error dumping ACLs to %s\n",
                     (fileName) ? fileName : "stdout");
        return;
    }
    else
    {
        FM_LOG_PRINT("Dumping ACLs to %s\n",
                     (fileName) ? fileName : "stdout");
    }

    fprintf(dest,
            "\n\n\n/***************   Dumping ACLs for switch %d   "
            "***************/\n\n",
            sw);
    
    fprintf(dest, "    {\n");

    if (is_fm6000)
    {
        /**************************************************
         * Declare ACL Attributes if values aren't default.
         **************************************************/

        err = DumpCheckDeclarationsAsC(sw, dest);

        if (err != FM_OK)
        {
            goto ABORT;
        }
    }
    
    
    /**************************************************
     * Process each ACL in turn.
     **************************************************/

    for (err = fmGetACLFirst(sw, &aclId) ;
         err == FM_OK ;
         err = fmGetACLNext(sw, aclId, &aclId))
    {
        err = fmGetACL(sw, aclId, &args);
        if (err != FM_OK)
        {
            goto ABORT;
        }

        /**************************************************
         * Format scenarios.
         **************************************************/

        numScenarios = 0;

        if ((args.scenarios & FM_ACL_SCENARIO_ANY_FRAME_TYPE) == FM_ACL_SCENARIO_ANY_FRAME_TYPE)
        {
            scenarioList[numScenarios++] = "FM_ACL_SCENARIO_ANY_FRAME_TYPE";
        }
        else
        {
            if (args.scenarios & FM_ACL_SCENARIO_NONIP)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_NONIP";
            }

            if (args.scenarios & FM_ACL_SCENARIO_IPv4)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_IPv4";
            }

            if (args.scenarios & FM_ACL_SCENARIO_IPv6)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_IPv6";
            }
        }

        if ((args.scenarios & FM_ACL_SCENARIO_ANY_ROUTING_TYPE) == FM_ACL_SCENARIO_ANY_ROUTING_TYPE)
        {
            scenarioList[numScenarios++] = "FM_ACL_SCENARIO_ANY_ROUTING_TYPE";
        }
        else
        {
            if (args.scenarios & FM_ACL_SCENARIO_SWITCHED)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_SWITCHED";
            }

            if (args.scenarios & FM_ACL_SCENARIO_UNICAST_ROUTED)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_UNICAST_ROUTED";
            }

            if (args.scenarios & FM_ACL_SCENARIO_MULTICAST_ROUTED)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_MULTICAST_ROUTED";
            }
        }

        if ((args.scenarios & FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE) == FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE)
        {
            scenarioList[numScenarios++] = "FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE";
        }
        else
        {
            if (args.scenarios & FM_ACL_SCENARIO_SWITCHED_GLORT)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_SWITCHED_GLORT";
            }

            if (args.scenarios & FM_ACL_SCENARIO_UCAST_ROUTED_GLORT)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_UCAST_ROUTED_GLORT";
            }

            if (args.scenarios & FM_ACL_SCENARIO_MCAST_ROUTED_GLORT)
            {
                scenarioList[numScenarios++] = "FM_ACL_SCENARIO_MCAST_ROUTED_GLORT";
            }
        }

        if (numScenarios == 0)
        {
            scenarioList[numScenarios++] = "0";
        }

        FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_DEBUG, 
                               numScenarios <= MAX_NUM_SCENARIOS, 
                               (void)err,
                               "numScenarios = %d, which is > MAX_NUM_SCENARIOS\n",
                               numScenarios);

        /**************************************************
         * Create ACL.
         **************************************************/

        fprintf(dest, "        err = fmCreateACLExt(%d, %d,\n", sw, aclId);

        for (i = 0 ; i < numScenarios ; ++i)
        {
            if (i != 0)
            {
                fprintf(dest, "     |\n");
            }
            fprintf(dest, "                             %s", scenarioList[i]);
        }
        fprintf(dest, "    ,\n");

        fprintf(dest, "                             %d);\n", args.precedence);

        fprintf(dest, "        if (err != FM_OK)\n");
        fprintf(dest, "        {\n");
        fprintf(dest, "            terminateTest(\"ERROR: Unable to create ACL %d - %%s\\n\",\n", aclId);
        fprintf(dest, "                          fmErrorMsg(err));\n");
        fprintf(dest, "        }\n\n");

        if (is_fm6000)
        {
            err = fmGetACLAttribute(sw, aclId, FM_ACL_KEEP_UNUSED_KEYS, &keep_unused_keys);
            
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_DEBUG,
                             "Error dumping ACLs attribute FM_ACL_KEEP_UNUSED_KEYS to %s\n",
                             (fileName) ? fileName : "stdout");
            }
            else
            {
                if(keep_unused_keys == FM_ENABLED)
                {
                    fprintf(dest, "        keep_unused_keys = FM_ENABLED;\n\n");
                    
                    fprintf(dest,
                            "        err = fmSetACLAttribute(%d, %d, FM_ACL_KEEP_UNUSED_KEYS, &keep_unused_keys);\n", sw, aclId);
                    fprintf(dest, "        if (err != FM_OK)\n");
                    fprintf(dest, "        {\n");
                    
                    fprintf(dest,
                            "            terminateTest(\"ERROR: Unable to set attribute - \"keep_unused_keys\" for ACL %d - %%s\\n\",\n",
                            aclId);
                    
                    fprintf(dest, "                          fmErrorMsg(err));\n");
                    fprintf(dest, "        }\n\n");
                }
            }
            
            err = fmGetACLAttribute(sw, aclId, FM_ACL_TOP_KEY_SUPPORT, &top_key_support);
            
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_DEBUG,
                             "Error dumping ACLs attribute FM_ACL_TOP_KEY_SUPPORT to %s\n",
                             (fileName) ? fileName : "stdout");
            }
            else
            {
                if(top_key_support == FM_DISABLED)
                {
                    fprintf(dest, "        top_key_support = FM_DISABLED;\n\n");
                    fprintf(dest, "        err = fmSetACLAttribute(%d, %d, FM_ACL_TOP_KEY_SUPPORT, &top_key_support);\n", sw, aclId);
                    fprintf(dest, "        if (err != FM_OK)\n");
                    fprintf(dest, "        {\n");
                    
                    fprintf(dest,
                            "            terminateTest(\"ERROR: Unable to set attribute - \"top_key_support\" for ACL %d - %%s\\n\",\n",
                            aclId);
                    
                    fprintf(dest, "                          fmErrorMsg(err));\n");
                    fprintf(dest, "        }\n\n");
                }
            }
                         
            err = fmGetACLAttribute(sw, aclId, FM_ACL_TABLE_SELECTION, &table_selection);

            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_DEBUG,
                             "Error dumping ACLs attribute FM_ACL_TABLE_SELECTION to %s\n",
                             (fileName) ? fileName : "stdout");
            }
            else
            {
                if(table_selection != FM_ACL_TABLE_BEST_FIT)
                {
                    if(table_selection == FM_ACL_TABLE_0)
                    {
                        fprintf(dest, "        table_selection = FM_ACL_TABLE_0;\n\n");
                        fprintf(dest, "        err = fmSetACLAttribute(%d, %d, FM_ACL_TABLE_SELECTION, &table_selection);\n", sw, aclId);
                        fprintf(dest, "        if (err != FM_OK)\n");
                        fprintf(dest, "        {\n");
                        
                        fprintf(dest,
                                "            terminateTest(\"ERROR: Unable to set attribute - \"table_selection\" for ACL %d - %%s\\n\",\n", 
                                aclId);
                        
                        fprintf(dest, "                          fmErrorMsg(err));\n");
                        fprintf(dest, "        }\n\n");
                    }
                    if(table_selection == FM_ACL_TABLE_1)
                    {
                        fprintf(dest, "        table_selection = FM_ACL_TABLE_1;\n\n");
                        fprintf(dest, "        err = fmSetACLAttribute(%d, %d, FM_ACL_TABLE_SELECTION, &table_selection);\n", sw, aclId);
                        fprintf(dest, "        if (err != FM_OK)\n");
                        fprintf(dest, "        {\n");
                        
                        fprintf(dest,
                                "            terminateTest(\"ERROR: Unable to set attribute - \"table_selection\" for ACL %d - %%s\\n\",\n", 
                                aclId);
                        
                        fprintf(dest, "                          fmErrorMsg(err));\n");
                        fprintf(dest, "        }\n\n");
                    }
                    if(table_selection == FM_ACL_TABLE_BST)
                    {
                        fprintf(dest, "        table_selection = FM_ACL_TABLE_BST;\n\n");
                        fprintf(dest, "        err = fmSetACLAttribute(%d, %d, FM_ACL_TABLE_SELECTION, &table_selection);\n", sw, aclId);
                        fprintf(dest, "        if (err != FM_OK)\n");
                        fprintf(dest, "        {\n");
                        
                        fprintf(dest,
                                "            terminateTest(\"ERROR: Unable to set attribute - \"table_selection\" for ACL %d - %%s\\n\",\n",
                                aclId);
                        
                        fprintf(dest, "                          fmErrorMsg(err));\n");
                        fprintf(dest, "        }\n\n");
                    }    
                }
            }
        }

        /**************************************************
         * Process each rule in the ACL.
         **************************************************/

        for (err = fmGetACLRuleFirstExt(sw, aclId, &ruleId, &cond, &value,
                                        &action, &param) ;
             err == FM_OK ;
             err = fmGetACLRuleNextExt(sw, aclId, ruleId, &ruleId, &cond, &value,
                                       &action, &param))
        {
            fm_bool srcStart = FALSE;
            fm_bool srcEnd = FALSE;
            fm_bool srcMask = FALSE;
            fm_bool dstStart = FALSE;
            fm_bool dstEnd = FALSE;
            fm_bool dstMask = FALSE;

            /**************************************************
             * Format conditions.
             **************************************************/

            numConditions = 0;

            if (cond & FM_ACL_MATCH_SRC_MAC)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SRC_MAC";
                fprintf(dest,
                        "    value.src = FM_LITERAL_U64(0x"
                        FM_FORMAT_ADDR ");\n", value.src);
                fprintf(dest,
                        "    value.srcMask = FM_LITERAL_U64(0x"
                        FM_FORMAT_ADDR ");\n", value.srcMask);
            }
            
            if (cond & FM_ACL_MATCH_DST_MAC)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_DST_MAC";
                fprintf(dest,
                        "    value.dst = FM_LITERAL_U64(0x"
                        FM_FORMAT_ADDR ");\n", value.dst);
                fprintf(dest,
                        "    value.dstMask = FM_LITERAL_U64(0x"
                        FM_FORMAT_ADDR ");\n", value.dstMask);
            }
            
            if (cond & FM_ACL_MATCH_ETHERTYPE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_ETHERTYPE";
                fprintf(dest, "        value.ethType = 0x%04x;\n", value.ethType);
                fprintf(dest, "        value.ethTypeMask = 0x%04x;\n",
                        value.ethTypeMask);
            }
            
            if (cond & FM_ACL_MATCH_VLAN)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_VLAN";
                fprintf(dest, "        value.vlanId = 0x%03x;\n", value.vlanId);
                fprintf(dest, "        value.vlanIdMask = 0x%03x;\n",
                        value.vlanIdMask);
            }
            
            if (cond & FM_ACL_MATCH_PRIORITY)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_PRIORITY";
                fprintf(dest, "        value.vlanPri = %u;\n", value.vlanPri);
                fprintf(dest, "        value.vlanPriMask = %u;\n",
                        value.vlanPriMask);
            }

            if (cond & FM_ACL_MATCH_VLAN2)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_VLAN2";
                fprintf(dest, "        value.vlanId2 = 0x%03x;\n", value.vlanId2);
                fprintf(dest, "        value.vlanId2Mask = 0x%03x;\n",
                        value.vlanId2Mask);
            }
            
            if (cond & FM_ACL_MATCH_PRIORITY2)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_PRIORITY2";
                fprintf(dest, "        value.vlanPri2 = %u;\n", value.vlanPri2);
                fprintf(dest, "        value.vlanPri2Mask = %u;\n",
                        value.vlanPri2Mask);
            }
            
            if (cond & FM_ACL_MATCH_SRC_IP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SRC_IP";
                PrintIP(dest, "srcIp", &value.srcIp);
                PrintIP(dest, "srcIpMask", &value.srcIpMask);
            }
            
            if (cond & FM_ACL_MATCH_DST_IP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_DST_IP";
                PrintIP(dest, "dstIp", &value.dstIp);
                PrintIP(dest, "dstIpMask", &value.dstIpMask);
            }
            
            if (cond & FM_ACL_MATCH_TTL)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TTL";
                fprintf(dest, "        value.ttl = %u;\n", value.ttl);
                fprintf(dest, "        value.ttlMask = %u;\n", value.ttlMask);
            }
            
            if (cond & FM_ACL_MATCH_PROTOCOL)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_PROTOCOL";
                fprintf(dest, "        value.protocol = %u;\n", value.protocol);
                fprintf(dest, "        value.protocolMask = %u;\n",
                        value.protocolMask);
            }
            
            if (cond & FM_ACL_MATCH_FLAGS)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_FLAGS";
                fprintf(dest, "        value.flags = %u;\n", value.flags);
                fprintf(dest, "        value.flagsMask = %u;\n", value.flagsMask);
            }

            if (cond & FM_ACL_MATCH_FRAG)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_FRAG";
                fprintf(dest, "        value.fragType = %u;\n", value.fragType);
            }

            if (cond & FM_ACL_MATCH_FRAME_TYPE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_FRAME_TYPE";
                fprintf(dest, "        value.frameType = %u;\n", value.frameType);
            }
            
            if (cond & FM_ACL_MATCH_DSCP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_DSCP";
                fprintf(dest, "        value.dscp = %u;\n", value.dscp);
                fprintf(dest, "        value.dscpMask = %u;\n", value.dscpMask);
            }
            
            if (cond & FM_ACL_MATCH_FLOW)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_FLOW";
                fprintf(dest, "        value.flow = 0x%05x;\n", value.flow);
                fprintf(dest, "        value.flowMask = 0x%05x;\n", value.flowMask);
            }
            
            if (cond & FM_ACL_MATCH_L4_SRC_PORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_SRC_PORT";
                srcStart = TRUE;
            }
            
            if (cond & FM_ACL_MATCH_L4_DST_PORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_DST_PORT";
                dstStart = TRUE;
            }
            
            if (cond & FM_ACL_MATCH_L4_SRC_PORT_RANGE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_SRC_PORT_RANGE";
                srcStart = TRUE;
                srcEnd = TRUE;
            }
            
            if (cond & FM_ACL_MATCH_L4_DST_PORT_RANGE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_DST_PORT_RANGE";
                dstStart = TRUE;
                dstEnd = TRUE;
            }
            
            if (cond & FM_ACL_MATCH_L4_DEEP_INSPECTION)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_DEEP_INSPECTION";
                fprintf(dest,
                        "    value.L4DeepInspection = FM_LITERAL_U64(0x%016"
                        FM_FORMAT_64 "X);\n", value.L4DeepInspection);
                fprintf(dest,
                        "    value.L4DeepInspectionMask = FM_LITERAL_U64(0x%016"
                        FM_FORMAT_64 "X);\n", value.L4DeepInspectionMask);
            }
            
            if (cond & FM_ACL_MATCH_SWITCH_PRIORITY)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SWITCH_PRIORITY";
                fprintf(dest, "        value.switchPri = %u;\n", value.switchPri);
                fprintf(dest, "        value.switchPriMask = %u;\n",
                        value.switchPriMask);
            }
            
            if (cond & FM_ACL_MATCH_TCP_FLAGS)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TCP_FLAGS";
                fprintf(dest, "        value.tcpFlags = %u;\n", value.tcpFlags);
                fprintf(dest, "        value.tcpFlagsMask = %u;\n",
                        value.tcpFlagsMask);
            }

            if (cond & FM_ACL_MATCH_L4_ENTRY)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_ENTRY";
                fprintf(dest, "        value.L4Entry = %u;\n", value.L4Entry);
                fprintf(dest, "        value.L4EntryMask = %u;\n",
                        value.L4EntryMask);
            }
            
            if (cond & FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK)
            {
                conditionList[numConditions++] = 
                    "FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK";
                srcStart = TRUE;
                srcMask = TRUE;
            }
            
            if (cond & FM_ACL_MATCH_L4_DST_PORT_WITH_MASK)
            {
                conditionList[numConditions++] = 
                    "FM_ACL_MATCH_L4_DST_PORT_WITH_MASK";
                dstStart = TRUE;
                dstMask = TRUE;
            }

            if (cond & FM_ACL_MATCH_NON_IP_PAYLOAD)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_NON_IP_PAYLOAD";

                for (i = 0 ; i < FM_MAX_ACL_NON_IP_PAYLOAD_BYTES ; ++i)
                {
                    fprintf(dest,
                            "    value.nonIPPayload[%2d] = 0x%02x;\n",
                            i,
                            value.nonIPPayload[i]);
                }

                for (i = 0 ; i < FM_MAX_ACL_NON_IP_PAYLOAD_BYTES ; ++i)
                {
                    fprintf(dest,
                            "    value.nonIPPayloadMask[%2d] = 0x%02x;\n",
                            i,
                            value.nonIPPayloadMask[i]);
                }
            }

            if (cond & FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT";

                for (i = 0 ; i < FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES ; ++i)
                {
                    fprintf(dest,
                            "    value.L4DeepInspectionExt[%2d] = 0x%02x;\n",
                            i,
                            value.L4DeepInspectionExt[i]);
                }

                for (i = 0 ; i < FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES ; ++i)
                {
                    fprintf(dest,
                            "    value.L4DeepInspectionExtMask[%2d] = 0x%02x;\n",
                            i,
                            value.L4DeepInspectionExtMask[i]);
                }
            }
            
            if (cond & FM_ACL_MATCH_INGRESS_PORT_MASK)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_INGRESS_PORT_MASK";
                fprintf(dest,
                        "    value.ingressPortMask = 0x%06x;\n",
                        value.ingressPortMask);
            }
            
            if (cond & FM_ACL_MATCH_INGRESS_PORT_SET)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_INGRESS_PORT_SET";
                fprintf(dest,
                        "    value.portSet = %u;\n",
                        value.portSet);
            }

            if (cond & FM_ACL_MATCH_SRC_PORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SRC_PORT";
                fprintf(dest, "    value.logicalPort = %d;\n", value.logicalPort);
            }

            if (cond & FM_ACL_MATCH_TOS)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TOS";
                fprintf(dest, "        value.tos = %u;\n", value.tos);
                fprintf(dest, "        value.tosMask = %u;\n", value.tosMask);
            }

            if (cond & FM_ACL_MATCH_VLAN_TAG_TYPE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_VLAN_TAG_TYPE";
                fprintf(dest, "        value.vlanTag =");
                switch (value.vlanTag)
                {
                    case FM_ACL_VLAN_TAG_TYPE_NONE:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_NONE;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_STANDARD:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_STANDARD;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_USER_A:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_USER_A;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_USER_B:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_USER_B;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_VLAN1:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_VLAN1;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_VLAN2:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_VLAN2;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN1:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN1;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN2:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN2;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_VLAN1_VLAN2:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_VLAN1_VLAN2;\n");
                        break;
                    case FM_ACL_VLAN_TAG_TYPE_VLAN2_UNTAG:
                        fprintf(dest, "     FM_ACL_VLAN_TAG_TYPE_VLAN2_UNTAG;\n");
                        break;
                }
            }

            if (cond & FM_ACL_MATCH_SOURCE_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SOURCE_MAP";
                fprintf(dest, "        value.mappedSourcePort = %u;\n",
                        value.mappedSourcePort);
                fprintf(dest, "        value.mappedSourcePortMask = %u;\n",
                        value.mappedSourcePortMask);
            }

            if (cond & FM_ACL_MATCH_PROTOCOL_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_PROTOCOL_MAP";
                fprintf(dest, "        value.mappedProtocol = %u;\n",
                        value.mappedProtocol);
                fprintf(dest, "        value.mappedProtocolMask = %u;\n",
                        value.mappedProtocolMask);
            }

            if (cond & FM_ACL_MATCH_L4_SRC_PORT_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_SRC_PORT_MAP";
                fprintf(dest, "        value.mappedL4SrcPort = %u;\n",
                        value.mappedL4SrcPort);
                fprintf(dest, "        value.mappedL4SrcPortMask = %u;\n",
                        value.mappedL4SrcPortMask);
            }

            if (cond & FM_ACL_MATCH_L4_DST_PORT_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_L4_DST_PORT_MAP";
                fprintf(dest, "        value.mappedL4DstPort = %u;\n",
                        value.mappedL4DstPort);
                fprintf(dest, "        value.mappedL4DstPortMask = %u;\n",
                        value.mappedL4DstPortMask);
            }

            if (cond & FM_ACL_MATCH_DST_MAC_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_DST_MAC_MAP";
                fprintf(dest, "        value.mappedDstMac = %u;\n",
                        value.mappedDstMac);
                fprintf(dest, "        value.mappedDstMacMask = %u;\n",
                        value.mappedDstMacMask);
            }

            if (cond & FM_ACL_MATCH_SRC_MAC_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SRC_MAC_MAP";
                fprintf(dest, "        value.mappedSrcMac = %u;\n",
                        value.mappedSrcMac);
                fprintf(dest, "        value.mappedSrcMacMask = %u;\n",
                        value.mappedSrcMacMask);
            }

            if (cond & FM_ACL_MATCH_ETH_TYPE_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_ETH_TYPE_MAP";
                fprintf(dest, "        value.mappedEthType = %u;\n",
                        value.mappedEthType);
                fprintf(dest, "        value.mappedEthTypeMask = %u;\n",
                        value.mappedEthTypeMask);
            }

            if (cond & FM_ACL_MATCH_IP_LENGTH_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_IP_LENGTH_MAP";
                fprintf(dest, "        value.mappedIpLength = %u;\n",
                        value.mappedIpLength);
                fprintf(dest, "        value.mappedIpLengthMask = %u;\n",
                        value.mappedIpLengthMask);
            }

            if (cond & FM_ACL_MATCH_DST_IP_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_DST_IP_MAP";
                fprintf(dest, "        value.mappedDstIp = %u;\n",
                        value.mappedDstIp);
                fprintf(dest, "        value.mappedDstIpMask = %u;\n",
                        value.mappedDstIpMask);
            }

            if (cond & FM_ACL_MATCH_SRC_IP_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SRC_IP_MAP";
                fprintf(dest, "        value.mappedSrcIp = %u;\n",
                        value.mappedSrcIp);
                fprintf(dest, "        value.mappedSrcIpMask = %u;\n",
                        value.mappedSrcIpMask);
            }

            if (cond & FM_ACL_MATCH_VLAN_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_VLAN_MAP";
                fprintf(dest, "        value.mappedVlanId = %u;\n",
                        value.mappedVlanId);
                fprintf(dest, "        value.mappedVlanIdMask = %u;\n",
                        value.mappedVlanIdMask);
            }

            if (cond & FM_ACL_MATCH_IP_LENGTH)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_IP_LENGTH";
                fprintf(dest, "        value.ipLength = %u;\n",
                        value.ipLength);
                fprintf(dest, "        value.ipLengthMask = %u;\n",
                        value.ipLengthMask);
            }

            if (cond & FM_ACL_MATCH_ISL_FTYPE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_ISL_FTYPE";
                fprintf(dest, "        value.fType =");
                switch (value.fType)
                {
                    case FM_ACL_ISL_FTYPE_NORMAL:
                        fprintf(dest, "     FM_ACL_ISL_FTYPE_NORMAL;\n");
                        break;

                    case FM_ACL_ISL_FTYPE_ROUTED:
                        fprintf(dest, "     FM_ACL_ISL_FTYPE_ROUTED;\n");
                        break;

                    case FM_ACL_ISL_FTYPE_SPECIAL:
                        fprintf(dest, "     FM_ACL_ISL_FTYPE_SPECIAL;\n");
                        break;
                }
            }

            if (cond & FM_ACL_MATCH_ISL_USER)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_ISL_USER";
                fprintf(dest, "        value.islUser = %u;\n",
                        value.islUser);
                fprintf(dest, "        value.islUserMask = %u;\n",
                        value.islUserMask);
            }

            if (cond & FM_ACL_MATCH_SRC_GLORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SRC_GLORT";
                fprintf(dest, "        value.srcGlort = %u;\n",
                        value.srcGlort);
                fprintf(dest, "        value.srcGlortMask = %u;\n",
                        value.srcGlortMask);
            }

            if (cond & FM_ACL_MATCH_DST_GLORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_DST_GLORT";
                fprintf(dest, "        value.dstGlort = %u;\n",
                        value.dstGlort);
                fprintf(dest, "        value.dstGlortMask = %u;\n",
                        value.dstGlortMask);
            }

            if (cond & FM_ACL_MATCH_TRILL_SRC_MAC)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TRILL_SRC_MAC";
                fprintf(dest,
                        "    value.trillSrc = FM_LITERAL_U64(0x"
                        FM_FORMAT_ADDR ");\n", value.trillSrc);
                fprintf(dest,
                        "    value.trillSrcMask = FM_LITERAL_U64(0x"
                        FM_FORMAT_ADDR ");\n", value.trillSrcMask);
            }

            if (cond & FM_ACL_MATCH_TRILL_TYPE)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TRILL_TYPE";
                fprintf(dest, "        value.trillType = %u;\n", value.trillType);
            }

            if (cond & FM_ACL_MATCH_TRILL_SRB)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TRILL_SRB";
                fprintf(dest, "        value.trillSRB = %u;\n",
                        value.trillSRB);
                fprintf(dest, "        value.trillSRBMask = %u;\n",
                        value.trillSRBMask);
            }

            if (cond & FM_ACL_MATCH_TRILL_RRB)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TRILL_RRB";
                fprintf(dest, "        value.trillRRB = %u;\n",
                        value.trillRRB);
                fprintf(dest, "        value.trillRRBMask = %u;\n",
                        value.trillRRBMask);
            }

            if (cond & FM_ACL_MATCH_TABLE1_CONDITION)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TABLE1_CONDITION";
                fprintf(dest, "        value.table1Condition = %u;\n",
                        value.table1Condition);
                fprintf(dest, "        value.table1ConditionMask = %u;\n",
                        value.table1ConditionMask);
            }

            if (cond & FM_ACL_MATCH_VLAN2_MAP)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_VLAN2_MAP";
                fprintf(dest, "        value.mappedVlanId2 = %u;\n",
                        value.mappedVlanId2);
                fprintf(dest, "        value.mappedVlanId2Mask = %u;\n",
                        value.mappedVlanId2Mask);
            }

            if (cond & FM_ACL_MATCH_TUNNEL_ID)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_TUNNEL_ID";
                fprintf(dest, "        value.tunnelId = %u;\n",
                        value.tunnelId);
                fprintf(dest, "        value.tunnelIdMask = %u;\n",
                        value.tunnelIdMask);
            }

            if (cond & FM_ACL_MATCH_SCENARIO_FLAGS)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_SCENARIO_FLAGS";
                fprintf(dest, "        value.scenarioFlags = %u;\n",
                        value.scenarioFlags);
                fprintf(dest, "        value.scenarioFlagsMask = %u;\n",
                        value.scenarioFlagsMask);
            }

            if (cond & FM_ACL_MATCH_MAP_L4_SRC_PORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_MAP_L4_SRC_PORT";
                srcStart = TRUE;
            }
            
            if (cond & FM_ACL_MATCH_MAP_L4_DST_PORT)
            {
                conditionList[numConditions++] = "FM_ACL_MATCH_MAP_L4_DST_PORT";
                dstStart = TRUE;
            }

            if (numConditions == 0)
            {
                conditionList[numConditions++] = "0";
            }

            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_DEBUG, 
                                   numConditions <= MAX_NUM_CONDITIONS, 
                                   (void)err,
                                   "numConditions = %d which is > MAX_NUM_CONDITIONS\n",
                                   numConditions);

            /**************************************************
             * Print L4 port values.
             **************************************************/

            if (srcStart)
            {
                fprintf(dest, "        value.L4SrcStart = %u;\n", value.L4SrcStart);
            }

            if (srcEnd)
            {
                fprintf(dest, "        value.L4SrcEnd = %u;\n", value.L4SrcEnd);
            }

            if (srcMask)
            {
                fprintf(dest, "        value.L4SrcMask = %u;\n", value.L4SrcMask);
            }

            if (dstStart)
            {
                fprintf(dest, "        value.L4DstStart = %u;\n", value.L4DstStart);
            }

            if (dstEnd)
            {
                fprintf(dest, "        value.L4DstEnd = %u;\n", value.L4DstEnd);
            }

            if (dstMask)
            {
                fprintf(dest, "        value.L4DstMask = %u;\n", value.L4DstMask);
            }

            /**************************************************
             * Format actions.
             **************************************************/

            numActions = 0;

            if ((action & FM_ACL_ACTIONEXT_PERMIT) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_PERMIT";
            }
            
            if ((action & FM_ACL_ACTIONEXT_DENY) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_DENY";
            }
            
            if ((action & FM_ACL_ACTIONEXT_TRAP) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_TRAP";
            }
            
            if ((action & FM_ACL_ACTIONEXT_MIRROR) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_MIRROR";
                fprintf(dest, "        param.mirrorPort = %d;\n", param.mirrorPort);
            }
            
            if ((action & FM_ACL_ACTIONEXT_LOG) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_LOG";
            }
            
            if ((action & FM_ACL_ACTIONEXT_COUNT) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_COUNT";
            }
            
            if ((action & FM_ACL_ACTIONEXT_COUNT_NOTIFY) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_COUNT_NOTIFY";
            }

            if ((action & FM_ACL_ACTIONEXT_NOROUTE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_NOROUTE";
            }

            if ((action & FM_ACL_ACTIONEXT_POLICE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_POLICE";
                fprintf(dest, "        param.policer = %d;\n", param.policer);
            }
            
            if ((action & FM_ACL_ACTIONEXT_SET_VLAN) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_VLAN";
                fprintf(dest, "        param.vlan = 0x%03x;\n", param.vlan);
            }

            if ((action & FM_ACL_ACTIONEXT_UPD_OR_ADD_VLAN) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_UPD_OR_ADD_VLAN";
                fprintf(dest, "        param.vlan = 0x%03x;\n", param.vlan);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY";
                fprintf(dest, "        param.vlanPriority = %u;\n",
                        param.vlanPriority);
            }
            
            if ((action & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY";
                fprintf(dest, "        param.switchPriority = %u;\n",
                        param.switchPriority);
            }
            
            if ((action & FM_ACL_ACTIONEXT_SET_DSCP) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_DSCP";
                fprintf(dest, "        param.dscp = %u;\n", param.dscp);
            }
            
            if ((action & FM_ACL_ACTIONEXT_SET_USER) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_USER";
                fprintf(dest, "        param.user = 0x%02x;\n", param.user);
                fprintf(dest, "        param.userMask = 0x%02x;\n", param.userMask);
            }

            if ((action & FM_ACL_ACTIONEXT_LOAD_BALANCE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_LOAD_BALANCE";
                fprintf(dest, "        param.lbgNumber = 0x%02x;\n", param.lbgNumber);
            }

            if ((action & FM_ACL_ACTIONEXT_TRAP_ALWAYS) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_TRAP_ALWAYS";
            }

            if ((action & FM_ACL_ACTIONEXT_REDIRECT) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_REDIRECT";
                fprintf(dest, "        param.logicalPort = %d;\n", param.logicalPort);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_FLOOD_DEST) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_FLOOD_DEST";
                fprintf(dest, "        param.logicalPort = %d;\n", param.logicalPort);
            }

            if ((action & FM_ACL_ACTIONEXT_ROUTE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_ROUTE";
                fprintf(dest, "        param.groupId = %d;\n", param.groupId);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_L3_HASH_PROFILE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_L3_HASH_PROFILE";
                fprintf(dest, "        param.l3HashProfile = %d;\n", param.l3HashProfile);
            }

            if ((action & FM_ACL_ACTIONEXT_MIRROR_GRP) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_MIRROR_GRP";
                fprintf(dest, "        param.mirrorGrp = %d;\n", param.mirrorGrp);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_TRIG_ID) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_TRIG_ID";
                fprintf(dest, "    param.trigId = %d;\n", param.trigId);
                fprintf(dest, "    param.trigIdMask = %d;\n", param.trigIdMask);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_VLAN2) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_VLAN2";
                fprintf(dest, "        param.vlan2 = %d;\n", param.vlan2);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY";
                fprintf(dest, "        param.vlanPriority2 = %d;\n", param.vlanPriority2);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_SRC_GLORT) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_SRC_GLORT";
                fprintf(dest, "        param.srcGlort = %d;\n", param.srcGlort);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_TRILL_DIST_TREE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_TRILL_DIST_TREE";
                fprintf(dest, "        param.distTree = %d;\n", param.distTree);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID";
                fprintf(dest, "        param.tunnelId = %d;\n", param.tunnelId);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_PRECEDENCE) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_PRECEDENCE";
                fprintf(dest, "        param.precedence = %d;\n", param.precedence);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT";
                fprintf(dest, "        param.srcGlort = %d;\n", param.srcGlort);
                fprintf(dest, "        param.tunnelId = %d;\n", param.tunnelId);
            }

            if ((action & FM_ACL_ACTIONEXT_VN_TUNNEL) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_VN_TUNNEL";
                fprintf(dest, "        param.vnTunnel = %d;\n", param.vnTunnel);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_CONDITION) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_CONDITION";
                fprintf(dest, "        param.condition = %d;\n", param.condition);
            }

            if ((action & FM_ACL_ACTIONEXT_PUSH_VLAN) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_PUSH_VLAN";
                fprintf(dest, "        param.vlan = 0x%03x;\n", param.vlan);
            }

            if ((action & FM_ACL_ACTIONEXT_POP_VLAN) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_POP_VLAN";
            }

            if ((action & FM_ACL_ACTIONEXT_SET_DMAC) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_DMAC";
                fprintf(dest, "        param.dmac = 0x%012llx;\n", param.dmac);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_SMAC) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_SMAC";
                fprintf(dest, "        param.smac = 0x%012llx;\n", param.smac);
            }

            if ((action & FM_ACL_ACTIONEXT_SET_MPLS_TC) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_SET_MPLS_TC";
                fprintf(dest, "        param.mplsTc = %d;\n", param.mplsTc);
            }

            if ((action & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_REDIRECT_TUNNEL";
                fprintf(dest, "    param.tunnelGroup = %d;\n", param.tunnelGroup);
                fprintf(dest, "    param.tunnelRule = %d;\n", param.tunnelRule);
            }

            if ((action & FM_ACL_ACTIONEXT_CAPTURE_EGRESS_TIMESTAMP) != 0)
            {
                actionList[numActions++] = "FM_ACL_ACTIONEXT_CAPTURE_EGRESS_TIMESTAMP";
                fprintf(dest, 
                        "        param.captureEgressTimestamp = %d;\n", 
                        param.captureEgressTimestamp);
            }

            if (numActions == 0)
            {
                actionList[numActions++] = "0";
            }

            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_DEBUG, 
                                   numActions <= MAX_NUM_ACTIONS, 
                                   (void)err,
                                   "numActions = %d which is > MAX_NUM_ACTIONS\n",
                                   numActions);

            /**************************************************
             * Create rule.
             **************************************************/
            fprintf(dest, "        err = fmAddACLRuleExt(%d, %d, %d,\n",
                    sw, aclId, ruleId);

            for (i = 0 ; i < numConditions ; ++i)
            {
                if (i != 0)
                {
                    fprintf(dest, "     |\n");
                }
                fprintf(dest, "                              %s", conditionList[i]);
            }
            fprintf(dest, "    ,\n");

            fprintf(dest, "                              &value,\n");

            for (i = 0 ; i < numActions ; ++i)
            {
                if (i != 0)
                {
                    fprintf(dest, "     |\n");
                }
                fprintf(dest, "                              %s", actionList[i]);
            }
            fprintf(dest, "    ,\n");

            fprintf(dest, "                              &param);\n");

            fprintf(dest, "        if (err != FM_OK)\n");
            fprintf(dest, "        {\n");
            fprintf(dest, "            terminateTest(\"ERROR: Unable to create "
                    "rule %d in ACL %d - %%s\\n\",\n", ruleId, aclId);
            fprintf(dest, "                          fmErrorMsg(err));\n");
            fprintf(dest, "        }\n\n");
        }

        /**************************************************
         * Apply ACL to ports.
         **************************************************/
        for (err = fmGetACLPortFirst(sw, aclId, &portAndType) ;
             err == FM_OK ;
             err = fmGetACLPortNext(sw, aclId, &portAndType))
        {
            fprintf(dest, "        err = fmAddACLPort(%d, %d, %d, %s);\n",
                    sw, aclId, portAndType.port,
                    (portAndType.type == FM_ACL_TYPE_EGRESS ?
                     "FM_ACL_TYPE_EGRESS" : "FM_ACL_TYPE_INGRESS"));
            
            fprintf(dest, "        if (err != FM_OK)\n");
            fprintf(dest, "        {\n");
            fprintf(dest, "            terminateTest(\"ERROR: Unable to associate "
                    "port %d with ACL %d - %%s\\n\",\n",
                    portAndType.port, aclId);
            fprintf(dest, "                          fmErrorMsg(err));\n");
            fprintf(dest, "        }\n\n");
        }
            
    }
    fprintf(dest, "    }\n\n");
ABORT:
    if (fileName)
    {
        fclose(dest);
    }

}   /* end fmDbgDumpACLsAsC */
