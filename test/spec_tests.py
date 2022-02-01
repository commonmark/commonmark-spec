#!/usr/bin/env python3
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
    parser.add_argument('-p', '--program', dest='program', nargs='?', default=None,
            help='program to test')
    parser.add_argument('-s', '--spec', dest='spec', nargs='?', default='spec.txt',
            help='path to spec')
    parser.add_argument('-P', '--pattern', dest='pattern', nargs='?',
            default=None, help='limit to sections matching regex pattern')
    parser.add_argument('--library-dir', dest='library_dir', nargs='?',
            default=None, help='directory containing dynamic library')
    parser.add_argument('--no-normalize', dest='normalize',
            action='store_const', const=False, default=True,
            help='do not normalize HTML')
    parser.add_argument('-d', '--dump-tests', dest='dump_tests',
            action='store_const', const=True, default=False,
            help='dump tests in JSON format')
    parser.add_argument('--debug-normalization', dest='debug_normalization',
            action='store_const', const=True,
            default=False, help='filter stdin through normalizer for testing')
    parser.add_argument('-n', '--number', type=int, default=None,
            help='only consider the test with the given number')
    parser.add_argument('--track', metavar='path',
            help='track which test cases pass/fail in the given JSON file and only report changes')
    args = parser.parse_args(sys.argv[1:])

def out(str):
    sys.stdout.buffer.write(str.encode('utf-8')) 

def print_test_header(test):
    out("Example %d (lines %d-%d) %s\n" % (test['example'], test['start_line'], test['end_line'], test['section']))

def do_test(test, normalize, prev_result):
    [retcode, actual_html_bytes, err] = cmark.to_html(test['markdown'])
    if retcode != 0:
        if prev_result != 'error':
            print_test_header(test)
            out("program returned error code %d\n" % retcode)
            sys.stdout.buffer.write(err)
        return 'error'

    expected_html = test['html']

    try:
        actual_html = actual_html_bytes.decode('utf-8')
    except UnicodeDecodeError as e:
        if prev_result != 'fail':
            print_test_header(test)
            out(test['markdown'] + '\n')
            out("Unicode error: " + str(e) + '\n')
            out("Expected: " + repr(expected_html) + '\n')
            out("Got:      " + repr(actual_html_bytes) + '\n')
            out('\n')
        return 'fail'

    if normalize:
        actual_html = normalize_html(actual_html) + '\n'
        expected_html = normalize_html(expected_html) + '\n'

    if actual_html != expected_html:
        if prev_result != 'fail':
            print_test_header(test)
            out(test['markdown'] + '\n')
            expected_html_lines = expected_html.splitlines(True)
            actual_html_lines = actual_html.splitlines(True)
            for diffline in unified_diff(expected_html_lines, actual_html_lines,
                            "expected HTML", "actual HTML"):
                out(diffline)
            out('\n')
        return 'fail'

    if prev_result and prev_result != 'pass':
        print_test_header(test)
        print('fixed!')

    return 'pass'

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

    with open(specfile, 'r', encoding='utf-8', newline='\n') as specf:
        for line in specf:
            line_number = line_number + 1
            l = line.strip()
            if l == "`" * 32 + " example":
                state = 1
            elif state == 2 and l == "`" * 32:
                state = 0
                example_number = example_number + 1
                end_line = line_number
                tests.append({
                    "markdown":''.join(markdown_lines).replace('→',"\t"),
                    "html":''.join(html_lines).replace('→',"\t"),
                    "example": example_number,
                    "start_line": start_line,
                    "end_line": end_line,
                    "section": headertext})
                start_line = 0
                markdown_lines = []
                html_lines = []
            elif l == ".":
                state = 2
            elif state == 1:
                if start_line == 0:
                    start_line = line_number - 1
                markdown_lines.append(line)
            elif state == 2:
                html_lines.append(line)
            elif state == 0 and re.match(header_re, line):
                headertext = header_re.sub('', line).strip()
    return tests

if __name__ == "__main__":
    if args.debug_normalization:
        out(normalize_html(sys.stdin.read()))
        exit(0)

    all_tests = get_tests(args.spec)
    if args.pattern:
        pattern_re = re.compile(args.pattern, re.IGNORECASE)
    else:
        pattern_re = re.compile('.')
    tests = [ test for test in all_tests if re.search(pattern_re, test['section']) and (not args.number or test['example'] == args.number) ]
    if args.dump_tests:
        out(json.dumps(tests, ensure_ascii=False, indent=2))
        exit(0)
    else:
        skipped = len(all_tests) - len(tests)
        cmark = CMark(prog=args.program, library_dir=args.library_dir)
        result_counts = {'pass': 0, 'fail': 0, 'error': 0, 'skip': skipped}

        previous = {}

        if args.track:
            try:
                with open(args.track) as f:
                    previous = json.load(f)
            except FileNotFoundError:
                pass

        results = {}

        for test in tests:
            result = do_test(test, args.normalize, previous.get(str(test['example'])))
            result_counts[result] += 1
            results[test['example']] = result

        if args.track:
            with open(args.track, 'w') as f:
                json.dump(results, f)

        out("{pass} passed, {fail} failed, {error} errored, {skip} skipped\n".format(**result_counts))
        exit(result_counts['fail'] + result_counts['error'])
