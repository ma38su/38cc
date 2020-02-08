#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "38cc.h"

char *filename;

void dump_tokenize() {
  Token *p;
  int count = 0;

  p = token;
  while (p) {
    if (p->kind == TK_NUM) {
      printf(" %d", p->val);
    } else {
      printf(" %s", substring(p->str, p->len));
    }
    if (++count >= 10) {
      printf("\n");
      count = 0;
    }
    p = p->next;
  }
  printf("\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    return 1;
  }

  filename = argv[1];
  user_input = read_file(filename);

  token = tokenize(user_input);
  // dump_tokenize();
  // return 0;

  program();

  print_header();
  for (int i = 0; code[i]; i++) {
    printf("# gen %d\n", i);
    gen_defined(code[i]);
  }
  return 0;
}

void error_at(char *loc, char *fmt, ...) {

  char *line = loc;
  while (user_input < line && line[-1] != '\n') {
    line--;
  }

  char *end = loc;
  while (*end != '\n') {
    end++;
  }

  int line_num = 1;
  for (char *p = user_input; p < line; ++p) {
    if (*p == '\n') {
      line_num++;
    }
  }

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int) (end - line), line);

  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error(char *fmt, ...) {
  fprintf(stderr, "ERR: ");

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  exit(1);
}

void assert(int condition) {
  if (!condition) {
    error("assertion error");
  }
}
