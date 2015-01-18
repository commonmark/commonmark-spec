#!/usr/bin/env node
"use strict";

var fs = require('fs');
var commonmark = require('./lib/index.js');

// Home made mini-version of the npm ansi module:
var escSeq = function(s) {
    return function (){
        process.stdout.write('\u001b' + s);
        return this;
    };
};

var repeat = function(pattern, count) {
    if (count < 1) {
        return '';
    }
    var result = '';
    while (count > 1) {
        if (count & 1) {
            result += pattern;
        }
        count >>= 1;
        pattern += pattern;
    }
    return result + pattern;
};

var cursor = {
    write: function (s) {
        process.stdout.write(s);
        return this;
    },
    green: escSeq('[0;32m'),
    red: escSeq('[0;31m'),
    cyan: escSeq('[0;36m'),
    reset: escSeq('[0m')
};

var writer = new commonmark.HtmlRenderer();
var reader = new commonmark.Parser();

var results = {
    passed: 0,
    failed: 0
};

var showSpaces = function(s) {
    var t = s;
    return t.replace(/\t/g, '→')
        .replace(/ /g, '␣');
};

var pathologicalTest = function(testcase, res) {
    cursor.write(testcase.name + ' ');
    console.time('  elapsed time');
    var actual = writer.render(reader.parse(testcase.input));
    if (actual === testcase.expected) {
        cursor.green().write('✓\n').reset();
        res.passed += 1;
    } else {
        cursor.red().write('✘\n');
        cursor.cyan();
        cursor.write('=== markdown ===============\n');
        cursor.write(showSpaces(testcase.input));
        cursor.write('=== expected ===============\n');
        cursor.write(showSpaces(testcase.expected));
        cursor.write('=== got ====================\n');
        cursor.write(showSpaces(actual));
        cursor.write('\n');
        cursor.reset();
        res.failed += 1;
    }
    console.timeEnd('  elapsed time');
};

fs.readFile('spec.txt', 'utf8', function(err, data) {
    if (err) {
        return console.log(err);
    }
    var i;
    var examples = [];
    var current_section = "";
    var example_number = 0;
    var tests = data
            .replace(/\r\n?/g, "\n") // Normalize newlines for platform independence
            .replace(/^<!-- END TESTS -->(.|[\n])*/m, '');

    tests.replace(/^\.\n([\s\S]*?)^\.\n([\s\S]*?)^\.$|^#{1,6} *(.*)$/gm,
                  function(_, markdownSubmatch, htmlSubmatch, sectionSubmatch){
                      if (sectionSubmatch) {
                          current_section = sectionSubmatch;
                      } else {
                          example_number++;
                          examples.push({markdown: markdownSubmatch,
                                         html: htmlSubmatch,
                                         section: current_section,
                                         number: example_number});
                      }
                  });

    current_section = "";

    cursor.write('Spec tests:\n\n');
    console.time("Elapsed time");

    for (i = 0; i < examples.length; i++) {
        var example = examples[i];
        if (example.section !== current_section) {
            if (current_section !== '') {
                cursor.write('\n');
            }
            current_section = example.section;
            cursor.reset().write(current_section).reset().write('  ');
        }
        var actual = writer.render(reader.parse(example.markdown.replace(/→/g, '\t')));
        if (actual === example.html) {
            results.passed++;
            cursor.green().write('✓').reset();
        } else {
            results.failed++;
            cursor.write('\n');

            cursor.red().write('✘ Example ' + example.number + '\n');
            cursor.cyan();
            cursor.write('=== markdown ===============\n');
            cursor.write(showSpaces(example.markdown));
            cursor.write('=== expected ===============\n');
            cursor.write(showSpaces(example.html));
            cursor.write('=== got ====================\n');
            cursor.write(showSpaces(actual));
            cursor.reset();
        }
    }
    cursor.write('\n');
    console.timeEnd("Elapsed time");

    // pathological cases
    cursor.write('\nPathological cases:\n');

    var cases = [
        { name: 'U+0000 in input',
          input: 'abc\u0000xyz\u0000\n',
          expected: '<p>abc\ufffdxyz\ufffd</p>\n' },
        { name: 'nested strong emph 10000 deep',
          input: repeat('*a **a ', 10000) + 'b' + repeat(' a** a*', 10000),
          expected: '<p>' + repeat('<em>a <strong>a ', 10000) + 'b' +
          repeat(' a</strong> a</em>', 10000) + '</p>\n' },
        { name: 'nested brackets 10000 deep',
          input: repeat('[', 10000) + 'a' + repeat(']', 10000),
          expected: '<p>' + repeat('[', 10000) + 'a' + repeat(']', 10000) +
          '</p>\n' },
        { name: 'nested block quote 10000 deep',
          input: repeat('> ', 10000) + 'a\n',
          expected: repeat('<blockquote>\n', 10000) + '<p>a</p>\n' +
                    repeat('</blockquote>\n', 10000) }
    ];

    for (i = 0; i < cases.length; i++) {
        pathologicalTest(cases[i], results);
    }
    cursor.write('\n');

    cursor.write(results.passed.toString() + ' tests passed, ' +
                 results.failed.toString() + ' failed.\n');
});
