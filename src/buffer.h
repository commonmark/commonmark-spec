#ifndef INCLUDE_buffer_h__
#define INCLUDE_buffer_h__

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

typedef struct {
	unsigned char *ptr;
	int asize, size;
} gh_buf;

extern unsigned char gh_buf__initbuf[];
extern unsigned char gh_buf__oom[];

#define GH_BUF_INIT { gh_buf__initbuf, 0, 0 }

/**
 * Initialize a gh_buf structure.
 *
 * For the cases where GH_BUF_INIT cannot be used to do static
 * initialization.
 */
extern void gh_buf_init(gh_buf *buf, int initial_size);

static inline void gh_buf_static(gh_buf *buf, unsigned char *source)
{
	buf->ptr = source;
	buf->size = strlen(source);
	buf->asize = -1;
}

/**
 * Attempt to grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error.  If mark_oom is true,
 * this will mark the buffer as invalid for future operations; if false,
 * existing buffer content will be preserved, but calling code must handle
 * that buffer was not expanded.
 */
extern int gh_buf_try_grow(gh_buf *buf, int target_size, bool mark_oom);

/**
 * Grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error and the buffer will be
 * marked as invalid for future operations, invaliding contents.
 *
 * @return 0 on success or -1 on failure
 */
static inline int gh_buf_grow(gh_buf *buf, int target_size)
{
	return gh_buf_try_grow(buf, target_size, true);
}

extern void gh_buf_free(gh_buf *buf);
extern void gh_buf_swap(gh_buf *buf_a, gh_buf *buf_b);

/**
 * Test if there have been any reallocation failures with this gh_buf.
 *
 * Any function that writes to a gh_buf can fail due to memory allocation
 * issues.  If one fails, the gh_buf will be marked with an OOM error and
 * further calls to modify the buffer will fail.  Check gh_buf_oom() at the
 * end of your sequence and it will be true if you ran out of memory at any
 * point with that buffer.
 *
 * @return false if no error, true if allocation error
 */
static inline bool gh_buf_oom(const gh_buf *buf)
{
	return (buf->ptr == gh_buf__oom);
}


static inline size_t gh_buf_len(const gh_buf *buf)
{
	return buf->size;
}

extern int gh_buf_cmp(const gh_buf *a, const gh_buf *b);

extern void gh_buf_attach(gh_buf *buf, char *ptr, int asize);
extern char *gh_buf_detach(gh_buf *buf);
extern void gh_buf_copy_cstr(char *data, int datasize, const gh_buf *buf);

static inline const char *gh_buf_cstr(const gh_buf *buf)
{
	return buf->ptr;
}

#define gh_buf_at(buf, n) ((buf)->ptr[n])

/*
 * Functions below that return int value error codes will return 0 on
 * success or -1 on failure (which generally means an allocation failed).
 * Using a gh_buf where the allocation has failed with result in -1 from
 * all further calls using that buffer.  As a result, you can ignore the
 * return code of these functions and call them in a series then just call
 * gh_buf_oom at the end.
 */
extern int gh_buf_set(gh_buf *buf, const char *data, int len);
extern int gh_buf_sets(gh_buf *buf, const char *string);
extern int gh_buf_putc(gh_buf *buf, char c);
extern int gh_buf_put(gh_buf *buf, const char *data, int len);
extern int gh_buf_puts(gh_buf *buf, const char *string);
extern int gh_buf_printf(gh_buf *buf, const char *format, ...)
	__attribute__((format (printf, 2, 3)));
extern int gh_buf_vprintf(gh_buf *buf, const char *format, va_list ap);
extern void gh_buf_clear(gh_buf *buf);

int gh_buf_strchr(const gh_buf *buf, int c, int pos);
int gh_buf_strrchr(const gh_buf *buf, int c, int pos);
void gh_buf_truncate(gh_buf *buf, int len);
void gh_buf_ltruncate(gh_buf *buf, int len);
void gh_buf_trim(gh_buf *buf);

#endif
