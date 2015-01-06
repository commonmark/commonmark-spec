#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "cmark_ctype.h"
#include "buffer.h"

/* Used as default value for cmark_strbuf->ptr so that people can always
 * assume ptr is non-NULL and zero terminated even for new cmark_strbufs.
 */
unsigned char cmark_strbuf__initbuf[1];
unsigned char cmark_strbuf__oom[1];

#define ENSURE_SIZE(b, d)					\
	if ((d) > buf->asize && cmark_strbuf_grow(b, (d)) < 0)	\
		return -1;

#ifndef MIN
#define MIN(x,y)  ((x<y) ? x : y)
#endif

void cmark_strbuf_init(cmark_strbuf *buf, int initial_size)
{
	buf->asize = 0;
	buf->size = 0;
	buf->ptr = cmark_strbuf__initbuf;

	if (initial_size)
		cmark_strbuf_grow(buf, initial_size);
}

int cmark_strbuf_try_grow(cmark_strbuf *buf, int target_size, bool mark_oom)
{
	unsigned char *new_ptr;
	int new_size;

	if (buf->ptr == cmark_strbuf__oom)
		return -1;

	if (target_size <= buf->asize)
		return 0;

	if (buf->asize == 0) {
		new_size = target_size;
		new_ptr = NULL;
	} else {
		new_size = buf->asize;
		new_ptr = buf->ptr;
	}

	/* grow the buffer size by 1.5, until it's big enough
	 * to fit our target size */
	while (new_size < target_size)
		new_size = (new_size << 1) - (new_size >> 1);

	/* round allocation up to multiple of 8 */
	new_size = (new_size + 7) & ~7;

	new_ptr = (unsigned char *)realloc(new_ptr, new_size);

	if (!new_ptr) {
		if (mark_oom)
			buf->ptr = cmark_strbuf__oom;
		return -1;
	}

	buf->asize = new_size;
	buf->ptr   = new_ptr;

	/* truncate the existing buffer size if necessary */
	if (buf->size >= buf->asize)
		buf->size = buf->asize - 1;
	buf->ptr[buf->size] = '\0';

	return 0;
}

int cmark_strbuf_grow(cmark_strbuf *buf, int target_size)
{
	return cmark_strbuf_try_grow(buf, target_size, true);
}

bool cmark_strbuf_oom(const cmark_strbuf *buf)
{
	return (buf->ptr == cmark_strbuf__oom);
}

size_t cmark_strbuf_len(const cmark_strbuf *buf)
{
	return buf->size;
}

void cmark_strbuf_free(cmark_strbuf *buf)
{
	if (!buf) return;

	if (buf->ptr != cmark_strbuf__initbuf && buf->ptr != cmark_strbuf__oom)
		free(buf->ptr);

	cmark_strbuf_init(buf, 0);
}

void cmark_strbuf_clear(cmark_strbuf *buf)
{
	buf->size = 0;

	if (buf->asize > 0)
		buf->ptr[0] = '\0';
}

int cmark_strbuf_set(cmark_strbuf *buf, const unsigned char *data, int len)
{
	if (len <= 0 || data == NULL) {
		cmark_strbuf_clear(buf);
	} else {
		if (data != buf->ptr) {
			ENSURE_SIZE(buf, len + 1);
			memmove(buf->ptr, data, len);
		}
		buf->size = len;
		buf->ptr[buf->size] = '\0';
	}
	return 0;
}

int cmark_strbuf_sets(cmark_strbuf *buf, const char *string)
{
	return cmark_strbuf_set(buf,
	                        (const unsigned char *)string,
	                        string ? strlen(string) : 0);
}

int cmark_strbuf_putc(cmark_strbuf *buf, int c)
{
	ENSURE_SIZE(buf, buf->size + 2);
	buf->ptr[buf->size++] = c;
	buf->ptr[buf->size] = '\0';
	return 0;
}

int cmark_strbuf_put(cmark_strbuf *buf, const unsigned char *data, int len)
{
	if (len <= 0)
		return 0;

	ENSURE_SIZE(buf, buf->size + len + 1);
	memmove(buf->ptr + buf->size, data, len);
	buf->size += len;
	buf->ptr[buf->size] = '\0';
	return 0;
}

int cmark_strbuf_puts(cmark_strbuf *buf, const char *string)
{
	return cmark_strbuf_put(buf, (const unsigned char *)string, strlen(string));
}

int cmark_strbuf_vprintf(cmark_strbuf *buf, const char *format, va_list ap)
{
	const int expected_size = buf->size + (strlen(format) * 2);
	int len;

	ENSURE_SIZE(buf, expected_size);

	while (1) {
		va_list args;
		va_copy(args, ap);

		len = vsnprintf(
		          (char *)buf->ptr + buf->size,
		          buf->asize - buf->size,
		          format, args
		      );

		va_end(args);

		if (len < 0) {
			free(buf->ptr);
			buf->ptr = cmark_strbuf__oom;
			return -1;
		}

		if (len + 1 <= buf->asize - buf->size) {
			buf->size += len;
			break;
		}

		ENSURE_SIZE(buf, buf->size + len + 1);
	}

	return 0;
}

