#ifndef CMARK_NODE_H
#define CMARK_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "cmark.h"
#include "buffer.h"
#include "chunk.h"

typedef struct {
	cmark_list_type   list_type;
	int               marker_offset;
	int               padding;
	int               start;
	cmark_delim_type  delimiter;
	unsigned char     bullet_char;
	bool              tight;
} cmark_list;

typedef struct {
	bool              fenced;
	int               fence_length;
	int               fence_offset;
	unsigned char     fence_char;
	cmark_strbuf      info;
} cmark_code;

typedef struct {
	int level;
	bool setext;
} cmark_header;

typedef struct {
	unsigned char *url;
	unsigned char *title;
} cmark_link;

struct cmark_node {
	cmark_node_type type;

	struct cmark_node *next;
	struct cmark_node *prev;
	struct cmark_node *parent;
	struct cmark_node *first_child;
	struct cmark_node *last_child;

	int start_line;
	int start_column;
	int end_line;
	bool open;
	bool last_line_blank;

	cmark_strbuf string_content;

	union {
		cmark_chunk       literal;
		cmark_list        list;
		cmark_code        code;
		cmark_header      header;
		cmark_link        link;
	} as;
};

CMARK_EXPORT int
cmark_node_check(cmark_node *node, FILE *out);

#ifdef __cplusplus
}
#endif

#endif

