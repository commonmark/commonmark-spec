#ifndef _INLINES_H_
#define _INLINES_H_

unsigned char *clean_url(chunk *url);
unsigned char *clean_autolink(chunk *url, int is_email);
unsigned char *clean_title(chunk *title);

node_inl* parse_inlines(strbuf *input, reference_map *refmap);
void free_inlines(node_inl* e);

int parse_reference_inline(strbuf *input, reference_map *refmap);

#endif
