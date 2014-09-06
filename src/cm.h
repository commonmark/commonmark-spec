#ifndef HAVE_STMD_H
#define HAVE_STMD_H
#include <stdbool.h>
#include <bstrlib.h>
#include <uthash.h>

#define VERSION "0.1"
#define CODE_INDENT 4

#ifdef PHP_WIN32
# define STMD_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define STMD_API __attribute__ ((visibility("default")))
#else
# define STMD_API
#endif

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

STMD_API int parse_inline(subject* subj, inl ** last);
STMD_API inl* parse_inlines(bstring input, reference** refmap);
STMD_API inl* parse_inlines_while(subject* subj, int (*f)(subject*));
STMD_API void free_inlines(inl* e);
STMD_API int parse_reference(bstring input, reference** refmap);
STMD_API void free_reference(reference *ref);
STMD_API void free_reference_map(reference **refmap);
STMD_API reference* make_reference(bstring label, bstring url, bstring title);
STMD_API reference* lookup_reference(reference** refmap, bstring label);
STMD_API void add_reference(reference** refmap, reference* ref);
STMD_API int unescape(bstring s);

STMD_API block* make_document();
STMD_API block* add_child(block* parent,
                        int block_type, int start_line, int start_column);
STMD_API void free_blocks(block* e);

// FOR NOW:
STMD_API int process_inlines(block* cur, reference** refmap);
STMD_API int incorporate_line(bstring ln, int line_number, block** curptr);
STMD_API int finalize(block* b, int line_number);

STMD_API void print_inlines(inl* ils, int indent);
STMD_API void print_blocks(block* blk, int indent);

STMD_API int blocks_to_html(block* b, bstring* result, bool tight);
STMD_API int inlines_to_html(inl* b, bstring* result);

STMD_API int bdetab(bstring s, int utf8);
#endif
