#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "config.h"
#include "node.h"
#include "parser.h"
#include "references.h"
#include "cmark.h"
#include "html/houdini.h"
#include "utf8.h"
#include "scanners.h"
#include "inlines.h"


// Macros for creating various kinds of simple.
#define make_str(s) make_literal(CMARK_NODE_STRING, s)
#define make_code(s) make_literal(CMARK_NODE_INLINE_CODE, s)
#define make_raw_html(s) make_literal(CMARK_NODE_INLINE_HTML, s)
#define make_linebreak() make_simple(CMARK_NODE_LINEBREAK)
#define make_softbreak() make_simple(CMARK_NODE_SOFTBREAK)
#define make_emph(contents) make_inlines(CMARK_NODE_EMPH, contents)
#define make_strong(contents) make_inlines(CMARK_NODE_STRONG, contents)

typedef struct DelimiterStack {
	struct DelimiterStack *previous;
	struct DelimiterStack *next;
	cmark_node *first_inline;
	int delim_count;
	unsigned char delim_char;
	int position;
	bool can_open;
	bool can_close;
} delimiter_stack;

typedef struct Subject {
	chunk input;
	int pos;
	reference_map *refmap;
	delimiter_stack *delimiters;
} subject;

static cmark_node *parse_inlines_from_subject(subject* subj);
static int parse_inline(subject* subj, cmark_node ** last);

static void subject_from_buf(subject *e, strbuf *buffer, reference_map *refmap);
static int subject_find_special_char(subject *subj);

static unsigned char *cmark_clean_autolink(chunk *url, int is_email)
{
	strbuf buf = GH_BUF_INIT;

	chunk_trim(url);

	if (url->len == 0)
		return NULL;

	if (is_email)
		strbuf_puts(&buf, "mailto:");

	houdini_unescape_html_f(&buf, url->data, url->len);
	return strbuf_detach(&buf);
}

static inline cmark_node *make_link(cmark_node *label, unsigned char *url, unsigned char *title)
{
	cmark_node* e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = CMARK_NODE_LINK;
		e->first_child   = label;
                e->last_child    = label;
		e->as.link.url   = url;
		e->as.link.title = title;
		e->next = NULL;
                label->parent = e;
	}
	return e;
}

static inline cmark_node* make_autolink(cmark_node* label, cmark_chunk url, int is_email)
{
	return make_link(label, cmark_clean_autolink(&url, is_email), NULL);
}

// Setting 'last_child' and the parent of 'contents' is up to the caller.
static inline cmark_node* make_inlines(cmark_node_type t, cmark_node* contents)
{
	cmark_node * e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = t;
		e->first_child = contents;
		e->next = NULL;
	}
	return e;
}

// Create an inline with a literal string value.
static inline cmark_node* make_literal(cmark_node_type t, cmark_chunk s)
{
	cmark_node * e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = t;
		e->as.literal = s;
		e->next = NULL;
	}
	return e;
}

// Create an inline with no value.
static inline cmark_node* make_simple(cmark_node_type t)
{
	cmark_node* e = (cmark_node *)calloc(1, sizeof(*e));
	if(e != NULL) {
		e->type = t;
		e->next = NULL;
	}
	return e;
}

static unsigned char *bufdup(const unsigned char *buf)
{
	unsigned char *new_buf = NULL;

	if (buf) {
		int len = strlen((char *)buf);
		new_buf = (unsigned char *)calloc(len + 1, sizeof(*new_buf));
		if(new_buf != NULL) {
			memcpy(new_buf, buf, len + 1);
		}
	}

	return new_buf;
}

static void subject_from_buf(subject *e, strbuf *buffer, reference_map *refmap)
{
	e->input.data = buffer->ptr;
	e->input.len = buffer->size;
	e->input.alloc = 0;
	e->pos = 0;
	e->refmap = refmap;
	e->delimiters = NULL;

	chunk_rtrim(&e->input);
}

static inline int isbacktick(int c)
{
	return (c == '`');
}

