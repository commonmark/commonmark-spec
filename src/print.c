#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmark.h"
#include "buffer.h"
#include "node.h"

#define INDENT 2

static void print_str(cmark_strbuf* buffer, const unsigned char *s, int len)
{
	int i;

	if (len < 0)
		len = strlen((char *)s);

	cmark_strbuf_putc(buffer, '"');
	for (i = 0; i < len; ++i) {
		unsigned char c = s[i];

		switch (c) {
		case '\n':
			cmark_strbuf_printf(buffer, "\\n");
			break;
		case '"':
			cmark_strbuf_printf(buffer, "\\\"");
			break;
		case '\\':
			cmark_strbuf_printf(buffer, "\\\\");
			break;
		default:
			cmark_strbuf_putc(buffer, (int)c);
		}
	}
	cmark_strbuf_putc(buffer, '"');
}

// Prettyprint an inline list, for debugging.
static void render_nodes(cmark_strbuf* buffer, cmark_node* node, int indent)
{
	int i;
	cmark_list *data;

	while(node != NULL) {
		for (i=0; i < indent; i++) {
			cmark_strbuf_putc(buffer, ' ');
		}
		switch(node->type) {
		case NODE_DOCUMENT:
			break;
		case NODE_BLOCK_QUOTE:
			cmark_strbuf_printf(buffer, "block_quote\n");
			break;
		case NODE_LIST_ITEM:
			cmark_strbuf_printf(buffer, "list_item\n");
			break;
		case NODE_LIST:
			data = &(node->as.list);
			if (data->list_type == CMARK_ORDERED_LIST) {
				cmark_strbuf_printf(buffer, "list (type=ordered tight=%s start=%d delim=%s)\n",
				       (data->tight ? "true" : "false"),
				       data->start,
				       (data->delimiter == CMARK_PAREN_DELIM ? "parens" : "period"));
			} else {
				cmark_strbuf_printf(buffer, "list (type=bullet tight=%s bullet_char=%c)\n",
				       (data->tight ? "true" : "false"),
				       data->bullet_char);
			}
			break;
		case NODE_HEADER:
			cmark_strbuf_printf(buffer, "header (level=%d)\n", node->as.header.level);
			break;
		case NODE_PARAGRAPH:
			cmark_strbuf_printf(buffer, "paragraph\n");
			break;
		case NODE_HRULE:
			cmark_strbuf_printf(buffer, "hrule\n");
			break;
		case NODE_CODE_BLOCK:
			cmark_strbuf_printf(buffer, "code_block fenced=%d fence_length=%d info=", node->as.code.fenced, node->as.code.fence_length);
			print_str(buffer, node->as.code.info.data,
				  node->as.code.info.len);
			cmark_strbuf_putc(buffer, ' ');
			print_str(buffer, node->as.code.literal.data,
				  node->as.code.literal.len);
			cmark_strbuf_putc(buffer, '\n');
			break;
		case NODE_HTML:
			cmark_strbuf_printf(buffer, "html ");
			print_str(buffer, node->as.literal.data,
				  node->as.literal.len);
			cmark_strbuf_putc(buffer, '\n');
			break;
		case NODE_TEXT:
			cmark_strbuf_printf(buffer, "text ");
			print_str(buffer, node->as.literal.data, node->as.literal.len);
			cmark_strbuf_putc(buffer, '\n');
			break;
		case NODE_LINEBREAK:
			cmark_strbuf_printf(buffer, "linebreak\n");
			break;
		case NODE_SOFTBREAK:
			cmark_strbuf_printf(buffer, "softbreak\n");
			break;
		case NODE_CODE:
			cmark_strbuf_printf(buffer, "code ");
			print_str(buffer, node->as.literal.data, node->as.literal.len);
			cmark_strbuf_putc(buffer, '\n');
			break;
		case NODE_INLINE_HTML:
			cmark_strbuf_printf(buffer, "inline_html ");
			print_str(buffer, node->as.literal.data, node->as.literal.len);
			cmark_strbuf_putc(buffer, '\n');
			break;
		case NODE_LINK:
		case NODE_IMAGE:
			cmark_strbuf_printf(buffer, "%s url=", node->type == NODE_LINK ? "link" : "image");

			if (node->as.link.url)
				print_str(buffer, node->as.link.url, -1);

			if (node->as.link.title) {
				cmark_strbuf_printf(buffer, " title=");
				print_str(buffer, node->as.link.title, -1);
			}
			cmark_strbuf_putc(buffer, '\n');
			break;
		case NODE_STRONG:
			cmark_strbuf_printf(buffer, "strong\n");
			break;
		case NODE_EMPH:
			cmark_strbuf_printf(buffer, "emph\n");
			break;
		default:
			break;
		}
		if (node->first_child) { // render children if any
			indent += INDENT;
			node = node->first_child;
		} else if (node->next) { // otherwise render next sibling
			node = node->next;
		} else {
			node = node->parent;  // back up to parent
			while (node) {
				indent -= INDENT;
				if (node->next) {
					node = node->next;
					break;
				} else {
					node = node->parent;
				}
				if (!node) {
					break;
				}
			}
		}
	}
}

char *cmark_render_ast(cmark_node *root)
{
	char* result;
	cmark_strbuf buffer = GH_BUF_INIT;
	render_nodes(&buffer, root, -2);
	result = (char *)cmark_strbuf_detach(&buffer);
	cmark_strbuf_free(&buffer);
	return result;
}
