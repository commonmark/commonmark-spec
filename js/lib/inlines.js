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
var C_GREATERTHAN = 62;
var C_BANG = 33;
var C_BACKSLASH = 92;
var C_AMPERSAND = 38;
var C_OPEN_PAREN = 40;
var C_COLON = 58;

// Some regexps used in inline parser:

var ESCAPABLE = '[!"#$%&\'()*+,./:;<=>?@[\\\\\\]^_`{|}~-]';
var ESCAPED_CHAR = '\\\\' + ESCAPABLE;
var IN_DOUBLE_QUOTES = '"(' + ESCAPED_CHAR + '|[^"\\x00])*"';
var IN_SINGLE_QUOTES = '\'(' + ESCAPED_CHAR + '|[^\'\\x00])*\'';
var IN_PARENS = '\\((' + ESCAPED_CHAR + '|[^)\\x00])*\\)';
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

var reEscapedChar = new RegExp('^\\\\(' + ESCAPABLE + ')');

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
        .replace(/\s+/,' ')
        .toUpperCase();
};

// INLINE PARSER

// These are methods of an InlineParser object, defined below.
// An InlineParser keeps track of a subject (a string to be
// parsed) and a position in that subject.

// If re matches at current position in the subject, advance
// position in subject and return the match; otherwise return null.
var match = function(re) {
    var match = re.exec(this.subject.slice(this.pos));
    if (match) {
        this.pos += match.index + match[0].length;
        return match[0];
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
    var startpos = this.pos;
    var ticks = this.match(/^`+/);
    if (!ticks) {
        return 0;
    }
    var afterOpenTicks = this.pos;
    var foundCode = false;
    var match;
    while (!foundCode && (match = this.match(/`+/m))) {
        if (match == ticks) {
            inlines.push({ t: 'Code', c: this.subject.slice(afterOpenTicks,
                                                      this.pos - ticks.length)
                     .replace(/[ \n]+/g,' ')
                      .trim() });
            return true;
        }
    }
    // If we got here, we didn't match a closing backtick sequence.
    this.pos = afterOpenTicks;
    inlines.push({ t: 'Str', c: ticks });
    return true;
};

