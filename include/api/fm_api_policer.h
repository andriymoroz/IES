/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_policer.h
 * Creation Date:   November 2007
 * Description:     Structures and functions for dealing with policers
 *
 * Copyright (c) 2007 - 2013, Intel Corporation
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

#ifndef __FM_FM_API_POLICER_H
#define __FM_FM_API_POLICER_H


/* Flag to auto-allocate policers to banks. */
#define FM_POLICER_BANK_AUTOMATIC  -1

/** The maximum number of differentiated services code points.
 *  \ingroup constSystem */
#define FM_MAX_DSCP_VALUES         64


/****************************************************************************/
/** \ingroup typeStruct
 * Policer Action Data
 * Used as an argument extension to ''fmCreatePolicer''. Used 
 * when the action specified is either ''FM_POLICER_ACTION_SET_SWPRI_VPRI'' or 
 * ''FM_POLICER_ACTION_SET_SWPRI_DSCP''.
 ****************************************************************************/
typedef struct _fm_policerActionData
{
    /** New switch priority value, for use with
     *  ''FM_POLICER_ACTION_SET_SWPRI_VPRI'' or
     *  ''FM_POLICER_ACTION_SET_SWPRI_DSCP''. */
    fm_int    swPri;

    /** New DSCP value, for use with ''FM_POLICER_ACTION_SET_SWPRI_DSCP''. */
    fm_int    dscp;

    /** New vlan priority value, for use with
     *  ''FM_POLICER_ACTION_SET_SWPRI_VPRI''. */
    fm_int    vPri;

} fm_policerActionData;


/****************************************************************************/
/** \ingroup typeStruct
 * Policer Configuration
 * Used as an argument to ''fmCreatePolicer''. Each of the members of this
 * structure can be (re)configured after the policer has been created
 * by calling ''fmSetPolicerAttribute''. See also ''Policer Attributes''.
 ****************************************************************************/
