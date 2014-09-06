#ifndef _H_STMD_UTF8_
#define _H_STMD_UTF8_

#include "buffer.h"

void utf8proc_case_fold(strbuf *dest, const unsigned char *str, int len);
void utf8proc_encode_char(int32_t uc, strbuf *buf);
ssize_t utf8proc_iterate(const uint8_t *str, ssize_t str_len, int32_t *dst);
ssize_t utf8proc_charlen(const uint8_t *str, ssize_t str_len);
void utf8proc_detab(strbuf *dest, const unsigned char *line, size_t size);

#endif
