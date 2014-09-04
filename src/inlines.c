#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "stmd.h"
#include "uthash.h"
#include "scanners.h"

typedef struct Subject {
	chunk input;
	int pos;
	int            label_nestlevel;
	reference**    reference_map;
} subject;

reference* lookup_reference(reference** refmap, chunk *label);
reference* make_reference(chunk *label, chunk *url, chunk *title);

static unsigned char *clean_url(chunk *url, int is_email);
static unsigned char *clean_title(chunk *title);

inline static void chunk_free(chunk *c);
inline static void chunk_trim(chunk *c);

inline static chunk chunk_literal(const char *data);
inline static chunk chunk_buf_detach(strbuf *buf);
inline static chunk chunk_dup(const chunk *ch, int pos, int len);

static inl *parse_chunk_inlines(chunk *chunk, reference** refmap);
static inl *parse_inlines_while(subject* subj, int (*f)(subject*));
static int parse_inline(subject* subj, inl ** last);

static void subject_from_chunk(subject *e, chunk *chunk, reference** refmap);
static void subject_from_buf(subject *e, strbuf *buffer, reference** refmap);
static int subject_find_special_char(subject *subj);

static void normalize_whitespace(strbuf *s);

extern void free_reference(reference *ref) {
	free(ref->label);
	free(ref->url);
	free(ref->title);
	free(ref);
}

extern void free_reference_map(reference **refmap) {
	/* free the hash table contents */
	reference *s;
	reference *tmp;
	if (refmap != NULL) {
		HASH_ITER(hh, *refmap, s, tmp) {
			HASH_DEL(*refmap, s);
			free_reference(s);
		}
		free(refmap);
	}
}

// normalize reference:  collapse internal whitespace to single space,
// remove leading/trailing whitespace, case fold
static unsigned char *normalize_reference(chunk *ref)
{
	strbuf normalized = GH_BUF_INIT;

	utf8proc_case_fold(&normalized, ref->data, ref->len);
	strbuf_trim(&normalized);
	normalize_whitespace(&normalized);

	return strbuf_detach(&normalized);
}

// Returns reference if refmap contains a reference with matching
// label, otherwise NULL.
extern reference* lookup_reference(reference** refmap, chunk *label)
{
	reference *ref = NULL;
	unsigned char *norm = normalize_reference(label);
	if (refmap != NULL) {
		HASH_FIND_STR(*refmap, (char*)norm, ref);
	}
	free(norm);
	return ref;
}

extern reference* make_reference(chunk *label, chunk *url, chunk *title)
{
	reference *ref;
	ref = malloc(sizeof(reference));
	ref->label = normalize_reference(label);
	ref->url = clean_url(url, 0);
	ref->title = clean_title(title);
	return ref;
}

extern void add_reference(reference** refmap, reference* ref)
{
	reference * t = NULL;
	const char *label = (const char *)ref->label;

	HASH_FIND(hh, *refmap, label, strlen(label), t);

	if (t == NULL) {
		HASH_ADD_KEYPTR(hh, *refmap, label, strlen(label), ref);
	} else {
		free_reference(ref);  // we free this now since it won't be in the refmap
	}
}

inline static inl* make_link_from_reference(inl* label, reference *ref)
{
	inl* e = (inl*) malloc(sizeof(inl));
	e->tag = INL_LINK;
	e->content.linkable.label = label;
	e->content.linkable.url   = strdup(ref->url);
	e->content.linkable.title = ref->title ? strdup(ref->title) : NULL;
	e->next = NULL;
	return e;
}

// Create an inline with a linkable string value.
inline static inl* make_link(inl* label, chunk url, chunk title, int is_email)
{
	inl* e = (inl*) malloc(sizeof(inl));
	e->tag = INL_LINK;
	e->content.linkable.label = label;
	e->content.linkable.url   = clean_url(&url, is_email);
	e->content.linkable.title = clean_title(&title);
	e->next = NULL;
	return e;
}

inline static inl* make_inlines(int t, inl* contents)
{
	inl* e = (inl*) malloc(sizeof(inl));
	e->tag = t;
	e->content.inlines = contents;
	e->next = NULL;
	return e;
}

// Create an inline with a literal string value.
inline static inl* make_literal(int t, chunk s)
{
	inl* e = (inl*) malloc(sizeof(inl));
	e->tag = t;
	e->content.literal = s;
	e->next = NULL;
	return e;
}

