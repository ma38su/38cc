#include <stdio.h>

#include "38cc.h"

char *filename;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    return 1;
  }

  filename = argv[1];
  user_input = read_file(filename);

  token = tokenize(user_input);

  program();
  codegen();
  return 0;
}


