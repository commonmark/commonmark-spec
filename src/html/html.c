#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "cmark.h"
#include "debug.h"
#include "html/houdini.h"

typedef struct RenderStack {
    struct RenderStack *previous;
    chunk literal;
    union {
	node_inl *inl;
	node_block *block;
    } next_sibling;
    bool tight;
} render_stack;

static void free_render_stack(render_stack * stack)
{
    render_stack * tempstack;
    while (stack) {
	tempstack = stack;
	stack = stack->previous;
	chunk_free(&tempstack->literal);
	free(tempstack);
    }
}

static render_stack* push_inline(render_stack* stack,
				 node_inl* inl,
				 char* literal)
{
    render_stack* newstack;
    newstack = (render_stack*)malloc(sizeof(render_stack));
    newstack->previous = stack;
    newstack->next_sibling.inl = inl;
    newstack->literal = chunk_literal(literal);
    return newstack;
}

static render_stack* push_block(render_stack* stack,
				node_block* block,
				char* literal,
				bool tight)
{
    render_stack* newstack;
    newstack = (render_stack*)malloc(sizeof(render_stack));
    newstack->previous = stack;
    newstack->next_sibling.block = block;
    newstack->literal = chunk_literal(literal);
    newstack->tight = tight;
    return newstack;
}

static render_stack* pop_render_stack(render_stack* stack)
{
    render_stack* top = stack;
    if (stack == NULL) {
	return NULL;
    }
    stack = stack->previous;
    return top;
}

// Functions to convert node_block and inline lists to HTML strings.

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
static void inlines_to_html(strbuf *html, node_inl* ils)
{
	strbuf scrap = GH_BUF_INIT;

	while(ils != NULL) {
		switch(ils->tag) {
			case INL_STRING:
				escape_html(html, ils->content.literal.data, ils->content.literal.len);
				break;

			case INL_LINEBREAK:
				strbuf_puts(html, "<br />\n");
				break;

			case INL_SOFTBREAK:
				strbuf_putc(html, '\n');
				break;

			case INL_CODE:
				strbuf_puts(html, "<code>");
				escape_html(html, ils->content.literal.data, ils->content.literal.len);
				strbuf_puts(html, "</code>");
				break;

			case INL_RAW_HTML:
				strbuf_put(html,
						ils->content.literal.data,
						ils->content.literal.len);
				break;

			case INL_LINK:
				strbuf_puts(html, "<a href=\"");
				if (ils->content.linkable.url)
					escape_href(html, ils->content.linkable.url, -1);

				if (ils->content.linkable.title) {
					strbuf_puts(html, "\" title=\"");
					escape_html(html, ils->content.linkable.title, -1);
				}

				strbuf_puts(html, "\">");
				inlines_to_html(html, ils->content.inlines);
				strbuf_puts(html, "</a>");
				break;

			case INL_IMAGE:
				strbuf_puts(html, "<img src=\"");
				if (ils->content.linkable.url)
					escape_href(html, ils->content.linkable.url, -1);

				inlines_to_html(&scrap, ils->content.inlines);
				strbuf_puts(html, "\" alt=\"");
				if (scrap.size)
					escape_html(html, scrap.ptr, scrap.size);
				strbuf_clear(&scrap);

				if (ils->content.linkable.title) {
					strbuf_puts(html, "\" title=\"");
					escape_html(html, ils->content.linkable.title, -1);
				}

				strbuf_puts(html, "\"/>");
				break;

			case INL_STRONG:
				strbuf_puts(html, "<strong>");
				inlines_to_html(html, ils->content.inlines);
				strbuf_puts(html, "</strong>");
				break;

			case INL_EMPH:
				strbuf_puts(html, "<em>");
				inlines_to_html(html, ils->content.inlines);
				strbuf_puts(html, "</em>");
				break;
		}
		ils = ils->next;
	}

	strbuf_free(&scrap);
}

// Convert a node_block list to HTML.  Returns 0 on success, and sets result.
static void blocks_to_html(strbuf *html, node_block *b, bool tight)
{
	struct ListData *data;

	while(b != NULL) {
		switch(b->tag) {
			case BLOCK_DOCUMENT:
				blocks_to_html(html, b->children, false);
				break;

			case BLOCK_PARAGRAPH:
				if (tight) {
					inlines_to_html(html, b->inline_content);
				} else {
					cr(html);
					strbuf_puts(html, "<p>");
					inlines_to_html(html, b->inline_content);
					strbuf_puts(html, "</p>\n");
				}
				break;

			case BLOCK_BQUOTE:
				cr(html);
				strbuf_puts(html, "<blockquote>\n");
				blocks_to_html(html, b->children, false);
				strbuf_puts(html, "</blockquote>\n");
				break;

			case BLOCK_LIST_ITEM:
				cr(html);
				strbuf_puts(html, "<li>");
				blocks_to_html(html, b->children, tight);
				strbuf_trim(html); /* TODO: rtrim */
				strbuf_puts(html, "</li>\n");
				break;

			case BLOCK_LIST:
				// make sure a list starts at the beginning of the line:
				cr(html);
				data = &(b->as.list);

				if (data->start > 1) {
					strbuf_printf(html, "<%s start=\"%d\">\n",
							data->list_type == bullet ? "ul" : "ol",
							data->start);
				} else {
					strbuf_puts(html, data->list_type == bullet ? "<ul>\n" : "<ol>\n");
				}

				blocks_to_html(html, b->children, data->tight);
				strbuf_puts(html, data->list_type == bullet ? "</ul>" : "</ol>");
				strbuf_putc(html, '\n');
				break;

			case BLOCK_ATX_HEADER:
			case BLOCK_SETEXT_HEADER:
				cr(html);
				strbuf_printf(html, "<h%d>", b->as.header.level);
				inlines_to_html(html, b->inline_content);
				strbuf_printf(html, "</h%d>\n", b->as.header.level);
				break;

			case BLOCK_INDENTED_CODE:
			case BLOCK_FENCED_CODE:
				cr(html);

				strbuf_puts(html, "<pre><code");

				if (b->tag == BLOCK_FENCED_CODE) {
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

			case BLOCK_HTML:
				strbuf_put(html, b->string_content.ptr, b->string_content.size);
				break;

			case BLOCK_HRULE:
				strbuf_puts(html, "<hr />\n");
				break;

			case BLOCK_REFERENCE_DEF:
				break;

			default:
				assert(false);
		}

		b = b->next;
	}
}

void cmark_render_html(strbuf *html, node_block *root)
{
	blocks_to_html(html, root, false);
}
