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

#include "webcontrol_buffer.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>

#define BUFFER_PIECE_SIZE 64
#define BUFFER_MAX_REUSE_SIZE 256

buffer* buffer_init()
{
	buffer* b;

	b = malloc(sizeof(*b));
	assert(b);

	b->ptr = NULL;
	b->capacity = 0;
	b->len = 0;

	return b;
}

buffer* buffer_init_string(const char *str)
{
	buffer* b = buffer_init();
	buffer_copy_string(b, str);
	return b;
}

buffer* buffer_init_string_len(const char *str, size_t len)
{
	buffer* b = buffer_init();
	buffer_copy_string_len(b, str, len);
	return b;
}

buffer* buffer_init_buffer(buffer* src) {
	buffer *b = buffer_init();
	buffer_copy_string_buffer(b, src);
	return b;
}

int buffer_prepare_copy(buffer* b, size_t size)
{
	if (!b) return -1;

	if ((0 == b->capacity) ||
	    (size > b->capacity)) {
		if (b->capacity) free(b->ptr);

		b->capacity = size;

		/* always allocate a multiply of BUFFER_PIECE_SIZE */
		b->capacity += BUFFER_PIECE_SIZE - (b->capacity % BUFFER_PIECE_SIZE);

		b->ptr = malloc(b->capacity);
		assert(b->ptr);
	}
	b->len = 0;
	return 0;
}

int buffer_copy_string(buffer* b, const char *s)
{
	size_t s_len;

	if (!s || !b) return -1;

	s_len = strlen(s) + 1;
	buffer_prepare_copy(b, s_len);

	memcpy(b->ptr, s, s_len);
	b->len = s_len;

	return 0;
}

int buffer_copy_string_buffer(buffer *b, const buffer *src) {
	if (!src) return -1;

	if (src->len == 0) {
		buffer_reset(b);
		return 0;
	}
	return buffer_copy_string_len(b, src->ptr, src->len - 1);
}

int buffer_copy_string_len(buffer *b, const char *s, size_t s_len) {
	if (!s || !b) return -1;
#if 0
	/* removed optimization as we have to keep the empty string
	 * in some cases for the config handling
	 *
	 * url.access-deny = ( "" )
	 */
	if (s_len == 0) return 0;
#endif
	buffer_prepare_copy(b, s_len + 1);

	memcpy(b->ptr, s, s_len);
	b->ptr[s_len] = '\0';
	b->len = s_len + 1;

	return 0;
}


int buffer_prepare_append(buffer* b, size_t size)
{
	if (!b) return -1;

	if (0 == b->capacity) {
		b->capacity = size;

		/* always allocate a multiply of BUFFER_PIECE_SIZE */
		b->capacity += BUFFER_PIECE_SIZE - (b->capacity % BUFFER_PIECE_SIZE);

		b->ptr = malloc(b->capacity);
		b->len = 0;
		assert(b->ptr);
	} else if (b->len + size > b->capacity) {
		b->capacity += size;

		/* always allocate a multiply of BUFFER_PIECE_SIZE */
		b->capacity += BUFFER_PIECE_SIZE - (b->capacity % BUFFER_PIECE_SIZE);

		b->ptr = realloc(b->ptr, b->capacity);
		assert(b->ptr);
	}
	return 0;
}

int buffer_append_string(buffer* b, const char *s)
{
	size_t s_len;

	if (!s || !b) return -1;

	s_len = strlen(s);
	buffer_prepare_append(b, s_len + 1);
	if (b->len == 0)
		b->len++;

	memcpy(b->ptr + b->len - 1, s, s_len + 1);
	b->len += s_len;

	return 0;
}

int buffer_append_string_len(buffer* b, const char *s, size_t s_len) {
	if (!s || !b) return -1;
	if (s_len == 0) return 0;

	buffer_prepare_append(b, s_len + 1);
	if (b->len == 0)
		b->len++;

	memcpy(b->ptr + b->len - 1, s, s_len);
	b->len += s_len;
	b->ptr[b->len - 1] = '\0';

	return 0;
}

void buffer_free(buffer* b)
{
	if (!b) return;

	free(b->ptr);
	free(b);
}

void buffer_reset(buffer *b) {
	if (!b) return;

	/* limit don't reuse buffer larger than ... bytes */
	if (b->capacity > BUFFER_MAX_REUSE_SIZE) {
		free(b->ptr);
		b->ptr = NULL;
		b->capacity = 0;
	} else if (b->capacity) {
		b->ptr[0] = '\0';
	}

	b->len = 0;
}

size_t buffer_getline(buffer* b, FILE* f)
{
	size_t nbytes = getline(&b->ptr, &b->capacity, f);
	b->len = nbytes;
	return nbytes;
}






void truncate_string(char* str, size_t max_len, const char* sep)
{
	size_t len = strlen(str);

	//printf("Len is %zu\n", len);

	if(len <= max_len) return;

	size_t sep_len = strlen(sep);
	
	assert(max_len >= sep_len);

	size_t start = (max_len-sep_len) / 2;

	//printf("Start is %d\n", start);

	size_t x = len-start;

	if(max_len % 2 != 0)
	{
		x--;
	}

	for(size_t i=start;i<max_len;++i)
	{
		ptrdiff_t sep_pos = i-start;

		if(sep_pos < sep_len)
		{
			str[i] = sep[sep_pos];
		}
		else
		{
			str[i] = str[x++];
		}
	}

	str[max_len] = '\0';
}


void strtoupper(char* str)
{
    for (char* c = str; *c; c++) {
            if (*c >= 'a' && *c <= 'z') {
                    *c &= ~32;
            }
    }
}

void strtolower(char* str)
{
    for (char* c = str; *c; c++) {
            if (*c >= 'A' && *c <= 'Z') {
                    *c |= 32;
            }
    }
}