#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
//#include <ctype.h>
//#include <stdbool.h>
//#include <string.h>

#include "mcc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    return 1;
  }

  user_input = argv[1];
  tokenize(user_input);
  program();

  print_header();

  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  for (int i = 0; code[i]; i++) {
    gen(code[i]);

    printf("  pop rax\n");
  }

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

