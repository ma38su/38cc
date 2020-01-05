#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "mcc.h"

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


  user_input = argv[1];
  token = tokenize(user_input);
  // dump_tokenize();
  // return 0;

  program();
  print_header();
  for (int i = 0; code[i]; i++) {
    gen_defined(code[i]);
  }
  return 0;
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  // while (token) {
  //   fprintf(stderr, "%s\n", token->str);
  //   token = token->next;
  // }
  exit(1);
}
