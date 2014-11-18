#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "buffer.h"
#include "debug.h"
#include "html/houdini.h"

typedef struct RenderStack {
	struct RenderStack *previous;
	char* literal;
	union {
		cmark_node *inl;
		cmark_node *block;
	} next_sibling;
	bool tight;
	bool trim;
} render_stack;

static void free_render_stack(render_stack * rstack)
{
	render_stack * tempstack;
	while (rstack) {
		tempstack = rstack;
		rstack = rstack->previous;
		free(tempstack);
	}
}

static render_stack* push_inline(render_stack* rstack,
				 cmark_node* inl,
				 char* literal)
{
	render_stack* newstack;
	newstack = (render_stack*)malloc(sizeof(render_stack));
	if (newstack == NULL) {
		return NULL;
	}
	newstack->previous = rstack;
	newstack->next_sibling.inl = inl;
	newstack->literal = literal;
	newstack->tight = false;
	newstack->trim = false;
	return newstack;
}

static render_stack* push_block(render_stack* rstack,
				cmark_node* block,
				char* literal,
				bool tight,
				bool trim)
{
	render_stack* newstack;
	newstack = (render_stack*)malloc(sizeof(render_stack));
	if (newstack == NULL) {
		return NULL;
	}
	newstack->previous = rstack;
	newstack->next_sibling.block = block;
	newstack->literal = literal;
	newstack->tight = tight;
	newstack->trim = trim;
	return newstack;
}

static render_stack* pop_render_stack(render_stack* rstack)
{
	render_stack* top = rstack;

	if (rstack == NULL) {
		return NULL;
	}
	rstack = rstack->previous;
	top->previous = NULL;
	free_render_stack(top);
	return rstack;
}

// Functions to convert cmark_node and inline lists to HTML strings.

static void escape_html(strbuf *dest, const unsigned char *source, int length)
{
	if (length < 0)
		length = strlen((char *)source);

	houdini_escape_html0(dest, source, (size_t)length, 0);
}

static void escape_href(strbuf *dest, const unsigned char *source, int length)
{
	if (length < 0)
		length = strlen((char *)source);

	houdini_escape_href(dest, source, (size_t)length);
}

static inline void cr(strbuf *html)
{
	if (html->size && html->ptr[html->size - 1] != '\n')
		strbuf_putc(html, '\n');
}

// Convert an inline list to HTML.  Returns 0 on success, and sets result.
static void inlines_to_plain_html(strbuf *html, cmark_node* ils)
{
	cmark_node* children;
	bool visit_children;
	render_stack* rstack = NULL;

	while(ils != NULL) {
		visit_children = false;
		switch(ils->type) {
		case NODE_STRING:
		case NODE_INLINE_CODE:
		case NODE_INLINE_HTML:
			escape_html(html, ils->as.literal.data, ils->as.literal.len);
			break;

		case NODE_LINEBREAK:
		case NODE_SOFTBREAK:
			strbuf_putc(html, '\n');
			break;

		case NODE_LINK:
		case NODE_IMAGE:
			children = ils->as.link.label;
			visit_children = true;
			rstack = push_inline(rstack, ils->next, "");
			break;

		case NODE_STRONG:
		case NODE_EMPH:
			children = ils->first_child;
			visit_children = true;
			rstack = push_inline(rstack, ils->next, "");
			break;
		default:
			break;
		}
		if (visit_children) {
			ils = children;
		} else {
			ils = ils->next;
		}
		while (ils == NULL && rstack != NULL) {
			strbuf_puts(html, rstack->literal);
			ils = rstack->next_sibling.inl;
			rstack = pop_render_stack(rstack);
		}
	}

	free_render_stack(rstack);
}


// Convert an inline list to HTML.  Returns 0 on success, and sets result.
static void inlines_to_html(strbuf *html, cmark_node* ils)
{
	cmark_node* children;
	render_stack* rstack = NULL;

	while(ils != NULL) {
	        children = NULL;
		switch(ils->type) {
		case NODE_STRING:
			escape_html(html, ils->as.literal.data, ils->as.literal.len);
			break;

		case NODE_LINEBREAK:
			strbuf_puts(html, "<br />\n");
			break;

		case NODE_SOFTBREAK:
			strbuf_putc(html, '\n');
			break;

		case NODE_INLINE_CODE:
			strbuf_puts(html, "<code>");
			escape_html(html, ils->as.literal.data, ils->as.literal.len);
			strbuf_puts(html, "</code>");
			break;

		case NODE_INLINE_HTML:
			strbuf_put(html,
				   ils->as.literal.data,
				   ils->as.literal.len);
			break;

		case NODE_LINK:
			strbuf_puts(html, "<a href=\"");
			if (ils->as.link.url)
				escape_href(html, ils->as.link.url, -1);

			if (ils->as.link.title) {
				strbuf_puts(html, "\" title=\"");
				escape_html(html, ils->as.link.title, -1);
			}

			strbuf_puts(html, "\">");
			children = ils->as.link.label;
			rstack = push_inline(rstack, ils->next, "</a>");
			break;

		case NODE_IMAGE:
			strbuf_puts(html, "<img src=\"");
			if (ils->as.link.url)
				escape_href(html, ils->as.link.url, -1);

			strbuf_puts(html, "\" alt=\"");
			inlines_to_plain_html(html, ils->as.link.label);

			if (ils->as.link.title) {
				strbuf_puts(html, "\" title=\"");
				escape_html(html, ils->as.link.title, -1);
			}

			strbuf_puts(html, "\"/>");
			break;

		case NODE_STRONG:
			strbuf_puts(html, "<strong>");
			children = ils->first_child;
			rstack = push_inline(rstack, ils->next, "</strong>");
			break;

		case NODE_EMPH:
			strbuf_puts(html, "<em>");
			children = ils->first_child;
			rstack = push_inline(rstack, ils->next, "</em>");
			break;
		default:
			break;
		}
		if (children) {
			ils = children;
		} else {
			ils = ils->next;
		}
		while (ils == NULL && rstack != NULL) {
			strbuf_puts(html, rstack->literal);
			ils = rstack->next_sibling.inl;
			rstack = pop_render_stack(rstack);
		}
	}

	free_render_stack(rstack);
}

