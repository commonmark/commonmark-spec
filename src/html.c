#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "buffer.h"
#include "houdini.h"

// Functions to convert cmark_nodes to HTML strings.

static bool
finish_node(strbuf *html, cmark_node *node, bool tight);

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

// Convert the inline children of a node to a plain string.
static void inlines_to_plain_html(strbuf *html, cmark_node* node)
{
	cmark_node* cur = node->first_child;

	if (cur == NULL) {
		return;
	}

	while (true) {
		switch(cur->type) {
		case NODE_TEXT:
		case NODE_INLINE_CODE:
		case NODE_INLINE_HTML:
			escape_html(html, cur->as.literal.data, cur->as.literal.len);
			break;

		case NODE_LINEBREAK:
		case NODE_SOFTBREAK:
			strbuf_putc(html, ' ');
			break;

		default:
			break;
		}

		if (cur->first_child) {
			cur = cur->first_child;
			continue;
		}

	next_sibling:
		if (cur->next) {
			cur = cur->next;
			continue;
		}
		cur = cur->parent;
		if (cur == node) {
			break;
		}
		goto next_sibling;
	}
}


// Convert a cmark_node to HTML.
static void node_to_html(strbuf *html, cmark_node *node)
{
	cmark_node *cur;
	char start_header[] = "<h0>";
	bool tight = false;
	bool visit_children;
	strbuf *info;

	if (node == NULL) {
		return;
	}

	cur = node;
	while (true) {
		// Only NODE_IMAGE wants to skip its children.
		visit_children = true;

		switch(cur->type) {
		case NODE_DOCUMENT:
			break;

		case NODE_PARAGRAPH:
			if (!tight) {
				cr(html);
				strbuf_puts(html, "<p>");
			}
			break;

		case NODE_BLOCK_QUOTE:
			cr(html);
			strbuf_puts(html, "<blockquote>\n");
			// BLOCK_QUOTE doesn't use any of the 'as' structs,
			// so the 'list' member can be used to store the
			// current value of 'tight'.
			cur->as.list.tight = tight;
			tight = false;
			break;

		case NODE_LIST_ITEM:
			cr(html);
			strbuf_puts(html, "<li>");
			break;

		case NODE_LIST: {
			cmark_list *list = &cur->as.list;
			bool tmp;

			// make sure a list starts at the beginning of the line:
			cr(html);

			if (list->list_type == CMARK_BULLET_LIST) {
				strbuf_puts(html, "<ul>\n");
			}
			else if (list->start == 1) {
				strbuf_puts(html, "<ol>\n");
			}
			else {
				strbuf_printf(html, "<ol start=\"%d\">\n",
					      list->start);
			}

			// Store the current value of 'tight' by swapping.
			tmp = list->tight;
			list->tight = tight;
			tight = tmp;
			break;
		}

		case NODE_HEADER:
			cr(html);
			start_header[2] = '0' + cur->as.header.level;
			strbuf_puts(html, start_header);
			break;

		case NODE_CODE_BLOCK:
			info = &cur->as.code.info;
			cr(html);

			if (&cur->as.code.fence_length == 0
			    || strbuf_len(info) == 0) {
				strbuf_puts(html, "<pre><code>");
			}
			else {
				int first_tag = strbuf_strchr(info, ' ', 0);
				if (first_tag < 0)
					first_tag = strbuf_len(info);

				strbuf_puts(html,
					    "<pre><code class=\"language-");
				escape_html(html, info->ptr, first_tag);
				strbuf_puts(html, "\">");
			}

			escape_html(html, cur->string_content.ptr, cur->string_content.size);
			break;

		case NODE_HTML:
			cr(html);
			strbuf_put(html, cur->string_content.ptr, cur->string_content.size);
			break;

		case NODE_HRULE:
			cr(html);
			strbuf_puts(html, "<hr />\n");
			break;

		case NODE_REFERENCE_DEF:
			break;

		case NODE_TEXT:
			escape_html(html, cur->as.literal.data, cur->as.literal.len);
			break;

		case NODE_LINEBREAK:
			strbuf_puts(html, "<br />\n");
			break;

		case NODE_SOFTBREAK:
			strbuf_putc(html, '\n');
			break;

		case NODE_INLINE_CODE:
			strbuf_puts(html, "<code>");
			escape_html(html, cur->as.literal.data, cur->as.literal.len);
			break;

		case NODE_INLINE_HTML:
			strbuf_put(html,
				   cur->as.literal.data,
				   cur->as.literal.len);
			break;

		case NODE_LINK:
			strbuf_puts(html, "<a href=\"");
			if (cur->as.link.url)
				escape_href(html, cur->as.link.url, -1);

			if (cur->as.link.title) {
				strbuf_puts(html, "\" title=\"");
				escape_html(html, cur->as.link.title, -1);
			}

			strbuf_puts(html, "\">");
			break;

		case NODE_IMAGE:
			strbuf_puts(html, "<img src=\"");
			if (cur->as.link.url)
				escape_href(html, cur->as.link.url, -1);

			strbuf_puts(html, "\" alt=\"");
			inlines_to_plain_html(html, cur);

			if (cur->as.link.title) {
				strbuf_puts(html, "\" title=\"");
				escape_html(html, cur->as.link.title, -1);
			}

			strbuf_puts(html, "\" />");
			visit_children = false;
			break;

		case NODE_STRONG:
			strbuf_puts(html, "<strong>");
			break;

		case NODE_EMPH:
			strbuf_puts(html, "<em>");
			break;

		default:
			assert(false);
		}

		if (visit_children && cur->first_child) {
			cur = cur->first_child;
			continue;
		}

	next_sibling:
		tight = finish_node(html, cur, tight);
		if (cur == node) {
			break;
		}
		if (cur->next) {
			cur = cur->next;
			continue;
		}
		cur = cur->parent;
		goto next_sibling;
	}
}

