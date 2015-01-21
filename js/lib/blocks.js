"use strict";

var Node = require('./node');
var unescapeString = require('./common').unescapeString;

var C_GREATERTHAN = 62;
var C_NEWLINE = 10;
var C_SPACE = 32;
var C_OPEN_BRACKET = 91;

var CODE_INDENT = 4;

var InlineParser = require('./inlines');

var BLOCKTAGNAME = '(?:article|header|aside|hgroup|iframe|blockquote|hr|body|li|map|button|object|canvas|ol|caption|output|col|p|colgroup|pre|dd|progress|div|section|dl|table|td|dt|tbody|embed|textarea|fieldset|tfoot|figcaption|th|figure|thead|footer|footer|tr|form|ul|h1|h2|h3|h4|h5|h6|video|script|style)';

var HTMLBLOCKOPEN = "<(?:" + BLOCKTAGNAME + "[\\s/>]" + "|" +
        "/" + BLOCKTAGNAME + "[\\s>]" + "|" + "[?!])";

var reHtmlBlockOpen = new RegExp('^' + HTMLBLOCKOPEN, 'i');

var reHrule = /^(?:(?:\* *){3,}|(?:_ *){3,}|(?:- *){3,}) *$/;

var reMaybeSpecial = /^[#`~*+_=<>0-9-]/;

var reNonSpace = /[^ \t\n]/;

var reBulletListMarker = /^[*+-]( +|$)/;

var reOrderedListMarker = /^(\d+)([.)])( +|$)/;

var reATXHeaderMarker = /^#{1,6}(?: +|$)/;

var reCodeFence = /^`{3,}(?!.*`)|^~{3,}(?!.*~)/;