static inline unsigned char peek_char(subject *subj)
{
	return (subj->pos < subj->input.len) ? subj->input.data[subj->pos] : 0;
}

static inline unsigned char peek_at(subject *subj, int pos)
{
	return subj->input.data[pos];
}

// Return true if there are more characters in the subject.
static inline int is_eof(subject* subj)
{
	return (subj->pos >= subj->input.len);
}

// Advance the subject.  Doesn't check for eof.
#define advance(subj) (subj)->pos += 1

// Take characters while a predicate holds, and return a string.
static inline chunk take_while(subject* subj, int (*f)(int))
{
	unsigned char c;
	int startpos = subj->pos;
	int len = 0;

	while ((c = peek_char(subj)) && (*f)(c)) {
		advance(subj);
		len++;
	}

	return chunk_dup(&subj->input, startpos, len);
}

// Append inline list b to the end of inline list a.
// Return pointer to head of new list.
static inline cmark_node* cmark_append_inlines(cmark_node* a, cmark_node* b)
{
	if (a == NULL) {  // NULL acts like an empty list
		return b;
	}
	cmark_node* cur = a;
	while (cur->next != NULL) {
		cur = cur->next;
	}
	cur->next = b;
	b->prev = cur;
	return a;
}

// Try to process a backtick code span that began with a
// span of ticks of length openticklength length (already
// parsed).  Return 0 if you don't find matching closing
// backticks, otherwise return the position in the subject
// after the closing backticks.
static int scan_to_closing_backticks(subject* subj, int openticklength)
{
	// read non backticks
	unsigned char c;
	while ((c = peek_char(subj)) && c != '`') {
		advance(subj);
	}
	if (is_eof(subj)) {
		return 0;  // did not find closing ticks, return 0
	}
	int numticks = 0;
	while (peek_char(subj) == '`') {
		advance(subj);
		numticks++;
	}
	if (numticks != openticklength){
		return(scan_to_closing_backticks(subj, openticklength));
	}
	return (subj->pos);
}

// Parse backtick code section or raw backticks, return an inline.
// Assumes that the subject has a backtick at the current position.
static cmark_node* handle_backticks(subject *subj)
{
	chunk openticks = take_while(subj, isbacktick);
	int startpos = subj->pos;
	int endpos = scan_to_closing_backticks(subj, openticks.len);

	if (endpos == 0) { // not found
		subj->pos = startpos; // rewind
		return make_str(openticks);
	} else {
		strbuf buf = GH_BUF_INIT;

		strbuf_set(&buf, subj->input.data + startpos, endpos - startpos - openticks.len);
		strbuf_trim(&buf);
		strbuf_normalize_whitespace(&buf);

		return make_code(chunk_buf_detach(&buf));
	}
}

// Scan ***, **, or * and return number scanned, or 0.
// Advances position.
static int scan_delims(subject* subj, unsigned char c, bool * can_open, bool * can_close)
{
	int numdelims = 0;
	unsigned char char_before, char_after;

	char_before = subj->pos == 0 ? '\n' : peek_at(subj, subj->pos - 1);
	while (peek_char(subj) == c) {
		numdelims++;
		advance(subj);
	}
	char_after = peek_char(subj);
	*can_open = numdelims > 0 && !isspace(char_after);
	*can_close = numdelims > 0 && !isspace(char_before);
	if (c == '_') {
		*can_open = *can_open && !isalnum(char_before);
		*can_close = *can_close && !isalnum(char_after);
	}
	return numdelims;
}

/*
static void print_delimiters(subject *subj)
{
	delimiter_stack *tempstack;
	tempstack = subj->delimiters;
	while (tempstack != NULL) {
		printf("Item at %p: %d %d %d %d next(%p) prev(%p)\n",
		       tempstack, tempstack->delim_count, tempstack->delim_char,
		       tempstack->can_open, tempstack->can_close,
		       tempstack->next, tempstack->previous);
		tempstack = tempstack->previous;
	}
}
*/

