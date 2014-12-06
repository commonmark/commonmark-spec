#ifndef CMARK_H
#define CMARK_H

#include <stdio.h>
#include "cmark_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/** .SH NAME
 *
 * .B cmark
 * \- CommonMark parsing, manipulating, and rendering
 */

/** .SH SIMPLE INTERFACE
 */

/** Current version of library.
 */
#define CMARK_VERSION "0.1"

/** Convert 'text' (assumed to be a UTF-8 encoded string with length
 * 'len' from CommonMark Markdown to HTML, returning a null-terminated,
 * UTF-8-encoded string.
 */
CMARK_EXPORT
char *cmark_markdown_to_html(const char *text, int len);

/** .SH NODE STRUCTURE
 */

/**
 */
typedef enum {
	/* Error status */
	CMARK_NODE_NONE,

	/* Block */
	CMARK_NODE_DOCUMENT,
	CMARK_NODE_BLOCK_QUOTE,
	CMARK_NODE_LIST,
	CMARK_NODE_LIST_ITEM,
	CMARK_NODE_CODE_BLOCK,
	CMARK_NODE_HTML,
	CMARK_NODE_PARAGRAPH,
	CMARK_NODE_HEADER,
	CMARK_NODE_HRULE,
	CMARK_NODE_REFERENCE_DEF,

	CMARK_NODE_FIRST_BLOCK = CMARK_NODE_DOCUMENT,
	CMARK_NODE_LAST_BLOCK  = CMARK_NODE_REFERENCE_DEF,

	/* Inline */
	CMARK_NODE_TEXT,
	CMARK_NODE_SOFTBREAK,
	CMARK_NODE_LINEBREAK,
	CMARK_NODE_INLINE_CODE,
	CMARK_NODE_INLINE_HTML,
	CMARK_NODE_EMPH,
	CMARK_NODE_STRONG,
	CMARK_NODE_LINK,
	CMARK_NODE_IMAGE,

	CMARK_NODE_FIRST_INLINE = CMARK_NODE_TEXT,
	CMARK_NODE_LAST_INLINE  = CMARK_NODE_IMAGE,
} cmark_node_type;


/**
 */
typedef enum {
	CMARK_NO_LIST,
	CMARK_BULLET_LIST,
	CMARK_ORDERED_LIST
}  cmark_list_type;

/**
 */
typedef enum {
	CMARK_PERIOD_DELIM,
	CMARK_PAREN_DELIM
} cmark_delim_type;

typedef struct cmark_node cmark_node;
typedef struct cmark_parser cmark_parser;

/**
 * .SH CREATING AND DESTROYING NODES
 */

/**
 */
CMARK_EXPORT cmark_node*
cmark_node_new(cmark_node_type type);

/**
 */
CMARK_EXPORT void
cmark_node_free(cmark_node *node);

/**
 * .SH TREE TRAVERSAL
 */
CMARK_EXPORT cmark_node*
cmark_node_next(cmark_node *node);

/**
 */
CMARK_EXPORT cmark_node*
cmark_node_previous(cmark_node *node);

/**
 */
CMARK_EXPORT cmark_node*
cmark_node_parent(cmark_node *node);

/**
 */
CMARK_EXPORT cmark_node*
cmark_node_first_child(cmark_node *node);

/**
 */
CMARK_EXPORT cmark_node*
cmark_node_last_child(cmark_node *node);

/**
 * .SH ACCESSORS
 */

/**
 */
CMARK_EXPORT cmark_node_type
cmark_node_get_type(cmark_node *node);

/**
 */
