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

#include "webcontrol_netutil.h"

#include <netdb.h>
#include <string.h>
#include <limits.h>

#include "webcontrol_buffer.h"

buffer* webcontrol_rdns_lookup_buf(const ipaddr* addr)
{
	char host[HOST_NAME_MAX+1];
	bzero(host,HOST_NAME_MAX);

	return (webcontrol_rdns_lookup(addr, host, HOST_NAME_MAX) == 0) ? buffer_init_string(host) : NULL;
}

inline int webcontrol_rdns_lookup(const ipaddr* addr, char* buffer, size_t len)
{
	return getnameinfo((struct sockaddr*)&addr->sockaddr, sizeof(addr->sockaddr), buffer, len, NULL, 0, NI_NAMEREQD);
}