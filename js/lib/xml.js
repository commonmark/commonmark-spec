"use strict";

var escapeXml = require('./common').escapeXml;

// Helper function to produce an XML tag.
var tag = function(name, attrs, selfclosing) {
    var result = '<' + name;
    if (attrs && attrs.length > 0) {
        var i = 0;
        var attrib;
        while ((attrib = attrs[i]) !== undefined) {
            result += ' ' + attrib[0] + '="' + attrib[1] + '"';
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

var toTagName = function(s) {
    return s.replace(/([a-z])([A-Z])/g, "$1_$2").toLowerCase();
};

var renderNodes = function(block) {

    var attrs;
    var tagname;
    var walker = block.walker();
    var event, node, entering;
    var buffer = "";
    var lastOut = "\n";
    var disableTags = 0;
    var indentLevel = 0;
    var indent = '  ';
    var unescapedContents;
    var container;
    var selfClosing;
    var nodetype;

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

    buffer += '<?xml version="1.0" encoding="UTF-8"?>\n';
    buffer += '<!DOCTYPE CommonMark SYSTEM "CommonMark.dtd">\n';

    while ((event = walker.next())) {
        entering = event.entering;
        node = event.node;
        nodetype = node.t;

        if (nodetype === 'ReferenceDef') {
            continue;
        }

        container = node.isContainer();
        selfClosing = nodetype === 'HorizontalRule' || nodetype === 'Hardbreak' ||
            nodetype === 'Softbreak' || nodetype === 'Image';
        unescapedContents = nodetype === 'Html' || nodetype === 'HtmlInline';
        tagname = toTagName(nodetype);

        if (entering) {

            attrs = [];

            switch (nodetype) {
            case 'List':
                var data = node.list_data;
                if (data.type !== undefined) {
                    attrs.push(['type', data.type.toLowerCase()]);
                }
                if (data.start !== undefined) {
                    attrs.push(['start', String(data.start)]);
                }
                if (data.tight !== undefined) {
                    attrs.push(['tight', (data.tight ? 'true' : 'false')]);
                }
                if (data.delimiter !== undefined) {
                    var delimword = '';
                    if (data.delimiter === '.') {
                        delimword = 'period';
                    } else {
                        delimword = 'paren';
                    }
                    attrs.push(['delimiter', delimword]);
                }
                break;
            case 'CodeBlock':
                if (node.info) {
                    attrs.push(['info', node.info]);
                }
                break;
            case 'Header':
                attrs.push(['level', String(node.level)]);
                break;
            case 'Link':
            case 'Image':
                attrs.push(['destination', node.destination]);
                attrs.push(['title', node.title]);
                break;
            default:
                break;
            }
            if (options.sourcepos) {
                var pos = node.sourcepos;
                if (pos !== undefined) {
                    attrs.push(['data-sourcepos', String(pos[0][0]) + ':' +
                                String(pos[0][1]) + '-' + String(pos[1][0]) + ':' +
                                String(pos[1][1])]);
                }
            }

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

// The XmlRenderer object.
function XmlRenderer(options){
    return {
        // default options:
        softbreak: '\n', // by default, soft breaks are rendered as newlines in HTML
        // set to "<br />" to make them hard breaks
        // set to " " if you want to ignore line wrapping in source
        escape: escapeXml,
        options: options || {},
        render: renderNodes
    };
}

module.exports = XmlRenderer;
