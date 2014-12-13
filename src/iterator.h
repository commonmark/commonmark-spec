#ifndef CMARK_ITERATOR_H
#define CMARK_ITERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmark.h"
#include "node.h"

struct cmark_iter {
	cmark_node       *current;
	cmark_node       *root;
	cmark_event_type event_type;
};

#ifdef __cplusplus
}
#endif

#endif
