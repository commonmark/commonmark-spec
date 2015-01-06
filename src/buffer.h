#ifndef CMARK_BUFFER_H
#define CMARK_BUFFER_H

#include <stddef.h>
#include <stdarg.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned char *ptr;
	int asize, size;
} cmark_strbuf;

extern unsigned char cmark_strbuf__initbuf[];

extern unsigned char cmark_strbuf__oom[];

#define GH_BUF_INIT { cmark_strbuf__initbuf, 0, 0 }

/**
 * Initialize a cmark_strbuf structure.
 *
 * For the cases where GH_BUF_INIT cannot be used to do static
 * initialization.
 */
void cmark_strbuf_init(cmark_strbuf *buf, int initial_size);

/**
 * Attempt to grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error.  If mark_oom is true,
 * this will mark the buffer as invalid for future operations; if false,
 * existing buffer content will be preserved, but calling code must handle
 * that buffer was not expanded.
 */
int cmark_strbuf_try_grow(cmark_strbuf *buf, int target_size, bool mark_oom);

/**
 * Grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error and the buffer will be
 * marked as invalid for future operations, invaliding contents.
 *
 * @return 0 on success or -1 on failure
 */
int cmark_strbuf_grow(cmark_strbuf *buf, int target_size);

void cmark_strbuf_free(cmark_strbuf *buf);
void cmark_strbuf_swap(cmark_strbuf *buf_a, cmark_strbuf *buf_b);

/**
 * Test if there have been any reallocation failures with this cmark_strbuf.
 *
 * Any function that writes to a cmark_strbuf can fail due to memory allocation
 * issues.  If one fails, the cmark_strbuf will be marked with an OOM error and
 * further calls to modify the buffer will fail.  Check cmark_strbuf_oom() at the
 * end of your sequence and it will be true if you ran out of memory at any
 * point with that buffer.
 *
 * @return false if no error, true if allocation error
 */
bool cmark_strbuf_oom(const cmark_strbuf *buf);

size_t cmark_strbuf_len(const cmark_strbuf *buf);

int cmark_strbuf_cmp(const cmark_strbuf *a, const cmark_strbuf *b);

void cmark_strbuf_attach(cmark_strbuf *buf, unsigned char *ptr, int asize);
unsigned char *cmark_strbuf_detach(cmark_strbuf *buf);
void cmark_strbuf_copy_cstr(char *data, int datasize, const cmark_strbuf *buf);

static inline const char *cmark_strbuf_cstr(const cmark_strbuf *buf)
{
	return (char *)buf->ptr;
}

#define cmark_strbuf_at(buf, n) ((buf)->ptr[n])

/*
 * Functions below that return int value error codes will return 0 on
 * success or -1 on failure (which generally means an allocation failed).
 * Using a cmark_strbuf where the allocation has failed with result in -1 from
 * all further calls using that buffer.  As a result, you can ignore the
 * return code of these functions and call them in a series then just call
 * cmark_strbuf_oom at the end.
 */
int cmark_strbuf_set(cmark_strbuf *buf, const unsigned char *data, int len);
int cmark_strbuf_sets(cmark_strbuf *buf, const char *string);
int cmark_strbuf_putc(cmark_strbuf *buf, int c);
int cmark_strbuf_put(cmark_strbuf *buf, const unsigned char *data, int len);
int cmark_strbuf_puts(cmark_strbuf *buf, const char *string);
int cmark_strbuf_printf(cmark_strbuf *buf, const char *format, ...)
CMARK_ATTRIBUTE((format (printf, 2, 3)));
int cmark_strbuf_vprintf(cmark_strbuf *buf, const char *format, va_list ap);
void cmark_strbuf_clear(cmark_strbuf *buf);

int cmark_strbuf_strchr(const cmark_strbuf *buf, int c, int pos);
int cmark_strbuf_strrchr(const cmark_strbuf *buf, int c, int pos);
void cmark_strbuf_drop(cmark_strbuf *buf, int n);
void cmark_strbuf_truncate(cmark_strbuf *buf, int len);
void cmark_strbuf_rtrim(cmark_strbuf *buf);
void cmark_strbuf_trim(cmark_strbuf *buf);
void cmark_strbuf_normalize_whitespace(cmark_strbuf *s);
void cmark_strbuf_unescape(cmark_strbuf *s);

#ifdef __cplusplus
}
#endif

#endif
