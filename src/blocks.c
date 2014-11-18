#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "config.h"
#include "ast.h"
#include "cmark.h"
#include "node.h"
#include "references.h"
#include "utf8.h"
#include "scanners.h"
#include "inlines.h"
#include "html/houdini.h"
#include "buffer.h"
#include "debug.h"

#define CODE_INDENT 4
#define peek_at(i, n) (i)->data[n]

static cmark_node* make_block(cmark_node_type tag, int start_line, int start_column)
{
	cmark_node* e;

	e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = tag;
		e->open = true;
		e->start_line = start_line;
		e->start_column = start_column;
		e->end_line = start_line;
		strbuf_init(&e->string_content, 32);
	}

	return e;
}

// Create a root document cmark_node.
static cmark_node* make_document()
{
	cmark_node *e = make_block(NODE_DOCUMENT, 1, 1);
	return e;
}

cmark_doc_parser *cmark_new_doc_parser()
{
	cmark_doc_parser *parser = (cmark_doc_parser*)malloc(sizeof(cmark_doc_parser));
	cmark_node *document = make_document();
	strbuf *line = (strbuf*)malloc(sizeof(strbuf));
	cmark_strbuf_init(line, 256);

	parser->refmap = reference_map_new();
	parser->root = document;
	parser->current = document;
	parser->line_number = 0;
	parser->curline = line;

	return parser;
}

void cmark_free_doc_parser(cmark_doc_parser *parser)
{
	cmark_strbuf_free(parser->curline);
	free(parser->curline);
	cmark_reference_map_free(parser->refmap);
	free(parser);
}

static void finalize(cmark_doc_parser *parser, cmark_node* b, int line_number);

// Returns true if line has only space characters, else false.
static bool is_blank(strbuf *s, int offset)
{
	while (offset < s->size) {
		switch (s->ptr[offset]) {
			case '\n':
				return true;
			case ' ':
				offset++;
				break;
			default:
				return false;
		}
	}

	return true;
}

static inline bool can_contain(cmark_node_type parent_type, cmark_node_type child_type)
{
	return ( parent_type == NODE_DOCUMENT ||
			parent_type == NODE_BQUOTE ||
			parent_type == NODE_LIST_ITEM ||
			(parent_type == NODE_LIST && child_type == NODE_LIST_ITEM) );
}

static inline bool accepts_lines(cmark_node_type block_type)
{
	return (block_type == NODE_PARAGRAPH ||
			block_type == NODE_ATX_HEADER ||
			block_type == NODE_INDENTED_CODE ||
			block_type == NODE_FENCED_CODE);
}

static void add_line(cmark_node* cmark_node, chunk *ch, int offset)
{
	assert(cmark_node->open);
	strbuf_put(&cmark_node->string_content, ch->data + offset, ch->len - offset);
}

static void remove_trailing_blank_lines(strbuf *ln)
{
	int i;

	for (i = ln->size - 1; i >= 0; --i) {
		unsigned char c = ln->ptr[i];

		if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			break;
	}

	if (i < 0) {
		strbuf_clear(ln);
		return;
	}

	i = strbuf_strchr(ln, '\n', i);
	if (i >= 0)
		strbuf_truncate(ln, i);
}

// Check to see if a cmark_node ends with a blank line, descending
// if needed into lists and sublists.
static bool ends_with_blank_line(cmark_node* cmark_node)
{
	if (cmark_node->last_line_blank) {
		return true;
	}
	if ((cmark_node->type == NODE_LIST || cmark_node->type == NODE_LIST_ITEM) && cmark_node->last_child) {
		return ends_with_blank_line(cmark_node->last_child);
	} else {
		return false;
	}
}

// Break out of all containing lists
static int break_out_of_lists(cmark_doc_parser *parser, cmark_node ** bptr, int line_number)
{
	cmark_node *container = *bptr;
	cmark_node *b = parser->root;
	// find first containing NODE_LIST:
	while (b && b->type != NODE_LIST) {
		b = b->last_child;
	}
	if (b) {
		while (container && container != b) {
			finalize(parser, container, line_number);
			container = container->parent;
		}
		finalize(parser, b, line_number);
		*bptr = b->parent;
	}
	return 0;
}