static void remove_delimiter(subject *subj, delimiter_stack *stack)
{
	if (stack->previous != NULL) {
		stack->previous->next = stack->next;
	}
	if (stack->next == NULL) {
		// top of stack
		subj->delimiters = stack->previous;
	} else {
		stack->next->previous = stack->previous;
	}
	free(stack);
}

static delimiter_stack * push_delimiter(subject *subj,
					int numdelims,
					unsigned char c,
					bool can_open,
					bool can_close,
					cmark_node *inl_text)
{
	delimiter_stack *istack =
		(delimiter_stack*)malloc(sizeof(delimiter_stack));
	if (istack == NULL) {
		return NULL;
	}
	istack->delim_count = numdelims;
	istack->delim_char = c;
	istack->can_open = can_open;
	istack->can_close = can_close;
	istack->first_inline = inl_text;
	istack->previous = subj->delimiters;
	istack->next = NULL;
	if (istack->previous != NULL) {
		istack->previous->next = istack;
	}
	istack->position = subj->pos;
	return istack;
}

// Parse strong/emph or a fallback.
// Assumes the subject has '_' or '*' at the current position.
static cmark_node* handle_strong_emph(subject* subj, unsigned char c, cmark_node **last)
{
	int numdelims;
	cmark_node * inl_text;
	bool can_open, can_close;

	numdelims = scan_delims(subj, c, &can_open, &can_close);

	inl_text = make_str(chunk_dup(&subj->input, subj->pos - numdelims, numdelims));

	if (can_open || can_close) {
		subj->delimiters = push_delimiter(subj, numdelims, c, can_open, can_close,
						  inl_text);
	}

	return inl_text;
}

static void process_emphasis(subject *subj, delimiter_stack *stack_bottom)
{
	delimiter_stack *closer = subj->delimiters;
	delimiter_stack *opener, *tempstack, *nextstack;
	int use_delims;
	cmark_node *inl, *tmp, *emph;

	// move back to first relevant delim.
	while (closer != NULL && closer->previous != stack_bottom) {
		closer = closer->previous;
	}

	// now move forward, looking for closers, and handling each
	while (closer != NULL) {
		if (closer->can_close &&
		    (closer->delim_char == '*' || closer->delim_char == '_')) {
			// Now look backwards for first matching opener:
			opener = closer->previous;
			while (opener != NULL && opener != stack_bottom) {
				if (opener->delim_char == closer->delim_char &&
				    opener->can_open) {
					break;
				}
				opener = opener->previous;
			}
			if (opener != NULL && opener != stack_bottom) {
				// calculate the actual number of delimeters used from this closer
				if (closer->delim_count < 3 || opener->delim_count < 3) {
					use_delims = closer->delim_count <= opener->delim_count ?
						closer->delim_count : opener->delim_count;
				} else { // closer and opener both have >= 3 delims
					use_delims = closer->delim_count % 2 == 0 ? 2 : 1;
				}

				inl = opener->first_inline;

				// remove used delimiters from stack elements and associated inlines.
				opener->delim_count -= use_delims;
				closer->delim_count -= use_delims;
				inl->as.literal.len = opener->delim_count;
				closer->first_inline->as.literal.len = closer->delim_count;

				// free delimiters between opener and closer
				tempstack = closer->previous;
				while (tempstack != NULL && tempstack != opener) {
					nextstack = tempstack->previous;
					remove_delimiter(subj, tempstack);
					tempstack = nextstack;
				}

				// create new emph or strong, and splice it in to our inlines
				// between the opener and closer
				emph = use_delims == 1 ? make_emph(inl->next) : make_strong(inl->next);
				emph->next = closer->first_inline;
				emph->prev = inl;
				inl->next = emph;

				// if opener has 0 delims, remove it and its associated inline
				if (opener->delim_count == 0) {
					// replace empty opener inline with emph
					chunk_free(&(inl->as.literal));
					inl->type = emph->type;
					inl->next = emph->next;
					inl->first_child = emph->first_child;
					free(emph);
					emph = inl;
					// remove opener from stack
					remove_delimiter(subj, opener);
				}

				// fix tree structure
				tmp = emph->first_child;
				while (tmp->next != NULL && tmp->next != closer->first_inline) {
					tmp->parent = emph;
					tmp = tmp->next;
				}
				tmp->parent = emph;
				if (tmp->next) {
					tmp->next->prev = emph;
				}
				tmp->next = NULL;
				emph->last_child = tmp;

				// if closer has 0 delims, remove it and its associated inline
				if (closer->delim_count == 0) {
					// remove empty closer inline
					tmp = closer->first_inline;
					emph->next = tmp->next;
					if (tmp->next) {
						tmp->next->prev = emph;
					}
					tmp->next = NULL;
					cmark_free_nodes(tmp);
					// remove closer from stack
					tempstack = closer->next;
					remove_delimiter(subj, closer);
					closer = tempstack;
				}
			} else {
				closer = closer->next;
			}
		} else {
			closer = closer->next;
		}
	}
	// free all delimiters in stack down to stack_bottom:
	while (subj->delimiters != stack_bottom) {
		remove_delimiter(subj, subj->delimiters);
	}
}

