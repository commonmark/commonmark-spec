#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "bstrlib.h"
#include "stmd.h"
#include "debug.h"
#include "scanners.h"

// Functions to convert block and inline lists to HTML strings.

// Escape special characters in HTML.  More efficient than
// three calls to bfindreplace.  If preserve_entities is set,
// existing entities are left alone.
static bstring escape_html(bstring inp, bool preserve_entities)
{
  int pos = 0;
  int match;
  char c;
  bstring escapable = blk2bstr("&<>\"", 4);
  bstring ent;
  bstring s = bstrcpy(inp);
  while ((pos = binchr(s, pos, escapable)) != BSTR_ERR) {
    c = bchar(s,pos);
    switch (c) {
    case '<':
      bdelete(s, pos, 1);
      ent = blk2bstr("&lt;", 4);
      binsert(s, pos, ent, ' ');
      bdestroy(ent);
      pos += 4;
      break;
    case '>':
      bdelete(s, pos, 1);
      ent = blk2bstr("&gt;", 4);
      binsert(s, pos, ent, ' ');
      bdestroy(ent);
      pos += 4;
      break;
    case '&':
      if (preserve_entities && (match = scan_entity(s, pos))) {
        pos += match;
      } else {
        bdelete(s, pos, 1);
        ent = blk2bstr("&amp;", 5);
        binsert(s, pos, ent, ' ');
        bdestroy(ent);
        pos += 5;
      }
      break;
    case '"':
      bdelete(s, pos, 1);
      ent = blk2bstr("&quot;", 6);
      binsert(s, pos, ent, ' ');
      bdestroy(ent);
      pos += 6;
      break;
    default:
      bdelete(s, pos, 1);
      log_err("unexpected character %02x", c);
    }
  }
  bdestroy(escapable);
  return s;
}

static inline void cr(bstring buffer)
{
  int c = bchar(buffer, blength(buffer) - 1);
  if (c != '\n' && c) {
    bconchar(buffer, '\n');
  }
}

// Convert a block list to HTML.  Returns 0 on success, and sets result.
extern int blocks_to_html(block* b, bstring* result, bool tight)
{
  bstring contents = NULL;
  bstring escaped, escaped2;
  struct bstrList * info_words;
  struct ListData * data;
  bstring mbstart;
  bstring html = blk2bstr("", 0);

  while(b != NULL) {
    switch(b->tag) {
    case document:
      check(blocks_to_html(b->children, &contents, false) == 0,
            "error converting blocks to html");
      bformata(html, "%s", contents->data);
      bdestroy(contents);
      break;
    case paragraph:
      check(inlines_to_html(b->inline_content, &contents) == 0,
            "error converting inlines to html");
      if (tight) {
        bformata(html, "%s", contents->data);
      } else {
        cr(html);
        bformata(html, "<p>%s</p>", contents->data);
        cr(html);
      }
      bdestroy(contents);
      break;
    case block_quote:
      check(blocks_to_html(b->children, &contents, false) == 0,
            "error converting blocks to html");
      cr(html);
      bformata(html, "<blockquote>\n%s</blockquote>", contents->data);
      cr(html);
      bdestroy(contents);
      break;
    case list_item:
      check(blocks_to_html(b->children, &contents, tight) == 0,
            "error converting blocks to html");
      brtrimws(contents);
      cr(html);
      bformata(html, "<li>%s</li>", contents->data);
      cr(html);
      bdestroy(contents);
      break;
    case list:
      // make sure a list starts at the beginning of the line:
      cr(html);
      data = &(b->attributes.list_data);
      check(blocks_to_html(b->children, &contents, data->tight) == 0,
            "error converting blocks to html");
      mbstart = bformat(" start=\"%d\"", data->start);
      bformata(html, "<%s%s>\n%s</%s>",
               data->list_type == bullet ? "ul" : "ol",
               data->start == 1 ? "" : (char*) mbstart->data,
               contents->data,
               data->list_type == bullet ? "ul" : "ol");
      cr(html);
      bdestroy(contents);
      bdestroy(mbstart);
      break;
    case atx_header:
    case setext_header:
      check(inlines_to_html(b->inline_content, &contents) == 0,
            "error converting inlines to html");
      cr(html);
      bformata(html, "<h%d>%s</h%d>",
               b->attributes.header_level,
               contents->data,
               b->attributes.header_level);
      cr(html);
      bdestroy(contents);
      break;
    case indented_code:
      escaped = escape_html(b->string_content, false);
      cr(html);
      bformata(html, "<pre><code>%s</code></pre>", escaped->data);
      cr(html);
      bdestroy(escaped);
      break;
    case fenced_code:
      escaped = escape_html(b->string_content, false);
      cr(html);
      bformata(html, "<pre><code");
      if (blength(b->attributes.fenced_code_data.info) > 0) {
        escaped2 = escape_html(b->attributes.fenced_code_data.info, true);
        info_words = bsplit(escaped2, ' ');
        bformata(html, " class=\"language-%s\"", info_words->entry[0]->data);
        bdestroy(escaped2);
        bstrListDestroy(info_words);
      }
      bformata(html, ">%s</code></pre>", escaped->data);
      cr(html);
      bdestroy(escaped);
      break;
    case html_block:
      bformata(html, "%s", b->string_content->data);
      break;
    case hrule:
      bformata(html, "<hr />");
      cr(html);
      break;
    case reference_def:
      break;
    default:
      log_warn("block type %d not implemented\n", b->tag);
      break;
    }
    b = b->next;
  }
  *result = html;
  return 0;
 error:
  return -1;
}

