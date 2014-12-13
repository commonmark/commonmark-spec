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

struct render_state {
	strbuf* html;
	cmark_node *plain;
};

static int
S_render_node(cmark_node *node, int entering, void *vstate)
{
	struct render_state *state = vstate;
	cmark_node *parent;
	cmark_node *grandparent;
	strbuf *html = state->html;
	char start_header[] = "<h0>";
	char end_header[] = "</h0>";
	strbuf *info;
	bool tight;

	if (state->plain == node) { // back at original node
		state->plain = NULL;
	}

	if (state->plain != NULL) {
		switch(node->type) {
		case CMARK_NODE_TEXT:
		case CMARK_NODE_INLINE_CODE:
		case CMARK_NODE_INLINE_HTML:
			escape_html(html, node->as.literal.data,
				    node->as.literal.len);
			break;

		case CMARK_NODE_LINEBREAK:
		case CMARK_NODE_SOFTBREAK:
			strbuf_putc(html, ' ');
			break;

		default:
			break;
		}
		return 1;
	}

	switch (node->type) {
	case CMARK_NODE_BLOCK_QUOTE:
		if (entering) {
			cr(html);
			strbuf_puts(html, "<blockquote>\n");
		} else {
			cr(html);
			strbuf_puts(html, "</blockquote>\n");
		}
		break;

	case CMARK_NODE_LIST: {
		cmark_list_type list_type = node->as.list.list_type;
		int start = node->as.list.start;

		if (entering) {
			cr(html);
			if (list_type == CMARK_BULLET_LIST) {
				strbuf_puts(html, "<ul>\n");
			}
			else if (start == 1) {
				strbuf_puts(html, "<ol>\n");
			}
			else {
				strbuf_printf(html, "<ol start=\"%d\">\n",
					      start);
			}
		} else {
			strbuf_puts(html,
				    list_type == CMARK_BULLET_LIST ?
				    "</ul>\n" : "</ol>\n");
		}
		break;
	}

	case CMARK_NODE_LIST_ITEM:
		if (entering) {
			cr(html);
			strbuf_puts(html, "<li>");
		} else {
			strbuf_puts(html, "</li>\n");
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			cr(html);
			start_header[2] = '0' + node->as.header.level;
			strbuf_puts(html, start_header);
		} else {
			end_header[3] = '0' + node->as.header.level;
			strbuf_puts(html, end_header);
			strbuf_putc(html, '\n');
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		info = &node->as.code.info;
		cr(html);

		if (&node->as.code.fence_length == 0
		    || strbuf_len(info) == 0) {
			strbuf_puts(html, "<pre><code>");
		}
		else {
			int first_tag = strbuf_strchr(info, ' ', 0);
			if (first_tag < 0)
				first_tag = strbuf_len(info);

			strbuf_puts(html, "<pre><code class=\"language-");
			escape_html(html, info->ptr, first_tag);
			strbuf_puts(html, "\">");
		}

		escape_html(html, node->string_content.ptr, node->string_content.size);
		strbuf_puts(html, "</code></pre>\n");
		break;

	case CMARK_NODE_HTML:
		cr(html);
		strbuf_put(html, node->string_content.ptr,
			   node->string_content.size);
		break;

	case CMARK_NODE_HRULE:
		cr(html);
		strbuf_puts(html, "<hr />\n");
		break;

	case CMARK_NODE_PARAGRAPH:
		parent = cmark_node_parent(node);
		grandparent = cmark_node_parent(parent);
		if (grandparent != NULL &&
		    grandparent->type == CMARK_NODE_LIST) {
			tight = grandparent->as.list.tight;
		} else {
			tight = false;
		}
		if (!tight) {
			if (entering) {
				cr(html);
				strbuf_puts(html, "<p>");
			} else {
				strbuf_puts(html, "</p>\n");
			}
		}
		break;

	case CMARK_NODE_TEXT:
		escape_html(html, node->as.literal.data,
			    node->as.literal.len);
		break;

	case CMARK_NODE_LINEBREAK:
		strbuf_puts(html, "<br />\n");
		break;

	case CMARK_NODE_SOFTBREAK:
		strbuf_putc(html, '\n');
		break;

	case CMARK_NODE_INLINE_CODE:
		strbuf_puts(html, "<code>");
		escape_html(html, node->as.literal.data, node->as.literal.len);
		strbuf_puts(html, "</code>");
		break;

	case CMARK_NODE_INLINE_HTML:
		strbuf_put(html, node->as.literal.data, node->as.literal.len);
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			strbuf_puts(html, "<strong>");
		} else {
			strbuf_puts(html, "</strong>");
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			strbuf_puts(html, "<em>");
		} else {
			strbuf_puts(html, "</em>");
		}
		break;

	case CMARK_NODE_LINK:
		if (entering) {
			strbuf_puts(html, "<a href=\"");
			if (node->as.link.url)
				escape_href(html, node->as.link.url, -1);

			if (node->as.link.title) {
				strbuf_puts(html, "\" title=\"");
				escape_html(html, node->as.link.title, -1);
			}

			strbuf_puts(html, "\">");
		} else {
			strbuf_puts(html, "</a>");
		}
		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			strbuf_puts(html, "<img src=\"");
			if (node->as.link.url)
				escape_href(html, node->as.link.url, -1);

			strbuf_puts(html, "\" alt=\"");
			state->plain = node;
		} else {
			if (node->as.link.title) {
				strbuf_puts(html, "\" title=\"");
				escape_html(html, node->as.link.title, -1);
			}

			strbuf_puts(html, "\" />");
		}
		break;

	default:
		assert(false);
		break;
	}

	// strbuf_putc(html, 'x');
	return 1;
}

char *cmark_render_html(cmark_node *root)
{
	char *result;
	strbuf html = GH_BUF_INIT;
	struct render_state state = { &html, NULL };
	if (cmark_walk(root, S_render_node, &state)) {
		result = (char *)strbuf_detach(&html);
		strbuf_free(&html);
		return result;
	} else {
		return NULL;
	}
}
