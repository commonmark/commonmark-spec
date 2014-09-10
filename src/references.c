#include "stmd.h"
#include "utf8.h"
#include "references.h"

static unsigned int
refhash(const unsigned char *link_ref)
{
	unsigned int hash = 0;

	while (*link_ref)
		hash = (*link_ref++) + (hash << 6) + (hash << 16) - hash;

	return hash;
}

// normalize reference:  collapse internal whitespace to single space,
// remove leading/trailing whitespace, case fold
static unsigned char *normalize_reference(chunk *ref)
{
	strbuf normalized = GH_BUF_INIT;

	utf8proc_case_fold(&normalized, ref->data, ref->len);
	strbuf_trim(&normalized);
	strbuf_normalize_whitespace(&normalized);

	return strbuf_detach(&normalized);
}

static void add_reference(reference_map *map, reference* ref)
{
	ref->next = map->table[ref->hash % REFMAP_SIZE];
	map->table[ref->hash % REFMAP_SIZE] = ref;
}

extern reference *reference_create(reference_map *map, chunk *label, chunk *url, chunk *title)
{
	reference *ref;
	ref = malloc(sizeof(reference));
	ref->label = normalize_reference(label);
	ref->hash = refhash(ref->label);
	ref->url = clean_url(url);
	ref->title = clean_title(title);
	ref->next = NULL;

	add_reference(map, ref);

	return ref;
}

// Returns reference if refmap contains a reference with matching
// label, otherwise NULL.
reference* reference_lookup(reference_map *map, chunk *label)
{
	reference *ref = NULL;
	unsigned char *norm;
	unsigned int hash;
	
	if (map == NULL)
		return NULL;
	
	norm = normalize_reference(label);
	hash = refhash(norm);
	ref = map->table[hash % REFMAP_SIZE];

	while (ref) {
		if (ref->label[0] == norm[0] &&
			!strcmp((char *)ref->label, (char *)norm))
			break;
		ref = ref->next;
	}

	free(norm);
	return ref;
}

static void reference_free(reference *ref)
{
	free(ref->label);
	free(ref->url);
	free(ref->title);
	free(ref);
}

void reference_map_free(reference_map *map)
{
	unsigned int i;

	for (i = 0; i < REFMAP_SIZE; ++i) {
		reference *ref = map->table[i];
		reference *next;

		while (ref) {
			next = ref->next;
			reference_free(ref);
			ref = next;
		}
	}

	free(map->table);
	free(map);
}

reference_map *reference_map_new(void)
{
	reference_map *map = malloc(sizeof(reference_map));
	memset(map, 0x0, sizeof(reference_map));
	return map;
}

