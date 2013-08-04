// Copyright (C) 2009-2012 Jan Niklas Schulze
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef _WEBCONTROL_H
#define _WEBCONTROL_H

#include "config.h"
#include "webcontrol_base.h"
#include "webcontrol_buffer.h"
#include "webcontrol_plugin.h"
#include "webcontrol_request.h"
#include "webcontrol_netutil.h"
#include "webcontrol_debug.h"
#include "thirdparty/c-pthread-queue/queue.h"

#include <pthread.h>
#include <ev.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_LIBCONFUSE
 #include <confuse.h>
#endif

extern pthread_key_t current_thread_key;
extern pthread_key_t current_thread_name_key;

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

typedef enum { false, true } bool;

#define str4_cmp(str, c0, c1, c2, c3) *((unsigned int*)str) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#if 0
#define DEBUG(format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#endif

#if 0
#define DEBUG(format, ...) fprintf(stderr, "[%s:%d@%zu:DEBUG] " format "\n", __FILE__, __LINE__, current_thread_number(), ##__VA_ARGS__);
#endif

#if 0
#define DEBUG(format, ...) fprintf(stderr, "[%s:%d@%zu:DEBUG %-20s] " format "\n", __FILE__, __LINE__, current_thread_number(), "P", ##__VA_ARGS__);
#endif

#if 0
#define DEBUG(format, ...) fprintf(stderr, "[%-30.30s:%.4d@%-2zu] " format "\n", __FILE__, __LINE__, current_thread_number(), ##__VA_ARGS__);
#define ERROR(format, ...) fprintf(stderr, "[%s:%d@%zu:ERROR] " format "\n", __FILE__, __LINE__, current_thread_number(), ##__VA_ARGS__);
#define INFO(format, ...) fprintf(stderr, "[%s:%d@%zu:INFO] " format "\n", __FILE__, __LINE__, current_thread_number(), ##__VA_ARGS__);
#endif

#if 0
#define LOG(level, format, ...) fprintf(stderr, "[%-30.30s:%4d@%-2zu%s] " format "\n", __FILE__, __LINE__, current_thread_number(), level, ##__VA_ARGS__);
#endif

// Strips off filename
#if 0
#define LOG(level, format, ...) fprintf(stderr, "[%-30.30s:%4d@%-2zu%s] " format "\n", __FILE__, __LINE__, current_thread_number(), level, ##__VA_ARGS__);
#endif

#if 0
#define LOG(level, format, ...)\
    do\
    {\
        char __FILE__TRUNCATED__[strlen(__FILE__)];\
        strcpy(__FILE__TRUNCATED__, __FILE__);\
        truncate_string(__FILE__TRUNCATED__, 24, "..");\
        fprintf(stderr, "[%-24s:%4d@%-2zu%s] " format "\n", __FILE__TRUNCATED__, __LINE__, current_thread_number(), level, ##__VA_ARGS__);\
    } while(0)
#endif

#define LOG(level, format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);

#define DEBUG(format, ...) LOG("DEBUG", format, ##__VA_ARGS__);
#define INFO(format, ...) LOG("INFO", format, ##__VA_ARGS__);
#define ERROR(format, ...) LOG("ERROR", format, ##__VA_ARGS__);
#define TRACE(format, ...) LOG("TRACE", format, ##__VA_ARGS__);

inline thread* current_thread()
{
	return pthread_getspecific(current_thread_key);
}

inline size_t current_thread_number()
{
	thread* t = current_thread();
	return t ? t->number : 0;
}

typedef struct webcontrol_core_ctx
{
	EV_P;

	/* config structure */
	#ifdef HAVE_LIBCONFUSE
	cfg_t* cfg;
	cfg_opt_t* opts;
	#endif

	/* event stuff */
	ev_io* ev_stdin_watcher;
	ev_signal* ev_signal_watcher;
	ev_async* ev_thread_signal_watcher;

	/* line buffer */
	//buffer* line_buf;
	buffer* tmp_line_buf;


	/* job queue buffer */
	void* job_queue_buffer[256];

	/* job queue */
	queue_t* job_queue;

	/* signal queue buffer */
	void* thread_signal_queue_buffer[8];

	/* signal queue */
	queue_t* thread_signal_queue;

	size_t active_workers;

	/* plugin array */
	struct plugin** plugins;
	size_t num_plugins;

	/* whether deny is the default behavior */
	int deny_by_default;

	size_t request_counter;

	pthread_mutex_t stdout_lock;

	FILE* logfile;

	uint8_t loglevel;
  
} webcontrol_core_ctx;

#endif
