#ifndef _CMARK_H_
#define _CMARK_H_

#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMARK_VERSION "0.1"
#define CMARK_CODE_INDENT 4

#define CMARK_MAX_LINK_LABEL_LENGTH 1000

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
#endif

#ifdef __cplusplus
}
#endif

#endif
