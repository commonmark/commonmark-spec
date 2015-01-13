#!/usr/bin/env node

var fs = require('fs');
var util = require('util');

fs.readFile('spec.txt', 'utf8', function(err, data) {
  if (err) {
    return console.log(err);
  }
  var examples = [];
  data.replace(/^\.\n([\s\S]*?)^\.\n([\s\S]*?)^\.$/gm,
        function(_,x,y){
          examples.push({markdown: x, html: y});
      });
  console.log(util.inspect(examples, { depth: null }));
  console.warn(examples.length + ' examples');
});
