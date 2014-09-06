#include <cm.h>

/*!re2c
  re2c:define:YYCTYPE  = "unsigned char";
  re2c:define:YYCURSOR = p;
  re2c:define:YYMARKER = marker;
  re2c:define:YYCTXMARKER = marker;
  re2c:yyfill:enable = 0;

  wordchar = [^\x00-\x20];

  spacechar = [ \t\n];

  reg_char     = [^\\()\x00-\x20];

  escaped_char = [\\][!"#$%&'()*+,./:;<=>?@[\\\]^_`{|}~-];

  tagname = [A-Za-z][A-Za-z0-9]*;

  blocktagname = 'article'|'header'|'aside'|'hgroup'|'blockquote'|'hr'|'body'|'li'|'br'|'map'|'button'|'object'|'canvas'|'ol'|'caption'|'output'|'col'|'p'|'colgroup'|'pre'|'dd'|'progress'|'div'|'section'|'dl'|'table'|'td'|'dt'|'tbody'|'embed'|'textarea'|'fieldset'|'tfoot'|'figcaption'|'th'|'figure'|'thead'|'footer'|'footer'|'tr'|'form'|'ul'|'h1'|'h2'|'h3'|'h4'|'h5'|'h6'|'video'|'script'|'style';

  attributename = [a-zA-Z_:][a-zA-Z0-9:._-]*;

  unquotedvalue = [^\"'=<>`\x00]+;
  singlequotedvalue = ['][^'\x00]*['];
  doublequotedvalue = [\"][^\"\x00]*[\"];

  attributevalue = unquotedvalue | singlequotedvalue | doublequotedvalue;

  attributevaluespec = spacechar* [=] spacechar* attributevalue;

  attribute = spacechar+ attributename attributevaluespec?;

  opentag = tagname attribute* spacechar* [/]? [>];
  closetag = [/] tagname spacechar* [>];

  htmlcomment = "!--" ([^-\x00]+ | [-][^-\x00]+)* "-->";

  processinginstruction = "?" ([^?>\x00]+ | [?][^>\x00])* "?>";

  declaration = "!" [A-Z]+ spacechar+ [^>\x00]* ">";

  cdata = "![CDATA[" ([^\]\x00]+ | "]" [^\]\x00] | "]]" [^>\x00])* "]]>";

  htmltag = opentag | closetag | htmlcomment | processinginstruction |
            declaration | cdata;

  in_parens_nosp   = [(] (reg_char|escaped_char)* [)];

  in_double_quotes = ["] (escaped_char|[^"\x00])* ["];
  in_single_quotes = ['] (escaped_char|[^'\x00])* ['];
  in_parens        = [(] (escaped_char|[^)\x00])* [)];

  scheme = 'coap'|'doi'|'javascript'|'aaa'|'aaas'|'about'|'acap'|'cap'|'cid'|'crid'|'data'|'dav'|'dict'|'dns'|'file'|'ftp'|'geo'|'go'|'gopher'|'h323'|'http'|'https'|'iax'|'icap'|'im'|'imap'|'info'|'ipp'|'iris'|'iris.beep'|'iris.xpc'|'iris.xpcs'|'iris.lwz'|'ldap'|'mailto'|'mid'|'msrp'|'msrps'|'mtqp'|'mupdate'|'news'|'nfs'|'ni'|'nih'|'nntp'|'opaquelocktoken'|'pop'|'pres'|'rtsp'|'service'|'session'|'shttp'|'sieve'|'sip'|'sips'|'sms'|'snmp'|'soap.beep'|'soap.beeps'|'tag'|'tel'|'telnet'|'tftp'|'thismessage'|'tn3270'|'tip'|'tv'|'urn'|'vemmi'|'ws'|'wss'|'xcon'|'xcon-userid'|'xmlrpc.beep'|'xmlrpc.beeps'|'xmpp'|'z39.50r'|'z39.50s'|'adiumxtra'|'afp'|'afs'|'aim'|'apt'|'attachment'|'aw'|'beshare'|'bitcoin'|'bolo'|'callto'|'chrome'|'chrome-extension'|'com-eventbrite-attendee'|'content'|'cvs'|'dlna-playsingle'|'dlna-playcontainer'|'dtn'|'dvb'|'ed2k'|'facetime'|'feed'|'finger'|'fish'|'gg'|'git'|'gizmoproject'|'gtalk'|'hcp'|'icon'|'ipn'|'irc'|'irc6'|'ircs'|'itms'|'jar'|'jms'|'keyparc'|'lastfm'|'ldaps'|'magnet'|'maps'|'market'|'message'|'mms'|'ms-help'|'msnim'|'mumble'|'mvn'|'notes'|'oid'|'palm'|'paparazzi'|'platform'|'proxy'|'psyc'|'query'|'res'|'resource'|'rmi'|'rsync'|'rtmp'|'secondlife'|'sftp'|'sgn'|'skype'|'smb'|'soldat'|'spotify'|'ssh'|'steam'|'svn'|'teamspeak'|'things'|'udp'|'unreal'|'ut2004'|'ventrilo'|'view-source'|'webcal'|'wtai'|'wyciwyg'|'xfire'|'xri'|'ymsgr';
*/

