#ifndef CMARK_AST_H
#define CMARK_AST_H

#include <stdio.h>
#include "node.h"
#include "references.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINK_LABEL_LENGTH 1000

struct cmark_doc_parser {
	struct cmark_reference_map *refmap;
	struct cmark_node* root;
	struct cmark_node* current;
	int line_number;
	cmark_strbuf *curline;
};

#ifdef __cplusplus
}
#endif

#endif