// Parse backslash-escape or just a backslash, returning an inline.
static cmark_node* handle_backslash(subject *subj)
{
	advance(subj);
	unsigned char nextchar = peek_char(subj);
	if (ispunct(nextchar)) {  // only ascii symbols and newline can be escaped
		advance(subj);
		return make_str(chunk_dup(&subj->input, subj->pos - 1, 1));
	} else if (nextchar == '\n') {
		advance(subj);
		return make_linebreak();
	} else {
		return make_str(chunk_literal("\\"));
	}
}

// Parse an entity or a regular "&" string.
// Assumes the subject has an '&' character at the current position.
static cmark_node* handle_entity(subject* subj)
{
	strbuf ent = GH_BUF_INIT;
	size_t len;

	advance(subj);

	len = houdini_unescape_ent(&ent,
				   subj->input.data + subj->pos,
				   subj->input.len - subj->pos
				   );

	if (len == 0)
		return make_str(chunk_literal("&"));

	subj->pos += len;
	return make_str(chunk_buf_detach(&ent));
}

// Like make_str, but parses entities.
// Returns an inline sequence consisting of str and entity elements.
static cmark_node *make_str_with_entities(chunk *content)
{
	strbuf unescaped = GH_BUF_INIT;

	if (houdini_unescape_html(&unescaped, content->data, (size_t)content->len)) {
		return make_str(chunk_buf_detach(&unescaped));
	} else {
		return make_str(*content);
	}
}

// Clean a URL: remove surrounding whitespace and surrounding <>,
// and remove \ that escape punctuation.
unsigned char *clean_url(chunk *url)
{
	strbuf buf = GH_BUF_INIT;

	chunk_trim(url);

	if (url->len == 0)
		return NULL;

	if (url->data[0] == '<' && url->data[url->len - 1] == '>') {
		houdini_unescape_html_f(&buf, url->data + 1, url->len - 2);
	} else {
		houdini_unescape_html_f(&buf, url->data, url->len);
	}

	strbuf_unescape(&buf);
	return strbuf_detach(&buf);
}

unsigned char *clean_title(chunk *title)
{
       strbuf buf = GH_BUF_INIT;
       unsigned char first, last;

       if (title->len == 0)
               return NULL;

       first = title->data[0];
       last = title->data[title->len - 1];

       // remove surrounding quotes if any:
       if ((first == '\'' && last == '\'') ||
           (first == '(' && last == ')') ||
           (first == '"' && last == '"')) {
               houdini_unescape_html_f(&buf, title->data + 1, title->len - 2);
       } else {
               houdini_unescape_html_f(&buf, title->data, title->len);
       }

       strbuf_unescape(&buf);
       return strbuf_detach(&buf);
}

