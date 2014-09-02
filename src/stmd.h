#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "uthash.h"

#define VERSION "0.1"
#define CODE_INDENT 4

typedef struct {
	const unsigned char *data;
	int len;
	int alloc;
} chunk;

typedef struct Inline {
	enum { INL_STRING, INL_SOFTBREAK, INL_LINEBREAK, INL_CODE, INL_RAW_HTML, INL_ENTITY,
		INL_EMPH, INL_STRONG, INL_LINK, INL_IMAGE } tag;
	union {
		chunk literal;
		struct Inline *inlines;
		struct {
			struct Inline *label;
			unsigned char *url;
			unsigned char *title;
		} linkable;
	} content;
	struct Inline *next;
} inl;

typedef struct Reference {
  unsigned char *label;
  unsigned char *url;
  unsigned char *title;
  UT_hash_handle  hh;      // used by uthash
} reference;

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
  gh_buf            info;
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
  gh_buf			 string_content;
  int				 string_pos;
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

inl* parse_inlines(gh_buf *input, int input_pos, reference** refmap);
void free_inlines(inl* e);

int parse_reference(gh_buf *input, int input_pos, reference** refmap);
void free_reference(reference *ref);
void free_reference_map(reference **refmap);

void add_reference(reference** refmap, reference* ref);
void unescape_buffer(gh_buf *buf);

extern block* make_document();
extern block* add_child(block* parent,
                        int block_type, int start_line, int start_column);
void free_blocks(block* e);

extern block *stmd_parse_document(const unsigned char *buffer, size_t len);
extern block *stmd_parse_file(FILE *f);

void print_inlines(inl* ils, int indent);
void print_blocks(block* blk, int indent);

void blocks_to_html(gh_buf *html, block *b, bool tight);
void inlines_to_html(gh_buf *html, inl *b);

void utf8proc_case_fold(gh_buf *dest, const unsigned char *str, int len);

