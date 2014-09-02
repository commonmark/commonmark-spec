#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include "buffer.h"

/* Used as default value for gh_buf->ptr so that people can always
 * assume ptr is non-NULL and zero terminated even for new gh_bufs.
 */
unsigned char gh_buf__initbuf[1];
unsigned char gh_buf__oom[1];

#define ENSURE_SIZE(b, d) \
	if ((d) > buf->asize && gh_buf_grow(b, (d)) < 0)\
		return -1;

void gh_buf_init(gh_buf *buf, int initial_size)
{
	buf->asize = 0;
	buf->size = 0;
	buf->ptr = gh_buf__initbuf;

	if (initial_size)
		gh_buf_grow(buf, initial_size);
}

int gh_buf_try_grow(gh_buf *buf, int target_size, bool mark_oom)
{
	unsigned char *new_ptr;
	int new_size;

	if (buf->ptr == gh_buf__oom)
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
			buf->ptr = gh_buf__oom;
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

void gh_buf_free(gh_buf *buf)
{
	if (!buf) return;

	if (buf->ptr != gh_buf__initbuf && buf->ptr != gh_buf__oom)
		free(buf->ptr);

	gh_buf_init(buf, 0);
}

void gh_buf_clear(gh_buf *buf)
{
	buf->size = 0;

	if (buf->asize > 0)
		buf->ptr[0] = '\0';
}

int gh_buf_set(gh_buf *buf, const unsigned char *data, int len)
{
	if (len == 0 || data == NULL) {
		gh_buf_clear(buf);
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

int gh_buf_sets(gh_buf *buf, const char *string)
{
	return gh_buf_set(buf,
		(const unsigned char *)string,
		string ? strlen(string) : 0);
}

int gh_buf_putc(gh_buf *buf, int c)
{
	ENSURE_SIZE(buf, buf->size + 2);
	buf->ptr[buf->size++] = c;
	buf->ptr[buf->size] = '\0';
	return 0;
}

int gh_buf_put(gh_buf *buf, const unsigned char *data, int len)
{
	ENSURE_SIZE(buf, buf->size + len + 1);
	memmove(buf->ptr + buf->size, data, len);
	buf->size += len;
	buf->ptr[buf->size] = '\0';
	return 0;
}

int gh_buf_puts(gh_buf *buf, const char *string)
{
	return gh_buf_put(buf, (const unsigned char *)string, strlen(string));
}

int gh_buf_vprintf(gh_buf *buf, const char *format, va_list ap)
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
			buf->ptr = gh_buf__oom;
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

int gh_buf_printf(gh_buf *buf, const char *format, ...)
{
	int r;
	va_list ap;

	va_start(ap, format);
	r = gh_buf_vprintf(buf, format, ap);
	va_end(ap);

	return r;
}

void gh_buf_copy_cstr(char *data, int datasize, const gh_buf *buf)
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

void gh_buf_swap(gh_buf *buf_a, gh_buf *buf_b)
{
	gh_buf t = *buf_a;
	*buf_a = *buf_b;
	*buf_b = t;
}

unsigned char *gh_buf_detach(gh_buf *buf)
{
	unsigned char *data = buf->ptr;

	if (buf->asize == 0 || buf->ptr == gh_buf__oom)
		return NULL;

	gh_buf_init(buf, 0);

	return data;
}

void gh_buf_attach(gh_buf *buf, unsigned char *ptr, int asize)
{
	gh_buf_free(buf);

	if (ptr) {
		buf->ptr = ptr;
		buf->size = strlen((char *)ptr);
		if (asize)
			buf->asize = (asize < buf->size) ? buf->size + 1 : asize;
		else /* pass 0 to fall back on strlen + 1 */
			buf->asize = buf->size + 1;
	} else {
		gh_buf_grow(buf, asize);
	}
}

int gh_buf_cmp(const gh_buf *a, const gh_buf *b)
{
	int result = memcmp(a->ptr, b->ptr, MIN(a->size, b->size));
	return (result != 0) ? result :
		(a->size < b->size) ? -1 : (a->size > b->size) ? 1 : 0;
}

int gh_buf_strchr(const gh_buf *buf, int c, int pos)
{
	const char *p = memchr(buf->ptr + pos, c, buf->size - pos);
	if (!p)
		return -1;

	return (int)(p - buf->ptr);
}

int gh_buf_strrchr(const gh_buf *buf, int c, int pos)
{
	int i;

	for (i = pos; i >= 0; i--) {
		if (buf->ptr[i] == (unsigned char) c)
			return i;
	}

	return -1;
}

void gh_buf_truncate(gh_buf *buf, size_t len)
{
	if (len < buf->size) {
		buf->size = len;
		buf->ptr[buf->size] = '\0';
	}
}

void gh_buf_trim(gh_buf *buf)
{
	/* TODO: leading whitespace? */
	/*
	while (i < buf->size && isspace(buf->ptr[i]))
		i++;

	gh_buf_truncate(buf, i);
	*/

	/* rtrim */
	while (buf->size > 0) {
		if (!isspace(buf->ptr[buf->size - 1]))
			break;

		buf->size--;
	}

	buf->ptr[buf->size] = '\0';
}