int cmark_strbuf_printf(cmark_strbuf *buf, const char *format, ...)
{
	int r;
	va_list ap;

	va_start(ap, format);
	r = cmark_strbuf_vprintf(buf, format, ap);
	va_end(ap);

	return r;
}

void cmark_strbuf_copy_cstr(char *data, int datasize, const cmark_strbuf *buf)
{
	int copylen;

	assert(data && datasize && buf);

	data[0] = '\0';

	if (buf->size == 0 || buf->asize <= 0)
		return;

	copylen = buf->size;
	if (copylen > datasize - 1)
		copylen = datasize - 1;
	memmove(data, buf->ptr, copylen);
	data[copylen] = '\0';
}

void cmark_strbuf_swap(cmark_strbuf *buf_a, cmark_strbuf *buf_b)
{
	cmark_strbuf t = *buf_a;
	*buf_a = *buf_b;
	*buf_b = t;
}

unsigned char *cmark_strbuf_detach(cmark_strbuf *buf)
{
	unsigned char *data = buf->ptr;

	if (buf->asize == 0 || buf->ptr == cmark_strbuf__oom) {
		/* return an empty string */
		return (unsigned char *)calloc(1, 1);
	}

	cmark_strbuf_init(buf, 0);
	return data;
}

void cmark_strbuf_attach(cmark_strbuf *buf, unsigned char *ptr, int asize)
{
	cmark_strbuf_free(buf);

	if (ptr) {
		buf->ptr = ptr;
		buf->size = strlen((char *)ptr);
		if (asize)
			buf->asize = (asize < buf->size) ? buf->size + 1 : asize;
		else /* pass 0 to fall back on strlen + 1 */
			buf->asize = buf->size + 1;
	} else {
		cmark_strbuf_grow(buf, asize);
	}
}

int cmark_strbuf_cmp(const cmark_strbuf *a, const cmark_strbuf *b)
{
	int result = memcmp(a->ptr, b->ptr, MIN(a->size, b->size));
	return (result != 0) ? result :
	       (a->size < b->size) ? -1 : (a->size > b->size) ? 1 : 0;
}

int cmark_strbuf_strchr(const cmark_strbuf *buf, int c, int pos)
{
	const unsigned char *p = (unsigned char *)memchr(buf->ptr + pos, c, buf->size - pos);
	if (!p)
		return -1;

	return (int)(p - (const unsigned char *)buf->ptr);
}

int cmark_strbuf_strrchr(const cmark_strbuf *buf, int c, int pos)
{
	int i;

	for (i = pos; i >= 0; i--) {
		if (buf->ptr[i] == (unsigned char) c)
			return i;
	}

	return -1;
}

void cmark_strbuf_truncate(cmark_strbuf *buf, int len)
{
	if (len < buf->size) {
		buf->size = len;
		buf->ptr[buf->size] = '\0';
	}
}

void cmark_strbuf_drop(cmark_strbuf *buf, int n)
{
	if (n > 0) {
		buf->size = buf->size - n;
		if (buf->size)
			memmove(buf->ptr, buf->ptr + n, buf->size);

		buf->ptr[buf->size] = '\0';
	}
}

void cmark_strbuf_rtrim(cmark_strbuf *buf)
{
	if (!buf->size)
		return;

	while (buf->size > 0) {
		if (!cmark_isspace(buf->ptr[buf->size - 1]))
			break;

		buf->size--;
	}

	buf->ptr[buf->size] = '\0';
}

void cmark_strbuf_trim(cmark_strbuf *buf)
{
	int i = 0;

	if (!buf->size)
		return;

	while (i < buf->size && cmark_isspace(buf->ptr[i]))
		i++;

	cmark_strbuf_drop(buf, i);

	cmark_strbuf_rtrim(buf);
}

// Destructively modify string, collapsing consecutive
// space and newline characters into a single space.
void cmark_strbuf_normalize_whitespace(cmark_strbuf *s)
{
	bool last_char_was_space = false;
	int r, w;

	for (r = 0, w = 0; r < s->size; ++r) {
		switch (s->ptr[r]) {
		case ' ':
		case '\n':
			if (last_char_was_space)
				break;

			s->ptr[w++] = ' ';
			last_char_was_space = true;
			break;

		default:
			s->ptr[w++] = s->ptr[r];
			last_char_was_space = false;
		}
	}

	cmark_strbuf_truncate(s, w);
}

// Destructively unescape a string: remove backslashes before punctuation chars.
extern void cmark_strbuf_unescape(cmark_strbuf *buf)
{
	int r, w;

	for (r = 0, w = 0; r < buf->size; ++r) {
		if (buf->ptr[r] == '\\' && cmark_ispunct(buf->ptr[r + 1]))
			continue;

		buf->ptr[w++] = buf->ptr[r];
	}

	cmark_strbuf_truncate(buf, w);
}
