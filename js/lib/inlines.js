var fromCodePoint = require('./from-code-point.js');
var entityToChar = require('./html5-entities.js').entityToChar;

// Constants for character codes:

var C_NEWLINE = 10;
var C_SPACE = 32;
var C_ASTERISK = 42;
var C_UNDERSCORE = 95;
var C_BACKTICK = 96;
var C_OPEN_BRACKET = 91;
var C_CLOSE_BRACKET = 93;
var C_LESSTHAN = 60;
var C_BANG = 33;
var C_BACKSLASH = 92;
var C_AMPERSAND = 38;
var C_OPEN_PAREN = 40;
var C_COLON = 58;

// Some regexps used in inline parser:

var ESCAPABLE = '[!"#$%&\'()*+,./:;<=>?@[\\\\\\]^_`{|}~-]';
var ESCAPED_CHAR = '\\\\' + ESCAPABLE;
var REG_CHAR = '[^\\\\()\\x00-\\x20]';
var IN_PARENS_NOSP = '\\((' + REG_CHAR + '|' + ESCAPED_CHAR + ')*\\)';
var TAGNAME = '[A-Za-z][A-Za-z0-9]*';
var ATTRIBUTENAME = '[a-zA-Z_:][a-zA-Z0-9:._-]*';
var UNQUOTEDVALUE = "[^\"'=<>`\\x00-\\x20]+";
var SINGLEQUOTEDVALUE = "'[^']*'";
var DOUBLEQUOTEDVALUE = '"[^"]*"';
var ATTRIBUTEVALUE = "(?:" + UNQUOTEDVALUE + "|" + SINGLEQUOTEDVALUE + "|" + DOUBLEQUOTEDVALUE + ")";
var ATTRIBUTEVALUESPEC = "(?:" + "\\s*=" + "\\s*" + ATTRIBUTEVALUE + ")";
var ATTRIBUTE = "(?:" + "\\s+" + ATTRIBUTENAME + ATTRIBUTEVALUESPEC + "?)";
var OPENTAG = "<" + TAGNAME + ATTRIBUTE + "*" + "\\s*/?>";
var CLOSETAG = "</" + TAGNAME + "\\s*[>]";
var HTMLCOMMENT = "<!--([^-]+|[-][^-]+)*-->";
var PROCESSINGINSTRUCTION = "[<][?].*?[?][>]";
var DECLARATION = "<![A-Z]+" + "\\s+[^>]*>";
var CDATA = "<!\\[CDATA\\[([^\\]]+|\\][^\\]]|\\]\\][^>])*\\]\\]>";
var HTMLTAG = "(?:" + OPENTAG + "|" + CLOSETAG + "|" + HTMLCOMMENT + "|" +
        PROCESSINGINSTRUCTION + "|" + DECLARATION + "|" + CDATA + ")";
var ENTITY = "&(?:#x[a-f0-9]{1,8}|#[0-9]{1,8}|[a-z][a-z0-9]{1,31});";

