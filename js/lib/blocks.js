var Node = require('./node');

var C_GREATERTHAN = 62;
var C_NEWLINE = 10;
var C_SPACE = 32;
var C_OPEN_BRACKET = 91;

var InlineParser = require('./inlines');

var unescapeString = new InlineParser().unescapeString;

var BLOCKTAGNAME = '(?:article|header|aside|hgroup|iframe|blockquote|hr|body|li|map|button|object|canvas|ol|caption|output|col|p|colgroup|pre|dd|progress|div|section|dl|table|td|dt|tbody|embed|textarea|fieldset|tfoot|figcaption|th|figure|thead|footer|footer|tr|form|ul|h1|h2|h3|h4|h5|h6|video|script|style)';

var HTMLBLOCKOPEN = "<(?:" + BLOCKTAGNAME + "[\\s/>]" + "|" +
        "/" + BLOCKTAGNAME + "[\\s>]" + "|" + "[?!])";

var reHtmlBlockOpen = new RegExp('^' + HTMLBLOCKOPEN, 'i');

var reHrule = /^(?:(?:\* *){3,}|(?:_ *){3,}|(?:- *){3,}) *$/;

var reMaybeSpecial = /^[ #`~*+_=<>0-9-]/;

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
    "use strict";
    return !(reNonSpace.test(s));
};

// Convert tabs to spaces on each line using a 4-space tab stop.
var detabLine = function(text) {
    "use strict";
    if (text.indexOf('\u0000') !== -1) {
        // replace NUL for security
        text = text.replace(/\0/g, '\uFFFD');
    }
    if (text.indexOf('\t') === -1) {
        return text;
    } else {
        var lastStop = 0;
        return text.replace(/\t/g, function(match, offset) {
            var result = '    '.slice((offset - lastStop) % 4);
            lastStop = offset + 1;
            return result;
        });
    }
};

// Attempt to match a regex in string s at offset offset.
// Return index of match or -1.
var matchAt = function(re, s, offset) {
    "use strict";
    var res = s.slice(offset).match(re);
    if (res) {
        return offset + res.index;
    } else {
        return -1;
    }
};

// destructively trip final blank lines in an array of strings
var stripFinalBlankLines = function(lns) {
    "use strict";
    var i = lns.length - 1;
    while (!reNonSpace.test(lns[i])) {
        lns.pop();
        i--;
    }
};

// DOC PARSER

// These are methods of a DocParser object, defined below.

// Returns true if parent block can contain child block.
var canContain = function(parent_type, child_type) {
    "use strict";
    return ( parent_type === 'Document' ||
             parent_type === 'BlockQuote' ||
             parent_type === 'ListItem' ||
             (parent_type === 'List' && child_type === 'ListItem') );
};

// Returns true if block type can accept lines of text.
var acceptsLines = function(block_type) {
    "use strict";
    return ( block_type === 'Paragraph' ||
             block_type === 'IndentedCode' ||
             block_type === 'FencedCode' );
};