// Parse an autolink or HTML tag.
// Assumes the subject has a '<' character at the current position.
static cmark_node* handle_pointy_brace(subject* subj)
{
	int matchlen = 0;
	chunk contents;

	advance(subj);  // advance past first <

	// first try to match a URL autolink
	matchlen = scan_autolink_uri(&subj->input, subj->pos);
	if (matchlen > 0) {
		contents = chunk_dup(&subj->input, subj->pos, matchlen - 1);
		subj->pos += matchlen;

		return make_autolink(
				     make_str_with_entities(&contents),
				     contents, 0
				     );
	}

	// next try to match an email autolink
	matchlen = scan_autolink_email(&subj->input, subj->pos);
	if (matchlen > 0) {
		contents = chunk_dup(&subj->input, subj->pos, matchlen - 1);
		subj->pos += matchlen;

		return make_autolink(
				     make_str_with_entities(&contents),
				     contents, 1
				     );
	}

	// finally, try to match an html tag
	matchlen = scan_html_tag(&subj->input, subj->pos);
	if (matchlen > 0) {
		contents = chunk_dup(&subj->input, subj->pos - 1, matchlen + 1);
		subj->pos += matchlen;
		return make_raw_html(contents);
	}

	// if nothing matches, just return the opening <:
	return make_str(chunk_literal("<"));
}

// Parse a link label.  Returns 1 if successful.
// Note:  unescaped brackets are not allowed in labels.
// The label begins with `[` and ends with the first `]` character
// encountered.  Backticks in labels do not start code spans.
static int link_label(subject* subj, chunk *raw_label)
{
	int startpos = subj->pos;
	int length = 0;

	advance(subj);  // advance past [
	unsigned char c;
	while ((c = peek_char(subj)) && c != '[' && c != ']') {
		if (c == '\\') {
			advance(subj);
			length++;
			if (ispunct(peek_char(subj))) {
				advance(subj);
				length++;
			}
		} else {
			advance(subj);
			length++;
		}
		if (length > MAX_LINK_LABEL_LENGTH) {
			goto noMatch;
		}
	}

	if (c == ']') { // match found
		*raw_label = chunk_dup(&subj->input, startpos + 1, subj->pos - (startpos + 1));
		advance(subj);  // advance past ]
		return 1;
	}

 noMatch:
	subj->pos = startpos; // rewind
	return 0;

}

