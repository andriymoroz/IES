/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos.h
 * Creation Date:   2005
 * Description:     Wrapper file for all of the ALOS headers
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

#ifndef __FM_FM_ALOS_H
#define __FM_FM_ALOS_H


/**********************************************************************
 * This file is the primary include file for the externally-exposed
 * "common" portions of the Fulcrum ControlPoint SDK.
 *
 * Internal dependencies inside this subsystem should be documented here.
 * The only external requirements allowed here are the architectural
 * definitions and the "common" subsystem.
 *
 * As documented in fm_sdk.h, fm_alos_event_queue.h  and fm_alos_threads.h
 * assume that the fm_event typedef has been forward-defined.
 * This is assumed to have been done prior to inclusion of this file.
 *
 * As of 04/17/2007, the following order-dependencies exist amongst the files
 * referenced here:
 *
 *    fm_alos_threads.h depends upon fm_alos_event_queue.h
 *    fm_alos_threads.h depends upon fm_alos_lock.h
 *    fm_alos_threads.h depends upon fm_alos_time.h
 **********************************************************************/

#include <fm_alos_logging.h>
#include <fm_alos_init.h>
#include <fm_alos_time.h>
#include <fm_alos_lock.h>
#include <fm_alos_rwlock.h>
#include <fm_alos_sem.h>
#include <fm_alos_event_queue.h>
#include <fm_alos_threads.h>
#include <fm_alos_alloc.h>
#include <fm_alos_dynamic_load.h>
#include <fm_alos_rand.h>

#endif /* __FM_FM_ALOS_H */
