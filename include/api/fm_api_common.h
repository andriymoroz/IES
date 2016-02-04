/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_common.h
 * Creation Date:   June 28, 2005
 * Description:     Constants shared by multiple software components
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

#ifndef __FM_FM_API_COMMON_H
#define __FM_FM_API_COMMON_H


/* Used in the VCNT field of the VLAN Table when the VLAN is
 * not associated with a counter.
 * By default, the counter id entry for each VLAN is 0. This will result in
 * traffic on each VLAN being registered to counter set zero until a counter
 * is allocated for that VLAN via fmAllocateVLANCounters
 */
#define FM_UNUSED_VLAN_COUNTER_ID     0

/** \ingroup constSystem
 *  The number of internal switch priorities. */
#define FM_SWITCH_PRIORITY_MAX        16

/** \ingroup constSystem
 *  A logical port value specifying that no logical port exists. */
#define FM_LOGICAL_PORT_NONE          -2

/** \ingroup constSystem
 *  A logical port value specifying that any logical port can be used. */
#define FM_LOGICAL_PORT_ANY           -1

/* A handle value specifying that no pre-allocated handle exists. */
#define FM_HANDLE_NONE                FM_LOGICAL_PORT_NONE

/***************************************************
 * Special logical port numbers.
 **************************************************/

/** \ingroup constSystem
 *  The defined broadcast logical port. */
#define FM_PORT_BCAST           (FM_MAX_LOGICAL_PORT - 1)

/** \ingroup constSystem
 *  The defined flood logical port. */
#define FM_PORT_FLOOD           (FM_MAX_LOGICAL_PORT - 2)

/** \ingroup constSystem
 *  The defined drop logical port. */
#define FM_PORT_DROP            (FM_MAX_LOGICAL_PORT - 3)

/* Special port used by the MAC table maintenance code in the FM4000. */
#define FM_PORT_NOP_PURGE       (FM_MAX_LOGICAL_PORT - 4)

/* Special port for handling Reverse Path Forwarding failures. */
#define FM_PORT_RPF_FAILURE     (FM_MAX_LOGICAL_PORT - 5)

#define FM_PORT_NOP_FLOW        (FM_MAX_LOGICAL_PORT - 6)
#define FM_PORT_FLOOD_CPU       (FM_MAX_LOGICAL_PORT - 7)   /* FM6000 */

/* Special port used to manage the multicast flooding glort
 * in the FM10000. */
#define FM_PORT_MCAST             (FM_MAX_LOGICAL_PORT - 8)

/* Special ports for handling undefined NAT and DeNAT Flow. These ports 
 * will map to CPU port's glort. It doesn't have any glort mapping on
 * its own. Hence it is chosen outside of FM_MAX_LOGICAL_PORT. */ 
#define FM_PORT_NAT_UNDEF_FLOW    (FM_MAX_LOGICAL_PORT + 1)

#define FM_PORT_DENAT_UNDEF_FLOW  (FM_MAX_LOGICAL_PORT + 2)

/** Use to determine whether switch is accessible */
#define FM_IS_STATE_ALIVE(state) \
    (((state) >= FM_SWITCH_STATE_BOOT_DONE) && ((state) <= FM_SWITCH_STATE_UP))

#ifndef FM_SET_MEM_i
/** Initializes a single-word, 1-dimensional register. */
#define FM_SET_MEM_i(switchPtr, in, reg, value)                               \
    {                                                                         \
        fm_int i;                                                             \
        for (i = 0 ; i < (in) ; i++)                                          \
        {                                                                     \
            (switchPtr)->WriteUINT32((switchPtr)->switchNumber,               \
                                     reg(i),                                  \
                                     (value));                                \
        }                                                                     \
    }
#endif  /* FM_SET_MEM_i */
                                                                    
#ifndef FM_SET_MEM_iw
/** Initializes a multi-word, 1-dimensional register. */
#define FM_SET_MEM_iw(switchPtr, in, wn, reg, value)                          \
    {                                                                         \
        fm_int i, w;                                                          \
        for (i = 0 ; i < (in) ; i++)                                          \
        {                                                                     \
            for (w = 0 ; w < (wn) ; w++)                                      \
            {                                                                 \
                (switchPtr)->WriteUINT32((switchPtr)->switchNumber,           \
                                         reg(i, w),                           \
                                         (value));                            \
            }                                                                 \
        }                                                                     \
    }
#endif  /* FM_SET_MEM_iw */

