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

#ifndef _WEBCONTROL_DEBUG_H
#define _WEBCONTROL_DEBUG_H

#include "webcontrol_base.h"
#include <stdio.h>
#include <pthread.h>

#if 0
#define DEBUG(msg,...) fprintf(stderr, "[" __FILE__ ":" STR(__LINE__) "] " msg "\n", __VA_ARGS__+0);
#endif


#if 0
#define DEBUG(format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#endif

#if 0
#define CURRENT_THREAD pthread_getspecific(current_thread_key)
#endif

#if 0
#define DEBUG(format, ...)\
do\
{\
	struct thread* ___CURRENT_THREAD = (struct thread*)CURRENT_THREAD;\
    if(___CURRENT_THREAD != NULL)\
    {\
		printf("NUmber is %d\n", ___CURRENT_THREAD->number);\
    }\
    else\
    {\
	    printf("moin");\
    }\
} while(0)
#endif



#endif