#ifndef _INLINES_H_
#define _INLINES_H_

unsigned char *cmark_clean_url(cmark_chunk *url);
unsigned char *cmark_clean_autolink(cmark_chunk *url, int is_email);
unsigned char *cmark_clean_title(cmark_chunk *title);

cmark_node_inl* cmark_parse_inlines(cmark_strbuf *input, cmark_reference_map *refmap);
void cmark_free_inlines(cmark_node_inl* e);

int cmark_parse_reference_inline(cmark_strbuf *input, cmark_reference_map *refmap);

#ifndef CMARK_NO_SHORT_NAMES
  #define clean_url                 cmark_clean_url
  #define clean_autolink            cmark_clean_autolink
  #define clean_title               cmark_clean_title
  #define parse_inlines             cmark_parse_inlines
  #define free_inlines              cmark_free_inlines
  #define parse_reference_inline    cmark_parse_reference_inline
#endif

#endif
