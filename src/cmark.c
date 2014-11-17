#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "references.h"
#include "html/houdini.h"
#include "cmark.h"
#include "buffer.h"
#include "ast.h"

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

