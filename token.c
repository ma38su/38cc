#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "mcc.h"


Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

bool is_alnum(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') || (c == '_');
}

int lvar_len(char *p0) {
  char *p = p0;
  if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z')) {
    while (is_alnum(*p)) {
      p++;
    }
  }
  return p - p0;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  int cnt = *p;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (memcmp(p, "==", 2) == 0 || memcmp(p, "!=", 2) == 0 ||
        memcmp(p, "<=", 2) == 0 || memcmp(p, ">=", 2) == 0 ||
        memcmp(p, "+=", 2) == 0 || memcmp(p, "-=", 2) == 0 ||
        memcmp(p, "*=", 2) == 0 || memcmp(p, "/=", 2) == 0 ||
        memcmp(p, "&=", 2) == 0 || memcmp(p, "|=", 2) == 0 ||
        memcmp(p, "++", 2) == 0 || memcmp(p, "--", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p);
      cur->len = 2;
      p += 2;
      continue;
    }

    if (strchr("=,(){}<>+-*/%^;", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 1;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    int l = lvar_len(p);
    if (l > 0) {
      if (l == 6 && memcmp(p, "return", l) == 0) {
        cur = new_token(TK_RETURN, cur, p);
      } else if (l == 2 && memcmp(p, "if", l) == 0) {
        cur = new_token(TK_IF, cur, p);
      } else if (l == 4 && memcmp(p, "else", l) == 0) {
        cur = new_token(TK_ELSE, cur, p);
      } else if (l == 5 && memcmp(p, "while", l) == 0) {
        cur = new_token(TK_WHILE, cur, p);
      } else if (l == 3 && memcmp(p, "for", l) == 0) {
        cur = new_token(TK_FOR, cur, p);
      } else {
        cur = new_token(TK_IDENT, cur, p);
      }
      p += l;
      cur->len = l;
      continue;
    }

    error_at(p, "unexpected token");
  }
  new_token(TK_EOF, cur, p);

  return head.next;
}

