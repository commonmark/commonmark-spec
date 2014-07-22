var Benchmark = require('benchmark').Benchmark;
var suite = new Benchmark.Suite;
var fs = require('fs');
var sm = require('./stmd');
// https://github.com/coreyti/showdown
var showdown = require('../../showdown/src/showdown');
// https://github.com/chjj/marked
var marked = require('../../marked/marked.min.js');

var benchfile = process.argv[2] || 'oldtests/Original/Markdown_Documentation_Syntax.markdown';

var contents = fs.readFileSync(benchfile, 'utf8');

// var converter = new showdown.converter();

suite.add('stmd markdown->html', function() {
  var doc = new sm.DocParser().parse(contents);
  var renderer = new sm.HtmlRenderer();
  renderer.renderBlock(doc);
})

.add('showdown.js markdown->html', function() {
  var converter = new showdown.converter();
  converter.makeHtml(contents);
})

.add('marked.js markdown->html', function() {
  marked(contents);
})

.on('cycle', function(event) {
  console.log(String(event.target));
})
.run();

