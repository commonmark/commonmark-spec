#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stmd.h"
#include "debug.h"

static void print_str(const unsigned char *s, int len)
{
	int i;

	if (len < 0)
		len = strlen(s);

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

// Functions to pretty-print inline and block lists, for debugging.
// Prettyprint an inline list, for debugging.
extern void print_blocks(block* b, int indent)
{
	struct ListData *data;

	while(b != NULL) {
		// printf("%3d %3d %3d| ", b->start_line, b->start_column, b->end_line);
		for (int i=0; i < indent; i++) {
			putchar(' ');
		}

		switch(b->tag) {
		case document:
			printf("document\n");
			print_blocks(b->children, indent + 2);
			break;
		case block_quote:
			printf("block_quote\n");
			print_blocks(b->children, indent + 2);
			break;
		case list_item:
			data = &(b->attributes.list_data);
			printf("list_item\n");
			print_blocks(b->children, indent + 2);
			break;
		case list:
			data = &(b->attributes.list_data);
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
		case atx_header:
			printf("atx_header (level=%d)\n", b->attributes.header_level);
			print_inlines(b->inline_content, indent + 2);
			break;
		case setext_header:
			printf("setext_header (level=%d)\n", b->attributes.header_level);
			print_inlines(b->inline_content, indent + 2);
			break;
		case paragraph:
			printf("paragraph\n");
			print_inlines(b->inline_content, indent + 2);
			break;
		case hrule:
			printf("hrule\n");
			break;
		case indented_code:
			printf("indented_code ");
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case fenced_code:
			printf("fenced_code length=%d info=",
				b->attributes.fenced_code_data.fence_length);
			print_str(b->attributes.fenced_code_data.info.ptr, -1);
			putchar(' ');
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case html_block:
			printf("html_block ");
			print_str(b->string_content.ptr, -1);
			putchar('\n');
			break;
		case reference_def:
			printf("reference_def\n");
			break;
		default:
			printf("# NOT IMPLEMENTED (%d)\n", b->tag);
			break;
		}
		b = b->next;
	}
}

// Prettyprint an inline list, for debugging.
extern void print_inlines(inl* ils, int indent)
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
		case INL_ENTITY:
			printf("entity ");
			print_str(ils->content.literal.data, ils->content.literal.len);
			putchar('\n');
			break;
		case INL_LINK:
		case INL_IMAGE:
			printf("%s url=", ils->tag == INL_LINK ? "link" : "image");
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
