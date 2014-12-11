var C_GREATERTHAN = 62;
var C_SPACE = 32;
var C_OPEN_BRACKET = 91;

var InlineParser = require('./inlines');
var unescapeString = new InlineParser().unescapeString;

// Returns true if string contains only space characters.
var isBlank = function(s) {
    return /^\s*$/.test(s);
};

// Convert tabs to spaces on each line using a 4-space tab stop.
var detabLine = function(text) {
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
    var res = s.slice(offset).match(re);
    if (res) {
        return offset + res.index;
    } else {
        return -1;
    }
};

var BLOCKTAGNAME = '(?:article|header|aside|hgroup|iframe|blockquote|hr|body|li|map|button|object|canvas|ol|caption|output|col|p|colgroup|pre|dd|progress|div|section|dl|table|td|dt|tbody|embed|textarea|fieldset|tfoot|figcaption|th|figure|thead|footer|footer|tr|form|ul|h1|h2|h3|h4|h5|h6|video|script|style)';
var HTMLBLOCKOPEN = "<(?:" + BLOCKTAGNAME + "[\\s/>]" + "|" +
        "/" + BLOCKTAGNAME + "[\\s>]" + "|" + "[?!])";
var reHtmlBlockOpen = new RegExp('^' + HTMLBLOCKOPEN, 'i');

var reHrule = /^(?:(?:\* *){3,}|(?:_ *){3,}|(?:- *){3,}) *$/;


// DOC PARSER

// These are methods of a DocParser object, defined below.

var makeBlock = function(tag, start_line, start_column) {
    return { t: tag,
             open: true,
             last_line_blank: false,
             start_line: start_line,
             start_column: start_column,
             end_line: start_line,
             children: [],
             parent: null,
             // string_content is formed by concatenating strings, in finalize:
             string_content: "",
             strings: [],
             inline_content: []
           };
};

// Returns true if parent block can contain child block.
var canContain = function(parent_type, child_type) {
    return ( parent_type === 'Document' ||
             parent_type === 'BlockQuote' ||
             parent_type === 'ListItem' ||
             (parent_type === 'List' && child_type === 'ListItem') );
};

// Returns true if block type can accept lines of text.
var acceptsLines = function(block_type) {
    return ( block_type === 'Paragraph' ||
             block_type === 'IndentedCode' ||
             block_type === 'FencedCode' );
};

// Returns true if block ends with a blank line, descending if needed
// into lists and sublists.
var endsWithBlankLine = function(block) {
    if (block.last_line_blank) {
        return true;
    }
    if ((block.t === 'List' || block.t === 'ListItem') && block.children.length > 0) {
        return endsWithBlankLine(block.children[block.children.length - 1]);
    } else {
        return false;
    }
};

// Break out of all containing lists, resetting the tip of the
// document to the parent of the highest list, and finalizing
// all the lists.  (This is used to implement the "two blank lines
// break of of all lists" feature.)
var breakOutOfLists = function(block, line_number) {
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
    while (!canContain(this.tip.t, tag)) {
        this.finalize(this.tip, line_number);
    }

    var column_number = offset + 1; // offset 0 = column 1
    var newBlock = makeBlock(tag, line_number, column_number);
    this.tip.children.push(newBlock);
    newBlock.parent = this.tip;
    this.tip = newBlock;
    return newBlock;
};

// Parse a list marker and return data on the marker (type,
// start, delimiter, bullet character, padding) or null.
var parseListMarker = function(ln, offset) {
    var rest = ln.slice(offset);
    var match;
    var spaces_after_marker;
    var data = {};
    if (rest.match(reHrule)) {
        return null;
    }
    if ((match = rest.match(/^[*+-]( +|$)/))) {
        spaces_after_marker = match[1].length;
        data.type = 'Bullet';
        data.bullet_char = match[0][0];

    } else if ((match = rest.match(/^(\d+)([.)])( +|$)/))) {
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
            list_data.bullet_char === item_data.bullet_char);
};

