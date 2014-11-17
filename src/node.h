#ifndef CMARK_NODE_H
#define CMARK_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmark.h"
#include "buffer.h"
#include "chunk.h"

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
    
    // Other
    CMARK_NODE_LINK_LABEL
} cmark_node_type;

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
	int               fence_length;
	int               fence_offset;
	unsigned char     fence_char;
	cmark_strbuf      info;
} cmark_fenced_code;

typedef struct {
	int level;
} cmark_header;

typedef struct {
	struct cmark_node *label;
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
		cmark_fenced_code code;
		cmark_header      header;
		cmark_link        link;
	} as;
};

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

#define NODE_DOCUMENT CMARK_NODE_DOCUMENT
#define NODE_BQUOTE CMARK_NODE_BQUOTE
#define NODE_LIST CMARK_NODE_LIST
#define NODE_LIST_ITEM CMARK_NODE_LIST_ITEM
#define NODE_FENCED_CODE CMARK_NODE_FENCED_CODE
#define NODE_INDENTED_CODE CMARK_NODE_INDENTED_CODE
#define NODE_HTML CMARK_NODE_HTML
#define NODE_PARAGRAPH CMARK_NODE_PARAGRAPH
#define NODE_ATX_HEADER CMARK_NODE_ATX_HEADER
#define NODE_SETEXT_HEADER CMARK_NODE_SETEXT_HEADER
#define NODE_HRULE CMARK_NODE_HRULE
#define NODE_REFERENCE_DEF CMARK_NODE_REFERENCE_DEF

#define NODE_STRING CMARK_NODE_STRING
#define NODE_SOFTBREAK CMARK_NODE_SOFTBREAK
#define NODE_LINEBREAK CMARK_NODE_LINEBREAK
#define NODE_INLINE_CODE CMARK_NODE_INLINE_CODE
#define NODE_INLINE_HTML CMARK_NODE_INLINE_HTML
#define NODE_EMPH CMARK_NODE_EMPH
#define NODE_STRONG CMARK_NODE_STRONG
#define NODE_LINK CMARK_NODE_LINK
#define NODE_IMAGE CMARK_NODE_IMAGE

#define NODE_LINK_LABEL CMARK_NODE_LINK_LABEL

#ifdef __cplusplus
}
#endif

#endif

