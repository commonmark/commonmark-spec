#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "stmd.h"
#include "debug.h"
#include "html/houdini.h"

// Functions to convert block and inline lists to HTML strings.

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

// Convert a block list to HTML.  Returns 0 on success, and sets result.
void blocks_to_html(strbuf *html, block *b, bool tight)
{
	struct ListData *data;

	while(b != NULL) {
		switch(b->tag) {
			case document:
				blocks_to_html(html, b->children, false);
				break;

			case paragraph:
				if (tight) {
					inlines_to_html(html, b->inline_content);
				} else {
					cr(html);
					strbuf_puts(html, "<p>");
					inlines_to_html(html, b->inline_content);
					strbuf_puts(html, "</p>\n");
				}
				break;

			case block_quote:
				cr(html);
				strbuf_puts(html, "<blockquote>\n");
				blocks_to_html(html, b->children, false);
				strbuf_puts(html, "</blockquote>\n");
				break;

			case list_item:
				cr(html);
				strbuf_puts(html, "<li>");
				blocks_to_html(html, b->children, tight);
				strbuf_trim(html); /* TODO: rtrim */
				strbuf_puts(html, "</li>\n");
				break;

			case list:
				// make sure a list starts at the beginning of the line:
				cr(html);
				data = &(b->attributes.list_data);

				if (data->start > 1) {
					strbuf_printf(html, "<%s start=\"%d\">\n",
							data->list_type == bullet ? "ul" : "ol",
							data->start);
				} else {
					strbuf_puts(html, data->list_type == bullet ? "<ul>\n" : "<ol>\n");
				}

				blocks_to_html(html, b->children, data->tight);
				strbuf_puts(html, data->list_type == bullet ? "</ul>" : "</ol>");
				strbuf_putc(html, '\n');
				break;

			case atx_header:
			case setext_header:
				cr(html);
				strbuf_printf(html, "<h%d>", b->attributes.header_level);
				inlines_to_html(html, b->inline_content);
				strbuf_printf(html, "</h%d>\n", b->attributes.header_level);
				break;

			case indented_code:
			case fenced_code:
				cr(html);

				strbuf_puts(html, "<pre");

				if (b->tag == fenced_code) {
					strbuf *info = &b->attributes.fenced_code_data.info;

					if (strbuf_len(info) > 0) {
						int first_tag = strbuf_strchr(info, ' ', 0);
						if (first_tag < 0)
							first_tag = strbuf_len(info);


						strbuf_puts(html, " class=\"");
						escape_html(html, info->ptr, first_tag);
						strbuf_putc(html, '"');
					}
				}

				strbuf_puts(html, "><code>");
				escape_html(html, b->string_content.ptr, b->string_content.size);
				strbuf_puts(html, "</code></pre>\n");
				break;

			case html_block:
				strbuf_put(html, b->string_content.ptr, b->string_content.size);
				break;

			case hrule:
				strbuf_puts(html, "<hr />\n");
				break;

			case reference_def:
				break;

			default:
				assert(false);
		}

		b = b->next;
	}
}

// Convert an inline list to HTML.  Returns 0 on success, and sets result.
void inlines_to_html(strbuf *html, struct inl* ils)
{
	strbuf scrap = GH_BUF_INIT;

	while(ils != NULL) {
		switch(ils->tag) {
			case INL_STRING:
				escape_html(html, ils->content.literal.data, ils->content.literal.len);
				break;

			case INL_LINEBREAK:
				strbuf_puts(html, "<br />\n");
				break;

			case INL_SOFTBREAK:
				strbuf_putc(html, '\n');
				break;

			case INL_CODE:
				strbuf_puts(html, "<code>");
				escape_html(html, ils->content.literal.data, ils->content.literal.len);
				strbuf_puts(html, "</code>");
				break;

			case INL_RAW_HTML:
			case INL_ENTITY:
				strbuf_put(html,
						ils->content.literal.data,
						ils->content.literal.len);
				break;

			case INL_LINK:
				strbuf_puts(html, "<a href=\"");
				if (ils->content.linkable.url)
					escape_href(html, ils->content.linkable.url, -1);

				if (ils->content.linkable.title) {
					strbuf_puts(html, "\" title=\"");
					escape_html(html, ils->content.linkable.title, -1);
				}

				strbuf_puts(html, "\">");
				inlines_to_html(html, ils->content.inlines);
				strbuf_puts(html, "</a>");
				break;

			case INL_IMAGE:
				strbuf_puts(html, "<img src=\"");
				if (ils->content.linkable.url)
					escape_href(html, ils->content.linkable.url, -1);

				inlines_to_html(&scrap, ils->content.inlines);
				strbuf_puts(html, "\" alt=\"");
				if (scrap.size)
					escape_html(html, scrap.ptr, scrap.size);
				strbuf_clear(&scrap);

				if (ils->content.linkable.title) {
					strbuf_puts(html, "\" title=\"");
					escape_html(html, ils->content.linkable.title, -1);
				}

				strbuf_puts(html, "\"/>");
				break;

			case INL_STRONG:
				strbuf_puts(html, "<strong>");
				inlines_to_html(html, ils->content.inlines);
				strbuf_puts(html, "</strong>");
				break;

			case INL_EMPH:
				strbuf_puts(html, "<em>");
				inlines_to_html(html, ils->content.inlines);
				strbuf_puts(html, "</em>");
				break;
		}
		ils = ils->next;
	}
}