// Convert a cmark_node list to HTML.  Returns 0 on success, and sets result.
static void blocks_to_html(strbuf *html, cmark_node *b)
{
	cmark_list *data;
	render_stack* rstack = NULL;
	bool visit_children = false;
	bool tight = false;

	while(b != NULL) {
		visit_children = false;
		switch(b->type) {
		case NODE_DOCUMENT:
			rstack = push_block(rstack, b->next, "", false, false);
			visit_children = true;
			break;

		case NODE_PARAGRAPH:
			if (tight) {
				inlines_to_html(html, b->first_child);
			} else {
				cr(html);
				strbuf_puts(html, "<p>");
				inlines_to_html(html, b->first_child);
				strbuf_puts(html, "</p>\n");
			}
			break;

		case NODE_BQUOTE:
			cr(html);
			strbuf_puts(html, "<blockquote>\n");
			rstack = push_block(rstack, b->next, "</blockquote>\n", tight, false);
			tight = false;
			visit_children = true;
			break;

		case NODE_LIST_ITEM:
			cr(html);
			strbuf_puts(html, "<li>");
			rstack = push_block(rstack, b->next, "</li>\n", tight, true);
			visit_children = true;
			break;

		case NODE_LIST:
			// make sure a list starts at the beginning of the line:
			cr(html);
			data = &(b->as.list);

			if (data->start > 1) {
				strbuf_printf(html, "<%s start=\"%d\">\n",
					      data->list_type == CMARK_BULLET_LIST ? "ul" : "ol",
					      data->start);
			} else {
				strbuf_puts(html, data->list_type == CMARK_BULLET_LIST ? "<ul>\n" : "<ol>\n");
			}

			rstack = push_block(rstack, b->next,
					    data->list_type == CMARK_BULLET_LIST ?
					    "\n</ul>\n" : "\n</ol>\n", tight, false);
			tight = data->tight;
			visit_children = true;
			break;

		case NODE_ATX_HEADER:
		case NODE_SETEXT_HEADER:
			cr(html);
			strbuf_printf(html, "<h%d>", b->as.header.level);
			inlines_to_html(html, b->first_child);
			strbuf_printf(html, "</h%d>\n", b->as.header.level);
			break;

		case NODE_INDENTED_CODE:
		case NODE_FENCED_CODE:
			cr(html);

			strbuf_puts(html, "<pre><code");

			if (b->type == NODE_FENCED_CODE) {
				strbuf *info = &b->as.code.info;

				if (strbuf_len(info) > 0) {
					int first_tag = strbuf_strchr(info, ' ', 0);
					if (first_tag < 0)
						first_tag = strbuf_len(info);

					strbuf_puts(html, " class=\"language-");
					escape_html(html, info->ptr, first_tag);
					strbuf_putc(html, '"');
				}
			}

			strbuf_putc(html, '>');
			escape_html(html, b->string_content.ptr, b->string_content.size);
			strbuf_puts(html, "</code></pre>\n");
			break;

		case NODE_HTML:
			strbuf_put(html, b->string_content.ptr, b->string_content.size);
			break;

		case NODE_HRULE:
			strbuf_puts(html, "<hr />\n");
			break;

		case NODE_REFERENCE_DEF:
			break;

		default:
			assert(false);
		}
		if (visit_children) {
			b = b->first_child;
		} else {
			b = b->next;
		}
		while (b == NULL && rstack != NULL) {
			strbuf_puts(html, rstack->literal);
			if (rstack->trim) {
				strbuf_rtrim(html);
			}
			tight = rstack->tight;
			b = rstack->next_sibling.block;
			rstack = pop_render_stack(rstack);
		}
	}

	free_render_stack(rstack);
}

unsigned char *cmark_render_html(cmark_node *root)
{
	unsigned char *result;
	strbuf html = GH_BUF_INIT;
	blocks_to_html(&html, root);
	result = strbuf_detach(&html);
	strbuf_free(&html);
	return result;
}