var reClosingCodeFence = /^(?:`{3,}|~{3,})(?= *$)/;

var reSetextHeaderLine = /^(?:=+|-+) *$/;

var reLineEnding = /\r\n|\n|\r/;

// Returns true if string contains only space characters.
var isBlank = function(s) {
    return !(reNonSpace.test(s));
};

var tabSpaces = ['    ', '   ', '  ', ' '];

// Convert tabs to spaces on each line using a 4-space tab stop.
var detabLine = function(text) {
    var start = 0;
    var offset;
    var lastStop = 0;

    while ((offset = text.indexOf('\t', start)) !== -1) {
        var numspaces = (offset - lastStop) % 4;
        var spaces = tabSpaces[numspaces];
        text = text.slice(0, offset) + spaces + text.slice(offset + 1);
        lastStop = offset + numspaces;
        start = lastStop;
    }

    return text;
};

// Attempt to match a regex in string s at offset offset.
// Return index of match or -1.
var matchAt = function(re, s, offset) {
    var res = s.slice(offset).match(re);
    if (res === null) {
        return -1;
    } else {
        return offset + res.index;
    }
};

// destructively trip final blank lines in an array of strings
var stripFinalBlankLines = function(lns) {
    var i = lns.length - 1;
    while (!reNonSpace.test(lns[i])) {
        lns.pop();
        i--;
    }
};

// DOC PARSER

// These are methods of a Parser object, defined below.

// Returns true if parent block can contain child block.
var canContain = function(parent_type, child_type) {
    return ( parent_type === 'Document' ||
             parent_type === 'BlockQuote' ||
             parent_type === 'Item' ||
             (parent_type === 'List' && child_type === 'Item') );
};

// Returns true if block type can accept lines of text.
var acceptsLines = function(block_type) {
    return ( block_type === 'Paragraph' ||
             block_type === 'CodeBlock' );
};

// Returns true if block ends with a blank line, descending if needed
// into lists and sublists.
var endsWithBlankLine = function(block) {
    while (block) {
        if (block._lastLineBlank) {
            return true;
        }
        var t = block.type;
        if (t === 'List' || t === 'Item') {
            block = block._lastChild;
        } else {
            break;
        }
    }
    return false;
};

// Break out of all containing lists, resetting the tip of the
// document to the parent of the highest list, and finalizing
// all the lists.  (This is used to implement the "two blank lines
// break of of all lists" feature.)
var breakOutOfLists = function(block) {
    var b = block;
    var last_list = null;
    do {
        if (b.type === 'List') {
            last_list = b;
        }
        b = b._parent;
    } while (b);

    if (last_list) {
        while (block !== last_list) {
            this.finalize(block, this.lineNumber);
            block = block._parent;
        }
        this.finalize(last_list, this.lineNumber);
        this.tip = last_list._parent;
    }
};

// Add a line to the block at the tip.  We assume the tip
// can accept lines -- that check should be done before calling this.
var addLine = function(ln) {
    this.tip._strings.push(ln.slice(this.offset));
};

// Add block of type tag as a child of the tip.  If the tip can't
// accept children, close and finalize it and try its parent,
// and so on til we find a block that can accept children.
var addChild = function(tag, offset) {
    while (!canContain(this.tip.type, tag)) {
        this.finalize(this.tip, this.lineNumber - 1);
    }

    var column_number = offset + 1; // offset 0 = column 1
    var newBlock = new Node(tag, [[this.lineNumber, column_number], [0, 0]]);
    newBlock._strings = [];
    newBlock._string_content = null;
    this.tip.appendChild(newBlock);
    this.tip = newBlock;
    return newBlock;
};

// Parse a list marker and return data on the marker (type,
// start, delimiter, bullet character, padding) or null.
var parseListMarker = function(ln, offset, indent) {
    var rest = ln.slice(offset);
    var match;
    var spaces_after_marker;
    var data = { type: null,
                 tight: true,  // lists are tight by default
                 bulletChar: null,
                 start: null,
                 delimiter: null,
                 padding: null,
                 markerOffset: indent };
    if (rest.match(reHrule)) {
        return null;
    }
    if ((match = rest.match(reBulletListMarker))) {
        spaces_after_marker = match[1].length;
        data.type = 'Bullet';
        data.bulletChar = match[0][0];

    } else if ((match = rest.match(reOrderedListMarker))) {
        spaces_after_marker = match[3].length;
        data.type = 'Ordered';
        data.start = parseInt(match[1]);
        data.delimiter = match[2];
    } else {
        return null;
    }
    var blank_item = match[0].length === rest.length;
    if (spaces_after_marker >= 5 ||
        spaces_after_marker < 1 ||
        blank_item) {
        data.padding = match[0].length - spaces_after_marker + 1;
    } else {
        data.padding = match[0].length;
    }
    return data;
};

// Returns true if the two list items are of the same type,
// with the same delimiter and bullet character.  This is used
// in agglomerating list items into lists.
var listsMatch = function(list_data, item_data) {
    return (list_data.type === item_data.type &&
            list_data.delimiter === item_data.delimiter &&
            list_data.bulletChar === item_data.bulletChar);
};

// Finalize and close any unmatched blocks. Returns true.
var closeUnmatchedBlocks = function() {
    // finalize any blocks not matched
    while (this.oldtip !== this.lastMatchedContainer) {
        var parent = this.oldtip._parent;
        this.finalize(this.oldtip, this.lineNumber - 1);
        this.oldtip = parent;
    }
    return true;
};

// 'finalize' is run when the block is closed.
// 'continue' is run to check whether the block is continuing
// at a certain line and offset (e.g. whether a block quote
// contains a `>`.  It returns 0 for matched, 1 for not matched,
// and 2 for "we've dealt with this line completely, go to next."
var blocks = {
    Document: {
        continue: function(parser, container, next_nonspace) {
            return 0;
        },
        finalize: function(parser, block) {
            return;
        }
    },
    List: {
        continue: function(parser, container, next_nonspace) {
            return 0;
        },
        finalize: function(parser, block) {
            var item = block._firstChild;
            while (item) {
                // check for non-final list item ending with blank line:
                if (endsWithBlankLine(item) && item._next) {
                    block._listData.tight = false;
                    break;
                }
                // recurse into children of list item, to see if there are
                // spaces between any of them:
                var subitem = item._firstChild;
                while (subitem) {
                    if (endsWithBlankLine(subitem) &&
                        (item._next || subitem._next)) {
                        block._listData.tight = false;
                        break;
                    }
                    subitem = subitem._next;
                }
                item = item._next;
            }
        }
    },
    BlockQuote: {
        continue: function(parser, container, next_nonspace) {
            var ln = parser.currentLine;
            if (next_nonspace - parser.offset <= 3 &&
                ln.charCodeAt(next_nonspace) === C_GREATERTHAN) {
                parser.offset = next_nonspace + 1;
                if (ln.charCodeAt(parser.offset) === C_SPACE) {
                    parser.offset++;
                }
            } else {
                return 1;
            }
            return 0;
        },
        finalize: function(parser, block) {
            return;
        }
    },
    Item: {
        continue: function(parser, container, next_nonspace) {
            if (next_nonspace === parser.currentLine.length) { // blank
                parser.offset = next_nonspace;
            } else if (next_nonspace - parser.offset >=
                       container._listData.markerOffset +
                       container._listData.padding) {
                parser.offset += container._listData.markerOffset +
                    container._listData.padding;
            } else {
                return 1;
            }
            return 0;
        },
        finalize: function(parser, block) {
            return;
        }
    },
    Header: {
        continue: function(parser, container, next_nonspace) {
            // a header can never container > 1 line, so fail to match:
            return 1;
        },
        finalize: function(parser, block) {
            block._string_content = block._strings.join('\n');
        }
    },
    HorizontalRule: {
        continue: function(parser, container, next_nonspace) {
            // an hrule can never container > 1 line, so fail to match:
            return 1;
        },
        finalize: function(parser, block) {
            return;
        }
    },
    CodeBlock: {
        continue: function(parser, container, next_nonspace) {
            var ln = parser.currentLine;
            var indent = next_nonspace - parser.offset;
            if (container._isFenced) { // fenced
                var match = (indent <= 3 &&
                    ln.charAt(next_nonspace) === container._fenceChar &&
                    ln.slice(next_nonspace).match(reClosingCodeFence));
                if (match && match[0].length >= container._fenceLength) {
                    // closing fence - we're at end of line, so we can return
                    parser.finalize(container, parser.lineNumber);
                    return 2;
                } else {
                    // skip optional spaces of fence offset
                    var i = container._fenceOffset;
                    while (i > 0 && ln.charCodeAt(parser.offset) === C_SPACE) {
                        parser.offset++;
                        i--;
                    }
                }
            } else { // indented
                if (indent >= CODE_INDENT) {
                    parser.offset += CODE_INDENT;
                } else if (next_nonspace === ln.length) { // blank
                    parser.offset = next_nonspace;
                } else {
                    return 1;
                }
            }
            return 0;
        },
        finalize: function(parser, block) {
            if (block._isFenced) { // fenced
                // first line becomes info string
                block.info = unescapeString(block._strings[0].trim());
                if (block._strings.length === 1) {
                    block._literal = '';
                } else {
                    block._literal = block._strings.slice(1).join('\n') + '\n';
                }
            } else { // indented
                stripFinalBlankLines(block._strings);
                block._literal = block._strings.join('\n') + '\n';
            }
        }
    },
    HtmlBlock: {
        continue: function(parser, container, next_nonspace) {
            return (next_nonspace === parser.currentLine.length ? 1 : 0);
        },
        finalize: function(parser, block) {
            block._literal = block._strings.join('\n');
        }
    },
    Paragraph: {
        continue: function(parser, container, next_nonspace) {
            return (next_nonspace === parser.currentLine.length ? 1 : 0);
        },
        finalize: function(parser, block) {
            var pos;
            block._string_content = block._strings.join('\n');

            // try parsing the beginning as link reference definitions:
            while (block._string_content.charCodeAt(0) === C_OPEN_BRACKET &&
                   (pos =
                    parser.inlineParser.parseReference(block._string_content,
                                                       parser.refmap))) {
                block._string_content = block._string_content.slice(pos);
                if (isBlank(block._string_content)) {
                    block.unlink();
                    break;
                }
            }
        }
    }
};

// Analyze a line of text and update the document appropriately.
// We parse markdown text by calling this on each line of input,
// then finalizing the document.
var incorporateLine = function(ln) {
    var all_matched = true;
    var next_nonspace;
    var match;
    var data;
    var blank;
    var indent;
    var i;
    var allClosed;

    var container = this.doc;
    this.oldtip = this.tip;
    this.offset = 0;
    this.lineNumber += 1;

    // replace NUL characters for security
    if (ln.indexOf('\u0000') !== -1) {
        ln = ln.replace(/\0/g, '\uFFFD');
    }

    // Convert tabs to spaces:
    ln = detabLine(ln);
    this.currentLine = ln;

    // For each containing block, try to parse the associated line start.
    // Bail out on failure: container will point to the last matching block.
    // Set all_matched to false if not all containers match.
    var lastChild;
    while ((lastChild = container._lastChild) && lastChild._open) {
        container = lastChild;

        match = matchAt(reNonSpace, ln, this.offset);
        if (match === -1) {
            next_nonspace = ln.length;
        } else {
            next_nonspace = match;
        }

        switch (this.blocks[container.type].continue(this, container, next_nonspace)) {
        case 0: // we've matched, keep going
            break;
        case 1: // we've failed to match a block
            all_matched = false;
            break;
        case 2: // we've hit end of line for fenced code close and can return
            this.lastLineLength = ln.length - 1; // -1 for newline
            return;
        default:
            throw 'continue returned illegal value, must be 0, 1, or 2';
        }
        if (!all_matched) {
            container = container._parent; // back up to last matching block
            break;
        }
    }

    blank = next_nonspace === ln.length;

    allClosed = (container === this.oldtip);
    this.lastMatchedContainer = container;

    // Check to see if we've hit 2nd blank line; if so break out of list:
    if (blank && container._lastLineBlank) {
        this.breakOutOfLists(container);
    }

    // Unless last matched container is a code block, try new container starts,
    // adding children to the last matched container:
    while (true) {
        var t = container.type;

        match = matchAt(reNonSpace, ln, this.offset);
        if (match === -1) {
            next_nonspace = ln.length;
            blank = true;
            break;
        } else {
            next_nonspace = match;
            blank = false;
        }
        indent = next_nonspace - this.offset;

        if (t === 'CodeBlock' || t === 'HtmlBlock') {
            break;
        }

        if (indent >= CODE_INDENT) {
            // indented code
            if (this.tip.type !== 'Paragraph' && !blank) {
                this.offset += CODE_INDENT;
                allClosed = allClosed ||
                    this.closeUnmatchedBlocks();
                container = this.addChild('CodeBlock', this.offset);
            }
            break;
        }

        // this is a little performance optimization:
        if (matchAt(reMaybeSpecial, ln, next_nonspace) === -1) {
            break;
        }

        this.offset = next_nonspace;

        var cc = ln.charCodeAt(this.offset);

        if (cc === C_GREATERTHAN) {
            // blockquote
            this.offset += 1;
            // optional following space
            if (ln.charCodeAt(this.offset) === C_SPACE) {
                this.offset++;
            }
            allClosed = allClosed || this.closeUnmatchedBlocks();
            container = this.addChild('BlockQuote', next_nonspace);

        } else if ((match = ln.slice(this.offset).match(reATXHeaderMarker))) {
            // ATX header
            this.offset += match[0].length;
            allClosed = allClosed || this.closeUnmatchedBlocks();
            container = this.addChild('Header', next_nonspace);
            container.level = match[0].trim().length; // number of #s
            // remove trailing ###s:
            container._strings =
                [ln.slice(this.offset).replace(/^ *#+ *$/, '').replace(/ +#+ *$/, '')];
            break;

        } else if ((match = ln.slice(this.offset).match(reCodeFence))) {
            // fenced code block
            var fenceLength = match[0].length;
            allClosed = allClosed || this.closeUnmatchedBlocks();
            container = this.addChild('CodeBlock', next_nonspace);
            container._isFenced = true;
            container._fenceLength = fenceLength;
            container._fenceChar = match[0][0];
            container._fenceOffset = indent;
            this.offset += fenceLength;

        } else if (matchAt(reHtmlBlockOpen, ln, this.offset) !== -1) {
            // html block
            allClosed = allClosed || this.closeUnmatchedBlocks();
            container = this.addChild('HtmlBlock', this.offset);
            this.offset -= indent; // back up so spaces are part of block
            break;

        } else if (t === 'Paragraph' &&
                   container._strings.length === 1 &&
                   ((match = ln.slice(this.offset).match(reSetextHeaderLine)))) {
            // setext header line
            allClosed = allClosed || this.closeUnmatchedBlocks();
            var header = new Node('Header', container.sourcepos);
            header.level = match[0][0] === '=' ? 1 : 2;
            header._strings = container._strings;
            container.insertAfter(header);
            container.unlink();
            container = header;
            this.tip = header;
            this.offset = ln.length;
            break;

        } else if (matchAt(reHrule, ln, this.offset) !== -1) {
            // hrule
            allClosed = allClosed || this.closeUnmatchedBlocks();
            container = this.addChild('HorizontalRule', next_nonspace);
            this.offset = ln.length - 1;
            break;

        } else if ((data = parseListMarker(ln, this.offset, indent))) {
            // list item
            allClosed = allClosed || this.closeUnmatchedBlocks();
            this.offset += data.padding;

            // add the list if needed
            if (t !== 'List' ||
                !(listsMatch(container._listData, data))) {
                container = this.addChild('List', next_nonspace);
                container._listData = data;
            }

            // add the list item
            container = this.addChild('Item', next_nonspace);
            container._listData = data;

        } else {
            break;

        }

    }

    // What remains at the offset is a text line.  Add the text to the
    // appropriate container.

   // First check for a lazy paragraph continuation:
    if (!allClosed && !blank &&
        this.tip.type === 'Paragraph' &&
        this.tip._strings.length > 0) {
        // lazy paragraph continuation

        this.addLine(ln);

    } else { // not a lazy continuation

        // finalize any blocks not matched
        allClosed = allClosed || this.closeUnmatchedBlocks();
        if (blank && container.lastChild) {
            container.lastChild._lastLineBlank = true;
        }

        t = container.type;

        // Block quote lines are never blank as they start with >
        // and we don't count blanks in fenced code for purposes of tight/loose
        // lists or breaking out of lists.  We also don't set _lastLineBlank
        // on an empty list item, or if we just closed a fenced block.
        var lastLineBlank = blank &&
            !(t === 'BlockQuote' ||
              (t === 'CodeBlock' && container._isFenced) ||
              (t === 'Item' &&
               !container._firstChild &&
               container.sourcepos[0][0] === this.lineNumber));

        // propagate lastLineBlank up through parents:
        var cont = container;
        while (cont) {
            cont._lastLineBlank = lastLineBlank;
            cont = cont._parent;
        }

        switch (t) {
        case 'HtmlBlock':
        case 'CodeBlock':
            this.addLine(ln);
            break;

        case 'Header':
        case 'HorizontalRule':
            // nothing to do; we already added the contents.
            break;

        default:
            this.offset = next_nonspace;
            if (acceptsLines(t)) {
                this.addLine(ln);
            } else if (blank) {
                break;
            } else {
                // create paragraph container for line
                container = this.addChild('Paragraph', this.offset);
                this.addLine(ln);
            }
        }
    }
    this.lastLineLength = ln.length - 1; // -1 for newline
};

// Finalize a block.  Close it and do any necessary postprocessing,
// e.g. creating string_content from strings, setting the 'tight'
// or 'loose' status of a list, and parsing the beginnings
// of paragraphs for reference definitions.  Reset the tip to the
// parent of the closed block.
var finalize = function(block, lineNumber) {
    var above = block._parent || this.top;
    block._open = false;
    block.sourcepos[1] = [lineNumber, this.lastLineLength + 1];

    this.blocks[block.type].finalize(this, block);

    this.tip = above;
};

// Walk through a block & children recursively, parsing string content
// into inline content where appropriate.  Returns new object.
var processInlines = function(block) {
    var node, event, t;
    var walker = block.walker();
    while ((event = walker.next())) {
        node = event.node;
        t = node.type;
        if (!event.entering && (t === 'Paragraph' || t === 'Header')) {
            this.inlineParser.parse(node, this.refmap);
        }
    }
};

var Document = function() {
    var doc = new Node('Document', [[1, 1], [0, 0]]);
    doc._string_content = null;
    doc._strings = [];
    return doc;
};

// The main parsing function.  Returns a parsed document AST.
var parse = function(input) {
    this.doc = new Document();
    this.tip = this.doc;
    this.refmap = {};
    if (this.options.time) { console.time("preparing input"); }
    var lines = input.split(reLineEnding);
    var len = lines.length;
    if (input.charCodeAt(input.length - 1) === C_NEWLINE) {
        // ignore last blank line created by final newline
        len -= 1;
    }
    if (this.options.time) { console.timeEnd("preparing input"); }
    if (this.options.time) { console.time("block parsing"); }
    for (var i = 0; i < len; i++) {
        this.incorporateLine(lines[i]);
    }
    while (this.tip) {
        this.finalize(this.tip, len);
    }
    if (this.options.time) { console.timeEnd("block parsing"); }
    if (this.options.time) { console.time("inline parsing"); }
    this.processInlines(this.doc);
    if (this.options.time) { console.timeEnd("inline parsing"); }
    return this.doc;
};


// The Parser object.
function Parser(options){
    return {
        doc: new Document(),
        blocks: blocks,
        tip: this.doc,
        oldtip: this.doc,
        currentLine: "",
        lineNumber: 0,
        offset: 0,
        lastMatchedContainer: this.doc,
        refmap: {},
        lastLineLength: 0,
        inlineParser: new InlineParser(),
        breakOutOfLists: breakOutOfLists,
        addLine: addLine,
        addChild: addChild,
        incorporateLine: incorporateLine,
        finalize: finalize,
        processInlines: processInlines,
        closeUnmatchedBlocks: closeUnmatchedBlocks,
        parse: parse,
        options: options || {}
    };
}

module.exports = Parser;
