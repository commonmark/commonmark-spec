#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "ast.h"
#include "inlines.h"
#include "references.h"

// Free a node_block list and any children.
void cmark_free_blocks(node_block *e)
{
	node_block * next;
	while (e != NULL) {
		free_inlines(e->inline_content);
		strbuf_free(&e->string_content);
		if (e->tag == BLOCK_FENCED_CODE) {
			strbuf_free(&e->as.code.info);
		} else if (e->tag == BLOCK_DOCUMENT) {
			reference_map_free(e->as.document.refmap);
		}
		if (e->last_child) {
			// Splice children into list
			e->last_child->next = e->next;
			e->next = e->children;
		}
		next = e->next;
		free(e);
		e = next;
	}
}

