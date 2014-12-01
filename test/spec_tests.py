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

def print_test_header(headertext, example_number, start_line, end_line):
    print "Example %d (lines %d-%d) %s" % (example_number,start_line,end_line,headertext)

def do_test(test, normalize):
    [retcode, actual_html, err] = cmark.to_html(test['markdown'])
    if retcode == 0:
        expected_html = test['html']
        if normalize:
            passed = normalize_html(actual_html) == normalize_html(expected_html)
        else:
            passed = actual_html == expected_html
        if passed:
            return 'pass'
        else:
            print_test_header(test['section'], test['example'], test['start_line'], test['end_line'])
            sys.stdout.write(test['markdown'])
            expected_html_lines = expected_html.splitlines(True)
            actual_html_lines = actual_html.splitlines(True)
            for diffline in unified_diff(expected_html_lines, actual_html_lines,
                            "expected HTML", "actual HTML"):
                sys.stdout.write(diffline)
            sys.stdout.write('\n')
            return 'fail'
    else:
        print_test_header(test['section'], test['example'], test['start_line'], test['end_line'])
        print "program returned error code %d" % retcode
        print(err)
        return 'error'

def get_tests(specfile):
    line_number = 0
    start_line = 0
    end_line = 0
    example_number = 0
    markdown_lines = []
    html_lines = []
    state = 0  # 0 regular text, 1 markdown example, 2 html output
    headertext = ''
    tests = []

    header_re = re.compile('#+ ')

    with open(specfile, 'r') as specf:
        for line in specf:
            line_number = line_number + 1
            if state == 0 and re.match(header_re, line):
                headertext = header_re.sub('', line).strip()
            if line.strip() == ".":
                state = (state + 1) % 3
                if state == 0:
                    example_number = example_number + 1
                    end_line = line_number
                    tests.append({
                        "markdown":''.join(markdown_lines).replace('→',"\t"),
                        "html":''.join(html_lines),
                        "example": example_number,
                        "start_line": start_line,
                        "end_line": end_line,
                        "section": headertext})
                    start_line = 0
                    markdown_lines = []
                    html_lines = []
            elif state == 1:
                if start_line == 0:
                    start_line = line_number - 1
                markdown_lines.append(line)
            elif state == 2:
                html_lines.append(line)
    return tests

def do_tests(cmark, tests, pattern, normalize):
    passed = 0
    errored = 0
    failed = 0
    skipped = 0
    if pattern:
        pattern_re = re.compile(pattern, re.IGNORECASE)
    else:
        pattern_re = re.compile('.')
    for test in tests:
        if re.search(pattern_re, test['section']):
            result = do_test(test, normalize)
            if result == 'pass':
                passed += 1
            elif result == 'fail':
                failed += 1
            else:
                errored += 1
        else:
            skipped += 1
    print "%d passed, %d failed, %d errored, %d skipped" % (passed, failed, errored, skipped)
    return (failed == 0 and errored == 0)

if __name__ == "__main__":
    if args.debug_normalization:
        print normalize_html(sys.stdin.read())
        exit(0)

    tests = get_tests(args.spec)
    if args.dump_tests:
        print json.dumps(tests, ensure_ascii=False, indent=2)
        exit(0)
    else:
        cmark = CMark(prog=args.program, library_dir=args.library_dir)
        if do_tests(cmark, tests, args.pattern, args.normalize):
            exit(0)
        else:
            exit(1)
