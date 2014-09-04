#ifndef _STDMD_H_
#define _STDMD_H_

#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "chunk.h"
#include "uthash.h"

#define VERSION "0.1"
#define CODE_INDENT 4

struct node_inl {
	enum {
		INL_STRING,
		INL_SOFTBREAK,
		INL_LINEBREAK,
		INL_CODE,
		INL_RAW_HTML,
		INL_ENTITY,
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

struct reference {
  unsigned char *label;
  unsigned char *url;
  unsigned char *title;
  UT_hash_handle  hh; // used by uthash
};

typedef struct reference reference;

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
	char              fence_char;
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
		struct ListData list_data;
		struct FencedCodeData fenced_code_data;
		int header_level;
		reference** refmap;
	}                     attributes;
	struct node_block *     next;
	struct node_block *     prev;
};

typedef struct node_block node_block;

node_inl* parse_inlines(strbuf *input, reference** refmap);
void free_inlines(node_inl* e);

int parse_reference(strbuf *input, reference** refmap);
void free_reference(reference *ref);
void free_reference_map(reference **refmap);

void add_reference(reference** refmap, reference* ref);
void unescape_buffer(strbuf *buf);

extern node_block* make_document();
extern node_block* add_child(node_block* parent,
                        int block_type, int start_line, int start_column);
void free_blocks(node_block* e);

extern node_block *stmd_parse_document(const unsigned char *buffer, size_t len);
extern node_block *stmd_parse_file(FILE *f);

void print_inlines(node_inl* ils, int indent);
void print_blocks(node_block* blk, int indent);

void blocks_to_html(strbuf *html, node_block *b, bool tight);
void inlines_to_html(strbuf *html, node_inl *b);

void utf8proc_case_fold(strbuf *dest, const unsigned char *str, int len);

#endif
