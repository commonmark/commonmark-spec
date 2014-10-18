// stmd.js - CommomMark in JavaScript
// Copyright (C) 2014 John MacFarlane
// License: BSD3.

// Basic usage:
//
// var stmd = require('stmd');
// var parser = new stmd.DocParser();
// var renderer = new stmd.HtmlRenderer();
// console.log(renderer.render(parser.parse('Hello *world*')));

    require('./from-code-point.js');
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
    var BLOCKTAGNAME = '(?:article|header|aside|hgroup|iframe|blockquote|hr|body|li|map|button|object|canvas|ol|caption|output|col|p|colgroup|pre|dd|progress|div|section|dl|table|td|dt|tbody|embed|textarea|fieldset|tfoot|figcaption|th|figure|thead|footer|footer|tr|form|ul|h1|h2|h3|h4|h5|h6|video|script|style)';
    var ATTRIBUTENAME = '[a-zA-Z_:][a-zA-Z0-9:._-]*';
    var UNQUOTEDVALUE = "[^\"'=<>`\\x00-\\x20]+";
    var SINGLEQUOTEDVALUE = "'[^']*'";
    var DOUBLEQUOTEDVALUE = '"[^"]*"';
    var ATTRIBUTEVALUE = "(?:" + UNQUOTEDVALUE + "|" + SINGLEQUOTEDVALUE + "|" + DOUBLEQUOTEDVALUE + ")";
    var ATTRIBUTEVALUESPEC = "(?:" + "\\s*=" + "\\s*" + ATTRIBUTEVALUE + ")";
    var ATTRIBUTE = "(?:" + "\\s+" + ATTRIBUTENAME + ATTRIBUTEVALUESPEC + "?)";
    var OPENTAG = "<" + TAGNAME + ATTRIBUTE + "*" + "\\s*/?>";
    var CLOSETAG = "</" + TAGNAME + "\\s*[>]";
    var OPENBLOCKTAG = "<" + BLOCKTAGNAME + ATTRIBUTE + "*" + "\\s*/?>";
    var CLOSEBLOCKTAG = "</" + BLOCKTAGNAME + "\\s*[>]";
    var HTMLCOMMENT = "<!--([^-]+|[-][^-]+)*-->";
    var PROCESSINGINSTRUCTION = "[<][?].*?[?][>]";
    var DECLARATION = "<![A-Z]+" + "\\s+[^>]*>";
    var CDATA = "<!\\[CDATA\\[([^\\]]+|\\][^\\]]|\\]\\][^>])*\\]\\]>";
    var HTMLTAG = "(?:" + OPENTAG + "|" + CLOSETAG + "|" + HTMLCOMMENT + "|" +
            PROCESSINGINSTRUCTION + "|" + DECLARATION + "|" + CDATA + ")";
    var HTMLBLOCKOPEN = "<(?:" + BLOCKTAGNAME + "[\\s/>]" + "|" +
            "/" + BLOCKTAGNAME + "[\\s>]" + "|" + "[?!])";
    var ENTITY = "&(?:#x[a-f0-9]{1,8}|#[0-9]{1,8}|[a-z][a-z0-9]{1,31});";

    var reHtmlTag = new RegExp('^' + HTMLTAG, 'i');

    var reHtmlBlockOpen = new RegExp('^' + HTMLBLOCKOPEN, 'i');

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

    var reAllTab = /\t/g;

    var reHrule = /^(?:(?:\* *){3,}|(?:_ *){3,}|(?:- *){3,}) *$/;

    var reEntityHere = new RegExp('^' + ENTITY, 'i');

    var reEntity = new RegExp(ENTITY, 'gi');

    // Matches a character with a special meaning in markdown,
    // or a string of non-special characters.  Note:  we match
    // clumps of _ or * or `, because they need to be handled in groups.
    var reMain = /^(?:[_*`\n]+|[\[\]\\!<&*_]|(?: *[^\n `\[\]\\!<&*_]+)+|[ \n]+)/m;

    // Replace entities and backslash escapes with literal characters.
    var unescapeEntBS = function(s) {
        return s.replace(reAllEscapedChar, '$1')
                .replace(reEntity, entityToChar);
    };

    // Returns true if string contains only space characters.
    var isBlank = function(s) {
        return /^\s*$/.test(s);
    };

    // Normalize reference label: collapse internal whitespace
    // to single space, remove leading/trailing whitespace, case fold.
    var normalizeReference = function(s) {
        return s.trim()
            .replace(/\s+/,' ')
            .toUpperCase();
    };

    // Attempt to match a regex in string s at offset offset.
    // Return index of match or null.
    var matchAt = function(re, s, offset) {
        var res = s.slice(offset).match(re);
        if (res) {
            return offset + res.index;
        } else {
            return null;
        }
    };

    // Convert tabs to spaces on each line using a 4-space tab stop.
    var detabLine = function(text) {
        if (text.indexOf('\t') == -1) {
            return text;
        } else {
            var lastStop = 0;
            return text.replace(reAllTab, function(match, offset) {
                var result = '    '.slice((offset - lastStop) % 4);
                lastStop = offset + 1;
                return result;
            });
        }
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
            char_after = String.fromCodePoint(cc_after);
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
        c = String.fromCodePoint(cc);

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
            return unescapeEntBS(title.substr(1, title.length - 2));
        } else {
            return null;
        }
    };

    // Attempt to parse link destination, returning the string or
    // null if no match.
    var parseLinkDestination = function() {
        var res = this.match(reLinkDestinationBraces);
        if (res) {  // chop off surrounding <..>:
            return encodeURI(unescape(unescapeEntBS(res.substr(1, res.length - 2))));
        } else {
            res = this.match(reLinkDestination);
            if (res !== null) {
                return encodeURI(unescape(unescapeEntBS(res)));
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
            inlines.push({t: 'Str', c: String.fromCodePoint(c)});
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
        return ( parent_type == 'Document' ||
                 parent_type == 'BlockQuote' ||
                 parent_type == 'ListItem' ||
                 (parent_type == 'List' && child_type == 'ListItem') );
    };

    // Returns true if block type can accept lines of text.
    var acceptsLines = function(block_type) {
        return ( block_type == 'Paragraph' ||
                 block_type == 'IndentedCode' ||
                 block_type == 'FencedCode' );
    };

    // Returns true if block ends with a blank line, descending if needed
    // into lists and sublists.
    var endsWithBlankLine = function(block) {
        if (block.last_line_blank) {
            return true;
        }
        if ((block.t == 'List' || block.t == 'ListItem') && block.children.length > 0) {
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
            while (block != last_list) {
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
            throw({ msg: "Attempted to add line (" + ln + ") to closed container." });
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
            if (match === null) {
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

            case 'ATXHeader':
            case 'SetextHeader':
            case 'HorizontalRule':
                // a header can never container > 1 line, so fail to match:
                all_matched = false;
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
            // finalize any blocks not matched
            while (!already_done && oldtip != last_matched_container) {
                mythis.finalize(oldtip, line_number);
                oldtip = oldtip.parent;
            }
            var already_done = true;
        };

        // Check to see if we've hit 2nd blank line; if so break out of list:
        if (blank && container.last_line_blank) {
            this.breakOutOfLists(container, line_number);
        }

        // Unless last matched container is a code block, try new container starts,
        // adding children to the last matched container:
        while (container.t != 'FencedCode' &&
               container.t != 'IndentedCode' &&
               container.t != 'HtmlBlock' &&
               // this is a little performance optimization:
               matchAt(/^[ #`~*+_=<>0-9-]/,ln,offset) !== null) {

            match = matchAt(/[^ ]/, ln, offset);
            if (match === null) {
                first_nonspace = ln.length;
                blank = true;
            } else {
                first_nonspace = match;
                blank = false;
            }
            indent = first_nonspace - offset;

            if (indent >= CODE_INDENT) {
                // indented code
                if (this.tip.t != 'Paragraph' && !blank) {
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
                container = this.addChild('ATXHeader', line_number, first_nonspace);
                container.level = match[0].trim().length; // number of #s
                // remove trailing ###s:
                container.strings =
                    [ln.slice(offset).replace(/(?:(\\#) *#*| *#+) *$/,'$1')];
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

            } else if (matchAt(reHtmlBlockOpen, ln, first_nonspace) !== null) {
                // html block
                closeUnmatchedBlocks(this);
                container = this.addChild('HtmlBlock', line_number, first_nonspace);
                // note, we don't adjust offset because the tag is part of the text
                break;

            } else if (container.t == 'Paragraph' &&
                       container.strings.length === 1 &&
                       ((match = ln.slice(first_nonspace).match(/^(?:=+|-+) *$/)))) {
                // setext header line
                closeUnmatchedBlocks(this);
                container.t = 'SetextHeader'; // convert Paragraph to SetextHeader
                container.level = match[0][0] === '=' ? 1 : 2;
                offset = ln.length;

            } else if (matchAt(reHrule, ln, first_nonspace) !== null) {
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
        if (match === null) {
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
            this.tip.t == 'Paragraph' &&
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
                !(container.t == 'BlockQuote' ||
                  container.t == 'FencedCode' ||
                  (container.t == 'ListItem' &&
                   container.children.length === 0 &&
                   container.start_line == line_number));

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
                         ln.charAt(first_nonspace) == container.fence_char &&
                         ln.slice(first_nonspace).match(/^(?:`{3,}|~{3,})(?= *$)/));
                if (match && match[0].length >= container.fence_length) {
                    // don't add closing fence to container; instead, close it:
                    this.finalize(container, line_number);
                } else {
                    this.addLine(ln, offset);
                }
                break;

            case 'ATXHeader':
            case 'SetextHeader':
            case 'HorizontalRule':
                // nothing to do; we already added the contents.
                break;

            default:
                if (acceptsLines(container.t)) {
                    this.addLine(ln, first_nonspace);
                } else if (blank) {
                    // do nothing
                } else if (container.t != 'HorizontalRule' &&
                           container.t != 'SetextHeader') {
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
            block.string_content = block.strings.join('\n').replace(/^  */m,'');

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

        case 'ATXHeader':
        case 'SetextHeader':
        case 'HtmlBlock':
            block.string_content = block.strings.join('\n');
            break;

        case 'IndentedCode':
            block.string_content = block.strings.join('\n').replace(/(\n *)*$/,'\n');
            break;

        case 'FencedCode':
            // first line becomes info string
            block.info = unescapeEntBS(block.strings[0].trim());
            if (block.strings.length == 1) {
                block.string_content = '';
            } else {
                block.string_content = block.strings.slice(1).join('\n') + '\n';
            }
            break;

        case 'List':
            block.tight = true; // tight by default

            var numitems = block.children.length;
            var i = 0;
            while (i < numitems) {
                var item = block.children[i];
                // check for non-final list item ending with blank line:
                var last_item = i == numitems - 1;
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
                    var last_subitem = j == numsubitems - 1;
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
    // into inline content where appropriate.
    var processInlines = function(block) {
        switch(block.t) {
        case 'Paragraph':
        case 'SetextHeader':
        case 'ATXHeader':
            block.inline_content =
                this.inlineParser.parse(block.string_content.trim(), this.refmap);
            block.string_content = "";
            break;
        default:
            break;
        }

        if (block.children) {
            for (var i = 0; i < block.children.length; i++) {
                this.processInlines(block.children[i]);
            }
        }

    };

    // The main parsing function.  Returns a parsed document AST.
    var parse = function(input) {
        this.doc = makeBlock('Document', 1, 1);
        this.tip = this.doc;
        this.refmap = {};
        var lines = input.replace(/\n$/,'').split(/\r\n|\n|\r/);
        var len = lines.length;
        for (var i = 0; i < len; i++) {
            this.incorporateLine(lines[i], i+1);
        }
        while (this.tip) {
            this.finalize(this.tip, len - 1);
        }
        this.processInlines(this.doc);
        return this.doc;
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

    module.exports.DocParser = DocParser;
    module.exports.HtmlRenderer = require('./html-renderer.js');
