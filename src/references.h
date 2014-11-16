#ifndef CMARK_REFERENCES_H
#define CMARK_REFERENCES_H

#include "chunk.h"
#include "ast.h"

cmark_reference_map *cmark_reference_map_new(void);
void cmark_reference_map_free(cmark_reference_map *map);
cmark_reference* cmark_reference_lookup(cmark_reference_map *map, cmark_chunk *label);
extern void cmark_reference_create(cmark_reference_map *map, cmark_chunk *label, cmark_chunk *url, cmark_chunk *title);

#ifndef CMARK_NO_SHORT_NAMES
  #define reference             cmark_reference
  #define reference_map         cmark_reference_map
  #define reference_map_new     cmark_reference_map_new
  #define reference_map_free    cmark_reference_map_free
  #define reference_lookup      cmark_reference_lookup
  #define reference_create      cmark_reference_create
#endif

#endif
