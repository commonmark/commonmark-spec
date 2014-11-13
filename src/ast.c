#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "ast.h"
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

// Utility function used by free_inlines
static void splice_into_list(node_inl* e, node_inl* children) {
	node_inl * tmp;
	if (children) {
		tmp = children;
		// Find last child
		while (tmp->next) {
			tmp = tmp->next;
		}
		// Splice children into list
		tmp->next = e->next;
		e->next = children;
	}
	return ;
}

// Free an inline list.  Avoid recursion to prevent stack overflows
// on deeply nested structures.
extern void free_inlines(node_inl* e)
{
	node_inl * next;

	while (e != NULL) {
		switch (e->tag){
		case INL_STRING:
		case INL_RAW_HTML:
		case INL_CODE:
			chunk_free(&e->content.literal);
			break;
		case INL_LINEBREAK:
		case INL_SOFTBREAK:
			break;
		case INL_LINK:
		case INL_IMAGE:
			free(e->content.linkable.url);
			free(e->content.linkable.title);
			splice_into_list(e, e->content.linkable.label);
			break;
		case INL_EMPH:
		case INL_STRONG:
		        splice_into_list(e, e->content.inlines);
			break;
		default:
		        fprintf(stderr, "[WARN] (%s:%d) Unknown inline tag %d",
				__FILE__, __LINE__, e->tag);
			break;
		}
		next = e->next;
		free(e);
		e = next;
	}
}
