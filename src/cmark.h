#ifndef _STDMD_H_
#define _STDMD_H_

#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "chunk.h"
#include "references.h"

#define VERSION "0.1"
#define CODE_INDENT 4

#define MAX_LINK_LABEL_LENGTH 1000

struct node_inl {
	enum {
		INL_STRING,
		INL_SOFTBREAK,
		INL_LINEBREAK,
		INL_CODE,
		INL_RAW_HTML,
		INL_EMPH,
		INL_STRONG,
		INL_LINK,
		INL_IMAGE
	} tag;
	union {
		chunk literal;
		struct node_inl *inlines;
		struct {
			struct node_inl *label;
			unsigned char *url;
			unsigned char *title;
		} linkable;
	} content;
	struct node_inl *next;
};

typedef struct node_inl node_inl;

// Types for blocks
struct ListData {
	enum {
		bullet,
		ordered
	}  list_type;
	int               marker_offset;
	int               padding;
	int               start;
	enum {
		period,
		parens
	} delimiter;
	unsigned char     bullet_char;
	bool              tight;
};

struct FencedCodeData {
	int               fence_length;
	int               fence_offset;
	unsigned char     fence_char;
	strbuf            info;
};

struct node_block {
	enum {
		BLOCK_DOCUMENT,
		BLOCK_BQUOTE,
		BLOCK_LIST,
		BLOCK_LIST_ITEM,
		BLOCK_FENCED_CODE,
		BLOCK_INDENTED_CODE,
		BLOCK_HTML,
		BLOCK_PARAGRAPH,
		BLOCK_ATX_HEADER,
		BLOCK_SETEXT_HEADER,
		BLOCK_HRULE,
		BLOCK_REFERENCE_DEF
	} tag;
	int start_line;
	int start_column;
	int end_line;
	bool open;
	bool last_line_blank;
	struct node_block* children;
	struct node_block* last_child;
	struct node_block* parent;
	struct node_block* top;
	strbuf string_content;
	node_inl* inline_content;

	union  {
		struct ListData list;
		struct FencedCodeData code;
		struct {
			int level;
		} header;
		struct {
			reference_map *refmap;
		} document;
	} as;

	struct node_block *next;
	struct node_block *prev;
};

typedef struct node_block node_block;

node_block *cmark_parse_document(const unsigned char *buffer, size_t len);
node_block *cmark_parse_file(FILE *f);

void cmark_free_nodes(node_block *e);

void cmark_debug_print(node_block *root);
void cmark_render_html(strbuf *html, node_block *root);

#endif
