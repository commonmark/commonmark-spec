"use strict";

// Helper function to produce an HTML tag.
var tag = function(name, attribs, selfclosing) {
    var result = '<' + name;
    if (attribs) {
        var i = 0;
        var attrib;
        while ((attrib = attribs[i]) !== undefined) {
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

var renderNodes = function(block) {

    var attrs;
    var info_words;
    var tagname;
    var walker = block.walker();
    var event, node, entering;
    var buffer = [];
    var disableTags = 0;
    var grandparent;
    var out = function(s) {
        if (disableTags > 0) {
            buffer.push(s.replace(/\<[^>]*\>/g, ''));
        } else {
            buffer.push(s);
        }
    };
    var esc = this.escape;
    var cr = function() {
        if (buffer.length > 0 && buffer[buffer.length - 1] !== '\n') {
            out('\n');
        }
    };

    while ((event = walker.next())) {
        entering = event.entering;
        node = event.node;

        switch (node.t) {
        case 'Text':
            out(esc(node.c));
            break;

        case 'Softbreak':
            out(this.softbreak);
            break;

        case 'Hardbreak':
            out(tag('br', [], true));
            cr();
            break;

        case 'Emph':
            out(tag(entering ? 'em' : '/em'));
            break;

        case 'Strong':
            out(tag(entering ? 'strong' : '/strong'));
            break;

        case 'Emph':
            out(tag(entering ? 'strong' : '/strong'));
            break;

        case 'Html':
            out(node.c);
            break;

        case 'Link':
            if (entering) {
                attrs = [['href', esc(node.destination, true)]];
                if (node.title) {
                    attrs.push(['title', esc(node.title, true)]);
                }
                out(tag('a', attrs));
            } else {
                out(tag('/a'));
            }
            break;

        case 'Image':
            if (entering) {
                if (disableTags === 0) {
                    out('<img src="' + esc(node.destination, true) +
                        '" alt="');
                }
                disableTags += 1;
            } else {
                disableTags -= 1;
                if (disableTags === 0) {
                    if (node.title) {
                        out('" title="' + esc(node.title, true));
                    }
                    out('" />');
                }
            }
            break;

        case 'Code':
            out(tag('code') + esc(node.c) + tag('/code'));
            break;

        case 'Document':
            break;

        case 'Paragraph':
            grandparent = node.parent.parent;
            if (grandparent !== null &&
                grandparent.t === 'List') {
                if (grandparent.list_data.tight) {
                    break;
                }
            }
            if (entering) {
                cr();
                out(tag('p'));
            } else {
                out(tag('/p'));
                cr();
            }
            break;

        case 'BlockQuote':
            if (entering) {
                cr();
                out(tag('blockquote'));
                cr();
            } else {
                cr();
                out(tag('/blockquote'));
                cr();
            }
            break;

        case 'ListItem':
            if (entering) {
                out(tag('li'));
            } else {
                out(tag('/li'));
                cr();
            }
            break;

        case 'List':
            tagname = node.list_data.type === 'Bullet' ? 'ul' : 'ol';
            if (entering) {
                attrs = (!node.list_data.start || node.list_data.start === 1) ?
                    [] : [['start', node.list_data.start.toString()]];
                cr();
                out(tag(tagname, attrs));
                cr();
            } else {
                cr();
                out(tag('/' + tagname));
                cr();
            }
            break;

        case 'Header':
            tagname = 'h' + node.level;
            if (entering) {
                cr();
                out(tag(tagname));
            } else {
                out(tag('/' + tagname));
                cr();
            }
            break;

        case 'CodeBlock':
            info_words = node.info ? node.info.split(/ +/) : [];
            attrs = (info_words.length === 0 || info_words[0].length === 0)
                ? [] : [['class', 'language-' + esc(info_words[0], true)]];
            cr();
            out(tag('pre') + tag('code', attrs));
            out(this.escape(node.string_content));
            out(tag('/code') + tag('/pre'));
            cr();
            break;

        case 'HtmlBlock':
            cr();
            out(node.string_content);
            cr();
            break;

        case 'HorizontalRule':
            cr();
            out(tag('hr', [], true));
            cr();
            break;


        case 'ReferenceDef':
            break;

        default:
            console.log("Unknown node type " + node.t);
        }

    }
    return buffer.join('');
};

var sub = function(s) {
    if (s === '&') {
        return '&amp;';
    } else if (s === '<') {
        return '&lt;';
    } else if (s === '>') {
        return '&gt;';
    } else if (s === '"') {
        return '&quot;';
    } else {
        return s;
    }
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
                return s.replace(/[&](?:[#](x[a-f0-9]{1,8}|[0-9]{1,8});|[a-z][a-z0-9]{1,31};)|[&<>"]/gi, sub);
            } else {
                return s.replace(/[&<>"]/g, sub);
            }
        },
        render: renderNodes
    };
}

module.exports = HtmlRenderer;
