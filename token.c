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

char *skip_brackets(char *p) {
  // skip (*)
  while (isspace(*p)) ++p;

  if (*p != '(') {
    return p;
  }
  ++p;

  int brackets = 1;
  while (brackets > 0) {
    if (*p == ')') {
      --brackets;
    } else if (*p == '(') {
      ++brackets;
    }
    ++p;
  }
  return p;
}

int starts_with(char *p, int pl, char *string) {
  int l = strlen(string);
  return pl == l && memcmp(p, string, l) == 0;
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

    if (memcmp(p, "__", 2) == 0) {
      // skip __extension__
      if (memcmp(p, "__extension__", 13) == 0) {
        p += 13;
        continue;
      }
      // skip __attribute__ (*)
      if (memcmp(p, "__attribute__", 13) == 0) {
        p += 13;
        p = skip_brackets(p);
        continue;
      }
      // skip __restrict
      if (memcmp(p, "__restrict", 10) == 0) {
        p += 10;
        continue;
      }
      if (memcmp(p, "__inline", 8) == 0) {
        p += 8;
        continue;
      }
      if (memcmp(p, "__restrict", 10) == 0) {
        p += 10;
        continue;
      }
      if (memcmp(p, "__builtin_bswap32", 17) == 0) {
        p += 17;
        continue;
      }
      if (memcmp(p, "__builtin_bswap64", 17) == 0) {
        p += 17;
        continue;
      }
    }

    // character literal
    if (*p == '\'') {
      p++;

      cur = new_token(TK_CHAR, cur, p);

      if (*p == '\\') {
        cur->len = 4;
        cur->val = to_escape_char(*(++p));
      } else {
        cur->len = 3;
        cur->val = *p;
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
      char* p0 = ++p;
      while (*p) {
        if (!*p) error("EOF");
        p = next_ptr(p, '"');
        if (*(p - 1) != '\\') break;
        p++;
      }

      cur = new_token(TK_STR, cur, p0);
      cur->len = p - p0;

      p++;
      continue;
    }

    if (memcmp(p, "...", 3) == 0) {
      cur = new_token(TK_VA, cur, p);
      cur->len = 3;
      p += 3;
      continue;
    }
    if (memcmp(p, ">>=", 3) == 0 || memcmp(p, "<<=", 3) == 0) {
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

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    int l = lvar_len(p);
    if (l > 0) {
      if (starts_with(p, l, "static")) {
        p += l;
        continue;
      }

      if (starts_with(p, l, "continue")) {
        cur = new_token(TK_CONTINUE, cur, p);
      } else if (starts_with(p, l, "break")) {
        cur = new_token(TK_BREAK, cur, p);
      } else if (starts_with(p, l, "const")) {
        cur = new_token(TK_CONST, cur, p);
      } else if (starts_with(p, l, "enum")) {
        cur = new_token(TK_ENUM, cur, p);
      } else if (starts_with(p, l, "struct")) {
        cur = new_token(TK_STRUCT, cur, p);
      } else if (starts_with(p, l, "union")) {
        cur = new_token(TK_UNION, cur, p);
      } else if (starts_with(p, l, "extern")) {
        cur = new_token(TK_EXTERN, cur, p);
      } else if (starts_with(p, l, "sizeof")) {
        cur = new_token(TK_SIZEOF, cur, p);
      } else if (starts_with(p, l, "return")) {
        cur = new_token(TK_RETURN, cur, p);
      } else if (starts_with(p, l, "typedef")) {
        cur = new_token(TK_TYPEDEF, cur, p);
      } else if (starts_with(p, l, "if")) {
        cur = new_token(TK_IF, cur, p);
      } else if (starts_with(p, l, "else")) {
        cur = new_token(TK_ELSE, cur, p);
      } else if (starts_with(p, l, "while")) {
        cur = new_token(TK_WHILE, cur, p);
      } else if (starts_with(p, l, "for")) {
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