// Try to match URI autolink after first <, returning number of chars matched.
STMD_API int scan_autolink_uri(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  scheme [:]([^\x00-\x20<>\\]|escaped_char)*[>]  { return (p - start); }
  .? { return 0; }
*/
}

// Try to match email autolink after first <, returning num of chars matched.
STMD_API int scan_autolink_email(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+
    [@]
    [a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?
    ([.][a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*
    [>] { return (p - start); }
  .? { return 0; }
*/
}

// Try to match an HTML tag after first <, returning num of chars matched.
STMD_API int scan_html_tag(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  htmltag { return (p - start); }
  .? { return 0; }
*/
}

// Try to match an HTML block tag including first <,
// returning num of chars matched.
STMD_API int scan_html_block_tag(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [<] [/] blocktagname (spacechar | [>])  { return (p - start); }
  [<] blocktagname (spacechar | [/>]) { return (p - start); }
  [<] [!?] { return (p - start); }
  .? { return 0; }
*/
}

// Try to match a URL in a link or reference, return number of chars matched.
// This may optionally be contained in <..>; otherwise
// whitespace and unbalanced right parentheses aren't allowed.
// Newlines aren't ever allowed.
STMD_API int scan_link_url(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [ \n]* [<] ([^<>\n\\\x00] | escaped_char | [\\])* [>] { return (p - start); }
  [ \n]* (reg_char+ | escaped_char | in_parens_nosp)* { return (p - start); }
  .? { return 0; }
*/
}

// Try to match a link title (in single quotes, in double quotes, or
// in parentheses), returning number of chars matched.  Allow one
// level of internal nesting (quotes within quotes).
STMD_API int scan_link_title(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  ["] (escaped_char|[^"\x00])* ["]   { return (p - start); }
  ['] (escaped_char|[^'\x00])* ['] { return (p - start); }
  [(] (escaped_char|[^)\x00])* [)]  { return (p - start); }
  .? { return 0; }
*/
}

// Match space characters, including newlines.
STMD_API int scan_spacechars(bstring s, int pos)
{
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [ \t\n]* { return (p - start); }
  . { return 0; }
*/
}

// Match ATX header start.
STMD_API int scan_atx_header_start(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [#]{1,6} ([ ]+|[\n])  { return (p - start); }
  .? { return 0; }
*/
}

// Match sexext header line.  Return 1 for level-1 header,
// 2 for level-2, 0 for no match.
STMD_API int scan_setext_header_line(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
/*!re2c
  [=]+ [ ]* [\n] { return 1; }
  [-]+ [ ]* [\n] { return 2; }
  .? { return 0; }
*/
}

// Scan a horizontal rule line: "...three or more hyphens, asterisks,
// or underscores on a line by themselves. If you wish, you may use
// spaces between the hyphens or asterisks."
STMD_API int scan_hrule(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  ([*][ ]*){3,} [ \t]* [\n] { return (p - start); }
  ([_][ ]*){3,} [ \t]* [\n] { return (p - start); }
  ([-][ ]*){3,} [ \t]* [\n] { return (p - start); }
  .? { return 0; }
*/
}

// Scan an opening code fence.
STMD_API int scan_open_code_fence(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [`]{3,} / [^`\n\x00]*[\n] { return (p - start); }
  [~]{3,} / [^~\n\x00]*[\n] { return (p - start); }
  .?                        { return 0; }
*/
}

// Scan a closing code fence with length at least len.
STMD_API int scan_close_code_fence(bstring s, int pos, int len)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  ([`]{3,} | [~]{3,}) / spacechar* [\n]
                              { if (p - start > len) {
                                return (p - start);
                              } else {
                                return 0;
                              } }
  .? { return 0; }
*/
}

// Scans an entity.
// Returns number of chars matched.
STMD_API int scan_entity(bstring s, int pos)
{
  unsigned char * marker = NULL;
  unsigned char * p = &(s->data[pos]);
  unsigned char * start = p;
/*!re2c
  [&] ([#] ([Xx][A-Fa-f0-9]{1,8}|[0-9]{1,8}) |[A-Za-z][A-Za-z0-9]{1,31} ) [;]
     { return (p - start); }
  .? { return 0; }
*/
}
