#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "cmark.h"
#include "debug.h"
#include "bench.h"

void print_usage()
{
	printf("Usage:   cmark [FILE*]\n");
	printf("Options: --help, -h    Print usage information\n");
	printf("         --ast         Print AST instead of HTML\n");
	printf("         --version     Print version\n");
}

static void print_document(cmark_node *document, bool ast)
{
	char *result;
	if (ast) {
		result = cmark_render_ast(document);
	} else {

		result = cmark_render_html(document);
	}
	printf("%s", result);
	free(result);
}

int main(int argc, char *argv[])
{
	int i, numfps = 0;
	bool ast = false;
	int *files;
	char buffer[4096];
	cmark_doc_parser *parser;
	size_t offset;
	cmark_node *document;

	parser = cmark_new_doc_parser();
	files = (int *)malloc(argc * sizeof(*files));

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--version") == 0) {
			printf("cmark %s", CMARK_VERSION);
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
			files[numfps++] = i;
		}
	}

	for (i = 0; i < numfps; i++) {
		FILE *fp = fopen(argv[files[i]], "r");
		if (fp == NULL) {
			fprintf(stderr, "Error opening file %s: %s\n",
				argv[files[i]], strerror(errno));
			exit(1);
		}

		start_timer();
		while (fgets((char *)buffer, sizeof(buffer), fp)) {
			offset = strlen((char *)buffer);
			cmark_process_line(parser, buffer, offset);
		}
		end_timer("processing lines");

		fclose(fp);
	}

	if (numfps == 0) {
		/*
		document = cmark_parse_file(stdin);
		print_document(document, ast);
		exit(0);
		*/

		while (fgets((char *)buffer, sizeof(buffer), stdin)) {
			offset = strlen((char *)buffer);
			cmark_process_line(parser, buffer, offset);
		}
	}

	start_timer();
	document = cmark_finish(parser);
	end_timer("finishing document");
	cmark_free_doc_parser(parser);

	start_timer();
	print_document(document, ast);
	end_timer("print_document");

	start_timer();
	cmark_free_nodes(document);
	end_timer("free_blocks");

	free(files);

	return 0;
}
