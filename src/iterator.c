#include <stdlib.h>

#include "config.h"
#include "node.h"
#include "cmark.h"
#include "iterator.h"

static const int S_leaf_mask =
	(1 << CMARK_NODE_HTML)        |
	(1 << CMARK_NODE_HRULE)       |
	(1 << CMARK_NODE_CODE_BLOCK)  |
	(1 << CMARK_NODE_TEXT)        |
	(1 << CMARK_NODE_SOFTBREAK)   |
	(1 << CMARK_NODE_LINEBREAK)   |
	(1 << CMARK_NODE_CODE)        |
	(1 << CMARK_NODE_INLINE_HTML);

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

static bool
S_is_leaf(cmark_node *node)
{
	return (1 << node->type) & S_leaf_mask;
}

void
cmark_iter_reset(cmark_iter *iter, cmark_node *current,
                 cmark_event_type event_type)
{
	iter->event_type = event_type;
	iter->current = current;
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


void cmark_consolidate_text_nodes(cmark_node *root)
{
	cmark_iter *iter = cmark_iter_new(root);
	cmark_strbuf buf = GH_BUF_INIT;
	cmark_event_type ev_type;
	cmark_node *cur, *tmp, *next;

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cur = cmark_iter_get_node(iter);
		if (ev_type == CMARK_EVENT_ENTER &&
		    cur->type == CMARK_NODE_TEXT &&
		    cur->next &&
		    cur->next->type == CMARK_NODE_TEXT) {
			cmark_strbuf_clear(&buf);
			cmark_strbuf_puts(&buf, cmark_node_get_literal(cur));
			tmp = cur->next;
			while (tmp && tmp->type == CMARK_NODE_TEXT) {
				cmark_iter_get_node(iter); // advance pointer
				cmark_strbuf_puts(&buf, cmark_node_get_literal(tmp));
				next = tmp->next;
				cmark_node_free(tmp);
				tmp = next;
			}
			cmark_node_set_literal(cur, (char *)cmark_strbuf_detach(&buf));
		}
	}

	cmark_iter_free(iter);
}
