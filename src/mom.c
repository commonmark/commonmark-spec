#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "buffer.h"

// Functions to convert cmark_nodes to groff mom strings.

static void escape_mom(cmark_strbuf *dest, const unsigned char *source, int length)
{
	int i;
	unsigned char c;
	bool beginLine = true;

	for (i = 0; i < length; i++) {
		c = source[i];
		if (c == '.' && beginLine) {
			cmark_strbuf_puts(dest, "\\&.");
		} else if (c == '\'' && beginLine) {
			cmark_strbuf_puts(dest, "\\&'");
		} else if (c == '-') {
			cmark_strbuf_puts(dest, "\\-");
		} else if (c == '\\') {
			cmark_strbuf_puts(dest, "\\e");
		} else {
			cmark_strbuf_putc(dest, source[i]);
		}
		beginLine = (c == '\n');
	}
}

static inline void cr(cmark_strbuf *mom)
{
	if (mom->size && mom->ptr[mom->size - 1] != '\n')
		cmark_strbuf_putc(mom, '\n');
}

struct render_state {
	cmark_strbuf* mom;
	cmark_node *plain;
};

static int
S_render_node(cmark_node *node, cmark_event_type ev_type,
              struct render_state *state)
{
	cmark_strbuf *mom = state->mom;
	int list_start;
	int list_delim;
	bool entering = (ev_type == CMARK_EVENT_ENTER);

	if (state->plain == node) { // back at original node
		state->plain = NULL;
	}

	if (state->plain != NULL) {
		switch(node->type) {
		case CMARK_NODE_TEXT:
		case CMARK_NODE_CODE:
			escape_mom(mom, node->as.literal.data,
			           node->as.literal.len);
			break;

		case CMARK_NODE_LINEBREAK:
		case CMARK_NODE_SOFTBREAK:
			cmark_strbuf_putc(mom, ' ');
			break;

		default:
			break;
		}
		return 1;
	}

	switch (node->type) {
	case CMARK_NODE_DOCUMENT:
		break;

	case CMARK_NODE_BLOCK_QUOTE:
		if (entering) {
			cr(mom);
			cmark_strbuf_puts(mom, ".BLOCKQUOTE");
			cr(mom);
		} else {
			cr(mom);
			cmark_strbuf_puts(mom, ".BLOCKQUOTE OFF");
			cr(mom);
		}
		break;

	case CMARK_NODE_LIST:
		cr(mom);
		if (entering) {
			if (cmark_node_get_list_type(node) ==
			    CMARK_BULLET_LIST) {
				cmark_strbuf_puts(mom, ".LIST BULLET");
			} else {
				list_start = cmark_node_get_list_start(node->parent);
				list_delim = cmark_node_get_list_delim(node->parent);
				cmark_strbuf_printf(mom, ".LIST DIGIT %s",
						    list_delim == CMARK_PAREN_DELIM ?
						    ")" : ".");
				if (list_start != 1) {
					cr(mom);
					cmark_strbuf_printf(mom, ".RESET_LIST %d", list_start);
				}
			}
		} else {
			cmark_strbuf_puts(mom, ".LIST OFF");
		}
		cr(mom);
		break;

	case CMARK_NODE_ITEM:
		if (entering) {
			cr(mom);
			cmark_strbuf_puts(mom, ".ITEM");
			cr(mom);
		} else {
			cr(mom);
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			cr(mom);
			cmark_strbuf_printf(mom,
					    ".HEADING %d \"",
					    cmark_node_get_header_level(node));
		} else {
			cmark_strbuf_printf(mom, "\"");
			cr(mom);
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		cr(mom);
		cmark_strbuf_puts(mom, ".QUOTE\n.CODE\n");
		escape_mom(mom, node->as.code.literal.data,
		           node->as.code.literal.len);
		cr(mom);
		cmark_strbuf_puts(mom, ".CODE OFF\n.QUOTE OFF");
		cr(mom);
		break;

	case CMARK_NODE_HTML:
		break;

	case CMARK_NODE_HRULE:
		cr(mom);
		cmark_strbuf_puts(mom, ".DRH");
		cr(mom);
		break;

	case CMARK_NODE_PARAGRAPH:
		if (entering) {
			// no blank line if first paragraph in list:
			if (node->parent &&
			    node->parent->type == CMARK_NODE_ITEM &&
			    node->prev == NULL) {
				// no blank line or .PP
			} else {
				cr(mom);
				cmark_strbuf_puts(mom, ".PP");
				cr(mom);
			}
		} else {
			cr(mom);
		}
		break;

	case CMARK_NODE_TEXT:
		escape_mom(mom, node->as.literal.data,
		           node->as.literal.len);
		break;

	case CMARK_NODE_LINEBREAK:
		cmark_strbuf_puts(mom, ".LINEBREAK");
		cr(mom);
		break;

	case CMARK_NODE_SOFTBREAK:
		cmark_strbuf_putc(mom, '\n');
		break;

	case CMARK_NODE_CODE:
		cmark_strbuf_puts(mom, "\\f[CR]");
		escape_mom(mom, node->as.literal.data, node->as.literal.len);
		cmark_strbuf_puts(mom, "\\f[]");
		break;

	case CMARK_NODE_INLINE_HTML:
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			cmark_strbuf_puts(mom, "\\f[B]");
		} else {
			cmark_strbuf_puts(mom, "\\f[]");
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			cmark_strbuf_puts(mom, "\\f[I]");
		} else {
			cmark_strbuf_puts(mom, "\\f[]");
		}
		break;

	case CMARK_NODE_LINK:
		if (!entering) {
			cmark_strbuf_printf(mom, " (%s)",
			                    cmark_node_get_url(node));
		}
		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			cmark_strbuf_puts(mom, "[IMAGE: ");
			state->plain = node;
		} else {
			cmark_strbuf_puts(mom, "]");
		}
		break;

	default:
		assert(false);
		break;
	}

	// cmark_strbuf_putc(mom, 'x');
	return 1;
}

char *cmark_render_mom(cmark_node *root, long options)
{
	char *result;
	cmark_strbuf mom = GH_BUF_INIT;
	struct render_state state = { &mom, NULL };
	cmark_node *cur;
	cmark_event_type ev_type;
	cmark_iter *iter = cmark_iter_new(root);

	if (options == 0) options = 0; // avoid warning about unused parameters

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cur = cmark_iter_get_node(iter);
		S_render_node(cur, ev_type, &state);
	}
	result = (char *)cmark_strbuf_detach(&mom);

	cmark_iter_free(iter);
	cmark_strbuf_free(&mom);
	return result;
}
