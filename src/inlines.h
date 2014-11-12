#ifndef _INLINES_H_
#define _INLINES_H_

unsigned char *clean_url(cmark_chunk *url);
unsigned char *clean_autolink(cmark_chunk *url, int is_email);
unsigned char *clean_title(cmark_chunk *title);

node_inl* parse_inlines(cmark_strbuf *input, reference_map *refmap);
void free_inlines(node_inl* e);

int parse_reference_inline(cmark_strbuf *input, reference_map *refmap);

#endif
