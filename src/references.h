#ifndef _REFERENCES_H_
#define _REFERENCES_H_

#define REFMAP_SIZE 16

struct reference {
	struct reference *next;
	unsigned char *label;
	unsigned char *url;
	unsigned char *title;
	unsigned int hash;
};

typedef struct reference reference;

struct reference_map {
	reference *table[REFMAP_SIZE];
};

typedef struct reference_map reference_map;

reference_map *reference_map_new(void);
void reference_map_free(reference_map *map);
reference* reference_lookup(reference_map *map, cmark_chunk *label);
extern void reference_create(reference_map *map, cmark_chunk *label, cmark_chunk *url, cmark_chunk *title);

#endif
