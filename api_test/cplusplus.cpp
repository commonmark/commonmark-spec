#include <cstdlib>

#include "cmark.h"

#include "harness.h"

extern "C" void
test_cplusplus(test_batch_runner *runner)
{
    static const char md[] = "paragraph\n";
    char *html = cmark_markdown_to_html(md, sizeof(md) - 1);
    STR_EQ(runner, html, "<p>paragraph</p>\n", "libcmark works with C++");
    free(html);
}

