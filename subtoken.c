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

int is_alnum(char c) {
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