var rePunctuation = new RegExp(/^[\u2000-\u206F\u2E00-\u2E7F\\'!"#\$%&\(\)\*\+,\-\.\/:;<=>\?@\[\]\^_`\{\|\}~]/);

var reHtmlTag = new RegExp('^' + HTMLTAG, 'i');

var reLinkTitle = new RegExp(
    '^(?:"(' + ESCAPED_CHAR + '|[^"\\x00])*"' +
        '|' +
        '\'(' + ESCAPED_CHAR + '|[^\'\\x00])*\'' +
        '|' +
        '\\((' + ESCAPED_CHAR + '|[^)\\x00])*\\))');

var reLinkDestinationBraces = new RegExp(
    '^(?:[<](?:[^<>\\n\\\\\\x00]' + '|' + ESCAPED_CHAR + '|' + '\\\\)*[>])');

var reLinkDestination = new RegExp(
    '^(?:' + REG_CHAR + '+|' + ESCAPED_CHAR + '|' + IN_PARENS_NOSP + ')*');

var reEscapable = new RegExp(ESCAPABLE);

var reAllEscapedChar = new RegExp('\\\\(' + ESCAPABLE + ')', 'g');

var reEntityHere = new RegExp('^' + ENTITY, 'i');

var reEntity = new RegExp(ENTITY, 'gi');

// Matches a character with a special meaning in markdown,
// or a string of non-special characters.  Note:  we match
// clumps of _ or * or `, because they need to be handled in groups.
var reMain = /^(?:[_*`\n]+|[\[\]\\!<&*_]|(?: *[^\n `\[\]\\!<&*_]+)+|[ \n]+)/m;

// Replace entities and backslash escapes with literal characters.
var unescapeString = function(s) {
    return s.replace(reAllEscapedChar, '$1')
            .replace(reEntity, entityToChar);
};

// Normalize reference label: collapse internal whitespace
// to single space, remove leading/trailing whitespace, case fold.
var normalizeReference = function(s) {
    return s.trim()
        .replace(/\s+/, ' ')
        .toUpperCase();
};

// INLINE PARSER

// These are methods of an InlineParser object, defined below.
// An InlineParser keeps track of a subject (a string to be
// parsed) and a position in that subject.

// If re matches at current position in the subject, advance
// position in subject and return the match; otherwise return null.
var match = function(re) {
    var m = re.exec(this.subject.slice(this.pos));
    if (m) {
        this.pos += m.index + m[0].length;
        return m[0];
    } else {
        return null;
    }
};

// Returns the code for the character at the current subject position, or -1
// there are no more characters.
var peek = function() {
    if (this.pos < this.subject.length) {
        return this.subject.charCodeAt(this.pos);
    } else {
        return -1;
    }
};

// Parse zero or more space characters, including at most one newline
var spnl = function() {
    this.match(/^ *(?:\n *)?/);
    return 1;
};

// All of the parsers below try to match something at the current position
// in the subject.  If they succeed in matching anything, they
// return the inline matched, advancing the subject.

// Attempt to parse backticks, returning either a backtick code span or a
// literal sequence of backticks.
var parseBackticks = function(inlines) {
    var ticks = this.match(/^`+/);
    if (!ticks) {
        return 0;
    }
    var afterOpenTicks = this.pos;
    var foundCode = false;
    var matched;
    while (!foundCode && (matched = this.match(/`+/m))) {
        if (matched === ticks) {
            inlines.push({ t: 'Code', c: this.subject.slice(afterOpenTicks,
                                                      this.pos - ticks.length)
                     .replace(/[ \n]+/g, ' ')
                      .trim() });
            return true;
        }
    }
    // If we got here, we didn't match a closing backtick sequence.
    this.pos = afterOpenTicks;
    inlines.push({ t: 'Text', c: ticks });
    return true;
};

// Parse a backslash-escaped special character, adding either the escaped
// character, a hard line break (if the backslash is followed by a newline),
// or a literal backslash to the 'inlines' list.
var parseBackslash = function(inlines) {
    var subj = this.subject,
        pos = this.pos;
    if (subj.charCodeAt(pos) === C_BACKSLASH) {
        if (subj.charAt(pos + 1) === '\n') {
            this.pos = this.pos + 2;
            inlines.push({ t: 'Hardbreak' });
        } else if (reEscapable.test(subj.charAt(pos + 1))) {
            this.pos = this.pos + 2;
            inlines.push({ t: 'Text', c: subj.charAt(pos + 1) });
        } else {
            this.pos++;
            inlines.push({t: 'Text', c: '\\'});
        }
        return true;
    } else {
        return false;
    }
};

