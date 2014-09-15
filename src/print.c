#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stmd.h"
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
static void print_inlines(node_inl* ils, int indent)
{
	while(ils != NULL) {
		for (int i=0; i < indent; i++) {
			putchar(' ');
		}
		switch(ils->tag) {
		case INL_STRING:
			printf("str ");
			print_str(ils->content.literal.data, ils->content.literal.len);
			putchar('\n');
			break;
		case INL_LINEBREAK:
			printf("linebreak\n");
			break;
		case INL_SOFTBREAK:
			printf("softbreak\n");
			break;
		case INL_CODE:
			printf("code ");
			print_str(ils->content.literal.data, ils->content.literal.len);
			putchar('\n');
			break;
		case INL_RAW_HTML:
			printf("html ");
			print_str(ils->content.literal.data, ils->content.literal.len);
			putchar('\n');
			break;
		case INL_LINK:
		case INL_IMAGE:
			printf("%s url=", ils->tag == INL_LINK ? "link" : "image");

			if (ils->content.linkable.url)
				print_str(ils->content.linkable.url, -1);

			if (ils->content.linkable.title) {
				printf(" title=");
				print_str(ils->content.linkable.title, -1);
			}
			putchar('\n');
			print_inlines(ils->content.linkable.label, indent + 2);
			break;
		case INL_STRONG:
			printf("strong\n");
			print_inlines(ils->content.linkable.label, indent + 2);
			break;
		case INL_EMPH:
			printf("emph\n");
			print_inlines(ils->content.linkable.label, indent + 2);
			break;
		}
		ils = ils->next;
	}
}

// Functions to pretty-print inline and node_block lists, for debugging.
// Prettyprint an inline list, for debugging.
static void print_blocks(node_block* b, int indent)
{
	struct ListData *data;

	while(b != NULL) {
		for (int i=0; i < indent; i++) {
			putchar(' ');
		}

		switch(b->tag) {
		case BLOCK_DOCUMENT:
			printf("document\n");
			print_blocks(b->children, indent + 2);
			break;
		case BLOCK_BQUOTE:
			printf("block_quote\n");
			print_blocks(b->children, indent + 2);
			break;
		case BLOCK_LIST_ITEM:
			printf("list_item\n");
			print_blocks(b->children, indent + 2);
			break;
		case BLOCK_LIST:
			data = &(b->as.list);
			if (data->list_type == ordered) {
				printf("list (type=ordered tight=%s start=%d delim=%s)\n",
						(data->tight ? "true" : "false"),
						data->start,
						(data->delimiter == parens ? "parens" : "period"));
			} else {
				printf("list (type=bullet tight=%s bullet_char=%c)\n",
						(data->tight ? "true" : "false"),
						data->bullet_char);
			}
			print_blocks(b->children, indent + 2);
			break;
		case BLOCK_ATX_HEADER:
			printf("atx_header (level=%d)\n", b->as.header.level);
			print_inlines(b->inline_content, indent + 2);
			break;
		case BLOCK_SETEXT_HEADER:
			printf("setext_header (level=%d)\n", b->as.header.level);
			print_inlines(b->inline_content, indent + 2);
			break;
		case BLOCK_PARAGRAPH:
			printf("paragraph\n");
			print_inlines(b->inline_content, indent + 2);
			break;
		case BLOCK_HRULE:
			printf("hrule\n");
			break;
		case BLOCK_INDENTED_CODE:
			printf("indented_code ");
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case BLOCK_FENCED_CODE:
			printf("fenced_code length=%d info=",
				b->as.code.fence_length);
			print_str(b->as.code.info.ptr, -1);
			putchar(' ');
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case BLOCK_HTML:
			printf("html_block ");
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case BLOCK_REFERENCE_DEF:
			printf("reference_def\n");
			break;
		default:
			printf("# NOT IMPLEMENTED (%d)\n", b->tag);
			break;
		}
		b = b->next;
	}
}

void stmd_debug_print(node_block *root)
{
	print_blocks(root, 0);
}
