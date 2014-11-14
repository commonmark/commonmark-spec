#ifndef _CMARK_H_
#define _CMARK_H_

#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "chunk.h"
#include "references.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMARK_VERSION "0.1"
#define CMARK_CODE_INDENT 4

#define CMARK_MAX_LINK_LABEL_LENGTH 1000

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
void cmark_free_inlines(cmark_node_inl* e);
void cmark_free_simple(cmark_node_inl *e);
cmark_node_inl* cmark_append_inlines(cmark_node_inl* a, cmark_node_inl* b);

cmark_node_inl *cmark_make_link(cmark_node_inl *label, unsigned char *url, unsigned char *title);
cmark_node_inl* cmark_make_autolink(cmark_node_inl* label, cmark_chunk url, int is_email);

cmark_node_inl* cmark_make_inlines(int t, cmark_node_inl* contents);
cmark_node_inl* cmark_make_literal(int t, cmark_chunk s);
cmark_node_inl* cmark_make_simple(int t);

// Macros for creating various kinds of simple.
#define cmark_make_str(s) cmark_make_literal(INL_STRING, s)
#define cmark_make_code(s) cmark_make_literal(INL_CODE, s)
#define cmark_make_raw_html(s) cmark_make_literal(INL_RAW_HTML, s)
#define cmark_make_linebreak() cmark_make_simple(INL_LINEBREAK)
#define cmark_make_softbreak() cmark_make_simple(INL_SOFTBREAK)
#define cmark_make_emph(contents) cmark_make_inlines(INL_EMPH, contents)
#define cmark_make_strong(contents) cmark_make_inlines(INL_STRONG, contents)

__attribute__((visibility("default")))
cmark_node_block *cmark_parse_document(const unsigned char *buffer, size_t len);

__attribute__((visibility("default")))
cmark_node_block *cmark_parse_file(FILE *f);

__attribute__((visibility("default")))
void cmark_debug_print(cmark_node_block *root);

__attribute__((visibility("default")))
void cmark_render_html(cmark_strbuf *html, cmark_node_block *root);

__attribute__((visibility("default")))
unsigned char *cmark_markdown_to_html(unsigned char *text, int len);



#ifndef CMARK_NO_SHORT_NAMES
  #define VERSION                   CMARK_VERSION
  #define CODE_INDENT               CMARK_CODE_INDENT
  #define MAX_LINK_LABEL_LENGTH     CMARK_MAX_LINK_LABEL_LENGTH
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
  #define free_simple               cmark_free_simple
  #define free_blocks               cmark_free_blocks
  #define append_simple             cmark_append_simple
  #define make_link                 cmark_make_link
  #define make_autolink             cmark_make_autolink
  #define make_str                  cmark_make_str
  #define make_code                 cmark_make_code
  #define make_raw_html             cmark_make_raw_html
  #define make_linebreak            cmark_make_linebreak
  #define make_softbreak            cmark_make_softbreak
  #define make_emph                 cmark_make_emph
  #define make_strong               cmark_make_strong
  #define make_simple              cmark_make_simple
  #define make_simple              cmark_make_simple
  #define make_simple              cmark_make_simple
#endif

#ifdef __cplusplus
}
#endif

#endif
