#ifndef CMARK_H
#define CMARK_H

#include <stdio.h>
#include "cmark_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMARK_VERSION "0.1"

typedef enum {
    // Block
    CMARK_NODE_DOCUMENT,
    CMARK_NODE_BQUOTE,
    CMARK_NODE_LIST,
    CMARK_NODE_LIST_ITEM,
    CMARK_NODE_FENCED_CODE,
    CMARK_NODE_INDENTED_CODE,
    CMARK_NODE_HTML,
    CMARK_NODE_PARAGRAPH,
    CMARK_NODE_ATX_HEADER,
    CMARK_NODE_SETEXT_HEADER,
    CMARK_NODE_HRULE,
    CMARK_NODE_REFERENCE_DEF,

    CMARK_NODE_FIRST_BLOCK = CMARK_NODE_DOCUMENT,
    CMARK_NODE_LAST_BLOCK  = CMARK_NODE_REFERENCE_DEF,

    // Inline
    CMARK_NODE_STRING,
    CMARK_NODE_SOFTBREAK,
    CMARK_NODE_LINEBREAK,
    CMARK_NODE_INLINE_CODE,
    CMARK_NODE_INLINE_HTML,
    CMARK_NODE_EMPH,
    CMARK_NODE_STRONG,
    CMARK_NODE_LINK,
    CMARK_NODE_IMAGE,

    CMARK_NODE_FIRST_INLINE = CMARK_NODE_STRING,
    CMARK_NODE_LAST_INLINE  = CMARK_NODE_IMAGE,
} cmark_node_type;

typedef enum {
	CMARK_BULLET_LIST,
	CMARK_ORDERED_LIST
}  cmark_list_type;

typedef enum {
	CMARK_PERIOD_DELIM,
	CMARK_PAREN_DELIM
} cmark_delim_type;

typedef struct cmark_node cmark_node;
typedef struct cmark_doc_parser cmark_doc_parser;

CMARK_EXPORT cmark_node_type
cmark_node_get_type(cmark_node *node);

// Tree traversal

CMARK_EXPORT cmark_node*
cmark_node_next(cmark_node *node);

CMARK_EXPORT cmark_node*
cmark_node_previous(cmark_node *node);

CMARK_EXPORT cmark_node*
cmark_node_parent(cmark_node *node);

CMARK_EXPORT cmark_node*
cmark_node_first_child(cmark_node *node);

CMARK_EXPORT cmark_node*
cmark_node_last_child(cmark_node *node);

// Tree manipulation

CMARK_EXPORT void
cmark_node_unlink(cmark_node *node);

CMARK_EXPORT int
cmark_node_insert_before(cmark_node *node, cmark_node *sibling);

CMARK_EXPORT int
cmark_node_insert_before(cmark_node *node, cmark_node *sibling);

CMARK_EXPORT int
cmark_node_prepend_child(cmark_node *node, cmark_node *child);

CMARK_EXPORT int
cmark_node_append_child(cmark_node *node, cmark_node *child);

// Parser

CMARK_EXPORT
cmark_doc_parser *cmark_new_doc_parser();

CMARK_EXPORT
void cmark_free_doc_parser(cmark_doc_parser *parser);

CMARK_EXPORT
cmark_node *cmark_finish(cmark_doc_parser *parser);

CMARK_EXPORT
void cmark_process_line(cmark_doc_parser *parser, const unsigned char *buffer, size_t bytes);

CMARK_EXPORT
cmark_node *cmark_parse_document(const unsigned char *buffer, size_t len);

CMARK_EXPORT
cmark_node *cmark_parse_file(FILE *f);

// Renderer

CMARK_EXPORT
void cmark_debug_print(cmark_node *root);

CMARK_EXPORT
unsigned char *cmark_render_html(cmark_node *root);

CMARK_EXPORT
unsigned char *cmark_markdown_to_html(unsigned char *text, int len);

CMARK_EXPORT
void cmark_free_nodes(cmark_node *e);

#ifndef CMARK_NO_SHORT_NAMES
  #define NODE_DOCUMENT             CMARK_NODE_DOCUMENT
  #define NODE_BQUOTE               CMARK_NODE_BQUOTE
  #define NODE_LIST                 CMARK_NODE_LIST
  #define NODE_LIST_ITEM            CMARK_NODE_LIST_ITEM
  #define NODE_FENCED_CODE          CMARK_NODE_FENCED_CODE
  #define NODE_INDENTED_CODE        CMARK_NODE_INDENTED_CODE
  #define NODE_HTML                 CMARK_NODE_HTML
  #define NODE_PARAGRAPH            CMARK_NODE_PARAGRAPH
  #define NODE_ATX_HEADER           CMARK_NODE_ATX_HEADER
  #define NODE_SETEXT_HEADER        CMARK_NODE_SETEXT_HEADER
  #define NODE_HRULE                CMARK_NODE_HRULE
  #define NODE_REFERENCE_DEF        CMARK_NODE_REFERENCE_DEF
  #define NODE_STRING               CMARK_NODE_STRING
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
  #define free_simple               cmark_free_simple
  #define free_blocks               cmark_free_blocks
  #define doc_parser                cmark_doc_parser
  #define new_doc_parser            cmark_new_doc_parser
  #define free_doc_parser           cmark_free_doc_parser
  #define process_line              cmark_process_line
  #define finish                    cmark_finish
  #define block_next                cmark_block_next
  #define block_previous            cmark_block_previous
  #define block_parent              cmark_block_parent
  #define block_children            cmark_block_children
  #define block_delete              cmark_block_delete
  #define block_insert_before       cmark_block_insert_before
  #define block_insert_after        cmark_block_insert_after
#endif

#ifdef __cplusplus
}
#endif

#endif
