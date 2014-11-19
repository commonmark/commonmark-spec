#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "node.h"

static void
S_node_unlink(cmark_node *node);

cmark_node*
cmark_node_new(cmark_node_type type) {
	cmark_node *node = (cmark_node *)calloc(1, sizeof(*node));
	node->type = type;
	return node;
}

void
cmark_node_destroy(cmark_node *node) {
	S_node_unlink(node);
	node->next = NULL;
	cmark_free_nodes(node);
}

cmark_node_type
cmark_node_get_type(cmark_node *node)
{
	return node->type;
}

static const char*
S_type_string(cmark_node *node)
{
	switch (node->type) {
	case CMARK_NODE_DOCUMENT:      return "DOCUMENT";
	case CMARK_NODE_BQUOTE:        return "BQUOTE";
	case CMARK_NODE_LIST:          return "LIST";
	case CMARK_NODE_LIST_ITEM:     return "LIST_ITEM";
	case CMARK_NODE_FENCED_CODE:   return "FENCED_CODE";
	case CMARK_NODE_INDENTED_CODE: return "INDENTED_CODE";
	case CMARK_NODE_HTML:          return "HTML";
	case CMARK_NODE_PARAGRAPH:     return "PARAGRAPH";
	case CMARK_NODE_ATX_HEADER:    return "ATX_HEADER";
	case CMARK_NODE_SETEXT_HEADER: return "SETEXT_HEADER";
	case CMARK_NODE_HRULE:         return "HRULE";
	case CMARK_NODE_REFERENCE_DEF: return "REFERENCE_DEF";
	case CMARK_NODE_STRING:        return "STRING";
	case CMARK_NODE_SOFTBREAK:     return "SOFTBREAK";
	case CMARK_NODE_LINEBREAK:     return "LINEBREAK";
	case CMARK_NODE_INLINE_CODE:   return "INLINE_CODE";
	case CMARK_NODE_INLINE_HTML:   return "INLINE_HTML";
	case CMARK_NODE_EMPH:          return "EMPH";
	case CMARK_NODE_STRONG:        return "STRONG";
	case CMARK_NODE_LINK:          return "LINK";
	case CMARK_NODE_IMAGE:         return "IMAGE";
	}

	return "<unknown>";
}

cmark_node*
cmark_node_next(cmark_node *node)
{
	return node->next;
}

cmark_node*
cmark_node_previous(cmark_node *node)
{
	return node->prev;
}

cmark_node*
cmark_node_parent(cmark_node *node)
{
	return node->parent;
}

cmark_node*
cmark_node_first_child(cmark_node *node)
{
	return node->first_child;
}

cmark_node*
cmark_node_last_child(cmark_node *node)
{
	return node->last_child;
}

static char*
S_strdup(const char *str) {
	size_t size = strlen(str) + 1;
	char *dup = (char *)malloc(size);
	memcpy(dup, str, size);
	return dup;
}

const char*
cmark_node_get_string_content(cmark_node *node) {
	switch (node->type) {
	case NODE_INDENTED_CODE:
	case NODE_FENCED_CODE:
	case NODE_HTML:
		return cmark_strbuf_cstr(&node->string_content);

	case NODE_STRING:
	case NODE_INLINE_HTML:
	case NODE_INLINE_CODE:
		return cmark_chunk_to_cstr(&node->as.literal);
	default:
		break;
	}

	return NULL;
}

int
cmark_node_set_string_content(cmark_node *node, const char *content) {
	switch (node->type) {
	case NODE_INDENTED_CODE:
	case NODE_FENCED_CODE:
	case NODE_HTML:
		cmark_strbuf_sets(&node->string_content, content);

	case NODE_STRING:
	case NODE_INLINE_HTML:
	case NODE_INLINE_CODE:
		cmark_chunk_set_cstr(&node->as.literal, content);
		return 1;
	default:
		break;
	}

	return 0;
}

int
cmark_node_get_header_level(cmark_node *node) {
	switch (node->type) {
	case CMARK_NODE_ATX_HEADER:
	case CMARK_NODE_SETEXT_HEADER:
		return node->as.header.level;

	default:
		break;
	}

	return 0;
}

int
cmark_node_set_header_level(cmark_node *node, int level) {
	switch (node->type) {
	case CMARK_NODE_ATX_HEADER:
	case CMARK_NODE_SETEXT_HEADER:
		node->as.header.level = level;
		return 1;

	default:
		break;
	}

	return 0;
}

const char*
cmark_node_get_url(cmark_node *node) {
	switch (node->type) {
	case NODE_LINK:
	case NODE_IMAGE:
		return (char *)node->as.link.url;
	default:
		break;
	}

	return NULL;
}

int
cmark_node_set_url(cmark_node *node, const char *url) {
	switch (node->type) {
	case NODE_LINK:
	case NODE_IMAGE:
		free(node->as.link.url);
		node->as.link.url = (unsigned char *)S_strdup(url);
		return 1;
	default:
		break;
	}

	return 0;
}

static inline bool
S_is_block(cmark_node *node) {
	return node->type >= CMARK_NODE_FIRST_BLOCK
	       && node->type <= CMARK_NODE_LAST_BLOCK;
}

static inline bool
S_is_inline(cmark_node *node) {
	return node->type >= CMARK_NODE_FIRST_INLINE
	       && node->type <= CMARK_NODE_LAST_INLINE;
}

