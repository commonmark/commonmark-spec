#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "bstrlib.h"
#include "stmd.h"
#include "uthash.h"
#include "debug.h"
#include "scanners.h"

static block* make_block(int tag, int start_line, int start_column)
{
  block* e;
  e = (block*) malloc(sizeof(block));
  e->tag = tag;
  e->open = true;
  e->last_line_blank = false;
  e->start_line = start_line;
  e->start_column = start_column;
  e->end_line = start_line;
  e->children = NULL;
  e->last_child = NULL;
  e->parent = NULL;
  e->top = NULL;
  e->attributes.refmap = NULL;
  e->string_content = bfromcstr("");
  e->inline_content = NULL;
  e->next = NULL;
  e->prev = NULL;
  return e;
}

// Create a root document block.
extern block* make_document()
{
  block * e = make_block(document, 1, 1);
  reference * map = NULL;
  reference ** refmap;
  refmap = (reference**) malloc(sizeof(reference*));
  *refmap = map;
  e->attributes.refmap = refmap;
  e->top = e;
  return e;
}

// Returns true if line has only space characters, else false.
bool is_blank(bstring s, int offset)
{
  char c;
  while ((c = bchar(s, offset))) {
    if (c == '\n') {
      return true;
    } else if (c == ' ') {
      offset++;
    } else {
      return false;
    }
  }
  return true;
}

static inline bool can_contain(int parent_type, int child_type)
{
  return ( parent_type == document ||
           parent_type == block_quote ||
           parent_type == list_item ||
           (parent_type == list && child_type == list_item) );
}

static inline bool accepts_lines(int block_type)
{
  return (block_type == paragraph ||
          block_type == atx_header ||
          block_type == indented_code ||
          block_type == fenced_code);
}

static int add_line(block* block, bstring ln, int offset)
{
  bstring s = bmidstr(ln, offset, blength(ln) - offset);
  check(block->open, "attempted to add line (%s) to closed container (%d)",
        ln->data, block->tag);
  check(bformata(block->string_content, "%s", s->data) == 0,
        "could not append line to string_content");
  bdestroy(s);
  return 0;
 error:
  return -1;
}

static int remove_trailing_blank_lines(bstring ln)
{
  bstring tofind = bfromcstr(" \t\r\n");
  int pos;
  // find last nonspace:
  pos = bninchrr(ln, blength(ln) - 1, tofind);
  if (pos == BSTR_ERR) { // all spaces
    bassigncstr(ln, "");
  } else {
    // find next newline after it
    pos = bstrchrp(ln, '\n', pos);
    if (pos != BSTR_ERR) {
      check(bdelete(ln, pos, blength(ln) - pos) != BSTR_ERR,
        "failed to delete trailing blank lines");
    }
  }
  bdestroy(tofind);
  return 0;
 error:
  return -1;
}

// Check to see if a block ends with a blank line, descending
// if needed into lists and sublists.
static bool ends_with_blank_line(block* block)
{
  if (block->last_line_blank) {
    return true;
  }
  if ((block->tag == list || block->tag == list_item) && block->last_child) {
    return ends_with_blank_line(block->last_child);
  } else {
    return false;
  }
}

