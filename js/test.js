#!/usr/bin/env node
"use strict";

var fs = require('fs');
var commonmark = require('./lib/index.js');
var ansi = require('./ansi/ansi');
var cursor = ansi(process.stdout);

var writer = new commonmark.HtmlRenderer();
var reader = new commonmark.DocParser();

var passed = 0;
var failed = 0;

var showSpaces = function(s) {
  var t = s;
  return t.replace(/\t/g, '→')
    .replace(/ /g, '␣');
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
    var actual = writer.renderBlock(reader.parse(example.markdown.replace(/→/g, '\t')));
    if (actual === example.html) {
      passed++;
      cursor.green().write('✓').reset();
    } else {
      failed++;
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
  cursor.write('\n' + passed.toString() + ' tests passed, ' +
      failed.toString() + ' failed.\n');

  console.timeEnd("Elapsed time");
});
