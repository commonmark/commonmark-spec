#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include "buffer.h"

/* Used as default value for strbuf->ptr so that people can always
 * assume ptr is non-NULL and zero terminated even for new strbufs.
 */
unsigned char strbuf__initbuf[1];
unsigned char strbuf__oom[1];

#define ENSURE_SIZE(b, d) \
	if ((d) > buf->asize && strbuf_grow(b, (d)) < 0)\
		return -1;

#define MIN(x,y)  ((x<y) ? x : y)

void strbuf_init(strbuf *buf, int initial_size)
{
	buf->asize = 0;
	buf->size = 0;
	buf->ptr = strbuf__initbuf;

	if (initial_size)
		strbuf_grow(buf, initial_size);
}

int strbuf_try_grow(strbuf *buf, int target_size, bool mark_oom)
{
	unsigned char *new_ptr;
	int new_size;

	if (buf->ptr == strbuf__oom)
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

	new_ptr = realloc(new_ptr, new_size);

	if (!new_ptr) {
		if (mark_oom)
			buf->ptr = strbuf__oom;
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

void strbuf_free(strbuf *buf)
{
	if (!buf) return;

	if (buf->ptr != strbuf__initbuf && buf->ptr != strbuf__oom)
		free(buf->ptr);

	strbuf_init(buf, 0);
}

void strbuf_clear(strbuf *buf)
{
	buf->size = 0;

	if (buf->asize > 0)
		buf->ptr[0] = '\0';
}

int strbuf_set(strbuf *buf, const unsigned char *data, int len)
{
	if (len <= 0 || data == NULL) {
		strbuf_clear(buf);
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

int strbuf_sets(strbuf *buf, const char *string)
{
	return strbuf_set(buf,
		(const unsigned char *)string,
		string ? strlen(string) : 0);
}

int strbuf_putc(strbuf *buf, int c)
{
	ENSURE_SIZE(buf, buf->size + 2);
	buf->ptr[buf->size++] = c;
	buf->ptr[buf->size] = '\0';
	return 0;
}

int strbuf_put(strbuf *buf, const unsigned char *data, int len)
{
	if (len <= 0)
		return 0;

	ENSURE_SIZE(buf, buf->size + len + 1);
	memmove(buf->ptr + buf->size, data, len);
	buf->size += len;
	buf->ptr[buf->size] = '\0';
	return 0;
}

int strbuf_puts(strbuf *buf, const char *string)
{
	return strbuf_put(buf, (const unsigned char *)string, strlen(string));
}

int strbuf_vprintf(strbuf *buf, const char *format, va_list ap)
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

		if (len < 0) {
			free(buf->ptr);
			buf->ptr = strbuf__oom;
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

int strbuf_printf(strbuf *buf, const char *format, ...)
{
	int r;
	va_list ap;

	va_start(ap, format);
	r = strbuf_vprintf(buf, format, ap);
	va_end(ap);

	return r;
}

void strbuf_copy_cstr(char *data, int datasize, const strbuf *buf)
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

void strbuf_swap(strbuf *buf_a, strbuf *buf_b)
{
	strbuf t = *buf_a;
	*buf_a = *buf_b;
	*buf_b = t;
}

unsigned char *strbuf_detach(strbuf *buf)
{
	unsigned char *data = buf->ptr;

	if (buf->asize == 0 || buf->ptr == strbuf__oom) {
		/* return an empty string */
		return calloc(1, 1);
	}

	strbuf_init(buf, 0);
	return data;
}

void strbuf_attach(strbuf *buf, unsigned char *ptr, int asize)
{
	strbuf_free(buf);

	if (ptr) {
		buf->ptr = ptr;
		buf->size = strlen((char *)ptr);
		if (asize)
			buf->asize = (asize < buf->size) ? buf->size + 1 : asize;
		else /* pass 0 to fall back on strlen + 1 */
			buf->asize = buf->size + 1;
	} else {
		strbuf_grow(buf, asize);
	}
}

int strbuf_cmp(const strbuf *a, const strbuf *b)
{
	int result = memcmp(a->ptr, b->ptr, MIN(a->size, b->size));
	return (result != 0) ? result :
		(a->size < b->size) ? -1 : (a->size > b->size) ? 1 : 0;
}

int strbuf_strchr(const strbuf *buf, int c, int pos)
{
	const unsigned char *p = memchr(buf->ptr + pos, c, buf->size - pos);
	if (!p)
		return -1;

	return (int)(p - (const unsigned char *)buf->ptr);
}

int strbuf_strrchr(const strbuf *buf, int c, int pos)
{
	int i;

	for (i = pos; i >= 0; i--) {
		if (buf->ptr[i] == (unsigned char) c)
			return i;
	}

	return -1;
}

void strbuf_truncate(strbuf *buf, int len)
{
	if (len < buf->size) {
		buf->size = len;
		buf->ptr[buf->size] = '\0';
	}
}

void strbuf_drop(strbuf *buf, int n)
{
	if (n > 0) {
		buf->size = buf->size - n;
		if (buf->size)
			memmove(buf->ptr, buf->ptr + n, buf->size);

		buf->ptr[buf->size] = '\0';
	}
}

void strbuf_trim(strbuf *buf)
{
	int i = 0;

	if (!buf->size)
		return;

	while (i < buf->size && isspace(buf->ptr[i]))
		i++;

	strbuf_drop(buf, i);

	/* rtrim */
	while (buf->size > 0) {
		if (!isspace(buf->ptr[buf->size - 1]))
			break;

		buf->size--;
	}

	buf->ptr[buf->size] = '\0';
}

// Destructively modify string, collapsing consecutive
// space and newline characters into a single space.
void strbuf_normalize_whitespace(strbuf *s)
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

	strbuf_truncate(s, w);
}

// Destructively unescape a string: remove backslashes before punctuation chars.
extern void strbuf_unescape(strbuf *buf)
{
	int r, w;

	for (r = 0, w = 0; r < buf->size; ++r) {
		if (buf->ptr[r] == '\\' && ispunct(buf->ptr[r + 1]))
			continue;

		buf->ptr[w++] = buf->ptr[r];
	}

	strbuf_truncate(buf, w);
}