static void finalize(cmark_doc_parser *parser, cmark_node* b, int line_number)
{
	int firstlinelen;
	int pos;
	cmark_node* item;
	cmark_node* subitem;

	if (!b->open)
		return; // don't do anything if the cmark_node is already closed

	b->open = false;
	if (line_number > b->start_line) {
		b->end_line = line_number - 1;
	} else {
		b->end_line = line_number;
	}

	switch (b->type) {
		case NODE_PARAGRAPH:
			pos = 0;
			while (strbuf_at(&b->string_content, 0) == '[' &&
					(pos = parse_reference_inline(&b->string_content, parser->refmap))) {

				strbuf_drop(&b->string_content, pos);
			}
			if (is_blank(&b->string_content, 0)) {
				b->type = NODE_REFERENCE_DEF;
			}
			break;

		case NODE_INDENTED_CODE:
			remove_trailing_blank_lines(&b->string_content);
			strbuf_putc(&b->string_content, '\n');
			break;

		case NODE_FENCED_CODE:
			// first line of contents becomes info
			firstlinelen = strbuf_strchr(&b->string_content, '\n', 0);

			strbuf_init(&b->as.code.info, 0);
			houdini_unescape_html_f(
					&b->as.code.info,
					b->string_content.ptr,
					firstlinelen
					);

			strbuf_drop(&b->string_content, firstlinelen + 1);

			strbuf_trim(&b->as.code.info);
			strbuf_unescape(&b->as.code.info);
			break;

		case NODE_LIST: // determine tight/loose status
			b->as.list.tight = true; // tight by default
			item = b->first_child;

			while (item) {
				// check for non-final non-empty list item ending with blank line:
				if (item->last_line_blank && item->next) {
					b->as.list.tight = false;
					break;
				}
				// recurse into children of list item, to see if there are
				// spaces between them:
				subitem = item->first_child;
				while (subitem) {
					if (ends_with_blank_line(subitem) &&
							(item->next || subitem->next)) {
						b->as.list.tight = false;
						break;
					}
					subitem = subitem->next;
				}
				if (!(b->as.list.tight)) {
					break;
				}
				item = item->next;
			}

			break;

		default:
			break;
	}
}

// Add a cmark_node as child of another.  Return pointer to child.
static cmark_node* add_child(cmark_doc_parser *parser, cmark_node* parent,
		cmark_node_type block_type, int start_line, int start_column)
{
	assert(parent);

	// if 'parent' isn't the kind of cmark_node that can accept this child,
	// then back up til we hit a cmark_node that can.
	while (!can_contain(parent->type, block_type)) {
		finalize(parser, parent, start_line);
		parent = parent->parent;
	}

	cmark_node* child = make_block(block_type, start_line, start_column);
	child->parent = parent;

	if (parent->last_child) {
		parent->last_child->next = child;
		child->prev = parent->last_child;
	} else {
		parent->first_child = child;
		child->prev = NULL;
	}
	parent->last_child = child;
	return child;
}


typedef struct BlockStack {
	struct BlockStack *previous;
	cmark_node *next_sibling;
} block_stack;

// Walk through cmark_node and all children, recursively, parsing
// string content into inline content where appropriate.
static void process_inlines(cmark_node* cur, reference_map *refmap)
{
	block_stack* stack = NULL;
	block_stack* newstack = NULL;

	while (cur != NULL) {
		switch (cur->type) {
			case NODE_PARAGRAPH:
			case NODE_ATX_HEADER:
			case NODE_SETEXT_HEADER:
				cur->inline_content = parse_inlines(&cur->string_content, refmap);
				break;

			default:
				break;
		}

		if (cur->first_child) {
			newstack = (block_stack*)malloc(sizeof(block_stack));
			if (newstack == NULL) return;
			newstack->previous = stack;
			stack = newstack;
			stack->next_sibling = cur->next;
			cur = cur->first_child;
		} else {
			cur = cur->next;
		}

		while (cur == NULL && stack != NULL) {
			cur = stack->next_sibling;
			newstack = stack->previous;
			free(stack);
			stack = newstack;
		}
	}
	while (stack != NULL) {
		newstack = stack->previous;
		free(stack);
		stack = newstack;
	}
}

