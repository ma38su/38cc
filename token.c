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
Token *read_literal(Token *cur, char *p);
Token *consume_reserved_token(Token *cur, char *p);

char *next_ptr(char *p0, char c);
char to_escape_char(char v);
char *skip_brackets(char *p);
bool is_space(char p);
char *skip_brackets(char *p);
char *skip(char *p);


Token *consume_reserved_token(Token *cur, char *p) {
  if (memcmp(p, ">>=", 3) == 0 || memcmp(p, "<<=", 3) == 0
      || memcmp(p, "...", 3) == 0) {
    Token *tok = new_token(TK_RESERVED, cur, p);
    tok->len = 3;
    return tok;
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
    Token *tok = new_token(TK_RESERVED, cur, p);
    tok->len = 2;
    return tok;
  }

  if (strchr("=,.(){}[]<>+-*/%^;&^|~!?:", *p)) {
    Token *tok = new_token(TK_RESERVED, cur, p);
    tok->len = 1;
    return tok;
  }
  return NULL;
}

Token *tokenize(char *p) {

  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // skip blank
    char* p1 = skip(p);
    if (p1) {
      p = p1;
      continue;
    }

    Token *tok;
    tok = read_literal(cur, p);
    if (tok) {
      p += tok->len + 2;
      cur = tok;
      continue;
    }

    tok = consume_reserved_token(cur, p);
    if (tok) {
      p += tok->len;
      cur = tok;
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

    int l = word_len(p);

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
