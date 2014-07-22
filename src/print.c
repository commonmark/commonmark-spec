#include <stdlib.h>
#include <stdio.h>
#include "bstrlib.h"
#include "stmd.h"
#include "debug.h"

static bstring format_str(bstring s)
{
  int pos = 0;
  int len = blength(s);
  bstring result = bfromcstr("");
  char c;
  bformata(result, "\"");
  while (pos < len) {
    c = bchar(s, pos);
    switch (c) {
    case '\n':
      bformata(result, "\\n");
      break;
    case '"':
      bformata(result, "\\\"");
      break;
    case '\\':
      bformata(result, "\\\\");
      break;
    default:
      bformata(result, "%c", c);
    }
    pos++;
  }
  bformata(result, "\"");
  return result;
}

// Functions to pretty-print inline and block lists, for debugging.
// Prettyprint an inline list, for debugging.
extern void print_blocks(block* b, int indent)
{
  struct ListData * data;
  while(b != NULL) {
    // printf("%3d %3d %3d| ", b->start_line, b->start_column, b->end_line);
    for (int i=0; i < indent; i++) {
      putchar(' ');
    }
    switch(b->tag) {
    case document:
      printf("document\n");
      print_blocks(b->children, indent + 2);
      break;
    case block_quote:
      printf("block_quote\n");
      print_blocks(b->children, indent + 2);
      break;
    case list_item:
      data = &(b->attributes.list_data);
      printf("list_item\n");
      print_blocks(b->children, indent + 2);
      break;
    case list:
      data = &(b->attributes.list_data);
      if (data->list_type == ordered) {
        printf("list (type=ordered tight=%s start=%d delim=%s)\n",
               (data->tight ? "true" : "false"),
               data->start,
               (data->delimiter == parens ? "parens" : "period"));
      } else {
        printf("list (type=bullet tight=%s bullet_char=%c)\n",
               (data->tight ? "true" : "false"),
               data->bullet_char);
      }
      print_blocks(b->children, indent + 2);
      break;
    case atx_header:
      printf("atx_header (level=%d)\n", b->attributes.header_level);
      print_inlines(b->inline_content, indent + 2);
      break;
    case setext_header:
      printf("setext_header (level=%d)\n", b->attributes.header_level);
      print_inlines(b->inline_content, indent + 2);
      break;
    case paragraph:
      printf("paragraph\n");
      print_inlines(b->inline_content, indent + 2);
      break;
    case hrule:
      printf("hrule\n");
      break;
    case indented_code:
      printf("indented_code %s\n", format_str(b->string_content)->data);
      break;
    case fenced_code:
      printf("fenced_code length=%d info=%s %s\n",
             b->attributes.fenced_code_data.fence_length,
             format_str(b->attributes.fenced_code_data.info)->data,
             format_str(b->string_content)->data);
      break;
    case html_block:
      printf("html_block %s\n", format_str(b->string_content)->data);
      break;
    case reference_def:
      printf("reference_def\n");
      break;
    default:
      log_warn("block type %d not implemented\n", b->tag);
      break;
    }
    b = b->next;
  }
}

// Prettyprint an inline list, for debugging.
extern void print_inlines(inl* ils, int indent)
{
  while(ils != NULL) {
    /*
    // we add 11 extra spaces for the line/column info
    for (int i=0; i < 11; i++) {
      putchar(' ');
    }
    putchar('|');
    putchar(' ');
    */
    for (int i=0; i < indent; i++) {
      putchar(' ');
    }
    switch(ils->tag) {
    case str:
      printf("str %s\n", format_str(ils->content.literal)->data);
      break;
    case linebreak:
      printf("linebreak\n");
      break;
    case softbreak:
      printf("softbreak\n");
      break;
    case code:
      printf("code %s\n", format_str(ils->content.literal)->data);
      break;
    case raw_html:
      printf("html %s\n", format_str(ils->content.literal)->data);
      break;
    case entity:
      printf("entity %s\n", format_str(ils->content.literal)->data);
      break;
    case link:
      printf("link url=%s title=%s\n",
             format_str(ils->content.linkable.url)->data,
             format_str(ils->content.linkable.title)->data);
      print_inlines(ils->content.linkable.label, indent + 2);
      break;
    case image:
      printf("image url=%s title=%s\n",
             format_str(ils->content.linkable.url)->data,
             format_str(ils->content.linkable.title)->data);
      print_inlines(ils->content.linkable.label, indent + 2);
      break;
    case strong:
      printf("strong\n");
      print_inlines(ils->content.linkable.label, indent + 2);
      break;
    case emph:
      printf("emph\n");
      print_inlines(ils->content.linkable.label, indent + 2);
      break;
    }
    ils = ils->next;
  }
}
