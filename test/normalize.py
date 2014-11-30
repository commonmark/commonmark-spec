from HTMLParser import HTMLParser, HTMLParseError
from htmlentitydefs import name2codepoint
import sys
import re
import cgi

# Normalization code, adapted from
# https://github.com/karlcow/markdown-testsuite/
significant_attrs = ["alt", "href", "src", "title"]
whitespace_re = re.compile('/s+/')
class MyHTMLParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.last = "starttag"
        self.in_pre = False
        self.output = u""
        self.last_tag = ""
    def handle_data(self, data):
        after_tag = self.last == "endtag" or self.last == "starttag"
        after_block_tag = after_tag and self.is_block_tag(self.last_tag)
        if after_tag and self.last_tag == "br":
            data = data.lstrip('\n')
        data = whitespace_re.sub(' ', data)
        if after_block_tag and not self.in_pre:
            if self.last == "starttag":
                data = data.lstrip()
            elif self.last == "endtag":
                data = data.strip()
        self.output += data
        self.last = "data"
    def handle_endtag(self, tag):
        if tag == "pre":
            self.in_pre = False
        if self.is_block_tag(tag):
            self.output = self.output.rstrip()
        self.output += "</" + tag + ">"
        self.last_tag = tag
        self.last = "endtag"
    def handle_starttag(self, tag, attrs):
        if tag == "pre":
            self.in_pre = True
        self.output += "<" + tag
        # For now we don't strip out 'extra' attributes, because of
        # raw HTML test cases.
        # attrs = filter(lambda attr: attr[0] in significant_attrs, attrs)
        if attrs:
            attrs.sort()
            for (k,v) in attrs:
                self.output += " " + k
                if v != None:
                    self.output += ("=" + '"' + cgi.escape(v,quote=True) + '"')
        self.output += ">"
        self.last_tag = tag
        self.last = "starttag"
    def handle_startendtag(self, tag, attrs):
        """Ignore closing tag for self-closing """
        self.handle_starttag(tag, attrs)
        self.last_tag = tag
        self.last = "endtag"
    def handle_comment(self, data):
        self.output += '<!--' + data + '-->'
        self.last = "comment"
    def handle_decl(self, data):
        self.output += '<!' + data + '>'
        self.last = "decl"
    def unknown_decl(self, data):
        self.output += '<!' + data + '>'
        self.last = "decl"
    def handle_pi(self,data):
        self.output += '<?' + data + '>'
        self.last = "pi"
    def handle_entityref(self, name):
        try:
            c = unichr(name2codepoint[name])
        except KeyError:
            c = None
        self.output_char(c, '&' + name + ';')
        self.last = "ref"
    def handle_charref(self, name):
        try:
            if name.startswith("x"):
                c = unichr(int(name[1:], 16))
            else:
                c = unichr(int(name))
        except ValueError:
                c = None
        self.output_char(c, '&' + name + ';')
        self.last = "ref"
    # Helpers.
    def output_char(self, c, fallback):
        if c == u'<':
            self.output += "&lt;"
        elif c == u'>':
            self.output += "&gt;"
        elif c == u'&':
            self.output += "&amp;"
        elif c == u'"':
            self.output += "&quot;"
        elif c == None:
            self.output += fallback
        else:
            self.output += c

    def is_block_tag(self,tag):
        return (tag in ['article', 'header', 'aside', 'hgroup', 'blockquote',
            'hr', 'iframe', 'body', 'li', 'map', 'button', 'object', 'canvas',
            'ol', 'caption', 'output', 'col', 'p', 'colgroup', 'pre', 'dd',
            'progress', 'div', 'section', 'dl', 'table', 'td', 'dt',
            'tbody', 'embed', 'textarea', 'fieldset', 'tfoot', 'figcaption',
            'th', 'figure', 'thead', 'footer', 'tr', 'form', 'ul',
            'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'video', 'script', 'style'])

def normalize_html(html):
    r"""
    Return normalized form of HTML which ignores insignificant output
    differences:

    * Multiple inner whitespaces are collapsed to a single space (except
      in pre tags).
    * Outer whitespace (outside block-level tags) is removed.
    * Self-closing tags are converted to open tags.
    * Attributes are sorted and lowercased.
    * References are converted to unicode, except that '<', '>', '&', and
      '&' are rendered using entities.
    """
    html_chunk_re = re.compile("(\<!\[CDATA\[.*?\]\]\>|\<[^>]*\>|[^<]+)")
    try:
        parser = MyHTMLParser()
        # We work around HTMLParser's limitations parsing CDATA
        # by breaking the input into chunks and passing CDATA chunks
        # through verbatim.
        for chunk in re.finditer(html_chunk_re, html):
            if chunk.group(0)[:8] == "<![CDATA":
                parser.output += chunk.group(0)
            else:
                parser.feed(chunk.group(0).decode(encoding='UTF-8'))
        parser.close()
        return parser.output
    except HTMLParseError as e:
        sys.stderr.write("Normalization error: " + e.msg + "\n")
        return html  # on error, return unnormalized HTML
