#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmark.h"
#include "node.h"

#include "harness.h"

static void
create_tree(test_batch_runner *runner)
{
	char *html;
	cmark_node *doc = cmark_node_new(CMARK_NODE_DOCUMENT);

	cmark_node *p = cmark_node_new(CMARK_NODE_PARAGRAPH);
	OK(runner, cmark_node_append_child(doc, p), "append1");
	INT_EQ(runner, cmark_node_check(doc), 0, "append1 consistent");

	cmark_node *emph = cmark_node_new(CMARK_NODE_EMPH);
	OK(runner, cmark_node_prepend_child(p, emph), "prepend1");
	INT_EQ(runner, cmark_node_check(doc), 0, "prepend1 consistent");

	cmark_node *str1 = cmark_node_new(CMARK_NODE_STRING);
	cmark_node_set_string_content(str1, "Hello, ");
	OK(runner, cmark_node_prepend_child(p, str1), "prepend2");
	INT_EQ(runner, cmark_node_check(doc), 0, "prepend2 consistent");

	cmark_node *str3 = cmark_node_new(CMARK_NODE_STRING);
	cmark_node_set_string_content(str3, "!");
	OK(runner, cmark_node_append_child(p, str3), "append2");
	INT_EQ(runner, cmark_node_check(doc), 0, "append2 consistent");

	cmark_node *str2 = cmark_node_new(CMARK_NODE_STRING);
	cmark_node_set_string_content(str2, "world");
	OK(runner, cmark_node_append_child(emph, str2), "append3");
	INT_EQ(runner, cmark_node_check(doc), 0, "append3 consistent");

	html = (char *)cmark_render_html(doc);
	STR_EQ(runner, html, "<p>Hello, <em>world</em>!</p>\n",
	       "render_html");
	free(html);

	OK(runner, cmark_node_insert_before(str1, str3), "ins before1");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins before1 consistent");
	// 31e
	OK(runner, cmark_node_first_child(p) == str3, "ins before1 works");

	OK(runner, cmark_node_insert_before(str1, emph), "ins before2");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins before2 consistent");
	// 3e1
	OK(runner, cmark_node_last_child(p) == str1, "ins before2 works");

	OK(runner, cmark_node_insert_after(str1, str3), "ins after1");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins after1 consistent");
	// e13
	OK(runner, cmark_node_last_child(p) == str3, "ins after1 works");

	OK(runner, cmark_node_insert_after(str1, emph), "ins after2");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins after2 consistent");
	// 1e3
	OK(runner, cmark_node_first_child(p) == str1, "ins after2 works");

	html = (char *)cmark_render_html(doc);
	STR_EQ(runner, html, "<p>Hello, <em>world</em>!</p>\n",
	       "render_html after shuffling");
	free(html);

	cmark_node_destroy(doc);
}

int main() {
	int retval;
	test_batch_runner *runner = test_batch_runner_new();

	create_tree(runner);

	test_print_summary(runner);
	retval =  test_ok(runner) ? 0 : 1;
	free(runner);

	return retval;
}

