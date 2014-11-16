#ifndef CMARK_H
#define CMARK_H

#include <stdio.h>
#include "cmark_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMARK_VERSION "0.1"
#define CMARK_CODE_INDENT 4
#define CMARK_MAX_LINK_LABEL_LENGTH 1000

typedef struct cmark_node_inl cmark_node_inl;
typedef struct cmark_node_block cmark_node_block;
typedef struct cmark_doc_parser cmark_doc_parser;

CMARK_EXPORT
cmark_doc_parser *cmark_new_doc_parser();

CMARK_EXPORT
void cmark_free_doc_parser(cmark_doc_parser *parser);

CMARK_EXPORT
cmark_node_block *cmark_finish(cmark_doc_parser *parser);

CMARK_EXPORT
void cmark_process_line(cmark_doc_parser *parser, const unsigned char *buffer, size_t bytes);

CMARK_EXPORT
cmark_node_block *cmark_finish(cmark_doc_parser *parser);

CMARK_EXPORT
cmark_node_block *cmark_parse_document(const unsigned char *buffer, size_t len);

CMARK_EXPORT
cmark_node_block *cmark_parse_file(FILE *f);

CMARK_EXPORT
void cmark_debug_print(cmark_node_block *root);

CMARK_EXPORT
unsigned char *cmark_render_html(cmark_node_block *root);

CMARK_EXPORT
unsigned char *cmark_markdown_to_html(unsigned char *text, int len);

CMARK_EXPORT
void cmark_free_blocks(cmark_node_block *e);

CMARK_EXPORT
void cmark_free_inlines(cmark_node_inl* e);

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
  #define doc_parser                cmark_doc_parser
  #define new_doc_parser            cmark_new_doc_parser
  #define free_doc_parser           cmark_free_doc_parser
  #define process_line              cmark_process_line
  #define finish                    cmark_finish
#endif

#ifdef __cplusplus
}
#endif

#endif