// Create an inline with no value.
inline static inl* make_simple(int t)
{
	inl* e = (inl*) malloc(sizeof(inl));
	e->tag = t;
	e->next = NULL;
	return e;
}

// Macros for creating various kinds of inlines.
#define make_str(s) make_literal(INL_STRING, s)
#define make_code(s) make_literal(INL_CODE, s)
#define make_raw_html(s) make_literal(INL_RAW_HTML, s)
#define make_entity(s) make_literal(INL_ENTITY, s)
#define make_linebreak() make_simple(INL_LINEBREAK)
#define make_softbreak() make_simple(INL_SOFTBREAK)
#define make_emph(contents) make_inlines(INL_EMPH, contents)
#define make_strong(contents) make_inlines(INL_STRONG, contents)

// Free an inline list.
extern void free_inlines(inl* e)
{
	inl * next;
	while (e != NULL) {
		switch (e->tag){
			case INL_STRING:
			case INL_RAW_HTML:
			case INL_CODE:
			case INL_ENTITY:
				chunk_free(&e->content.literal);
				break;
			case INL_LINEBREAK:
			case INL_SOFTBREAK:
				break;
			case INL_LINK:
			case INL_IMAGE:
				free(e->content.linkable.url);
				free(e->content.linkable.title);
				free_inlines(e->content.linkable.label);
				break;
			case INL_EMPH:
			case INL_STRONG:
				free_inlines(e->content.inlines);
				break;
			default:
				break;
		}
		next = e->next;
		free(e);
		e = next;
	}
}

// Append inline list b to the end of inline list a.
// Return pointer to head of new list.
inline static inl* append_inlines(inl* a, inl* b)
{
	if (a == NULL) {  // NULL acts like an empty list
		return b;
	}
	inl* cur = a;
	while (cur->next) {
		cur = cur->next;
	}
	cur->next = b;
	return a;
}

static void subject_from_buf(subject *e, strbuf *buffer, reference** refmap)
{
	e->input.data = buffer->ptr;
	e->input.len = buffer->size;
	e->input.alloc = 0;
	e->pos = 0;
	e->label_nestlevel = 0;
	e->reference_map = refmap;

	chunk_rtrim(&e->input);
}

static void subject_from_chunk(subject *e, chunk *chunk, reference** refmap)
{
	e->input.data = chunk->data;
	e->input.len = chunk->len;
	e->input.alloc = 0;
	e->pos = 0;
	e->label_nestlevel = 0;
	e->reference_map = refmap;

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
	char c;
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

// Destructively modify string, collapsing consecutive
// space and newline characters into a single space.
static void normalize_whitespace(strbuf *s)
{
	bool last_char_was_space = false;
	int r, w;

	for (r = 0, w = 0; r < s->size; ++r) {
		switch (s->ptr[r]) {
		case ' ':
		case '\n':
			if (last_char_was_space)
				break;

			s->ptr[w++] = ' ';
			last_char_was_space = true;
			break;

		default:
			s->ptr[w++] = s->ptr[r];
			last_char_was_space = false;
		}
	}

	strbuf_truncate(s, w);
}

// Parse backtick code section or raw backticks, return an inline.
// Assumes that the subject has a backtick at the current position.
static inl* handle_backticks(subject *subj)
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
		normalize_whitespace(&buf);

		return make_code(chunk_buf_detach(&buf));
	}
}

// Scan ***, **, or * and return number scanned, or 0.
// Don't advance position.
static int scan_delims(subject* subj, char c, bool * can_open, bool * can_close)
{
	int numdelims = 0;
	char char_before, char_after;
	int startpos = subj->pos;

	char_before = subj->pos == 0 ? '\n' : peek_at(subj, subj->pos - 1);
	while (peek_char(subj) == c) {
		numdelims++;
		advance(subj);
	}
	char_after = peek_char(subj);
	*can_open = numdelims > 0 && numdelims <= 3 && !isspace(char_after);
	*can_close = numdelims > 0 && numdelims <= 3 && !isspace(char_before);
	if (c == '_') {
		*can_open = *can_open && !isalnum(char_before);
		*can_close = *can_close && !isalnum(char_after);
	}
	subj->pos = startpos;
	return numdelims;
}

