#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "houdini.h"
#include "utf8.h"
#include "html_unescape.h"

size_t
houdini_unescape_ent(strbuf *ob, const uint8_t *src, size_t size)
{
	size_t i = 0;

	if (size > 3 && src[0] == '#') {
		int codepoint = 0;

		if (_isdigit(src[1])) {
			for (i = 1; i < size && _isdigit(src[i]); ++i) {
				int cp = (codepoint * 10) + (src[i] - '0');

				if (cp < codepoint)
					return 0;

				codepoint = cp;
			}
		}

		else if (src[1] == 'x' || src[1] == 'X') {
			for (i = 2; i < size && _isxdigit(src[i]); ++i) {
				int cp = (codepoint * 16) + ((src[i] | 32) % 39 - 9);

				if (cp < codepoint)
					return 0;

				codepoint = cp;
			}
		}

		if (i < size && src[i] == ';' && codepoint) {
			utf8proc_encode_char(codepoint, ob);
			return i + 1;
		}
	}

	else {
		if (size > MAX_WORD_LENGTH)
			size = MAX_WORD_LENGTH;

		for (i = MIN_WORD_LENGTH; i < size; ++i) {
			if (src[i] == ' ')
				break;

			if (src[i] == ';') {
				const struct html_ent *entity = find_entity((char *)src, i);

				if (entity != NULL) {
					strbuf_put(ob, entity->utf8, entity->utf8_len);
					return i + 1;
				}

				break;
			}
		}
	}

	return 0;
}

int
houdini_unescape_html(strbuf *ob, const uint8_t *src, size_t size)
{
	size_t  i = 0, org, ent;

	while (i < size) {
		org = i;
		while (i < size && src[i] != '&')
			i++;

		if (likely(i > org)) {
			if (unlikely(org == 0)) {
				if (i >= size)
					return 0;

				strbuf_grow(ob, HOUDINI_UNESCAPED_SIZE(size));
			}

			strbuf_put(ob, src + org, i - org);
		}

		/* escaping */
		if (i >= size)
			break;

		i++;

		ent = houdini_unescape_ent(ob, src + i, size - i);
		i += ent;

		/* not really an entity */
		if (ent == 0)
			strbuf_putc(ob, '&');
	}

	return 1;
}

void houdini_unescape_html_f(strbuf *ob, const uint8_t *src, size_t size)
{
	if (!houdini_unescape_html(ob, src, size))
		strbuf_put(ob, src, size);
}
