// commonmark.js - CommomMark in JavaScript
// Copyright (C) 2014 John MacFarlane
// License: BSD3.

// Basic usage:
//
// var commonmark = require('commonmark');
// var parser = new commonmark.DocParser();
// var renderer = new commonmark.HtmlRenderer();
// console.log(renderer.render(parser.parse('Hello *world*')));

"use strict";

var util = require('util');

var renderAST = function(tree) {
    return util.inspect(tree, {depth: null});
};

module.exports.DocParser = require('./blocks');
module.exports.HtmlRenderer = require('./html-renderer');
module.exports.ASTRenderer = renderAST;