// Parse strong/emph or a fallback.
// Assumes the subject has '_' or '*' at the current position.
static inl* handle_strong_emph(subject* subj, char c)
{
	bool can_open, can_close;
	inl * result = NULL;
	inl ** last = malloc(sizeof(inl *));
	inl * new;
	inl * il;
	inl * first_head = NULL;
	inl * first_close = NULL;
	int first_close_delims = 0;
	int numdelims;

	*last = NULL;

	numdelims = scan_delims(subj, c, &can_open, &can_close);
	subj->pos += numdelims;

	new = make_str(chunk_dup(&subj->input, subj->pos - numdelims, numdelims));
	*last = new;
	first_head = new;
	result = new;

	if (!can_open || numdelims == 0) {
		goto done;
	}

	switch (numdelims) {
		case 1:
			while (true) {
				numdelims = scan_delims(subj, c, &can_open, &can_close);
				if (numdelims >= 1 && can_close) {
					subj->pos += 1;
					first_head->tag = INL_EMPH;
					chunk_free(&first_head->content.literal);
					first_head->content.inlines = first_head->next;
					first_head->next = NULL;
					goto done;
				} else {
					if (!parse_inline(subj, last)) {
						goto done;
					}
				}
			}
			break;
		case 2:
			while (true) {
				numdelims = scan_delims(subj, c, &can_open, &can_close);
				if (numdelims >= 2 && can_close) {
					subj->pos += 2;
					first_head->tag = INL_STRONG;
					chunk_free(&first_head->content.literal);
					first_head->content.inlines = first_head->next;
					first_head->next = NULL;
					goto done;
				} else {
					if (!parse_inline(subj, last)) {
						goto done;
					}
				}
			}
			break;
		case 3:
			while (true) {
				numdelims = scan_delims(subj, c, &can_open, &can_close);
				if (can_close && numdelims >= 1 && numdelims <= 3 &&
						numdelims != first_close_delims) {
					new = make_str(chunk_dup(&subj->input, subj->pos, numdelims));
					append_inlines(*last, new);
					*last = new;
					if (first_close_delims == 1 && numdelims > 2) {
						numdelims = 2;
					} else if (first_close_delims == 2) {
						numdelims = 1;
					} else if (numdelims == 3) {
						// If we opened with ***, we interpret it as ** followed by *
						// giving us <strong><em>
						numdelims = 1;
					}
					subj->pos += numdelims;
					if (first_close) {
						first_head->tag = first_close_delims == 1 ? INL_STRONG : INL_EMPH;
						chunk_free(&first_head->content.literal);
						first_head->content.inlines =
							make_inlines(first_close_delims == 1 ? INL_EMPH : INL_STRONG,
									first_head->next);

						il = first_head->next;
						while (il->next && il->next != first_close) {
							il = il->next;
						}
						il->next = NULL;

						first_head->content.inlines->next = first_close->next;

						il = first_head->content.inlines;
						while (il->next && il->next != *last) {
							il = il->next;
						}
						il->next = NULL;
						free_inlines(*last);

						first_close->next = NULL;
						free_inlines(first_close);
						first_head->next = NULL;
						goto done;
					} else {
						first_close = *last;
						first_close_delims = numdelims;
					}
				} else {
					if (!parse_inline(subj, last)) {
						goto done;
					}
				}
			}
			break;
		default:
			goto done;
	}

done:
	free(last);
	return result;
}

// Parse backslash-escape or just a backslash, returning an inline.
static inl* handle_backslash(subject *subj)
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
static inl* handle_entity(subject* subj)
{
	int match;
	inl *result;
	match = scan_entity(&subj->input, subj->pos);
	if (match) {
		result = make_entity(chunk_dup(&subj->input, subj->pos, match));
		subj->pos += match;
	} else {
		advance(subj);
		result = make_str(chunk_literal("&"));
	}
	return result;
}

// Like make_str, but parses entities.
// Returns an inline sequence consisting of str and entity elements.
static inl *make_str_with_entities(chunk *content)
{
	inl *result = NULL;
	inl *new;
	int searchpos;
	char c;
	subject subj;

	subject_from_chunk(&subj, content, NULL);

	while ((c = peek_char(&subj))) {
		switch (c) {
			case '&':
				new = handle_entity(&subj);
				break;
			default:
				searchpos = chunk_strchr(&subj.input, '&', subj.pos);
				new = make_str(chunk_dup(&subj.input, subj.pos, searchpos - subj.pos));
				subj.pos = searchpos;
		}
		result = append_inlines(result, new);
	}

	return result;
}

// Destructively unescape a string: remove backslashes before punctuation chars.
extern void unescape_buffer(strbuf *buf)
{
	int r, w;

	for (r = 0, w = 0; r < buf->size; ++r) {
		if (buf->ptr[r] == '\\' && ispunct(buf->ptr[r + 1]))
			continue;

		buf->ptr[w++] = buf->ptr[r];
	}

	strbuf_truncate(buf, w);
}