static bool
S_can_contain(cmark_node *node, cmark_node *child)
{
	if (child->type == CMARK_NODE_DOCUMENT) {
		return false;
	}

	switch (node->type) {
	case CMARK_NODE_DOCUMENT:
	case CMARK_NODE_BQUOTE:
	case CMARK_NODE_LIST_ITEM:
		return S_is_block(child)
		       && child->type != CMARK_NODE_LIST_ITEM;

	case CMARK_NODE_LIST:
		return child->type == CMARK_NODE_LIST_ITEM;

	case CMARK_NODE_PARAGRAPH:
	case CMARK_NODE_ATX_HEADER:
	case CMARK_NODE_SETEXT_HEADER:
	case CMARK_NODE_EMPH:
	case CMARK_NODE_STRONG:
	case CMARK_NODE_LINK:
	case CMARK_NODE_IMAGE:
		return S_is_inline(child);

	default:
		break;
	}

	return false;
}

// Unlink a node without adjusting its next, prev, and parent pointers.
static void
S_node_unlink(cmark_node *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}

	// Adjust first_child and last_child of parent.
	cmark_node *parent = node->parent;
	if (parent) {
		if (parent->first_child == node) {
			parent->first_child = node->next;
		}
		if (parent->last_child == node) {
			parent->last_child = node->prev;
		}
	}
}

void
cmark_node_unlink(cmark_node *node) {
	S_node_unlink(node);

	node->next   = NULL;
	node->prev   = NULL;
	node->parent = NULL;

}

int
cmark_node_insert_before(cmark_node *node, cmark_node *sibling)
{
	if (!S_can_contain(node->parent, sibling)) {
		return 0;
	}

	S_node_unlink(sibling);

	cmark_node *old_prev = node->prev;

	// Insert 'sibling' between 'old_prev' and 'node'.
	if (old_prev) {
		old_prev->next = sibling;
	}
	sibling->prev = old_prev;
	sibling->next = node;
	node->prev    = sibling;

	// Set new parent.
	cmark_node *parent = node->parent;
	sibling->parent = parent;

	// Adjust first_child of parent if inserted as first child.
	if (parent && !old_prev) {
		parent->first_child = sibling;
	}

	return 1;
}

int
cmark_node_insert_after(cmark_node *node, cmark_node *sibling)
{
	if (!S_can_contain(node->parent, sibling)) {
		return 0;
	}

	S_node_unlink(sibling);

	cmark_node *old_next = node->next;

	// Insert 'sibling' between 'node' and 'old_next'.
	if (old_next) {
		old_next->prev = sibling;
	}
	sibling->next = old_next;
	sibling->prev = node;
	node->next    = sibling;

	// Set new parent.
	cmark_node *parent = node->parent;
	sibling->parent = parent;

	// Adjust last_child of parent if inserted as last child.
	if (parent && !old_next) {
		parent->last_child = sibling;
	}

	return 1;
}

int
cmark_node_prepend_child(cmark_node *node, cmark_node *child)
{
	if (!S_can_contain(node, child)) {
		return 0;
	}

	S_node_unlink(child);

	cmark_node *old_first_child = node->first_child;

	child->next       = old_first_child;
	child->prev       = NULL;
	child->parent     = node;
	node->first_child = child;

	if (old_first_child) {
		old_first_child->prev = child;
	}
	else {
		// Also set last_child if node previously had no children.
		node->last_child = child;
	}

	return 1;
}

int
cmark_node_append_child(cmark_node *node, cmark_node *child)
{
	if (!S_can_contain(node, child)) {
		return 0;
	}

	S_node_unlink(child);

	cmark_node *old_last_child = node->last_child;

	child->next      = NULL;
	child->prev      = old_last_child;
	child->parent    = node;
	node->last_child = child;

	if (old_last_child) {
		old_last_child->next = child;
	}
	else {
		// Also set first_child if node previously had no children.
		node->first_child = child;
	}

	return 1;
}

static void
S_print_error(cmark_node *node, const char *elem)
{
	fprintf(stderr, "Invalid '%s' in node type %s at %d:%d\n", elem,
		S_type_string(node), node->start_line, node->start_column);
}

int
cmark_node_check(cmark_node *node)
{
	cmark_node *cur = node;
	int errors = 0;

	while (cur) {
		if (cur->first_child) {
			if (cur->first_child->parent != cur) {
				S_print_error(cur->first_child, "parent");
				cur->first_child->parent = cur;
				++errors;
			}
			cur = cur->first_child;
		}
		else if (cur->next) {
			if (cur->next->prev != cur) {
				S_print_error(cur->next, "prev");
				cur->next->prev = cur;
				++errors;
			}
			if (cur->next->parent != cur->parent) {
				S_print_error(cur->next, "parent");
				cur->next->parent = cur->parent;
				++errors;
			}
			cur = cur->next;
		}
		else {
			if (cur->parent->last_child != cur) {
				S_print_error(cur->parent, "last_child");
				cur->parent->last_child = cur;
				++errors;
			}

			cmark_node *ancestor = cur->parent;
			cur = NULL;

			while (ancestor != node->parent) {
				if (ancestor->next) {
					cur = ancestor->next;
					break;
				}
				ancestor = ancestor->parent;
			}
		}
	}

	return errors;
}

// Free a cmark_node list and any children.
void cmark_free_nodes(cmark_node *e)
{
	cmark_node *next;
	while (e != NULL) {
		strbuf_free(&e->string_content);
		switch (e->type){
		case NODE_FENCED_CODE:
			strbuf_free(&e->as.code.info);
			break;
		case NODE_STRING:
		case NODE_INLINE_HTML:
		case NODE_INLINE_CODE:
			cmark_chunk_free(&e->as.literal);
			break;
		case NODE_LINK:
		case NODE_IMAGE:
			free(e->as.link.url);
			free(e->as.link.title);
			break;
		default:
			break;
		}
		if (e->last_child) {
			// Splice children into list
			e->last_child->next = e->next;
			e->next = e->first_child;
		}
		next = e->next;
		free(e);
		e = next;
	}
}


