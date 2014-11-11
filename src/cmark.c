#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "cmark.h"
#include "buffer.h"

extern unsigned char *cmark_markdown_to_html(unsigned char *text)
{
	node_block *blocks;
	strbuf htmlbuf = GH_BUF_INIT;

	blocks = cmark_parse_document(text, sizeof(text));

	cmark_render_html(&htmlbuf, blocks);
	cmark_free_nodes(blocks);

	return strbuf_detach(&htmlbuf);
}
