#include <stdlib.h>
#include <stdio.h>
#include "bstrlib.h"
#include "stmd.h"
#include "debug.h"

void print_usage()
{
  printf("Usage:   stmd [FILE*]\n");
  printf("Options: --help, -h    Print usage information\n");
  printf("         --ast         Print AST instead of HTML\n");
  printf("         --version     Print version\n");
}

int main(int argc, char *argv[]) {
  int i;
  bool ast = false;
  int g = 0;
  int numfps = 0;
  int files[argc];

  for (i=1; i < argc; i++) {
    if (strcmp(argv[i], "--version") == 0) {
      printf("stmd %s", VERSION);
      printf(" - CommonMark converter (c) 2014 John MacFarlane\n");
      exit(0);
    } else if ((strcmp(argv[i], "--help") == 0) ||
               (strcmp(argv[i], "-h") == 0)) {
      print_usage();
      exit(0);
    } else if (strcmp(argv[i], "--ast") == 0) {
      ast = true;
    } else if (*argv[i] == '-') {
      print_usage();
      exit(1);
    } else { // treat as file argument
      files[g] = i;
      g++;
    }
  }

  numfps = g;
  bstring s = NULL;
  bstring html;
  g = 0;
  block * cur = make_document();
  int linenum = 1;
  extern int errno;
  FILE * fp = NULL;

  if (numfps == 0) {
    // read from stdin
    while ((s = bgets((bNgetc) fgetc, stdin, '\n'))) {
      check(incorporate_line(s, linenum, &cur) == 0,
          "error incorporating line %d", linenum);
      bdestroy(s);
      linenum++;
    }
  } else {
    // iterate over input file pointers
    for (g=0; g < numfps; g++) {

      fp = fopen(argv[files[g]], "r");
      if (fp == NULL) {
        fprintf(stderr, "Error opening file %s: %s\n",
                argv[files[g]], strerror(errno));
        exit(1);
      }

      while ((s = bgets((bNgetc) fgetc, fp, '\n'))) {
        check(incorporate_line(s, linenum, &cur) == 0,
            "error incorporating line %d", linenum);
        bdestroy(s);
        linenum++;
      }
      fclose(fp);
    }
  }

  while (cur != cur->top) {
    finalize(cur, linenum);
    cur = cur->parent;
  }
  check(cur == cur->top, "problems finalizing open containers");
  finalize(cur, linenum);
  process_inlines(cur, cur->attributes.refmap);
  if (ast) {
    print_blocks(cur, 0);
  } else {
    check(blocks_to_html(cur, &html, false) == 0, "could not format as HTML");
    printf("%s", html->data);
    bdestroy(html);
  }
  free_blocks(cur);
  return 0;
error:
  return -1;
}