typedef struct _fm_policerConfig
{
    /** Indicates whether to mark down the frame's Differentiated Services
     *  Code Point, according to the color map specified by
     *  dscpMkdnMap, when the rate limit is exceeded. Corresponds to the
     *  ''FM_POLICER_MKDN_DSCP'' attribute.
     *                                                                 \lb\lb
     *  This is a per-bank attribute. All policers assigned to the same
     *  bank must have this attribute set to the same value. If bank
     *  assignment is left to the ACL compiler (''fmCompileACL''), it will
     *  attempt to group policers into banks accordingly.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool   mkdnDscp;

    /** Indicates whether to mark down the frame's switch priority,
     *  according to the color map specified by swPriMkdnMap, when the rate
     *  limit is exceeded. Corresponds to the ''FM_POLICER_MKDN_SWPRI'' 
     *  attribute.
     *                                                                 \lb\lb
     *  This is a per-bank attribute. All policers assigned to the same
     *  bank must have this attribute set to the same value. If bank
     *  assignment is left to the ACL compiler (''fmCompileACL''), it will
     *  attempt to group policers into banks accordingly.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_bool   mkdnSwPri;

    /** The color source for an ingressing frame being managed by the policer.
     *  Corresponds to the ''FM_POLICER_COLOR_SOURCE'' attribute. May be one of
     *  the following values:                                               
     *                                                                  \lb\lb
     *  FM_POLICER_COLOR_SRC_GREEN to assume all ingressing frames are
     *  colored green.                                                      
     *                                                                  \lb\lb
     *  FM_POLICER_COLOR_SRC_DSCP to take the color source from the frame's
     *  Differentiated Services Code Point field.                           
     *                                                                  \lb\lb
     *  FM_POLICER_COLOR_SRC_SWPRI to take the color source from the frame's
     *  switch priority.
     *                                                                 \lb\lb
     *  See the ''FM_POLICER_SWPRI_MKDN_MAP'' policer attribute for more
     *  information on colors.
     *                                                                 \lb\lb
     *  This is a per-bank attribute. All policers assigned to the same
     *  bank must have this attribute set to the same value. If bank
     *  assignment is left to the ACL compiler (''fmCompileACL''), it will
     *  attempt to group policers into banks accordingly.
     *  
     *  The FM10000 switch family only supports FM_POLICER_COLOR_SRC_GREEN.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_int    colorSource;

    /** The action to take when the policer's committed rate is exceeded.
     *  Corresponds to the ''FM_POLICER_CIR_ACTION'' attribute. See
     *  ''Policer Actions'' for a list of valid values.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_int    cirAction;

    /** The policer's commited rate token bucket capacity in bytes. The capacity
     *  represents the maximum traffic burst size. The specified value will be 
     *  rounded  up to the nearest kilobyte (1024 byte) boundary. Corresponds 
     *  to the ''FM_POLICER_CIR_CAPACITY'' attribute.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32 cirCapacity;

    /** The rate at which the policer's commited rate token bucket is
     *  refilled in kilobits (1000 bits) per second
     *  (100,000 => 100Mbps, 10,000,000 => 10Gbps).
     *  The actual value will be the nearest value supported by the hardware.
     *  Corresponds to the ''FM_POLICER_CIR_RATE'' attribute. 
     *                                                                  \lb\lb
     *  Note that the minimum rate supported by policers is approximately
     *  100Mbps on FM4000 and 24Kbps on FM6000.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32 cirRate;

    /** The action to take when the policer's excess rate is exceeded.
     *  Corresponds to the ''FM_POLICER_EIR_ACTION'' attribute. See
     *  ''Policer Actions'' for a list of valid values.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_int    eirAction;

    /** The policer's excess rate token bucket capacity in bytes. The capacity
     *  represents the maximum traffic burst size. The specified value will be 
     *  rounded  up to the nearest kilobyte (1024 byte) boundary. Corresponds 
     *  to the ''FM_POLICER_EIR_CAPACITY'' attribute.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32 eirCapacity;

    /** The rate at which the policer's excess rate token bucket is
     *  refilled in kilobits (1000 bits) per second (100,000 => 100Mbps, 
     *  10,000,000 => 10Gbps). The actual value will be the nearest value 
     *  supported by the hardware. Corresponds to the ''FM_POLICER_EIR_RATE'' 
     *  attribute. 
     *                                                                  \lb\lb
     *  Note that the minimum rate supported by policers is approximately
     *  100Mbps on FM4000 and 24Kbps on FM6000.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint32 eirRate;

    /** Some cir actions require an argument upon which the action will be
     *  performed against.
     *  
     *  \chips  FM6000 */
    fm_policerActionData cirActionData;

    /** Some eir actions require an argument upon which the action will be
     *  performed against.
     *  
     *  \chips  FM6000 */
    fm_policerActionData eirActionData;

} fm_policerConfig;


/****************************************************************************/
/** \ingroup constPolicerAttr
 *
 * Policer Attributes, used as an argument to ''fmSetPolicerAttribute'' and
 * ''fmGetPolicerAttribute''. The scope of each attribute is per individual
 * policer unless otherwise noted.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 * Policer attributes are not available for the FM2000 family.
 ****************************************************************************/
