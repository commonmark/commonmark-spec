#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "bstrlib.h"
#include "stmd.h"
#include "uthash.h"
#include "debug.h"
#include "scanners.h"
#include "utf8.h"

extern void free_reference(reference *ref) {
  bdestroy(ref->label);
  bdestroy(ref->url);
  bdestroy(ref->title);
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
static bstring normalize_reference(bstring s)
{
  bstring normalized = case_fold(s);
  int pos = 0;
  int startpos;
  char c;
  while ((c = bchar(normalized, pos))) {
    if (isspace(c)) {
      startpos = pos;
      // skip til next non-space
      pos++;
      while (isspace(bchar(s, pos))) {
        pos++;
      }
      bdelete(normalized, startpos, pos - startpos);
      binsertch(normalized, startpos, 1, ' ');
      pos = startpos + 1;
    }
    pos++;
  }
  btrimws(normalized);
  return normalized;
}

// Returns reference if refmap contains a reference with matching
// label, otherwise NULL.
extern reference* lookup_reference(reference** refmap, bstring lab)
{
  reference * ref = NULL;
  bstring label = normalize_reference(lab);
  if (refmap != NULL) {
    HASH_FIND_STR(*refmap, (char*) label->data, ref);
  }
  bdestroy(label);
  return ref;
}

extern reference* make_reference(bstring label, bstring url, bstring title)
{
  reference * ref;
  ref = malloc(sizeof(reference));
  ref->label = normalize_reference(label);
  ref->url = bstrcpy(url);
  ref->title = bstrcpy(title);
  return ref;
}

extern void add_reference(reference** refmap, reference* ref)
{
  reference * t = NULL;
  HASH_FIND(hh, *refmap, (char*) ref->label->data,
            (unsigned) blength(ref->label), t);
  if (t == NULL) {
    HASH_ADD_KEYPTR(hh, *refmap, (char*) ref->label->data,
                    (unsigned) blength(ref->label), ref);
  } else {
    free_reference(ref);  // we free this now since it won't be in the refmap
  }
}

// Create an inline with a linkable string value.
inline static inl* make_linkable(int t, inl* label, bstring url, bstring title)
{
  inl* e = (inl*) malloc(sizeof(inl));
  e->tag = t;
  e->content.linkable.label = label;
  e->content.linkable.url   = url;
  e->content.linkable.title = title;
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
inline static inl* make_literal(int t, bstring s)
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
#define make_str(s) make_literal(str, s)
#define make_code(s) make_literal(code, s)
#define make_raw_html(s) make_literal(raw_html, s)
#define make_entity(s) make_literal(entity, s)
#define make_linebreak() make_simple(linebreak)
#define make_softbreak() make_simple(softbreak)
#define make_link(label, url, title) make_linkable(link, label, url, title)
#define make_image(alt, url, title) make_linkable(image, alt, url, title)
#define make_emph(contents) make_inlines(emph, contents)
#define make_strong(contents) make_inlines(strong, contents)

// Free an inline list.
extern void free_inlines(inl* e)
{
  inl * next;
  while (e != NULL) {
    switch (e->tag){
    case str:
    case raw_html:
    case code:
    case entity:
      bdestroy(e->content.literal);
      break;
    case linebreak:
    case softbreak:
      break;
    case link:
    case image:
      bdestroy(e->content.linkable.url);
      bdestroy(e->content.linkable.title);
      free_inlines(e->content.linkable.label);
      break;
    case emph:
    case strong:
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

// Make a 'subject' from an input string.
static subject* make_subject(bstring s, reference** refmap)
{
  subject* e = (subject*) malloc(sizeof(subject));
  // remove final whitespace
  brtrimws(s);
  e->buffer = s;
  e->pos = 0;
  e->label_nestlevel = 0;
  e->reference_map = refmap;
  return e;
}

inline static int isbacktick(int c)
{
  return (c == '`');
}

// Return the next character in the subject, without advancing.
// Return 0 if at the end of the subject.
#define peek_char(subj) bchar(subj->buffer, subj->pos)

// Return true if there are more characters in the subject.
inline static int is_eof(subject* subj)
{
  return (subj->pos >= blength(subj->buffer));
}

// Advance the subject.  Doesn't check for eof.
#define advance(subj) subj->pos += 1

// Take characters while a predicate holds, and return a string.
inline static bstring take_while(subject* subj, int (*f)(int))
{
  unsigned char c;
  int startpos = subj->pos;
  int len = 0;
  while ((c = peek_char(subj)) && (*f)(c)) {
    advance(subj);
    len++;
  }
  return bmidstr(subj->buffer, startpos, len);
}

// Take one character and return a string, or NULL if eof.
inline static bstring take_one(subject* subj)
{
  int startpos = subj->pos;
  if (is_eof(subj)){
    return NULL;
  } else {
    advance(subj);
    return bmidstr(subj->buffer, startpos, 1);
  }
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

// Destructively modify bstring, collapsing consecutive
// space and newline characters into a single space.
static int normalize_whitespace(bstring s)
{
  bool last_char_was_space = false;
  int pos = 0;
  char c;
  while ((c = bchar(s, pos))) {
    switch (c) {
    case ' ':
      if (last_char_was_space) {
        bdelete(s, pos, 1);
      } else {
        pos++;
      }
      last_char_was_space = true;
      break;
    case '\n':
      if (last_char_was_space) {
        bdelete(s, pos, 1);
      } else {
        bdelete(s, pos, 1);
        binsertch(s, pos, 1, ' ');
        pos++;
      }
      last_char_was_space = true;
      break;
    default:
      pos++;
      last_char_was_space = false;
    }
  }
  return 0;
}

// Parse backtick code section or raw backticks, return an inline.
// Assumes that the subject has a backtick at the current position.
static inl* handle_backticks(subject *subj)
{
  bstring openticks = take_while(subj, isbacktick);
  bstring result;
  int ticklength = blength(openticks);
  int startpos = subj->pos;
  int endpos = scan_to_closing_backticks(subj, ticklength);
  if (endpos == 0) { // not found
    subj->pos = startpos; // rewind
    return make_str(openticks);
  } else {
    bdestroy(openticks);
    result = bmidstr(subj->buffer, startpos, endpos - startpos - ticklength);
    btrimws(result);
    normalize_whitespace(result);
    return make_code(result);
  }
}

// Scan ***, **, or * and return number scanned, or 0.
// Don't advance position.
static int scan_delims(subject* subj, char c, bool * can_open, bool * can_close)
{
  int numdelims = 0;
  char char_before, char_after;
  int startpos = subj->pos;

  char_before = subj->pos == 0 ? '\n' : bchar(subj->buffer, subj->pos - 1);
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

  new = make_str(bmidstr(subj->buffer, subj->pos - numdelims, numdelims));
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
        first_head->tag = emph;
        bdestroy(first_head->content.literal);
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
        first_head->tag = strong;
        bdestroy(first_head->content.literal);
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
        new = make_str(bmidstr(subj->buffer, subj->pos, numdelims));
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
          first_head->tag = first_close_delims == 1 ? strong : emph;
          bdestroy(first_head->content.literal);
          first_head->content.inlines =
            make_inlines(first_close_delims == 1 ? emph : strong,
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
    return make_str(bformat("%c", nextchar));
  } else if (nextchar == '\n') {
    advance(subj);
    return make_linebreak();
  } else {
    return make_str(bfromcstr("\\"));
  }
}

// Parse an entity or a regular "&" string.
// Assumes the subject has an '&' character at the current position.
static inl* handle_entity(subject* subj)
{
  int match;
  inl * result;
  match = scan_entity(subj->buffer, subj->pos);
  if (match) {
    result = make_entity(bmidstr(subj->buffer, subj->pos, match));
    subj->pos += match;
  } else {
    advance(subj);
    result = make_str(bfromcstr("&"));
  }
  return result;
}

// Like make_str, but parses entities.
// Returns an inline sequence consisting of str and entity elements.
static inl * make_str_with_entities(bstring s)
{
  inl * result = NULL;
  inl * new;
  int searchpos;
  char c;
  subject * subj = make_subject(s, NULL);

  while ((c = peek_char(subj))) {
    switch (c) {
    case '&':
      new = handle_entity(subj);
      break;
    default:
      searchpos = bstrchrp(subj->buffer, '&', subj->pos);
      if (searchpos == BSTR_ERR) {
        searchpos = blength(subj->buffer);
      }
      new = make_str(bmidstr(subj->buffer, subj->pos, searchpos - subj->pos));
      subj->pos = searchpos;
    }
    result = append_inlines(result, new);
  }
  free(subj);
  return result;
}

// Destructively unescape a string: remove backslashes before punctuation chars.
extern int unescape(bstring url)
{
  // remove backslashes before punctuation chars:
  int searchpos = 0;
  while ((searchpos = bstrchrp(url, '\\', searchpos)) != BSTR_ERR) {
    if (ispunct(bchar(url, searchpos + 1))) {
      bdelete(url, searchpos, 1);
    } else {
      searchpos++;
    }
  }
  return 0;
}

// Clean a URL: remove surrounding whitespace and surrounding <>,
// and remove \ that escape punctuation.
static int clean_url(bstring url)
{
  // remove surrounding <> if any:
  int urllength = blength(url);
  btrimws(url);
  if (bchar(url, 0) == '<' && bchar(url, urllength - 1) == '>') {
    bdelete(url, 0, 1);
    bdelete(url, urllength - 2, 1);
  }
  unescape(url);
  return 0;
}

// Clean a title: remove surrounding quotes and remove \ that escape punctuation.
static int clean_title(bstring title)
{
  // remove surrounding quotes if any:
  int titlelength = blength(title);
  if ((bchar(title, 0) == '\'' && bchar(title, titlelength - 1) == '\'') ||
      (bchar(title, 0) == '(' && bchar(title, titlelength - 1) == ')') ||
      (bchar(title, 0) == '"' && bchar(title, titlelength - 1) == '"')) {
    bdelete(title, 0, 1);
    bdelete(title, titlelength - 2, 1);
  }
  unescape(title);
  return 0;
}

// Parse an autolink or HTML tag.
// Assumes the subject has a '<' character at the current position.
static inl* handle_pointy_brace(subject* subj)
{
  int matchlen = 0;
  bstring contents;
  inl* result;

  advance(subj);  // advance past first <
  // first try to match a URL autolink
  matchlen = scan_autolink_uri(subj->buffer, subj->pos);
  if (matchlen > 0) {
    contents = bmidstr(subj->buffer, subj->pos, matchlen - 1);
    subj->pos += matchlen;
    result =  make_link(make_str_with_entities(contents),
                        bstrcpy(contents), bfromcstr(""));
    bdestroy(contents);
    return result;
  }
  // next try to match an email autolink
  matchlen = scan_autolink_email(subj->buffer, subj->pos);
  if (matchlen > 0) {
    contents = bmidstr(subj->buffer, subj->pos, matchlen - 1);
    subj->pos += matchlen;
    result = make_link(make_str_with_entities(contents),
                       bformat("mailto:%s", contents->data),
                       bfromcstr(""));
    bdestroy(contents);
    return result;
  }
  // finally, try to match an html tag
  matchlen = scan_html_tag(subj->buffer, subj->pos);
  if (matchlen > 0) {
    contents = bmidstr(subj->buffer, subj->pos, matchlen);
    binsertch(contents, 0, 1, '<');
    subj->pos += matchlen;
    return make_raw_html(contents);
  } else {// if nothing matches, just return the opening <:
    return make_str(bfromcstr("<"));
  }
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
static int link_label(subject* subj, bstring* raw_label)
{
  int nestlevel = 0;
  inl* tmp = NULL;
  bstring raw;
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
    if (raw_label != NULL) {
      raw = bmidstr(subj->buffer, startpos + 1, subj->pos - (startpos + 1));
      *raw_label = raw;
    }
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
  inl* lab = NULL;
  inl* result = NULL;
  reference* ref;
  int n;
  int sps;
  int found_label;
  int endlabel, starturl, endurl, starttitle, endtitle, endall;
  bstring url, title, rawlabel, reflabel;
  bstring rawlabel2 = NULL;
  found_label = link_label(subj, &rawlabel);
  endlabel = subj->pos;
  if (found_label) {
    if (peek_char(subj) == '(' &&
        ((sps = scan_spacechars(subj->buffer, subj->pos + 1)) > -1) &&
        ((n = scan_link_url(subj->buffer, subj->pos + 1 + sps)) > -1)) {
      // try to parse an explicit link:
      starturl = subj->pos + 1 + sps; // after (
      endurl = starturl + n;
      starttitle = endurl + scan_spacechars(subj->buffer, endurl);
      // ensure there are spaces btw url and title
      endtitle = (starttitle == endurl) ? starttitle :
                 starttitle + scan_link_title(subj->buffer, starttitle);
      endall = endtitle + scan_spacechars(subj->buffer, endtitle);
      if (bchar(subj->buffer, endall) == ')') {
        subj->pos = endall + 1;
        url = bmidstr(subj->buffer, starturl, endurl - starturl);
        clean_url(url);
        title = bmidstr(subj->buffer, starttitle, endtitle - starttitle);
        clean_title(title);
        lab = parse_inlines(rawlabel, NULL);
        bdestroy(rawlabel);
        return make_link(lab, url, title);
      } else {
        // if we get here, we matched a label but didn't get further:
        subj->pos = endlabel;
        lab = parse_inlines(rawlabel, subj->reference_map);
        bdestroy(rawlabel);
        result = append_inlines(make_str(bfromcstr("[")),
                                append_inlines(lab,
                                               make_str(bfromcstr("]"))));
        return result;
      }
    } else {
      // Check for reference link.
      // First, see if there's another label:
      subj->pos = subj->pos + scan_spacechars(subj->buffer, endlabel);
      reflabel = rawlabel;
      // if followed by a nonempty link label, we change reflabel to it:
      if (peek_char(subj) == '[' &&
          link_label(subj, &rawlabel2)) {
        if (blength(rawlabel2) > 0) {
          reflabel = rawlabel2;
        }
      } else {
        subj->pos = endlabel;
      }
      // lookup rawlabel in subject->reference_map:
      ref = lookup_reference(subj->reference_map, reflabel);
      if (ref != NULL) { // found
        lab = parse_inlines(rawlabel, NULL);
        result = make_link(lab, bstrcpy(ref->url), bstrcpy(ref->title));
      } else {
        subj->pos = endlabel;
        lab = parse_inlines(rawlabel, subj->reference_map);
        result = append_inlines(make_str(bfromcstr("[")),
                               append_inlines(lab, make_str(bfromcstr("]"))));
      }
      bdestroy(rawlabel);
      bdestroy(rawlabel2);
      return result;
    }
  }
  // If we fall through to here, it means we didn't match a link:
  advance(subj);  // advance past [
  return make_str(bfromcstr("["));
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
      bchar(subj->buffer, nlpos - 1) == ' ' &&
      bchar(subj->buffer, nlpos - 2) == ' ') {
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

// Parse an inline, advancing subject, and add it to last element.
// Adjust tail to point to new last element of list.
// Return 0 if no inline can be parsed, 1 otherwise.
extern int parse_inline(subject* subj, inl ** last)
{
  inl* new = NULL;
  bstring contents;
  bstring special_chars;
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
      if (subj->pos > 0 && (isalnum(bchar(subj->buffer, subj->pos - 1)) ||
                            bchar(subj->buffer, subj->pos - 1) == '_')) {
        new = make_str(take_one(subj));
      } else {
        new = handle_strong_emph(subj, '_');
      }
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
        if (new != NULL && new->tag == link) {
          new->tag = image;
        } else {
          new = append_inlines(make_str(bfromcstr("!")), new);
        }
      } else {
        new = make_str(bfromcstr("!"));
      }
      break;
    default:
      // we read until we hit a special character
      special_chars = bfromcstr("\n\\`&_*[]<!");
      endpos = binchr(subj->buffer, subj->pos, special_chars);
      bdestroy(special_chars);
      if (endpos == subj->pos) {
        // current char is special: read a 1-character str
        contents = take_one(subj);
      } else if (endpos == BSTR_ERR) {
        // special char not found, take whole rest of buffer:
        endpos = subj->buffer->slen;
        contents = bmidstr(subj->buffer, subj->pos, endpos - subj->pos);
        subj->pos = endpos;
      } else {
        // take buffer from subj->pos to endpos to str.
        contents = bmidstr(subj->buffer, subj->pos, endpos - subj->pos);
        subj->pos = endpos;
        // if we're at a newline, strip trailing spaces.
        if (peek_char(subj) == '\n') {
          brtrimws(contents);
        }
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

extern inl* parse_inlines(bstring input, reference** refmap)
{
  subject * subj = make_subject(input, refmap);
  inl * result = parse_inlines_while(subj, not_eof);
  free(subj);
  return result;
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
extern int parse_reference(bstring input, reference** refmap)
{
  subject * subj = make_subject(input, NULL);
  bstring lab = NULL;
  bstring url = NULL;
  bstring title = NULL;
  int matchlen = 0;
  int beforetitle;
  reference * new = NULL;
  int newpos;

  // parse label:
  if (!link_label(subj, &lab)) {
    free(subj);
    return 0;
  }
  // colon:
  if (peek_char(subj) == ':') {
    advance(subj);
  } else {
    free(subj);
    bdestroy(lab);
    return 0;
  }
  // parse link url:
  spnl(subj);
  matchlen = scan_link_url(subj->buffer, subj->pos);
  if (matchlen) {
    url = bmidstr(subj->buffer, subj->pos, matchlen);
    clean_url(url);
    subj->pos += matchlen;
  } else {
    free(subj);
    bdestroy(lab);
    bdestroy(url);
    return 0;
  }
  // parse optional link_title
  beforetitle = subj->pos;
  spnl(subj);
  matchlen = scan_link_title(subj->buffer, subj->pos);
  if (matchlen) {
    title = bmidstr(subj->buffer, subj->pos, matchlen);
    clean_title(title);
    subj->pos += matchlen;
  } else {
    subj->pos = beforetitle;
    title = bfromcstr("");
  }
  // parse final spaces and newline:
  while (peek_char(subj) == ' ') {
    advance(subj);
  }
  if (peek_char(subj) == '\n') {
    advance(subj);
  } else if (peek_char(subj) != 0) {
    free(subj);
    bdestroy(lab);
    bdestroy(url);
    bdestroy(title);
    return 0;
  }
  // insert reference into refmap
  new = make_reference(lab, url, title);
  add_reference(refmap, new);

  newpos = subj->pos;
  free(subj);
  bdestroy(lab);
  bdestroy(url);
  bdestroy(title);
  return newpos;
}

