/** Locale-independent versions of functions from ctype.h.
 * We want cmark to behave the same no matter what the system locale.
 */

int isspace(char c);

int ispunct(char c);

int isalnum(char c);

int isdigit(char c);
