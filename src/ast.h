#ifndef _AST_H_
#define _AST_H_

#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "chunk.h"
#include "references.h"

struct cmark_node_inl {
	enum {
		CMARK_INL_STRING,
		CMARK_INL_SOFTBREAK,
		CMARK_INL_LINEBREAK,
		CMARK_INL_CODE,
		CMARK_INL_RAW_HTML,
		CMARK_INL_EMPH,
		CMARK_INL_STRONG,
		CMARK_INL_LINK,
		CMARK_INL_IMAGE
	} tag;
	union {
		cmark_chunk literal;
		struct cmark_node_inl *inlines;
		struct {
			struct cmark_node_inl *label;
			unsigned char *url;
			unsigned char *title;
		} linkable;
	} content;
	struct cmark_node_inl *next;
};

typedef struct cmark_node_inl cmark_node_inl;

// Types for blocks
struct cmark_ListData {
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

struct cmark_FencedCodeData {
	int               fence_length;
	int               fence_offset;
	unsigned char     fence_char;
	cmark_strbuf      info;
};

struct cmark_node_block {
	enum {
		CMARK_BLOCK_DOCUMENT,
		CMARK_BLOCK_BQUOTE,
		CMARK_BLOCK_LIST,
		CMARK_BLOCK_LIST_ITEM,
		CMARK_BLOCK_FENCED_CODE,
		CMARK_BLOCK_INDENTED_CODE,
		CMARK_BLOCK_HTML,
		CMARK_BLOCK_PARAGRAPH,
		CMARK_BLOCK_ATX_HEADER,
		CMARK_BLOCK_SETEXT_HEADER,
		CMARK_BLOCK_HRULE,
		CMARK_BLOCK_REFERENCE_DEF
	} tag;
	int start_line;
	int start_column;
	int end_line;
	bool open;
	bool last_line_blank;
	struct cmark_node_block* children;
	struct cmark_node_block* last_child;
	struct cmark_node_block* parent;
	struct cmark_node_block* top;
	cmark_strbuf string_content;
	cmark_node_inl* inline_content;

	union  {
		struct cmark_ListData list;
		struct cmark_FencedCodeData code;
		struct {
			int level;
		} header;
		struct {
			cmark_reference_map *refmap;
		} document;
	} as;

	struct cmark_node_block *next;
	struct cmark_node_block *prev;
};

typedef struct cmark_node_block cmark_node_block;

void cmark_free_blocks(cmark_node_block *e);

#ifndef CMARK_NO_SHORT_NAMES
  #define node_inl                  cmark_node_inl
  #define INL_STRING                CMARK_INL_STRING
  #define INL_SOFTBREAK             CMARK_INL_SOFTBREAK
  #define INL_LINEBREAK             CMARK_INL_LINEBREAK
  #define INL_CODE                  CMARK_INL_CODE
  #define INL_RAW_HTML              CMARK_INL_RAW_HTML
  #define INL_EMPH                  CMARK_INL_EMPH
  #define INL_STRONG                CMARK_INL_STRONG
  #define INL_LINK                  CMARK_INL_LINK
  #define INL_IMAGE                 CMARK_INL_IMAGE
  #define ListData                  cmark_ListData
  #define FencedCodeData            cmark_FencedCodeData
  #define node_block                cmark_node_block
  #define BLOCK_DOCUMENT            CMARK_BLOCK_DOCUMENT
  #define BLOCK_BQUOTE              CMARK_BLOCK_BQUOTE
  #define BLOCK_LIST                CMARK_BLOCK_LIST
  #define BLOCK_LIST_ITEM           CMARK_BLOCK_LIST_ITEM
  #define BLOCK_FENCED_CODE         CMARK_BLOCK_FENCED_CODE
  #define BLOCK_INDENTED_CODE       CMARK_BLOCK_INDENTED_CODE
  #define BLOCK_HTML                CMARK_BLOCK_HTML
  #define BLOCK_PARAGRAPH           CMARK_BLOCK_PARAGRAPH
  #define BLOCK_ATX_HEADER          CMARK_BLOCK_ATX_HEADER
  #define BLOCK_SETEXT_HEADER       CMARK_BLOCK_SETEXT_HEADER
  #define BLOCK_HRULE               CMARK_BLOCK_HRULE
  #define BLOCK_REFERENCE_DEF       CMARK_BLOCK_REFERENCE_DEF
#endif

#endif
