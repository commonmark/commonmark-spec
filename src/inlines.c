#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "cmark.h"
#include "html/houdini.h"
#include "utf8.h"
#include "scanners.h"
#include "inlines.h"
#include "debug.h"

typedef struct OpenerStack {
	struct OpenerStack *previous;
	node_inl *first_inline;
	int delim_count;
	unsigned char delim_char;
	int position;
} opener_stack;

typedef struct Subject {
	chunk input;
	int pos;
	reference_map *refmap;
	opener_stack *openers;
} subject;

static node_inl *parse_inlines_from_subject(subject* subj);
static int parse_inline(subject* subj, node_inl ** last);

static void subject_from_buf(subject *e, strbuf *buffer, reference_map *refmap);
static int subject_find_special_char(subject *subj);

static unsigned char *bufdup(const unsigned char *buf)
{
	unsigned char *new = NULL;

	if (buf) {
		int len = strlen((char *)buf);
		new = calloc(len + 1, sizeof(*new));
		if(new != NULL) {
			memcpy(new, buf, len + 1);
		}
	}

	return new;
}

static inline node_inl *make_link_(node_inl *label, unsigned char *url, unsigned char *title)
{
	node_inl* e = calloc(1, sizeof(*e));
	if(e != NULL) {
		e->tag = INL_LINK;
		e->content.linkable.label = label;
		e->content.linkable.url   = url;
		e->content.linkable.title = title;
		e->next = NULL;
	}
	return e;
}

inline static node_inl* make_autolink(node_inl* label, chunk url, int is_email)
{
	return make_link_(label, clean_autolink(&url, is_email), NULL);
}

inline static node_inl* make_inlines(int t, node_inl* contents)
{
	node_inl * e = calloc(1, sizeof(*e));
	if(e != NULL) {
		e->tag = t;
		e->content.inlines = contents;
		e->next = NULL;
	}
	return e;
}

// Create an inline with a literal string value.
inline static node_inl* make_literal(int t, chunk s)
{
	node_inl * e = calloc(1, sizeof(*e));
	if(e != NULL) {
		e->tag = t;
		e->content.literal = s;
		e->next = NULL;
	}
	return e;
}

// Create an inline with no value.
inline static node_inl* make_simple(int t)
{
	node_inl* e = calloc(1, sizeof(*e));
	if(e != NULL) {
		e->tag = t;
		e->next = NULL;
	}
	return e;
}

// Macros for creating various kinds of inlines.
#define make_str(s) make_literal(INL_STRING, s)
#define make_code(s) make_literal(INL_CODE, s)
#define make_raw_html(s) make_literal(INL_RAW_HTML, s)
#define make_linebreak() make_simple(INL_LINEBREAK)
#define make_softbreak() make_simple(INL_SOFTBREAK)
#define make_emph(contents) make_inlines(INL_EMPH, contents)
#define make_strong(contents) make_inlines(INL_STRONG, contents)

// Utility function used by free_inlines
void splice_into_list(node_inl* e, node_inl* children) {
	node_inl * tmp;
	if (children) {
		tmp = children;
		// Find last child
		while (tmp->next) {
			tmp = tmp->next;
		}
		// Splice children into list
		tmp->next = e->next;
		e->next = children;
	}
	return ;
}

// Free an inline list.  Avoid recursion to prevent stack overflows
// on deeply nested structures.
extern void free_inlines(node_inl* e)
{
	node_inl * next;

	while (e != NULL) {
		switch (e->tag){
		case INL_STRING:
		case INL_RAW_HTML:
		case INL_CODE:
			chunk_free(&e->content.literal);
			break;
		case INL_LINEBREAK:
		case INL_SOFTBREAK:
			break;
		case INL_LINK:
		case INL_IMAGE:
			free(e->content.linkable.url);
			free(e->content.linkable.title);
			splice_into_list(e, e->content.linkable.label);
			break;
		case INL_EMPH:
		case INL_STRONG:
		        splice_into_list(e, e->content.inlines);
			break;
		default:
		        log_warn("Unknown inline tag %d", e->tag);
			break;
		}
		next = e->next;
		free(e);
		e = next;
	}
}

// Append inline list b to the end of inline list a.
// Return pointer to head of new list.
inline static node_inl* append_inlines(node_inl* a, node_inl* b)
{
	if (a == NULL) {  // NULL acts like an empty list
		return b;
	}
	node_inl* cur = a;
	while (cur->next) {
		cur = cur->next;
	}
	cur->next = b;
	return a;
}