// Returns true if block ends with a blank line, descending if needed
// into lists and sublists.
var endsWithBlankLine = function(block) {
    "use strict";
    while (block) {
        if (block.last_line_blank) {
            return true;
        }
        if (block.t === 'List' || block.t === 'ListItem') {
            block = block.lastChild;
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
var breakOutOfLists = function(block, line_number) {
    "use strict";
    var b = block;
    var last_list = null;
    do {
        if (b.t === 'List') {
            last_list = b;
        }
        b = b.parent;
    } while (b);

    if (last_list) {
        while (block !== last_list) {
            this.finalize(block, line_number);
            block = block.parent;
        }
        this.finalize(last_list, line_number);
        this.tip = last_list.parent;
    }
};

// Add a line to the block at the tip.  We assume the tip
// can accept lines -- that check should be done before calling this.
var addLine = function(ln, offset) {
    "use strict";
    var s = ln.slice(offset);
    if (!(this.tip.open)) {
        throw { msg: "Attempted to add line (" + ln + ") to closed container." };
    }
    this.tip.strings.push(s);
};

// Add block of type tag as a child of the tip.  If the tip can't
// accept children, close and finalize it and try its parent,
// and so on til we find a block that can accept children.
var addChild = function(tag, line_number, offset) {
    "use strict";
    while (!canContain(this.tip.t, tag)) {
        this.finalize(this.tip, line_number - 1);
    }

    var column_number = offset + 1; // offset 0 = column 1
    var newBlock = new Node(tag, [[line_number, column_number], [0, 0]]);
    newBlock.strings = [];
    newBlock.string_content = undefined;
    this.tip.appendChild(newBlock);
    this.tip = newBlock;
    return newBlock;
};

// Parse a list marker and return data on the marker (type,
// start, delimiter, bullet character, padding) or null.
var parseListMarker = function(ln, offset) {
    "use strict";
    var rest = ln.slice(offset);
    var match;
    var spaces_after_marker;
    var data = { type: undefined,
                 tight: true,
                 bullet_char: undefined,
                 start: undefined,
                 delimiter: undefined,
                 padding: undefined };
    if (rest.match(reHrule)) {
        return null;
    }
    if ((match = rest.match(reBulletListMarker))) {
        spaces_after_marker = match[1].length;
        data.type = 'Bullet';
        data.bullet_char = match[0][0];

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
    "use strict";
    return (list_data.type === item_data.type &&
            list_data.delimiter === item_data.delimiter &&
            list_data.bullet_char === item_data.bullet_char);
};

// Analyze a line of text and update the document appropriately.
// We parse markdown text by calling this on each line of input,
// then finalizing the document.
var incorporateLine = function(ln, line_number) {
    "use strict";

    var all_matched = true;
    var first_nonspace;
    var offset = 0;
    var match;
    var data;
    var blank;
    var indent;
    var last_matched_container;
    var i;
    var CODE_INDENT = 4;

    var container = this.doc;
    var oldtip = this.tip;

    // Convert tabs to spaces:
    ln = detabLine(ln);

    // For each containing block, try to parse the associated line start.
    // Bail out on failure: container will point to the last matching block.
    // Set all_matched to false if not all containers match.
    while (container.lastChild) {
        if (!container.lastChild.open) {
            break;
        }
        container = container.lastChild;

        match = matchAt(reNonSpace, ln, offset);
        if (match === -1) {
            first_nonspace = ln.length;
            blank = true;
        } else {
            first_nonspace = match;
            blank = false;
        }
        indent = first_nonspace - offset;

        switch (container.t) {
        case 'BlockQuote':
            if (indent <= 3 && ln.charCodeAt(first_nonspace) === C_GREATERTHAN) {
                offset = first_nonspace + 1;
                if (ln.charCodeAt(offset) === C_SPACE) {
                    offset++;
                }
            } else {
                all_matched = false;
            }
            break;

        case 'ListItem':
            if (indent >= container.list_data.marker_offset +
                container.list_data.padding) {
                offset += container.list_data.marker_offset +
                    container.list_data.padding;
            } else if (blank) {
                offset = first_nonspace;
            } else {
                all_matched = false;
            }
            break;

        case 'IndentedCode':
            if (indent >= CODE_INDENT) {
                offset += CODE_INDENT;
            } else if (blank) {
                offset = first_nonspace;
            } else {
                all_matched = false;
            }
            break;

        case 'Header':
        case 'HorizontalRule':
            // a header can never container > 1 line, so fail to match:
            all_matched = false;
            if (blank) {
                container.last_line_blank = true;
            }
            break;

        case 'FencedCode':
            // skip optional spaces of fence offset
            i = container.fence_offset;
            while (i > 0 && ln.charCodeAt(offset) === C_SPACE) {
                offset++;
                i--;
            }
            break;

        case 'HtmlBlock':
            if (blank) {
                container.last_line_blank = true;
                all_matched = false;
            }
            break;

        case 'Paragraph':
            if (blank) {
                container.last_line_blank = true;
                all_matched = false;
            }
            break;

        default:
        }

        if (!all_matched) {
            container = container.parent; // back up to last matching block
            break;
        }
    }

    last_matched_container = container;

    // This function is used to finalize and close any unmatched
    // blocks.  We aren't ready to do this now, because we might
    // have a lazy paragraph continuation, in which case we don't
    // want to close unmatched blocks.  So we store this closure for
    // use later, when we have more information.
    var closeUnmatchedBlocks = function(mythis) {
        var already_done = false;
        // finalize any blocks not matched
        while (!already_done && oldtip !== last_matched_container) {
            mythis.finalize(oldtip, line_number - 1);
            oldtip = oldtip.parent;
        }
        already_done = true;
    };

    // Check to see if we've hit 2nd blank line; if so break out of list:
    if (blank && container.last_line_blank) {
        this.breakOutOfLists(container, line_number);
    }

    // Unless last matched container is a code block, try new container starts,
    // adding children to the last matched container:
    while (container.t !== 'FencedCode' &&
           container.t !== 'IndentedCode' &&
           container.t !== 'HtmlBlock' &&
           // this is a little performance optimization:
           matchAt(reMaybeSpecial, ln, offset) !== -1) {

        match = matchAt(reNonSpace, ln, offset);
        if (match === -1) {
            first_nonspace = ln.length;
            blank = true;
        } else {
            first_nonspace = match;
            blank = false;
        }
        indent = first_nonspace - offset;

        if (indent >= CODE_INDENT) {
            // indented code
            if (this.tip.t !== 'Paragraph' && !blank) {
                offset += CODE_INDENT;
                closeUnmatchedBlocks(this);
                container = this.addChild('IndentedCode', line_number, offset);
            } else { // indent > 4 in a lazy paragraph continuation
                break;
            }

        } else if (ln.charCodeAt(first_nonspace) === C_GREATERTHAN) {
            // blockquote
            offset = first_nonspace + 1;
            // optional following space
            if (ln.charCodeAt(offset) === C_SPACE) {
                offset++;
            }
            closeUnmatchedBlocks(this);
            container = this.addChild('BlockQuote', line_number, offset);

        } else if ((match = ln.slice(first_nonspace).match(reATXHeaderMarker))) {
            // ATX header
            offset = first_nonspace + match[0].length;
            closeUnmatchedBlocks(this);
            container = this.addChild('Header', line_number, first_nonspace);
            container.level = match[0].trim().length; // number of #s
            // remove trailing ###s:
            container.strings =
                [ln.slice(offset).replace(/^ *#+ *$/, '').replace(/ +#+ *$/, '')];
            break;

        } else if ((match = ln.slice(first_nonspace).match(reCodeFence))) {
            // fenced code block
            var fence_length = match[0].length;
            closeUnmatchedBlocks(this);
            container = this.addChild('FencedCode', line_number, first_nonspace);
            container.fence_length = fence_length;
            container.fence_char = match[0][0];
            container.fence_offset = first_nonspace - offset;
            offset = first_nonspace + fence_length;
            break;

        } else if (matchAt(reHtmlBlockOpen, ln, first_nonspace) !== -1) {
            // html block
            closeUnmatchedBlocks(this);
            container = this.addChild('HtmlBlock', line_number, first_nonspace);
            // note, we don't adjust offset because the tag is part of the text
            break;

        } else if (container.t === 'Paragraph' &&
                   container.strings.length === 1 &&
                   ((match = ln.slice(first_nonspace).match(reSetextHeaderLine)))) {
            // setext header line
            closeUnmatchedBlocks(this);
            container.t = 'Header'; // convert Paragraph to SetextHeader
            container.level = match[0][0] === '=' ? 1 : 2;
            offset = ln.length;

        } else if (matchAt(reHrule, ln, first_nonspace) !== -1) {
            // hrule
            closeUnmatchedBlocks(this);
            container = this.addChild('HorizontalRule', line_number, first_nonspace);
            offset = ln.length - 1;
            break;

        } else if ((data = parseListMarker(ln, first_nonspace))) {
            // list item
            closeUnmatchedBlocks(this);
            data.marker_offset = indent;
            offset = first_nonspace + data.padding;

            // add the list if needed
            if (container.t !== 'List' ||
                !(listsMatch(container.list_data, data))) {
                container = this.addChild('List', line_number, first_nonspace);
                container.list_data = data;
            }

            // add the list item
            container = this.addChild('ListItem', line_number, first_nonspace);
            container.list_data = data;

        } else {
            break;

        }

        if (acceptsLines(container.t)) {
            // if it's a line container, it can't contain other containers
            break;
        }
    }

    // What remains at the offset is a text line.  Add the text to the
    // appropriate container.

    match = matchAt(reNonSpace, ln, offset);
    if (match === -1) {
        first_nonspace = ln.length;
        blank = true;
    } else {
        first_nonspace = match;
        blank = false;
    }
    indent = first_nonspace - offset;

    // First check for a lazy paragraph continuation:
    if (this.tip !== last_matched_container &&
        !blank &&
        this.tip.t === 'Paragraph' &&
        this.tip.strings.length > 0) {
        // lazy paragraph continuation

        this.last_line_blank = false;
        this.addLine(ln, offset);

    } else { // not a lazy continuation

        // finalize any blocks not matched
        closeUnmatchedBlocks(this);

        // Block quote lines are never blank as they start with >
        // and we don't count blanks in fenced code for purposes of tight/loose
        // lists or breaking out of lists.  We also don't set last_line_blank
        // on an empty list item.
        container.last_line_blank = blank &&
            !(container.t === 'BlockQuote' ||
              container.t === 'Header' ||
              container.t === 'FencedCode' ||
              (container.t === 'ListItem' &&
               !container.firstChild &&
               container.sourcepos[0][0] === line_number));

        var cont = container;
        while (cont.parent) {
            cont.parent.last_line_blank = false;
            cont = cont.parent;
        }

        switch (container.t) {
        case 'IndentedCode':
        case 'HtmlBlock':
            this.addLine(ln, offset);
            break;

        case 'FencedCode':
            // check for closing code fence:
            match = (indent <= 3 &&
                     ln.charAt(first_nonspace) === container.fence_char &&
                     ln.slice(first_nonspace).match(reClosingCodeFence));
            if (match && match[0].length >= container.fence_length) {
                // don't add closing fence to container; instead, close it:
                this.finalize(container, line_number);
            } else {
                this.addLine(ln, offset);
            }
            break;

        case 'Header':
        case 'HorizontalRule':
            // nothing to do; we already added the contents.
            break;

        default:
            if (acceptsLines(container.t)) {
                this.addLine(ln, first_nonspace);
            } else if (blank) {
                break;
            } else {
                // create paragraph container for line
                container = this.addChild('Paragraph', line_number, first_nonspace);
                this.addLine(ln, first_nonspace);
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
var finalize = function(block, line_number) {
    "use strict";
    var pos;
    // don't do anything if the block is already closed
    if (!block.open) {
        return 0;
    }
    block.open = false;
    block.sourcepos[1] = [line_number, this.lastLineLength + 1];

    switch (block.t) {
    case 'Paragraph':
        block.string_content = block.strings.join('\n');

        // try parsing the beginning as link reference definitions:
        while (block.string_content.charCodeAt(0) === C_OPEN_BRACKET &&
               (pos = this.inlineParser.parseReference(block.string_content,
                                                       this.refmap))) {
            block.string_content = block.string_content.slice(pos);
            if (isBlank(block.string_content)) {
                block.t = 'ReferenceDef';
                break;
            }
        }
        break;

    case 'Header':
        block.string_content = block.strings.join('\n');
        break;

    case 'HtmlBlock':
        block.literal = block.strings.join('\n');
        break;

    case 'IndentedCode':
        stripFinalBlankLines(block.strings);
        block.literal = block.strings.join('\n') + '\n';
        block.t = 'CodeBlock';
        break;

    case 'FencedCode':
        // first line becomes info string
        block.info = unescapeString(block.strings[0].trim());
        if (block.strings.length === 1) {
            block.literal = '';
        } else {
            block.literal = block.strings.slice(1).join('\n') + '\n';
        }
        block.t = 'CodeBlock';
        break;

    case 'List':
        block.list_data.tight = true; // tight by default

        var item = block.firstChild;
        while (item) {
            // check for non-final list item ending with blank line:
            if (endsWithBlankLine(item) && item.next) {
                block.list_data.tight = false;
                break;
            }
            // recurse into children of list item, to see if there are
            // spaces between any of them:
            var subitem = item.firstChild;
            while (subitem) {
                if (endsWithBlankLine(subitem) && (item.next || subitem.next)) {
                    block.list_data.tight = false;
                    break;
                }
                subitem = subitem.next;
            }
            item = item.next;
        }
        break;

    default:
        break;
    }

    this.tip = block.parent || this.top;
};

// Walk through a block & children recursively, parsing string content
// into inline content where appropriate.  Returns new object.
var processInlines = function(block) {
    "use strict";
    var node, event;
    var walker = block.walker();
    while ((event = walker.next())) {
        node = event.node;
        if (!event.entering && (node.t === 'Paragraph' ||
                                node.t === 'Header')) {
            this.inlineParser.parse(node, this.refmap);
        }
    }
};

var Document = function() {
    "use strict";
    var doc = new Node('Document', [[1, 1], [0, 0]]);
    doc.string_content = undefined;
    doc.strings = [];
    return doc;
};

// The main parsing function.  Returns a parsed document AST.
var parse = function(input) {
    "use strict";
    this.doc = Document();
    this.tip = this.doc;
    this.refmap = {};
    var lines = input.split(reLineEnding);
    var len = lines.length;
    if (input.charCodeAt(input.length - 1) === C_NEWLINE) {
        // ignore last blank line created by final newline
        len -= 1;
    }
    for (var i = 0; i < len; i++) {
        this.incorporateLine(lines[i], i + 1);
    }
    while (this.tip) {
        this.finalize(this.tip, len);
    }
    this.processInlines(this.doc);
    return this.doc;
};


// The DocParser object.
function DocParser(){
    "use strict";
    return {
        doc: Document(),
        tip: this.doc,
        refmap: {},
        lastLineLength: 0,
        inlineParser: new InlineParser(),
        breakOutOfLists: breakOutOfLists,
        addLine: addLine,
        addChild: addChild,
        incorporateLine: incorporateLine,
        finalize: finalize,
        processInlines: processInlines,
        parse: parse
    };
}

module.exports = DocParser;
