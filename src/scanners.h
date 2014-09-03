#include "stmd.h"

int scan_autolink_uri(chunk *c, int offset);
int scan_autolink_email(chunk *c, int offset);
int scan_html_tag(chunk *c, int offset);
int scan_html_block_tag(chunk *c, int offset);
int scan_link_url(chunk *c, int offset);
int scan_link_title(chunk *c, int offset);
int scan_spacechars(chunk *c, int offset);
int scan_atx_header_start(chunk *c, int offset);
int scan_setext_header_line(chunk *c, int offset);
int scan_hrule(chunk *c, int offset);
int scan_open_code_fence(chunk *c, int offset);
int scan_close_code_fence(chunk *c, int offset, int len);
int scan_entity(chunk *c, int offset);
