#ifndef CMARK_CHUNK_H
#define CMARK_CHUNK_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "cmark_ctype.h"
#include "buffer.h"

typedef struct {
	unsigned char *data;
	int len;
	int alloc;  // also implies a NULL-terminated string
} cmark_chunk;

static inline void cmark_chunk_free(cmark_chunk *c)
{
	if (c->alloc)
		free(c->data);

	c->data = NULL;
	c->alloc = 0;
	c->len = 0;
}

static inline void cmark_chunk_ltrim(cmark_chunk *c)
{
	assert(!c->alloc);

	while (c->len && cmark_isspace(c->data[0])) {
		c->data++;
		c->len--;
	}
}

static inline void cmark_chunk_rtrim(cmark_chunk *c)
{
	while (c->len > 0) {
		if (!cmark_isspace(c->data[c->len - 1]))
			break;

		c->len--;
	}
}

static inline void cmark_chunk_trim(cmark_chunk *c)
{
	cmark_chunk_ltrim(c);
	cmark_chunk_rtrim(c);
}

static inline int cmark_chunk_strchr(cmark_chunk *ch, int c, int offset)
{
	const unsigned char *p = (unsigned char *)memchr(ch->data + offset, c, ch->len - offset);
	return p ? (int)(p - ch->data) : ch->len;
}

static inline const char *cmark_chunk_to_cstr(cmark_chunk *c)
{
	unsigned char *str;

	if (c->alloc) {
		return (char *)c->data;
	}
	str = (unsigned char *)malloc(c->len + 1);
	if(str != NULL) {
		memcpy(str, c->data, c->len);
		str[c->len] = 0;
	}
	c->data  = str;
	c->alloc = 1;

	return (char *)str;
}

static inline void cmark_chunk_set_cstr(cmark_chunk *c, const char *str)
{
	if (c->alloc) {
		free(c->data);
	}
	c->len   = strlen(str);
	c->data  = (unsigned char *)malloc(c->len + 1);
	c->alloc = 1;
	memcpy(c->data, str, c->len + 1);
}

static inline cmark_chunk cmark_chunk_literal(const char *data)
{
	cmark_chunk c = {(unsigned char *)data, data ? strlen(data) : 0, 0};
	return c;
}

static inline cmark_chunk cmark_chunk_dup(const cmark_chunk *ch, int pos, int len)
{
	cmark_chunk c = {ch->data + pos, len, 0};
	return c;
}

static inline cmark_chunk cmark_chunk_buf_detach(cmark_strbuf *buf)
{
	cmark_chunk c;

	c.len = buf->size;
	c.data = cmark_strbuf_detach(buf);
	c.alloc = 1;

	return c;
}

#endif
