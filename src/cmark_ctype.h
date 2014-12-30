/** Locale-independent versions of functions from ctype.h.
 * We want cmark to behave the same no matter what the system locale.
 */

int cmark_isspace(char c);

int cmark_ispunct(char c);

int cmark_isalnum(char c);

int cmark_isdigit(char c);
