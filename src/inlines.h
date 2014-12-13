#ifndef CMARK_INLINES_H
#define CMARK_INLINES_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned char *cmark_clean_url(chunk *url);
unsigned char *cmark_clean_title(chunk *title);

void cmark_parse_inlines(cmark_node* parent, cmark_reference_map *refmap);

int cmark_parse_reference_inline(strbuf *input, cmark_reference_map *refmap);

#ifdef __cplusplus
}
#endif

#endif
