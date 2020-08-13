#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// for debug
#include <stdio.h>

#include "38cc.h"

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

bool is_alnum(char c) {
  return ('a' <= c && c <= 'z')
    || ('A' <= c && c <= 'Z')
    || ('0' <= c && c <= '9')
    || (c == '_');
}

int lvar_len(char *p0) {
  char *p = p0;
  if (('a' <= *p && *p <= 'z')
    || ('A' <= *p && *p <= 'Z')
    || *p == '_') {

    while (is_alnum(*p)) p++;
  }
  return p - p0;
}

char *next_ptr(char *p0, char c) {
  char *p = p0;
  while (*p != c) p++;
  return p;
}

char to_escape_char(char v) {
  if (v == '0') {
    return '\0';
  } else if (v == 'a') {
    return '\a'; // beep
  } else if (v == 'b') {
    return '\b'; // backspace
  } else if (v == 'f') {
    return '\f';
  } else if (v == 'n') {
    return '\n';
  } else if (v == 'r') {
    return '\r';
  } else if (v == 't') {
    return '\t';
  } else {
    return v;
  }
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // skip blank
    if (isspace(*p)) {
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
    // skip __extension__
    if (memcmp(p, "__extension__", 13) == 0) {
      p += 13;
      continue;
    }
    // skip __restrict
    if (memcmp(p, "__restrict", 10) == 0) {
      p += 10;
      continue;
    }
    // skip const
    if (memcmp(p, "const", 5) == 0) {
      cur = new_token(TK_CONST, cur, p);
      cur->len = 5;
      p += 5;
      continue;
    }

    // character literal
    if (*p == '\'') {
      p++;

      cur = new_token(TK_CHAR, cur, p);

      if (*p == '\\') {
        cur->len = 3;
        cur->val = to_escape_char(*(++p));
      } else {
        cur->len = 1;
      }
      p++;

      if (*p != '\'') {
        error_at(p, "unexpected token. expected token is \"'\"");
      }
      p++;
      continue;
    }

    // string literal
    if (*p == '"') {
      p++;

      cur = new_token(TK_STR, cur, p);

      char* p0 = p;
      p = next_ptr(p, '"');
      cur->len = p - p0;

      p++;
      continue;
    }
    if (memcmp(p, "enum", 4) == 0) {
      cur = new_token(TK_ENUM, cur, p);
      cur->len = 4;
      p += 4;
      continue;
    }
    if (memcmp(p, "struct", 6) == 0) {
      cur = new_token(TK_STRUCT, cur, p);
      cur->len = 6;
      p += 6;
      continue;
    }
    if (memcmp(p, "union", 5) == 0) {
      cur = new_token(TK_UNION, cur, p);
      cur->len = 5;
      p += 5;
      continue;
    }
    if (memcmp(p, "extern", 6) == 0) {
      cur = new_token(TK_EXTERN, cur, p);
      cur->len = 6;
      p += 6;
      continue;
    }
    if (memcmp(p, "typedef", 7) == 0) {
      cur = new_token(TK_TYPEDEF, cur, p);
      cur->len = 7;
      p += 7;
      continue;
    }
    if (memcmp(p, "...", 3) == 0) {
      cur = new_token(TK_VA, cur, p);
      cur->len = 3;
      p += 3;
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

    if (strchr("=,(){}[]<>+-*/%^;&", *p)) {
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
      if (l == 6 && memcmp(p, "sizeof", l) == 0) {
        cur = new_token(TK_SIZEOF, cur, p);
      } else if (l == 6 && memcmp(p, "return", l) == 0) {
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

