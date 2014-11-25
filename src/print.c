#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmark.h"
#include "node.h"

#define INDENT 2

static void print_str(const unsigned char *s, int len)
{
	int i;

	if (len < 0)
		len = strlen((char *)s);

	putchar('"');
	for (i = 0; i < len; ++i) {
		unsigned char c = s[i];

		switch (c) {
		case '\n':
			printf("\\n");
			break;
		case '"':
			printf("\\\"");
			break;
		case '\\':
			printf("\\\\");
			break;
		default:
			putchar((int)c);
		}
	}
	putchar('"');
}

// Prettyprint an inline list, for debugging.
static void print_nodes(cmark_node* node, int indent)
{
	int i;
	cmark_list *data;

	while(node != NULL) {
		for (i=0; i < indent; i++) {
			putchar(' ');
		}
		switch(node->type) {
		case NODE_DOCUMENT:
			print_nodes(node->first_child, 0);
			break;
		case NODE_BLOCK_QUOTE:
			printf("block_quote\n");
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_LIST_ITEM:
			printf("list_item\n");
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_LIST:
			data = &(node->as.list);
			if (data->list_type == CMARK_ORDERED_LIST) {
				printf("list (type=ordered tight=%s start=%d delim=%s)\n",
				       (data->tight ? "true" : "false"),
				       data->start,
				       (data->delimiter == CMARK_PAREN_DELIM ? "parens" : "period"));
			} else {
				printf("list (type=bullet tight=%s bullet_char=%c)\n",
				       (data->tight ? "true" : "false"),
				       data->bullet_char);
			}
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_HEADER:
			printf("setext_header (level=%d)\n", node->as.header.level);
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_PARAGRAPH:
			printf("paragraph\n");
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_HRULE:
			printf("hrule\n");
			break;
		case NODE_CODE_BLOCK:
			printf("code block info=");
			print_str(node->as.code.info.ptr, -1);
			putchar(' ');
			print_str(node->string_content.ptr, -1);
			putchar('\n');
			break;
		case NODE_HTML:
			printf("html_block ");
			print_str(node->string_content.ptr, -1);
			putchar('\n');
			break;
		case NODE_REFERENCE_DEF:
			printf("reference_def\n");
			break;
		case NODE_TEXT:
			printf("text ");
			print_str(node->as.literal.data, node->as.literal.len);
			putchar('\n');
			break;
		case NODE_LINEBREAK:
			printf("linebreak\n");
			break;
		case NODE_SOFTBREAK:
			printf("softbreak\n");
			break;
		case NODE_INLINE_CODE:
			printf("code ");
			print_str(node->as.literal.data, node->as.literal.len);
			putchar('\n');
			break;
		case NODE_INLINE_HTML:
			printf("html ");
			print_str(node->as.literal.data, node->as.literal.len);
			putchar('\n');
			break;
		case NODE_LINK:
		case NODE_IMAGE:
			printf("%s url=", node->type == NODE_LINK ? "link" : "image");

			if (node->as.link.url)
				print_str(node->as.link.url, -1);

			if (node->as.link.title) {
				printf(" title=");
				print_str(node->as.link.title, -1);
			}
			putchar('\n');
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_STRONG:
			printf("strong\n");
			print_nodes(node->first_child, indent + INDENT);
			break;
		case NODE_EMPH:
			printf("emph\n");
			print_nodes(node->first_child, indent + INDENT);
			break;
		default:
			break;
		}
		node = node->next;
	}
}

void cmark_debug_print(cmark_node *root)
{
	print_nodes(root, 0);
}
