#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "references.h"
#include "html/houdini.h"
#include "cmark.h"
#include "buffer.h"
#include "ast.h"

unsigned char *cmark_markdown_to_html(unsigned char *text, int len)
{
	node_block *blocks;
	unsigned char *result;

	blocks = cmark_parse_document(text, len);

	result = cmark_render_html(blocks);
	cmark_free_blocks(blocks);

	return result;
}

// Utility function used by free_inlines
static void splice_into_list(cmark_node_inl* e, node_inl* children) {
	cmark_node_inl * tmp;
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
void cmark_free_inlines(cmark_node_inl* e)
{
	node_inl * next;

	while (e != NULL) {
		switch (e->tag){
		case CMARK_INL_STRING:
		case CMARK_INL_RAW_HTML:
		case CMARK_INL_CODE:
			cmark_chunk_free(&e->content.literal);
			break;
		case CMARK_INL_LINEBREAK:
		case CMARK_INL_SOFTBREAK:
			break;
		case CMARK_INL_LINK:
		case CMARK_INL_IMAGE:
			free(e->content.linkable.url);
			free(e->content.linkable.title);
			splice_into_list(e, e->content.linkable.label);
			break;
		case CMARK_INL_EMPH:
		case CMARK_INL_STRONG:
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

// Free a node_block list and any children.
void cmark_free_blocks(cmark_node_block *e)
{
	cmark_node_block * next;
	while (e != NULL) {
		cmark_free_inlines(e->inline_content);
		strbuf_free(&e->string_content);
		if (e->tag == CMARK_BLOCK_FENCED_CODE) {
			strbuf_free(&e->as.code.info);
		} else if (e->tag == CMARK_BLOCK_DOCUMENT) {
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

