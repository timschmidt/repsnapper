/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12 martin.dieringer@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_POSIX_THREADS
#include <pthread.h>
typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t cond_t;
#define thread_create( thread, start_func, arg ) pthread_create( thread, NULL, start_func, arg )
#define thread_join( thread ) pthread_join( thread, NULL )
#define thread_exit() pthread_exit( NULL )
#define HAS_THREAD_CANCEL
#define thread_cancel pthread_cancel
#define mutex_init( m ) pthread_mutex_init( m, NULL )
#define mutex_destroy pthread_mutex_destroy
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock
#define cond_init( c ) pthread_cond_init( c, NULL )
#define cond_destroy pthread_cond_destroy
#define cond_wait pthread_cond_wait
#define cond_signal pthread_cond_signal
#define cond_broadcast pthread_cond_broadcast
#else
// USE_GLIB_THREADS
#include <glib.h>
#include <glib-object.h>
typedef GThread *thread_t;
typedef GMutex mutex_t;
typedef GCond cond_t;
inline int thread_create( thread_t *thread, void *(*start_func)( void *), void *arg ) { *thread = g_thread_new( "PrinterThread", start_func, arg ); return 0; };
inline int thread_join( thread_t thread ) { g_thread_join( thread ); return 0; };
#define thread_exit() g_thread_exit( NULL )
#define mutex_init g_mutex_init
#define mutex_destroy g_mutex_clear
inline int mutex_lock( mutex_t *m ) { g_mutex_lock( m ); return 0; };
inline int mutex_unlock( mutex_t *m ) { g_mutex_unlock( m ); return 0; };
#define cond_init g_cond_init
#define cond_destroy g_cond_clear
inline int cond_wait( cond_t *c, mutex_t *m ) { g_cond_wait( c, m ); return 0; };
inline int cond_signal( cond_t *c ) { g_cond_signal( c ); return 0; };
inline int cond_broadcast( cond_t *c ) { g_cond_broadcast( c ); return 0; };
#endif

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
typedef struct timespec ntime_t;
inline int nsleep( const ntime_t *req ) { return nanosleep( req, NULL ); };
#else
typedef struct timespec {
  unsigned long tv_sec;
  long tv_nsec;
} ntime_t;

inline int nsleep( const ntime_t *req ) { Sleep( req->tv_sec * 1000 + ( tv_nsec + 999999 ) / 1000000 ); return 0; };
#endif
