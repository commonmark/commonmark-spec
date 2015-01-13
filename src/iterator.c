#include <assert.h>
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
	if (root == NULL) {
		return NULL;
	}
	cmark_iter *iter = (cmark_iter*)malloc(sizeof(cmark_iter));
	if (iter == NULL) {
		return NULL;
	}
	iter->root         = root;
	iter->cur.ev_type  = CMARK_EVENT_NONE;
	iter->cur.node     = NULL;
	iter->next.ev_type = CMARK_EVENT_ENTER;
	iter->next.node    = root;
	return iter;
}

void
cmark_iter_free(cmark_iter *iter)
{
	free(iter);
}

static bool
S_is_leaf(cmark_node *node)
{
	return (1 << node->type) & S_leaf_mask;
}

cmark_event_type
cmark_iter_next(cmark_iter *iter)
{
	cmark_event_type  ev_type = iter->next.ev_type;
	cmark_node       *node    = iter->next.node;

	iter->cur.ev_type = ev_type;
	iter->cur.node    = node;

	if (ev_type == CMARK_EVENT_DONE) {
		return ev_type;
	}

	/* roll forward to next item, setting both fields */
	if (ev_type == CMARK_EVENT_ENTER && !S_is_leaf(node)) {
		if (node->first_child == NULL) {
			/* stay on this node but exit */
			iter->next.ev_type = CMARK_EVENT_EXIT;
		} else {
			iter->next.ev_type = CMARK_EVENT_ENTER;
			iter->next.node    = node->first_child;
		}
	} else if (node == iter->root) {
		/* don't move past root */
		iter->next.ev_type = CMARK_EVENT_DONE;
		iter->next.node    = NULL;
	} else if (node->next) {
		iter->next.ev_type = CMARK_EVENT_ENTER;
		iter->next.node    = node->next;
	} else if (node->parent) {
		iter->next.ev_type = CMARK_EVENT_EXIT;
		iter->next.node    = node->parent;
	} else {
		assert(false);
		iter->next.ev_type = CMARK_EVENT_DONE;
		iter->next.node    = NULL;
	}

	return ev_type;
}

void
cmark_iter_reset(cmark_iter *iter, cmark_node *current,
                 cmark_event_type event_type)
{
	iter->next.ev_type = event_type;
	iter->next.node    = current;
	cmark_iter_next(iter);
}

cmark_node*
cmark_iter_get_node(cmark_iter *iter)
{
	return iter->cur.node;
}

cmark_event_type
cmark_iter_get_event_type(cmark_iter *iter)
{
	return iter->cur.ev_type;
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