// Return a link, an image, or a literal close bracket.
static cmark_node* handle_close_bracket(subject* subj, cmark_node **last)
{
	int initial_pos;
	int starturl, endurl, starttitle, endtitle, endall;
	int n;
	int sps;
	reference *ref;
	bool is_image = false;
	chunk urlchunk, titlechunk;
	unsigned char *url, *title;
	delimiter_stack *opener;
	delimiter_stack *tempstack;
	cmark_node *link_text;
	cmark_node *inl;
	chunk raw_label;

	advance(subj);  // advance past ]
	initial_pos = subj->pos;

	// look through stack of delimiters for a [ or !
	opener = subj->delimiters;
	while (opener) {
		if (opener->delim_char == '[' || opener->delim_char == '!') {
			break;
		}
		opener = opener->previous;
	}

	if (opener == NULL) {
		return make_str(chunk_literal("]"));
	}

	// If we got here, we matched a potential link/image text.
	is_image = opener->delim_char == '!';
	link_text = opener->first_inline->next;

	// Now we check to see if it's a link/image.

	// First, look for an inline link.
	if (peek_char(subj) == '(' &&
	    ((sps = scan_spacechars(&subj->input, subj->pos + 1)) > -1) &&
	    ((n = scan_link_url(&subj->input, subj->pos + 1 + sps)) > -1)) {

		// try to parse an explicit link:
		starturl = subj->pos + 1 + sps; // after (
		endurl = starturl + n;
		starttitle = endurl + scan_spacechars(&subj->input, endurl);

		// ensure there are spaces btw url and title
		endtitle = (starttitle == endurl) ? starttitle :
			starttitle + scan_link_title(&subj->input, starttitle);

		endall = endtitle + scan_spacechars(&subj->input, endtitle);

		if (peek_at(subj, endall) == ')') {
			subj->pos = endall + 1;

			urlchunk = chunk_dup(&subj->input, starturl, endurl - starturl);
			titlechunk = chunk_dup(&subj->input, starttitle, endtitle - starttitle);
			url = clean_url(&urlchunk);
			title = clean_title(&titlechunk);
			chunk_free(&urlchunk);
			chunk_free(&titlechunk);
			goto match;

		} else {
			goto noMatch;
		}
	}

	// Next, look for a following [link label] that matches in refmap.
	// skip spaces
	subj->pos = subj->pos + scan_spacechars(&subj->input, subj->pos);
	raw_label = chunk_literal("");
	if (!link_label(subj, &raw_label) || raw_label.len == 0) {
		chunk_free(&raw_label);
		raw_label = chunk_dup(&subj->input, opener->position,
				      initial_pos - opener->position - 1);
	}

	ref = reference_lookup(subj->refmap, &raw_label);
	chunk_free(&raw_label);

	if (ref != NULL) { // found
		url = bufdup(ref->url);
		title = bufdup(ref->title);
		goto match;
	} else {
		goto noMatch;
	}

noMatch:
	// If we fall through to here, it means we didn't match a link:
	remove_delimiter(subj, opener);  // remove this opener from delimiter stack
	subj->pos = initial_pos;
	return make_str(chunk_literal("]"));

match:
	inl = opener->first_inline;
	inl->type = is_image ? NODE_IMAGE : NODE_LINK;
	chunk_free(&inl->as.literal);
	inl->first_child = link_text;
	process_emphasis(subj, opener->previous);
	inl->as.link.url   = url;
	inl->as.link.title = title;
	inl->next = NULL;
	if (link_text) {
		cmark_node *tmp;
		link_text->prev = NULL;
		for (tmp = link_text; tmp->next != NULL; tmp = tmp->next) {
			tmp->parent = inl;
		}
		tmp->parent = inl;
		inl->last_child = tmp;
	}
	*last = inl;

	// process_emphasis will remove this delimiter and all later ones.
	// Now, if we have a link, we also want to remove earlier link
        // delimiters. (This code can be removed if we decide to allow links
	// inside links.)
	if (!is_image) {
		opener = subj->delimiters;
		while (opener != NULL) {
			tempstack = opener->previous;
			if (opener->delim_char == '[') {
				remove_delimiter(subj, opener);
			}
			opener = tempstack;
		}
	}

	return NULL;
}

// Parse a hard or soft linebreak, returning an inline.
// Assumes the subject has a newline at the current position.
static cmark_node* handle_newline(subject *subj)
{
	int nlpos = subj->pos;
	// skip over newline
	advance(subj);
	// skip spaces at beginning of line
	while (peek_char(subj) == ' ') {
		advance(subj);
	}
	if (nlpos > 1 &&
	    peek_at(subj, nlpos - 1) == ' ' &&
	    peek_at(subj, nlpos - 2) == ' ') {
		return make_linebreak();
	} else {
		return make_softbreak();
	}
}

// Parse inlines til end of subject, returning inlines.
extern cmark_node* parse_inlines_from_subject(subject* subj)
{
	cmark_node* result = NULL;
	cmark_node** last = &result;
	cmark_node* first = NULL;
	while (!is_eof(subj) && parse_inline(subj, last)) {
		if (!first) {
			first = *last;
		}
	}

	process_emphasis(subj, NULL);

	return first;
}

