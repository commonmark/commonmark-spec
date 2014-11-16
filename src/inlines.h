#ifndef CMARK_INLINES_H
#define CMARK_INLINES_H

unsigned char *cmark_clean_url(cmark_chunk *url);
unsigned char *cmark_clean_title(cmark_chunk *title);

cmark_node_inl* cmark_parse_inlines(cmark_strbuf *input, cmark_reference_map *refmap);

int cmark_parse_reference_inline(cmark_strbuf *input, cmark_reference_map *refmap);

#ifndef CMARK_NO_SHORT_NAMES
  #define parse_inlines             cmark_parse_inlines
  #define parse_reference_inline    cmark_parse_reference_inline
  #define clean_url                 cmark_clean_url
  #define clean_title               cmark_clean_title
#endif

#endif
