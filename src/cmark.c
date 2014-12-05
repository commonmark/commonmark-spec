#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "houdini.h"
#include "cmark.h"
#include "buffer.h"

char *cmark_markdown_to_html(const char *text, int len)
{
	cmark_node *doc;
	char *result;

	doc = cmark_parse_document(text, len);

	result = cmark_render_html(doc);
	cmark_node_free(doc);

	return result;
}

