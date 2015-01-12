var entityToChar = require('./html5-entities.js').entityToChar;

var ENTITY = "&(?:#x[a-f0-9]{1,8}|#[0-9]{1,8}|[a-z][a-z0-9]{1,31});";

var reBackslashOrAmp = /[\\&]/;

var ESCAPABLE = '[!"#$%&\'()*+,./:;<=>?@[\\\\\\]^_`{|}~-]';

var reEntityOrEscapedChar = new RegExp('\\\\' + ESCAPABLE + '|' + ENTITY, 'gi');

var unescapeChar = function(s) {
    "use strict";
    if (s[0] === '\\') {
        return s[1];
    } else {
        return entityToChar(s);
    }
};

// Replace entities and backslash escapes with literal characters.
var unescapeString = function(s) {
    "use strict";
    if (reBackslashOrAmp.test(s)) {
        return s.replace(reEntityOrEscapedChar, unescapeChar);
    } else {
        return s;
    }
};

var normalizeURI = function(uri) {
    "use strict";
    return encodeURI(unescape(uri));
}

module.exports = { unescapeString: unescapeString,
                   normalizeURI: normalizeURI
                 };
