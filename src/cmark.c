#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "cmark.h"
#include "buffer.h"

extern unsigned char *cmark_markdown_to_html(unsigned char *text, int len)
{
	node_block *blocks;
	strbuf htmlbuf = GH_BUF_INIT;
	unsigned char *result;

	blocks = cmark_parse_document(text, len);

	cmark_render_html(&htmlbuf, blocks);
	cmark_free_nodes(blocks);

	result = strbuf_detach(&htmlbuf);
	strbuf_free(&htmlbuf);

	return result;
}
