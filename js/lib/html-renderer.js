// Helper function to produce content in a pair of HTML tags.
var inTags = function(tag, attribs, contents, selfclosing) {
    var result = '<' + tag;
    if (attribs) {
        var i = 0;
        var attrib;
        while ((attrib = attribs[i]) !== undefined) {
            result = result.concat(' ', attrib[0], '="', attrib[1], '"');
            i++;
        }
    }
    if (contents) {
        result = result.concat('>', contents, '</', tag, '>');
    } else if (selfclosing) {
        result = result + ' />';
    } else {
        result = result.concat('></', tag, '>');
    }
    return result;
};

// Render an inline element as HTML.
var renderInline = function(inline) {
    var attrs;
    switch (inline.t) {
    case 'Text':
        return this.escape(inline.c);
    case 'Softbreak':
        return this.softbreak;
    case 'Hardbreak':
        return inTags('br', [], "", true) + '\n';
    case 'Emph':
        return inTags('em', [], this.renderInlines(inline.c));
    case 'Strong':
        return inTags('strong', [], this.renderInlines(inline.c));
    case 'Html':
        return inline.c;
    case 'Link':
        attrs = [['href', this.escape(inline.destination, true)]];
        if (inline.title) {
            attrs.push(['title', this.escape(inline.title, true)]);
        }
        return inTags('a', attrs, this.renderInlines(inline.label));
    case 'Image':
        attrs = [['src', this.escape(inline.destination, true)],
                 ['alt', this.renderInlines(inline.label).
                    replace(/\<[^>]*alt="([^"]*)"[^>]*\>/g, '$1').
                    replace(/\<[^>]*\>/g,'')]];
        if (inline.title) {
            attrs.push(['title', this.escape(inline.title, true)]);
        }
        return inTags('img', attrs, "", true);
    case 'Code':
        return inTags('code', [], this.escape(inline.c));
    default:
        console.log("Unknown inline type " + inline.t);
        return "";
    }
};

// Render a list of inlines.
var renderInlines = function(inlines) {
    var result = '';
    for (var i = 0; i < inlines.length; i++) {
        result = result + this.renderInline(inlines[i]);
    }
    return result;
};

// Render a single block element.
var renderBlock = function(block, in_tight_list) {
    var tag;
    var attr;
    var info_words;
    switch (block.t) {
    case 'Document':
        var whole_doc = this.renderBlocks(block.children);
        return (whole_doc === '' ? '' : whole_doc + '\n');
    case 'Paragraph':
        if (in_tight_list) {
            return this.renderInlines(block.inline_content);
        } else {
            return inTags('p', [], this.renderInlines(block.inline_content));
        }
        break;
    case 'BlockQuote':
        var filling = this.renderBlocks(block.children);
        return inTags('blockquote', [], filling === '' ? this.innersep :
                      this.innersep + filling + this.innersep);
    case 'ListItem':
        var contents = this.renderBlocks(block.children, in_tight_list);
        if (/^[<]/.test(contents)) {
            contents = '\n' + contents;
        }
        if (/[>]$/.test(contents)) {
            contents = contents + '\n';
        }
        return inTags('li', [], contents, false).trim();
    case 'List':
        tag = block.list_data.type === 'Bullet' ? 'ul' : 'ol';
        attr = (!block.list_data.start || block.list_data.start === 1) ?
            [] : [['start', block.list_data.start.toString()]];
        return inTags(tag, attr, this.innersep +
                      this.renderBlocks(block.children, block.tight) +
                      this.innersep);
    case 'Header':
        tag = 'h' + block.level;
        return inTags(tag, [], this.renderInlines(block.inline_content));
    case 'CodeBlock':
        info_words = block.info ? block.info.split(/ +/) : [];
        attr = (info_words.length === 0 || info_words[0].length === 0) ?
            [] : [['class', 'language-' + this.escape(info_words[0], true)]];
        return inTags('pre', [],
                      inTags('code', attr, this.escape(block.string_content)));
    case 'HtmlBlock':
        return block.string_content;
    case 'ReferenceDef':
        return "";
    case 'HorizontalRule':
        return inTags('hr', [], "", true);
    default:
        console.log("Unknown block type " + block.t);
        return "";
    }
};

// Render a list of block elements, separated by this.blocksep.
var renderBlocks = function(blocks, in_tight_list) {
    var result = [];
    for (var i = 0; i < blocks.length; i++) {
        if (blocks[i].t !== 'ReferenceDef') {
            result.push(this.renderBlock(blocks[i], in_tight_list));
        }
    }
    return result.join(this.blocksep);
};

// The HtmlRenderer object.
function HtmlRenderer(){
    return {
        // default options:
        blocksep: '\n',  // space between blocks
        innersep: '\n',  // space between block container tag and contents
        softbreak: '\n', // by default, soft breaks are rendered as newlines in HTML
        // set to "<br />" to make them hard breaks
        // set to " " if you want to ignore line wrapping in source
        escape: function(s, preserve_entities) {
            if (preserve_entities) {
                return s.replace(/[&](?![#](x[a-f0-9]{1,8}|[0-9]{1,8});|[a-z][a-z0-9]{1,31};)/gi,'&amp;')
                    .replace(/[<]/g,'&lt;')
                    .replace(/[>]/g,'&gt;')
                    .replace(/["]/g,'&quot;');
            } else {
                return s.replace(/[&]/g,'&amp;')
                    .replace(/[<]/g,'&lt;')
                    .replace(/[>]/g,'&gt;')
                    .replace(/["]/g,'&quot;');
            }
        },
        renderInline: renderInline,
        renderInlines: renderInlines,
        renderBlock: renderBlock,
        renderBlocks: renderBlocks,
        render: renderBlock
    };
}

module.exports = HtmlRenderer;
