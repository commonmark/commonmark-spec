#include <unistd.h>
#include <stdlib.h>
#include "libstmd.h"

#define TEST_STRING "# Testing\n\nThis is a simple Test\n\nOf the library"

int main(int argc, char *argv[]) {
  char * ret;
  size_t ret_len;

  if(stmd_process_block(TEST_STRING, sizeof(TEST_STRING)-1, &ret, &ret_len)) { //-1 because there is no need for the tailing \0 (but including it is harmless)
    write(STDOUT_FILENO, ret, ret_len);
    free(ret); //Don't forget to free the memory after
  } else {
    //there should never be any errors, no matter the input. An error indicates a bug.
    return 1;
  }

  return 0;
}