CMARK_EXPORT const char*
cmark_node_get_string_content(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_string_content(cmark_node *node, const char *content);

/**
 */
CMARK_EXPORT int
cmark_node_get_header_level(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_header_level(cmark_node *node, int level);

/**
 */
CMARK_EXPORT cmark_list_type
cmark_node_get_list_type(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_list_type(cmark_node *node, cmark_list_type type);

/**
 */
CMARK_EXPORT int
cmark_node_get_list_start(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_list_start(cmark_node *node, int start);

/**
 */
CMARK_EXPORT int
cmark_node_get_list_tight(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_list_tight(cmark_node *node, int tight);

/**
 */
CMARK_EXPORT const char*
cmark_node_get_fence_info(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_fence_info(cmark_node *node, const char *info);

/**
 */
CMARK_EXPORT const char*
cmark_node_get_url(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_url(cmark_node *node, const char *url);

/**
 */
CMARK_EXPORT const char*
cmark_node_get_title(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_set_title(cmark_node *node, const char *title);

/**
 */
CMARK_EXPORT int
cmark_node_get_start_line(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_get_start_column(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_get_end_line(cmark_node *node);

/**
 * .SH TREE MANIPULATION
 */

/**
 */
CMARK_EXPORT void
cmark_node_unlink(cmark_node *node);

/**
 */
CMARK_EXPORT int
cmark_node_insert_before(cmark_node *node, cmark_node *sibling);

/**
 */
CMARK_EXPORT int
cmark_node_insert_after(cmark_node *node, cmark_node *sibling);

/**
 */
CMARK_EXPORT int
cmark_node_prepend_child(cmark_node *node, cmark_node *child);

/**
 */
CMARK_EXPORT int
cmark_node_append_child(cmark_node *node, cmark_node *child);

/**
 * .SH PARSING
 */

/**
 */
CMARK_EXPORT
cmark_parser *cmark_parser_new();

/**
 */
CMARK_EXPORT
void cmark_parser_free(cmark_parser *parser);

/**
 */
CMARK_EXPORT
cmark_node *cmark_parser_finish(cmark_parser *parser);

/**
 */
CMARK_EXPORT
void cmark_parser_feed(cmark_parser *parser, const char *buffer, size_t len);

/**
 */
CMARK_EXPORT
cmark_node *cmark_parse_document(const char *buffer, size_t len);

/**
 */
CMARK_EXPORT
cmark_node *cmark_parse_file(FILE *f);

/**
 * .SH RENDERING
 */

/**
 */
CMARK_EXPORT
char *cmark_render_ast(cmark_node *root);

/**
 */
CMARK_EXPORT
char *cmark_render_html(cmark_node *root);

/** .SH AUTHORS
 *
 * John MacFarlane, Vicent Marti,  Kārlis Gaņģis, Nick Wellnhofer.
 */

#ifndef CMARK_NO_SHORT_NAMES
  #define NODE_DOCUMENT             CMARK_NODE_DOCUMENT
  #define NODE_BLOCK_QUOTE          CMARK_NODE_BLOCK_QUOTE
  #define NODE_LIST                 CMARK_NODE_LIST
  #define NODE_LIST_ITEM            CMARK_NODE_LIST_ITEM
  #define NODE_CODE_BLOCK           CMARK_NODE_CODE_BLOCK
  #define NODE_HTML                 CMARK_NODE_HTML
  #define NODE_PARAGRAPH            CMARK_NODE_PARAGRAPH
  #define NODE_HEADER		    CMARK_NODE_HEADER
  #define NODE_HRULE                CMARK_NODE_HRULE
  #define NODE_REFERENCE_DEF        CMARK_NODE_REFERENCE_DEF
  #define NODE_TEXT                 CMARK_NODE_TEXT
  #define NODE_SOFTBREAK            CMARK_NODE_SOFTBREAK
  #define NODE_LINEBREAK            CMARK_NODE_LINEBREAK
  #define NODE_INLINE_CODE          CMARK_NODE_INLINE_CODE
  #define NODE_INLINE_HTML          CMARK_NODE_INLINE_HTML
  #define NODE_EMPH                 CMARK_NODE_EMPH
  #define NODE_STRONG               CMARK_NODE_STRONG
  #define NODE_LINK                 CMARK_NODE_LINK
  #define NODE_IMAGE                CMARK_NODE_IMAGE
  #define NODE_LINK_LABEL           CMARK_NODE_LINK_LABEL
  #define BULLET_LIST               CMARK_BULLET_LIST
  #define ORDERED_LIST              CMARK_ORDERED_LIST
  #define PERIOD_DELIM              CMARK_PERIOD_DELIM
  #define PAREN_DELIM               CMARK_PAREN_DELIM
#endif

#ifdef __cplusplus
}
#endif

#endif
