#include "buffer.h"

int scan_autolink_uri(const gh_buf *s, int pos);
int scan_autolink_email(const gh_buf *s, int pos);
int scan_html_tag(const gh_buf *s, int pos);
int scan_html_block_tag(const gh_buf *s, int pos);
int scan_link_url(const gh_buf *s, int pos);
int scan_link_title(const gh_buf *s, int pos);
int scan_spacechars(const gh_buf *s, int pos);
int scan_atx_header_start(const gh_buf *s, int pos);
int scan_setext_header_line(const gh_buf *s, int pos);
int scan_hrule(const gh_buf *s, int pos);
int scan_open_code_fence(const gh_buf *s, int pos);
int scan_close_code_fence(const gh_buf *s, int pos, int len);
int scan_entity(const gh_buf *s, int pos);
