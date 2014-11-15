#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "utf8.h"

static const int8_t utf8proc_utf8class[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0 };

static void encode_unknown(strbuf *buf)
{
	static const uint8_t repl[] = {239, 191, 189};
	strbuf_put(buf, repl, 3);
}

int utf8proc_charlen(const uint8_t *str, int str_len)
{
	int length, i;

	if (!str_len)
		return 0;

	length = utf8proc_utf8class[str[0]];

	if (!length)
		return -1;

	if (str_len >= 0 && length > str_len)
		return -str_len;

	for (i = 1; i < length; i++) {
		if ((str[i] & 0xC0) != 0x80)
			return -i;
	}

	return length;
}

void utf8proc_detab(strbuf *ob, const uint8_t *line, size_t size)
{
	static const uint8_t whitespace[] = "    ";

	size_t i = 0, tab = 0;

	while (i < size) {
		size_t org = i;

		while (i < size && line[i] != '\t' && line[i] <= 0x80) {
			i++; tab++;
		}

		if (i > org)
			strbuf_put(ob, line + org, i - org);

		if (i >= size)
			break;

		if (line[i] == '\t') {
			int numspaces = 4 - (tab % 4);
			strbuf_put(ob, whitespace, numspaces);
			i += 1;
			tab += numspaces;
		} else {
			int charlen = utf8proc_charlen(line + i, size - i);

			if (charlen >= 0) {
				strbuf_put(ob, line + i, charlen);
			} else {
				encode_unknown(ob);
				charlen = -charlen;
			}

			i += charlen;
			tab += 1;
		}
	}
}

int utf8proc_iterate(const uint8_t *str, int str_len, int32_t *dst)
{
	int length;
	int32_t uc = -1;

	*dst = -1;
	length = utf8proc_charlen(str, str_len);
	if (length < 0)
		return -1;

	switch (length) {
	case 1:
		uc = str[0];
		break;
	case 2:
		uc = ((str[0] & 0x1F) <<  6) + (str[1] & 0x3F);
		if (uc < 0x80) uc = -1;
		break;
	case 3:
		uc = ((str[0] & 0x0F) << 12) + ((str[1] & 0x3F) <<  6)
			+ (str[2] & 0x3F);
		if (uc < 0x800 || (uc >= 0xD800 && uc < 0xE000) ||
		    (uc >= 0xFDD0 && uc < 0xFDF0)) uc = -1;
		break;
	case 4:
		uc = ((str[0] & 0x07) << 18) + ((str[1] & 0x3F) << 12)
			+ ((str[2] & 0x3F) <<  6) + (str[3] & 0x3F);
		if (uc < 0x10000 || uc >= 0x110000) uc = -1;
		break;
	}

	if (uc < 0 || ((uc & 0xFFFF) >= 0xFFFE))
		return -1;

	*dst = uc;
	return length;
}

void utf8proc_encode_char(int32_t uc, strbuf *buf)
{
	uint8_t dst[4];
	int len = 0;

	assert(uc >= 0);

	if (uc < 0x80) {
		dst[0] = uc;
		len = 1;
	} else if (uc < 0x800) {
		dst[0] = 0xC0 + (uc >> 6);
		dst[1] = 0x80 + (uc & 0x3F);
		len = 2;
	} else if (uc == 0xFFFF) {
		dst[0] = 0xFF;
		len = 1;
	} else if (uc == 0xFFFE) {
		dst[0] = 0xFE;
		len = 1;
	} else if (uc < 0x10000) {
		dst[0] = 0xE0 + (uc >> 12);
		dst[1] = 0x80 + ((uc >> 6) & 0x3F);
		dst[2] = 0x80 + (uc & 0x3F);
		len = 3;
	} else if (uc < 0x110000) {
		dst[0] = 0xF0 + (uc >> 18);
		dst[1] = 0x80 + ((uc >> 12) & 0x3F);
		dst[2] = 0x80 + ((uc >> 6) & 0x3F);
		dst[3] = 0x80 + (uc & 0x3F);
		len = 4;
	} else {
		encode_unknown(buf);
		return;
	}

	strbuf_put(buf, dst, len);
}

void utf8proc_case_fold(strbuf *dest, const uint8_t *str, int len)
{
	int32_t c;

#define bufpush(x)				\
	utf8proc_encode_char(x, dest)

	while (len > 0) {
		int char_len = utf8proc_iterate(str, len, &c);

		if (char_len >= 0) {
#include "case_fold_switch.inc"
		} else {
			encode_unknown(dest);
			char_len = -char_len;
		}

		str += char_len;
		len -= char_len;
	}
}
