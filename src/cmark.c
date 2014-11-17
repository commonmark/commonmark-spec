#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "references.h"
#include "html/houdini.h"
#include "cmark.h"
#include "buffer.h"
#include "ast.h"

// AST traversal and manipulation functions

cmark_node_block *cmark_block_next(cmark_node_block *current)
{
	return current->next;
}

cmark_node_block *cmark_block_previous(cmark_node_block *current)
{
	return current->prev;
}

cmark_node_block *cmark_block_parent(cmark_node_block *current)
{
	return current->parent;
}

cmark_node_block *cmark_block_children(cmark_node_block *current)
{
	return current->children;
}

void cmark_block_insert_before(cmark_node_block *new, cmark_node_block *current)
{
	// Find last node in new:
	cmark_node_block *new_last = new;
	while (new_last->next) {
		new_last = new_last->next;
	}
	new_last->next = current;
	current->prev = new_last;
	if (current->prev) {
		current->prev->next = new;
		new->prev = current->prev;
	}
}

void cmark_block_insert_after(cmark_node_block *current, cmark_node_block *new)
{
	// Find last node in new:
	cmark_node_block *new_last = new;
	while (new_last->next) {
		new_last = new_last->next;
	}
	if (current->next) {
		new_last->next = current->next;
		current->next->prev = new_last;
	}
	current->next = new;
	new->prev = current;
}

/* * */

unsigned char *cmark_markdown_to_html(unsigned char *text, int len)
{
	cmark_node *blocks;
	unsigned char *result;

	blocks = cmark_parse_document(text, len);

	result = cmark_render_html(blocks);
	cmark_free_nodes(blocks);

	return result;
}

// Utility function used by cmark_free_nodes
static void splice_into_list(cmark_node* e, cmark_node* children) {
	cmark_node * tmp;
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

unsigned char *cmark_clean_autolink(chunk *url, int is_email)
{
	strbuf buf = GH_BUF_INIT;

	chunk_trim(url);

	if (url->len == 0)
		return NULL;

	if (is_email)
		strbuf_puts(&buf, "mailto:");

	houdini_unescape_html_f(&buf, url->data, url->len);
	return strbuf_detach(&buf);
}

// Free a cmark_node list and any children.
void cmark_free_nodes(cmark_node *e)
{
	cmark_node *next;
	while (e != NULL) {
		strbuf_free(&e->string_content);
		switch (e->type){
		case NODE_FENCED_CODE:
			strbuf_free(&e->as.code.info);
			break;
		case NODE_STRING:
		case NODE_INLINE_HTML:
		case NODE_INLINE_CODE:
			cmark_chunk_free(&e->as.literal);
			break;
		case NODE_LINK:
		case NODE_IMAGE:
			free(e->as.link.url);
			free(e->as.link.title);
			splice_into_list(e, e->as.link.label);
			break;
		default:
			break;
		}
		if (e->last_child) {
			// Splice children into list
			e->last_child->next = e->next;
			e->next = e->first_child;
		}
		next = e->next;
		free(e);
		e = next;
	}
}

