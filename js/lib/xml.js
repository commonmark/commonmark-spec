"use strict";

// Helper function to produce an XML tag.
var tag = function(name, attrs, selfclosing) {
    var result = '<' + name;
    if (attrs && attrs.length > 0) {
        var i = 0;
        var attrib;
        while ((attrib = attrs[i]) !== undefined) {
            result = result.concat(' ', attrib[0], '="', attrib[1], '"');
            i++;
        }
    }
    if (selfclosing) {
        result += ' /';
    }

    result += '>';
    return result;
};

var reXMLTag = /\<[^>]*\>/;

var renderNodes = function(block) {

    var attrs;
    var info_words;
    var tagname;
    var walker = block.walker();
    var event, node, entering;
    var buffer = "";
    var lastOut = "\n";
    var disableTags = 0;
    var indentLevel = 0;
    var indent = '  ';
    var grandparent;
    var unescapedContents;
    var container;
    var selfClosing;

    var out = function(s) {
        if (disableTags > 0) {
            buffer += s.replace(reXMLTag, '');
        } else {
            buffer += s;
        }
        lastOut = s;
    };
    var esc = this.escape;
    var cr = function() {
        if (lastOut !== '\n') {
            buffer += '\n';
            lastOut = '\n';
            for (var i = indentLevel; i--;) {
                buffer += indent;
            }
        }
    };

    var options = this.options;

    if (options.time) { console.time("rendering"); }

    while ((event = walker.next())) {
        entering = event.entering;
        node = event.node;
        container = node.isContainer();
        selfClosing = node.t === 'HorizontalRule' || node.t === 'Hardbreak' ||
            node.t === 'Softbreak' || node.t === 'Image';
        unescapedContents = node.t === 'Html' || node.t === 'HtmlInline';
        tagname = node.t.toLowerCase();

        attrs = [];

        if (node.list_data) {
            var data = node.list_data;
            if (data.type) {
                attrs.push(['type', data.type.toLowerCase()]);
            }
            if (data.start) {
                attrs.push(['start', String(data.start)]);
            }
            if (data.tight) {
                attrs.push(['tight', (data.tight ? 'true' : 'false')]);
            }
            if (data.delimiter) {
                var delimword = '';
                if (data.delimiter === '.') {
                  delimword = 'period';
                } else {
                  delimword = 'paren';
                }
                attrs.push(['delimiter', delimword]);
            }
        }

        if (options.sourcepos) {
            var pos = node.sourcepos;
            if (pos) {
                attrs.push(['data-sourcepos', String(pos[0][0]) + ':' +
                            String(pos[0][1]) + '-' + String(pos[1][0]) + ':' +
                            String(pos[1][1])]);
            }
        }

        if (entering) {
            cr();
            out(tag(tagname, attrs, selfClosing));
            if (container) {
                indentLevel += 1;
            } else if (!container && !selfClosing) {
                if (node.literal) {
                    out(unescapedContents ? node.literal : esc(node.literal));
                }
                out(tag('/' + tagname));
            }
        } else {
            indentLevel -= 1;
            cr();
            out(tag('/' + tagname));
        }


    }
    if (options.time) { console.timeEnd("rendering"); }
    buffer += '\n';
    return buffer;
};

var replaceUnsafeChar = function(s) {
    switch (s) {
    case '&':
        return '&amp;';
    case '<':
        return '&lt;';
    case '>':
        return '&gt;';
    case '"':
        return '&quot;';
    default:
        return s;
    }
};

var reNeedsEscaping = /[&<>"]/;

// The XmlRenderer object.
function XmlRenderer(options){
    return {
        // default options:
        softbreak: '\n', // by default, soft breaks are rendered as newlines in HTML
        // set to "<br />" to make them hard breaks
        // set to " " if you want to ignore line wrapping in source
        escape: function(s, preserve_entities) {
            if (reNeedsEscaping.test(s)) {
                if (preserve_entities) {
                    return s.replace(/[&](?:[#](x[a-f0-9]{1,8}|[0-9]{1,8});|[a-z][a-z0-9]{1,31};)|[&<>"]/gi, replaceUnsafeChar);
                } else {
                    return s.replace(/[&<>"]/g, replaceUnsafeChar);
                }
            } else {
                return s;
            }
        },
        options: options || {},
        render: renderNodes
    };
}

module.exports = XmlRenderer;
