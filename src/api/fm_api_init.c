/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_init.c
 * Creation Date:   2005
 * Description:     Functions for initialization and switch status/information
 *                  retrieval functions for the API
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
typedef struct _fm_switchModelEntry
{
    fm_switchFamily  family;
    fm_switchModel   model;
    const fm_switch *switchTable;

} fm_switchModelEntry;


#define FM_DISPATCH_QUEUE_SIZE  70

#define MAX_BUF_SIZE                    256
#define NUM_CAT                         64

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/* The precompiled headers process requires a fake variable to be placed in
 * the internal header files for the chip families.
 * In order to prevent undefined symbols this fake variable is defined
 * here.
 */
int                              fake_variable_1;

/* We extern the switch-specific structures and identify functions so that,
 * here in generic code, we can have minimal access to the requisite
 * functions and tables.
 */
#if FM_SUPPORT_FM2000

extern const fm_switch           FM2000SwitchDefaultTable;

extern fm_status fm2000IdentifySwitch(fm_int            sw,
                                      fm_switchFamily * family,
                                      fm_switchModel *  model,
                                      fm_switchVersion *version);

extern fm_status fm2000GetSwitchPartNumber(fm_int sw, fm_switchPartNum *pn);

#endif


#if FM_SUPPORT_FM4000

extern const fm_switch           FM4000SwitchDefaultTable;

extern fm_status fm4000IdentifySwitch(fm_int            sw,
                                      fm_switchFamily * family,
                                      fm_switchModel *  model,
                                      fm_switchVersion *version);

extern fm_status fm4000GetSwitchPartNumber(fm_int sw, fm_switchPartNum *pn);

#endif


#if FM_SUPPORT_FM6000

extern const fm_switch           FM6000SwitchDefaultTable;

extern fm_status fm6000IdentifySwitch(fm_int            sw,
                                      fm_switchFamily * family,
                                      fm_switchModel *  model,
                                      fm_switchVersion *version);

extern fm_status fm6000GetSwitchPartNumber(fm_int sw, fm_switchPartNum *pn);

#endif


#if FM_SUPPORT_FM10000

extern const fm_switch           FM10000SwitchDefaultTable;

extern fm_status fm10000IdentifySwitch(fm_int            sw,
                                       fm_switchFamily * family,
                                       fm_switchModel *  model,
                                       fm_switchVersion *version);

extern fm_status fm10000GetSwitchPartNumber(fm_int sw, fm_switchPartNum *pn);

#endif


#if FM_SUPPORT_SWAG

extern const fm_switch           FMSWAGSwitchDefaultTable;

#endif


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* True iff we are the process that owns the global event handler */
static fm_bool                   fmFirstProcess = FALSE;

static const fm_switchModelEntry fmSwitchModelList[] =
{
#if FM_SUPPORT_FM2000
    {
        FM_SWITCH_FAMILY_FM2000,            /* switch family */
        FM_SWITCH_MODEL_FM2224,             /* switch model */
        &FM2000SwitchDefaultTable,          /* Default Switch table */
    },
    {
        FM_SWITCH_FAMILY_FM2000,            /* switch family */
        FM_SWITCH_MODEL_FM2112,             /* switch model */
        &FM2000SwitchDefaultTable,          /* Default Switch table */
    },
#endif

#if FM_SUPPORT_FM4000
    {
        FM_SWITCH_FAMILY_FM4000,            /* switch family */
        FM_SWITCH_MODEL_FM4224,             /* switch model */
        &FM4000SwitchDefaultTable,          /* Default Switch table */
    },
    {
        FM_SWITCH_FAMILY_REMOTE_FM4000,     /* switch type */
        FM_SWITCH_MODEL_FM4224,             /* switch model */
        &FM4000SwitchDefaultTable,          /* Default Switch Table */
    },
#endif

#if FM_SUPPORT_SWAG
    {
        FM_SWITCH_FAMILY_SWAG,              /* switch family */
        FM_SWITCH_MODEL_SWAG_A,             /* switch model */
        &FMSWAGSwitchDefaultTable,          /* Default Switch table */
    },
    {
        FM_SWITCH_FAMILY_SWAG,              /* switch family */
        FM_SWITCH_MODEL_SWAG_B,             /* switch model */
        &FMSWAGSwitchDefaultTable,          /* Default Switch table */
    },
    {
        FM_SWITCH_FAMILY_SWAG,              /* switch family */
        FM_SWITCH_MODEL_SWAG_C,             /* switch model */
        &FMSWAGSwitchDefaultTable,          /* Default Switch table */
    },
#endif

#if FM_SUPPORT_FM6000
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6224,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6364,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6348,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6324,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6764,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6748,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM6724,
        &FM6000SwitchDefaultTable,
    },
    {
        FM_SWITCH_FAMILY_FM6000,
        FM_SWITCH_MODEL_FM5224,
        &FM6000SwitchDefaultTable,
    },
#endif

#if FM_SUPPORT_FM10000
    {
        FM_SWITCH_FAMILY_FM10000,
        FM_SWITCH_MODEL_FM10440,
        &FM10000SwitchDefaultTable,
    },
#endif

    /* This entry must be last */
    {
        FM_SWITCH_FAMILY_UNKNOWN,
        FM_SWITCH_MODEL_UNKNOWN,
        NULL,
    }

};  /* end fmSwitchModelList */


static fm_status (*const IdentifySwitch[])(fm_int sw,
                                           fm_switchFamily * family,
                                           fm_switchModel * model,
                                           fm_switchVersion * version) =
{
#if FM_SUPPORT_FM2000
    fm2000IdentifySwitch,
#endif

#if FM_SUPPORT_FM4000
    fm4000IdentifySwitch,
#endif

#if FM_SUPPORT_FM6000
    fm6000IdentifySwitch,
#endif

#if FM_SUPPORT_FM10000
    fm10000IdentifySwitch,
#endif

    /* This entry must be last */
    NULL

};  /* end IdentifySwitch */


typedef struct
{
    /* String description of the value */
    fm_text desc;

    /* Value for the corresponding string */
    fm_uint64  value;

} fm_logCatMap;

static fm_logCatMap logCatMap[] =
{
    { "logging",            FM_LOG_CAT_LOGGING            },
    { "link_state",         FM_LOG_CAT_LINK_STATE         },
    { "vlan",               FM_LOG_CAT_VLAN               },
    { "vlan_stp",           FM_LOG_CAT_VLAN_STP           },
    { "alos",               FM_LOG_CAT_ALOS               },
    { "debug",              FM_LOG_CAT_DEBUG              },
    { "phy",                FM_LOG_CAT_PHY                },
    { "platform",           FM_LOG_CAT_PLATFORM           },
    { "event",              FM_LOG_CAT_EVENT              },
    { "event_pkt_tx",       FM_LOG_CAT_EVENT_PKT_TX       },
    { "event_pkt_tx",       FM_LOG_CAT_EVENT_PKT_RX       },
    { "event_mac_maint",    FM_LOG_CAT_EVENT_MAC_MAINT    },
    { "switch",             FM_LOG_CAT_SWITCH             },
    { "mailbox",            FM_LOG_CAT_MAILBOX            },
    { "alos_dllib",         FM_LOG_CAT_ALOS_DLLIB         },
    { "alos_sem",           FM_LOG_CAT_ALOS_SEM           },
    { "alos_lock",          FM_LOG_CAT_ALOS_LOCK          },
    { "alos_rwlock",        FM_LOG_CAT_ALOS_RWLOCK        },
    { "alos_thread",        FM_LOG_CAT_ALOS_THREAD        },
    { "port",               FM_LOG_CAT_PORT               },
    { "serdes",             FM_LOG_CAT_SERDES             },
    { "alos_time",          FM_LOG_CAT_ALOS_TIME          },
    { "model",              FM_LOG_CAT_MODEL              },
    { "parity",             FM_LOG_CAT_PARITY             },
    { "spare24",            FM_LOG_CAT_SPARE24            },
    { "spare25",            FM_LOG_CAT_SPARE25            },
    { "lag",                FM_LOG_CAT_LAG                },
    { "spare27",            FM_LOG_CAT_SPARE27            },
    { "spare28",            FM_LOG_CAT_SPARE28            },
    { "event_port",         FM_LOG_CAT_EVENT_PORT         },
    { "nat",                FM_LOG_CAT_NAT                },
    { "te",                 FM_LOG_CAT_TE                 },
    { "event_intr",         FM_LOG_CAT_EVENT_INTR         },
    { "state_machine",      FM_LOG_CAT_STATE_MACHINE      },
    { "rbridge",            FM_LOG_CAT_RBRIDGE            },
    { "attr",               FM_LOG_CAT_ATTR               },
    { "multicast",          FM_LOG_CAT_MULTICAST          },
    { "storm",              FM_LOG_CAT_STORM              },
    { "routing",            FM_LOG_CAT_ROUTING            },
    { "addr",               FM_LOG_CAT_ADDR               },
    { "mirror",             FM_LOG_CAT_MIRROR             },
    { "qos",                FM_LOG_CAT_QOS                },
    { "trigger",            FM_LOG_CAT_TRIGGER            },
    { "bst",                FM_LOG_CAT_BST                },
    { "map",                FM_LOG_CAT_MAP                },
    { "ffu",                FM_LOG_CAT_FFU                },
    { "acl",                FM_LOG_CAT_ACL                },
    { "stp",                FM_LOG_CAT_STP                },
    { "buffer",             FM_LOG_CAT_BUFFER             },
    { "general",            FM_LOG_CAT_GENERAL            },
    { "api",                FM_LOG_CAT_API                },
    { "lbg",                FM_LOG_CAT_LBG                },
    { "swag",               FM_LOG_CAT_SWAG               },
    { "port_autoneg",       FM_LOG_CAT_PORT_AUTONEG       },
    { "stacking",           FM_LOG_CAT_STACKING           },
    { "sflow",              FM_LOG_CAT_SFLOW              },
    { "addr_offload",       FM_LOG_CAT_ADDR_OFFLOAD       },
    { "event_fast_maint",   FM_LOG_CAT_EVENT_FAST_MAINT   },
    { "fibm",               FM_LOG_CAT_FIBM               },
    { "application",        FM_LOG_CAT_APPLICATION        },
    { "crm",                FM_LOG_CAT_CRM                },
    { "flow",               FM_LOG_CAT_FLOW               },
    { "vn",                 FM_LOG_CAT_VN                 },
    { "spare63",            FM_LOG_CAT_SPARE63            },
    
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/* GetLogCatBitMask
 * \ingroup intSwitch
 *
 * \desc            Get integer equivalent of the string given the string
 *                  mapping.
 *
 * \param[in]       name is the name to find in the string mapping.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetLogCatBitMask(fm_text    name,
                                  fm_uint64 *value)
{
    fm_uint32   cnt;
    fm_int      lenName;
    fm_int      lenDesc;

    lenName = strlen(name);

    for (cnt = 0 ; cnt < FM_NENTRIES(logCatMap) ; cnt++)
    {
        lenDesc = strlen(logCatMap[cnt].desc);
        if ( (lenName == lenDesc) &&
             ( (strncasecmp( name, logCatMap[cnt].desc, lenDesc)) == 0 ) )
        {
            *value = logCatMap[cnt].value;
            return FM_OK;
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end GetLogCatBitMask */




/*****************************************************************************/
/* LogStrToMask 
 * \ingroup intSwitch
 *
 * \desc            Convert a string of log categories to a category mask. 
 *
 * \param[in]       str is the string.
 *
 * \param[out]      mask points to caller-allocated storage where this
 *                  function should place the obtained mask
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LogStrToMask(fm_text    name,
                              fm_uint64 *mask)
{
    fm_status err = FM_OK;
    fm_int    i;
    fm_char * token;
    fm_char * tokptr;
    fm_uint   strSize;
    fm_char   tmpText[MAX_BUF_SIZE+1];
    fm_int    strLen;
    fm_uint64 bitVal;

    *mask = 0;

    strLen = FM_STRNLEN_S(name, MAX_BUF_SIZE+1);

    if (strLen > MAX_BUF_SIZE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Size of buffer (%d) is too small for input string '%s'. "
                     "Length = %d.\n",
                     MAX_BUF_SIZE,
                     name,
                     strLen);

        strLen = MAX_BUF_SIZE;
    }

    FM_MEMCPY_S(tmpText, sizeof(tmpText), name, strLen);
    tmpText[strLen] = '\0';

    /* Comma delimited values */
    strSize = MAX_BUF_SIZE;
    token   = FM_STRTOK_S(tmpText, &strSize, ",", &tokptr);

    if (token == NULL)
    {
        return FM_OK;
    }

    for (i = 0 ; i < NUM_CAT ; i++)
    {
        if (GetLogCatBitMask(token, &bitVal) == FM_OK)
        {
            *mask |= bitVal;
        }
        else
        {
            FM_LOG_PRINT("Category '%s' not found or invalid.\n", 
                         token);
        }

        token = FM_STRTOK_S(NULL, &strSize, ",", &tokptr);

        if (token == NULL)
        {
            break;
        }
    }

    return err;

}   /* end LogStrToMask */




