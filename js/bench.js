var Benchmark = require('benchmark').Benchmark;
var suite = new Benchmark.Suite();
var fs = require('fs');
var sm = require('./lib/index.js');
// npm install showdown
var Showdown = require('showdown').converter;
// npm install marked
var marked = require('marked');
// npm install markdown-it
var markdownit = require('markdown-it')('commonmark');

var benchfile = process.argv[2];

var contents = fs.readFileSync(benchfile, 'utf8');

suite.add('commonmark.js markdown->html', function() {
  "use strict";
  var doc = new sm.DocParser().parse(contents);
  var renderer = new sm.HtmlRenderer();
  renderer.render(doc);
})

.add('showdown.js markdown->html', function() {
  "use strict";
  var converter = new Showdown();
  converter.makeHtml(contents);
})

.add('marked.js markdown->html', function() {
  "use strict";
  marked(contents);
})

.add('markdown-it markdown->html', function() {
  "use strict";
  markdownit.render(contents);
})

.on('cycle', function(event) {
  "use strict";
  console.log(String(event.target));
})
.run();
