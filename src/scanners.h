#include "bstrlib.h"

int scan_autolink_uri(bstring s, int pos);
int scan_autolink_email(bstring s, int pos);
int scan_html_tag(bstring s, int pos);
int scan_html_block_tag(bstring s, int pos);
int scan_link_url(bstring s, int pos);
int scan_link_title(bstring s, int pos);
int scan_spacechars(bstring s, int pos);
int scan_atx_header_start(bstring s, int pos);
int scan_setext_header_line(bstring s, int pos);
int scan_hrule(bstring s, int pos);
int scan_open_code_fence(bstring s, int pos);
int scan_close_code_fence(bstring s, int pos, int len);
int scan_entity(bstring s, int pos);
