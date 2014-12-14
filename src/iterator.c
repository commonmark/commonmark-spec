#include <stdlib.h>

#include "config.h"
#include "node.h"
#include "cmark.h"
#include "iterator.h"

cmark_iter*
cmark_iter_new(cmark_node *root)
{
	cmark_iter *iter = (cmark_iter*)malloc(sizeof(cmark_iter));
	if (iter == NULL) {
		return NULL;
	} else {
		iter->root = root;
		iter->current = root;
		iter->event_type = CMARK_EVENT_ENTER;
		return iter;
	}
}

void
cmark_iter_free(cmark_iter *iter)
{
	free(iter);
}

cmark_event_type
cmark_iter_next(cmark_iter *iter)
{
	return iter->event_type;
}

int S_is_leaf(cmark_node *node)
{
	switch (cmark_node_get_type(node)) {
	case CMARK_NODE_HTML:
	case CMARK_NODE_HRULE:
	case CMARK_NODE_CODE_BLOCK:
	case CMARK_NODE_TEXT:
	case CMARK_NODE_SOFTBREAK:
	case CMARK_NODE_LINEBREAK:
	case CMARK_NODE_CODE:
	case CMARK_NODE_INLINE_HTML:
		return 1;
	default:
		return 0;
	}
}

cmark_node*
cmark_iter_get_node(cmark_iter *iter)
{
	/* we'll return current */
	cmark_node *cur = iter->current;

	if (cur == NULL || iter->event_type == CMARK_EVENT_DONE) {
		return NULL;
	}

	/* roll forward to next item, setting both fields */
	if (iter->event_type == CMARK_EVENT_ENTER && !S_is_leaf(cur)) {
		if (cur->first_child == NULL) {
			/* stay on this node but exit */
			iter->event_type = CMARK_EVENT_EXIT;
		} else {
			iter->current = cur->first_child;
			iter->event_type = CMARK_EVENT_ENTER;
		}
	} else if (cur == iter->root) {
		/* don't move past root */
		iter->event_type = CMARK_EVENT_DONE;
		iter->current = NULL;
	} else if (cur->next) {
		iter->event_type = CMARK_EVENT_ENTER;
		iter->current = cur->next;
	} else if (cur->parent) {
		iter->event_type = CMARK_EVENT_EXIT;
		iter->current = cur->parent;
	} else {
		iter->event_type = CMARK_EVENT_DONE;
		iter->current = NULL;
	}

	return cur;
}