enum _fm_policerAttr
{
    /** Type fm_bool: Mark down the frame's Differentiated Services Code Point,
     *  according to the color map specified by the ''FM_POLICER_DSCP_MKDN_MAP''
     *  attribute, when the rate limit is exceeded (see ''FM_POLICER_CIR_ACTION''
     *  and ''FM_POLICER_EIR_ACTION''): FM_ENABLED or FM_DISABLED (default).
     *                                                                 \lb\lb
     *  This is a per-bank attribute. All policers assigned to the same
     *  bank must have this attribute set to the same value. If bank
     *  assignment is left to the ACL compiler (''fmCompileACL''), it will
     *  attempt to group policers into banks accordingly. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_POLICER_MKDN_DSCP = 0,

    /** Type fm_bool: Mark down the frame's switch priority,
     *  according to the color map specified by the ''FM_POLICER_SWPRI_MKDN_MAP''
     *  attribute, when the rate limit is exceeded (see ''FM_POLICER_CIR_ACTION''
     *  and ''FM_POLICER_EIR_ACTION''): FM_ENABLED or FM_DISABLED (default).
     *                                                                 \lb\lb
     *  This is a per-bank attribute. All policers assigned to the same
     *  bank must have this attribute set to the same value. If bank
     *  assignment is left to the ACL compiler (''fmCompileACL''), it will
     *  attempt to group policers into banks accordingly. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_POLICER_MKDN_SWPRI,

    /** Type fm_int: The color source for an ingressing frame being
     *  managed by a policer. A policer can identify the color of an ingressing
     *  frame using one of several criteria:                                
     *                                                                  \lb\lb
     *  FM_POLICER_COLOR_SRC_GREEN (default) to assume all ingressing frames
     *  are colored green.                                                      
     *                                                                  \lb\lb
     *  FM_POLICER_COLOR_SRC_DSCP to take the color source from
     *  the frame's Differentiated Services Code Point field.               
     *                                                                  \lb\lb
     *  FM_POLICER_COLOR_SRC_SWPRI to take the color source from the frame's
     *  switch priority.
     *                                                                 \lb\lb
     *  See the ''FM_POLICER_SWPRI_MKDN_MAP'' attribute for more information
     *  on colors.
     *                                                                 \lb\lb
     *  This is a per-bank attribute. All policers assigned to the same
     *  bank must have this attribute set to the same value. If bank
     *  assignment is left to the ACL compiler (''fmCompileACL''), it will
     *  attempt to group policers into banks accordingly.
     *  
     *  Note that the FM10000 switch family only supports
     *  FM_POLICER_COLOR_SRC_GREEN.
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_POLICER_COLOR_SOURCE,

    /** Type fm_int: The action to take when the policer's committed rate
     *  is exceeded. See ''Policer Actions'' for a list of valid values.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_CIR_ACTION,

    /** Type fm_int (read-only): The new DSCP value to set if the 
     *  policer action is ''FM_POLICER_ACTION_SET_SWPRI_DSCP''.
     *
     *  \chips  FM6000 */
    FM_POLICER_CIR_ACTION_DATA_DSCP,

    /** Type fm_int (read-only): The new VLAN priority value to set if the 
     *  policer action is ''FM_POLICER_ACTION_SET_SWPRI_VPRI''.
     *
     *  \chips  FM6000 */
    FM_POLICER_CIR_ACTION_DATA_VPRI,

    /** Type fm_int (read-only): The new switch priority value to set if the
     *  policer action is either ''FM_POLICER_ACTION_SET_SWPRI_VPRI'' or 
     *  ''FM_POLICER_ACTION_SET_SWPRI_DSCP''.
     *
     *  \chips  FM6000 */
    FM_POLICER_CIR_ACTION_DATA_SWPRI,

    /** Type fm_uint32: The policer's commited rate token bucket capacity in
     *  bytes, with a default value of 0. The specified value will be rounded
     *  up to the next kilobyte (1024 byte) boundary, so the value retrieved
     *  with a call to ''fmGetPolicerAttribute'' may not exactly match the
     *  value set with ''fmSetPolicerAttribute''. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_CIR_CAPACITY,

    /** Type fm_uint32: The rate at which the policer's commited rate token
     *  bucket is refilled in kilobits (1000 bits) per second (100,000 => 100Mbps,
     *  10,000,000 => 10Gbps), with a default value of 0. The actual value
     *  will be the nearest value supported by the hardware so the value
     *  retrieved with a call to ''fmGetPolicerAttribute'' may not exactly
     *  match the value set with ''fmSetPolicerAttribute''.
     *                                                                  \lb\lb
     *  Note that the minimum rate supported by policers is approximately
     *  100Mbps on FM3000/FM4000 and 24Kbps on FM6000. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_CIR_RATE,

    /** Type fm_int: The action to take when the policer's excess rate
     *  is exceeded.  See ''Policer Actions'' for a list of valid values.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_EIR_ACTION,

    /** Type fm_int (read-only): The new DSCP value to set if the 
     *  policer action is ''FM_POLICER_ACTION_SET_SWPRI_DSCP''.
     *
     *  \chips  FM6000 */
    FM_POLICER_EIR_ACTION_DATA_DSCP,

