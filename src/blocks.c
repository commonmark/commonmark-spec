#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "cmark.h"
#include "utf8.h"
#include "scanners.h"
#include "inlines.h"
#include "html/houdini.h"

#define peek_at(i, n) (i)->data[n]

static void incorporate_line(strbuf *ln, int line_number, node_block** curptr);
static void finalize(node_block* b, int line_number);

static node_block* make_block(int tag, int start_line, int start_column)
{
	node_block* e;

	e = calloc(1, sizeof(*e));
    if(e != NULL) {
    	e->tag = tag;
	    e->open = true;
    	e->start_line = start_line;
	    e->start_column = start_column;
    	e->end_line = start_line;
	    strbuf_init(&e->string_content, 32);
    }

	return e;
}

// Create a root document node_block.
extern node_block* make_document()
{
	node_block *e = make_block(BLOCK_DOCUMENT, 1, 1);
	e->as.document.refmap = reference_map_new();
	e->top = e;

	return e;
}

// Returns true if line has only space characters, else false.
bool is_blank(strbuf *s, int offset)
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

static inline bool can_contain(int parent_type, int child_type)
{
	return ( parent_type == BLOCK_DOCUMENT ||
		 parent_type == BLOCK_BQUOTE ||
		 parent_type == BLOCK_LIST_ITEM ||
		 (parent_type == BLOCK_LIST && child_type == BLOCK_LIST_ITEM) );
}

static inline bool accepts_lines(int block_type)
{
	return (block_type == BLOCK_PARAGRAPH ||
		block_type == BLOCK_ATX_HEADER ||
		block_type == BLOCK_INDENTED_CODE ||
		block_type == BLOCK_FENCED_CODE);
}