// Attempt to parse an autolink (URL or email in pointy brackets).
var parseAutolink = function(inlines) {
    var m;
    var dest;
    if ((m = this.match(/^<([a-zA-Z0-9.!#$%&'*+\/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*)>/))) {  // email autolink
        dest = m.slice(1, -1);
        inlines.push(
                {t: 'Link',
                 label: [{ t: 'Text', c: dest }],
                 destination: 'mailto:' + encodeURI(unescape(dest)) });
        return true;
    } else if ((m = this.match(/^<(?:coap|doi|javascript|aaa|aaas|about|acap|cap|cid|crid|data|dav|dict|dns|file|ftp|geo|go|gopher|h323|http|https|iax|icap|im|imap|info|ipp|iris|iris.beep|iris.xpc|iris.xpcs|iris.lwz|ldap|mailto|mid|msrp|msrps|mtqp|mupdate|news|nfs|ni|nih|nntp|opaquelocktoken|pop|pres|rtsp|service|session|shttp|sieve|sip|sips|sms|snmp|soap.beep|soap.beeps|tag|tel|telnet|tftp|thismessage|tn3270|tip|tv|urn|vemmi|ws|wss|xcon|xcon-userid|xmlrpc.beep|xmlrpc.beeps|xmpp|z39.50r|z39.50s|adiumxtra|afp|afs|aim|apt|attachment|aw|beshare|bitcoin|bolo|callto|chrome|chrome-extension|com-eventbrite-attendee|content|cvs|dlna-playsingle|dlna-playcontainer|dtn|dvb|ed2k|facetime|feed|finger|fish|gg|git|gizmoproject|gtalk|hcp|icon|ipn|irc|irc6|ircs|itms|jar|jms|keyparc|lastfm|ldaps|magnet|maps|market|message|mms|ms-help|msnim|mumble|mvn|notes|oid|palm|paparazzi|platform|proxy|psyc|query|res|resource|rmi|rsync|rtmp|secondlife|sftp|sgn|skype|smb|soldat|spotify|ssh|steam|svn|teamspeak|things|udp|unreal|ut2004|ventrilo|view-source|webcal|wtai|wyciwyg|xfire|xri|ymsgr):[^<>\x00-\x20]*>/i))) {
        dest = m.slice(1, -1);
        inlines.push({
                  t: 'Link',
                  label: [{ t: 'Text', c: dest }],
                  destination: encodeURI(unescape(dest)) });
        return true;
    } else {
        return false;
    }
};

// Attempt to parse a raw HTML tag.
var parseHtmlTag = function(inlines) {
    var m = this.match(reHtmlTag);
    if (m) {
        inlines.push({ t: 'Html', c: m });
        return true;
    } else {
        return false;
    }
};

// Scan a sequence of characters with code cc, and return information about
// the number of delimiters and whether they are positioned such that
// they can open and/or close emphasis or strong emphasis.  A utility
// function for strong/emph parsing.
var scanDelims = function(cc) {
    var numdelims = 0;
    var char_before, char_after, cc_after;
    var startpos = this.pos;

    char_before = this.pos === 0 ? '\n' :
        this.subject.charAt(this.pos - 1);

    while (this.peek() === cc) {
        numdelims++;
        this.pos++;
    }

    cc_after = this.peek();
    if (cc_after === -1) {
        char_after = '\n';
    } else {
        char_after = fromCodePoint(cc_after);
    }

    var can_open = numdelims > 0 && !(/\s/.test(char_after)) &&
            !(rePunctuation.test(char_after) &&
             !(/\s/.test(char_before)) &&
             !(rePunctuation.test(char_before)));
    var can_close = numdelims > 0 && !(/\s/.test(char_before)) &&
            !(rePunctuation.test(char_before) &&
              !(/\s/.test(char_after)) &&
              !(rePunctuation.test(char_after)));
    if (cc === C_UNDERSCORE) {
        can_open = can_open && !((/[a-z0-9]/i).test(char_before));
        can_close = can_close && !((/[a-z0-9]/i).test(char_after));
    }
    this.pos = startpos;
    return { numdelims: numdelims,
             can_open: can_open,
             can_close: can_close };
};

var Emph = function(ils) {
    return {t: 'Emph', c: ils};
};

var Strong = function(ils) {
    return {t: 'Strong', c: ils};
};

var Str = function(s) {
    return {t: 'Text', c: s};
};

// Attempt to parse emphasis or strong emphasis.
var parseEmphasis = function(cc, inlines) {

    var res = this.scanDelims(cc);
    var numdelims = res.numdelims;
    var startpos = this.pos;

    if (numdelims === 0) {
        return false;
    }

    this.pos += numdelims;
    inlines.push(Str(this.subject.slice(startpos, this.pos)));

    // Add entry to stack for this opener
    this.delimiters = { cc: cc,
                        numdelims: numdelims,
                        pos: inlines.length - 1,
                        previous: this.delimiters,
                        next: null,
                        can_open: res.can_open,
                        can_close: res.can_close,
                        active: true };
    if (this.delimiters.previous !== null) {
        this.delimiters.previous.next = this.delimiters;
    }

    return true;

};

var removeDelimiter = function(delim) {
    if (delim.previous !== null) {
        delim.previous.next = delim.next;
    }
    if (delim.next === null) {
        // top of stack
        this.delimiters = delim.previous;
    } else {
        delim.next.previous = delim.previous;
    }
};

var removeGaps = function(inlines) {
    // remove gaps from inlines
    var i, j;
    j = 0;
    for (i = 0 ; i < inlines.length; i++) {
        if (inlines[i] !== null) {
            inlines[j] = inlines[i];
            j++;
        }
    }
    inlines.splice(j);
};

var processEmphasis = function(inlines, stack_bottom) {
    "use strict";
    var opener, closer;
    var opener_inl, closer_inl;
    var nextstack, tempstack;
    var use_delims;
    var contents;
    var emph;
    var i;

    // find first closer above stack_bottom:
    closer = this.delimiters;
    while (closer !== null && closer.previous !== stack_bottom) {
        closer = closer.previous;
    }
    // move forward, looking for closers, and handling each
    while (closer !== null) {
        if (closer.can_close && (closer.cc === C_UNDERSCORE || closer.cc === C_ASTERISK)) {
            // found emphasis closer. now look back for first matching opener:
            opener = closer.previous;
            while (opener !== null && opener !== stack_bottom) {
                if (opener.cc === closer.cc && opener.can_open) {
                    break;
                }
                opener = opener.previous;
            }
            if (opener !== null && opener !== stack_bottom) {
                // calculate actual number of delimiters used from this closer
                if (closer.numdelims < 3 || opener.numdelims < 3) {
                    use_delims = closer.numdelims <= opener.numdelims ?
                        closer.numdelims : opener.numdelims;
                } else {
                    use_delims = closer.numdelims % 2 === 0 ? 2 : 1;
                }

                opener_inl = inlines[opener.pos];
                closer_inl = inlines[closer.pos];

                // remove used delimiters from stack elts and inlines
                opener.numdelims -= use_delims;
                closer.numdelims -= use_delims;
                opener_inl.c = opener_inl.c.slice(0, opener_inl.c.length - use_delims);
                closer_inl.c = closer_inl.c.slice(0, closer_inl.c.length - use_delims);

                // build contents for new emph element
                contents = inlines.slice(opener.pos + 1, closer.pos);
                removeGaps(contents);

                emph = use_delims === 1 ? Emph(contents) : Strong(contents);

                // insert into list of inlines
                inlines[opener.pos + 1] = emph;
                for (i = opener.pos + 2; i < closer.pos; i++) {
                    inlines[i] = null;
                }

                // remove elts btw opener and closer in delimiters stack
                tempstack = closer.previous;
                while (tempstack !== null && tempstack !== opener) {
                    nextstack = tempstack.previous;
                    this.removeDelimiter(tempstack);
                    tempstack = nextstack;
                }

                // if opener has 0 delims, remove it and the inline
                if (opener.numdelims === 0) {
                    inlines[opener.pos] = null;
                    this.removeDelimiter(opener);
                }

                if (closer.numdelims === 0) {
                    inlines[closer.pos] = null;
                    tempstack = closer.next;
                    this.removeDelimiter(closer);
                    closer = tempstack;
                }


            } else {
                closer = closer.next;
            }

        } else {
            closer = closer.next;
        }

    }

    removeGaps(inlines);

    // remove all delimiters
    while (this.delimiters !== stack_bottom) {
        this.removeDelimiter(this.delimiters);
    }
};

// Attempt to parse link title (sans quotes), returning the string
// or null if no match.
var parseLinkTitle = function() {
    var title = this.match(reLinkTitle);
    if (title) {
        // chop off quotes from title and unescape:
        return unescapeString(title.substr(1, title.length - 2));
    } else {
        return null;
    }
};

// Attempt to parse link destination, returning the string or
// null if no match.
var parseLinkDestination = function() {
    var res = this.match(reLinkDestinationBraces);
    if (res) {  // chop off surrounding <..>:
        return encodeURI(unescape(unescapeString(res.substr(1, res.length - 2))));
    } else {
        res = this.match(reLinkDestination);
        if (res !== null) {
            return encodeURI(unescape(unescapeString(res)));
        } else {
            return null;
        }
    }
};

// Attempt to parse a link label, returning number of characters parsed.
var parseLinkLabel = function() {
    var m = this.match(/^\[(?:[^\\\[\]]|\\[\[\]]){0,1000}\]/);
    return m === null ? 0 : m.length;
};

// Add open bracket to delimiter stack and add a Str to inlines.
var parseOpenBracket = function(inlines) {

    var startpos = this.pos;
    this.pos += 1;

    inlines.push(Str("["));

    // Add entry to stack for this opener
    this.delimiters = { cc: C_OPEN_BRACKET,
                        numdelims: 1,
                        pos: inlines.length - 1,
                        previous: this.delimiters,
                        next: null,
                        can_open: true,
                        can_close: false,
                        index: startpos,
                        active: true };
    if (this.delimiters.previous !== null) {
        this.delimiters.previous.next = this.delimiters;
    }

    return true;

};

// IF next character is [, and ! delimiter to delimiter stack and
// add a Str to inlines.  Otherwise just add a Str.
var parseBang = function(inlines) {

    var startpos = this.pos;
    this.pos += 1;
    if (this.peek() === C_OPEN_BRACKET) {
        this.pos += 1;
        inlines.push(Str("!["));

        // Add entry to stack for this opener
        this.delimiters = { cc: C_BANG,
                            numdelims: 1,
                            pos: inlines.length - 1,
                            previous: this.delimiters,
                            next: null,
                            can_open: true,
                            can_close: false,
                            index: startpos + 1,
                            active: true };
        if (this.delimiters.previous !== null) {
            this.delimiters.previous.next = this.delimiters;
        }
    } else {
        inlines.push(Str("!"));
    }
    return true;
};

// Try to match close bracket against an opening in the delimiter
// stack.  Add either a link or image, or a plain [ character,
// to the inlines stack.  If there is a matching delimiter,
// remove it from the delimiter stack.
var parseCloseBracket = function(inlines) {
    var startpos;
    var is_image;
    var dest;
    var title;
    var matched = false;
    var link_text;
    var i;
    var reflabel;
    var opener;

    this.pos += 1;
    startpos = this.pos;

    // look through stack of delimiters for a [ or ![
    opener = this.delimiters;

    while (opener !== null) {
        if (opener.cc === C_OPEN_BRACKET || opener.cc === C_BANG) {
            break;
        }
        opener = opener.previous;
    }

    if (opener === null) {
        // no matched opener, just return a literal
        inlines.push(Str("]"));
        return true;
    }

    if (!opener.active) {
        // no matched opener, just return a literal
        inlines.push(Str("]"));
        // take opener off emphasis stack
        this.removeDelimiter(opener);
        return true;
    }

    // If we got here, open is a potential opener
    is_image = opener.cc === C_BANG;
    // instead of copying a slice, we null out the
    // parts of inlines that don't correspond to link_text;
    // later, we'll collapse them.  This is awkward, and could
    // be simplified if we made inlines a linked list rather than
    // an array:
    link_text = inlines.slice(0);
    for (i = 0; i < opener.pos + 1; i++) {
        link_text[i] = null;
    }

    // Check to see if we have a link/image

    // Inline link?
    if (this.peek() === C_OPEN_PAREN) {
        this.pos++;
        if (this.spnl() &&
            ((dest = this.parseLinkDestination()) !== null) &&
            this.spnl() &&
            // make sure there's a space before the title:
            (/^\s/.test(this.subject.charAt(this.pos - 1)) &&
             (title = this.parseLinkTitle() || '') || true) &&
            this.spnl() &&
            this.match(/^\)/)) {
            matched = true;
        }
    } else {

        // Next, see if there's a link label
        var savepos = this.pos;
        this.spnl();
        var beforelabel = this.pos;
        var n = this.parseLinkLabel();
        if (n === 0 || n === 2) {
            // empty or missing second label
            reflabel = this.subject.slice(opener.index, startpos);
        } else {
            reflabel = this.subject.slice(beforelabel, beforelabel + n);
        }
        if (n === 0) {
            // If shortcut reference link, rewind before spaces we skipped.
            this.pos = savepos;
        }

        // lookup rawlabel in refmap
        var link = this.refmap[normalizeReference(reflabel)];
        if (link) {
            dest = link.destination;
            title = link.title;
            matched = true;
        }
    }

    if (matched) {
        this.processEmphasis(link_text, opener.previous);

        // remove the part of inlines that became link_text.
        // see note above on why we need to do this instead of splice:
        for (i = opener.pos; i < inlines.length; i++) {
            inlines[i] = null;
        }

        // processEmphasis will remove this and later delimiters.
        // Now, for a link, we also deactivate earlier link openers.
        // (no links in links)
        if (!is_image) {
          opener = this.delimiters;
          while (opener !== null) {
            if (opener.cc === C_OPEN_BRACKET) {
                opener.active = false; // deactivate this opener
            }
            opener = opener.previous;
          }
        }

        inlines.push({t: is_image ? 'Image' : 'Link',
                      destination: dest,
                      title: title,
                      label: link_text});
        return true;

    } else { // no match

        this.removeDelimiter(opener);  // remove this opener from stack
        this.pos = startpos;
        inlines.push(Str("]"));
        return true;
    }

};

// Attempt to parse an entity, return Entity object if successful.
var parseEntity = function(inlines) {
    var m;
    if ((m = this.match(reEntityHere))) {
        inlines.push({ t: 'Text', c: entityToChar(m) });
        return true;
    } else {
        return false;
    }
};

// Parse a run of ordinary characters, or a single character with
// a special meaning in markdown, as a plain string, adding to inlines.
var parseString = function(inlines) {
    var m;
    if ((m = this.match(reMain))) {
        inlines.push({ t: 'Text', c: m });
        return true;
    } else {
        return false;
    }
};

// Parse a newline.  If it was preceded by two spaces, return a hard
// line break; otherwise a soft line break.
var parseNewline = function(inlines) {
    var m = this.match(/^ *\n/);
    if (m) {
        if (m.length > 2) {
            inlines.push({ t: 'Hardbreak' });
        } else if (m.length > 0) {
            inlines.push({ t: 'Softbreak' });
        }
        return true;
    }
    return false;
};

// Attempt to parse a link reference, modifying refmap.
var parseReference = function(s, refmap) {
    this.subject = s;
    this.pos = 0;
    this.label_nest_level = 0;
    var rawlabel;
    var dest;
    var title;
    var matchChars;
    var startpos = this.pos;

    // label:
    matchChars = this.parseLinkLabel();
    if (matchChars === 0) {
        return 0;
    } else {
        rawlabel = this.subject.substr(0, matchChars);
    }

    // colon:
    if (this.peek() === C_COLON) {
        this.pos++;
    } else {
        this.pos = startpos;
        return 0;
    }

    //  link url
    this.spnl();

    dest = this.parseLinkDestination();
    if (dest === null || dest.length === 0) {
        this.pos = startpos;
        return 0;
    }

    var beforetitle = this.pos;
    this.spnl();
    title = this.parseLinkTitle();
    if (title === null) {
        title = '';
        // rewind before spaces
        this.pos = beforetitle;
    }

    // make sure we're at line end:
    if (this.match(/^ *(?:\n|$)/) === null) {
        this.pos = startpos;
        return 0;
    }

    var normlabel = normalizeReference(rawlabel);

    if (!refmap[normlabel]) {
        refmap[normlabel] = { destination: dest, title: title };
    }
    return this.pos - startpos;
};

// Parse the next inline element in subject, advancing subject position.
// On success, add the result to the inlines list, and return true.
// On failure, return false.
var parseInline = function(inlines) {
    "use strict";
    var c = this.peek();
    if (c === -1) {
        return false;
    }
    var res;
    switch(c) {
    case C_NEWLINE:
    case C_SPACE:
        res = this.parseNewline(inlines);
        break;
    case C_BACKSLASH:
        res = this.parseBackslash(inlines);
        break;
    case C_BACKTICK:
        res = this.parseBackticks(inlines);
        break;
    case C_ASTERISK:
    case C_UNDERSCORE:
        res = this.parseEmphasis(c, inlines);
        break;
    case C_OPEN_BRACKET:
        res = this.parseOpenBracket(inlines);
        break;
    case C_BANG:
        res = this.parseBang(inlines);
        break;
    case C_CLOSE_BRACKET:
        res = this.parseCloseBracket(inlines);
        break;
    case C_LESSTHAN:
        res = this.parseAutolink(inlines) || this.parseHtmlTag(inlines);
        break;
    case C_AMPERSAND:
        res = this.parseEntity(inlines);
        break;
    default:
        res = this.parseString(inlines);
        break;
    }
    if (!res) {
        this.pos += 1;
        inlines.push({t: 'Text', c: fromCodePoint(c)});
    }

    return true;
};

// Parse s as a list of inlines, using refmap to resolve references.
var parseInlines = function(s, refmap) {
    this.subject = s;
    this.pos = 0;
    this.refmap = refmap || {};
    this.delimiters = null;
    var inlines = [];
    while (this.parseInline(inlines)) {
    }
    this.processEmphasis(inlines, null);
    return inlines;
};

// The InlineParser object.
function InlineParser(){
    "use strict";
    return {
        subject: '',
        label_nest_level: 0, // used by parseLinkLabel method
        delimiters: null,  // used by parseEmphasis method
        pos: 0,
        refmap: {},
        match: match,
        peek: peek,
        spnl: spnl,
        unescapeString: unescapeString,
        parseBackticks: parseBackticks,
        parseBackslash: parseBackslash,
        parseAutolink: parseAutolink,
        parseHtmlTag: parseHtmlTag,
        scanDelims: scanDelims,
        parseEmphasis: parseEmphasis,
        parseLinkTitle: parseLinkTitle,
        parseLinkDestination: parseLinkDestination,
        parseLinkLabel: parseLinkLabel,
        parseOpenBracket: parseOpenBracket,
        parseCloseBracket: parseCloseBracket,
        parseBang: parseBang,
        parseEntity: parseEntity,
        parseString: parseString,
        parseNewline: parseNewline,
        parseReference: parseReference,
        parseInline: parseInline,
        processEmphasis: processEmphasis,
        removeDelimiter: removeDelimiter,
        parse: parseInlines
    };
}

module.exports = InlineParser;

