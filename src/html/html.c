#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "stmd.h"
#include "debug.h"
#include "scanners.h"
#include "html/houdini.h"

// Functions to convert block and inline lists to HTML strings.

static void escape_html(gh_buf *dest, const unsigned char *source, int length)
{
	if (length < 0)
		length = strlen((char *)source);

	houdini_escape_html0(dest, source, (size_t)length, 0);
}

static void escape_href(gh_buf *dest, const unsigned char *source, int length)
{
	if (length < 0)
		length = strlen((char *)source);

	houdini_escape_href(dest, source, (size_t)length);
}

static inline void cr(gh_buf *html)
{
	if (html->size && html->ptr[html->size - 1] != '\n')
		gh_buf_putc(html, '\n');
}

// Convert a block list to HTML.  Returns 0 on success, and sets result.
void blocks_to_html(gh_buf *html, block *b, bool tight)
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
					gh_buf_puts(html, "<p>");
					inlines_to_html(html, b->inline_content);
					gh_buf_puts(html, "</p>");
					cr(html);
				}
				break;

			case block_quote:
				cr(html);
				gh_buf_puts(html, "<blockquote>");
				blocks_to_html(html, b->children, false);
				gh_buf_puts(html, "</blockquote>");
				cr(html);
				break;

			case list_item:
				cr(html);
				gh_buf_puts(html, "<li>");
				blocks_to_html(html, b->children, tight);
				gh_buf_trim(html); /* TODO: rtrim */
				gh_buf_puts(html, "</li>");
				cr(html);
				break;

			case list:
				// make sure a list starts at the beginning of the line:
				cr(html);
				data = &(b->attributes.list_data);

				if (data->start > 1) {
					gh_buf_printf(html, "<%s start=\"%d\">\n",
							data->list_type == bullet ? "ul" : "ol",
							data->start);
				} else {
					gh_buf_puts(html, data->list_type == bullet ? "<ul>\n" : "<ol>\n");
				}

				blocks_to_html(html, b->children, data->tight);
				gh_buf_puts(html, data->list_type == bullet ? "</ul>" : "</ol>");
				cr(html);
				break;

			case atx_header:
			case setext_header:
				cr(html);
				gh_buf_printf(html, "<h%d>", b->attributes.header_level);
				inlines_to_html(html, b->inline_content);
				gh_buf_printf(html, "</h%d>", b->attributes.header_level);
				cr(html);
				break;

			case indented_code:
			case fenced_code:
				cr(html);

				gh_buf_puts(html, "<pre");

				if (b->tag == fenced_code) {
					gh_buf *info = &b->attributes.fenced_code_data.info;

					if (gh_buf_len(info) > 0) {
						int first_tag = gh_buf_strchr(info, ' ', 0);
						if (first_tag < 0)
							first_tag = gh_buf_len(info);


						gh_buf_puts(html, " class=\"");
						escape_html(html, info->ptr, first_tag);
						gh_buf_putc(html, '"');
					}
				}

				gh_buf_puts(html, "><code>");
				escape_html(html, b->string_content.ptr, b->string_content.size);
				gh_buf_puts(html, "</code></pre>");
				cr(html);
				break;

			case html_block:
				gh_buf_put(html, b->string_content.ptr, b->string_content.size);
				break;

			case hrule:
				gh_buf_puts(html, "<hr />");
				cr(html);
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
void inlines_to_html(gh_buf *html, inl* ils)
{
	gh_buf scrap = GH_BUF_INIT;

	while(ils != NULL) {
		switch(ils->tag) {
			case INL_STRING:
				escape_html(html, ils->content.literal.data, ils->content.literal.len);
				break;

			case INL_LINEBREAK:
				gh_buf_puts(html, "<br />\n");
				break;

			case INL_SOFTBREAK:
				gh_buf_putc(html, '\n');
				break;

			case INL_CODE:
				gh_buf_puts(html, "<code>");
				escape_html(html, ils->content.literal.data, ils->content.literal.len);
				gh_buf_puts(html, "</code>");
				break;

			case INL_RAW_HTML:
			case INL_ENTITY:
				gh_buf_put(html,
						ils->content.literal.data,
						ils->content.literal.len);
				break;

			case INL_LINK:
				gh_buf_puts(html, "<a href=\"");
				escape_href(html, ils->content.linkable.url, -1);

				if (ils->content.linkable.title) {
					gh_buf_puts(html, "\" title=\"");
					escape_html(html, ils->content.linkable.title, -1);
				}

				gh_buf_puts(html, "\">");
				inlines_to_html(html, ils->content.inlines);
				gh_buf_puts(html, "</a>");
				break;

			case INL_IMAGE:
				gh_buf_puts(html, "<img src=\"");
				escape_href(html, ils->content.linkable.url, -1);

				inlines_to_html(&scrap, ils->content.inlines);
				if (scrap.size) {
					gh_buf_puts(html, "\" alt=\"");
					escape_html(html, scrap.ptr, scrap.size);
				}
				gh_buf_clear(&scrap);

				if (ils->content.linkable.title) {
					gh_buf_puts(html, "\" title=\"");
					escape_html(html, ils->content.linkable.title, -1);
				}

				gh_buf_puts(html, "\"/>");
				break;

			case INL_STRONG:
				gh_buf_puts(html, "<strong>");
				inlines_to_html(html, ils->content.inlines);
				gh_buf_puts(html, "</strong>");
				break;

			case INL_EMPH:
				gh_buf_puts(html, "<em>");
				inlines_to_html(html, ils->content.inlines);
				gh_buf_puts(html, "</em>");
				break;
		}
		ils = ils->next;
	}
}
