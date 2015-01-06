#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "buffer.h"

// Functions to convert cmark_nodes to groff man strings.

static void escape_man(cmark_strbuf *dest, const unsigned char *source, int length)
{
	int i;
	unsigned char c;

	for (i = 0; i < length; i++) {
		c = source[i];
		if (c == '.' && i == 0) {
			cmark_strbuf_puts(dest, "\\&.");
		} else if (c == '\'' && i == 0) {
			cmark_strbuf_puts(dest, "\\&'");
		} else if (c == '-') {
			cmark_strbuf_puts(dest, "\\-");
		} else if (c == '\\') {
			cmark_strbuf_puts(dest, "\\e");
		} else {
			cmark_strbuf_putc(dest, source[i]);
		}
	}
}

static inline void cr(cmark_strbuf *man)
{
	if (man->size && man->ptr[man->size - 1] != '\n')
		cmark_strbuf_putc(man, '\n');
}

struct render_state {
	cmark_strbuf* man;
	cmark_node *plain;
};

static int
S_render_node(cmark_node *node, cmark_event_type ev_type,
              struct render_state *state)
{
	cmark_node *tmp;
	cmark_strbuf *man = state->man;
	int list_number;
	bool entering = (ev_type == CMARK_EVENT_ENTER);

	if (state->plain == node) { // back at original node
		state->plain = NULL;
	}

	if (state->plain != NULL) {
		switch(node->type) {
		case CMARK_NODE_TEXT:
		case CMARK_NODE_CODE:
			escape_man(man, node->as.literal.data,
			           node->as.literal.len);
			break;

		case CMARK_NODE_LINEBREAK:
		case CMARK_NODE_SOFTBREAK:
			cmark_strbuf_putc(man, ' ');
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
			cr(man);
			cmark_strbuf_puts(man, ".RS");
			cr(man);
		} else {
			cr(man);
			cmark_strbuf_puts(man, ".RE");
			cr(man);
		}
		break;

	case CMARK_NODE_LIST:
		break;

	case CMARK_NODE_ITEM:
		if (entering) {
			cr(man);
			cmark_strbuf_puts(man, ".IP ");
			if (cmark_node_get_list_type(node->parent) ==
			    CMARK_BULLET_LIST) {
				cmark_strbuf_puts(man, "\\[bu] 2");
			} else {
				list_number = cmark_node_get_list_start(node->parent);
				tmp = node;
				while (tmp->prev) {
					tmp = tmp->prev;
					list_number += 1;
				}
				cmark_strbuf_printf(man, "\"%d.\" 4", list_number);
			}
			cr(man);
		} else {
			cr(man);
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			cr(man);
			cmark_strbuf_puts(man,
			                  cmark_node_get_header_level(node) == 1 ?
			                  ".SH" : ".SS");
			cr(man);
		} else {
			cr(man);
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		cr(man);
		cmark_strbuf_puts(man, ".IP\n.nf\n\\f[C]\n");
		escape_man(man, node->as.code.literal.data,
		           node->as.code.literal.len);
		cr(man);
		cmark_strbuf_puts(man, "\\f[]\n.fi");
		cr(man);
		break;

	case CMARK_NODE_HTML:
		break;

	case CMARK_NODE_HRULE:
		cr(man);
		cmark_strbuf_puts(man, ".PP\n  *  *  *  *  *");
		cr(man);
		break;

	case CMARK_NODE_PARAGRAPH:
		if (entering) {
			// no blank line if first paragraph in list:
			if (node->parent &&
			    node->parent->type == CMARK_NODE_ITEM &&
			    node->prev == NULL) {
				// no blank line or .PP
			} else {
				cr(man);
				cmark_strbuf_puts(man, ".PP\n");
			}
		} else {
			cr(man);
		}
		break;

	case CMARK_NODE_TEXT:
		escape_man(man, node->as.literal.data,
		           node->as.literal.len);
		break;

	case CMARK_NODE_LINEBREAK:
		cmark_strbuf_puts(man, ".PD 0\n.P\n.PD");
		cr(man);
		break;

	case CMARK_NODE_SOFTBREAK:
		cmark_strbuf_putc(man, '\n');
		break;

	case CMARK_NODE_CODE:
		cmark_strbuf_puts(man, "\\f[C]");
		escape_man(man, node->as.literal.data, node->as.literal.len);
		cmark_strbuf_puts(man, "\\f[]");
		break;

	case CMARK_NODE_INLINE_HTML:
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			cmark_strbuf_puts(man, "\\f[B]");
		} else {
			cmark_strbuf_puts(man, "\\f[]");
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			cmark_strbuf_puts(man, "\\f[I]");
		} else {
			cmark_strbuf_puts(man, "\\f[]");
		}
		break;

	case CMARK_NODE_LINK:
		if (!entering) {
			cmark_strbuf_printf(man, " (%s)",
			                    cmark_node_get_url(node));
		}
		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			cmark_strbuf_puts(man, "[IMAGE: ");
			state->plain = node;
		} else {
			cmark_strbuf_puts(man, "]");
		}
		break;

	default:
		assert(false);
		break;
	}

	// cmark_strbuf_putc(man, 'x');
	return 1;
}

char *cmark_render_man(cmark_node *root, long options)
{
	char *result;
	cmark_strbuf man = GH_BUF_INIT;
	struct render_state state = { &man, NULL };
	cmark_node *cur;
	cmark_event_type ev_type;
	cmark_iter *iter = cmark_iter_new(root);

	if (options == 0) options = 0; // avoid warning about unused parameters

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cur = cmark_iter_get_node(iter);
		S_render_node(cur, ev_type, &state);
	}
	result = (char *)cmark_strbuf_detach(&man);

	cmark_iter_free(iter);
	cmark_strbuf_free(&man);
	return result;
}
