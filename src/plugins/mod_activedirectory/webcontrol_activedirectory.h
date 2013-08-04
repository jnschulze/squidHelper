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

#ifndef _WEBCONTROL_ACTIVEDIRECTORY_H
#define _WEBCONTROL_ACTIVEDIRECTORY_H

#if 0
#define WEBCONTROL_ACTIVEDIRECTORY_URI           "ldap://10.1.1.10"
#define WEBCONTROL_ACTIVEDIRECTORY_BIND_DN       "CN=Administrator,OU=admins,DC=ads,DC=gysar"
#define WEBCONTROL_ACTIVEDIRECTORY_BIND_PASSWORD "secret"
#define WEBCONTROL_ACTIVEDIRECTORY_SCOPE         "ou=Computer,dc=ads,dc=gysar"
#define WEBCONTROL_ACTIVEDIRECTORY_FILTER        "(&(objectclass=computer)(dNSHostName=%s))"   // %s is replaced by the resolved hostname
#define WEBCONTROL_ACTIVEDIRECTORY_ATTRIBUTENAME "webcontrolInetStatus"
#endif

#endif