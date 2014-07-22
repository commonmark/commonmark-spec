#include <stdlib.h>
#include "bstrlib.h"

extern unsigned char * from_utf8(unsigned char * s, unsigned int *n);
extern int to_utf8(unsigned int c, bstring dest);
extern bstring case_fold(bstring source);
