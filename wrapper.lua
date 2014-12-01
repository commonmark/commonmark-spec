#!/usr/bin/env luajit

local ffi = require("ffi")

cmark = ffi.load("libcmark")

ffi.cdef[[

char *cmark_markdown_to_html(const char *text, int len);
typedef enum {
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

typedef enum {
	CMARK_NO_LIST,
	CMARK_BULLET_LIST,
	CMARK_ORDERED_LIST
}  cmark_list_type;

typedef enum {
	CMARK_PERIOD_DELIM,
	CMARK_PAREN_DELIM
} cmark_delim_type;

typedef struct cmark_node cmark_node;
typedef struct cmark_parser cmark_parser;

cmark_node* cmark_node_new(cmark_node_type type);

void
cmark_node_free(cmark_node *node);

cmark_node* cmark_node_next(cmark_node *node);

cmark_node* cmark_node_previous(cmark_node *node);

cmark_node* cmark_node_parent(cmark_node *node);

cmark_node* cmark_node_first_child(cmark_node *node);

cmark_node* cmark_node_last_child(cmark_node *node);

cmark_node_type cmark_node_get_type(cmark_node *node);

const char* cmark_node_get_string_content(cmark_node *node);

int cmark_node_set_string_content(cmark_node *node, const char *content);

int cmark_node_get_header_level(cmark_node *node);

int cmark_node_set_header_level(cmark_node *node, int level);

cmark_list_type cmark_node_get_list_type(cmark_node *node);

int cmark_node_set_list_type(cmark_node *node, cmark_list_type type);

int cmark_node_get_list_start(cmark_node *node);

int cmark_node_set_list_start(cmark_node *node, int start);

int cmark_node_get_list_tight(cmark_node *node);

int cmark_node_set_list_tight(cmark_node *node, int tight);

const char* cmark_node_get_fence_info(cmark_node *node);

int cmark_node_set_fence_info(cmark_node *node, const char *info);

const char* cmark_node_get_url(cmark_node *node);

int cmark_node_set_url(cmark_node *node, const char *url);

const char* cmark_node_get_title(cmark_node *node);

int cmark_node_set_title(cmark_node *node, const char *title);

int cmark_node_get_start_line(cmark_node *node);

int cmark_node_get_start_column(cmark_node *node);

int cmark_node_get_end_line(cmark_node *node);

void cmark_node_unlink(cmark_node *node);

int cmark_node_insert_before(cmark_node *node, cmark_node *sibling);

int cmark_node_insert_after(cmark_node *node, cmark_node *sibling);

int cmark_node_prepend_child(cmark_node *node, cmark_node *child);

int cmark_node_append_child(cmark_node *node, cmark_node *child);

cmark_parser *cmark_parser_new();

void cmark_parser_free(cmark_parser *parser);

cmark_node *cmark_parser_finish(cmark_parser *parser);

void cmark_parser_feed(cmark_parser *parser, const char *buffer, size_t len);

cmark_node *cmark_parse_document(const char *buffer, size_t len);

char *cmark_render_ast(cmark_node *root);

char *cmark_render_html(cmark_node *root);

]]

local doc = cmark.cmark_parse_document("hi *there*", 10)
local d2 = cmark.cmark_node_first_child(doc)
local d3 = cmark.cmark_node_first_child(d2)
local d4 = cmark.cmark_node_next(d3)
cmark.cmark_node_unlink(d4)
local html = ffi.string(cmark.cmark_render_html(doc))
print(html)

