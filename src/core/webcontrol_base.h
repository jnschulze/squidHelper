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

#ifndef _WEBCONTROL_BASE_H
#define _WEBCONTROL_BASE_H

#include <pthread.h>
#include <netinet/in.h>

#include "webcontrol_buffer.h"

#ifdef HAVE_IPV6
 #define WEBCONTROL_IPADDRSTRLEN INET6_ADDRSTRLEN
 #define WEBCONTROL_IPADDR struct in6_addr
 #define WEBCONTROL_INETPROTO AF_INET6
#else
 #define WEBCONTROL_IPADDRSTRLEN INET_ADDRSTRLEN
 #define WEBCONTROL_IPADDR struct in_addr
 #define WEBCONTROL_INETPROTO AF_INET
#endif

struct webcontrol_core_ctx;

typedef struct
{
	struct sockaddr_storage sockaddr;

	#ifdef HAVE_IPV6
	char str[INET6_ADDRSTRLEN+1];
	#else
	char str[INET_ADDRSTRLEN+1];
	#endif
	
	size_t str_len;
} ipaddr;

typedef struct
{
	/* GLOBAL STUFF */
	struct webcontrol_core_ctx* webcontrol;

	/* Native Thread */
	pthread_t pthread;

	/* Thread number */
	size_t number;

	//buffer
	
} thread;

typedef enum
{
	WORKER_TERMINATE = 1,
} thread_signal;

typedef struct
{
	/* origin thread */
	thread* thread;

	/* signal or user-specific data */
	union
	{
		thread_signal signal;
		void* data;
	};
} thread_msg;

typedef enum
{
	HANDLER_GO_ON, // Unable to fulfil lookp request (no such object etc.)    
	HANDLER_ALLOW, // Lookup successful => allow access
	HANDLER_DENY,  // Lookup successful => deny success
	HANDLER_OK,    // Everything is ok (in non-lookup contexts)
	HANDLER_ERROR, // An Error occured
} handler_status;

size_t current_thread_id();










#endif