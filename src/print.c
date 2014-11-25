#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmark.h"
#include "buffer.h"
#include "node.h"

#define INDENT 2

static void print_str(strbuf* buffer, const unsigned char *s, int len)
{
	int i;

	if (len < 0)
		len = strlen((char *)s);

	strbuf_putc(buffer, '"');
	for (i = 0; i < len; ++i) {
		unsigned char c = s[i];

		switch (c) {
		case '\n':
			strbuf_printf(buffer, "\\n");
			break;
		case '"':
			strbuf_printf(buffer, "\\\"");
			break;
		case '\\':
			strbuf_printf(buffer, "\\\\");
			break;
		default:
			strbuf_putc(buffer, (int)c);
		}
	}
	strbuf_putc(buffer, '"');
}

// Prettyprint an inline list, for debugging.
static void render_nodes(strbuf* buffer, cmark_node* node, int indent)
{
	int i;
	cmark_list *data;

	while(node != NULL) {
		for (i=0; i < indent; i++) {
			strbuf_putc(buffer, ' ');
		}
		switch(node->type) {
		case NODE_DOCUMENT:
			break;
		case NODE_BLOCK_QUOTE:
			strbuf_printf(buffer, "block_quote\n");
			break;
		case NODE_LIST_ITEM:
			strbuf_printf(buffer, "list_item\n");
			break;
		case NODE_LIST:
			data = &(node->as.list);
			if (data->list_type == CMARK_ORDERED_LIST) {
				strbuf_printf(buffer, "list (type=ordered tight=%s start=%d delim=%s)\n",
				       (data->tight ? "true" : "false"),
				       data->start,
				       (data->delimiter == CMARK_PAREN_DELIM ? "parens" : "period"));
			} else {
				strbuf_printf(buffer, "list (type=bullet tight=%s bullet_char=%c)\n",
				       (data->tight ? "true" : "false"),
				       data->bullet_char);
			}
			break;
		case NODE_HEADER:
			strbuf_printf(buffer, "header (level=%d)\n", node->as.header.level);
			break;
		case NODE_PARAGRAPH:
			strbuf_printf(buffer, "paragraph\n");
			break;
		case NODE_HRULE:
			strbuf_printf(buffer, "hrule\n");
			break;
		case NODE_CODE_BLOCK:
			strbuf_printf(buffer, "code_block info=");
			print_str(buffer, node->as.code.info.ptr, -1);
			strbuf_putc(buffer, ' ');
			print_str(buffer, node->string_content.ptr, -1);
			strbuf_putc(buffer, '\n');
			break;
		case NODE_HTML:
			strbuf_printf(buffer, "html ");
			print_str(buffer, node->string_content.ptr, -1);
			strbuf_putc(buffer, '\n');
			break;
		case NODE_REFERENCE_DEF:
			// skip
			// strbuf_printf(buffer, "reference_def\n");
			break;
		case NODE_TEXT:
			strbuf_printf(buffer, "text ");
			print_str(buffer, node->as.literal.data, node->as.literal.len);
			strbuf_putc(buffer, '\n');
			break;
		case NODE_LINEBREAK:
			strbuf_printf(buffer, "linebreak\n");
			break;
		case NODE_SOFTBREAK:
			strbuf_printf(buffer, "softbreak\n");
			break;
		case NODE_INLINE_CODE:
			strbuf_printf(buffer, "code ");
			print_str(buffer, node->as.literal.data, node->as.literal.len);
			strbuf_putc(buffer, '\n');
			break;
		case NODE_INLINE_HTML:
			strbuf_printf(buffer, "inline_html ");
			print_str(buffer, node->as.literal.data, node->as.literal.len);
			strbuf_putc(buffer, '\n');
			break;
		case NODE_LINK:
		case NODE_IMAGE:
			strbuf_printf(buffer, "%s url=", node->type == NODE_LINK ? "link" : "image");

			if (node->as.link.url)
				print_str(buffer, node->as.link.url, -1);

			if (node->as.link.title) {
				strbuf_printf(buffer, " title=");
				print_str(buffer, node->as.link.title, -1);
			}
			strbuf_putc(buffer, '\n');
			break;
		case NODE_STRONG:
			strbuf_printf(buffer, "strong\n");
			break;
		case NODE_EMPH:
			strbuf_printf(buffer, "emph\n");
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
	strbuf buffer = GH_BUF_INIT;
	render_nodes(&buffer, root, -2);
	result = (char *)strbuf_detach(&buffer);
	strbuf_free(&buffer);
	return result;
}
