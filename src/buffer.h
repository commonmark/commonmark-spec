#ifndef INCLUDE_buffer_h__
#define INCLUDE_buffer_h__

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

typedef struct {
	unsigned char *ptr;
	int asize, size;
} strbuf;

extern unsigned char strbuf__initbuf[];
extern unsigned char strbuf__oom[];

#define GH_BUF_INIT { strbuf__initbuf, 0, 0 }

/**
 * Initialize a strbuf structure.
 *
 * For the cases where GH_BUF_INIT cannot be used to do static
 * initialization.
 */
extern void strbuf_init(strbuf *buf, int initial_size);

/**
 * Attempt to grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error.  If mark_oom is true,
 * this will mark the buffer as invalid for future operations; if false,
 * existing buffer content will be preserved, but calling code must handle
 * that buffer was not expanded.
 */
extern int strbuf_try_grow(strbuf *buf, int target_size, bool mark_oom);

/**
 * Grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error and the buffer will be
 * marked as invalid for future operations, invaliding contents.
 *
 * @return 0 on success or -1 on failure
 */
static inline int strbuf_grow(strbuf *buf, int target_size)
{
	return strbuf_try_grow(buf, target_size, true);
}

extern void strbuf_free(strbuf *buf);
extern void strbuf_swap(strbuf *buf_a, strbuf *buf_b);

/**
 * Test if there have been any reallocation failures with this strbuf.
 *
 * Any function that writes to a strbuf can fail due to memory allocation
 * issues.  If one fails, the strbuf will be marked with an OOM error and
 * further calls to modify the buffer will fail.  Check strbuf_oom() at the
 * end of your sequence and it will be true if you ran out of memory at any
 * point with that buffer.
 *
 * @return false if no error, true if allocation error
 */
static inline bool strbuf_oom(const strbuf *buf)
{
	return (buf->ptr == strbuf__oom);
}


static inline size_t strbuf_len(const strbuf *buf)
{
	return buf->size;
}

extern int strbuf_cmp(const strbuf *a, const strbuf *b);

extern void strbuf_attach(strbuf *buf, unsigned char *ptr, int asize);
extern unsigned char *strbuf_detach(strbuf *buf);
extern void strbuf_copy_cstr(char *data, int datasize, const strbuf *buf);

static inline const char *strbuf_cstr(const strbuf *buf)
{
	return (char *)buf->ptr;
}

#define strbuf_at(buf, n) ((buf)->ptr[n])

/*
 * Functions below that return int value error codes will return 0 on
 * success or -1 on failure (which generally means an allocation failed).
 * Using a strbuf where the allocation has failed with result in -1 from
 * all further calls using that buffer.  As a result, you can ignore the
 * return code of these functions and call them in a series then just call
 * strbuf_oom at the end.
 */
extern int strbuf_set(strbuf *buf, const unsigned char *data, int len);
extern int strbuf_sets(strbuf *buf, const char *string);
extern int strbuf_putc(strbuf *buf, int c);
extern int strbuf_put(strbuf *buf, const unsigned char *data, int len);
extern int strbuf_puts(strbuf *buf, const char *string);
extern int strbuf_printf(strbuf *buf, const char *format, ...)
	__attribute__((format (printf, 2, 3)));
extern int strbuf_vprintf(strbuf *buf, const char *format, va_list ap);
extern void strbuf_clear(strbuf *buf);

int strbuf_strchr(const strbuf *buf, int c, int pos);
int strbuf_strrchr(const strbuf *buf, int c, int pos);
void strbuf_drop(strbuf *buf, int n);
void strbuf_truncate(strbuf *buf, int len);
void strbuf_rtrim(strbuf *buf);
void strbuf_trim(strbuf *buf);
void strbuf_normalize_whitespace(strbuf *s);
void strbuf_unescape(strbuf *s);

#endif