static void add_line(node_block* node_block, chunk *ch, int offset)
{
	assert(node_block->open);
	strbuf_put(&node_block->string_content, ch->data + offset, ch->len - offset);
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

// Check to see if a node_block ends with a blank line, descending
// if needed into lists and sublists.
static bool ends_with_blank_line(node_block* node_block)
{
	if (node_block->last_line_blank) {
		return true;
	}
	if ((node_block->tag == BLOCK_LIST || node_block->tag == BLOCK_LIST_ITEM) && node_block->last_child) {
		return ends_with_blank_line(node_block->last_child);
	} else {
		return false;
	}
}

// Break out of all containing lists
static int break_out_of_lists(node_block ** bptr, int line_number)
{
	node_block *container = *bptr;
	node_block *b = container->top;
	// find first containing BLOCK_LIST:
	while (b && b->tag != BLOCK_LIST) {
		b = b->last_child;
	}
	if (b) {
		while (container && container != b) {
			finalize(container, line_number);
			container = container->parent;
		}
		finalize(b, line_number);
		*bptr = b->parent;
	}
	return 0;
}


static void finalize(node_block* b, int line_number)
{
	int firstlinelen;
	int pos;
	node_block* item;
	node_block* subitem;

	if (!b->open)
		return; // don't do anything if the node_block is already closed

	b->open = false;
	if (line_number > b->start_line) {
		b->end_line = line_number - 1;
	} else {
		b->end_line = line_number;
	}

	switch (b->tag) {
	case BLOCK_PARAGRAPH:
		pos = 0;
		while (strbuf_at(&b->string_content, 0) == '[' &&
		       (pos = parse_reference_inline(&b->string_content, b->top->as.document.refmap))) {

			strbuf_drop(&b->string_content, pos);
		}
		if (is_blank(&b->string_content, 0)) {
			b->tag = BLOCK_REFERENCE_DEF;
		}
		break;

	case BLOCK_INDENTED_CODE:
		remove_trailing_blank_lines(&b->string_content);
		strbuf_putc(&b->string_content, '\n');
		break;

	case BLOCK_FENCED_CODE:
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

	case BLOCK_LIST: // determine tight/loose status
		b->as.list.tight = true; // tight by default
		item = b->children;

		while (item) {
			// check for non-final non-empty list item ending with blank line:
			if (item->last_line_blank && item->next) {
				b->as.list.tight = false;
				break;
			}
			// recurse into children of list item, to see if there are
			// spaces between them:
			subitem = item->children;
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

// Add a node_block as child of another.  Return pointer to child.
static node_block* add_child(node_block* parent,
			     int block_type, int start_line, int start_column)
{
	assert(parent);

	// if 'parent' isn't the kind of node_block that can accept this child,
	// then back up til we hit a node_block that can.
	while (!can_contain(parent->tag, block_type)) {
		finalize(parent, start_line);
		parent = parent->parent;
	}

	node_block* child = make_block(block_type, start_line, start_column);
	child->parent = parent;
	child->top = parent->top;

	if (parent->last_child) {
		parent->last_child->next = child;
		child->prev = parent->last_child;
	} else {
		parent->children = child;
		child->prev = NULL;
	}
	parent->last_child = child;
	return child;
}


// Free a node_block list and any children.
void cmark_free_nodes(node_block *e)
{
	node_block * next;
	while (e != NULL) {
		free_inlines(e->inline_content);
		strbuf_free(&e->string_content);
		if (e->tag == BLOCK_FENCED_CODE) {
			strbuf_free(&e->as.code.info);
		} else if (e->tag == BLOCK_DOCUMENT) {
			reference_map_free(e->as.document.refmap);
		}
		if (e->last_child) {
		    // Splice children into list
		    e->last_child->next = e->next;
		    e->next = e->children;
		}
		next = e->next;
		free(e);
		e = next;
	}
}

// Walk through node_block and all children, recursively, parsing
// string content into inline content where appropriate.
void process_inlines(node_block* cur, reference_map *refmap)
{
	switch (cur->tag) {
	case BLOCK_PARAGRAPH:
	case BLOCK_ATX_HEADER:
	case BLOCK_SETEXT_HEADER:
		cur->inline_content = parse_inlines(&cur->string_content, refmap);
		break;

	default:
		break;
	}

	node_block *child = cur->children;
	while (child != NULL) {
		process_inlines(child, refmap);
		child = child->next;
	}
}

// Attempts to parse a list item marker (bullet or enumerated).
// On success, returns length of the marker, and populates
// data with the details.  On failure, returns 0.
static int parse_list_marker(chunk *input, int pos, struct ListData ** dataptr)
{
	unsigned char c;
	int startpos;
	struct ListData * data;

	startpos = pos;
	c = peek_at(input, pos);

	if ((c == '*' || c == '-' || c == '+') && !scan_hrule(input, pos)) {
		pos++;
		if (!isspace(peek_at(input, pos))) {
			return 0;
		}
		data = calloc(1, sizeof(*data));
        if(data == NULL) {
            return 0;
        } else {
    		data->marker_offset = 0; // will be adjusted later
	    	data->list_type = bullet;
		    data->bullet_char = c;
    		data->start = 1;
	    	data->delimiter = period;
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
			data = calloc(1, sizeof(*data));
            if(data == NULL) {
                return 0;
            } else {
    			data->marker_offset = 0; // will be adjusted later
	    		data->list_type = ordered;
		    	data->bullet_char = 0;
			    data->start = start;
    			data->delimiter = (c == '.' ? period : parens);
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
static int lists_match(struct ListData *list_data, struct ListData *item_data)
{
	return (list_data->list_type == item_data->list_type &&
		list_data->delimiter == item_data->delimiter &&
		// list_data->marker_offset == item_data.marker_offset &&
		list_data->bullet_char == item_data->bullet_char);
}

static node_block *finalize_document(node_block *document, int linenum)
{
	while (document != document->top) {
		finalize(document, linenum);
		document = document->parent;
	}

	finalize(document, linenum);
	process_inlines(document, document->as.document.refmap);

	return document;
}

extern node_block *cmark_parse_file(FILE *f)
{
	strbuf line = GH_BUF_INIT;
	unsigned char buffer[4096];
	int linenum = 1;
	node_block *document = make_document();

	while (fgets((char *)buffer, sizeof(buffer), f)) {
		utf8proc_detab(&line, buffer, strlen((char *)buffer));
		incorporate_line(&line, linenum, &document);
		strbuf_clear(&line);
		linenum++;
	}

	strbuf_free(&line);
	return finalize_document(document, linenum);
}

extern node_block *cmark_parse_document(const unsigned char *buffer, size_t len)
{
	strbuf line = GH_BUF_INIT;
	int linenum = 1;
	const unsigned char *end = buffer + len;
	node_block *document = make_document();

	while (buffer < end) {
		const unsigned char *eol = memchr(buffer, '\n', end - buffer);

		if (!eol) {
			utf8proc_detab(&line, buffer, end - buffer);
			buffer = end;
		} else {
			utf8proc_detab(&line, buffer, (eol - buffer) + 1);
			buffer += (eol - buffer) + 1;
		}

		incorporate_line(&line, linenum, &document);
		strbuf_clear(&line);
		linenum++;
	}

	strbuf_free(&line);
	return finalize_document(document, linenum);
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

// Process one line at a time, modifying a node_block.
static void incorporate_line(strbuf *line, int line_number, node_block** curptr)
{
	node_block* last_matched_container;
	int offset = 0;
	int matched = 0;
	int lev = 0;
	int i;
	struct ListData * data = NULL;
	bool all_matched = true;
	node_block* container;
	node_block* cur = *curptr;
	bool blank = false;
	int first_nonspace;
	int indent;
	chunk input;

	// Add a newline to the end if not present:
	if (line->ptr[line->size - 1] != '\n') {
	    strbuf_putc(line, '\n');
	}
	input.data = line->ptr;
	input.len = line->size;

	// container starts at the document root.
	container = cur->top;

	// for each containing node_block, try to parse the associated line start.
	// bail out on failure:  container will point to the last matching node_block.

	while (container->last_child && container->last_child->open) {
		container = container->last_child;

		first_nonspace = offset;
		while (peek_at(&input, first_nonspace) == ' ') {
			first_nonspace++;
		}

		indent = first_nonspace - offset;
		blank = peek_at(&input, first_nonspace) == '\n';

		if (container->tag == BLOCK_BQUOTE) {
			matched = indent <= 3 && peek_at(&input, first_nonspace) == '>';
			if (matched) {
				offset = first_nonspace + 1;
				if (peek_at(&input, offset) == ' ')
					offset++;
			} else {
				all_matched = false;
			}

		} else if (container->tag == BLOCK_LIST_ITEM) {

			if (indent >= container->as.list.marker_offset +
			    container->as.list.padding) {
				offset += container->as.list.marker_offset +
					container->as.list.padding;
			} else if (blank) {
				offset = first_nonspace;
			} else {
				all_matched = false;
			}

		} else if (container->tag == BLOCK_INDENTED_CODE) {

			if (indent >= CODE_INDENT) {
				offset += CODE_INDENT;
			} else if (blank) {
				offset = first_nonspace;
			} else {
				all_matched = false;
			}

		} else if (container->tag == BLOCK_ATX_HEADER ||
			   container->tag == BLOCK_SETEXT_HEADER) {

			// a header can never contain more than one line
			all_matched = false;

		} else if (container->tag == BLOCK_FENCED_CODE) {

			// skip optional spaces of fence offset
			i = container->as.code.fence_offset;
			while (i > 0 && peek_at(&input, offset) == ' ') {
				offset++;
				i--;
			}

		} else if (container->tag == BLOCK_HTML) {

			if (blank) {
				all_matched = false;
			}

		} else if (container->tag == BLOCK_PARAGRAPH) {

			if (blank) {
				container->last_line_blank = true;
				all_matched = false;
			}

		}

		if (!all_matched) {
			container = container->parent;  // back up to last matching node_block
			break;
		}
	}

	last_matched_container = container;

	// check to see if we've hit 2nd blank line, break out of list:
	if (blank && container->last_line_blank) {
		break_out_of_lists(&container, line_number);
	}

	// unless last matched container is code node_block, try new container starts:
	while (container->tag != BLOCK_FENCED_CODE && container->tag != BLOCK_INDENTED_CODE &&
	       container->tag != BLOCK_HTML) {

		first_nonspace = offset;
		while (peek_at(&input, first_nonspace) == ' ')
			first_nonspace++;

		indent = first_nonspace - offset;
		blank = peek_at(&input, first_nonspace) == '\n';

		if (indent >= CODE_INDENT) {
			if (cur->tag != BLOCK_PARAGRAPH && !blank) {
				offset += CODE_INDENT;
				container = add_child(container, BLOCK_INDENTED_CODE, line_number, offset + 1);
			} else { // indent > 4 in lazy line
				break;
			}

		} else if (peek_at(&input, first_nonspace) == '>') {

			offset = first_nonspace + 1;
			// optional following character
			if (peek_at(&input, offset) == ' ')
				offset++;
			container = add_child(container, BLOCK_BQUOTE, line_number, offset + 1);

		} else if ((matched = scan_atx_header_start(&input, first_nonspace))) {

			offset = first_nonspace + matched;
			container = add_child(container, BLOCK_ATX_HEADER, line_number, offset + 1);

			int hashpos = chunk_strchr(&input, '#', first_nonspace);
			int level = 0;

			while (peek_at(&input, hashpos) == '#') {
				level++;
				hashpos++;
			}
			container->as.header.level = level;

		} else if ((matched = scan_open_code_fence(&input, first_nonspace))) {

			container = add_child(container, BLOCK_FENCED_CODE, line_number, first_nonspace + 1);
			container->as.code.fence_char = peek_at(&input, first_nonspace);
			container->as.code.fence_length = matched;
			container->as.code.fence_offset = first_nonspace - offset;
			offset = first_nonspace + matched;

		} else if ((matched = scan_html_block_tag(&input, first_nonspace))) {

			container = add_child(container, BLOCK_HTML, line_number, first_nonspace + 1);
			// note, we don't adjust offset because the tag is part of the text

		} else if (container->tag == BLOCK_PARAGRAPH &&
			   (lev = scan_setext_header_line(&input, first_nonspace)) &&
			   // check that there is only one line in the paragraph:
			   strbuf_strrchr(&container->string_content, '\n',
					  strbuf_len(&container->string_content) - 2) < 0) {

			container->tag = BLOCK_SETEXT_HEADER;
			container->as.header.level = lev;
			offset = input.len - 1;

		} else if (!(container->tag == BLOCK_PARAGRAPH && !all_matched) &&
			   (matched = scan_hrule(&input, first_nonspace))) {

			// it's only now that we know the line is not part of a setext header:
			container = add_child(container, BLOCK_HRULE, line_number, first_nonspace + 1);
			finalize(container, line_number);
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

			if (container->tag != BLOCK_LIST ||
			    !lists_match(&container->as.list, data)) {
				container = add_child(container, BLOCK_LIST, line_number,
						      first_nonspace + 1);

				memcpy(&container->as.list, data, sizeof(*data));
			}

			// add the list item
			container = add_child(container, BLOCK_LIST_ITEM, line_number,
					      first_nonspace + 1);
			/* TODO: static */
			memcpy(&container->as.list, data, sizeof(*data));
			free(data);
		} else {
			break;
		}

		if (accepts_lines(container->tag)) {
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

	// node_block quote lines are never blank as they start with >
	// and we don't count blanks in fenced code for purposes of tight/loose
	// lists or breaking out of lists.  we also don't set last_line_blank
	// on an empty list item.
	container->last_line_blank = (blank &&
				      container->tag != BLOCK_BQUOTE &&
				      container->tag != BLOCK_FENCED_CODE &&
				      !(container->tag == BLOCK_LIST_ITEM &&
					container->children == NULL &&
					container->start_line == line_number));

	node_block *cont = container;
	while (cont->parent) {
		cont->parent->last_line_blank = false;
		cont = cont->parent;
	}

	if (cur != last_matched_container &&
	    container == last_matched_container &&
	    !blank &&
	    cur->tag == BLOCK_PARAGRAPH &&
	    strbuf_len(&cur->string_content) > 0) {

		add_line(cur, &input, offset);

	} else { // not a lazy continuation

		// finalize any blocks that were not matched and set cur to container:
		while (cur != last_matched_container) {
			finalize(cur, line_number);
			cur = cur->parent;
			assert(cur != NULL);
		}

		if (container->tag == BLOCK_INDENTED_CODE) {

			add_line(container, &input, offset);

		} else if (container->tag == BLOCK_FENCED_CODE) {
			matched = 0;

			if (indent <= 3 &&
			    peek_at(&input, first_nonspace) == container->as.code.fence_char) {
				int fence_len = scan_close_code_fence(&input, first_nonspace);
				if (fence_len > container->as.code.fence_length)
					matched = 1;
			}

			if (matched) {
				// if closing fence, don't add line to container; instead, close it:
				finalize(container, line_number);
				container = container->parent; // back up to parent
			} else {
				add_line(container, &input, offset);
			}

		} else if (container->tag == BLOCK_HTML) {

			add_line(container, &input, offset);

		} else if (blank) {

			// ??? do nothing

		} else if (container->tag == BLOCK_ATX_HEADER) {

			chop_trailing_hashtags(&input);
			add_line(container, &input, first_nonspace);
			finalize(container, line_number);
			container = container->parent;

		} else if (accepts_lines(container->tag)) {

			add_line(container, &input, first_nonspace);

		} else if (container->tag != BLOCK_HRULE && container->tag != BLOCK_SETEXT_HEADER) {

			// create paragraph container for line
			container = add_child(container, BLOCK_PARAGRAPH, line_number, first_nonspace + 1);
			add_line(container, &input, first_nonspace);

		} else {
			assert(false);
		}

		*curptr = container;
	}
}
