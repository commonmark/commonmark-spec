#include <stdbool.h>
#include "bstrlib.h"
#include "uthash.h"

#define VERSION "0.1"
#define CODE_INDENT 4

typedef struct Inline {
  enum { str, softbreak, linebreak, code, raw_html, entity,
         emph, strong, link, image } tag;
  union {
    bstring                  literal;
    struct Inline*           inlines;
    struct { struct Inline*  label;
             bstring         url;
             bstring         title;
           } linkable;
  } content;
  struct Inline*             next;
} inl;

typedef struct Reference {
  bstring         label;
  bstring         url;
  bstring         title;
  UT_hash_handle  hh;      // used by uthash
} reference;

typedef struct Subject {
  bstring        buffer;
  int            pos;
  reference**    reference_map;
  int            label_nestlevel;
} subject;

// Types for blocks

struct ListData {
  enum { bullet,
         ordered }  list_type;
  int               marker_offset;
  int               padding;
  int               start;
  enum { period,
         parens }   delimiter;
  unsigned char     bullet_char;
  bool              tight;
};

struct FencedCodeData {
  int               fence_length;
  int               fence_offset;
  char              fence_char;
  bstring           info;
};

typedef struct Block {
  enum { document,
         block_quote,
         list,
         list_item,
         fenced_code,
         indented_code,
         html_block,
         paragraph,
         atx_header,
         setext_header,
         hrule,
         reference_def
  }                  tag;
  int                start_line;
  int                start_column;
  int                end_line;
  bool               open;
  bool               last_line_blank;
  struct Block*      children;
  struct Block*      last_child;
  struct Block*      parent;
  struct Block*      top;
  bstring            string_content;
  inl*               inline_content;
  union  {
    struct ListData       list_data;
    struct FencedCodeData fenced_code_data;
    int                   header_level;
    reference**           refmap;
    }                     attributes;
  struct Block *     next;
  struct Block *     prev;
} block;

int parse_inline(subject* subj, inl ** last);
inl* parse_inlines(bstring input, reference** refmap);
inl* parse_inlines_while(subject* subj, int (*f)(subject*));
void free_inlines(inl* e);
int parse_reference(bstring input, reference** refmap);
void free_reference(reference *ref);
void free_reference_map(reference **refmap);
reference* make_reference(bstring label, bstring url, bstring title);
reference* lookup_reference(reference** refmap, bstring label);
void add_reference(reference** refmap, reference* ref);
int unescape(bstring s);

extern block* make_document();
extern block* add_child(block* parent,
                        int block_type, int start_line, int start_column);
void free_blocks(block* e);

// FOR NOW:
int process_inlines(block* cur, reference** refmap);
int incorporate_line(bstring ln, int line_number, block** curptr);
int finalize(block* b, int line_number);

void print_inlines(inl* ils, int indent);
void print_blocks(block* blk, int indent);

int blocks_to_html(block* b, bstring* result, bool tight);
int inlines_to_html(inl* b, bstring* result);

int bdetab(bstring s, int utf8);

