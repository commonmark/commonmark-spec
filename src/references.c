#include "cmark.h"
#include "utf8.h"
#include "parser.h"
#include "references.h"
#include "inlines.h"
#include "chunk.h"

static unsigned int
refhash(const unsigned char *link_ref)
{
	unsigned int hash = 0;

	while (*link_ref)
		hash = (*link_ref++) + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static void reference_free(cmark_reference *ref)
{
	if(ref != NULL) {
		free(ref->label);
		free(ref->url);
		free(ref->title);
		free(ref);
	}
}

// normalize reference:  collapse internal whitespace to single space,
// remove leading/trailing whitespace, case fold
// Return NULL if the reference name is actually empty (i.e. composed
// solely from whitespace)
static unsigned char *normalize_reference(cmark_chunk *ref)
{
	cmark_strbuf normalized = GH_BUF_INIT;
	unsigned char *result;

	if(ref == NULL)
		return NULL;

	if (ref->len == 0)
		return NULL;

	utf8proc_case_fold(&normalized, ref->data, ref->len);
	cmark_strbuf_trim(&normalized);
	cmark_strbuf_normalize_whitespace(&normalized);

	result = cmark_strbuf_detach(&normalized);
	assert(result);

	if (result[0] == '\0') {
		free(result);
		return NULL;
	}

	return result;
}

static void add_reference(cmark_reference_map *map, cmark_reference* ref)
{
	cmark_reference *t = ref->next = map->table[ref->hash % REFMAP_SIZE];

	while (t) {
		if (t->hash == ref->hash &&
		    !strcmp((char *)t->label, (char *)ref->label)) {
			reference_free(ref);
			return;
		}

		t = t->next;
	}

	map->table[ref->hash % REFMAP_SIZE] = ref;
}

void cmark_reference_create(cmark_reference_map *map, cmark_chunk *label, cmark_chunk *url,
                            cmark_chunk *title)
{
	cmark_reference *ref;
	unsigned char *reflabel = normalize_reference(label);

	/* empty reference name, or composed from only whitespace */
	if (reflabel == NULL)
		return;

	ref = (cmark_reference *)calloc(1, sizeof(*ref));
	if(ref != NULL) {
		ref->label = reflabel;
		ref->hash = refhash(ref->label);
		ref->url = cmark_clean_url(url);
		ref->title = cmark_clean_title(title);
		ref->next = NULL;

		add_reference(map, ref);
	}
}

// Returns reference if refmap contains a reference with matching
// label, otherwise NULL.
cmark_reference* cmark_reference_lookup(cmark_reference_map *map, cmark_chunk *label)
{
	cmark_reference *ref = NULL;
	unsigned char *norm;
	unsigned int hash;

	if (label->len > MAX_LINK_LABEL_LENGTH)
		return NULL;

	if (map == NULL)
		return NULL;

	norm = normalize_reference(label);
	if (norm == NULL)
		return NULL;

	hash = refhash(norm);
	ref = map->table[hash % REFMAP_SIZE];

	while (ref) {
		if (ref->hash == hash &&
		    !strcmp((char *)ref->label, (char *)norm))
			break;
		ref = ref->next;
	}

	free(norm);
	return ref;
}

void cmark_reference_map_free(cmark_reference_map *map)
{
	unsigned int i;

	if(map == NULL)
		return;

	for (i = 0; i < REFMAP_SIZE; ++i) {
		cmark_reference *ref = map->table[i];
		cmark_reference *next;

		while (ref) {
			next = ref->next;
			reference_free(ref);
			ref = next;
		}
	}

	free(map);
}

cmark_reference_map *cmark_reference_map_new(void)
{
	return (cmark_reference_map *)calloc(1, sizeof(cmark_reference_map));
}