// Analyze a line of text and update the document appropriately.
// We parse markdown text by calling this on each line of input,
// then finalizing the document.
var incorporateLine = function(ln, line_number) {

    var all_matched = true;
    var last_child;
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
    while (container.children.length > 0) {
        last_child = container.children[container.children.length - 1];
        if (!last_child.open) {
            break;
        }
        container = last_child;

        match = matchAt(/[^ ]/, ln, offset);
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
            mythis.finalize(oldtip, line_number);
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
           matchAt(/^[ #`~*+_=<>0-9-]/,ln,offset) !== -1) {

        match = matchAt(/[^ ]/, ln, offset);
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

        } else if ((match = ln.slice(first_nonspace).match(/^#{1,6}(?: +|$)/))) {
            // ATX header
            offset = first_nonspace + match[0].length;
            closeUnmatchedBlocks(this);
            container = this.addChild('Header', line_number, first_nonspace);
            container.level = match[0].trim().length; // number of #s
            // remove trailing ###s:
            container.strings =
                [ln.slice(offset).replace(/^ *#+ *$/, '').replace(/ +#+ *$/,'')];
            break;

        } else if ((match = ln.slice(first_nonspace).match(/^`{3,}(?!.*`)|^~{3,}(?!.*~)/))) {
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
                   ((match = ln.slice(first_nonspace).match(/^(?:=+|-+) *$/)))) {
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

    match = matchAt(/[^ ]/, ln, offset);
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
               container.children.length === 0 &&
               container.start_line === line_number));

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
                     ln.slice(first_nonspace).match(/^(?:`{3,}|~{3,})(?= *$)/));
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
            } else if (container.t !== 'HorizontalRule' &&
                       container.t !== 'Header') {
                // create paragraph container for line
                container = this.addChild('Paragraph', line_number, first_nonspace);
                this.addLine(ln, first_nonspace);
            } else {
                console.log("Line " + line_number.toString() +
                            " with container type " + container.t +
                            " did not match any condition.");

            }
        }
    }
};

// Finalize a block.  Close it and do any necessary postprocessing,
// e.g. creating string_content from strings, setting the 'tight'
// or 'loose' status of a list, and parsing the beginnings
// of paragraphs for reference definitions.  Reset the tip to the
// parent of the closed block.
var finalize = function(block, line_number) {
    var pos;
    // don't do anything if the block is already closed
    if (!block.open) {
        return 0;
    }
    block.open = false;
    if (line_number > block.start_line) {
        block.end_line = line_number - 1;
    } else {
        block.end_line = line_number;
    }

    switch (block.t) {
    case 'Paragraph':
        block.string_content = block.strings.join('\n').replace(/^ {2,}/m,'');
        // delete block.strings;

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
    case 'HtmlBlock':
        block.string_content = block.strings.join('\n');
        break;

    case 'IndentedCode':
        block.string_content = block.strings.join('\n').replace(/(\n *)*$/,'\n');
        block.t = 'CodeBlock';
        break;

    case 'FencedCode':
        // first line becomes info string
        block.info = unescapeString(block.strings[0].trim());
        if (block.strings.length === 1) {
            block.string_content = '';
        } else {
            block.string_content = block.strings.slice(1).join('\n') + '\n';
        }
        block.t = 'CodeBlock';
        break;

    case 'List':
        block.tight = true; // tight by default

        var numitems = block.children.length;
        var i = 0;
        while (i < numitems) {
            var item = block.children[i];
            // check for non-final list item ending with blank line:
            var last_item = i === numitems - 1;
            if (endsWithBlankLine(item) && !last_item) {
                block.tight = false;
                break;
            }
            // recurse into children of list item, to see if there are
            // spaces between any of them:
            var numsubitems = item.children.length;
            var j = 0;
            while (j < numsubitems) {
                var subitem = item.children[j];
                var last_subitem = j === numsubitems - 1;
                if (endsWithBlankLine(subitem) && !(last_item && last_subitem)) {
                    block.tight = false;
                    break;
                }
                j++;
            }
            i++;
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
    var newblock = {};
    newblock.t = block.t;
    newblock.start_line = block.start_line;
    newblock.start_column = block.start_column;
    newblock.end_line = block.end_line;

    switch(block.t) {
    case 'Paragraph':
        newblock.inline_content =
            this.inlineParser.parse(block.string_content.trim(), this.refmap);
        break;
    case 'Header':
        newblock.inline_content =
            this.inlineParser.parse(block.string_content.trim(), this.refmap);
        newblock.level = block.level;
        break;
    case 'List':
        newblock.list_data = block.list_data;
        newblock.tight = block.tight;
        break;
    case 'CodeBlock':
        newblock.string_content = block.string_content;
        newblock.info = block.info;
        break;
    case 'HtmlBlock':
        newblock.string_content = block.string_content;
        break;
    default:
        break;
    }

    if (block.children) {
        var newchildren = [];
        for (var i = 0; i < block.children.length; i++) {
            newchildren.push(this.processInlines(block.children[i]));
        }
        newblock.children = newchildren;
    }
    return newblock;
};

// The main parsing function.  Returns a parsed document AST.
var parse = function(input) {
    this.doc = makeBlock('Document', 1, 1);
    this.tip = this.doc;
    this.refmap = {};
    var lines = input.replace(/\n$/,'').split(/\r\n|\n|\r/);
    var len = lines.length;
    for (var i = 0; i < len; i++) {
        this.incorporateLine(lines[i], i + 1);
    }
    while (this.tip) {
        this.finalize(this.tip, len - 1);
    }
    return this.processInlines(this.doc);
};


// The DocParser object.
function DocParser(){
    return {
        doc: makeBlock('Document', 1, 1),
        tip: this.doc,
        refmap: {},
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
