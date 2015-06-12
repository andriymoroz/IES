/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_sdk.h
 * Creation Date:   2005
 * Description:     Wrapper to include all relevant files needed by user code
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_SDK_H
#define __FM_FM_SDK_H

#define FM_SDK_VERSION  4

/**********************************************************************
 * This file is the primary include file for the externally-exposed
 * portions of the Fulcrum ControlPoint SDK.
 *
 * Any order-dependencies among the top-level subsystems must be clearly
 * documented and handled here, not buried inside subordinate include files.
 * Internal dependencies inside a particular subsystem should be documented
 * in that sub-systems top-level include file.
 **********************************************************************/

/*
 * Include wrapper to OS & Standard C include files
 *
 * No dependencies upon Fulcrum ControlPoint SDK definitions
 *
 * No known order dependencies
 *
 */

#include <fm_alos_sys.h>        

/*
 * architecture specific header for type defs
 *
 * has no dependencies
 *
 * All other include files are permitted to assume that all
 * architecture definitions are available.
 */
#include "fm_std.h"

/*
 * Include the common subsystem.
 *
 * May only rely upon availability of architecture files
 */
#include <common/fm_common.h>

/*
 * customer-configurable constants.
 *
 * May only rely upon availability of architecture and common files.
 *
 * all ALOS, API and customer include files are permitted to assume that all
 * constants defined in this file are available.
 *
 */
#include <platform_defines.h>



/*
 * Default some system constants not specified by the
 * platform layer
 *
 */

/* Enable ALOS lock inversion defense by default */
#ifndef FM_LOCK_INVERSION_DEFENSE
#define FM_LOCK_INVERSION_DEFENSE       FM_ENABLED
#endif


/*
 * Include the ALOS subsystem.
 *
 * May only rely upon availability of architecture and common files,
 * except as explicitly documented here:
 *
 * alos/fm_alos_event_queue references fm_event typedef (pointer
 * references only).  fm_event is pretty tightly bound into
 * api/fm_api_events.h.  This reference is explicitly permitted here
 * by forward-defining the typedef at this point.
 */
typedef struct _fm_event    fm_event;
#include <fm_alos.h>

/*
 * Include the API subsystem.
 *
 * may rely upon architecture, common, and alos subsystems
 *
 */
#include <api/fm_api.h>

/*
 * Include the debug subsystem
 *
 * may rely upon architecture, common, alos, api, and drv subsystems
 *
 */
#include <debug/fm_debug.h>


#endif /* __FM_FM_SDK_H */
