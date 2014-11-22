#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "references.h"
#include "html/houdini.h"
#include "cmark.h"
#include "buffer.h"

char *cmark_markdown_to_html(const char *text, int len)
{
	cmark_node *blocks;
	char *result;

	blocks = cmark_parse_document(text, len);

	result = cmark_render_html(blocks);
	cmark_free_nodes(blocks);

	return result;
}

