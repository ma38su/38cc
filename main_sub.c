#include <stdio.h>
#include "38cc.h"

char *read_file(char *path);

void _main() {
  if (is_debug) fprintf(stderr, "read_file...\n");

  printf("  .file \"%s\"\n", filename);
  //user_input = read_file(filename);
  user_input = read_file(filename);

  if (is_debug) fprintf(stderr, "tokenize...\n");
  token = tokenize(user_input);
  if (!token) {
    error("No tokens!");
    return;
  }

  if (is_debug) fprintf(stderr, "program...\n");
  program();
  if (is_debug) fprintf(stderr, "codegen...\n");
  codegen();
  if (is_debug) fprintf(stderr, "end...\n");
}
