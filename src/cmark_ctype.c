/**
 * Returns 1 if c is a "whitespace" character as defined by the spec.
 */
int isspace(char c)
{
	return (c == 0x09 ||
		c == 0x20 ||
		c == 0x0a ||
		c == 0x0d);
}

/**
 * Returns 1 if c is an ascii punctuation character.
 */
int ispunct(char c)
{
	return ((c >= 33 && c <= 47) ||
		(c >= 58 && c <= 64) ||
		(c >= 91 && c <= 96) ||
		(c >= 123 && c <= 126));
}

int isalnum(char c)
{
	return ((c >= 48 && c <= 57) ||
		(c >= 65 && c <= 90) ||
		(c >= 97 && c <= 122));
}

int isdigit(char c)
{
	return (c >= 48 && c <= 57);
}
