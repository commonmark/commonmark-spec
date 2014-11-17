#ifndef CMARK_H
#define CMARK_H

#include <stdio.h>
#include "cmark_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMARK_VERSION "0.1"

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

CMARK_EXPORT
void cmark_debug_print(cmark_node *root);

CMARK_EXPORT
unsigned char *cmark_render_html(cmark_node *root);

CMARK_EXPORT
unsigned char *cmark_markdown_to_html(unsigned char *text, int len);

CMARK_EXPORT
void cmark_free_nodes(cmark_node *e);

#ifndef CMARK_NO_SHORT_NAMES
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
