#ifndef CMARK_BUFFER_H
#define CMARK_BUFFER_H

#include <stddef.h>
#include <stdarg.h>
#include "config.h"
#include "cmark_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned char *ptr;
	int asize, size;
} cmark_strbuf;

CMARK_EXPORT
extern unsigned char cmark_strbuf__initbuf[];

CMARK_EXPORT
extern unsigned char cmark_strbuf__oom[];

#define CMARK_GH_BUF_INIT { cmark_strbuf__initbuf, 0, 0 }

/**
 * Initialize a strbuf structure.
 *
 * For the cases where GH_BUF_INIT cannot be used to do static
 * initialization.
 */
CMARK_EXPORT
void cmark_strbuf_init(cmark_strbuf *buf, int initial_size);

/**
 * Attempt to grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error.  If mark_oom is true,
 * this will mark the buffer as invalid for future operations; if false,
 * existing buffer content will be preserved, but calling code must handle
 * that buffer was not expanded.
 */
CMARK_EXPORT
int cmark_strbuf_try_grow(cmark_strbuf *buf, int target_size, bool mark_oom);

/**
 * Grow the buffer to hold at least `target_size` bytes.
 *
 * If the allocation fails, this will return an error and the buffer will be
 * marked as invalid for future operations, invaliding contents.
 *
 * @return 0 on success or -1 on failure
 */
CMARK_EXPORT
int cmark_strbuf_grow(cmark_strbuf *buf, int target_size);

CMARK_EXPORT
void cmark_strbuf_free(cmark_strbuf *buf);
CMARK_EXPORT
void cmark_strbuf_swap(cmark_strbuf *buf_a, cmark_strbuf *buf_b);

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
CMARK_EXPORT
bool cmark_strbuf_oom(const cmark_strbuf *buf);

CMARK_EXPORT
size_t cmark_strbuf_len(const cmark_strbuf *buf);

CMARK_EXPORT
int cmark_strbuf_cmp(const cmark_strbuf *a, const cmark_strbuf *b);

CMARK_EXPORT
void cmark_strbuf_attach(cmark_strbuf *buf, unsigned char *ptr, int asize);
CMARK_EXPORT
unsigned char *cmark_strbuf_detach(cmark_strbuf *buf);
CMARK_EXPORT
void cmark_strbuf_copy_cstr(char *data, int datasize, const cmark_strbuf *buf);

static inline const char *cmark_strbuf_cstr(const cmark_strbuf *buf)
{
	return (char *)buf->ptr;
}

#define cmark_strbuf_at(buf, n) ((buf)->ptr[n])

/*
 * Functions below that return int value error codes will return 0 on
 * success or -1 on failure (which generally means an allocation failed).
 * Using a strbuf where the allocation has failed with result in -1 from
 * all further calls using that buffer.  As a result, you can ignore the
 * return code of these functions and call them in a series then just call
 * strbuf_oom at the end.
 */
CMARK_EXPORT
int cmark_strbuf_set(cmark_strbuf *buf, const unsigned char *data, int len);
CMARK_EXPORT
int cmark_strbuf_sets(cmark_strbuf *buf, const char *string);
CMARK_EXPORT
int cmark_strbuf_putc(cmark_strbuf *buf, int c);
CMARK_EXPORT
int cmark_strbuf_put(cmark_strbuf *buf, const unsigned char *data, int len);
CMARK_EXPORT
int cmark_strbuf_puts(cmark_strbuf *buf, const char *string);
CMARK_EXPORT
int cmark_strbuf_printf(cmark_strbuf *buf, const char *format, ...)
	CMARK_ATTRIBUTE((format (printf, 2, 3)));
CMARK_EXPORT
int cmark_strbuf_vprintf(cmark_strbuf *buf, const char *format, va_list ap);
CMARK_EXPORT
void cmark_strbuf_clear(cmark_strbuf *buf);

CMARK_EXPORT
int cmark_strbuf_strchr(const cmark_strbuf *buf, int c, int pos);
CMARK_EXPORT
int cmark_strbuf_strrchr(const cmark_strbuf *buf, int c, int pos);
CMARK_EXPORT
void cmark_strbuf_drop(cmark_strbuf *buf, int n);
CMARK_EXPORT
void cmark_strbuf_truncate(cmark_strbuf *buf, int len);
CMARK_EXPORT
void cmark_strbuf_rtrim(cmark_strbuf *buf);
CMARK_EXPORT
void cmark_strbuf_trim(cmark_strbuf *buf);
CMARK_EXPORT
void cmark_strbuf_normalize_whitespace(cmark_strbuf *s);
CMARK_EXPORT
void cmark_strbuf_unescape(cmark_strbuf *s);

// Convenience macros
#define strbuf                        cmark_strbuf
#define strbuf__initbuf               cmark_strbuf__initbuf
#define strbuf__oom                   cmark_strbuf__oom
#define GH_BUF_INIT                   CMARK_GH_BUF_INIT
#define strbuf_init                   cmark_strbuf_init
#define strbuf_try_grow               cmark_strbuf_try_grow
#define strbuf_grow                   cmark_strbuf_grow
#define strbuf_free                   cmark_strbuf_free
#define strbuf_swap                   cmark_strbuf_swap
#define strbuf_oom                    cmark_strbuf_oom
#define strbuf_len                    cmark_strbuf_len
#define strbuf_cmp                    cmark_strbuf_cmp
#define strbuf_attach                 cmark_strbuf_attach
#define strbuf_detach                 cmark_strbuf_detach
#define strbuf_copy_cstr              cmark_strbuf_copy_cstr
#define strbuf_at                     cmark_strbuf_at
#define strbuf_set                    cmark_strbuf_set
#define strbuf_sets                   cmark_strbuf_sets
#define strbuf_putc                   cmark_strbuf_putc
#define strbuf_put                    cmark_strbuf_put
#define strbuf_puts                   cmark_strbuf_puts
#define strbuf_printf                 cmark_strbuf_printf
#define strbuf_vprintf                cmark_strbuf_vprintf
#define strbuf_clear                  cmark_strbuf_clear
#define strbuf_strchr                 cmark_strbuf_strchr
#define strbuf_strrchr                cmark_strbuf_strrchr
#define strbuf_drop                   cmark_strbuf_drop
#define strbuf_truncate               cmark_strbuf_truncate
#define strbuf_rtrim                  cmark_strbuf_rtrim
#define strbuf_trim                   cmark_strbuf_trim
#define strbuf_normalize_whitespace   cmark_strbuf_normalize_whitespace
#define strbuf_unescape               cmark_strbuf_unescape

#ifdef __cplusplus
}
#endif

#endif
