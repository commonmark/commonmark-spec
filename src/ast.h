#ifndef CMARK_AST_H
#define CMARK_AST_H

#include <stdio.h>
#include "config.h"
#include "node.h"
#include "buffer.h"
#include "chunk.h"
#include "cmark.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REFMAP_SIZE 16
#define MAX_LINK_LABEL_LENGTH 1000

struct cmark_reference {
	struct cmark_reference *next;
	unsigned char *label;
	unsigned char *url;
	unsigned char *title;
	unsigned int hash;
};

typedef struct cmark_reference cmark_reference;

struct cmark_reference_map {
	cmark_reference *table[REFMAP_SIZE];
};

typedef struct cmark_reference_map cmark_reference_map;

struct cmark_doc_parser {
	struct cmark_reference_map *refmap;
	struct cmark_node* root;
	struct cmark_node* current;
	int line_number;
	cmark_strbuf *curline;
};

unsigned char *cmark_clean_autolink(chunk *url, int is_email);

static inline cmark_node *cmark_make_link(cmark_node *label, unsigned char *url, unsigned char *title)
{
	cmark_node* e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = CMARK_NODE_LINK;
		e->as.link.label = label;
		e->as.link.url   = url;
		e->as.link.title = title;
		e->next = NULL;
	}
	return e;
}

static inline cmark_node* cmark_make_autolink(cmark_node* label, cmark_chunk url, int is_email)
{
	return cmark_make_link(label, cmark_clean_autolink(&url, is_email), NULL);
}

static inline cmark_node* cmark_make_inlines(cmark_node_type t, cmark_node* contents)
{
	cmark_node * e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = t;
		e->first_child = contents;
		e->next = NULL;
	}
	return e;
}

// Create an inline with a literal string value.
static inline cmark_node* cmark_make_literal(cmark_node_type t, cmark_chunk s)
{
	cmark_node * e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = t;
		e->as.literal = s;
		e->next = NULL;
	}
	return e;
}

// Create an inline with no value.
static inline cmark_node* cmark_make_simple(cmark_node_type t)
{
	cmark_node* e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = t;
		e->next = NULL;
	}
	return e;
}

// Macros for creating various kinds of simple.
#define cmark_make_str(s) cmark_make_literal(CMARK_NODE_STRING, s)
#define cmark_make_code(s) cmark_make_literal(CMARK_NODE_INLINE_CODE, s)
#define cmark_make_raw_html(s) cmark_make_literal(CMARK_NODE_INLINE_HTML, s)
#define cmark_make_linebreak() cmark_make_simple(CMARK_NODE_LINEBREAK)
#define cmark_make_softbreak() cmark_make_simple(CMARK_NODE_SOFTBREAK)
#define cmark_make_emph(contents) cmark_make_inlines(CMARK_NODE_EMPH, contents)
#define cmark_make_strong(contents) cmark_make_inlines(CMARK_NODE_STRONG, contents)



#ifndef CMARK_NO_SHORT_NAMES
  #define make_link                 cmark_make_link
  #define make_autolink             cmark_make_autolink
  #define make_str                  cmark_make_str
  #define make_code                 cmark_make_code
  #define make_raw_html             cmark_make_raw_html
  #define make_linebreak            cmark_make_linebreak
  #define make_softbreak            cmark_make_softbreak
  #define make_emph                 cmark_make_emph
  #define make_strong               cmark_make_strong
  #define make_simple               cmark_make_simple
  #define make_literal              cmark_make_literal
  #define make_inlines              cmark_make_inlines
#endif

#ifdef __cplusplus
}
#endif

#endif