// Returns the restored value of 'tight'.
static bool
finish_node(strbuf *html, cmark_node *node, bool tight)
{
	char end_header[] = "</h0>\n";

	switch (node->type) {
	case NODE_PARAGRAPH:
		if (!tight) {
			strbuf_puts(html, "</p>\n");
		}
		break;

	case NODE_BLOCK_QUOTE: {
		cmark_list *list = &node->as.list;
		strbuf_puts(html, "</blockquote>\n");
		// Restore old 'tight' value.
		tight = list->tight;
		list->tight = false;
		break;
	}

	case NODE_LIST_ITEM:
		strbuf_puts(html, "</li>\n");
		break;

	case NODE_LIST: {
		cmark_list *list = &node->as.list;
		bool tmp;
		strbuf_puts(html,
			    list->list_type == CMARK_BULLET_LIST ?
			    "</ul>\n" : "</ol>\n");
		// Restore old 'tight' value.
		tmp = tight;
		tight = list->tight;
		list->tight = tmp;
		break;
	}

	case NODE_HEADER:
		end_header[3] = '0' + node->as.header.level;
		strbuf_puts(html, end_header);
		break;

	case NODE_CODE_BLOCK:
		strbuf_puts(html, "</code></pre>\n");
		break;

	case NODE_INLINE_CODE:
		strbuf_puts(html, "</code>");
		break;

	case NODE_LINK:
		strbuf_puts(html, "</a>");
		break;

	case NODE_STRONG:
		strbuf_puts(html, "</strong>");
		break;

	case NODE_EMPH:
		strbuf_puts(html, "</em>");
		break;

	default:
		break;
	}

	return tight;
}

char *cmark_render_html(cmark_node *root)
{
	char *result;
	strbuf html = GH_BUF_INIT;
	node_to_html(&html, root);
	result = (char *)strbuf_detach(&html);
	strbuf_free(&html);
	return result;
}