// Attempts to parse a list item marker (bullet or enumerated).
// On success, returns length of the marker, and populates
// data with the details.  On failure, returns 0.
static int parse_list_marker(chunk *input, int pos, cmark_list **dataptr)
{
	unsigned char c;
	int startpos;
	cmark_list *data;

	startpos = pos;
	c = peek_at(input, pos);

	if ((c == '*' || c == '-' || c == '+') && !scan_hrule(input, pos)) {
		pos++;
		if (!isspace(peek_at(input, pos))) {
			return 0;
		}
		data = (cmark_list *)calloc(1, sizeof(*data));
		if(data == NULL) {
			return 0;
		} else {
			data->marker_offset = 0; // will be adjusted later
			data->list_type = CMARK_BULLET_LIST;
			data->bullet_char = c;
			data->start = 1;
			data->delimiter = CMARK_PERIOD_DELIM;
			data->tight = false;
		}
	} else if (isdigit(c)) {
		int start = 0;

		do {
			start = (10 * start) + (peek_at(input, pos) - '0');
			pos++;
		} while (isdigit(peek_at(input, pos)));

		c = peek_at(input, pos);
		if (c == '.' || c == ')') {
			pos++;
			if (!isspace(peek_at(input, pos))) {
				return 0;
			}
			data = (cmark_list *)calloc(1, sizeof(*data));
			if(data == NULL) {
				return 0;
			} else {
				data->marker_offset = 0; // will be adjusted later
				data->list_type = CMARK_ORDERED_LIST;
				data->bullet_char = 0;
				data->start = start;
				data->delimiter = (c == '.' ? CMARK_PERIOD_DELIM : CMARK_PAREN_DELIM);
				data->tight = false;
			}
		} else {
			return 0;
		}

	} else {
		return 0;
	}

	*dataptr = data;
	return (pos - startpos);
}

// Return 1 if list item belongs in list, else 0.
static int lists_match(cmark_list *list_data, cmark_list *item_data)
{
	return (list_data->list_type == item_data->list_type &&
			list_data->delimiter == item_data->delimiter &&
			// list_data->marker_offset == item_data.marker_offset &&
			list_data->bullet_char == item_data->bullet_char);
}

static cmark_node *finalize_document(cmark_doc_parser *parser)
{
	while (parser->current != parser->root) {
		finalize(parser, parser->current, parser->line_number);
		parser->current = parser->current->parent;
	}

	finalize(parser, parser->root, parser->line_number);
	process_inlines(parser->root, parser->refmap);

	return parser->root;
}

extern cmark_node *cmark_parse_file(FILE *f)
{
	unsigned char buffer[4096];
	cmark_doc_parser *parser = cmark_new_doc_parser();
	size_t offset;
	cmark_node *document;

	while (fgets((char *)buffer, sizeof(buffer), f)) {
		offset = strlen((char *)buffer);
		cmark_process_line(parser, buffer, offset);
	}

	document = cmark_finish(parser);
	cmark_free_doc_parser(parser);
	return document;
}

extern cmark_node *cmark_parse_document(const unsigned char *buffer, size_t len)
{
	int linenum = 1;
	const unsigned char *end = buffer + len;
	size_t offset;
	cmark_doc_parser *parser = cmark_new_doc_parser();
	cmark_node *document;

	while (buffer < end) {
		const unsigned char *eol = (unsigned char *)memchr(buffer, '\n', end - buffer);
		offset = eol ? (eol - buffer) + 1 : eol - buffer;
		cmark_process_line(parser, buffer, offset);
		buffer += offset;
		linenum++;
	}

	document = cmark_finish(parser);
	cmark_free_doc_parser(parser);
	return document;
}

static void chop_trailing_hashtags(chunk *ch)
{
	int n, orig_n;

	chunk_rtrim(ch);
	orig_n = n = ch->len - 1;

	// if string ends in space followed by #s, remove these:
	while (n >= 0 && peek_at(ch, n) == '#')
		n--;

	// Check for a be a space before the final #s:
	if (n != orig_n && n >= 0 && peek_at(ch, n) == ' ') {
		ch->len = n;
		chunk_rtrim(ch);
	}
}

