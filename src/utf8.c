#include <stdlib.h>
#include "bstrlib.h"
#include "debug.h"

#define advance(s) \
  s++; \
  check(*s >> 6 == 0x02, "UTF-8 decode error on byte %x", *s);

// Reads a unicode code point from a UTF8-encoded string, and
// puts it in the pointer n. If something illegal
// is encountered, 0xFFFD is emitted.
// Returns a pointer to next position in string, or NULL if no
// more characters remain.
extern unsigned char * from_utf8(unsigned char * s, unsigned int *n)
{
  int x = 0;

  if (*s == 0) {
    return NULL;
  } else if (*s < 0x80) {
    x = *s;
  } else if (*s >> 5 == 0x06) {
    x = *s & 0x1F;
    advance(s);
    x = (x << 6) + (*s & 0x3F);
  } else if (*s >> 4 == 0x0E) {
    x = *s & 0x0F;
    advance(s);
    x = (x << 6) + (*s & 0x3F);
    advance(s);
    x = (x << 6) + (*s & 0x3F);
  } else if (*s >> 3 == 0x1E) {
    x = *s & 0x07;
    advance(s);
    x = (x << 6) + (*s & 0x3F);
    advance(s);
    x = (x << 6) + (*s & 0x3F);
    advance(s);
    x = (x << 6) + (*s & 0x3F);
  } else if (*s >> 2 == 0x3E) {
    x = *s & 0x03;
    advance(s);
    x = (x << 6) + (*s & 0x3F);
    advance(s);
    x = (x << 6) + (*s & 0x3F);
    advance(s);
    x = (x << 6) + (*s & 0x3F);
    advance(s);
    x = (x << 6) + (*s & 0x3F);
   } else {
    log_err("UTF-8 decode error on byte %x", *s);
    goto error;
  }
  *n = x;
  s++;
  return s;
 error:
  *n = 0xFFFD;
  return s;
}

// Converts the unicode code point c to UTF-8,
// putting the result in dest.  Returns 0 on success, -1 on error.
extern int to_utf8(unsigned int c, bstring dest)
{
  if (c < 0x80) {
    bconchar(dest, c);
  } else if (c < 0x800) {
    bconchar(dest, 192 + c/64);
    bconchar(dest, 128 + c%64);
  } else if (c - 0xd800u < 0x800) {
    goto error;
  } else if (c < 0x10000) {
    bconchar(dest, 224 + c / 4096);
    bconchar(dest, 128 + c /64%64);
    bconchar(dest, 128 + c%64);
  } else if (c < 0x110000) {
    bconchar(dest, 240 + c/262144);
    bconchar(dest, 128 + c/4096%64);
    bconchar(dest, 128 + c/64%64);
    bconchar(dest, 128 + c%64);
  } else {
    goto error;
  }
  return 0;
error:
  return -1;
}

#define bufpush(x) \
  check(to_utf8(x, buf) == 0, "UTF-8 encode error on code point  %04x", x)

// Returns the case-folded version of the source string, or NULL on error.
extern bstring case_fold(bstring source)
{
  unsigned char * s = source->data;
  unsigned int c = 0;
  bstring buf = bfromcstr("");
  while ((s = from_utf8(s, &c))) {
#include "case_fold_switch.c"
  }
  return buf;
error:
  return NULL;
}

