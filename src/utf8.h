#ifndef CMARK_UTF8_H
#define CMARK_UTF8_H

#include <stdint.h>
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void utf8proc_case_fold(cmark_strbuf *dest, const uint8_t *str, int len);
void utf8proc_encode_char(int32_t uc, cmark_strbuf *buf);
int utf8proc_iterate(const uint8_t *str, int str_len, int32_t *dst);
void utf8proc_detab(cmark_strbuf *dest, const uint8_t *line, size_t size);
int utf8proc_is_space(int32_t uc);
int utf8proc_is_punctuation(int32_t uc);

#ifdef __cplusplus
}
#endif

#endif