// Convert an inline list to HTML.  Returns 0 on success, and sets result.
extern int inlines_to_html(inl* ils, bstring* result)
{
  bstring contents = NULL;
  bstring html = blk2bstr("", 0);
  bstring mbtitle, escaped, escaped2;

  while(ils != NULL) {
    switch(ils->tag) {
    case str:
      escaped = escape_html(ils->content.literal, false);
      bformata(html, "%s", escaped->data);
      bdestroy(escaped);
      break;
    case linebreak:
      bformata(html, "<br />\n");
      break;
    case softbreak:
      bformata(html, "\n");
      break;
    case code:
      escaped = escape_html(ils->content.literal, false);
      bformata(html, "<code>%s</code>", escaped->data);
      bdestroy(escaped);
      break;
    case raw_html:
    case entity:
      bformata(html, "%s", ils->content.literal->data);
      break;
    case link:
      check(inlines_to_html(ils->content.inlines, &contents) == 0,
            "error converting inlines to html");
      if (blength(ils->content.linkable.title) > 0) {
        escaped = escape_html(ils->content.linkable.title, true);
        mbtitle = bformat(" title=\"%s\"", escaped->data);
        bdestroy(escaped);
      } else {
        mbtitle = blk2bstr("",0);
      }
      escaped = escape_html(ils->content.linkable.url, true);
      bformata(html, "<a href=\"%s\"%s>%s</a>",
               escaped->data,
               mbtitle->data,
               contents->data);
      bdestroy(escaped);
      bdestroy(mbtitle);
      bdestroy(contents);
      break;
    case image:
      check(inlines_to_html(ils->content.inlines, &contents) == 0,
            "error converting inlines to html");
      escaped  = escape_html(ils->content.linkable.url, true);
      escaped2 = escape_html(contents, false);
      bdestroy(contents);
      bformata(html, "<img src=\"%s\" alt=\"%s\"",
               escaped->data, escaped2->data);
      bdestroy(escaped);
      bdestroy(escaped2);
      if (blength(ils->content.linkable.title) > 0) {
        escaped = escape_html(ils->content.linkable.title, true);
        bformata(html, " title=\"%s\"", escaped->data);
        bdestroy(escaped);
      }
      bformata(html, " />");
      break;
    case strong:
      check(inlines_to_html(ils->content.inlines, &contents) == 0,
            "error converting inlines to html");
      bformata(html, "<strong>%s</strong>", contents->data);
      bdestroy(contents);
      break;
    case emph:
      check(inlines_to_html(ils->content.inlines, &contents) == 0,
            "error converting inlines to html");
      bformata(html, "<em>%s</em>", contents->data);
      bdestroy(contents);
      break;
    }
    ils = ils->next;
  }
  *result = html;
  return 0;
 error:
  return -1;
}