// Parse a backslash-escaped special character, adding either the escaped
// character, a hard line break (if the backslash is followed by a newline),
// or a literal backslash to the 'inlines' list.
var parseBackslash = function(inlines) {
    var subj = this.subject,
        pos  = this.pos;
    if (subj.charCodeAt(pos) === C_BACKSLASH) {
        if (subj.charAt(pos + 1) === '\n') {
            this.pos = this.pos + 2;
            inlines.push({ t: 'Hardbreak' });
        } else if (reEscapable.test(subj.charAt(pos + 1))) {
            this.pos = this.pos + 2;
            inlines.push({ t: 'Str', c: subj.charAt(pos + 1) });
        } else {
            this.pos++;
            inlines.push({t: 'Str', c: '\\'});
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
        dest = m.slice(1,-1);
        inlines.push(
                {t: 'Link',
                 label: [{ t: 'Str', c: dest }],
                 destination: 'mailto:' + encodeURI(unescape(dest)) });
        return true;
    } else if ((m = this.match(/^<(?:coap|doi|javascript|aaa|aaas|about|acap|cap|cid|crid|data|dav|dict|dns|file|ftp|geo|go|gopher|h323|http|https|iax|icap|im|imap|info|ipp|iris|iris.beep|iris.xpc|iris.xpcs|iris.lwz|ldap|mailto|mid|msrp|msrps|mtqp|mupdate|news|nfs|ni|nih|nntp|opaquelocktoken|pop|pres|rtsp|service|session|shttp|sieve|sip|sips|sms|snmp|soap.beep|soap.beeps|tag|tel|telnet|tftp|thismessage|tn3270|tip|tv|urn|vemmi|ws|wss|xcon|xcon-userid|xmlrpc.beep|xmlrpc.beeps|xmpp|z39.50r|z39.50s|adiumxtra|afp|afs|aim|apt|attachment|aw|beshare|bitcoin|bolo|callto|chrome|chrome-extension|com-eventbrite-attendee|content|cvs|dlna-playsingle|dlna-playcontainer|dtn|dvb|ed2k|facetime|feed|finger|fish|gg|git|gizmoproject|gtalk|hcp|icon|ipn|irc|irc6|ircs|itms|jar|jms|keyparc|lastfm|ldaps|magnet|maps|market|message|mms|ms-help|msnim|mumble|mvn|notes|oid|palm|paparazzi|platform|proxy|psyc|query|res|resource|rmi|rsync|rtmp|secondlife|sftp|sgn|skype|smb|soldat|spotify|ssh|steam|svn|teamspeak|things|udp|unreal|ut2004|ventrilo|view-source|webcal|wtai|wyciwyg|xfire|xri|ymsgr):[^<>\x00-\x20]*>/i))) {
        dest = m.slice(1,-1);
        inlines.push({
                  t: 'Link',
                  label: [{ t: 'Str', c: dest }],
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
    var first_close_delims = 0;
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

    var can_open = numdelims > 0 && numdelims <= 3 && !(/\s/.test(char_after));
    var can_close = numdelims > 0 && numdelims <= 3 && !(/\s/.test(char_before));
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
    return {t: 'Str', c: s};
};

// Attempt to parse emphasis or strong emphasis.
var parseEmphasis = function(cc,inlines) {
    var startpos = this.pos;
    var c ;
    var first_close = 0;
    c = fromCodePoint(cc);

    var numdelims;
    var numclosedelims;
    var delimpos;

    // Get opening delimiters.
    res = this.scanDelims(cc);
    numdelims = res.numdelims;

    if (numdelims === 0) {
        this.pos = startpos;
        return false;
    }

    if (numdelims >= 4 || !res.can_open) {
        this.pos += numdelims;
        inlines.push(Str(this.subject.slice(startpos, startpos + numdelims)));
        return true;
    }

    this.pos += numdelims;

    var delims_to_match = numdelims;

    var current = [];
    var firstend;
    var firstpos;
    var state = 0;
    var can_close = false;
    var can_open = false;
    var last_emphasis_closer = null;
    while (this.last_emphasis_closer[c] >= this.pos) {
        res = this.scanDelims(cc);
        numclosedelims = res.numdelims;

        if (res.can_close) {
            if (last_emphasis_closer === null ||
                last_emphasis_closer < this.pos) {
                last_emphasis_closer = this.pos;
            }
            if (numclosedelims === 3 && delims_to_match === 3) {
                delims_to_match -= 3;
                this.pos += 3;
                current = [{t: 'Strong', c: [{t: 'Emph', c: current}]}];
            } else if (numclosedelims >= 2 && delims_to_match >= 2) {
                delims_to_match -= 2;
                this.pos += 2;
                firstend = current.length;
                firstpos = this.pos;
                current = [{t: 'Strong', c: current}];
            } else if (numclosedelims >= 1 && delims_to_match >= 1) {
                delims_to_match -= 1;
                this.pos += 1;
                firstend = current.length;
                firstpos = this.pos;
                current = [{t: 'Emph', c: current}];
            } else {
                if (!(this.parseInline(current,true))) {
                    break;
                }
            }
            if (delims_to_match === 0) {
                Array.prototype.push.apply(inlines, current);
                return true;
            }
        } else if (!(this.parseInline(current,true))) {
            break;
        }
    }

    // we didn't match emphasis: fallback
    inlines.push(Str(this.subject.slice(startpos,
                                        startpos + delims_to_match)));
    if (delims_to_match < numdelims) {
        Array.prototype.push.apply(inlines, current.slice(0,firstend));
        this.pos = firstpos;
    } else { // delims_to_match === numdelims
        this.pos = startpos + delims_to_match;
    }

    if (last_emphasis_closer) {
        this.last_emphasis_closer[c] = last_emphasis_closer;
    }
    return true;
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
    if (this.peek() != C_OPEN_BRACKET) {
        return 0;
    }
    var startpos = this.pos;
    var nest_level = 0;
    if (this.label_nest_level > 0) {
        // If we've already checked to the end of this subject
        // for a label, even with a different starting [, we
        // know we won't find one here and we can just return.
        // This avoids lots of backtracking.
        // Note:  nest level 1 would be: [foo [bar]
        //        nest level 2 would be: [foo [bar [baz]
        this.label_nest_level--;
        return 0;
    }
    this.pos++;  // advance past [
    var c;
    while ((c = this.peek()) && c != -1 && (c != C_CLOSE_BRACKET || nest_level > 0)) {
        switch (c) {
        case C_BACKTICK:
            this.parseBackticks([]);
            break;
        case C_LESSTHAN:
            if (!(this.parseAutolink([]) || this.parseHtmlTag([]))) {
                this.pos++;
            }
            break;
        case C_OPEN_BRACKET:  // nested []
            nest_level++;
            this.pos++;
            break;
        case C_CLOSE_BRACKET:  // nested []
            nest_level--;
            this.pos++;
            break;
        case C_BACKSLASH:
            this.parseBackslash([]);
            break;
        default:
            this.parseString([]);
        }
    }
    if (c === C_CLOSE_BRACKET) {
        this.label_nest_level = 0;
        this.pos++; // advance past ]
        return this.pos - startpos;
    } else {
        if (c === -1) {
            this.label_nest_level = nest_level;
        }
        this.pos = startpos;
        return 0;
    }
};

// Parse raw link label, including surrounding [], and return
// inline contents.  (Note:  this is not a method of InlineParser.)
var parseRawLabel = function(s) {
    // note:  parse without a refmap; we don't want links to resolve
    // in nested brackets!
    return new InlineParser().parse(s.substr(1, s.length - 2), {});
};

// Attempt to parse a link.  If successful, return the link.
var parseLink = function(inlines) {
    var startpos = this.pos;
    var reflabel;
    var n;
    var dest;
    var title;

    n = this.parseLinkLabel();
    if (n === 0) {
        return false;
    }
    var afterlabel = this.pos;
    var rawlabel = this.subject.substr(startpos, n);

    // if we got this far, we've parsed a label.
    // Try to parse an explicit link: [label](url "title")
    if (this.peek() == C_OPEN_PAREN) {
        this.pos++;
        if (this.spnl() &&
            ((dest = this.parseLinkDestination()) !== null) &&
            this.spnl() &&
            // make sure there's a space before the title:
            (/^\s/.test(this.subject.charAt(this.pos - 1)) &&
             (title = this.parseLinkTitle() || '') || true) &&
            this.spnl() &&
            this.match(/^\)/)) {
            inlines.push({ t: 'Link',
                      destination: dest,
                      title: title,
                      label: parseRawLabel(rawlabel) });
            return true;
        } else {
            this.pos = startpos;
            return false;
        }
    }
    // If we're here, it wasn't an explicit link. Try to parse a reference link.
    // first, see if there's another label
    var savepos = this.pos;
    this.spnl();
    var beforelabel = this.pos;
    n = this.parseLinkLabel();
    if (n == 2) {
        // empty second label
        reflabel = rawlabel;
    } else if (n > 0) {
        reflabel = this.subject.slice(beforelabel, beforelabel + n);
    } else {
        this.pos = savepos;
        reflabel = rawlabel;
    }
    // lookup rawlabel in refmap
    var link = this.refmap[normalizeReference(reflabel)];
    if (link) {
        inlines.push({t: 'Link',
                 destination: link.destination,
                 title: link.title,
                 label: parseRawLabel(rawlabel) });
        return true;
    } else {
        this.pos = startpos;
        return false;
    }
    // Nothing worked, rewind:
    this.pos = startpos;
    return false;
};

// Attempt to parse an entity, return Entity object if successful.
var parseEntity = function(inlines) {
    var m;
    if ((m = this.match(reEntityHere))) {
        inlines.push({ t: 'Str', c: entityToChar(m) });
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
        inlines.push({ t: 'Str', c: m });
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

// Attempt to parse an image.  If the opening '!' is not followed
// by a link, return a literal '!'.
var parseImage = function(inlines) {
    if (this.match(/^!/)) {
        var link = this.parseLink(inlines);
        if (link) {
            inlines[inlines.length - 1].t = 'Image';
            return true;
        } else {
            inlines.push({ t: 'Str', c: '!' });
            return true;
        }
    } else {
        return false;
    }
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
    var match;

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
// If memoize is set, memoize the result.
// On success, add the result to the inlines list, and return true.
// On failure, return false.
var parseInline = function(inlines, memoize) {
    var startpos = this.pos;
    var origlen = inlines.length;
    var memoized = memoize && this.memo[startpos];
    if (memoized) {
        this.pos = memoized.endpos;
        Array.prototype.push.apply(inlines, memoized.inline);
        return true;
    }

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
        res = this.parseLink(inlines);
        break;
    case C_BANG:
        res = this.parseImage(inlines);
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
        inlines.push({t: 'Str', c: fromCodePoint(c)});
    }

    if (memoize) {
        this.memo[startpos] = { inline: inlines.slice(origlen),
                                endpos: this.pos };
    }
    return true;
};

// Parse s as a list of inlines, using refmap to resolve references.
var parseInlines = function(s, refmap) {
    this.subject = s;
    this.pos = 0;
    this.refmap = refmap || {};
    this.memo = {};
    this.last_emphasis_closer = { '*': s.length, '_': s.length };
    var inlines = [];
    while (this.parseInline(inlines, false)) {
    }
    return inlines;
};

// The InlineParser object.
function InlineParser(){
    return {
        subject: '',
        label_nest_level: 0, // used by parseLinkLabel method
        last_emphasis_closer: null,  // used by parseEmphasis method
        pos: 0,
        refmap: {},
        memo: {},
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
        parseLink: parseLink,
        parseEntity: parseEntity,
        parseString: parseString,
        parseNewline: parseNewline,
        parseImage: parseImage,
        parseReference: parseReference,
        parseInline: parseInline,
        parse: parseInlines
    };
}

module.exports = InlineParser;
