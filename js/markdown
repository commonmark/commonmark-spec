#!/usr/bin/env node
var fs = require('fs');
var util = require('util');
var stmd = require('./stmd');

file = process.argv[2] || '/dev/stdin';

fs.readFile(file, 'utf8', function(err, data) {
  if (err) {
    return console.log(err);
  }
  var parser   = new stmd.DocParser();
  var renderer = new stmd.HtmlRenderer();
  process.stdout.write(renderer.render(parser.parse(data)));
});