    /** Type fm_int (read-only): The new VLAN priority value to set if the 
     *  policer action is ''FM_POLICER_ACTION_SET_SWPRI_VPRI''.
     *
     *  \chips  FM6000 */
    FM_POLICER_EIR_ACTION_DATA_VPRI,

    /** Type fm_int (read-only): The new switch priority value to set if the
     *  policer action is either ''FM_POLICER_ACTION_SET_SWPRI_VPRI'' or 
     *  ''FM_POLICER_ACTION_SET_SWPRI_DSCP''.
     *
     *  \chips  FM6000 */
    FM_POLICER_EIR_ACTION_DATA_SWPRI,

    /** Type fm_uint32: The policer's excess rate token bucket capacity in
     *  bytes, with a default value of 0. The specified value will be
     *  rounded up to the next kilobyte (1024 byte) boundary, so the value
     *  retrieved with a call to ''fmGetPolicerAttribute'' may not exactly
     *  match the value set with ''fmSetPolicerAttribute''. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_EIR_CAPACITY,

    /** Type fm_uint32: The rate at which the policer's excess rate token
     *  bucket is refilled in kilobits (1000 bits) per second (100,000 => 100Mbps,
     *  10,000,000 => 10Gbps), with a default value of 0. The actual value
     *  will be the nearest value supported by the hardware so the value
     *  retrieved with a call to ''fmGetPolicerAttribute'' may not exactly
     *  match the value set with ''fmSetPolicerAttribute''.
     *                                                                  \lb\lb
     *  Note that the minimum rate supported by policers is approximately
     *  100Mbps on FM3000/FM4000 and 24Kbps on FM6000. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_EIR_RATE,

    /** Type array of fm_int: The switch priority down marking map. The map is
     *  an array of integers, ''FM_SWITCH_PRIORITY_MAX'' entries in length. The
     *  map is indexed by the current value of the frame's switch priority
     *  with the entry being the new priority value to mark down to. Each
     *  entry must be between 0 and (''FM_SWITCH_PRIORITY_MAX'' - 1).
     *                                                                  \lb\lb
     *  A policer regards frames as colored: green, yellow or red. If the
     *  policer's CIR has been exceeded, green frames may be marked down to
     *  yellow. If the policer's EIR has been exceeded, green and yellow
     *  frames may be marked down to red.
     *                                                                  \lb\lb
     *  The down marking map indicates which values of switch priority (or
     *  DSCP for ''FM_POLICER_DSCP_MKDN_MAP'') correspond to which color. If 
     *  using the frame's current switch priority as an index into the map 
     *  results in the same value, the current value is red. If the resulting 
     *  value is not the same and using the resulting value as an index yields
     *  the same resulting value, then the current value is yellow. Otherwise,
     *  the current value is green.
     *                                                                  \lb\lb
     *  As an example using switch priorities and assuming that priority
     *  0 is the lowest priority and 15 is the highest, the following down
     *  marking map would indicate that priorities 0 - 4 are red, 5 - 9 are
     *  yellow and 10 - 15 are green:                                       \lb
     *  0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9.
     *                                                                  \lb\lb
     *  This attribute is global across all policers. The policer argument
     *  to ''fmSetPolicerAttribute'' or ''fmGetPolicerAttribute'' is ignored. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_POLICER_SWPRI_MKDN_MAP,

    /** Type array of fm_int: The DSCP down marking map. The map is an array of
     *  integers, ''FM_MAX_DSCP_VALUES'' entries in length. The map is indexed by
     *  the current value of the frame's DSCP field with the entry being the
     *  new DSCP value to mark down to. Each entry must be between 0 and
     *  (''FM_MAX_DSCP_VALUES'' - 1).
     *                                                                  \lb\lb
     *  See ''FM_POLICER_SWPRI_MKDN_MAP'' for a detailed description of how
     *  down marking maps work.
     *                                                                  \lb\lb
     *  This attribute is global across all policers. The policer argument
     *  to ''fmSetPolicerAttribute'' or ''fmGetPolicerAttribute'' is ignored. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_POLICER_DSCP_MKDN_MAP,

    /** UNPUBLISHED: For internal use only. */
    FM_POLICER_ATTRIBUTE_MAX,

};

