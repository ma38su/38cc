#include <stdio.h>

#include "38cc.h"

char *filename;
bool is_debug = false;

char *read_file(char *path);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments: %d\n", argc);
    return 1;
  }

  filename = argv[1];
  user_input = read_file(filename);

  if (is_debug) fprintf(stderr, "tokenize...\n");

  token = tokenize(user_input);

  if (!token) error("No tokens!");

  program();

  codegen();
}