static void subject_from_buf(subject *e, strbuf *buffer, reference_map *refmap)
{
	e->input.data = buffer->ptr;
	e->input.len = buffer->size;
	e->input.alloc = 0;
	e->pos = 0;
	e->refmap = refmap;
	e->openers = NULL;

	chunk_rtrim(&e->input);
}

inline static int isbacktick(int c)
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
inline static int is_eof(subject* subj)
{
	return (subj->pos >= subj->input.len);
}

// Advance the subject.  Doesn't check for eof.
#define advance(subj) (subj)->pos += 1

// Take characters while a predicate holds, and return a string.
inline static chunk take_while(subject* subj, int (*f)(int))
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
static node_inl* handle_backticks(subject *subj)
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

static void free_openers(subject* subj, opener_stack* istack)
{
	opener_stack * tempstack;
	while (subj->openers != istack) {
		tempstack = subj->openers;
		subj->openers = subj->openers->previous;
		free(tempstack);
	}
}

static opener_stack * push_opener(subject *subj,
				     int numdelims,
				     unsigned char c,
				     node_inl *inl_text)
{
	opener_stack *istack =
		(opener_stack*)malloc(sizeof(opener_stack));
	if (istack == NULL) {
		return NULL;
	}
	istack->delim_count = numdelims;
	istack->delim_char = c;
	istack->first_inline = inl_text;
	istack->previous = subj->openers;
	istack->position = subj->pos;
	return istack;
}

// Parse strong/emph or a fallback.
// Assumes the subject has '_' or '*' at the current position.
static node_inl* handle_strong_emph(subject* subj, unsigned char c, node_inl **last)
{
	bool can_open, can_close;
	int numdelims;
	int useDelims;
	int openerDelims;
	opener_stack * istack;
	node_inl * inl;
	node_inl * emph;
	node_inl * inl_text;

	numdelims = scan_delims(subj, c, &can_open, &can_close);

	if (can_close)
		{
			// walk the stack and find a matching opener, if there is one
			istack = subj->openers;
			while (true)
				{
					if (istack == NULL)
						goto cannotClose;

					if (istack->delim_char == c)
						break;

					istack = istack->previous;
				}

			// calculate the actual number of delimeters used from this closer
			openerDelims = istack->delim_count;
			if (numdelims < 3 || openerDelims < 3) {
				useDelims = numdelims <= openerDelims ? numdelims : openerDelims;
			} else { // (numdelims >= 3 && openerDelims >= 3)
				useDelims = numdelims % 2 == 0 ? 2 : 1;
			}

			if (istack->delim_count == useDelims)
				{
					// the opener is completely used up - remove the stack entry and reuse the inline element
					inl = istack->first_inline;
					inl->tag = useDelims == 1 ? INL_EMPH : INL_STRONG;
					chunk_free(&inl->content.literal);
					inl->content.inlines = inl->next;
					inl->next = NULL;

					// remove this opener and all later ones from stack:
					free_openers(subj, istack->previous);
					*last = inl;
				}
			else
				{
					// the opener will only partially be used - stack entry remains (truncated) and a new inline is added.
					inl = istack->first_inline;
					istack->delim_count -= useDelims;
					inl->content.literal.len = istack->delim_count;

					emph = useDelims == 1 ? make_emph(inl->next) : make_strong(inl->next);
					inl->next = emph;

					// remove all later openers from stack:
					free_openers(subj, istack);

					*last = emph;
				}

			// if the closer was not fully used, move back a char or two and try again.
			if (useDelims < numdelims)
				{
					subj->pos = subj->pos - numdelims + useDelims;
					return NULL;
				}

			return NULL; // make_str(chunk_literal(""));
		}

 cannotClose:
	inl_text = make_str(chunk_dup(&subj->input, subj->pos - numdelims, numdelims));

	if (can_open)
		{
			subj->openers = push_opener(subj,
						    numdelims,
						    c,
						    inl_text);
		}

	return inl_text;
}

// Parse backslash-escape or just a backslash, returning an inline.
static node_inl* handle_backslash(subject *subj)
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
static node_inl* handle_entity(subject* subj)
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
static node_inl *make_str_with_entities(chunk *content)
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

unsigned char *clean_autolink(chunk *url, int is_email)
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

