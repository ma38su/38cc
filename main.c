#include <stdio.h>

#include "38cc.h"

char *filename;

int main(int argc, char **argv) {
  fprintf(stderr, "Hello, 38cc!\n");
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    return 1;
  }

  filename = argv[1];
  if (filename == NULL) {
    printf("  illegal argments.\n");
    return 1;
  }
  printf("  .file \"%s\"\n", filename);

  fprintf(stderr, "read_file! %s\n", filename);
  user_input = read_file(filename);

  fprintf(stderr, "tokenize()\n");
  token = tokenize();
  if (!token) {
    fprintf(stderr, "No tokens\n");
    return 0;
  }

  fprintf(stderr, "program()\n");
  program();
  fprintf(stderr, "codegen()\n");
  codegen();
  return 0;
}
