#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from difflib import unified_diff
import argparse
import re
import json
from cmark import CMark
from normalize import normalize_html

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run cmark tests.')
    parser.add_argument('--program', dest='program', nargs='?', default=None,
            help='program to test')
    parser.add_argument('--spec', dest='spec', nargs='?', default='spec.txt',
            help='path to spec')
    parser.add_argument('--pattern', dest='pattern', nargs='?',
            default=None, help='limit to sections matching regex pattern')
    parser.add_argument('--library-dir', dest='library_dir', nargs='?',
            default=None, help='directory containing dynamic library')
    parser.add_argument('--no-normalize', dest='normalize',
            action='store_const', const=False, default=True,
            help='do not normalize HTML')
    parser.add_argument('--dump-tests', dest='dump_tests',
            action='store_const', const=True, default=False,
            help='dump tests in JSON format')
    parser.add_argument('--debug-normalization', dest='debug_normalization',
            action='store_const', const=True,
            default=False, help='filter stdin through normalizer for testing')
    args = parser.parse_args(sys.argv[1:])

if not args.dump_tests:
    cmark = CMark(prog=args.program, library_dir=args.library_dir)

def print_test_header(headertext, example_number, start_line, end_line):
    print "Example %d (lines %d-%d) %s" % (example_number,start_line,end_line,headertext)

def do_test(markdown_lines, expected_html_lines, headertext,
            example_number, start_line, end_line, normalize):
    real_markdown_text = ''.join(markdown_lines).replace('→','\t')
    [retcode, actual_html, err] = cmark.to_html(real_markdown_text)
    if retcode == 0:
        actual_html_lines = actual_html.splitlines(True)
        expected_html = ''.join(expected_html_lines)
        if normalize:
            passed = normalize_html(actual_html) == normalize_html(expected_html)
        else:
            passed = actual_html == expected_html
        if passed:
            return 'pass'
        else:
            print_test_header(headertext, example_number,start_line,end_line)
            sys.stdout.write(real_markdown_text)
            for diffline in unified_diff(expected_html_lines, actual_html_lines,
                            "expected HTML", "actual HTML"):
                sys.stdout.write(diffline)
            sys.stdout.write('\n')
            return 'fail'
    else:
        print_test_header(headertext, example_number, start_line, end_line)
        print "program returned error code %d" % retcode
        print(err)
        return 'error'

def do_tests(specfile, pattern, normalize, dump_tests):
    line_number = 0
    start_line = 0
    end_line = 0
    example_number = 0
    passed = 0
    failed = 0
    errored = 0
    markdown_lines = []
    html_lines = []
    active = True
    state = 0  # 0 regular text, 1 markdown example, 2 html output
    headertext = ''
    tests_json = []

    header_re = re.compile('#+ ')
    if pattern:
        pattern_re = re.compile(pattern, re.IGNORECASE)

    with open(specfile, 'r') as specf:
        for line in specf:
            line_number = line_number + 1
            if state == 0 and re.match(header_re, line):
                headertext = header_re.sub('', line).strip()
                if pattern:
                    if re.search(pattern_re, line):
                        active = True
                    else:
                        active = False
            if line.strip() == ".":
                state = (state + 1) % 3
                if state == 0:
                    example_number = example_number + 1
                    end_line = line_number
                    if active:
                        if dump_tests:
                            tests_json.append({
                                 "markdown":''.join(markdown_lines).replace('→',"\t"),
                                 "html":''.join(html_lines),
                                 "example": example_number,
                                 "start_line": start_line,
                                 "end_line": end_line,
                                 "section": headertext})
                        else:
                            result = do_test(markdown_lines, html_lines,
                                             headertext, example_number,
                                             start_line, end_line, normalize)
                            if result == 'pass':
                                passed = passed + 1
                            elif result == 'fail':
                                failed = failed + 1
                            else:
                                errored = errored + 1
                    start_line = 0
                    markdown_lines = []
                    html_lines = []
            elif state == 1:
                if start_line == 0:
                    start_line = line_number - 1
                markdown_lines.append(line)
            elif state == 2:
                html_lines.append(line)

    if dump_tests:
        print json.dumps(tests_json, ensure_ascii=False, indent=2)
        return True
    else:
        print "%d passed, %d failed, %d errored" % (passed, failed, errored)
        return (failed == 0 and errored == 0)

if __name__ == "__main__":
    if args.debug_normalization:
        print normalize_html(sys.stdin.read())
    elif do_tests(args.spec, args.pattern, args.normalize, args.dump_tests):
        exit(0)
    else:
        exit(1)