static int subject_find_special_char(subject *subj)
{
	// "\n\\`&_*[]<!"
	static const int8_t SPECIAL_CHARS[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	int n = subj->pos + 1;

	while (n < subj->input.len) {
		if (SPECIAL_CHARS[subj->input.data[n]])
			return n;
		n++;
	}

	return subj->input.len;
}

// Parse an inline, advancing subject, and add it to last element.
// Adjust tail to point to new last element of list.
// Return 0 if no inline can be parsed, 1 otherwise.
static int parse_inline(subject* subj, cmark_node ** last)
{
	cmark_node* new_inl = NULL;
	chunk contents;
	unsigned char c;
	int endpos;
	c = peek_char(subj);
	if (c == 0) {
		return 0;
	}
	switch(c){
	case '\n':
		new_inl = handle_newline(subj);
		break;
	case '`':
		new_inl = handle_backticks(subj);
		break;
	case '\\':
		new_inl = handle_backslash(subj);
		break;
	case '&':
		new_inl = handle_entity(subj);
		break;
	case '<':
		new_inl = handle_pointy_brace(subj);
		break;
	case '*':
	case '_':
		new_inl = handle_strong_emph(subj, c, last);
		break;
	case '[':
		advance(subj);
		new_inl = make_str(chunk_literal("["));
		subj->delimiters = push_delimiter(subj, 1, '[', true, false, new_inl);
		break;
	case ']':
		new_inl = handle_close_bracket(subj, last);
		break;
	case '!':
		advance(subj);
		if (peek_char(subj) == '[') {
			advance(subj);
			new_inl = make_str(chunk_literal("!["));
			subj->delimiters = push_delimiter(subj, 1, '!', false, true, new_inl);
		} else {
			new_inl = make_str(chunk_literal("!"));
		}
		break;
	default:
		endpos = subject_find_special_char(subj);
		contents = chunk_dup(&subj->input, subj->pos, endpos - subj->pos);
		subj->pos = endpos;

		// if we're at a newline, strip trailing spaces.
		if (peek_char(subj) == '\n') {
			chunk_rtrim(&contents);
		}

		new_inl = make_str(contents);
	}
	if (*last == NULL) {
		*last = new_inl;
	} else if (new_inl) {
		cmark_append_inlines(*last, new_inl);
		*last = new_inl;
	}

	return 1;
}

extern cmark_node* parse_inlines(strbuf *input, reference_map *refmap)
{
	subject subj;
	subject_from_buf(&subj, input, refmap);
	return parse_inlines_from_subject(&subj);
}

// Parse zero or more space characters, including at most one newline.
static void spnl(subject* subj)
{
	bool seen_newline = false;
	while (peek_char(subj) == ' ' ||
	       (!seen_newline &&
		(seen_newline = peek_char(subj) == '\n'))) {
		advance(subj);
	}
}

// Parse reference.  Assumes string begins with '[' character.
// Modify refmap if a reference is encountered.
// Return 0 if no reference found, otherwise position of subject
// after reference is parsed.
int parse_reference_inline(strbuf *input, reference_map *refmap)
{
	subject subj;

	chunk lab;
	chunk url;
	chunk title;

	int matchlen = 0;
	int beforetitle;

	subject_from_buf(&subj, input, NULL);

	// parse label:
	if (!link_label(&subj, &lab))
		return 0;

	// colon:
	if (peek_char(&subj) == ':') {
		advance(&subj);
	} else {
		return 0;
	}

	// parse link url:
	spnl(&subj);
	matchlen = scan_link_url(&subj.input, subj.pos);
	if (matchlen) {
		url = chunk_dup(&subj.input, subj.pos, matchlen);
		subj.pos += matchlen;
	} else {
		return 0;
	}

	// parse optional link_title
	beforetitle = subj.pos;
	spnl(&subj);
	matchlen = scan_link_title(&subj.input, subj.pos);
	if (matchlen) {
		title = chunk_dup(&subj.input, subj.pos, matchlen);
		subj.pos += matchlen;
	} else {
		subj.pos = beforetitle;
		title = chunk_literal("");
	}
	// parse final spaces and newline:
	while (peek_char(&subj) == ' ') {
		advance(&subj);
	}
	if (peek_char(&subj) == '\n') {
		advance(&subj);
	} else if (peek_char(&subj) != 0) {
		return 0;
	}
	// insert reference into refmap
	reference_create(refmap, &lab, &url, &title);
	return subj.pos;
}
