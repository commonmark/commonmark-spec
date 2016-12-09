'use strict';

var fs = require('fs');
var path = require('path');

var extractSpecTests = function(data) {
    var examples = [];
    var current_section = "";
    var example_number = 0;
    var tests = data
        .replace(/\r\n?/g, "\n") // Normalize newlines for platform independence
        .replace(/^<!-- END TESTS -->(.|[\n])*/m, '');

    tests.replace(/^`{32} example\n([\s\S]*?)^\.\n([\s\S]*?)^`{32}$|^#{1,6} *(.*)$/gm,
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
    return examples;
};

exports.text = fs.readFileSync(path.join(__dirname, 'spec.txt'), 'utf8');
exports.tests = extractSpecTests(exports.text);