// Clean a URL: remove surrounding whitespace and surrounding <>,
// and remove \ that escape punctuation.
static unsigned char *clean_url(chunk *url, int is_email)
{
	strbuf buf = GH_BUF_INIT;

	chunk_trim(url);

	if (url->len == 0)
		return NULL;

	if (is_email)
		strbuf_puts(&buf, "mailto:");

	if (url->data[0] == '<' && url->data[url->len - 1] == '>') {
		strbuf_put(&buf, url->data + 1, url->len - 2);
	} else {
		strbuf_put(&buf, url->data, url->len);
	}

	unescape_buffer(&buf);
	return strbuf_detach(&buf);
}

// Clean a title: remove surrounding quotes and remove \ that escape punctuation.
static unsigned char *clean_title(chunk *title)
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
		strbuf_set(&buf, title->data + 1, title->len - 2);
	} else {
		strbuf_set(&buf, title->data, title->len);
	}

	unescape_buffer(&buf);
	return strbuf_detach(&buf);
}

// Parse an autolink or HTML tag.
// Assumes the subject has a '<' character at the current position.
static inl* handle_pointy_brace(subject* subj)
{
	int matchlen = 0;
	chunk contents;

	advance(subj);  // advance past first <

	// first try to match a URL autolink
	matchlen = scan_autolink_uri(&subj->input, subj->pos);
	if (matchlen > 0) {
		contents = chunk_dup(&subj->input, subj->pos, matchlen - 1);
		subj->pos += matchlen;

		return make_link(
			make_str_with_entities(&contents),
			contents,
			chunk_literal(""),
			0
		);
	}

	// next try to match an email autolink
	matchlen = scan_autolink_email(&subj->input, subj->pos);
	if (matchlen > 0) {
		contents = chunk_dup(&subj->input, subj->pos, matchlen - 1);
		subj->pos += matchlen;

		return make_link(
				make_str_with_entities(&contents),
				contents,
				chunk_literal(""),
				1
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
// Unless raw_label is null, it is set to point to the raw contents of the [].
// Assumes the subject has a '[' character at the current position.
// Returns 0 and does not advance if no matching ] is found.
// Note the precedence:  code backticks have precedence over label bracket
// markers, which have precedence over *, _, and other inline formatting
// markers. So, 2 below contains a link while 1 does not:
// 1. [a link `with a ](/url)` character
// 2. [a link *with emphasized ](/url) text*
static int link_label(subject* subj, chunk *raw_label)
{
	int nestlevel = 0;
	inl* tmp = NULL;
	int startpos = subj->pos;

	if (subj->label_nestlevel) {
		// if we've already checked to the end of the subject
		// for a label, even with a different starting [, we
		// know we won't find one here and we can just return.
		// Note:  nestlevel 1 would be: [foo [bar]
		// nestlevel 2 would be: [foo [bar [baz]
		subj->label_nestlevel--;
		return 0;
	}

	advance(subj);  // advance past [
	char c;
	while ((c = peek_char(subj)) && (c != ']' || nestlevel > 0)) {
		switch (c) {
			case '`':
				tmp = handle_backticks(subj);
				free_inlines(tmp);
				break;
			case '<':
				tmp = handle_pointy_brace(subj);
				free_inlines(tmp);
				break;
			case '[':  // nested []
				nestlevel++;
				advance(subj);
				break;
			case ']':  // nested []
				nestlevel--;
				advance(subj);
				break;
			case '\\':
				advance(subj);
				if (ispunct(peek_char(subj))) {
					advance(subj);
				}
				break;
			default:
				advance(subj);
		}
	}
	if (c == ']') {
		*raw_label = chunk_dup(&subj->input, startpos + 1, subj->pos - (startpos + 1));
		subj->label_nestlevel = 0;
		advance(subj);  // advance past ]
		return 1;
	} else {
		if (c == 0) {
			subj->label_nestlevel = nestlevel;
		}
		subj->pos = startpos; // rewind
		return 0;
	}
}

// Parse a link or the link portion of an image, or return a fallback.
static inl* handle_left_bracket(subject* subj)
{
	inl *lab = NULL;
	inl *result = NULL;
	reference *ref;
	int n;
	int sps;
	int found_label;
	int endlabel, starturl, endurl, starttitle, endtitle, endall;

	chunk rawlabel;
	chunk url, title;

	found_label = link_label(subj, &rawlabel);
	endlabel = subj->pos;

	if (found_label) {
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

				url = chunk_dup(&subj->input, starturl, endurl - starturl);
				title = chunk_dup(&subj->input, starttitle, endtitle - starttitle);
				lab = parse_chunk_inlines(&rawlabel, NULL);

				return make_link(lab, url, title, 0);
			} else {
				// if we get here, we matched a label but didn't get further:
				subj->pos = endlabel;
				lab = parse_chunk_inlines(&rawlabel, subj->reference_map);
				result = append_inlines(make_str(chunk_literal("[")),
						append_inlines(lab,
							make_str(chunk_literal("]"))));
				return result;
			}
		} else {
			chunk rawlabel_tmp;
			chunk reflabel;

			// Check for reference link.
			// First, see if there's another label:
			subj->pos = subj->pos + scan_spacechars(&subj->input, endlabel);
			reflabel = rawlabel;

			// if followed by a nonempty link label, we change reflabel to it:
			if (peek_char(subj) == '[' && link_label(subj, &rawlabel_tmp)) {
				if (rawlabel_tmp.len > 0)
					reflabel = rawlabel_tmp;
			} else {
				subj->pos = endlabel;
			}

			// lookup rawlabel in subject->reference_map:
			ref = lookup_reference(subj->reference_map, &reflabel);
			if (ref != NULL) { // found
				lab = parse_chunk_inlines(&rawlabel, NULL);
				result = make_link_from_reference(lab, ref);
			} else {
				subj->pos = endlabel;
				lab = parse_chunk_inlines(&rawlabel, subj->reference_map);
				result = append_inlines(make_str(chunk_literal("[")),
						append_inlines(lab, make_str(chunk_literal("]"))));
			}
			return result;
		}
	}
	// If we fall through to here, it means we didn't match a link:
	advance(subj);  // advance past [
	return make_str(chunk_literal("["));
}

// Parse a hard or soft linebreak, returning an inline.
// Assumes the subject has a newline at the current position.
static inl* handle_newline(subject *subj)
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

inline static int not_eof(subject* subj)
{
	return !is_eof(subj);
}

// Parse inlines while a predicate is satisfied.  Return inlines.
extern inl* parse_inlines_while(subject* subj, int (*f)(subject*))
{
	inl* result = NULL;
	inl** last = &result;
	while ((*f)(subj) && parse_inline(subj, last)) {
	}
	return result;
}

inl *parse_chunk_inlines(chunk *chunk, reference** refmap)
{
	subject subj;
	subject_from_chunk(&subj, chunk, refmap);
	return parse_inlines_while(&subj, not_eof);
}

static int subject_find_special_char(subject *subj)
{
	int n = subj->pos + 1;

	while (n < subj->input.len) {
		if (strchr("\n\\`&_*[]<!", subj->input.data[n]))
			return n;
		n++;
	}

	return subj->input.len;
}

// Parse an inline, advancing subject, and add it to last element.
// Adjust tail to point to new last element of list.
// Return 0 if no inline can be parsed, 1 otherwise.
static int parse_inline(subject* subj, inl ** last)
{
	inl* new = NULL;
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
			if (subj->pos > 0) {
				unsigned char prev = peek_at(subj, subj->pos - 1);
				if (isalnum(prev) || prev == '_') {
					new = make_str(chunk_literal("_"));
					advance(subj);
					break;
				}
			}

			new = handle_strong_emph(subj, '_');
			break;
		case '*':
			new = handle_strong_emph(subj, '*');
			break;
		case '[':
			new = handle_left_bracket(subj);
			break;
		case '!':
			advance(subj);
			if (peek_char(subj) == '[') {
				new = handle_left_bracket(subj);
				if (new != NULL && new->tag == INL_LINK) {
					new->tag = INL_IMAGE;
				} else {
					new = append_inlines(make_str(chunk_literal("!")), new);
				}
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
	} else {
		append_inlines(*last, new);
	}
	return 1;
}

extern inl* parse_inlines(strbuf *input, reference** refmap)
{
	subject subj;
	subject_from_buf(&subj, input, refmap);
	return parse_inlines_while(&subj, not_eof);
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
extern int parse_reference(strbuf *input, reference** refmap)
{
	subject subj;

	chunk lab;
	chunk url;
	chunk title;

	int matchlen = 0;
	int beforetitle;
	reference *new = NULL;

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
	new = make_reference(&lab, &url, &title);
	add_reference(refmap, new);

	return subj.pos;
}

