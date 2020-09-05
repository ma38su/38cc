#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

char *next_ptr(char *p0, char c) {
  char *p = p0;
  while (*p != c) p++;
  return p;
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

Token *read_char_literal(Token *cur, char *p) {
  // character literal
  if (*p != '\'') return NULL;

  Token *tok = new_token(TK_CHAR, cur, ++p);
  if (*p == '\\') {
    tok->len = 2;
    tok->val = to_escape_char(*(++p));
  } else {
    tok->len = 1;
    tok->val = *p;
  }
  if (*(++p) != '\'') {
    error_at(tok->str, "unexpected token. expected token is \"'\"");
  }
  return tok;
}

Token *read_str_literal(Token *cur, char *p) {
  if (*p != '"') return NULL;

  char* p0 = ++p;
  while (*p) {
    p = next_ptr(p, '"');
    if (*(p - 1) != '\\') break;
    p++;
  }

  Token *tok = new_token(TK_STR, cur, p0);
  tok->len = p - p0;
  p++;

  return tok;
}

Token *tokenize() {
  char *p = user_input;

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
      if (starts_with(p, l, "static")
          || starts_with(p, l, "unsigned")
          || starts_with(p, l, "signed")) {
        p += l;
        continue;
      }
      if (memcmp(p, "__", 2) == 0) {
        if (starts_with(p, l, "__extension__")
            || starts_with(p, l, "__restrict")) {
          p += l;
          continue;
        }
        if (starts_with(p, l, "__inline")) {
          cur = new_token(TK_RESERVED, cur, p);
          cur->len = l;
          p += l;
          continue;
        }
      }

      // skip builtin
      if (starts_with(p, l, "__builtin_bswap32")
          || starts_with(p, l, "__builtin_bswap64")) {
        p += l;
        continue;
      }

      if (starts_with(p, l, "do") || starts_with(p, l, "while")
          || starts_with(p, l, "continue") || starts_with(p, l, "break")
          || starts_with(p, l, "for") || starts_with(p, l, "return")
          || starts_with(p, l, "if") || starts_with(p, l, "else")
          || starts_with(p, l, "struct") || starts_with(p, l, "enum")
          || starts_with(p, l, "union")
          || starts_with(p, l, "typedef") || starts_with(p, l, "const")
          || starts_with(p, l, "extern") || starts_with(p, l, "sizeof")) {
        cur = new_token(TK_RESERVED, cur, p);
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
