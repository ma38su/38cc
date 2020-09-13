#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "38cc.h"

int is_alnum(char c);
int lvar_len(char *p0);
int starts_with(char *p, int pl, char *string);

Token *new_token(TokenKind kind, Token *cur, char *str);
char *next_ptr(char *p0, char c);
char to_escape_char(char v);
char *skip_brackets(char *p);
Token *read_char_literal(Token *cur, char *p);
Token *read_str_literal(Token *cur, char *p);

bool is_space(char p);
char *skip_brackets(char *p);

int lvar_len(char *p0) {
  char *p = p0;
  if (('a' <= *p && *p <= 'z')
      || ('A' <= *p && *p <= 'Z')
      || *p == '_') {
    do {
      p++;
    } while (is_alnum(*p));
  }
  return p - p0;
}

Token *tokenize() {
  char *p = user_input;

  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // skip blank
    if (is_space(*p)) {
      p++;
      continue;
    }

    // skip line comment
    if (memcmp(p, "//", 2) == 0) {
      p += 2;
      p = next_ptr(p, '\n');
      p++;
      continue;
    }

    // skip block comment
    if (memcmp(p, "/*", 2) == 0) {
      p += 2;
      while (*p != '*' || *(p+1) != '/') {
        p++;
      }
      p += 2;
      continue;
    }

    // skip preprocessor
    if (*p == '#') {
      p++;
      p = next_ptr(p, '\n');
      p++;
      continue;
    }

    Token *tok;
    tok = read_char_literal(cur, p);
    if (tok) {
      p += tok->len + 2;
      cur = tok;
      continue;
    }

    tok = read_str_literal(cur, p);
    if (tok) {
      p += tok->len + 2;
      cur = tok;
      continue;
    }

    if (memcmp(p, ">>=", 3) == 0 || memcmp(p, "<<=", 3) == 0 || memcmp(p, "...", 3) == 0) {
      cur = new_token(TK_RESERVED, cur, p);
      cur->len = 3;
      p += 3;
      continue;
    }

    if (memcmp(p, "==", 2) == 0 || memcmp(p, "!=", 2) == 0
        || memcmp(p, "<=", 2) == 0 || memcmp(p, ">=", 2) == 0
        || memcmp(p, "+=", 2) == 0 || memcmp(p, "-=", 2) == 0
        || memcmp(p, "*=", 2) == 0 || memcmp(p, "/=", 2) == 0
        || memcmp(p, "&=", 2) == 0 || memcmp(p, "|=", 2) == 0
        || memcmp(p, "^=", 2) == 0 || memcmp(p, "~=", 2) == 0
        || memcmp(p, "++", 2) == 0 || memcmp(p, "--", 2) == 0
        || memcmp(p, "&&", 2) == 0 || memcmp(p, "||", 2) == 0
        || memcmp(p, ">>", 2) == 0 || memcmp(p, "<<", 2) == 0
        || memcmp(p, "->", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p);
      cur->len = 2;
      p += 2;
      continue;
    }

    if (strchr("=,.(){}[]<>+-*/%^;&^|~!?:", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 1;
      continue;
    }

    if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 16);
      continue;
    }
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    // skip __attribute__ (*)
    if (memcmp(p, "__attribute__", 13) == 0) {
      p += 13;
      p = skip_brackets(p);
      continue;
    }

    int l = lvar_len(p);
    if (l > 0) {
      // skip
      if (starts_with(p, l, "signed") || starts_with(p, l, "volatile")) {
        p += l;
        continue;
      }
      if (memcmp(p, "__", 2) == 0) {
        if (starts_with(p, l, "__extension__")
            || starts_with(p, l, "__restrict")) {
          p += l;
          continue;
        }

        // skip builtin
        /*
        if (starts_with(p, l, "__builtin_bswap32")
            || starts_with(p, l, "__builtin_bswap64")) {
          p += l;
          continue;
        }
        */

        if (starts_with(p, l, "__inline")) {
          cur = new_token(TK_RESERVED, cur, p);
          cur->len = l;
          p += l;
          continue;
        }
      }

      if (starts_with(p, l, "do") || starts_with(p, l, "while")
          || starts_with(p, l, "continue") || starts_with(p, l, "break")
          || starts_with(p, l, "for") || starts_with(p, l, "return")
          || starts_with(p, l, "if") || starts_with(p, l, "else")
          || starts_with(p, l, "struct") || starts_with(p, l, "enum")
          || starts_with(p, l, "union") || starts_with(p, l, "typedef")
          || starts_with(p, l, "const") || starts_with(p, l, "static")
          || starts_with(p, l, "extern") || starts_with(p, l, "sizeof")) {
        cur = new_token(TK_RESERVED, cur, p);
      } else {
        cur = new_token(TK_IDENT, cur, p);
      }
      p += l;
      cur->len = l;
      continue;
    }
    
    error_at(p, "unexpected token %d %d", l, sizeof(p));
  }
  new_token(TK_EOF, cur, p);
  return head.next;
}