// Break out of all containing lists
static int break_out_of_lists(block ** bptr, int line_number)
{
  block * container = *bptr;
  block * b = container->top;
  // find first containing list:
  while (b && b->tag != list) {
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


extern int finalize(block* b, int line_number)
{
  int firstlinelen;
  int pos;
  block* item;
  block* subitem;

  check(b != NULL, "finalize called on null block");
  if (!b->open) {
    return 0; // don't do anything if the block is already closed
  }
  b->open = false;
  if (line_number > b->start_line) {
    b->end_line = line_number - 1;
  } else {
    b->end_line = line_number;
  }

  switch (b->tag) {

  case paragraph:
    pos = 0;
    while (bchar(b->string_content, 0) == '[' &&
           (pos = parse_reference(b->string_content,
                                  b->top->attributes.refmap))) {
      bdelete(b->string_content, 0, pos);
    }
    if (is_blank(b->string_content, 0)) {
      b->tag = reference_def;
    }
    break;

  case indented_code:
    remove_trailing_blank_lines(b->string_content);
    bformata(b->string_content, "\n");
    break;

  case fenced_code:
    // first line of contents becomes info
    firstlinelen = bstrchr(b->string_content, '\n');
    b->attributes.fenced_code_data.info =
      bmidstr(b->string_content, 0, firstlinelen);
    bdelete(b->string_content, 0, firstlinelen + 1); // +1 for \n
    btrimws(b->attributes.fenced_code_data.info);
    unescape(b->attributes.fenced_code_data.info);
    break;

  case list: // determine tight/loose status
    b->attributes.list_data.tight = true; // tight by default
    item = b->children;

    while (item) {
      // check for non-final non-empty list item ending with blank line:
      if (item->last_line_blank && item->next) {
        b->attributes.list_data.tight = false;
        break;
      }
      // recurse into children of list item, to see if there are
      // spaces between them:
      subitem = item->children;
      while (subitem) {
        if (ends_with_blank_line(subitem) &&
            (item->next || subitem->next)) {
          b->attributes.list_data.tight = false;
          break;
        }
        subitem = subitem->next;
      }
      if (!(b->attributes.list_data.tight)) {
        break;
      }
      item = item->next;
    }

    break;

  default:
    break;
  }

  return 0;
 error:
  return -1;
}

// Add a block as child of another.  Return pointer to child.
extern block* add_child(block* parent,
                        int block_type, int start_line, int start_column)
{
  // if 'parent' isn't the kind of block that can accept this child,
  // then back up til we hit a block that can.
  while (!can_contain(parent->tag, block_type)) {
    finalize(parent, start_line);
    parent = parent->parent;
  }

  check(parent != NULL, "parent container cannot accept children");

  block* child = make_block(block_type, start_line, start_column);
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
 error:
  return NULL;
}

// Free a block list and any children.
extern void free_blocks(block* e)
{
  block * next;
  while (e != NULL) {
    next = e->next;
    free_inlines(e->inline_content);
    bdestroy(e->string_content);
    if (e->tag == fenced_code) {
      bdestroy(e->attributes.fenced_code_data.info);
    } else if (e->tag == document) {
      free_reference_map(e->attributes.refmap);
    }
    free_blocks(e->children);
    free(e);
    e = next;
  }
}

// Walk through block and all children, recursively, parsing
// string content into inline content where appropriate.
int process_inlines(block* cur, reference** refmap)
{
  switch (cur->tag) {

  case paragraph:
  case atx_header:
  case setext_header:
    check(cur->string_content != NULL, "string_content is NULL");
    cur->inline_content = parse_inlines(cur->string_content, refmap);
    bdestroy(cur->string_content);
    cur->string_content = NULL;
    break;

  default:
    break;
  }

  block * child = cur->children;
  while (child != NULL) {
    process_inlines(child, refmap);
    child = child->next;
  }

  return 0;
 error:
  return -1;
}

// Attempts to parse a list item marker (bullet or enumerated).
// On success, returns length of the marker, and populates
// data with the details.  On failure, returns 0.
static int parse_list_marker(bstring ln, int pos,
                             struct ListData ** dataptr)
{
  char c;
  int startpos;
  int start = 1;
  struct ListData * data;

  startpos = pos;
  c = bchar(ln, pos);

  if ((c == '*' || c == '-' || c == '+') && !scan_hrule(ln, pos)) {
    pos++;
    if (!isspace(bchar(ln, pos))) {
      return 0;
    }
    data = malloc(sizeof(struct ListData));
    data->marker_offset = 0; // will be adjusted later
    data->list_type = bullet;
    data->bullet_char = c;
    data->start = 1;
    data->delimiter = period;
    data->tight = false;

  } else if (isdigit(c)) {

    pos++;
    while (isdigit(bchar(ln, pos))) {
      pos++;
    }

    if (!sscanf((char *) ln->data + startpos, "%d", &start)) {
      log_err("sscanf failed");
      return 0;
    }

    c = bchar(ln, pos);
    if (c == '.' || c == ')') {
      pos++;
      if (!isspace(bchar(ln, pos))) {
        return 0;
      }
      data = malloc(sizeof(struct ListData));
      data->marker_offset = 0; // will be adjusted later
      data->list_type = ordered;
      data->bullet_char = 0;
      data->start = start;
      data->delimiter = (c == '.' ? period : parens);
      data->tight = false;
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
static int lists_match(struct ListData list_data,
                       struct ListData item_data)
{
  return (list_data.list_type == item_data.list_type &&
          list_data.delimiter == item_data.delimiter &&
          // list_data.marker_offset == item_data.marker_offset &&
          list_data.bullet_char == item_data.bullet_char);
}

// Process one line at a time, modifying a block.
// Returns 0 if successful.  curptr is changed to point to
// the currently open block.
extern int incorporate_line(bstring ln, int line_number, block** curptr)
{
  block* last_matched_container;
  int offset = 0;
  int matched = 0;
  int lev = 0;
  int i;
  struct ListData * data = NULL;
  bool all_matched = true;
  block* container;
  block* cur = *curptr;
  bool blank = false;
  int first_nonspace;
  int indent;

  // detab input line
  check(bdetab(ln, 1) != BSTR_ERR,
        "invalid UTF-8 sequence in line %d\n", line_number);

  // container starts at the document root.
  container = cur->top;

  // for each containing block, try to parse the associated line start.
  // bail out on failure:  container will point to the last matching block.

  while (container->last_child && container->last_child->open) {
    container = container->last_child;

    first_nonspace = offset;
    while (bchar(ln, first_nonspace) == ' ') {
      first_nonspace++;
    }

    indent = first_nonspace - offset;
    blank = bchar(ln, first_nonspace) == '\n';

    if (container->tag == block_quote) {

      matched = indent <= 3 && bchar(ln, first_nonspace) == '>';
      if (matched) {
        offset = first_nonspace + 1;
        if (bchar(ln, offset) == ' ') {
          offset++;
        }
      } else {
        all_matched = false;
      }

    } else if (container->tag == list_item) {

      if (indent >= container->attributes.list_data.marker_offset +
          container->attributes.list_data.padding) {
        offset += container->attributes.list_data.marker_offset +
          container->attributes.list_data.padding;
      } else if (blank) {
        offset = first_nonspace;
      } else {
        all_matched = false;
      }

    } else if (container->tag == indented_code) {

      if (indent >= CODE_INDENT) {
        offset += CODE_INDENT;
      } else if (blank) {
        offset = first_nonspace;
      } else {
        all_matched = false;
      }

    } else if (container->tag == atx_header ||
               container->tag == setext_header) {

      // a header can never contain more than one line
      all_matched = false;

    } else if (container->tag == fenced_code) {

      // skip optional spaces of fence offset
      i = container->attributes.fenced_code_data.fence_offset;
      while (i > 0 && bchar(ln, offset) == ' ') {
        offset++;
        i--;
      }

    } else if (container->tag == html_block) {

      if (blank) {
        all_matched = false;
      }

    } else if (container->tag == paragraph) {

      if (blank) {
        container->last_line_blank =true;
        all_matched = false;
      }

    }

    if (!all_matched) {
      container = container->parent;  // back up to last matching block
      break;
    }
  }

  last_matched_container = container;

  // check to see if we've hit 2nd blank line, break out of list:
  if (blank && container->last_line_blank) {
    break_out_of_lists(&container, line_number);
  }

  // unless last matched container is code block, try new container starts:
  while (container->tag != fenced_code && container->tag != indented_code &&
         container->tag != html_block) {

    first_nonspace = offset;
    while (bchar(ln, first_nonspace) == ' ') {
      first_nonspace++;
    }

    indent = first_nonspace - offset;
    blank = bchar(ln, first_nonspace) == '\n';

    if (indent >= CODE_INDENT) {

      if (cur->tag != paragraph && !blank) {
        offset += CODE_INDENT;
        container = add_child(container, indented_code, line_number, offset + 1);
      } else { // indent > 4 in lazy line
        break;
      }

    } else if (bchar(ln, first_nonspace) == '>') {

      offset = first_nonspace + 1;
      // optional following character
      if (bchar(ln, offset) == ' ') {
        offset++;
      }
      container = add_child(container, block_quote, line_number, offset + 1);

    } else if ((matched = scan_atx_header_start(ln, first_nonspace))) {

      offset = first_nonspace + matched;
      container = add_child(container, atx_header, line_number, offset + 1);
      int hashpos = bstrchrp(ln, '#', first_nonspace);
      check(hashpos != BSTR_ERR, "no # found in atx header start");
      int level = 0;
      while (bchar(ln, hashpos) == '#') {
        level++;
        hashpos++;
      }
      container->attributes.header_level = level;

    } else if ((matched = scan_open_code_fence(ln, first_nonspace))) {

      container = add_child(container, fenced_code, line_number,
          first_nonspace + 1);
      container->attributes.fenced_code_data.fence_char = bchar(ln,
          first_nonspace);
      container->attributes.fenced_code_data.fence_length = matched;
      container->attributes.fenced_code_data.fence_offset =
        first_nonspace - offset;
      offset = first_nonspace + matched;

    } else if ((matched = scan_html_block_tag(ln, first_nonspace))) {

      container = add_child(container, html_block, line_number,
                            first_nonspace + 1);
      // note, we don't adjust offset because the tag is part of the text

    } else if (container->tag == paragraph &&
              (lev = scan_setext_header_line(ln, first_nonspace)) &&
               // check that there is only one line in the paragraph:
               bstrrchrp(container->string_content, '\n',
                         blength(container->string_content) - 2) == BSTR_ERR) {

        container->tag = setext_header;
        container->attributes.header_level = lev;
        offset = blength(ln) - 1;

    } else if (!(container->tag == paragraph && !all_matched) &&
               (matched = scan_hrule(ln, first_nonspace))) {

      // it's only now that we know the line is not part of a setext header:
      container = add_child(container, hrule, line_number, first_nonspace + 1);
      finalize(container, line_number);
      container = container->parent;
      offset = blength(ln) - 1;

    } else if ((matched = parse_list_marker(ln, first_nonspace, &data))) {

        // compute padding:
        offset = first_nonspace + matched;
        i = 0;
        while (i <= 5 && bchar(ln, offset + i) == ' ') {
          i++;
        }
        // i = number of spaces after marker, up to 5
        if (i >= 5 || i < 1 || bchar(ln, offset) == '\n') {
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

        if (container->tag != list ||
            !lists_match(container->attributes.list_data, *data)) {
          container = add_child(container, list, line_number,
              first_nonspace + 1);
          container->attributes.list_data = *data;
        }

        // add the list item
        container = add_child(container, list_item, line_number,
            first_nonspace + 1);
        container->attributes.list_data = *data;
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
  while (bchar(ln, first_nonspace) == ' ') {
    first_nonspace++;
  }

  indent = first_nonspace - offset;
  blank = bchar(ln, first_nonspace) == '\n';

  // block quote lines are never blank as they start with >
  // and we don't count blanks in fenced code for purposes of tight/loose
  // lists or breaking out of lists.  we also don't set last_line_blank
  // on an empty list item.
  container->last_line_blank = (blank &&
                                container->tag != block_quote &&
                                container->tag != fenced_code &&
                                !(container->tag == list_item &&
                                  container->children == NULL &&
                                  container->start_line == line_number));

    block *cont = container;
    while (cont->parent) {
      cont->parent->last_line_blank = false;
      cont = cont->parent;
    }

  if (cur != last_matched_container &&
      container == last_matched_container &&
      !blank &&
      cur->tag == paragraph &&
      blength(cur->string_content) > 0) {

    check(add_line(cur, ln, offset) == 0, "could not add line");

  } else { // not a lazy continuation

    // finalize any blocks that were not matched and set cur to container:
    while (cur != last_matched_container) {

      finalize(cur, line_number);
      cur = cur->parent;
      check(cur != NULL, "cur is NULL, last_matched_container->tag = %d",
            last_matched_container->tag);

    }

    if (container->tag == indented_code) {

      check(add_line(container, ln, offset) == 0, "could not add line");

    } else if (container->tag == fenced_code) {

      matched = (indent <= 3
        && bchar(ln, first_nonspace) == container->attributes.fenced_code_data.fence_char)
        && scan_close_code_fence(ln, first_nonspace,
                                 container->attributes.fenced_code_data.fence_length);
      if (matched) {
        // if closing fence, don't add line to container; instead, close it:
        finalize(container, line_number);
        container = container->parent; // back up to parent
      } else {
        check(add_line(container, ln, offset) == 0, "could not add line");
      }

    } else if (container->tag == html_block) {

      check(add_line(container, ln, offset) == 0, "could not add line");

    } else if (blank) {

      // ??? do nothing

    } else if (container->tag == atx_header) {

      // chop off trailing ###s...use a scanner?
      brtrimws(ln);
      int p = blength(ln) - 1;
      int numhashes = 0;
      // if string ends in #s, remove these:
      while (bchar(ln, p) == '#') {
        p--;
        numhashes++;
      }
      if (bchar(ln, p) == '\\') {
        // the last # was escaped, so we include it.
        p++;
        numhashes--;
      }
      check(bdelete(ln, p + 1, numhashes) != BSTR_ERR,
            "could not delete final hashes");
      check(add_line(container, ln, first_nonspace) == 0, "could not add line");
      finalize(container, line_number);
      container = container->parent;

    } else if (accepts_lines(container->tag)) {

      check(add_line(container, ln, first_nonspace) == 0, "could not add line");

    } else if (container->tag != hrule && container->tag != setext_header) {

      // create paragraph container for line
      container = add_child(container, paragraph, line_number, first_nonspace + 1);
      check(add_line(container, ln, first_nonspace) == 0, "could not add line");

    } else {

      log_warn("Line %d with container type %d did not match any condition:\n\"%s\"",
               line_number, container->tag, ln->data);

    }
    *curptr = container;
  }

  return 0;
 error:
  return -1;
}

