#ifndef _H_cmark_UTF8_
#define _H_cmark_UTF8_

#include <stdint.h>
#include "buffer.h"

void utf8proc_case_fold(strbuf *dest, const uint8_t *str, int len);
void utf8proc_encode_char(int32_t uc, strbuf *buf);
int utf8proc_iterate(const uint8_t *str, int str_len, int32_t *dst);
int utf8proc_charlen(const uint8_t *str, int str_len);
void utf8proc_detab(strbuf *dest, const uint8_t *line, size_t size);

#endif