/**************************************************
 * For FM_POLICER_COLOR_SOURCE (see)
 **************************************************/
enum
{
    FM_POLICER_COLOR_SRC_GREEN = 0,
    FM_POLICER_COLOR_SRC_DSCP,
    FM_POLICER_COLOR_SRC_SWPRI

};


/**************************************************
 * For FM_POLICER_CIR_ACTION and FM_POLICER_EIR_ACTION
 * (see)
 **************************************************/
/**************************************************/
/** \ingroup constPolicerAction                       
 *                                                 
 *  An action to be taken by a policer when either
 *  CIR or EIR is exceeded. Referenced by 
 *  ''fm_policerConfig'', ''FM_POLICER_CIR_ACTION''
 *  and ''FM_POLICER_EIR_ACTION''.
 **************************************************/
enum _fm_policerAction
{
    /** Drop the frame. This is the default policer action. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_POLICER_ACTION_DROP = 0,
    
    /** Mark down the color of the frame. The policer attributes
     *  ''FM_POLICER_MKDN_DSCP'' and ''FM_POLICER_MKDN_SWPRI''
     *  determine how the frame is marked down. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_POLICER_ACTION_MKDN,
    
    /** Change the differentiated services code point and the switch priority 
     *  of the frame. The DSCP and switch priority values to set are taken 
     *  from the dscp and swPri fields, respectively, of the 
     *  ''fm_policerActionData'' structure in the call to ''fmCreatePolicer''.
     *
     *  \chips  FM6000 */
    FM_POLICER_ACTION_SET_SWPRI_DSCP,

    /** Change the vlan priority and the switch priority of the frame. The
     *  vlan and switch priority values to set are taken from the vlan and
     *  swPri fields, respectively, of the ''fm_policerActionData'' structure
     *  in the call to ''fmCreatePolicer''.
     *
     *  \chips  FM6000 */
    FM_POLICER_ACTION_SET_SWPRI_VPRI,
    
    /** Trap the frame. 
     *
     *  \chips  FM6000 */
    FM_POLICER_ACTION_TRAP

};


fm_status fmCreatePolicer(fm_int                  sw,
                          fm_int                  bank,
                          fm_int                  policer,
                          const fm_policerConfig *config);
fm_status fmDeletePolicer(fm_int sw,
                          fm_int policer);
fm_status fmSetPolicerAttribute(fm_int      sw,
                                fm_int      policer,
                                fm_int      attr,
                                const void *value);
fm_status fmGetPolicerAttribute(fm_int     sw,
                                fm_int     policer,
                                fm_int     attr,
                                fm_voidptr value);
fm_status fmGetPolicerFirst(fm_int sw, fm_int *firstPolicer);
fm_status fmGetPolicerNext(fm_int  sw,
                           fm_int  currentPolicer,
                           fm_int *nextPolicer);
fm_status fmGetPolicerList(fm_int  sw,
                           fm_int *numPolicers,
                           fm_int *policers,
                           fm_int  max);
fm_status fmUpdatePolicer(fm_int sw, fm_int policer);

#endif /* __FM_FM_API_POLICER_H */
