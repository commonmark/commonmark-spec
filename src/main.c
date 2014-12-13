#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "cmark.h"
#include "debug.h"
#include "bench.h"

typedef enum {
	FORMAT_NONE,
	FORMAT_HTML,
	FORMAT_MAN,
	FORMAT_AST
} writer_format;

void print_usage()
{
	printf("Usage:   cmark [FILE*]\n");
	printf("Options:\n");
	printf("  --to, -t FORMAT  Specify output format (html, man, ast)\n");
	printf("  --help, -h       Print usage information\n");
	printf("  --version        Print version\n");
}

static void print_document(cmark_node *document, writer_format writer)
{
	char *result;
	switch (writer) {
	case FORMAT_AST:
		result = cmark_render_ast(document);
		break;
	case FORMAT_HTML:
		result = cmark_render_html(document);
		break;
	case FORMAT_MAN:
		result = cmark_render_man(document);
		break;
	default:
		fprintf(stderr, "Unknown format %d\n", writer);
		exit(1);
	}
	printf("%s", result);
	free(result);
}

int main(int argc, char *argv[])
{
	int i, numfps = 0;
	int *files;
	char buffer[4096];
	cmark_parser *parser;
	size_t bytes;
	cmark_node *document;
	writer_format writer = FORMAT_HTML;

	parser = cmark_parser_new();
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
		} else if ((strcmp(argv[i], "-t") == 0) ||
			   (strcmp(argv[i], "--to") == 0)) {
			i += 1;
			if (i < argc) {
				if (strcmp(argv[i], "man") == 0) {
					writer = FORMAT_MAN;
				} else if (strcmp(argv[i], "html") == 0) {
					writer = FORMAT_HTML;
				} else if (strcmp(argv[i], "ast") == 0) {
					writer = FORMAT_AST;
				} else {
					fprintf(stderr,
						"Unknown format %s\n", argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "No argument provided for %s\n",
					argv[i - 1]);
				exit(1);
			}
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
		while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			cmark_parser_feed(parser, buffer, bytes);
			if (bytes < sizeof(buffer)) {
				break;
			}
		}
		end_timer("processing lines");

		fclose(fp);
	}

	if (numfps == 0) {

		while ((bytes = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
			cmark_parser_feed(parser, buffer, bytes);
			if (bytes < sizeof(buffer)) {
				break;
			}
		}
	}

	start_timer();
	document = cmark_parser_finish(parser);
	end_timer("finishing document");
	cmark_parser_free(parser);

	start_timer();
	print_document(document, writer);
	end_timer("print_document");

	start_timer();
	cmark_node_free(document);
	end_timer("free_blocks");

	free(files);

	return 0;
}
