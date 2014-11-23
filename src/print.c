#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmark.h"
#include "node.h"
#include "debug.h"

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
static void print_inlines(cmark_node* ils, int indent)
{
	int i;

	while(ils != NULL) {
		for (i=0; i < indent; i++) {
			putchar(' ');
		}
		switch(ils->type) {
		case NODE_STRING:
			printf("str ");
			print_str(ils->as.literal.data, ils->as.literal.len);
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
			print_str(ils->as.literal.data, ils->as.literal.len);
			putchar('\n');
			break;
		case NODE_INLINE_HTML:
			printf("html ");
			print_str(ils->as.literal.data, ils->as.literal.len);
			putchar('\n');
			break;
		case NODE_LINK:
		case NODE_IMAGE:
			printf("%s url=", ils->type == NODE_LINK ? "link" : "image");

			if (ils->as.link.url)
				print_str(ils->as.link.url, -1);

			if (ils->as.link.title) {
				printf(" title=");
				print_str(ils->as.link.title, -1);
			}
			putchar('\n');
			print_inlines(ils->first_child, indent + 2);
			break;
		case NODE_STRONG:
			printf("strong\n");
			print_inlines(ils->first_child, indent + 2);
			break;
		case NODE_EMPH:
			printf("emph\n");
			print_inlines(ils->first_child, indent + 2);
			break;
		default:
			break;
		}
		ils = ils->next;
	}
}

// Functions to pretty-print inline and cmark_node lists, for debugging.
// Prettyprint an inline list, for debugging.
static void print_blocks(cmark_node* b, int indent)
{
	cmark_list *data;
	int i;

	while(b != NULL) {
		for (i=0; i < indent; i++) {
			putchar(' ');
		}

		switch(b->type) {
		case NODE_DOCUMENT:
			printf("document\n");
			print_blocks(b->first_child, indent + 2);
			break;
		case NODE_BLOCK_QUOTE:
			printf("block_quote\n");
			print_blocks(b->first_child, indent + 2);
			break;
		case NODE_LIST_ITEM:
			printf("list_item\n");
			print_blocks(b->first_child, indent + 2);
			break;
		case NODE_LIST:
			data = &(b->as.list);
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
			print_blocks(b->first_child, indent + 2);
			break;
		case NODE_HEADER:
			printf("setext_header (level=%d)\n", b->as.header.level);
			print_inlines(b->first_child, indent + 2);
			break;
		case NODE_PARAGRAPH:
			printf("paragraph\n");
			print_inlines(b->first_child, indent + 2);
			break;
		case NODE_HRULE:
			printf("hrule\n");
			break;
		case NODE_CODE_BLOCK:
			printf("code block info=");
			print_str(b->as.code.info.ptr, -1);
			putchar(' ');
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case NODE_HTML:
			printf("html_block ");
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case NODE_REFERENCE_DEF:
			printf("reference_def\n");
			break;
		default:
			printf("# NOT IMPLEMENTED (%d)\n", b->type);
			break;
		}
		b = b->next;
	}
}

void cmark_debug_print(cmark_node *root)
{
	print_blocks(root, 0);
}