/*****************************************************************************/
/** fmAllocateSwitchLocks
 * \ingroup intSwitch
 *
 * \desc            create switch locks
 *
 * \param[in]       swstate points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmAllocateSwitchLocks(fm_switch *swstate)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "swstate=%p (switch number %d)\n",
                 (void *) swstate,
                 swstate->switchNumber);

    /* The locks have already been initialized, do nothing. */
    if (swstate->accessLocksInitialized)
    {
        return FM_OK;
    }

    err = fmCreateLockV2("stateLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_STATE_LOCK,
                         &swstate->stateLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("L2Lock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_L2,
                         &swstate->L2Lock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    err = fmCreateLockV2("maPurgeLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_PURGE,
                         &swstate->maPurgeLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    err = fmCreateLockV2("macTableMaintLock",
                         swstate->switchNumber,
                         FM_LOCK_PREC_MATABLE_MAINT,
                         &swstate->macTableMaintWorkListLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    err = fmCreateLockV2("aclLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_ACLS,
                         &swstate->aclLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("tunnelLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_TUNNEL,
                         &swstate->tunnelLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("natLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_NAT,
                         &swstate->natLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    err = fmCreateLockV2("flowLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_FLOW,
                         &swstate->flowLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    err = fmCreateRwLockV2("RoutingLock", 
                           swstate->switchNumber,
                           FM_LOCK_PREC_ROUTING, 
                           &swstate->routingLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("portSetLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_PORT_SET,
                         &swstate->portSetLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("mirrorLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_MIRROR,
                         &swstate->mirrorLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("triggerLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_TRIGGERS,
                         &swstate->triggerLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("lagLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_LAGS,
                         &swstate->lagLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("pktIntLock", 
                         swstate->switchNumber,
                         FM_LOCK_SUPER_PRECEDENCE,
                         &swstate->pktIntLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("crmLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_CRM,
                         &swstate->crmLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("mtableLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_MTABLE,
                         &swstate->mtableLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("schedulerLock", 
                         swstate->switchNumber,
                         FM_LOCK_PREC_SCHEDULER,
                         &swstate->schedulerLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateLockV2("mailboxLock",
                         swstate->switchNumber,
                         FM_LOCK_PREC_MAILBOX,
                         &swstate->mailboxLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    swstate->accessLocksInitialized = TRUE;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmAllocateSwitchLocks */




/*****************************************************************************/
/** fmFreeSwitchLocks
 * \ingroup intSwitch
 *
 * \desc            create switch locks
 *
 * \param[in]       swstate points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmFreeSwitchLocks(fm_switch *swstate)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "swstate=%p (switch number %d)\n",
                 (void *) swstate,
                 swstate->switchNumber);

    /* The locks have not been initialized, error! */
    if (!swstate->accessLocksInitialized)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( ( err = fmDeleteLock(&swstate->stateLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->L2Lock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->maPurgeLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->macTableMaintWorkListLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->aclLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->tunnelLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->natLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->flowLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteRwLock(&swstate->routingLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->portSetLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->mirrorLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->triggerLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->lagLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->pktIntLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->crmLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->mtableLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->schedulerLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( ( err = fmDeleteLock(&swstate->mailboxLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    swstate->accessLocksInitialized = FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmFreeSwitchLocks */




/*****************************************************************************/
/** fmSetSwitchModelSpecificInfo
 * \ingroup intSwitch
 *
 * \desc            Sets per switch model specific information and writes it
 *                  to the fm_switch structure of the switch if and only if it
 *                  has not already been writen to.
 *                  
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetSwitchModelSpecificInfo(fm_int sw)
{
    fm_status           status = FM_OK;
    fm_switch          *switchPtr;
    fm_switchPartNum    swPartNum;
    fm_int              maxPhysPort;

    switchPtr = GET_SWITCH_PTR(sw);
       
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switch(switchPtr->switchFamily)
    {
#if FM_SUPPORT_FM2000
        case FM_SWITCH_FAMILY_FM2000:
            status = fm2000GetSwitchPartNumber(sw, &swPartNum);
            break;
#endif

#if FM_SUPPORT_FM4000
        case FM_SWITCH_FAMILY_FM4000:
        case FM_SWITCH_FAMILY_REMOTE_FM4000:
            status = fm4000GetSwitchPartNumber(sw, &swPartNum);
            break;
#endif

#if FM_SUPPORT_FM10000
        case FM_SWITCH_FAMILY_FM10000:
            status = fm10000GetSwitchPartNumber(sw, &swPartNum);
            break;
#endif

#if FM_SUPPORT_FM6000
        case FM_SWITCH_FAMILY_FM6000:
        case FM_SWITCH_FAMILY_REMOTE_FM6000:
            status = fm6000GetSwitchPartNumber(sw, &swPartNum);
            break;
#endif

        default: 
            swPartNum = FM_SWITCH_PART_NUM_UNKNOWN;
            break;
    }

    if (status != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                     "Could not retrieve switch #%d part number: %s\n",
                     sw, fmErrorMsg(status) );

        goto ABORT;
    }

    status = fmGetPartNumberMaxPort(swPartNum, &maxPhysPort);

    if (status != FM_OK)
    {
        goto ABORT;
    }

    /* Only change value if it did not already exist */
    if ((switchPtr->maxPhysicalPort == 0) && (maxPhysPort != 0))
    {
        switchPtr->maxPhysicalPort = maxPhysPort;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmSetSwitchModelSpecificInfo */




/*****************************************************************************/
/** fmAllocateSwitchDataStructures
 * \ingroup intSwitch
 *
 * \desc            Allocates memory associated with a switch state table.
 *                  Called on switch insertion.
 *                  Note: switch number, family, and model are assumed to have
 *                  been pre-initialized before this function was called.  All
 *                  Platform-specific function pointers must also be
 *                  pre-initialized before this function is called, since they
 *                  will have been needed for model identification.
 * \note            Please follow the convention: What is done in allocate when
 *                  switch is inserted, should be freed when switch is removed.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmAllocateSwitchDataStructures(fm_int sw)
{
    fm_status        err;
    int              i;
    fm_switch *      swstate;
    const fm_switch *defSwitchTable;
    fm_uint32 *      srcPtr;
    fm_uint32 *      destPtr;
    fm_int           wordCount;
    fm_int           remainder;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    swstate = GET_SWITCH_PTR(sw);

    err = fmSetSwitchModelSpecificInfo(sw);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /* find and copy family & model specific defaults */
    i              = 0;
    defSwitchTable = NULL;

    while (fmSwitchModelList[i].family != FM_SWITCH_FAMILY_UNKNOWN)
    {
        if ( (fmSwitchModelList[i].family == swstate->switchFamily)
            && (fmSwitchModelList[i].model == swstate->switchModel) )
        {
            defSwitchTable = fmSwitchModelList[i].switchTable;
            break;
        }

        i++;
    }

    if ( (defSwitchTable == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_SWITCH_TYPE);
    }

    /**************************************************
     * copy non-zero words from the default table into the active table.
     * If a word in the active table is non-zero, assume it has been
     * pre-initialized and honor the value already there.
     * Note: This code assumes that no 64-bit fields will be initialized
     * in the default table and pre-initialized in the switch table before
     * this code is executed.  If that assumption is not correct, system
     * failure could occur as words overwrite part of a 64-bit or larger
     * field.
     **************************************************/
    wordCount = sizeof(fm_switch) / sizeof(fm_uint32);
    remainder = sizeof(fm_switch) % sizeof(fm_uint32);

    if (remainder != 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_FAIL);
    }

    srcPtr  = (fm_uint32 *) defSwitchTable;
    destPtr = (fm_uint32 *) swstate;

    while (wordCount > 0)
    {
        if ( (*destPtr == 0) && (*srcPtr != 0) )
        {
            *destPtr = *srcPtr;
        }

        srcPtr++;
        destPtr++;
        wordCount--;
    }

    /**************************************************
     * Initialize default glort range information
     **************************************************/
    swstate->glortRange.glortBase       = 0;
    swstate->glortRange.glortMask       = 0;
    swstate->glortRange.portBaseGlort   = ~0;
    swstate->glortRange.portCount       = -1;
    swstate->glortRange.lagBaseGlort    = ~0;
    swstate->glortRange.lagCount        = -1;
    swstate->glortRange.mcastBaseGlort  = ~0;
    swstate->glortRange.mcastCount      = -1;
    swstate->glortRange.lbgBaseGlort    = ~0;
    swstate->glortRange.lbgCount        = -1;
    swstate->mailboxInfo.glortBase      = ~0;
    swstate->mailboxInfo.glortMask      = ~0;
    swstate->mailboxInfo.glortsPerPep   = 0;

    /**************************************************
     * Initialize additional glort information
     **************************************************/
    swstate->glortInfo.cpuBase        = FM_GLORT_CPU_BASE;
    swstate->glortInfo.cpuMask        = FM_GLORT_CPU_MASK;

    swstate->glortInfo.specialBase    = FM_GLORT_SPECIAL_BASE;
    swstate->glortInfo.specialMask    = FM_GLORT_SPECIAL_MASK;
    swstate->glortInfo.specialSize    = FM_GLORT_SPECIAL_SIZE;
    swstate->glortInfo.specialALength = FM_GLORT_SPECIAL_A_LENGTH;

    /**************************************************
     * Switch-Type-Specific Initializations
     **************************************************/
    if ( ( err = swstate->InitSwitch(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Parity Sweeper Initialization. 
     **************************************************/
    swstate->paritySweeperCfg.enabled = 
                fmGetBoolApiProperty(FM_AAK_API_PARITY_SWEEPER_ENABLE, 
                                     FM_AAD_API_PARITY_SWEEPER_ENABLE);

    swstate->paritySweeperCfg.readBurstSize = 
                fmGetIntApiProperty(FM_AAK_API_PARITY_SWEEPER_READ_BURST_SIZE,
                                    FM_AAD_API_PARITY_SWEEPER_READ_BURST_SIZE);

    swstate->paritySweeperCfg.sleepPeriod = 
                fmGetIntApiProperty(FM_AAK_API_PARITY_SWEEPER_SLEEP_PERIOD,
                                    FM_AAD_API_PARITY_SWEEPER_SLEEP_PERIOD);

    /* By default, the parity sweeper does not run for FIBM */ 
    if ( fmRootApi->isSwitchFibmSlave[sw] &&
         !fmGetBoolApiProperty(FM_AAK_API_PARITY_SWEEPER_FIBM_ENABLE, 
                               FM_AAD_API_PARITY_SWEEPER_FIBM_ENABLE) )
    {
        swstate->paritySweeperCfg.enabled = FALSE;
    }

    /* make sure the access locks get initialized */
    /**************************************************
     * Allocate switch-specific data structures
     **************************************************/
    FM_API_CALL_FAMILY(err, swstate->AllocateDataStructures, swstate);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Mac Table Maintenance
     **************************************************/
    if ( ( err = fmAllocateMacTableMaintenanceDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Address Table
     **************************************************/
    if ( ( err = fmAllocateAddressTableDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Vlan Table
     **************************************************/
    if ( ( err = fmAllocateVlanTableDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * ACL Table
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * Policer Table
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * STP Instance Table
     **************************************************/
    if ( ( err = fmAllocateStpInstanceTreeDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Mirror Groups
     **************************************************/
    if ( ( err = fmAllocatePortMirrorDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Counter Storage
     **************************************************/
    if ( ( err = fmAllocateCounterDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Load balancing groups
     **************************************************/
    if ( ( err = fmAllocateLBGDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /**************************************************
     * Virtual Networks.
     **************************************************/
    err = fmVNAlloc(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Router ressources allocation.
     *
     * If this switch does not support routing, this
     * function is expected to just return FM_OK.
     **************************************************/
    err = fmRouterAlloc(sw);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /************************************************** 
     *  NextHop ressources allocation
     **************************************************/
    err = fmNextHopAlloc(sw);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmAllocateSwitchDataStructures */




/*****************************************************************************/
/** fmFreeSwitchDataStructures
 * \ingroup intSwitch
 *
 * \desc            Free memory associated with a switch state table when a
 *                  switch is removed.
 *                  This is to undo what is done in fmAllocateSwitchDataStructures.
 *                  NOTE: it is assumed that any protection required for
 *                  performing this operation has already been done.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmFreeSwitchDataStructures(fm_int sw)
{
    fm_status  err     = FM_OK;
    fm_switch *swstate = fmRootApi->fmSwitchStateTable[sw];
    fm_status  retErr  = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    /**************************************************
     * Mac Table Maintenance
     **************************************************/
    if ( ( err = fmFreeMacTableMaintenanceDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free MAC table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Address Table
     **************************************************/
    if ( ( err = fmFreeAddressTableDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free Address table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Vlan Table
     **************************************************/
    if ( ( err = fmFreeVlanTableDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free Vlan table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * ACL Table
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * Policer Table
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * STP Instance Table
     **************************************************/
    if ( ( err = fmFreeStpInstanceTreeDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free STP Table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /***************************************************
     * Link Aggregation Groups.
     **************************************************/
    /* Nothing to do */

    /**************************************************
     * Mirror Groups
     **************************************************/
    if ( ( err = fmFreePortMirrorDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free mirror table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Counter Storage
     **************************************************/
    if ( ( err = fmFreeCounterDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free counter table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Load balancing groups
     **************************************************/
    if ( ( err = fmFreeLBGDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free LBG table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Virtual Networks
     **************************************************/
//    err = fmVNFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Fail to free virtual networks structures: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }


    /**************************************************
     * NextHop Subsystem.
     *
     **************************************************/
    err = fmNextHopFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free nextHop ressources: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Router Subsystem.
     *
     * If this switch does not support routing, this
     * function is expected to just return FM_OK.
     **************************************************/
    err = fmRouterFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free router ressources: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Multicast state.
     **************************************************/
    /* Nothing to do */

    /**************************************************
     * Switch-specific data structures.
     **************************************************/
    FM_API_CALL_FAMILY(err, swstate->FreeDataStructures, swstate);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free switch specific structure: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /*************************************************** 
     * Switch locks.
     * 
     * Must be done last, in case other free functions
     * need these locks.
     *
     * Note that fmAllocateSwitchDataStructures does
     * not allocate these locks because they are allocated
     * prior to the call to that function.
     **************************************************/
    if ( ( err = fmFreeSwitchLocks(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free switch lock: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, retErr);

}   /* end fmFreeSwitchDataStructures */




/*****************************************************************************/
/** fmInitializeSwitchDataStructure
 * \ingroup intSwitch
 *
 * \desc            Initializes the entry in the switch state table.
 *                  This function calls all the local initialization functions.
 *                  Called when the switch is brought up.
 *
 * \note            Please follow the convention: What is done in init when
 *                  switch is brought up, should be cleaned up when switch is 
 *                  brought down.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmInitializeSwitchDataStructure(fm_int sw)
{
    fm_status  err;
    fm_switch *swstate = fmRootApi->fmSwitchStateTable[sw];

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    /* Initialize the stacking API */
    err = fmInitStacking(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize mac address table */
    err = fmInitAddressTable(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize vlan table */
    err = fmInitVlanTable(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize Qos state */
    err = fmInitQOS(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize Lag Table */
    err = fmInitLAGTable(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize ACL Table */
    err = fmInitACLTable(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize Policer Table */
    err = fmInitPolicers(&swstate->policerInfo);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize PortSet Table */
    err = fmInitPortSetTable(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize statistics counters */
    err = fmInitCounters(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize Mac Table Maintenance State */
    err = fmInitMacTableMaintenance(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize spanning tree instances */
    err = fmInitStpInstanceTree(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize port mirroring state */
    err = fmInitPortMirror(swstate);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize NAT state */
    err = fmNatInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Virtual Networks.
     *
     * Virtual networking needs the route lookup feature
     * from the routing subsystem. So fmVNAlloc must
     * be called before fmRouterAlloc.
     **************************************************/
    err = fmVNInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize router state */
    err = fmRouterInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize nextHop state */
    err = fmNextHopInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);


    /* Initialize multicast state */
    err = fmMcastGroupInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize mailbox state */
    err = fmMailboxInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Misc. other stuff. */
    swstate->intrSendPackets    = FALSE;
    swstate->intrReceivePackets = FALSE;
    swstate->buffersNeeded      = FALSE;
    swstate->transmitterLock    = FALSE;
    /* all vlans share a single spanning tree instance */
    swstate->stpMode            = FM_SPANNING_TREE_SHARED;
    /* enable the automatic pause mode */
    swstate->autoPauseMode      = TRUE;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmInitializeSwitchDataStructure */




/*****************************************************************************/
/** fmCleanupSwitchDataStructures
 * \ingroup intSwitch
 *
 * \desc            Clean up switch data structure when switch is going down.
 *                  This is to undo what is done in fmInitializeSwitchDataStructure.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmCleanupSwitchDataStructures(fm_int sw)
{
    fm_status  err     = FM_OK;
    fm_switch *swstate = fmRootApi->fmSwitchStateTable[sw];
    fm_status  retErr  = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    /**************************************************
     * Mac Table Maintenance
     **************************************************/
    /* Nothing to do */

    /**************************************************
     * Address Table
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * Vlan Table
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * ACL Table
     **************************************************/
    if ( ( err = fmDestroyACLTable(&swstate->aclInfo) ) != FM_OK ) 
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to destroy ACL table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Policer Table
     **************************************************/
    if ( ( err = fmDestroyPolicers(&swstate->policerInfo) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to destroy policiers: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Portset Table
     **************************************************/
    if ( ( err = fmDestroyPortSetTable(&swstate->portSetInfo) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to destroy portset table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * STP Instance Table
     **************************************************/
    if ( ( err = fmDestroyStpInstanceTree(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to destroy stp instances: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }


    /***************************************************
     * Link Aggregation Groups.
     **************************************************/
    if ( ( err = fmDestroyLAGTable(sw) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to destroy LAG table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }


    /**************************************************
     * Mirror Groups
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * Counter Storage
     **************************************************/
    /* Nothing to do */


    /**************************************************
     * Load balancing groups
     **************************************************/
    /* Nothing to do */


    err = fmNatFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Fail to cleanup NAT: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Virtual Networks
     **************************************************/
    err = fmVNCleanup(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Fail to cleanup virtual networks: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * NextHop Subsystem.
     *
     **************************************************/
    err = fmNextHopCleanup(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to cleanup nextHop: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }


    /**************************************************
     * Router Subsystem.
     *
     * If this switch does not support routing, this
     * function is expected to just return FM_OK.
     **************************************************/
    err = fmRouterCleanup(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to cleanup router: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Multicast state.
     **************************************************/
    if ( ( err = fmFreeMcastGroupDataStructures(swstate) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free multicast table: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Stacking API.
     **************************************************/
    if ( ( err = fmFreeStackingResources(sw) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free stacking resources: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    /**************************************************
     * Mailbox API.
     **************************************************/
    if ( ( err = fmMailboxFreeResources(sw) ) != FM_OK )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, "Fail to free mailbox resources: %s\n",
                     fmErrorMsg(err));
        retErr = err;
        /* Don't return, just continue on */
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, retErr);

}   /* end fmCleanupSwitchDataStructures */




/*****************************************************************************/
/** CreateLocalDispatchThread
 * \ingroup intSwitch
 *
 * \desc            Creates a thread for calling the event handler
 *                  callback for the current process.  Adds this thread
 *                  to the list of processes.  Initially allow all events,
 *                  except only the first process gets FM_EVENT_PKT_RECV
 *                  events.
 *
 * \param           None
 *
 * \return          status
 *
 *****************************************************************************/
static fm_status CreateLocalDispatchThread(void)
{
    fm_status         err;
    fm_status         err2;
    fm_localDelivery *delivery;
    fm_int            myProcessId;
    fm_bool           firstProcess;
    char              threadName[64];

    myProcessId = fmGetCurrentProcessId();

    FM_SNPRINTF_S(threadName,
                  sizeof(threadName),
                  "Local event dispatch (%d)",
                  myProcessId);

    err = fmCaptureLock(&fmRootApi->localDeliveryLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        return err;
    }

    delivery = (fm_localDelivery *) fmAlloc( sizeof(fm_localDelivery) );

    if (delivery == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    delivery->thread = (fm_thread *) fmAlloc( sizeof(fm_thread) );

    if (delivery->thread == NULL)
    {
        fmFree(delivery);
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    firstProcess =
        (FM_DLL_GET_FIRST( (&fmRootApi->localDeliveryThreads), head ) == NULL);

    delivery->mask      = ~(firstProcess ? 0 : FM_EVENT_PKT_RECV);
    delivery->processId = myProcessId;

    err = fmCreateThread(threadName,
                         FM_DISPATCH_QUEUE_SIZE,
                         &fmLocalEventHandler,
                         NULL,
                         delivery->thread);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmDListInsertEnd(&fmRootApi->localDeliveryThreads, delivery);
    if (err == FM_OK)
    {
        fmRootApi->localDeliveryCount++;
    }

ABORT:
    err2 = fmReleaseLock(&fmRootApi->localDeliveryLock);

    if (err == FM_OK)
    {
        err = err2;
    }

    return err;

}   /* end CreateLocalDispatchThread */




/*****************************************************************************/
/** TerminateLocalDispatchThread
 * \ingroup intSwitch
 *
 * \desc            Terminates the local event dispatch thread and reclaims
 *                  all resources that were allocated.
 *
 * \param           None
 *
 * \return          status
 *
 *****************************************************************************/
static fm_status TerminateLocalDispatchThread(void)
{
    fm_status         err;
    fm_localDelivery *delivery;
    fm_thread        *thread;

    err = fmRemoveEventHandler(&delivery);

    if (err != FM_OK)
    {
        return err;
    }

    /* 
     * signal local event thread to exit
     */

    localDispatchThreadExit = TRUE;
    thread = delivery->thread;

    err = fmSignalThreadEventHandler(thread);
    if (err != FM_OK)
    {
        fmFree(delivery);
        return err;
    }

    err = fmWaitThreadExit(thread);

    fmFree(delivery);

    return err;

}   /* end TerminateLocalDispatchThread */




/*****************************************************************************/
/** fmApiRootInit
 * \ingroup intSwitch
 *
 * \desc            Initialize the fmRootApi data structure, which is shared
 *                  across all processes.
 *
 * \param           None
 *
 * \return          status
 *
 *****************************************************************************/
static fm_status fmApiRootInit(void)
{
    fm_status err;
    fm_int    sw;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "(no arguments)\n");

    fmFirstProcess = TRUE;

    fmRootApi = fmAlloc( sizeof(fm_rootApi) );

    if (fmRootApi == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    memset( fmRootApi, 0, sizeof(fm_rootApi) );

    /* initialize the event delivery data structures */
    fmDListInit(&fmRootApi->localDeliveryThreads);
    fmRootApi->localDeliveryCount = 0;

    err = fmCreateLock("Local event delivery lock",
                       &fmRootApi->localDeliveryLock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* no switches are present at startup */
    for (sw = 0 ; sw < FM_MAX_NUM_SWITCHES ; sw++)
    {
        fmRootApi->fmSwitchStateTable[sw] = NULL;
        fmRootApi->fmSwitchLockTable[sw]  = NULL;
    }

    /* create semaphore that holds the global event thread until we are ready */
    err = fmCreateSemaphore("start global event handler",
                            FM_SEM_BINARY,
                            &fmRootApi->startGlobalEventHandler,
                            0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* start event thread, must be done before fmPlatformInitialize
     * so it can receive events from platform, such as switch insert.
     */
    err = fmCreateThread("events",
                         FM_EVENT_QUEUE_SIZE_LARGE,
                         &fmGlobalEventHandler,
                         NULL,
                         &fmRootApi->eventThread);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Initialize the event handling subsystem */
    err = fmEventHandlingInitialize();
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* initialize interrupt signaling semaphore */
    err = fmCreateSemaphore("intrAvail",
                            FM_SEM_BINARY,
                            &fmRootApi->intrAvail,
                            0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateSemaphore("macTableMaintSemaphore",
                            FM_SEM_BINARY,
                            &fmRootApi->macTableMaintSemaphore,
                            0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateSemaphore("parityRepairSemaphore",
                            FM_SEM_BINARY,
                            &fmRootApi->parityRepairSemaphore,
                            0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Create the packet receive thread semaphore */
    err = fmCreateSemaphore("Packet Receive",
                            FM_SEM_BINARY,
                            &fmRootApi->packetReceiveSemaphore,
                            0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmCreateSemaphore("waitForBufferSemaphore",
                            FM_SEM_BINARY,
                            &fmRootApi->waitForBufferSemaphore,
                            1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* initialize the system up time */
    err = fmGetTime( &fmRootApi->apiInitTime );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmApiRootInit */




/*****************************************************************************/
/** fmApiThreadInit
 * \ingroup intSwitch
 *
 * \desc            Initialize the threads to start.
 *
 * \param           None
 *
 * \return          status
 *
 *****************************************************************************/
static fm_status fmApiThreadInit(void)
{
    fm_status err;
         

    /* Create the timer task (event-driven) */
    err = fmCreateTimerTask( "TimerTask", 
                             FM_TIMER_TASK_MODE_EVENT_DRIVEN,
                             NULL,
                             &fmRootApi->timerTask );
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /* Create the link state debouncer task */
    err = fmCreateThread("LinkStateDebounceTask",
                         FM_EVENT_QUEUE_SIZE_NONE,
                         fmDebounceLinkStateTask,
                         &fmRootApi->eventThread,
                         &fmRootApi->debounceTask);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    if ( !fmGetBoolApiProperty(FM_AAK_DEBUG_BOOT_INTERRUPT_HANDLER,
                               FM_AAD_DEBUG_BOOT_INTERRUPT_HANDLER) )
    {
        /* Create the interrupt handler task */
        err = fmCreateThread("InterruptHandlerTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmInterruptHandler,
                             &fmRootApi->eventThread,
                             &fmRootApi->interruptTask);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
        }
    }

    if (fmGetBoolApiProperty(FM_AAK_API_MA_TABLE_MAINTENENANCE_ENABLE,
                             FM_AAD_API_MA_TABLE_MAINTENENANCE_ENABLE))
    {
        /* Create the table maintenance task */
        err = fmCreateThread("MacTableMaintenanceTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmTableMaintenanceHandler,
                             &fmRootApi->eventThread,
                             &fmRootApi->maintenanceTask);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
        }
    }

    /* Create the fast maintenance task.
     * This needs to be done after fmPlatformInitialize so API attribute
     * can be controlled by the platform
     */
    if ( fmGetBoolApiProperty(FM_AAK_API_FAST_MAINTENANCE_ENABLE,
                              FM_AAD_API_FAST_MAINTENANCE_ENABLE) )
    {
        err = fmCreateThread("FastMaintenanceTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmFastMaintenanceTask,
                             &fmRootApi->eventThread,
                             &fmRootApi->fastMaintenanceTask);
        
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
        }
    }

    if ( fmGetBoolApiProperty(FM_AAK_API_PACKET_RECEIVE_ENABLE,
                              FM_AAD_API_PACKET_RECEIVE_ENABLE) )
    {
        /* Create the packet receive thread */
        err = fmCreateThread("PacketReceiveTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmReceivePacketTask,
                             NULL,
                             &fmRootApi->packetReceiveTask);
    }

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    }

    /* Create parity sweeper thread */
    if ( fmGetBoolApiProperty(FM_AAK_API_PARITY_SWEEPER_ENABLE, 
                              FM_AAD_API_PARITY_SWEEPER_ENABLE) )
    {
        err = fmCreateThread("ParitySweeperTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmParitySweeperTask,
                             &fmRootApi->eventThread,
                             &fmRootApi->paritySweeperTask);

        if ( err != FM_OK )
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
        }
    }

    /* Create parity repair thread. */
    if ( fmGetBoolApiProperty(FM_AAK_API_PARITY_REPAIR_ENABLE, 
                              FM_AAD_API_PARITY_REPAIR_ENABLE) )
    {
        err = fmCreateThread("ParityRepairTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmParityRepairTask,
                             &fmRootApi->eventThread,
                             &fmRootApi->parityRepairTask);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Create routing maintenance thread. */
    if ( fmGetBoolApiProperty(FM_AAK_API_ROUTING_MAINTENANCE_ENABLE, 
                              FM_AAD_API_ROUTING_MAINTENANCE_ENABLE) )
    {
        err = fmCreateThread("RoutingMaintenanceTask",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             fmRoutingMaintenanceTask,
                             &fmRootApi->eventThread,
                             &fmRootApi->routingMaintenanceTask);
        if ( err != FM_OK )
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmApiThreadInit */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmSetPreInitializationFlags
 * \ingroup intApi
 *
 * \desc            Function to override default initialization.  Must be
 *                  called before fmInitialize if non-standard behavior
 *                  is desired.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       flags is the new pre-initialization flags value.  See
 *                  the definition of this function in fm_api_init.h.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetPreInitializationFlags(fm_int sw, fm_uint32 flags)
{
    VALIDATE_SWITCH_INDEX(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d flags=0x%x\n", sw, flags);

    FM_NOT_USED(flags);

    FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                 "fmSetPreInitializationFlags is no longer supported\n");

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_UNSUPPORTED);

}   /* end fmSetPreInitializationFlags */




/*****************************************************************************/
/** fmInitialize
 * \ingroup api
 *
 * \desc            Called by the application to initialize the API.  
 *
 * \note            If multiple processes are using the API, each process
 *                  must call fmInitialize prior to making any other API
 *                  call. They must then also call ''fmTerminate'' upon exiting.
 *
 * \param[in]       eventHandlerFunc points to an event handler call-back
 *                  function in the application that will be responsible for
 *                  handling events reported by the API.  May be specified as
 *                  NULL if no handler is available.  The handler may be
 *                  (re)specified later with a call to ''fmSetEventHandler''.
 *                                                                      \lb\lb
 *                  ''fmSetProcessEventMask'' may be called to select which
 *                  events are reported to the handler.                
 *                                                                      \lb\lb
 *                  See ''fm_eventHandler'' for deatils on the prototype of
 *                  this call-back function and the nature of events reported
 *                  to it.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory for driver
 *                  threads.
 * \return          FM_FAIL if unable to create a lock for driver threads.
 * \return          FM_ERR_UNABLE_TO_CREATE_THREAD if operating system thread
 *                  creation failed.
 *
 *****************************************************************************/
fm_status fmInitialize(fm_eventHandler eventHandlerFunc)
{
    fm_int             switchCount;
    fm_status          err;
    fm_bool            firstProcess;
    fm_uint64          dbgLoggingCat;
    fm_text            dbgLoggingCatStr;
    fm_int             tsMode;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_API,
                     "eventHandlerFunc=%p\n",
                     (void *) (fm_uintptr) eventHandlerFunc);

    err = fmOSInitialize();
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }

    /* Do per-calling process initialization */
    err = fmInitProcess();
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }
    
    /* initialize debug facilities */
    err = fmDbgInitialize();
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }

    /* this ensures initial insertions are handled */
    fmSetEventHandler(eventHandlerFunc);

    /* initialize everything that is shared across all processes */
    err = fmGetRoot("api", (void **) &fmRootApi, fmApiRootInit);
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }


    /* Initialize the platform */
    err = fmPlatformInitialize(&switchCount);
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }

    tsMode = fmGetIntApiProperty( FM_AAK_API_GSME_TIMESTAMP_MODE,
                                  FM_AAD_API_GSME_TIMESTAMP_MODE );
    if ( tsMode < 0 || tsMode >= FM_GSME_TIMESTAMP_MODE_MAX )
    {
        FM_LOG_WARNING( FM_LOG_CAT_API, 
                        "Invalid GSME timestamp mode %d, "
                        "using default value of %d\n",
                        tsMode,
                        FM_AAD_API_GSME_TIMESTAMP_MODE );
        tsMode = FM_AAD_API_GSME_TIMESTAMP_MODE;
    }

    /* initialize the state machine engine */
    err = fmInitStateMachineEngine( &fmRootApi->apiInitTime, tsMode );
    if ( err != FM_OK )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }

    
    /* Get logging categories for debugging */
    dbgLoggingCatStr = fmGetTextApiProperty(FM_AAK_API_DEBUG_INIT_LOGGING_CAT, 
                                            FM_AAD_API_DEBUG_INIT_LOGGING_CAT);

    /* Extract categories and transform into mask */
    err = LogStrToMask(dbgLoggingCatStr, &dbgLoggingCat);

    if (err != FM_OK)
    {
        FM_LOG_PRINT("Failed to retrieve log mask\n");
        err = FM_OK;
    }
    else if (dbgLoggingCat == 0)
    {
        /* Do nothing */
    } 
    else 
    {
        /* For help in diagnosing API startup errors. */
        fmSetLoggingFilter(dbgLoggingCat,
                           FM_LOG_LEVEL_ALL,
                           NULL,
                           NULL);
    }

    /* Initialize the API threads. Done after fmPlatformInitialize so
     * it can read in the API attributes correctly setup by the platform
     */

    firstProcess =
        (FM_DLL_GET_FIRST( (&fmRootApi->localDeliveryThreads), head ) == NULL);

    if (firstProcess == TRUE)
    {
        err = fmApiThreadInit();
        if (err != FM_OK)
        {
            FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
        }
    }


    /* Create a local event dispatch thread for this process */
    err = CreateLocalDispatchThread();
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_API, err);
    }

    /* Start the global event handler thread */
    if (fmFirstProcess)
    {
        err = fmReleaseSemaphore(&fmRootApi->startGlobalEventHandler);
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_API, err);

}   /* end fmInitialize */




/*****************************************************************************/
/** fmTerminate
 * \ingroup api
 *
 * \desc            If multiple processes are using the API, 
 *                  each having called ''fmInitialize'', then each
 *                  process must call this function before it exits. This 
 *                  function does nothing when called by the process that
 *                  was the first to call ''fmInitialize'', which implies
 *                  that applications with only a single process need not
 *                  call fmTerminate.
 *
 * \param[in]       None
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if an internal API error occurs.
 * \return          FM_ERR_INVALID_ARGUMENT if an internal API error occurs.
 * \return          FM_ERR_UNABLE_TO_SIGNAL_COND if an internal API error occurs.
 *
 *****************************************************************************/
fm_status fmTerminate(void)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_API,
                     "first process = %d\n",
                     fmFirstProcess);

    /* Do not terminate if it's the first process */
    if (!fmFirstProcess)
    {
        /* Terminate local event dispatch thread for this process */
        err = TerminateLocalDispatchThread();
    
        /* allow the local dispatch thread to exit */
        fmYield();
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_API, err);

}   /* end fmTerminate */




/*****************************************************************************/
/** fmSetFrameHandlerPLLDividers
 * \ingroup intSwitch
 *
 * \desc            Sets the PLL divider values for the frame handler
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       p is a PLL divider parameter
 *
 * \param[in]       m is a PLL divider parameter
 *
 * \param[in]       n is a PLL divider parameter
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetFrameHandlerPLLDividers(fm_int sw,
                                       fm_int p,
                                       fm_int m,
                                       fm_int n)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d p=%d m=%d n=%d\n", sw, p, m, n);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.frameHandlerPLLDividers[FM_BOOT_PLL_MULT_P] = p;
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.frameHandlerPLLDividers[FM_BOOT_PLL_MULT_M] = m;
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.frameHandlerPLLDividers[FM_BOOT_PLL_MULT_N] = n;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmSetFrameHandlerPLLDividers */




/*****************************************************************************/
/** fmComputeFHClockFreq
 * \ingroup intSwitch
 *
 * \desc            Calculate the frame handler clock frequency from the
 *                  configured frame handler PLL multipliers.
 *
 * \param[in]       sw is the switch for which to calculate the frame handler
 *                  clock frequency.
 * 
 * \param[out]      fhMhz points to a caller-supplied location to receive
 *                  the frame handler clock frequency in MHz.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmComputeFHClockFreq(fm_int sw, fm_float *fhMhz)
{
    fm_status  err;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err, switchPtr->ComputeFHClockFreq, sw, fhMhz);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmComputeFHClockFreq */




/*****************************************************************************/
/** fmEnableEEPROMMode
 * \ingroup intSwitch
 *
 * \desc            enable/disable use of an EEPROM for boot/configuration
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable EEPROM mode, FALSE to disable it.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmEnableEEPROMMode(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.enableEEPROMMode = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmEnableEEPROMMode */




/*****************************************************************************/
/** fmEnableLEDInvert
 * \ingroup intSwitch
 *
 * \desc            enable/disable LED state inversion
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable LED state inversion,
 *                  FALSE to disable it.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmEnableLEDInvert(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.invertLED = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmEnableLEDInvert */




/*****************************************************************************/
/** fmEnableLED
 * \ingroup intSwitch
 *
 * \desc            enable/disable LED support in the switch
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable the LEDs, FALSE to disable them.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmEnableLED(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.enableLED = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmEnableLED */




/*****************************************************************************/
/** fmEnableDFT
 * \ingroup intSwitch
 *
 * \desc            enable/disable DFT support in the switch
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable DFT support, FALSE to disable it.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmEnableDFT(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.enableDFT = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmEnableDFT */




/*****************************************************************************/
/** fmEnableFrameHandlerPLLBypass
 * \ingroup intSwitch
 *
 * \desc            enable/disable PLL Bypass mode in the switch
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable PLL Bypass, FALSE to disable it.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmEnableFrameHandlerPLLBypass(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.bypassFrameHandlerPLL = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmEnableFrameHandlerPLLBypass */




/*****************************************************************************/
/** fmUseShadowFuseboxEntries
 * \ingroup intSwitch
 *
 * \desc            use/don't use the shadow fusebox entries for SRAM repair
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable the fusebox, FALSE to disable it.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmUseShadowFuseboxEntries(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.useShadowFuseboxEntries = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmUseShadowFuseboxEntries */




/*****************************************************************************/
/** fmSetShadowFuseboxEntry
 * \ingroup intSwitch
 *
 * \desc            write one of the shadow fusebox entries
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       entry is the entry number
 *
 * \param[in]       val is the value to write
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetShadowFuseboxEntry(fm_int sw, fm_int entry, fm_uint32 val)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d entry=%d value=0x%x\n",
                 sw,
                 entry,
                 val);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        if ( (entry < 0) || (entry >= FM_MAX_FUSEBOX_ENTRIES) )
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            fmRootApi->fmSwitchStateTable[sw]->bootInfo.shadowFuseboxEntries[entry] = val;
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmSetShadowFuseboxEntry */




/*****************************************************************************/
/** fmSetAutoBoot
 * \ingroup intSwitch
 *
 * \desc            enable/disable Auto Boot support in the switch
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable is TRUE to enable auto-boot, FALSE to disable it.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetAutoBoot(fm_int sw, fm_bool enable)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d enable=%d\n", sw, enable);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
    }
    else
    {
        fmRootApi->fmSwitchStateTable[sw]->bootInfo.autoBoot = enable;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmSetAutoBoot */




/*****************************************************************************/
/** fmSetSwitchState
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set the switch to an up or down state.
 *
 * \note            After the API has been initialized,
 *                  (see 'fmInitialize'), this function must be called
 *                  to bring the switch(es) to an UP state. The same is
 *                  true for switch aggregates after they have been created
 *                  and fully configured.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       state indicates whether to set the switch state to up
 *                  (TRUE) or down (FALSE).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_FAIL if general failure accessing switch structures.
 *
 *****************************************************************************/
fm_status fmSetSwitchState(fm_int sw, fm_bool state)
{
    fm_status      err;
    fm_switch *    switchPtr;
    fm_bool        switchLocked = FALSE;
#if FM_SUPPORT_SWAG
    fm_bool        pass1 = TRUE;
    fm_int         swagId = -1;
    fm_int         origSwagId = -1;
    fm_bool        isInSwag = FALSE;
    fm_bool        swagLocked = FALSE;
    fmSWAG_switch *aggregatePtr;
    fm_swagMember *member;
#endif

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d state=%d\n", sw, state);

    VALIDATE_SWITCH_LOCK(sw);

#if FM_SUPPORT_SWAG

RESTART:

#endif

    /* Take write access to the switch lock */
    err = LOCK_SWITCH(sw);

    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to capture switch write lock %p for switch %d\n",
                     (void *) fmRootApi->fmSwitchLockTable[sw],
                     sw);

        goto ABORT;
    }

    switchLocked = TRUE;

    /* return error if switch is not found. */
    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (state)
    {
        /* If switch is already in the correct state, we're done. */
        if (fmRootApi->fmSwitchStateTable[sw]->state == FM_SWITCH_STATE_UP)
        {
            err = FM_OK;
            goto ABORT;
        }
        if (fmRootApi->fmSwitchStateTable[sw]->state == FM_SWITCH_STATE_DOWN)
        {
            /* only this is valid, allow to continue */
            ;
        }
        else
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }
    else
    {
        /* If switch is already in the correct state, we're done. */
        if (fmRootApi->fmSwitchStateTable[sw]->state == FM_SWITCH_STATE_DOWN)
        {
            err = FM_OK;
            goto ABORT;
        }
        /* The rest, allow to go on to cleanup */
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

#if FM_SUPPORT_SWAG

    swagId = switchPtr->swag;

    if (pass1)
    {
        /* Is the switch in a swag? */
        if (swagId >= 0)
        {
            isInSwag = TRUE;

            UNLOCK_SWITCH(sw);
            switchLocked = FALSE;

            /* Validate the swag ID */
            VALIDATE_SWITCH_LOCK(swagId);

            /* Lock the SWAG */
            err = LOCK_SWITCH(swagId);

            if (err != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Unable to capture SWAG write lock %p for "
                             "switch %d\n",
                             (void *) fmRootApi->fmSwitchLockTable[swagId],
                             swagId);

                goto ABORT;
            }

            swagLocked = TRUE;

            /* If the switch is moving to an up state, the swag must also be
             * in the up state
             */
            if ( state &&
                !(FM_IS_STATE_ALIVE(fmRootApi->fmSwitchStateTable[swagId]->state)) )
            {
                err = FM_ERR_SWAG_NOT_UP;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }

            origSwagId = swagId;
            pass1      = FALSE;
            goto RESTART;
        }
    }
    else if (swagId != origSwagId)
    {
        UNLOCK_SWITCH(sw);
        UNLOCK_SWITCH(origSwagId);
        pass1    = TRUE;
        isInSwag = FALSE;
        goto RESTART;
    }
#endif

    if (!state)
    {
        switchPtr->state = FM_SWITCH_STATE_GOING_DOWN;

#if FM_SUPPORT_SWAG
        if (swagId >= 0)
        {
            /* The switch is moving to a down state, inform the switch
             * aggregate about the state change before the switch is
             * taken out of service
             */
            err = fmHandleSWAGSwitchStateChange(sw, swagId, state);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
#endif

        /* Free the pre-allocated resources */
        FM_API_CALL_FAMILY(err, switchPtr->FreeResources, sw);

        if (err != FM_OK)
        {
            FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                         "Unable to free pre-allocated resources, "
                         "err = %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
        }

        /* Just continue to cleanup even with errors */
    }

    /* Is the switch moving to an up state? */
    if ((switchPtr->state == FM_SWITCH_STATE_DOWN) && state)
    {
        switchPtr->state = FM_SWITCH_STATE_INIT;

        err = fmPlatformSwitchPreInitialize(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

#if FM_SUPPORT_SWAG
    /* If the switch is coming up, see if the switch aggregate wants to
     * perform any pre-initialization before the switch is brought up.
     */
    if (state && isInSwag)
    {
        err = fmPreInitializeSwitchInSWAG(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
#endif

    if (state)
    {
        /**************************************************
         * Switch is coming up.
         **************************************************/
        switchPtr->generateEventOnStaticAddr = 
                    fmGetBoolApiProperty(FM_AAK_API_MA_EVENT_ON_STATIC_ADDR, 
                                         FM_AAD_API_MA_EVENT_ON_STATIC_ADDR);

        switchPtr->generateEventOnDynamicAddr = 
                    fmGetBoolApiProperty(FM_AAK_API_MA_EVENT_ON_DYNAMIC_ADDR, 
                                         FM_AAD_API_MA_EVENT_ON_DYNAMIC_ADDR);

        FM_API_CALL_FAMILY(err, switchPtr->SetSwitchState, sw, state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* initialize data structures */
        err = fmInitializeSwitchDataStructure(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* Call platform layer post initialization */
        err = fmPlatformSwitchPostInitialize(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* The switch is now up, so we can proceed with the
         * event handling subsystem startup
         */
        FM_API_CALL_FAMILY(err, switchPtr->EventHandlingInitialize, sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* Perform switch-specific post-boot processing */
        FM_API_CALL_FAMILY(err, switchPtr->PostBootSwitch, sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if FM_SUPPORT_SWAG
        /* If the switch is in a switch aggregate, update the switch now that
         * the switch is up and we have a port table */
        if (swagId >= 0)
        {
            err = fmUpdateSwitchInSWAG(swagId, sw, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
#endif

#if FM_SUPPORT_SWAG
        if (swagId >= 0)
        {
            UNLOCK_SWITCH(sw);
            switchLocked = FALSE;

            err = fmHandleSWAGSwitchStateChange(sw, swagId, state);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
#endif

        err = FM_OK;
    }
    else
    {
        /**************************************************
         * Switch is shutting down.
         **************************************************/
        FM_API_CALL_FAMILY(err, switchPtr->SetSwitchState, sw, state);

        if (err != FM_OK)
        {
            FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                         "Error returned from switch-specific SetSwitchState, "
                         "sw = %d, state = %d, err = %d (%s)\n",
                         sw,
                         state,
                         err,
                         fmErrorMsg(err) );
        }

        /* Just continue to clean up even with errors */
        /* Clean up what is done in init */
        err = fmCleanupSwitchDataStructures(sw);
    }

#if FM_SUPPORT_SWAG
    if ( ( (err == FM_OK) || !state )
        && (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG)
        && fmGetBoolApiProperty(FM_AAK_API_SWAG_AUTO_SUB_SWITCHES, 
                                FM_AAD_API_SWAG_AUTO_SUB_SWITCHES) )
    {
        /* Automatic sub-switch maintenance is enabled, change the state
         * of all switches within the SWAG. */
        aggregatePtr = switchPtr->extension;
        member        = fmGetFirstSwitchInSWAG(aggregatePtr);
        while (member != NULL)
        {
            err = fmSetSwitchState(member->swId, state);
            switch (err)
            {
                case FM_OK:
                    break;

                case FM_ERR_INVALID_SWITCH:
                    err = FM_OK;
                    break;

                default:
                    FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                                 "Error returned from fmSetSwitchState for "
                                 "switch %d in swag %d, err = %d (%s)\n",
                                 member->swId,
                                 sw,
                                 err,
                                 fmErrorMsg(err) );

                    if (state)
                    {
                        goto ABORT;
                    }

                    break;
            }

            member = fmGetNextSwitchInSWAG(member);
        }

        if (state)
        {
            /* Initialize mailbox after all switches are up .*/
            if (switchPtr->switchModel == FM_SWITCH_MODEL_SWAG_C)
            {
                FM_API_CALL_FAMILY(err, switchPtr->MailboxInit, sw);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
        }
    }

    if (!state)
    {
        /* put the swag switch back into reset */
        if (switchPtr->ResetSwitch != NULL)
        {
            err = switchPtr->ResetSwitch(sw);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWAG, err);
        }
    }
#endif

    if (state)
    {
        switchPtr->state = FM_SWITCH_STATE_UP;
    }
    else
    {
        switchPtr->state = FM_SWITCH_STATE_DOWN;
    }

ABORT:

    if ((err != FM_OK)
        && (err != FM_ERR_SWAG_NOT_UP)
        && fmRootApi->fmSwitchStateTable[sw])
    {
        fmRootApi->fmSwitchStateTable[sw]->state = FM_SWITCH_STATE_FAILED;
    }

    if (switchLocked)
    {
        UNLOCK_SWITCH(sw);
    }

#if FM_SUPPORT_SWAG
    if ( swagLocked && (swagId >= 0) )
    {
        UNLOCK_SWITCH(swagId);
    }
#endif

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmSetSwitchState */




/*****************************************************************************/
/** fmGetSwitchState
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the simplified up/down state of a switch.
 *
 * \note            Use ''fmGetSwitchStateExt'' to obtain the detailed
 *                  switch state.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      state points to caller-allocated storage which will be
 *                  set to the simple switch state, where TRUE means up and 
 *                  FALSE means down.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if state is NULL.
 * \return          FM_FAIL if general failure accessing switch structures.
 *
 *****************************************************************************/
fm_status fmGetSwitchState(fm_int sw, fm_bool *state)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API( FM_LOG_CAT_SWITCH,
                     "sw=%d state=%p\n",
                     sw,
                     (void *) state);

    if (state == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_SWITCH_LOCK(sw);

    PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    if (switchPtr != NULL)
    {
        *state = (switchPtr->state == FM_SWITCH_STATE_UP)?TRUE:FALSE;
    }
    else
    {
        *state = FALSE;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmGetSwitchState */



/*****************************************************************************/
/** fmGetSwitchStateExt
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the detailed state of a switch.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      state points to caller-allocated storage which will be
 *                  set to the detailed switch state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if state is NULL.
 * \return          FM_FAIL if general failure accessing switch structures.
 *
 *****************************************************************************/
fm_status fmGetSwitchStateExt(fm_int sw, fm_switchState *state)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API( FM_LOG_CAT_SWITCH,
                     "sw=%d state=%p\n",
                     sw,
                     (void *) state);

    if (state == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_SWITCH_LOCK(sw);

    PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    if (switchPtr != NULL)
    {
        *state = switchPtr->state;
    }
    else
    {
        *state = FM_SWITCH_STATE_UNKNOWN;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmGetSwitchStateExt */




/*****************************************************************************/
/** fmSetEventHandler
 * \ingroup api
 *
 * \desc            Set or change the application's event handler call-back
 *                  function that will be responsible for handling events
 *                  reported by the API.
 *
 * \note            Sets the callback for events delivered to the current
 *                  process.  To determine which events are received by
 *                  the current process, call ''fmSetProcessEventMask''.
 *
 * \param[in]       eventHandlerFunc points to the event handler call-back
 *                  function. See ''fm_eventHandler'' for deatils on the
 *                  prototype of this call-back function and the nature
 *                  of events reported to it.
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fmSetEventHandler(fm_eventHandler eventHandlerFunc)
{
    FM_LOG_ENTRY_API(FM_LOG_CAT_API,
                     "eventHandlerFunc=%p\n",
                     (void *) (fm_uintptr) eventHandlerFunc);

    fmEventHandler = eventHandlerFunc;

    FM_LOG_EXIT_API(FM_LOG_CAT_API, FM_OK);

}   /* end fmSetEventHandler */




/*****************************************************************************/
/** fmSetSwitchEventHandler
 * \ingroup intApi
 *
 * \desc            Set or change a switch's event handler call-back
 *                  function that will be responsible for handling events
 *                  reported by the API.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       eventHandlerFunc points to the event handler call-back
 *                  function. See ''fm_switchEventHandler'' for details on the
 *                  prototype of this call-back function and the nature
 *                  of events reported to it.
 *
 * \return          FM_OK always.
 *
 *****************************************************************************/
fm_status fmSetSwitchEventHandler(fm_int                sw,
                                  fm_switchEventHandler eventHandlerFunc)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_API,
                     "sw=%d eventHandlerFunc=%p\n",
                     sw,
                     (void *) (fm_uintptr) eventHandlerFunc);
    
    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    switchPtr->eventHandler = eventHandlerFunc;

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_API, FM_OK);

}   /* end fmSetSwitchEventHandler */




/*****************************************************************************/
/** fmGetSwitchEventHandler
 * \ingroup intApi
 *
 * \desc            Return a switch's event handler call-back
 *                  function that will be responsible for handling events
 *                  reported by the API.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       eventHandlerFuncPtr is a pointer to caller-provided storage
 *                  into which the function will store the switch's event
 *                  handler function pointer.  If the switch is using the
 *                  global event handler, NULL will be stored.
 *
 * \return          FM_OK always.
 *
 *****************************************************************************/
fm_status fmGetSwitchEventHandler(fm_int                 sw,
                                  fm_switchEventHandler *eventHandlerFuncPtr)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_API,
                     "sw=%d, eventHandlerFuncPtr=%p\n",
                     sw,
                     (void *) eventHandlerFuncPtr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    *eventHandlerFuncPtr = switchPtr->eventHandler;

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_API, FM_OK);

}   /* end fmGetSwitchEventHandler */




/*****************************************************************************/
/** fmGetSwitchInfo
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve information about the switch
 *
 * \param[in]       sw is the switch for which to retrieve information.
 *
 * \param[out]      info is a pointer to an ''fm_switchInfo'' structure
 *                  to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmGetSwitchInfo(fm_int sw, fm_switchInfo *info)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d info=%p\n", sw, (fm_voidptr) info);

    err = fmGetSwitchInfoInternal(sw, info);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmGetSwitchInfo */




/*****************************************************************************/
/** fmGetSwitchInfoInternal
 * \ingroup intSwitch
 *
 * \desc            Retrieve information about the switch
 *
 * \note            This function provides the core of ''fmGetSwitchInfo''
 *                  but is an entry point to be called internally from the
 *                  API itself, bypassing the ''FM_LOG_ENTRY_API'' logging call.
 *
 * \param[in]       sw is the switch for which to retrieve information.
 *
 * \param[out]      info is a pointer to an ''fm_switchInfo'' structure
 *                  to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmGetSwitchInfoInternal(fm_int sw, fm_switchInfo *info)
{
    fm_status  err;
    fm_switch *swstate;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d info=%p\n", sw, (fm_voidptr) info);

    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    swstate = fmRootApi->fmSwitchStateTable[sw];

    /* check if switch exists or not, doesn't need to be up */
    /* Actually, due to bug:
     * http://internal.avlsi.com/bugzilla/show_bug.cgi?id=10441
     * GetSwitchInfo will not function correctly unless the switch is "up".
     * Though at the same time, "up" doesn't really mean the switch is up.
     */
    if (!swstate)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Switch not allocated\n");

        UNPROTECT_SWITCH(sw);

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_SWITCH);
    }

    if (swstate->state != FM_SWITCH_STATE_UP)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Switch not ready: current state is %d\n", 
                     swstate->state);
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_SWITCH_NOT_UP);
    }

    FM_API_CALL_FAMILY(err, swstate->GetSwitchInfo, sw, info);
    info->switchFamily  = swstate->switchFamily;
    info->switchModel   = swstate->switchModel;
    info->switchVersion = swstate->switchVersion;
    info->up            = (swstate->state == FM_SWITCH_STATE_UP)?TRUE:FALSE;

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmGetSwitchInfoInternal */




/*****************************************************************************/
/** fmGetSwitchModel
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve switch family and model.
 *
 * \param[in]       sw is the switch for which to retrieve information.
 *
 * \param[out]      family points to caller-provided storage into which the
 *                  switch family will be written. If NULL, no family value
 *                  will be returned.
 *
 * \param[out]      model points to caller-provided storage into which the
 *                  switch model will be written. If NULL, no model value
 *                  will be returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmGetSwitchModel(fm_int           sw,
                           fm_switchFamily *family,
                           fm_switchModel * model)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH,
                     "sw = %d, family = %p, model = %p\n",
                     sw,
                     (void *) family,
                     (void *) model);

    VALIDATE_SWITCH_LOCK(sw);

    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr == NULL)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_SWITCH);
    }

    if (family != NULL)
    {
        *family = switchPtr->switchFamily;
    }

    if (model != NULL)
    {
        *model = switchPtr->switchModel;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmGetSwitchModel */




/*****************************************************************************/
/* FindNextSwitch
 * \ingroup
 *
 * \desc            Helper function for finding the next switch in the
 *                  system.
 *
 * \param[in]       currentSwitch is the last switch number found by a
 *                  previous call to this function or fmGetSwitchFirst.
 *
 * \param[in]       nextSwitch points to caller-allocated storage where this
 *                  function should place the next switch number.
 *                  Will be set to -1 if no more switches found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_SWITCHES if no more switches.
 *
 *****************************************************************************/
static fm_status FindNextSwitch(fm_int currentSwitch, fm_int *nextSwitch)
{
    fm_int    sw;
    fm_status err = FM_OK;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "currentSwitch=%d nextSwitch=%p\n",
                         currentSwitch,
                         (void *) nextSwitch);

    for (sw = currentSwitch + 1 ; sw < FM_MAX_NUM_SWITCHES ; sw++)
    {
        if ( !SWITCH_LOCK_EXISTS(sw) )
        {
            continue;
        }

        /* Don't bother protecting access.  If we did protect it, we would
         * immediately release the protection after checking the pointer,
         * which means protecting it doesn't help the caller to know that
         * the switch number we return is still valid once he has it,
         * so we might as well just look at the value here without even
         * bothering with a lock.
         */
        if (fmRootApi->fmSwitchStateTable[sw] == NULL)
        {
            continue;
        }
        break;
    }

    *nextSwitch = sw;

    if (sw >= FM_MAX_NUM_SWITCHES)
    {
        *nextSwitch = -1;
        err         = FM_ERR_NO_SWITCHES;
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end FindNextSwitch */




/*****************************************************************************/
/** fmGetSwitchFirst
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first existing switch number.
 *
 * \note            This function reports the existence of a
 *                  switch regardless of switch state.  Callers MUST
 *                  either verify that the switch is up by calling
 *                  ''fmGetSwitchInfo'' prior to attempting any other operations
 *                  with the switch or must call ''fmSetSwitchState'' to bring
 *                  the switch up.  Any attempt to manipulate a switch that
 *                  has not been brought up will generate error code
 *                  ''FM_ERR_INVALID_SWITCH''.  Some low-level functions such as
 *                  ''fmReadUINT32'' can cause switch lockups if used prior to
 *                  bringing the switch up, so the caller must be very aware
 *                  of switch status before using such functions.
 *
 * \param[out]      firstSwitch points to caller-allocated storage where this
 *                  function should place the first switch number.
 *                  Will be set to -1 if no switches found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_SWITCHES if no switches.
 *
 *****************************************************************************/
fm_status fmGetSwitchFirst(fm_int *firstSwitch)
{
    fm_status err;

    FM_LOG_ENTRY_API_VERBOSE(FM_LOG_CAT_SWITCH,
                             "firstSwitch=%p\n",
                             (void *) firstSwitch);

    err = FindNextSwitch(-1, firstSwitch);

    FM_LOG_EXIT_API_VERBOSE(FM_LOG_CAT_SWITCH, err);


}   /* end fmGetSwitchFirst */




/*****************************************************************************/
/** fmGetSwitchNext
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next existing switch number.
 *
 * \note            This function reports the existence of a
 *                  switch regardless of switch state.  Callers MUST
 *                  either verify that the switch is up by calling
 *                  ''fmGetSwitchInfo'' prior to attempting any other operations
 *                  with the switch or must call ''fmSetSwitchState'' to bring
 *                  the switch up.  Any attempt to manipulate a switch that
 *                  has not been brought up will generate error code
 *                  ''FM_ERR_INVALID_SWITCH''.  Some low-level functions such as
 *                  ''fmReadUINT32'' can cause switch lockups if used prior to
 *                  bringing the switch up, so the caller must be very aware
 *                  of switch status before using such functions.
 *
 * \param[in]       currentSwitch is the last switch number found by a
 *                  previous call to this function or ''fmGetSwitchFirst''.
 *
 * \param[out]      nextSwitch points to caller-allocated storage where this
 *                  function should place the next switch number.
 *                  Will be set to -1 if no more switches found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if currentSwitch is invalid.
 * \return          FM_ERR_NO_SWITCHES if no more switches.
 *
 *****************************************************************************/
fm_status fmGetSwitchNext(fm_int currentSwitch, fm_int *nextSwitch)
{
    fm_status err;

    FM_LOG_ENTRY_API_VERBOSE(FM_LOG_CAT_SWITCH,
                             "currentSwitch=%d nextSwitch=%p\n",
                             currentSwitch,
                             (void *) nextSwitch);

    VALIDATE_SWITCH_INDEX(currentSwitch);

    err = FindNextSwitch(currentSwitch, nextSwitch);

    FM_LOG_EXIT_API_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fmGetSwitchNext */




/*****************************************************************************/
/** fmGetSwitchCount
 * \ingroup switch
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Returns the number of switches in the system.
 *
 * \param           None
 *
 * \return          Integer count of the number of switches.
 *
 * \note            The index of the last switch in the system is not
 *                  guaranteed to be equal to the number of switches
 *                  minus one!
 * 
 *****************************************************************************/
fm_uint fmGetSwitchCount(void)
{
    fm_uint count         = 0;
    fm_int  currentSwitch = -1;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "(no arguments)\n");

    if (fmGetSwitchFirst(&currentSwitch) == FM_ERR_NO_SWITCHES)
    {
        FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_SWITCH, 0, "No switches\n");
    }

    count = 1;

    while (fmGetSwitchNext(currentSwitch, &currentSwitch) == FM_OK)
    {
        ++count;
    }

    FM_LOG_PRINTF(FM_LOG_CAT_SWITCH,
                  FM_LOG_LEVEL_FUNC_EXIT,
                  "Returning %d\n",
                  count);

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_SWITCH, count, "%d switches\n", count);

}   /* end fmGetSwitchCount */




/*****************************************************************************/
/** fmLockSwitch
 * \ingroup intApi
 *
 * \desc            This function creates a switch lock, if needed, then
 *                  takes a write lock on it.
 *
 * \param[in]       sw is the switch number of the switch being locked.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmLockSwitch(fm_int sw)
{
    fm_status status = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    if (fmRootApi->fmSwitchLockTable[sw] == NULL)
    {
        /* The switch lock structure is created once for each
         *  switch.  Once created, it MUST NOT be deleted!  Code
         *  throughout the system relies upon this pointer being
         *  NULL if the switch has never been present.  It is not
         *  possible to avoid thread failure if the lock structure
         *  could ever disappear out from under a thread.  So, while
         *  it is legal to create and delete the actual switch state
         *  structure as needed, the lock pointer must never be
         *  deleted once created.
         */
        char       lockName[20];
        fm_rwLock *swLock;

        FM_SNPRINTF_S(lockName, sizeof(lockName), "SwitchRwLock%d", sw);
        swLock = (fm_rwLock *) fmAlloc( sizeof(fm_rwLock) );
        if (swLock == NULL)
        {
            status = FM_ERR_NO_MEM;
        }
        else
        {
            memset( swLock, 0, sizeof(fm_rwLock) );
            status = fmCreateRwLockV2(lockName, 
                                      sw,
                                      FM_LOCK_PREC_SWITCH, 
                                      swLock);
        }

        if (status != FM_OK)
        {
            FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                         "Unable to create switch R/W "
                         "Lock for switch %d\n",
                         sw);
        }
        else
        {
            fmRootApi->fmSwitchLockTable[sw] = swLock;
        }
    }

    /* Take write access to the switch lock */
    if (status == FM_OK)
    {
        status = LOCK_SWITCH(sw);

        if (status != FM_OK)
        {
            FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                         "Unable to capture switch write lock %p "
                         "for switch %d: %s\n",
                         (void *) fmRootApi->fmSwitchLockTable[sw],
                         sw,
                         fmErrorMsg(status) );
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmLockSwitch */




/*****************************************************************************/
/** fmHandleSwitchInserted
 * \ingroup intApi
 *
 * \desc            This function is called by the global event handler
 *                  (fmGLobalEventHandler) when a switch inserted
 *                  event is received.  This function handles all the initial
 *                  setup necessary to access said switch.
 *
 * \param[in]       sw is the switch number of the switch being inserted.
 *
 * \param[in]       insertEvent is the event information structure for
 *                  this event, notably containing the slot number.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmHandleSwitchInserted(fm_int sw, fm_eventSwitchInserted *insertEvent)
{
    fm_switch *      swptr;
    fm_status        status;
    fm_int           i;
    fm_switchFamily  family;
    fm_switchModel   model;
    fm_switchVersion version;
    fm_int           swagId = -1;
    fm_bool          isInSWAG;
    fm_bool          swagLocked = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d, insertEvent=%p<slot=%d,family=%d>\n",
                 sw,
                 (void *) insertEvent,
                 insertEvent ? insertEvent->slot : -1,
                 insertEvent ? (fm_int)insertEvent->family : -1);

    if (insertEvent == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    /* Some platform must first initialize management path prior to access
     * the switch. */
    status = fmPlatformSwitchPreInsert(sw);
    if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

#if FM_SUPPORT_SWAG
    /* BEFORE taking the switch lock, determine if the new switch is a member
     * of a switch aggregate.  If it is, lock the switch aggregate first.
     * This is to prevent deadlocks */
    status = fmIsSwitchInASWAG(sw, &swagId);

    if (status == FM_OK)
    {
        isInSWAG = TRUE;
    }
    else if (status == FM_ERR_SWITCH_NOT_IN_AGGREGATE)
    {
        isInSWAG = FALSE;
    }
    else
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }
#else
    isInSWAG = FALSE;
#endif

    if (isInSWAG)
    {
        status = fmLockSwitch(swagId);

        if (status != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
        }

        swagLocked = TRUE;
    }

    status = fmLockSwitch(sw);
    if (status != FM_OK)
    {
        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

    if (fmRootApi->fmSwitchStateTable[sw] != NULL)
    {
        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }
        UNLOCK_SWITCH(sw);
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);
    }

    swptr = (fm_switch *) fmAlloc( sizeof(fm_switch) );

    if (swptr == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Inserted Event Handler unable to allocate "
                     "switch state table for switch %d\n",
                     sw);

        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
    }

    FM_CLEAR(*swptr);

    /* Note: Any needed structure initialization MUST take place
     *  before the switch state table is updated! */
    /* no struct initialization, just set its state to down */
    swptr->switchNumber  = sw;
    swptr->swag          = -1;
    swptr->dropEcmpGroup = -1;

    switch (insertEvent->model)
    {
#if FM_SUPPORT_SWAG
        case FM_SWITCH_MODEL_SWAG_A:
        case FM_SWITCH_MODEL_SWAG_B:
        case FM_SWITCH_MODEL_SWAG_C:
            swptr->switchFamily  = FM_SWITCH_FAMILY_SWAG;
            swptr->switchModel   = insertEvent->model;
            swptr->switchVersion = FM_SWITCH_VERSION_UNKNOWN;
            break;
#endif

        default:
            swptr->switchFamily  = insertEvent->family;
            /* Older platforms might have this field set to -1 */
            if (insertEvent->model >= FM_SWITCH_MODEL_MAX)
            {
                swptr->switchModel = FM_SWITCH_MODEL_UNKNOWN;
            }
            else
            {
                swptr->switchModel = insertEvent->model;
            }
            swptr->switchVersion = insertEvent->version;
            break;
    }

    swptr->state                      = FM_SWITCH_STATE_DOWN;
    fmRootApi->fmSwitchStateTable[sw] = swptr;

    /* make sure the access locks get initialized */
    swptr->accessLocksInitialized = FALSE;

    /***************************************************
     * Must happen first in case other init funcs need
     * these locks.
     **************************************************/
    if ( ( status = fmAllocateSwitchLocks(swptr) ) != FM_OK )
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to allocate switch locks "
                     "(fmAllocateSwitchLocks returned error "
                     "%d(%s) for switch %d)!\n",
                     status,
                     fmErrorMsg(status),
                     sw);
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

    /* call the platform switch init function to populate the
     *  switch table with function pointers needed for chip
     *  identification */
    status = fmPlatformSwitchInitialize(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Inserted Event Handler: "
                     "fmPlatformSwitchInitialize (1st call) returned error "
                     "%d(%s) for switch %d\n",
                     status,
                     fmErrorMsg(status),
                     sw);

        fmFreeSwitchLocks(swptr);
        fmFree(fmRootApi->fmSwitchStateTable[sw]);
        fmRootApi->fmSwitchStateTable[sw] = NULL;

        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        /* Undo what is done before */
        fmPlatformSwitchTerminate(sw);
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

    if ( (swptr->switchModel != FM_SWITCH_MODEL_UNKNOWN) &&
         (swptr->switchFamily != FM_SWITCH_FAMILY_UNKNOWN) )
    {
        goto ALREADY_IDENTIFIED;
    }

    /* search for the chip model by calling each identification
     * function in turn, until one of the functions returns FM_OK
     * or we run out of functions.  In the latter case, the switch
     * model cannot be determined and the switch cannot be
     * supported.
     */
    family  = FM_SWITCH_FAMILY_UNKNOWN;
    model   = FM_SWITCH_MODEL_UNKNOWN;
    version = FM_SWITCH_VERSION_UNKNOWN;
    i       = 0;

    if ( !fmGetBoolApiProperty(FM_AAK_DEBUG_BOOT_IDENTIFYSWITCH,
                               FM_AAD_DEBUG_BOOT_IDENTIFYSWITCH) )
    {
        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);
    }


    /***************************************************
     * We need to do a reset cycle here, otherwise we
     * cannot guarantee the state of the chip at
     * this point, which is necessary for the identify
     * to work.
     **************************************************/
    fmPlatformReset(sw);

    fmDelay( 0, fmGetIntApiProperty(FM_AAK_API_BOOT_RESET_TIME,
                                    FM_AAD_API_BOOT_RESET_TIME) );

    fmPlatformRelease(sw);

    /***************************************************
     * Now the switch is out of reset, so iterate the
     * identifier functions to identify the switch.
     **************************************************/

    while (IdentifySwitch[i] != NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Calling identification handler #%d...\n",
                     i);

        status = IdentifySwitch[i](sw, &family, &model, &version);

        if (status != FM_OK)
        {
            FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                         "Failed to identify(%d) switch #%d. Error: %d(%s)\n",
                         i,
                         sw,
                         status,
                         fmErrorMsg(status));


            /* Clean up here since no switch is found 
             * Undo what is done before
             */

            fmFree(fmRootApi->fmSwitchStateTable[sw]);
            fmRootApi->fmSwitchStateTable[sw] = NULL;

            UNLOCK_SWITCH(sw);

            if (swagLocked)
            {
                UNLOCK_SWITCH(swagId);
            }

            fmPlatformSwitchTerminate(sw);
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_SWITCH);
        }

        if (family != FM_SWITCH_FAMILY_UNKNOWN)
        {
            swptr->switchFamily  = family;
            swptr->switchModel   = model;
            swptr->switchVersion = version;

            break;
        }

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Identify handler failed, attempting next handler...\n");

        i++;
    }

ALREADY_IDENTIFIED:

    if ( (swptr->switchFamily == FM_SWITCH_FAMILY_UNKNOWN) ||
         (swptr->switchModel  == FM_SWITCH_MODEL_UNKNOWN) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Inserted Event Handler unable to "
                     "identify switch %d chip model\n",
                     sw);

        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_FAIL);
    }

    /* The chip model has been identified, let the platform
     * code update any needed function pointers */
    status = fmPlatformSwitchInitialize(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Inserted Event Handler: "
                     "fmPlatformSwitchInitialize (2nd call) returned "
                     "error %d(%s) for switch %d\n",
                     status,
                     fmErrorMsg(status),
                     sw);

        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

    /* Allocate the rest of the switch and port structures */
    status = fmAllocateSwitchDataStructures(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH, 
                     "%s: Failed to allocate switch data structures...\n",
                     fmErrorMsg(status));

        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

    /* The switch is inserted/initialized in the API, let the platform
     * code perform some initialization that requires the presence of
     * the switch. */
    status = fmPlatformSwitchInserted(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Inserted Event Handler: "
                     "fmPlatformSwitchInserted returned "
                     "error %d(%s) for switch %d\n",
                     status,
                     fmErrorMsg(status),
                     sw);

        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }

    /* Initialize the debug register table for the switch */
    fmDbgInitSwitchRegisterTable(sw);

#if FM_SUPPORT_SWAG
    if (swptr->switchFamily != FM_SWITCH_FAMILY_SWAG)
    {
#endif
        if ( fmGetBoolApiProperty(FM_AAK_DEBUG_BOOT_RESET,
                                  FM_AAD_DEBUG_BOOT_RESET) )
        {
            if (swptr->ResetSwitch != NULL)
            {
                /* put the chip back into reset */
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                             "Putting chip %d into reset... \n",
                             sw);
                swptr->ResetSwitch(sw);
            }
            else
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                             "Unabled to put chip %d into reset, "
                             "no ResetSwitch function pointer\n",
                             sw);
            }
        }
#if FM_SUPPORT_SWAG
    }
#endif

#if FM_SUPPORT_SWAG
    /* If the switch is in a switch aggregate, update the switch accordingly */
    status = fmUpdateSwitchInSWAG(swagId, sw, isInSWAG);
    if (status != FM_OK)
    {
        UNLOCK_SWITCH(sw);

        if (swagLocked)
        {
            UNLOCK_SWITCH(swagId);
        }

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);
    }
#endif

    /* release the write lock */
    status = UNLOCK_SWITCH(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Inserted Event Handler unable to release "
                     "switch write lock %p for switch %d\n",
                     (void *) fmRootApi->fmSwitchLockTable[sw],
                     sw);
    }

    /* If a switch aggregate lock is held, release it */
    if (swagLocked)
    {
        UNLOCK_SWITCH(swagId);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmHandleSwitchInserted */




/*****************************************************************************/
/** fmHandleSwitchRemoved
 * \ingroup intApi
 *
 * \desc            This function is called by the global event handler
 *                  (fmGLobalEventHandler) when a switch removed
 *                  event is received.  This function handles all the teardown
 *                  necessary to cleanup after a switch is removed.  Note that
 *                  this function assumes that the caller generating this
 *                  event has already set the state of the switch to DOWN.
 *
 * \param[in]       sw is the switch number of the switch being removed.
 *
 * \param[in]       removeEvent is the event information structure for
 *                  this event, notably containing the slot number.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmHandleSwitchRemoved(fm_int sw, fm_eventSwitchRemoved *removeEvent)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "removeEvent.slot=%d\n",
                 removeEvent ? removeEvent->slot : -1);

    if (removeEvent)
    {
        sw = removeEvent->slot;
    }

    /* Take write access to the switch lock */
    status = LOCK_SWITCH(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Removed Event Handler unable to "
                     "capture switch write lock %p for switch %d\n",
                     (void *) fmRootApi->fmSwitchLockTable[sw],
                     sw);

        return status;
    }

    /* Switch might go directly from up to removed */
    if (fmRootApi->fmSwitchStateTable[sw]->state > FM_SWITCH_STATE_DOWN)
    {
        /* Bring the switch down first */
        (void)fmSetSwitchState(sw, FALSE);
    }

    /* Free all component structures */
    status = fmFreeSwitchDataStructures(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                     "Error freeing switch data structures for switch #%d: %s\n",
                     sw,
                     fmErrorMsg(status) );
    }


    /***************************************************
     * Free the switch extension, this should not be
     * used by anything else afterwards.  Note that
     * this relies on fmFreeSwitchDataStructures 
     * handling any switch specific allocated 
     * structures within the extension.
     **************************************************/
    fmFree(fmRootApi->fmSwitchStateTable[sw]->extension);

    fmFree(fmRootApi->fmSwitchStateTable[sw]);
    fmRootApi->fmSwitchStateTable[sw] = NULL;

    /* release the write lock */
    status = UNLOCK_SWITCH(sw);

    if (status != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Switch Removed Event Handler unable to release "
                     "switch write lock %p for switch %d\n",
                     (void *) fmRootApi->fmSwitchLockTable[sw],
                     sw);
    }

    status = fmPlatformSwitchTerminate(sw);
    if (status != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                     "Error doing platform clean up for switch #%d: %s\n",
                     sw, fmErrorMsg(status) );
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmHandleSwitchRemoved */


