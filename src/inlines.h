#ifndef CMARK_INLINES_H
#define CMARK_INLINES_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned char *cmark_clean_url(cmark_chunk *url);
unsigned char *cmark_clean_title(cmark_chunk *title);

void cmark_parse_inlines(cmark_node* parent, cmark_reference_map *refmap);

int cmark_parse_reference_inline(cmark_strbuf *input, cmark_reference_map *refmap);

#ifndef CMARK_NO_SHORT_NAMES
  #define parse_inlines             cmark_parse_inlines
  #define parse_reference_inline    cmark_parse_reference_inline
  #define clean_url                 cmark_clean_url
  #define clean_title               cmark_clean_title
#endif

#ifdef __cplusplus
}
#endif

#endif
