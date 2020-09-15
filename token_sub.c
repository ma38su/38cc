#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "38cc.h"


bool is_alpbar(char c);
bool is_alpbarn(char c);
int word_len(char *p0);
int starts_with(char *p, int pl, char *string);

Token *new_token(TokenKind kind, Token *cur, char *str);
char *next_ptr(char *p0, char c);
char to_escape_char(char v);
char *skip_brackets(char *p);
Token *read_char_literal(Token *cur, char *p);
Token *read_str_literal(Token *cur, char *p);

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

char *next_ptr(char *p0, char c) {
  char *p = p0;
  while (*p != c) p++;
  return p;
}

bool is_alpbar(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alpbarn(char c) {
  return is_alpbar(c) || ('0' <= c && c <= '9');
}

bool is_space(char p) {
  return p == ' ' || p == '\n' || p == '\t' || p == '\0' || p == '\r';
}

int word_len(char *p0) {
  char *p = p0;
  if (is_alpbar(*p)) {
    do {
      p++;
    } while (is_alpbarn(*p));
  }
  return p - p0;
}

char *skip_brackets(char *p) {
  // skip (*)
  while (is_space(*p)) ++p;

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

char *skip(char *p) {
  if (is_space(*p)) {
    p++;
    return p;
  }

  // skip line comment
  if (memcmp(p, "//", 2) == 0) {
    p += 2;
    p = next_ptr(p, '\n');
    p++;
    return p;
  }

  // skip block comment
  if (memcmp(p, "/*", 2) == 0) {
    p += 2;
    while (*p != '*' || *(p+1) != '/') {
      p++;
    }
    p += 2;
    return p;
  }

  // skip preprocessor
  if (*p == '#') {
    p++;
    p = next_ptr(p, '\n');
    p++;
    return p;
  }
  return NULL;
}

int starts_with(char *p, int pl, char *string) {
  int l = strlen(string);
  return pl == l && memcmp(p, string, l) == 0;
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

Token *read_literal(Token *cur, char *p) {
  Token *tok;

  tok = read_char_literal(cur, p);
  if (tok) return tok;

  tok = read_str_literal(cur, p);
  if (tok) return tok;

  return NULL;
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

