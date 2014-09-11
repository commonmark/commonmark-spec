/* $Id: getopt.c 4022 2008-03-31 06:11:07Z rra $
 *
 * Replacement implementation of getopt.
 *
 * This is a replacement implementation for getopt based on the my_getopt
 * distribution by Benjamin Sittler.  Only the getopt interface is included,
 * since remctl doesn't use GNU long options, and the code has been rearranged
 * and reworked somewhat to fit with the remctl coding style.
 *
 * Copyright 1997, 2000, 2001, 2002 Benjamin Sittler
 * Copyright 2008 Russ Allbery <rra@stanford.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/*
 * If we're running the test suite, rename getopt and the global variables to
 * avoid conflicts with the system version.
 */
#if TESTING
# define getopt test_getopt
int test_getopt(int, char **, const char *);
# define optind test_optind
# define opterr test_opterr
# define optopt test_optopt
# define optarg test_optarg
#endif

/* Initialize global interface variables. */
int optind = 1;
int opterr = 1;
int optopt = 0;
char *optarg = NULL;

/*
 * This is the plain old UNIX getopt, with GNU-style extensions.  If you're
 * porting some piece of UNIX software, this is all you need.  It supports
 * GNU-style permutation and optional arguments, but does not support the GNU
 * -W extension.
 *
 * This function is not re-entrant or thread-safe, has static variables, and
 * generally isn't a great interface, but normally you only call it once.
 */
int
getopt(int argc, char *argv[], const char *optstring)
{
    const char *p;
    size_t offset = 0;
    char mode = '\0';
    int colon_mode = 0;
    int option = -1;

    /* Holds the current position in the parameter being parsed. */
    static int charind = 0;

    /*
     * By default, getopt permutes argv as it scans and leaves all non-options
     * at the end.  This can be changed with the first character of optstring
     * or the environment variable POSIXLY_CORRECT.  With a first character of
     * '+' or when POSIXLY_CORRECT is set, option processing stops at the
     * first non-option.  If the first character is '-', each non-option argv
     * element is handled as if it were the argument of an option with
     * character code 1.  mode holds this character.
     *
     * After the optional leading '+' and '-', optstring may contain ':'.  If
     * present, missing arguments return ':' instead of '?'.  colon_mode holds
     * this setting.
     */
    if (getenv("POSIXLY_CORRECT") != NULL) {
        mode = '+';
        colon_mode = '+';
    } else {
        if (optstring[offset] == '+' || optstring[offset] == '-') {
            mode = optstring[offset];
            offset++;
        }
        if (optstring[offset] == ':') {
            colon_mode = 1;
            offset++;
        }
    }

    /*
     * charind holds where we left off.  If it's set, we were in the middle
     * of an argv element; if not, we pick up with the next element of
     * optind.
     */
    optarg = NULL;
    if (charind == 0) {
        if (optind >= argc)
            option = -1;
        else if (strcmp(argv[optind], "--") == 0) {
            optind++;
            option = -1;
        } else if (argv[optind][0] != '-' || argv[optind][1] == '\0') {
            char *tmp;
            int i, j, k, end;

            if (mode == '+')
                option = -1;
            else if (mode == '-') {
                optarg = argv[optind];
                optind++;
                option = 1;
            } else {
                for (i = optind + 1, j = optind; i < argc; i++)
                    if ((argv[i][0] == '-') && (argv[i][1] != '\0')) {
                        optind = i;
                        option = getopt(argc, argv, optstring);
                        while (i > j) {
                            --i;
                            tmp = argv[i];
                            end = (charind == 0) ? optind - 1 : optind;
                            for (k = i; k + 1 <= end; k++) {
                                argv[k] = argv[k + 1];
                            }
                            argv[end] = tmp;
                            --optind;
                        }
                        break;
                    }
                if (i == argc)
                    option = -1;
            }
            return option;
        } else {
            charind = 1;
        }
    }
    if (charind != 0) {
        optopt = argv[optind][charind];
        for (p = optstring + offset; *p != '\0'; p++)
            if (optopt == *p) {
                p++;
                if (*p == ':') {
                    if (argv[optind][charind + 1] != '\0') {
                        optarg = &argv[optind][charind + 1];
                        optind++;
                        charind = 0;
                    } else {
                        p++;
                        if (*p != ':') {
                            charind = 0;
                            optind++;
                            if (optind >= argc) {
                                if (opterr)
                                    fprintf(stderr, "%s: option requires"
                                            " an argument -- %c\n", argv[0],
                                            optopt);
                                option = colon_mode ? ':' : '?';
                                goto done;
                            } else {
                                optarg = argv[optind];
                                optind++;
                            }
                        }
                    }
                }
                option = optopt;
            }
        if (option == -1) {
            if (opterr)
                fprintf(stderr, "%s: illegal option -- %c\n", argv[0], optopt);
            option = '?';
        }
    }

done:
    if (charind != 0) {
        charind++;
        if (argv[optind][charind] == '\0') {
            optind++;
            charind = 0;
        }
    }
    if (optind > argc)
        optind = argc;
    return option;
}
