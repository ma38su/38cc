#include <stdio.h>

#include "38cc.h"

char *filename;

bool is_debug = false;

void _main();

int main(int argc, char *argv[]) {
  fprintf(stderr, "call main: %d\n", argc);
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    return 1;
  }

  filename = argv[1];
  fprintf(stderr, "argv[1]: %s\n", argv[0]);
  if (filename == NULL) {
    printf("  illegal argments.\n");
    return 1;
  }
  fprintf(stderr, "arg file: %s\n", filename);
  fprintf(stderr, "%ld", globals);
  _main();
  return 0;
}