#ifndef FM_SET_MEM_ij
/** Initializes a single-word, 2-dimensional register. */
#define FM_SET_MEM_ij(switchPtr, in, jn, reg, value)                          \
    {                                                                         \
        fm_int i, j;                                                          \
        for (i = 0 ; i < (in) ; i++)                                          \
        {                                                                     \
            for (j = 0 ; j < (jn) ; j++)                                      \
            {                                                                 \
                (switchPtr)->WriteUINT32((switchPtr)->switchNumber,           \
                                         reg(i, j),                           \
                                         (value));                            \
            }                                                                 \
        }                                                                     \
    }
#endif  /* FM_SET_MEM_ij */

#ifndef FM_SET_MEM_ijw
/** Initializes a multi-word, 2-dimensional register. */
#define FM_SET_MEM_ijw(switchPtr, in, jn, wn, reg, value)                     \
    {                                                                         \
        fm_int i, j, w;                                                       \
        for (i = 0 ; i < (in) ; i++)                                          \
        {                                                                     \
            for (j = 0 ; j < (jn) ; j++)                                      \
            {                                                                 \
                for (w = 0 ; w < (wn) ; w++)                                  \
                {                                                             \
                    (switchPtr)->WriteUINT32((switchPtr)->switchNumber,       \
                                             reg(i, j, w),                    \
                                             (value));                        \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }
#endif  /* FM_SET_MEM_ijw */
                                                                
#ifndef FM_SET_MEM_ijk
/** Initializes a single-word, 3-dimensional register. */
#define FM_SET_MEM_ijk(switchPtr, in, jn, kn, reg, value)                     \
    {                                                                         \
        fm_int i, j, k;                                                       \
        for (i = 0 ; i < (in) ; i++)                                          \
        {                                                                     \
            for (j = 0 ; j < (jn) ; j++)                                      \
            {                                                                 \
                for (k = 0 ; k < (kn) ; k++)                                  \
                {                                                             \
                    (switchPtr)->WriteUINT32((switchPtr)->switchNumber,       \
                                             reg(i, j, k),                    \
                                             (value));                        \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }
#endif  /* FM_SET_MEM_ijk */

#ifndef FM_SET_MEM_ijkw
/** Initializes a multi-word, 3-dimensional register. */
#define FM_SET_MEM_ijkw(switchPtr, in, jn, kn, wn, reg, value)                \
    {                                                                         \
        fm_int i, j, k, w;                                                    \
        for (i = 0 ; i < (in) ; i++)                                          \
        {                                                                     \
            for (j = 0 ; j < (jn) ; j++)                                      \
            {                                                                 \
                for (k = 0 ; k < (kn) ; k++)                                  \
                {                                                             \
                    for (w = 0 ; w < (wn) ; w++)                              \
                    {                                                         \
                        (switchPtr)->WriteUINT32((switchPtr)->switchNumber,   \
                                                 reg(i, j, k, w),             \
                                                 (value));                    \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }
#endif  /* FM_SET_MEM_ijkw */

/** Check a single-word, 1-dimensional register array */
#define FM_CHECK_MEM_i_(sw, pErr, in, reg, value)                             \
    {                                                                         \
        fm_int i;                                                             \
        fm_uint readValue;                                                    \
        fm_switch* switchPtr = GET_SWITCH_PTR(sw);                            \
        *pErr = FM_OK;                                                        \
        for (i = 0 ; i < (in) && (*pErr == FM_OK) ; i++)                      \
        {                                                                     \
            (switchPtr)->ReadUINT32( sw,                                      \
                                     reg(i),                                  \
                                     &readValue);                             \
            if (readValue != (value))                                         \
            {                                                                 \
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH," " #reg "[%d]  "              \
                             "expected = 0x%8.8x   read = 0x%8.8x\n",         \
                              i, value, readValue);                           \
                *pErr = FM_FAIL;                                              \
            }                                                                 \
        }                                                                     \
    }

/** Check a multi-word, 1-dimensional register array */
#define FM_CHECK_MEM_iw_(sw, pErr, in, wn, reg, value)                        \
    {                                                                         \
        fm_int i, w;                                                          \
        fm_uint readValue;                                                    \
        fm_switch* switchPtr = GET_SWITCH_PTR(sw);                            \
        *pErr = FM_OK;                                                        \
        for (i = 0 ; i < (in) && (*pErr == FM_OK); i++)                       \
        {                                                                     \
            for (w = 0 ; w < (wn) && (*pErr == FM_OK) ; w++)                  \
            {                                                                 \
                (switchPtr)->ReadUINT32( sw,                                  \
                                         reg(i, w),                           \
                                         &readValue);                         \
                if (readValue != (value))                                     \
                {                                                             \
                    FM_LOG_ERROR(FM_LOG_CAT_SWITCH," " #reg " [%d], w=%d  "   \
                                 "expected = 0x%8.8x   read = 0x%8.8x\n",     \
                                  i, w, value, readValue);                    \
                    *pErr = FM_FAIL;                                          \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }

/** Check a single-word, 2-dimensional register array */
#define FM_CHECK_MEM_ij_(sw, pErr, in, jn, reg, value)                        \
    {                                                                         \
        fm_int i, j;                                                          \
        fm_uint readValue;                                                    \
        fm_switch* switchPtr = GET_SWITCH_PTR(sw);                            \
        *pErr = FM_OK;                                                        \
        for (i = 0 ; i < (in) && (*pErr == FM_OK) ; i++)                      \
        {                                                                     \
            for (j = 0 ; j < (jn) && (*pErr == FM_OK) ; j++)                  \
            {                                                                 \
                (switchPtr)->ReadUINT32( sw,                                  \
                                         reg(i, j),                           \
                                         &readValue);                         \
                if (readValue != (value) )                                    \
                {                                                             \
                    FM_LOG_ERROR(FM_LOG_CAT_SWITCH," " #reg " [%d,%d]  "      \
                                 "expected = 0x%8.8x   read = 0x%8.8x\n",     \
                                  i, j, value, readValue);                    \
                    *pErr = FM_FAIL;                                          \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }

/** Check a multi-word, 2-dimensional register array */
#define FM_CHECK_MEM_ijw_(sw, pErr, in, jn, wn, reg, value)                   \
    {                                                                         \
        fm_int i, j, w;                                                       \
        fm_uint readValue;                                                    \
        fm_switch* switchPtr = GET_SWITCH_PTR(sw);                            \
        *pErr = FM_OK;                                                        \
        for (i = 0 ; i < (in) && (*pErr == FM_OK); i++)                       \
        {                                                                     \
            for (j = 0 ; j < (jn) && (*pErr == FM_OK); j++)                   \
            {                                                                 \
                for (w = 0 ; w < (wn) && (*pErr == FM_OK) ; w++)              \
                {                                                             \
                    (switchPtr)->ReadUINT32( sw,                              \
                                             reg(i, j, w),                    \
                                             &readValue);                     \
                    if (readValue != (value) )                                \
                    {                                                         \
                        FM_LOG_ERROR(FM_LOG_CAT_SWITCH," " #reg " [%d,%d], "  \
                                     "w= %d  expected = 0x%8.8x   "           \
                                     "read = 0x%8.8x\n",                      \
                                      i, j, w, value, readValue);             \
                        *pErr = FM_FAIL;                                      \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }
                                                                
/** Check a single-word, 3-dimensional register array */
#define FM_CHECK_MEM_ijk_(sw, pErr, in, jn, kn, reg, value)                   \
    {                                                                         \
        fm_int i, j, k;                                                       \
        fm_uint readValue;                                                    \
        fm_switch* switchPtr = GET_SWITCH_PTR(sw);                            \
        fm_int isOk = 1;                                                      \
        for (i = 0 ; i < (in) && (*pErr == FM_OK); i++)                       \
        {                                                                     \
            for (j = 0 ; j < (jn) && (*pErr == FM_OK); j++)                   \
            {                                                                 \
                for (k = 0 ; k < (kn) && (*pErr == FM_OK) ; k++)              \
                {                                                             \
                    (switchPtr)->ReadUINT32( sw,                              \
                                             reg(i, j, k),                    \
                                             &readValue);                     \
                    if (readValue != (value) )                                \
                    {                                                         \
                        FM_LOG_ERROR(FM_LOG_CAT_SWITCH," " #reg " [%d,%d,%d]" \
                                     "  expected = 0x%8.8x   read = 0x%8.8x\n", \
                                     i, j, k, value, readValue);              \
                        *pErr = FM_FAIL;                                      \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }

/** Check a multi-word, 3-dimensional register array */
#define FM_CHECK_MEM_ijkw_(sw, pErr, in, jn, kn, wn, reg, value)              \
    {                                                                         \
        fm_int i, j, k, w;                                                    \
        fm_uint readValue;                                                    \
        fm_switch* switchPtr = GET_SWITCH_PTR(sw);                            \
        *pErr = FM_OK;                                                        \
        for (i = 0 ; i < (in) && (*pErr == FM_OK); i++)                       \
        {                                                                     \
            for (j = 0 ; j < (jn) && (*pErr == FM_OK) ; j++)                  \
            {                                                                 \
                for (k = 0 ; k < (kn) && (*pErr == FM_OK); k++)               \
                {                                                             \
                    for (w = 0 ; w < (wn) && (*pErr == FM_OK) ; w++)          \
                    {                                                         \
                        (switchPtr)->ReadUINT32( sw,                          \
                                                 reg(i, j, k, w),             \
                                                 &readValue);                 \
                        if (readValue != (value) )                            \
                        {                                                     \
                            FM_LOG_ERROR(FM_LOG_CAT_SWITCH," " #reg " [%d,%d,%d],"\
                                         " w=%d  expected = 0x%8.8x   read = 0x%8.8x\n",\
                                         i, j, k, w, value, readValue);       \
                            *pErr = FM_FAIL;                                  \
                        }                                                     \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * switch family appearing in the ''fm_switchInfo''
 * structure returned by ''fmGetSwitchInfo''.
 **************************************************/
typedef enum
{
    /** Unknown switch family. */
    FM_SWITCH_FAMILY_UNKNOWN = 0,

    /** FM2000 switch family. */
    FM_SWITCH_FAMILY_FM2000,

    /** FM4000 switch family. */
    FM_SWITCH_FAMILY_FM4000,

    /** Remote Access FM4000 switch family. */
    FM_SWITCH_FAMILY_REMOTE_FM4000,

    /** Switch Aggregate */
    FM_SWITCH_FAMILY_SWAG,

    /** FM6000 switch family.  */
    FM_SWITCH_FAMILY_FM6000,

    /** Remote Access FM6000 switch family.  */
    FM_SWITCH_FAMILY_REMOTE_FM6000,

    /** FM10000 switch family. */
    FM_SWITCH_FAMILY_FM10000,

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_FAMILY_MAX

} fm_switchFamily;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * switch model appearing in the ''fm_switchInfo''
 * structure returned by ''fmGetSwitchInfo''.
 **************************************************/
typedef enum
{
    /** Unknown Switch Model. */
    FM_SWITCH_MODEL_UNKNOWN = 0,

    /** FM2224 Switch. 
     *  \chips  FM2000 */
    FM_SWITCH_MODEL_FM2224,

    /** FM2112 Switch. 
     *  \chips  FM2000 */
    FM_SWITCH_MODEL_FM2112,

    /** FM4224 Switch. 
     *  \chips  FM3000, FM4000 */
    FM_SWITCH_MODEL_FM4224,

    /** Legacy Switch Aggregate. Obsolete, use FM_SWITCH_MODEL_SWAG_A. */
    FM_SWITCH_MODEL_SWAG,

    /** Legacy Switch Aggregate.
     *  \chips FM4000 */
    FM_SWITCH_MODEL_SWAG_A = FM_SWITCH_MODEL_SWAG,

    /** FM6000 Switch Aggregate.
     *  \chips FM6000 */
    FM_SWITCH_MODEL_SWAG_B,

    /** FM10000 Switch Aggregate.
     *  \chips FM10000 */
    FM_SWITCH_MODEL_SWAG_C,

    /** FM10440 Switch.
     *  \chips  FM10000 */
    FM_SWITCH_MODEL_FM10440,

    /** FM6224 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6224,

    /** FM6364 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6364,

    /** FM6348 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6348,

    /** FM6324 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6324,

    /** FM6764 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6764,

    /** FM6748 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6748,

    /** FM6724 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM6724,

    /** FM5224 Switch.  
     *  \chips  FM6000 */
    FM_SWITCH_MODEL_FM5224,
    
    /** FM10840 Switch.
     *  \chips  FM10000 */
    FM_SWITCH_MODEL_FM10840,

    /** FM10420 Switch.
     *  \chips  FM10000 */
    FM_SWITCH_MODEL_FM10420,

    /** FM10064 Switch.
     *  \chips  FM10000 */
    FM_SWITCH_MODEL_FM10064,

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_MODEL_MAX

} fm_switchModel;


/**************************************************/
/** \ingroup typeEnum
 *  Set of possible part number values for each
 *  switch family. These enumerated values are returned by 
 *  the platform interface function, 
 *  ''fmPlatformGetSwitchPartNumber''.
 **************************************************/
typedef enum
{
    /** Unknown Switch Part Number. */
    FM_SWITCH_PART_NUM_UNKNOWN = 0,     /* 0 */

    /** FM2224 Switch. */
    FM_SWITCH_PART_NUM_FM2224,          

    /** FM2220 Switch. */
    FM_SWITCH_PART_NUM_FM2220,          

    /** FM2212 Switch. */
    FM_SWITCH_PART_NUM_FM2212,          

    /** FM2208 Switch. */
    FM_SWITCH_PART_NUM_FM2208,          

    /** FM2112 Switch. */
    FM_SWITCH_PART_NUM_FM2112,          /* 5 */

    /** FM2104 Switch. */
    FM_SWITCH_PART_NUM_FM2104,          

    /** FM2103 Switch. */
    FM_SWITCH_PART_NUM_FM2103,          

    /** FM3224 Switch. */
    FM_SWITCH_PART_NUM_FM3224,          

    /** FM3220 Switch. */
    FM_SWITCH_PART_NUM_FM3220,          

    /** FM3212 Switch. */
    FM_SWITCH_PART_NUM_FM3212,          /* 10 */

    /** FM3208 Switch. */
    FM_SWITCH_PART_NUM_FM3208,          

    /** FM3112 Switch. */
    FM_SWITCH_PART_NUM_FM3112,          

    /** FM3104 Switch. */
    FM_SWITCH_PART_NUM_FM3104,          

    /** FM3103 Switch. */
    FM_SWITCH_PART_NUM_FM3103,          

    /** FM3410 Switch. */
    FM_SWITCH_PART_NUM_FM3410,          /* 15 */

    /** FM4224 Switch. */
    FM_SWITCH_PART_NUM_FM4224,          

    /** FM4220 Switch. */
    FM_SWITCH_PART_NUM_FM4220,          

    /** FM4212 Switch. */
    FM_SWITCH_PART_NUM_FM4212,          

    /** FM4208 Switch. */
    FM_SWITCH_PART_NUM_FM4208,          

    /** FM4112 Switch. */
    FM_SWITCH_PART_NUM_FM4112,          /* 20 */

    /** FM4104 Switch. */
    FM_SWITCH_PART_NUM_FM4104,          

    /** FM4103 Switch. */
    FM_SWITCH_PART_NUM_FM4103,          

    /** FM4410 Switch. */
    FM_SWITCH_PART_NUM_FM4410,          

    /** FM10124 Switch. */
    FM_SWITCH_PART_NUM_FM10124,          

    /** FM10136 Switch. */
    FM_SWITCH_PART_NUM_FM10136,          /* 25 */

    /** FM10148 Switch. */
    FM_SWITCH_PART_NUM_FM10148,          

    /** FM10424 Switch. */
    FM_SWITCH_PART_NUM_FM10424,          
        
    /** FM10440 Switch. */
    FM_SWITCH_PART_NUM_FM10440,          

    /** FM6232 Switch. */
    FM_SWITCH_PART_NUM_FM6232,          

    /** FM6248 Switch. */
    FM_SWITCH_PART_NUM_FM6248,          

    /** FM6264 Switch. */
    FM_SWITCH_PART_NUM_FM6264,          /* 35 */

    /** FM6316 Switch. */
    FM_SWITCH_PART_NUM_FM6316,          
        
    /** FM6324 Switch. */
    FM_SWITCH_PART_NUM_FM6324,          

    /** FM6332 Switch. */
    FM_SWITCH_PART_NUM_FM6332,          

    /** FM6348 Switch. */
    FM_SWITCH_PART_NUM_FM6348,          

    /** FM6364 Switch. */
    FM_SWITCH_PART_NUM_FM6364,          /* 40 */

    /** FM6372 Switch. */
    FM_SWITCH_PART_NUM_FM6372,

    /** FM7232 Switch. */
    FM_SWITCH_PART_NUM_FM7232,

    /** FM7248 Switch. */
    FM_SWITCH_PART_NUM_FM7248,

    /** FM7264 Switch. */
    FM_SWITCH_PART_NUM_FM7264,

    /** FM7316 Switch. */
    FM_SWITCH_PART_NUM_FM7316,          /* 45 */
        
    /** FM7324 Switch. */
    FM_SWITCH_PART_NUM_FM7324,

    /** FM7332 Switch. */
    FM_SWITCH_PART_NUM_FM7332,

    /** FM7348 Switch. */
    FM_SWITCH_PART_NUM_FM7348, 

    /** FM7364 Switch. */
    FM_SWITCH_PART_NUM_FM7364,

    /** FM7372 Switch. */
    FM_SWITCH_PART_NUM_FM7372,          /* 50 */

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_PART_NUM_MAX

} fm_switchPartNum;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * switch version appearing in the ''fm_switchInfo''
 * structure returned by ''fmGetSwitchInfo''.
 **************************************************/
typedef enum
{
    /** FM2224 Versions A0 through A4. 
     *  \chips  FM2000 */
    FM_SWITCH_VERSION_FM2224_A0_A4 = 1,

    /** FM2224 Version A5. 
     *  \chips  FM2000 */
    FM_SWITCH_VERSION_FM2224_A5,

    /** FM4224 Version A1. 
     *  \chips  FM3000, FM4000 */
    FM_SWITCH_VERSION_FM4224_A1,

    /** FM4224 Version A1.5. 
     *  \chips  FM3000, FM4000 */
    FM_SWITCH_VERSION_FM4224_A1_5,

    /** FM4224 Version A1.6. 
     *  \chips  FM3000, FM4000 */
    FM_SWITCH_VERSION_FM4224_A1_6,

    /** FM4224 Version A2. 
     *  \chips  FM3000, FM4000 */
    FM_SWITCH_VERSION_FM4224_A2,

    /** FM4224 Version A3. 
     *  \chips  FM3000, FM4000 */
    FM_SWITCH_VERSION_FM4224_A3,

    /** Switch Aggregate Version 1 */
    FM_SWITCH_VERSION_SWAG_1,

    /** FM6224 Version A0.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6224_A0,

    /** FM6364 Version A0.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6364_A0,

    /** FM6224 Version B0.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6224_B0,

    /** FM6364 Version B0.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6364_B0,

    /** FM6224 Version B1.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6224_B1,

    /** FM6364 Version B1.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6364_B1,

    /** FM10440 Version A0.
     *  \chips  FM10000 */
    FM_SWITCH_VERSION_FM10440_A0,

    /** FM6224 Version B2.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6224_B2,

    /** FM6364 Version B2.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6364_B2,

    /** FM6348 Version B2.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6348_B2,

    /** FM6324 Version B2.  
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6324_B2,

    /** FM5224 Version B2.
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM5224_B2,

    /** FM6764 Version B2.
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6764_B2,

    /** FM6748 Version B2.
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6748_B2,

    /** FM6724 Version B2.
     *  \chips  FM6000 */
    FM_SWITCH_VERSION_FM6724_B2,

    /** FM10440 Version B0.
     *  \chips  FM10000 */
    FM_SWITCH_VERSION_FM10440_B0,

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_VERSION_MAX,

    /** Unknown Switch Version. */
    FM_SWITCH_VERSION_UNKNOWN = 0xffff,

} fm_switchVersion;


/**************************************************/
/** \ingroup typeEnum
 * Set of possible enumerated values for the
 * switch state returned by ''fmGetSwitchStateExt''.
 **************************************************/
typedef enum
{
    /** Switch is not present. */
    FM_SWITCH_STATE_UNKNOWN = 0,

    /** Switch is inserted but down. */
    FM_SWITCH_STATE_DOWN,

    /** Switch has begun the process of being brought up. */
    FM_SWITCH_STATE_INIT,

    /** Switch is done booting, continuing to be brought up. */
    FM_SWITCH_STATE_BOOT_DONE,

    /** Switch is up and operational. */
    FM_SWITCH_STATE_UP,

    /** Switch is in the process of being brought down. */
    FM_SWITCH_STATE_GOING_DOWN,

    /** Switch encountered a fatal error. */
    FM_SWITCH_STATE_FAILED,

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_STATE_MAX

} fm_switchState;



/**************************************************
 * The following comment block produces the intro-
 * ductory text for the Legacy Synonym section of
 * the API user documentation. The actual
 * synonym #defines are located throughout the API
 * header files, appearing near the prototypes for
 * the functions they map to.
 **************************************************/

/**************************************************/
/** Legacy API Synonyms
 * \ingroup macroSynonym
 * \page leagcySynonyms
 *
 * The following macros provide legacy synonyms for
 * API functions that have been renamed. The
 * synonyms are provided so that existing applications
 * that call the functions by their original names
 * do not have to be modified.
 **************************************************/


#endif /* __FM_FM_API_COMMON_H */