// Clean a title: remove surrounding quotes and remove \ that escape punctuation.
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
static node_inl* handle_pointy_brace(subject* subj)
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
static node_inl* handle_close_bracket(subject* subj, node_inl **last)
{
	int initial_pos;
	int starturl, endurl, starttitle, endtitle, endall;
	int n;
	int sps;
	reference *ref;
	bool is_image = false;
	chunk urlchunk, titlechunk;
	unsigned char *url, *title;
	opener_stack *ostack = subj->openers;
	opener_stack *closer_above;
	opener_stack *tempstack;
	node_inl *link_text;
	node_inl *inl;
	chunk raw_label;

	advance(subj);  // advance past ]
	initial_pos = subj->pos;

	// look through stack of openers for a [ or !
	while (ostack) {
		if (ostack->delim_char == '[' || ostack->delim_char == '!') {
			break;
		}
		ostack = ostack->previous;
	}

	if (ostack == NULL) {
		return make_str(chunk_literal("]"));
	}

	// If we got here, we matched a potential link/image text.
	is_image = ostack->delim_char == '!';
	link_text = ostack->first_inline->next;

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
		raw_label = chunk_dup(&subj->input, ostack->position, initial_pos - ostack->position - 1);
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
	subj->pos = initial_pos;
	return make_str(chunk_literal("]"));

match:
	inl = ostack->first_inline;
	inl->tag = is_image ? INL_IMAGE : INL_LINK;
	chunk_free(&inl->content.literal);
	inl->content.linkable.label = link_text;
	inl->content.linkable.url   = url;
	inl->content.linkable.title = title;
	inl->next = NULL;
	*last = inl;

	// remove this opener and all later ones:
	free_openers(subj, ostack->previous);

	// remove earlier ones of the same kind
	// (so, no links in links, and no images in images):
	// (This code can be removed if we decide to allow links
	// inside links and images inside images):
	ostack = subj->openers;
	closer_above = NULL;
	while (ostack != NULL) {
		tempstack = ostack->previous;
		if (ostack->delim_char == (is_image ? '!' : '[')) {
			free(ostack);
			if (closer_above) {
				closer_above->previous = tempstack;
			} else {
				subj->openers = tempstack;
			}
		} else {
			closer_above = ostack;
		}
		ostack = tempstack;
	}

	return NULL;
}

// Parse a hard or soft linebreak, returning an inline.
// Assumes the subject has a newline at the current position.
static node_inl* handle_newline(subject *subj)
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
extern node_inl* parse_inlines_from_subject(subject* subj)
{
	node_inl* result = NULL;
	node_inl** last = &result;
	node_inl* first = NULL;
	while (!is_eof(subj) && parse_inline(subj, last)) {
		if (!first) {
			first = *last;
		}
	}

	opener_stack* istack = subj->openers;
	opener_stack* temp;
	while (istack != NULL) {
		temp = istack->previous;
		free(istack);
		istack = temp;
	}

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
static int parse_inline(subject* subj, node_inl ** last)
{
	node_inl* new = NULL;
	chunk contents;
	unsigned char c;
	int endpos;
	c = peek_char(subj);
	if (c == 0) {
		return 0;
	}
	switch(c){
	case '\n':
		new = handle_newline(subj);
		break;
	case '`':
		new = handle_backticks(subj);
		break;
	case '\\':
		new = handle_backslash(subj);
		break;
	case '&':
		new = handle_entity(subj);
		break;
	case '<':
		new = handle_pointy_brace(subj);
		break;
	case '_':
		new = handle_strong_emph(subj, '_', last);
		break;
	case '*':
		new = handle_strong_emph(subj, '*', last);
		break;
	case '[':
		advance(subj);
		new = make_str(chunk_literal("["));
		subj->openers = push_opener(subj, 1, '[', new);
		break;
	case ']':
		new = handle_close_bracket(subj, last);
		break;
	case '!':
		advance(subj);
		if (peek_char(subj) == '[') {
			advance(subj);
			new = make_str(chunk_literal("!["));
			subj->openers = push_opener(subj, 1, '!', new);
		} else {
			new = make_str(chunk_literal("!"));
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

		new = make_str(contents);
	}
	if (*last == NULL) {
		*last = new;
	} else if (new) {
		append_inlines(*last, new);
		*last = new;
	}

	return 1;
}

extern node_inl* parse_inlines(strbuf *input, reference_map *refmap)
{
	subject subj;
	subject_from_buf(&subj, input, refmap);
	return parse_inlines_from_subject(&subj);
}

// Parse zero or more space characters, including at most one newline.
void spnl(subject* subj)
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