void cmark_process_line(cmark_doc_parser *parser, const unsigned char *buffer,
		 size_t bytes)
{
	cmark_node* last_matched_container;
	int offset = 0;
	int matched = 0;
	int lev = 0;
	int i;
	cmark_list *data = NULL;
	bool all_matched = true;
	cmark_node* container;
	cmark_node* cur = parser->current;
	bool blank = false;
	int first_nonspace;
	int indent;
	chunk input;

	utf8proc_detab(parser->curline, buffer, bytes);

	// Add a newline to the end if not present:
	// TODO this breaks abstraction:
	if (parser->curline->ptr[parser->curline->size - 1] != '\n') {
		strbuf_putc(parser->curline, '\n');
	}
	input.data = parser->curline->ptr;
	input.len = parser->curline->size;

	// container starts at the document root.
	container = parser->root;

	parser->line_number++;

	// for each containing cmark_node, try to parse the associated line start.
	// bail out on failure:  container will point to the last matching cmark_node.

	while (container->last_child && container->last_child->open) {
		container = container->last_child;

		first_nonspace = offset;
		while (peek_at(&input, first_nonspace) == ' ') {
			first_nonspace++;
		}

		indent = first_nonspace - offset;
		blank = peek_at(&input, first_nonspace) == '\n';

		if (container->type == NODE_BQUOTE) {
			matched = indent <= 3 && peek_at(&input, first_nonspace) == '>';
			if (matched) {
				offset = first_nonspace + 1;
				if (peek_at(&input, offset) == ' ')
					offset++;
			} else {
				all_matched = false;
			}

		} else if (container->type == NODE_LIST_ITEM) {

			if (indent >= container->as.list.marker_offset +
					container->as.list.padding) {
				offset += container->as.list.marker_offset +
					container->as.list.padding;
			} else if (blank) {
				offset = first_nonspace;
			} else {
				all_matched = false;
			}

		} else if (container->type == NODE_INDENTED_CODE) {

			if (indent >= CODE_INDENT) {
				offset += CODE_INDENT;
			} else if (blank) {
				offset = first_nonspace;
			} else {
				all_matched = false;
			}

		} else if (container->type == NODE_ATX_HEADER ||
				container->type == NODE_SETEXT_HEADER) {

			// a header can never contain more than one line
			all_matched = false;

		} else if (container->type == NODE_FENCED_CODE) {

			// skip optional spaces of fence offset
			i = container->as.code.fence_offset;
			while (i > 0 && peek_at(&input, offset) == ' ') {
				offset++;
				i--;
			}

		} else if (container->type == NODE_HTML) {

			if (blank) {
				all_matched = false;
			}

		} else if (container->type == NODE_PARAGRAPH) {

			if (blank) {
				container->last_line_blank = true;
				all_matched = false;
			}

		}

		if (!all_matched) {
			container = container->parent;  // back up to last matching cmark_node
			break;
		}
	}

	last_matched_container = container;

	// check to see if we've hit 2nd blank line, break out of list:
	if (blank && container->last_line_blank) {
		break_out_of_lists(parser, &container, parser->line_number);
	}

	// unless last matched container is code cmark_node, try new container starts:
	while (container->type != NODE_FENCED_CODE && container->type != NODE_INDENTED_CODE &&
			container->type != NODE_HTML) {

		first_nonspace = offset;
		while (peek_at(&input, first_nonspace) == ' ')
			first_nonspace++;

		indent = first_nonspace - offset;
		blank = peek_at(&input, first_nonspace) == '\n';

		if (indent >= CODE_INDENT) {
			if (cur->type != NODE_PARAGRAPH && !blank) {
				offset += CODE_INDENT;
				container = add_child(parser, container, NODE_INDENTED_CODE, parser->line_number, offset + 1);
			} else { // indent > 4 in lazy line
				break;
			}

		} else if (peek_at(&input, first_nonspace) == '>') {

			offset = first_nonspace + 1;
			// optional following character
			if (peek_at(&input, offset) == ' ')
				offset++;
			container = add_child(parser, container, NODE_BQUOTE, parser->line_number, offset + 1);

		} else if ((matched = scan_atx_header_start(&input, first_nonspace))) {

			offset = first_nonspace + matched;
			container = add_child(parser, container, NODE_ATX_HEADER, parser->line_number, offset + 1);

			int hashpos = chunk_strchr(&input, '#', first_nonspace);
			int level = 0;

			while (peek_at(&input, hashpos) == '#') {
				level++;
				hashpos++;
			}
			container->as.header.level = level;

		} else if ((matched = scan_open_code_fence(&input, first_nonspace))) {

			container = add_child(parser, container, NODE_FENCED_CODE, parser->line_number, first_nonspace + 1);
			container->as.code.fence_char = peek_at(&input, first_nonspace);
			container->as.code.fence_length = matched;
			container->as.code.fence_offset = first_nonspace - offset;
			offset = first_nonspace + matched;

		} else if ((matched = scan_html_block_tag(&input, first_nonspace))) {

			container = add_child(parser, container, NODE_HTML, parser->line_number, first_nonspace + 1);
			// note, we don't adjust offset because the tag is part of the text

		} else if (container->type == NODE_PARAGRAPH &&
				(lev = scan_setext_header_line(&input, first_nonspace)) &&
				// check that there is only one line in the paragraph:
				strbuf_strrchr(&container->string_content, '\n',
					strbuf_len(&container->string_content) - 2) < 0) {

			container->type = NODE_SETEXT_HEADER;
			container->as.header.level = lev;
			offset = input.len - 1;

		} else if (!(container->type == NODE_PARAGRAPH && !all_matched) &&
				(matched = scan_hrule(&input, first_nonspace))) {

			// it's only now that we know the line is not part of a setext header:
			container = add_child(parser, container, NODE_HRULE, parser->line_number, first_nonspace + 1);
			finalize(parser, container, parser->line_number);
			container = container->parent;
			offset = input.len - 1;

		} else if ((matched = parse_list_marker(&input, first_nonspace, &data))) {

			// compute padding:
			offset = first_nonspace + matched;
			i = 0;
			while (i <= 5 && peek_at(&input, offset + i) == ' ') {
				i++;
			}
			// i = number of spaces after marker, up to 5
			if (i >= 5 || i < 1 || peek_at(&input, offset) == '\n') {
				data->padding = matched + 1;
				if (i > 0) {
					offset += 1;
				}
			} else {
				data->padding = matched + i;
				offset += i;
			}

			// check container; if it's a list, see if this list item
			// can continue the list; otherwise, create a list container.

			data->marker_offset = indent;

			if (container->type != NODE_LIST ||
					!lists_match(&container->as.list, data)) {
				container = add_child(parser, container, NODE_LIST, parser->line_number,
						first_nonspace + 1);

				memcpy(&container->as.list, data, sizeof(*data));
			}

			// add the list item
			container = add_child(parser, container, NODE_LIST_ITEM, parser->line_number,
					first_nonspace + 1);
			/* TODO: static */
			memcpy(&container->as.list, data, sizeof(*data));
			free(data);
		} else {
			break;
		}

		if (accepts_lines(container->type)) {
			// if it's a line container, it can't contain other containers
			break;
		}
	}

	// what remains at offset is a text line.  add the text to the
	// appropriate container.

	first_nonspace = offset;
	while (peek_at(&input, first_nonspace) == ' ')
		first_nonspace++;

	indent = first_nonspace - offset;
	blank = peek_at(&input, first_nonspace) == '\n';

	// cmark_node quote lines are never blank as they start with >
	// and we don't count blanks in fenced code for purposes of tight/loose
	// lists or breaking out of lists.  we also don't set last_line_blank
	// on an empty list item.
	container->last_line_blank = (blank &&
			container->type != NODE_BQUOTE &&
			container->type != NODE_FENCED_CODE &&
			!(container->type == NODE_LIST_ITEM &&
				container->first_child == NULL &&
				container->start_line == parser->line_number));

	cmark_node *cont = container;
	while (cont->parent) {
		cont->parent->last_line_blank = false;
		cont = cont->parent;
	}

	if (cur != last_matched_container &&
			container == last_matched_container &&
			!blank &&
			cur->type == NODE_PARAGRAPH &&
			strbuf_len(&cur->string_content) > 0) {

		add_line(cur, &input, offset);

	} else { // not a lazy continuation

		// finalize any blocks that were not matched and set cur to container:
		while (cur != last_matched_container) {
			finalize(parser, cur, parser->line_number);
			cur = cur->parent;
			assert(cur != NULL);
		}

		if (container->type == NODE_INDENTED_CODE) {

			add_line(container, &input, offset);

		} else if (container->type == NODE_FENCED_CODE) {
			matched = 0;

			if (indent <= 3 &&
					peek_at(&input, first_nonspace) == container->as.code.fence_char) {
				int fence_len = scan_close_code_fence(&input, first_nonspace);
				if (fence_len > container->as.code.fence_length)
					matched = 1;
			}

			if (matched) {
				// if closing fence, don't add line to container; instead, close it:
				finalize(parser, container, parser->line_number);
				container = container->parent; // back up to parent
			} else {
				add_line(container, &input, offset);
			}

		} else if (container->type == NODE_HTML) {

			add_line(container, &input, offset);

		} else if (blank) {

			// ??? do nothing

		} else if (container->type == NODE_ATX_HEADER) {

			chop_trailing_hashtags(&input);
			add_line(container, &input, first_nonspace);
			finalize(parser, container, parser->line_number);
			container = container->parent;

		} else if (accepts_lines(container->type)) {

			add_line(container, &input, first_nonspace);

		} else if (container->type != NODE_HRULE && container->type != NODE_SETEXT_HEADER) {

			// create paragraph container for line
			container = add_child(parser, container, NODE_PARAGRAPH, parser->line_number, first_nonspace + 1);
			add_line(container, &input, first_nonspace);

		} else {
			assert(false);
		}

		parser->current = container;
	}
	strbuf_clear(parser->curline);

}

cmark_node *cmark_finish(cmark_doc_parser *parser)
{
	finalize_document(parser);
	strbuf_free(parser->curline);
	return parser->root;
}

