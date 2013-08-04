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

#ifndef _WEBCONTROL_BUFFER_H
#define _WEBCONTROL_BUFFER_H

#include <stdlib.h>
#include <stdio.h>

typedef struct
{
	char* ptr;
    size_t len;
	size_t capacity;
} buffer;

buffer* buffer_init();
buffer* buffer_init_buffer(buffer* b);
buffer* buffer_init_string(const char* str);
buffer* buffer_init_string_len(const char* str, size_t len);

void buffer_reset(buffer* b);
void buffer_free(buffer* b);

int buffer_copy_string(buffer* b, const char* s);
int buffer_copy_string_buffer(buffer *b, const buffer *src);
int buffer_copy_string_len(buffer *b, const char *s, size_t s_len);
int buffer_append_string(buffer *b, const char *s);

int buffer_prepare_copy(buffer* b, size_t size);
int buffer_prepare_append(buffer *b, size_t size);

int buffer_append_string_len(buffer *b, const char *s, size_t s_len);

size_t buffer_getline(buffer* b, FILE* f);

void truncate_string(char* str, size_t max_len, const char* sep);

void strtoupper(char* str);
void strtolower(char* str);

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define CONST_BUF_LEN(x) x->ptr, x->used ? x->used - 1 : 0

#endif