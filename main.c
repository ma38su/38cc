#include <stdio.h>

#include "38cc.h"

char *filename;
bool is_debug = false;

char *read_file(char *path);

void dump_args(int argc, char** argv) {
  fprintf(stderr, "argc: %d\n", argc);
  for (int i = 0; i < argc; ++i) {
    fprintf(stderr, "argv[%d]: %s\n", i, argv[i]);
  }
}

int main(int argc, char *argv[]) {
  fprintf(stderr, "call main: %d\n", argc);
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    dump_args(argc, argv);
    return 1;
  }

  if (is_debug) dump_args(argc, argv);

  filename = argv[1];
  if (filename == NULL) {
    fprintf(stderr, "no arg file\n");
    return 1;
  }

  if (is_debug) fprintf(stderr, "read_file... %s\n", filename);

  printf("  .file \"%s\"\n", filename);
  user_input = read_file(filename);

  if (is_debug) fprintf(stderr, "tokenize...\n");

  token = tokenize(user_input);

  if (!token) error("No tokens!");

  if (is_debug) fprintf(stderr, "program...\n");
  program();
  if (is_debug) fprintf(stderr, "codegen...\n");
  codegen();
  if (is_debug) fprintf(stderr, "end...\n");
}
