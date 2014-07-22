#include "bstrlib.h"

// UTF-8 aware detab:  assumes s has no newlines, or only a final newline.
// Return 0 on success, BSTR_ERR if invalid UTF-8.
extern int bdetab(bstring s, int utf8)
{
  unsigned char c;
  int pos = 0;  // a count of characters
  int byte = 0; // a count of bytes
  int high_chars_to_skip = 0;
  int numspaces = 0;
  while ((c = bchar(s, byte))) {
    if (utf8 && high_chars_to_skip > 0) {
      if (c >= 0x80) {
        high_chars_to_skip--;
        byte++;
      } else {
        return BSTR_ERR; // invalid utf-8
      }
    } else if (c == '\t') {
      bdelete(s, byte, 1); // delete tab character
      numspaces = 4 - (pos % 4);
      binsertch(s, byte, numspaces, ' ');
      byte += numspaces;
      pos  += numspaces;
    } else if (c <= 0x80 || !utf8) {
      byte++;
      pos++;
    } else {  // multibyte utf8 sequences
      if (c >> 1 == 0176) {
        high_chars_to_skip = 5;
      } else if (c >> 2 == 076) {
        high_chars_to_skip = 4;
      } else if (c >> 3 == 036) {
        high_chars_to_skip = 3;
      } else if (c >> 4 == 016) {
        high_chars_to_skip = 2;
      } else if (c >> 5 == 06) {
        high_chars_to_skip = 1;
      } else {
        return BSTR_ERR; // invalid utf-8
      }
      pos++;
      byte++;
    }
  }
  return 0;
}
